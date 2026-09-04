#include "game.h"
#include <string.h>

const Move game_moves[7]={
 {0,0,0,0,0,0,0,HIGH},
 {5,3,8,9,185,18,12,HIGH},
 {11,3,16,18,265,45,20,MID},
 {10,3,17,12,240,30,17,LOW},
 {18,4,24,26,240,100,40,MID},
 {8,1,24,22,145,125,40,GRAB},
 {10,4,25,24,175,160,40,MID}
};
static int absolute(int n){return n<0?-n:n;}
static int root(unsigned n){
 unsigned r=0,bit=1u<<30;
 while(bit>n)bit>>=2;
 while(bit){if(n>=r+bit){n-=r+bit;r=(r>>1)+bit;}else r>>=1;bit>>=2;}
 return (int)r;
}
int game_distance(const Fighter*a,const Fighter*b){
 int x=a->x-b->x,z=a->z-b->z; return root((unsigned)(x*x+z*z));
}
const char *game_character_name(int c){return c?"BASALT":"EMBER";}
static void face(Fighter*a,const Fighter*b){
 int d=game_distance(a,b); if(d){a->dx=(b->x-a->x)*1024/d;a->dz=(b->z-a->z)*1024/d;}
}
static void reset_round(Game*g){
 int i; for(i=0;i<2;i++){
  Fighter*f=&g->f[i]; int c=f->character,w=f->wins;
  memset(f,0,sizeof(*f)); f->character=c;f->wins=w;f->hp=MAX_HP;
  f->x=i?240:-240; f->dx=i?-1024:1024;
 }
 g->phase=INTRO;g->tick=0;g->remaining=ROUND_TICKS;g->reason=NONE;g->winner=-1;
}
void game_init(Game*g,int c1,int c2,uint32_t seed){
 memset(g,0,sizeof(*g));g->f[0].character=!!c1;g->f[1].character=!!c2;
 g->rng=seed?seed:1;g->round=1;reset_round(g);
}
static void begin(Fighter*f,int action){f->action=action;f->tick=0;f->landed=0;f->guard=0;}
static void advance(Fighter*f,Fighter*other,uint16_t in){
 unsigned pressed=in&~f->previous;f->previous=in;f->walk=0;
 f->x+=f->vx;f->z+=f->vz;f->vx=f->vx*3/4;f->vz=f->vz*3/4;
 if(f->stun){
  f->stun--;f->tick++;f->guard=0;f->crouch=0;
  if(!f->stun){f->action=IDLE;f->tick=0;}return;
 }
 if(f->action==JUMP){
  f->tick++;f->y=f->tick*(36-f->tick)/2;
  if(f->tick>=36){f->y=0;f->action=IDLE;f->tick=0;}return;
 }
 if(f->action!=IDLE){
  const Move*m=&game_moves[f->action];
  if(f->action==SHOULDER&&f->tick<=m->startup){
   f->x+=f->dx*7/1024;f->z+=f->dz*7/1024;
  }
  f->tick++; if(f->tick>=m->startup+m->active+m->recovery){f->action=IDLE;f->tick=0;}
  return;
 }
 face(f,other);f->crouch=!!(in&IN_DOWN);f->guard=!!(in&IN_GUARD);
 if(pressed&IN_SHOULDER)begin(f,SHOULDER);
 else if((in&(IN_PUNCH|IN_GUARD))==(IN_PUNCH|IN_GUARD) && (pressed&(IN_PUNCH|IN_GUARD)))begin(f,THROW);
 else if(pressed&IN_KICK)begin(f,f->crouch?LOW_KICK:((in&IN_PUNCH)?LAUNCH_KICK:KICK));
 else if(pressed&IN_PUNCH)begin(f,PUNCH);
 else if(pressed&IN_JUMP)begin(f,JUMP);
 else if(!f->guard){
  int speed=f->crouch?2:(f->character?4:5);
  int x=!!(in&IN_RIGHT)-!!(in&IN_LEFT);
  int side=!!(in&IN_STEP_R)-!!(in&IN_STEP_L);
  /* Left/right are screen/world X; shoulders circle the opponent. */
  f->x+=x*speed-side*f->dz*speed/1024;
  f->z+=side*f->dx*speed/1024;
  f->walk=!!(x||side);
 }
}
typedef struct {int hit,damage,push,stun,blocked,knock,dx,dz;} Contact;
static Contact contact(Fighter*a,Fighter*b){
 Contact c={0};const Move*m;int d,dot;
 if(a->action<PUNCH||a->action>SHOULDER||a->landed)return c;
 m=&game_moves[a->action];
 if(a->tick<m->startup||a->tick>=m->startup+m->active)return c;
 d=game_distance(a,b);if(d>m->reach)return c;
 dot=(b->x-a->x)*a->dx+(b->z-a->z)*a->dz;
 if(dot<d*600)return c;
 if(b->action==DOWN)return c;
 if(m->level==HIGH&&(b->crouch||b->y>65))return c;
 if(m->level==LOW&&b->y>25)return c;
 if(m->level==GRAB&&(b->y>0||b->crouch||b->stun||b->action!=IDLE))return c;
 c.hit=1;c.dx=a->dx;c.dz=a->dz;
 c.blocked=b->guard && ((m->level==LOW&&b->crouch)||((m->level==HIGH||m->level==MID)&&!b->crouch));
 c.damage=c.blocked?0:m->damage+(a->character?2:0);
 c.push=c.blocked?m->push/3:m->push;
 c.stun=c.blocked?8:m->stun;
 c.knock=!c.blocked&&(a->action==LAUNCH_KICK||a->action==THROW||a->action==SHOULDER);
 a->landed=1;return c;
}
static void apply(Game*g,Fighter*b,Contact c){
 if(!c.hit)return;
 b->hp-=c.damage;if(b->hp<0)b->hp=0;
 b->vx=c.dx*c.push/4096;b->vz=c.dz*c.push/4096;
 b->stun=c.stun;b->tick=0;b->action=c.knock?DOWN:HURT;
 b->y=0;b->guard=0;b->crouch=0;
 g->events|=c.blocked?EV_BLOCK:EV_HIT;
}
static void finish(Game*g,int winner,int reason){
 g->winner=winner;g->reason=reason;g->phase=ROUND_OVER;g->tick=0;
 if(winner>=0)g->f[winner].wins++;
 g->events|=EV_ROUND;
}
void game_tick(Game*g,uint16_t p1,uint16_t p2){
 Contact a,b;int out0,out1,i;g->events=0;g->tick++;
 if(g->phase==MATCH_OVER)return;
 if(g->phase==INTRO){
  g->f[0].previous=p1;g->f[1].previous=p2;
  if(g->tick>=INTRO_TICKS){g->phase=FIGHT;g->tick=0;}return;
 }
 if(g->phase==ROUND_OVER){
  if(g->tick>=RESULT_TICKS){
   if(g->f[0].wins>=2||g->f[1].wins>=2){g->phase=MATCH_OVER;g->tick=0;}
   else{g->round++;reset_round(g);}
  }return;
 }
 advance(&g->f[0],&g->f[1],p1);advance(&g->f[1],&g->f[0],p2);
 for(i=0;i<2;i++)if(g->f[i].action>=PUNCH&&g->f[i].action<=SHOULDER&&!g->f[i].tick)g->events|=EV_SWING;
 /* Evaluate both contacts before applying either: same-frame strikes trade. */
 a=contact(&g->f[0],&g->f[1]);b=contact(&g->f[1],&g->f[0]);
 if((a.hit&&g->f[0].action==THROW)||(b.hit&&g->f[1].action==THROW))g->events|=EV_THROW;
 apply(g,&g->f[1],a);apply(g,&g->f[0],b);
 /* A small collision cylinder prevents walking through an opponent. */
 {int d=game_distance(&g->f[0],&g->f[1]);
  if(d<90){
   int dx=g->f[1].x-g->f[0].x,dz=g->f[1].z-g->f[0].z,shift=(90-d+1)/2;
   if(!d){dx=1;dz=0;d=1;}
   dx=dx*shift/d;dz=dz*shift/d;
   g->f[0].x-=dx;g->f[0].z-=dz;g->f[1].x+=dx;g->f[1].z+=dz;
  }
 }
 out0=absolute(g->f[0].x)>RING_EDGE||absolute(g->f[0].z)>RING_EDGE;
 out1=absolute(g->f[1].x)>RING_EDGE||absolute(g->f[1].z)>RING_EDGE;
 if(out0||out1){finish(g,out0&&out1?-1:(out0?1:0),out0&&out1?DRAW:RING_OUT);return;}
 if(!g->f[0].hp||!g->f[1].hp){finish(g,!g->f[0].hp&&!g->f[1].hp?-1:(!g->f[0].hp?1:0),!g->f[0].hp&&!g->f[1].hp?DRAW:KO);return;}
 if(--g->remaining<=0){
  int h0=g->f[0].hp,h1=g->f[1].hp;finish(g,h0==h1?-1:(h0>h1?0:1),h0==h1?DRAW:TIME_UP);
 }
}
uint16_t game_cpu(Game*g,int player,int difficulty){
 Fighter*a=&g->f[player],*b=&g->f[player^1];int d=game_distance(a,b);unsigned r;
 g->rng^=g->rng<<13;g->rng^=g->rng>>17;g->rng^=g->rng<<5;r=g->rng;
 if(g->phase!=FIGHT)return 0;
 if(absolute(a->x)>640||absolute(a->z)>640){
  int side=(-a->z*a->dx+a->x*a->dz)>0;
  return (a->x>0?IN_LEFT:IN_RIGHT)|(side?IN_STEP_R:IN_STEP_L);
 }
 if(b->action>=PUNCH&&b->action<=SHOULDER && (int)(r%100)<35+difficulty*15)
  return IN_GUARD|(b->action==LOW_KICK?IN_DOWN:0);
 if(d>220)return b->x>a->x?IN_RIGHT:IN_LEFT;
 if(r%19==0)return IN_SHOULDER;
 if(r%13==0)return IN_GUARD|IN_PUNCH;
 if(r%11==0)return IN_DOWN|IN_KICK;
 if(r%17==0)return IN_PUNCH|IN_KICK;
 if(r%7==0)return IN_KICK;
 if(r%5==0)return IN_PUNCH;
 return 0;
}
