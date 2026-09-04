#include "render.h"
#include <stdint.h>
#include <string.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <psxgte.h>
#define OT_SIZE 1024
#define PACKET_SIZE 65536
/* Statically allocated, aligned GPU packets: no per-frame heap allocation. */
typedef struct {DRAWENV draw;DISPENV disp;uint32_t ot[OT_SIZE];uint32_t packet[PACKET_SIZE/4];} Buffer;
static Buffer buffers[2];static int active;static uint8_t*next;
static int center_x,center_z,camera=2100;
static int yaw,pitch=250,yaw_sin,yaw_cos=4096,pitch_sin,pitch_cos;
static int shake,shake_strength=2,old_hp=200;
typedef struct {int x,y,z;} V;
typedef struct {int x,y,z;} Screen;
typedef struct {int r,g,b;} Color;
typedef struct {int initialized,crouch,roll,roll_sin,roll_cos,clock,upper[2],lower[2],thigh[2],shin[2];} MotionPose;
static MotionPose motion[2],*current_pose;
static int blend(int a,int b){int d=b-a;return a+(d*3+(d>0?3:-3))/4;}
static int ease(int t){if(t<0)t=0;if(t>1024)t=1024;return t*t*(3072-2*t)/1048576;}

static Color color(int r,int g,int b){Color c={r,g,b};return c;}
static V v(int x,int y,int z){V p={x,y,z};return p;}
static void*alloc(int size){
 uint8_t*p=next;if(next+size>(uint8_t*)buffers[active].packet+PACKET_SIZE)return 0;next+=size;return p;
}
static Screen project(V p){
 Screen s;int x=p.x-center_x,z=p.z-center_z;
 int rx=(x*yaw_cos+z*yaw_sin)/4096;
 z=(-x*yaw_sin+z*yaw_cos)/4096;
 s.z=camera+(z*pitch_cos-p.y*pitch_sin)/4096;
 if(s.z<160)s.z=160;
 s.x=160+rx*420/s.z+(shake?((shake&1)?1:-1)*shake*shake_strength/8:0);
 s.y=184+((-z*pitch_sin-p.y*pitch_cos)/4096)*420/s.z;
 return s;
}
static void screen_quad(Screen a,Screen b,Screen c,Screen d,Color col){
 Screen p[4]={a,b,c,d};
 int z=(p[0].z+p[1].z+p[2].z+p[3].z)/16;
 POLY_F4*q;
 if(z<2)z=2;
 if(z>=OT_SIZE)z=OT_SIZE-1;
 q=alloc(sizeof(*q));if(!q)return;
 setPolyF4(q);setRGB0(q,col.r,col.g,col.b);
 setXY4(q,p[0].x,p[0].y,p[1].x,p[1].y,p[2].x,p[2].y,p[3].x,p[3].y);
 addPrim(&buffers[active].ot[z],q);
}
static void quad(V a,V b,V c,V d,Color col){
 screen_quad(project(a),project(b),project(c),project(d),col);
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
 if(current_pose&&current_pose->roll){
  int angle=current_pose->roll,cs=current_pose->roll_cos,sn=current_pose->roll_sin,u=p.x,h=p.y-120;
  p.x=(u*cs+h*sn)/4096;p.y=120+(h*cs-u*sn)/4096-angle*80/1024;
 }
 return v(f->x+(p.x*f->dx-p.z*f->dz)/1024,f->y+p.y,f->z+(p.x*f->dz+p.z*f->dx)/1024);
}
static void screen_triangle(Screen a,Screen b,Screen c,Color col){
 if((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)>=0)return;
 Screen p[3]={a,b,c};
 int z=(p[0].z+p[1].z+p[2].z)/12;
 POLY_F3*q;
 if(z<2)z=2;
 if(z>=OT_SIZE)z=OT_SIZE-1;
 q=alloc(sizeof(*q));if(!q)return;
 setPolyF3(q);setRGB0(q,col.r,col.g,col.b);
 setXY3(q,p[0].x,p[0].y,p[1].x,p[1].y,p[2].x,p[2].y);
 addPrim(&buffers[active].ot[z],q);
}
/* Eight-sided tapered solid: 16 vertices, 8 quads + 12 cap triangles.
   Each face receives its own constant light intensity (flat shading). */
static void tapered(const Fighter*f,V a,V b,int wa,int da,int wb,int db,Color col){
 static const int ring[8][2]={{1024,0},{724,724},{0,1024},{-724,724},
  {-1024,0},{-724,-724},{0,-1024},{724,-724}};
 static const int light[8]={108,100,82,65,55,65,82,100};
 Screen p[16];int i,end,dy=b.y-a.y,dx=b.x-a.x;
 int len=(dx<0?-dx:dx)+(dy<0?-dy:dy);
 if(!len)len=1;
 for(end=0;end<2;end++){
  V point=end?b:a;int width=end?wb:wa,depth=end?db:da;
  for(i=0;i<8;i++){
   int offset=ring[i][0]*width/1024;
   p[end*8+i]=project(world(f,v(point.x+dy*offset/len,point.y-dx*offset/len,
    point.z+ring[i][1]*depth/1024)));
  }
 }
 for(i=0;i<8;i++){
  int j=(i+1)%8,l=light[i];
  if((p[j].x-p[i].x)*(p[i+8].y-p[i].y)-(p[j].y-p[i].y)*(p[i+8].x-p[i].x)>=0)continue;
  screen_quad(p[i],p[j],p[i+8],p[j+8],color(col.r*l/100,col.g*l/100,col.b*l/100));
 }
 for(i=1;i<7;i++){
  screen_triangle(p[0],p[i+1],p[i],color(col.r*6/10,col.g*6/10,col.b*6/10));
  screen_triangle(p[8],p[8+i],p[9+i],col);
 }
}
static void limb(const Fighter*f,V a,V b,int width,int depth,Color col){
 tapered(f,a,b,width,depth,width*4/5,depth*4/5,col);
}
static void fighter(const Fighter*f,int frame,int player,int frozen,int reset){
 MotionPose*pose=&motion[player];int crouch,phase=frame%32,wave=(phase<16?phase:32-phase)-8;
 current_pose=pose;
 if(reset||!pose->initialized){
  int j;memset(pose,0,sizeof(*pose));pose->initialized=1;
  for(j=0;j<2;j++){pose->upper[j]=-700;pose->lower[j]=480;pose->thigh[j]=-900;pose->shin[j]=-1148;}
 }
 if(!frozen){pose->crouch=blend(pose->crouch,f->crouch?85:0);pose->roll=blend(pose->roll,f->action==DOWN?1024:0);}
 pose->roll_sin=isin(pose->roll);pose->roll_cos=icos(pose->roll);
 if(!frozen)pose->clock=frame;
 phase=pose->clock%32;wave=(phase<16?phase:32-phase)-8;
 crouch=pose->crouch;
 int stride=f->walk?wave*6:0,bob=f->walk?(wave<0?-wave:wave):0;
 int extend=0,kick=0,low=0;
 Color skin=color(198,143,102),cloth=f->character?color(34,126,155):color(211,204,183);
 Color wraps=f->character?color(30,75,89):color(172,39,31);
 Color dark=color(30,35,48),top=f->character?color(40,55,74):color(207,203,180);
 V hip=v(0,205-crouch,0),neck=v(8,344-crouch+bob,0);
 V shoulder[2],elbow[2],hand[2],knee[2],foot[2];int i;
 if(f->action>=PUNCH&&f->action<=SHOULDER){
  const Move*m=&game_moves[f->action];int t=f->tick;
  extend=t<=m->startup?ease(t*1024/m->startup):t<m->startup+m->active?1024:
   1024-ease((t-m->startup-m->active)*1024/m->recovery);
  if(extend<0)extend=0;
  if(f->action==KICK||f->action==LOW_KICK||f->action==LAUNCH_KICK){kick=extend;extend=0;low=f->action==LOW_KICK;}
 }
 if(f->action==SHOULDER){neck.x+=extend*65/1024;neck.y-=extend*25/1024;}
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
 if(f->action==SHOULDER){
  shoulder[0].x+=extend*75/1024;shoulder[0].y-=extend*30/1024;
  elbow[0].x=30;elbow[0].y=250;hand[0].x=50;hand[0].y=275;
  foot[0].x+=extend*60/1024;foot[1].x-=extend*45/1024;
  extend=0;
 }
 /* Fixed bone lengths. The fist follows elbow rotation, never stretches. */
 for(i=0;i<2;i++){
  int upper=-700,lower=480;
  if(f->guard){upper=-500;lower=900;}
  else if(f->action==SHOULDER){upper=-900;lower=900;}
  else if((i==0||f->action==THROW)&&extend){
   upper+=extend*700/1024;lower-=extend*480/1024;
  }
  if(!frozen){pose->upper[i]=blend(pose->upper[i],upper);pose->lower[i]=blend(pose->lower[i],lower);}
  upper=pose->upper[i];lower=pose->lower[i];
  elbow[i]=v(shoulder[i].x+icos(upper)*70/4096,
   shoulder[i].y+isin(upper)*70/4096,shoulder[i].z);
  hand[i]=v(elbow[i].x+icos(lower)*65/4096,
   elbow[i].y+isin(lower)*65/4096,elbow[i].z);
 }
 /* Interpolate leg angles as well: fixed 100/95-unit bones. */
 for(i=0;i<2;i++){
  int side=i?1:-1,thigh=-900+crouch*520/85,shin=-1148-crouch*552/85;
  if(f->walk){thigh+=side*wave*20;shin-=side*wave*12;}
  if(f->action==BACKSTEP){thigh+=side*isin(f->tick*180)*180/4096;}
  if(f->action==JUMP){thigh=-500;shin=-1600;}
  if(i==0&&kick){thigh+=((low?-600:300)-thigh)*kick/1024;shin+=((low?-350:150)-shin)*kick/1024;}
  if(!frozen){pose->thigh[i]=blend(pose->thigh[i],thigh);pose->shin[i]=blend(pose->shin[i],shin);}
  thigh=pose->thigh[i];shin=pose->shin[i];
  knee[i]=v(hip.x+icos(thigh)*100/4096,hip.y+isin(thigh)*100/4096,side*38);
  foot[i]=v(knee[i].x+icos(shin)*95/4096,knee[i].y+isin(shin)*95/4096,side*38);
 }
 /* Hip -> waist -> rib cage -> shoulders: an actual torso silhouette. */
 tapered(f,hip,v(2,245-crouch,0),34,49,28,40,top);
 tapered(f,v(2,245-crouch,0),v(6,302-crouch+bob,0),28,40,
  f->character?47:38,f->character?70:61,top);
 tapered(f,v(6,302-crouch+bob,0),neck,f->character?47:38,
  f->character?70:61,26,40,top);
 limb(f,v(0,190-crouch,0),v(0,216-crouch,0),39,62,dark);
 for(i=0;i<2;i++){
  tapered(f,v(hip.x,hip.y,i?38:-38),knee[i],33,30,23,23,cloth);
  tapered(f,knee[i],foot[i],25,24,16,18,cloth);
  limb(f,v(foot[i].x-10,foot[i].y,foot[i].z),v(foot[i].x+38,foot[i].y+2,foot[i].z),14,25,dark);
  tapered(f,shoulder[i],elbow[i],25,25,17,18,skin);tapered(f,elbow[i],hand[i],19,20,12,13,skin);
  limb(f,hand[i],v(hand[i].x+18,hand[i].y,hand[i].z),17,18,skin);
  limb(f,v(hand[i].x-7,hand[i].y,hand[i].z),hand[i],16,18,wraps);
 }
 limb(f,v(neck.x,neck.y,0),v(neck.x,neck.y+20,0),17,21,skin);
 tapered(f,v(neck.x+7,neck.y+19,0),v(neck.x+4,neck.y+69,0),23,24,32,33,skin);
 limb(f,v(neck.x+2,neck.y+66,0),v(neck.x+2,neck.y+83,0),33,35,dark);
 /* Angular nose and two brows give the face a readable forward profile. */
 quad(world(f,v(neck.x+37,neck.y+57,-6)),world(f,v(neck.x+48,neck.y+43,-4)),
  world(f,v(neck.x+37,neck.y+57,6)),world(f,v(neck.x+48,neck.y+43,4)),color(221,166,119));
 for(i=0;i<2;i++){
  int side=i?1:-1;
  quad(world(f,v(neck.x+35,neck.y+60,side*9)),world(f,v(neck.x+35,neck.y+60,side*24)),
   world(f,v(neck.x+36,neck.y+55,side*9)),world(f,v(neck.x+36,neck.y+55,side*24)),dark);
 }
 if(!f->character){
  /* Crossed gi lapels, a red headband, and two black belt ends. */
  quad(world(f,v(34,331-crouch,-29)),world(f,v(37,322-crouch,-38)),
   world(f,v(35,269-crouch,26)),world(f,v(35,257-crouch,17)),color(244,234,205));
  quad(world(f,v(35,331-crouch,29)),world(f,v(37,322-crouch,38)),
   world(f,v(35,269-crouch,-26)),world(f,v(35,257-crouch,-17)),color(162,153,133));
  limb(f,v(neck.x+3,neck.y+64,0),v(neck.x+3,neck.y+71,0),34,35,wraps);
  limb(f,v(36,201-crouch,-8),v(45,155-crouch,-16),5,7,dark);
  limb(f,v(36,201-crouch,8),v(50,161-crouch,20),5,7,dark);
 }
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
static int heading(int x,int z){
 int ax=x<0?-x:x,az=z<0?-z:z;
 int angle=(ax+az)?az*1024/(ax+az):0;
 if(x<0)angle=2048-angle;
 return z<0?(-angle)&4095:angle;
}
void render_begin(const Game*g,int frame,int presentation){
 int d=game_distance(&g->f[0],&g->f[1]);
 int tx=(g->f[0].x+g->f[1].x)/2,tz=(g->f[0].z+g->f[1].z)/2;
 int angle=heading(g->f[1].x-g->f[0].x,g->f[1].z-g->f[0].z);
 int zoom=1600+(d>500?(d-500)*3/4:0),tilt=220,delta,hp;
 next=(uint8_t*)buffers[active].packet;ClearOTagR(buffers[active].ot,OT_SIZE);
 if(presentation==0){angle=isin(frame*2)*400/4096;zoom=1900;tilt=280;}
 else if(presentation==1){angle=isin(frame)*180/4096;zoom=1700;tilt=190;}
 else if(g->phase==INTRO){angle+=(INTRO_TICKS-g->tick)*4;zoom+= (INTRO_TICKS-g->tick)*5;tilt=220+(INTRO_TICKS-g->tick);}
 else if((g->phase==ROUND_OVER||g->phase==MATCH_OVER)&&g->winner>=0){
  const Fighter*w=&g->f[g->winner];
  tx=(tx+w->x*3)/4;tz=(tz+w->z*3)/4;zoom=1400;angle+=280;tilt=160;
 }
 delta=((angle-yaw+2048)&4095)-2048;yaw=(yaw+delta/12)&4095;
 center_x+=(tx-center_x)/10;center_z+=(tz-center_z)/10;
 camera+=(zoom-camera)/12;pitch+=(tilt-pitch)/12;
 yaw_sin=isin(yaw);yaw_cos=icos(yaw);pitch_sin=isin(pitch);pitch_cos=icos(pitch);
 hp=g->f[0].hp+g->f[1].hp;
 if(presentation==2&&hp<old_hp){shake=8;shake_strength=old_hp-hp>=18?5:2;}else if(shake)shake--;
 old_hp=hp;
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
  fighter(f,frame,i,g->hitstop>0,g->phase==INTRO&&g->tick<2);
 }
}
static void hit_effect(const Game*g,int player){
 int life=g->effect_life[player],i,age=14-life;
 Screen p;
 if(life<=0)return;
 p=project(v(g->effect_x[player],g->effect_y[player],g->effect_z[player]));
 for(i=0;i<8;i++){
  int angle=i*512+age*45,radius=4+age*2;
  int x=p.x+icos(angle)*radius/4096,y=p.y+isin(angle)*radius/4096;
  LINE_F2*line=alloc(sizeof(*line));
  if(!line)return;
  setLineF2(line);setXY2(line,x,y,x+icos(angle)*life/4096,y+isin(angle)*life/4096);
  setRGB0(line,g->effect_block[player]?100:245,g->effect_block[player]?210:170,life*16);
  addPrim(&buffers[active].ot[1],line);
 }
 if(life>10){
  TILE*t=alloc(sizeof(*t));if(!t)return;setTile(t);setXY0(t,p.x-2,p.y-2);
  setWH(t,5,5);setRGB0(t,255,248,220);addPrim(&buffers[active].ot[1],t);
 }
}
void render_effects(const Game*g){hit_effect(g,0);hit_effect(g,1);}
void render_end(void){
 DrawSync(0);VSync(0);PutDispEnv(&buffers[active^1].disp);
 DrawOTagEnv(&buffers[active].ot[OT_SIZE-1],&buffers[active].draw);active^=1;
}
