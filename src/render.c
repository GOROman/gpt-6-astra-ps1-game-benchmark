#include "render.h"
#include <stdint.h>
#include <string.h>
#include <psxgpu.h>
#include <psxetc.h>
#define OT_SIZE 1024
#define PACKET_SIZE 65536
/* Statically allocated, aligned GPU packets: no per-frame heap allocation. */
typedef struct {DRAWENV draw;DISPENV disp;uint32_t ot[OT_SIZE];uint32_t packet[PACKET_SIZE/4];} Buffer;
static Buffer buffers[2];static int active;static uint8_t*next;
static int center_x,center_z,camera;
typedef struct {int x,y,z;} V;
typedef struct {int x,y,z;} Screen;
typedef struct {int r,g,b;} Color;
static Color color(int r,int g,int b){Color c={r,g,b};return c;}
static V v(int x,int y,int z){V p={x,y,z};return p;}
static void*alloc(int size){
 uint8_t*p=next;if(next+size>(uint8_t*)buffers[active].packet+PACKET_SIZE)return 0;next+=size;return p;
}
static Screen project(V p){
 Screen s;int x=p.x-center_x,z=p.z-center_z;
 /* Fixed 22-degree downward camera. Positive world Y is up. */
 s.z=camera+(z*950-p.y*380)/1024;
 if(s.z<160)s.z=160;
 s.x=160+x*420/s.z;s.y=153+(z*380-p.y*950)*420/1024/s.z;
 return s;
}
static void quad(V a,V b,V c,V d,Color col){
 Screen p[4]={project(a),project(b),project(c),project(d)};
 int z=(p[0].z+p[1].z+p[2].z+p[3].z)/16;
 POLY_F4*q;
 if(z<2)z=2;if(z>=OT_SIZE)z=OT_SIZE-1;
 q=alloc(sizeof(*q));if(!q)return;
 setPolyF4(q);setRGB0(q,col.r,col.g,col.b);
 setXY4(q,p[0].x,p[0].y,p[1].x,p[1].y,p[2].x,p[2].y,p[3].x,p[3].y);
 addPrim(&buffers[active].ot[z],q);
}
void render_rect(int x,int y,int w,int h,int r,int g,int b){
 TILE*t=alloc(sizeof(*t));if(!t)return;setTile(t);setXY0(t,x,y);setWH(t,w,h);setRGB0(t,r,g,b);addPrim(&buffers[active].ot[0],t);
}
void render_text(int x,int y,const char*text){
 /* SDK font packets need <= 16 bytes per character plus a texture page. */
 if(next+strlen(text)*20+32>(uint8_t*)buffers[active].packet+PACKET_SIZE)return;
 next=(uint8_t*)FntSort(&buffers[active].ot[0],next,x,y,text);
}
void render_center(int y,const char*text){render_text(160-(int)strlen(text)*4,y,text);}
static V world(const Fighter*f,V p){
 if(f->action==DOWN){int t=p.x;p.x=p.y-120;p.y=35-t/4;}
 return v(f->x+(p.x*f->dx-p.z*f->dz)/1024,f->y+p.y,f->z+(p.x*f->dz+p.z*f->dx)/1024);
}
static void limb(const Fighter*f,V a,V b,int width,int depth,Color col){
 V p[8];int i,dy=b.y-a.y,dx=b.x-a.x;
 int len=(dx<0?-dx:dx)+(dy<0?-dy:dy);int ox,oy;
 if(!len)len=1;ox=dy*width/len;oy=-dx*width/len;
 for(i=0;i<8;i++){
  V end=i&4?b:a;int s=i&1?1:-1,t=i&2?1:-1;
  p[i]=world(f,v(end.x+s*ox,end.y+s*oy,end.z+t*depth));
 }
 quad(p[0],p[1],p[2],p[3],color(col.r*6/10,col.g*6/10,col.b*6/10));
 quad(p[4],p[6],p[5],p[7],col);
 quad(p[0],p[4],p[1],p[5],color(col.r*7/10,col.g*7/10,col.b*7/10));
 quad(p[2],p[3],p[6],p[7],color(col.r*9/10,col.g*9/10,col.b*9/10));
 quad(p[0],p[2],p[4],p[6],color(col.r*8/10,col.g*8/10,col.b*8/10));
 quad(p[1],p[5],p[3],p[7],color(col.r*11/10,col.g*11/10,col.b*11/10));
}
static void fighter(const Fighter*f,int frame){
 int crouch=f->crouch?85:0,phase=frame%32,wave=(phase<16?phase:32-phase)-8;
 int stride=f->walk?wave*6:0,bob=f->walk?(wave<0?-wave:wave):0;
 int extend=0,kick=0,low=0;
 Color skin=color(198,143,102),cloth=f->character?color(34,126,155):color(204,63,43);
 Color dark=color(30,35,48),top=f->character?color(40,55,74):color(207,203,180);
 V hip=v(0,205-crouch,0),neck=v(8,344-crouch+bob,0);
 V shoulder[2],elbow[2],hand[2],knee[2],foot[2];int i;
 if(f->action>=PUNCH&&f->action<=THROW){
  const Move*m=&game_moves[f->action];int t=f->tick;
  extend=t<=m->startup?t*1024/m->startup:1024-(t-m->startup)*1024/(m->active+m->recovery);
  if(extend<0)extend=0;
  if(f->action==KICK||f->action==LOW_KICK||f->action==LAUNCH_KICK){kick=extend;extend=0;low=f->action==LOW_KICK;}
 }
 if(f->action==HURT){neck.x=-35;}
 for(i=0;i<2;i++){
  int side=i?1:-1;
  shoulder[i]=v(8,322-crouch+bob,side*62);
  elbow[i]=v(40,265-crouch,side*84);
  hand[i]=v(85,307-crouch,side*66);
  knee[i]=v(side*stride+24,108-crouch/2,side*44);
  foot[i]=v(side*stride+side*34,15,side*58);
  if(f->guard){elbow[i].x=65;elbow[i].y+=20;hand[i].x=65;hand[i].y=353-crouch;hand[i].z=side*28;}
 }
 hand[0].x+=extend*150/1024;hand[0].y+=extend*10/1024;elbow[0].x+=extend*90/1024;
 knee[0].x+=kick*110/1024;knee[0].y+=kick*(low?0:110)/1024;
 foot[0].x+=kick*265/1024;foot[0].y+=kick*(low?40:270)/1024;
 if(f->action==JUMP){knee[0].y+=55;knee[1].y+=55;foot[0].y+=65;foot[1].y+=65;}
 limb(f,hip,neck,f->character?44:35,59,top);
 limb(f,v(0,190-crouch,0),v(0,216-crouch,0),39,62,dark);
 for(i=0;i<2;i++){
  limb(f,v(hip.x,hip.y,i?38:-38),knee[i],29,28,cloth);
  limb(f,knee[i],foot[i],22,23,cloth);
  limb(f,v(foot[i].x-10,foot[i].y,foot[i].z),v(foot[i].x+38,foot[i].y+2,foot[i].z),14,25,dark);
  limb(f,shoulder[i],elbow[i],21,23,skin);limb(f,elbow[i],hand[i],17,19,skin);
  limb(f,hand[i],v(hand[i].x+21,hand[i].y,hand[i].z),20,21,cloth);
 }
 limb(f,v(neck.x,neck.y,0),v(neck.x,neck.y+20,0),17,21,skin);
 limb(f,v(neck.x+6,neck.y+20,0),v(neck.x+6,neck.y+73,0),31,33,skin);
 limb(f,v(neck.x+2,neck.y+66,0),v(neck.x+2,neck.y+83,0),33,35,dark);
 /* Dark brows across the forward plane identify face orientation. */
 limb(f,v(neck.x+39,neck.y+54,-23),v(neck.x+39,neck.y+59,23),2,4,dark);
}
void render_init(void){int i;
 ResetGraph(0);SetVideoMode(0);FntLoad(960,0);
 for(i=0;i<2;i++){
  SetDefDrawEnv(&buffers[i].draw,0,i*240,320,240);
  SetDefDispEnv(&buffers[i].disp,0,i*240,320,240);
  buffers[i].draw.isbg=1;setRGB0(&buffers[i].draw,10,16,31);
 }
 active=0;SetDispMask(1);
}
void render_begin(const Game*g,int frame){
 int d=game_distance(&g->f[0],&g->f[1]);(void)frame;
 next=(uint8_t*)buffers[active].packet;ClearOTagR(buffers[active].ot,OT_SIZE);
 center_x=(g->f[0].x+g->f[1].x)/4;center_z=(g->f[0].z+g->f[1].z)/4;
 camera=1850+(d>600?(d-600)/2:0);
}
void render_scene(const Game*g,int frame){int x,z,i;
 /* Individual flat tiles make perspective and foot movement readable. */
 for(z=-800;z<800;z+=200)for(x=-800;x<800;x+=200){
  int shade=((x+z)/200)&1;
  quad(v(x,0,z),v(x+200,0,z),v(x,0,z+200),v(x+200,0,z+200),shade?color(73,87,99):color(84,100,112));
 }
 quad(v(-800,0,-800),v(800,0,-800),v(-800,-85,-800),v(800,-85,-800),color(29,45,61));
 quad(v(-800,0,800),v(-800,0,-800),v(-800,-85,800),v(-800,-85,-800),color(25,36,50));
 quad(v(800,0,-800),v(800,0,800),v(800,-85,-800),v(800,-85,800),color(25,36,50));
 for(i=0;i<2;i++){
  int p=i?770:-800;
  quad(v(-800,2,p),v(800,2,p),v(-800,2,p+30),v(800,2,p+30),color(212,149,51));
  quad(v(p,2,-800),v(p+30,2,-800),v(p,2,800),v(p+30,2,800),color(212,149,51));
 }
 for(i=0;i<2;i++){
  const Fighter*f=&g->f[i];
  quad(v(f->x-65,3,f->z-55),v(f->x+65,3,f->z-55),v(f->x-65,3,f->z+55),v(f->x+65,3,f->z+55),color(30,40,48));
  fighter(f,frame);
 }
}
void render_end(void){
 DrawSync(0);VSync(0);PutDispEnv(&buffers[active^1].disp);
 DrawOTagEnv(&buffers[active].ot[OT_SIZE-1],&buffers[active].draw);active^=1;
}
