#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Game g;
static void start(int distance){game_init(&g,0,0,1);g.phase=FIGHT;g.f[0].x=-distance/2;g.f[1].x=distance/2;}
static void ticks(int n,int a,int b){while(n--)game_tick(&g,a,b);}
static void strike(int a,int b){game_tick(&g,a,b);ticks(25,0,b);}
int main(void){
 start(150);strike(IN_PUNCH,0);assert(g.f[1].hp==91);
 start(150);strike(IN_PUNCH,IN_GUARD);assert(g.f[1].hp==100);
 start(150);strike(IN_PUNCH,IN_DOWN);assert(g.f[1].hp==100);
 start(150);strike(IN_KICK,IN_GUARD|IN_DOWN);assert(g.f[1].hp==82);
 start(150);strike(IN_DOWN|IN_KICK,IN_GUARD);assert(g.f[1].hp==88);
 start(150);strike(IN_DOWN|IN_KICK,IN_GUARD|IN_DOWN);assert(g.f[1].hp==100);
 start(130);strike(IN_PUNCH|IN_GUARD,IN_GUARD);assert(g.f[1].hp==78);
 start(130);strike(IN_PUNCH|IN_GUARD,IN_DOWN);assert(g.f[1].hp==100);
 start(150);game_tick(&g,IN_DOWN|IN_KICK,IN_JUMP);ticks(20,0,0);assert(g.f[1].hp==100);
 start(150);game_tick(&g,IN_PUNCH,IN_PUNCH);ticks(10,0,0);assert(g.f[0].hp==91&&g.f[1].hp==91);
 start(235);strike(IN_SHOULDER,0);assert(g.f[1].hp==76&&g.f[1].action==DOWN);
 start(235);strike(IN_SHOULDER,IN_GUARD);assert(g.f[1].hp==100);
 start(235);strike(IN_SHOULDER,IN_GUARD|IN_DOWN);assert(g.f[1].hp==76);
 start(400);strike(IN_SHOULDER,0);assert(g.f[1].hp==100);
 start(150);g.f[0].x=570;g.f[1].x=720;strike(IN_SHOULDER,0);assert(g.reason==RING_OUT&&g.winner==0);
 start(400);strike(IN_PUNCH,0);assert(g.f[1].hp==100);
 start(150);g.f[1].hp=1;strike(IN_PUNCH,0);assert(g.reason==KO&&g.winner==0&&g.f[0].wins==1);
 ticks(RESULT_TICKS,0,0);assert(g.phase==INTRO&&g.round==2);ticks(INTRO_TICKS,0,0);assert(g.phase==FIGHT);
 g.f[1].hp=0;game_tick(&g,0,0);ticks(RESULT_TICKS,0,0);assert(g.phase==MATCH_OVER&&g.f[0].wins==2);
 start(150);g.f[0].hp=g.f[1].hp=1;game_tick(&g,IN_PUNCH,IN_PUNCH);ticks(10,0,0);assert(g.reason==DRAW&&g.winner==-1&&!g.f[0].wins);
 start(150);g.f[0].x=RING_EDGE+1;game_tick(&g,0,0);assert(g.reason==RING_OUT&&g.winner==1);
 start(150);g.remaining=1;g.f[0].hp=90;game_tick(&g,0,0);assert(g.reason==TIME_UP&&g.winner==1);
 start(150);g.remaining=1;game_tick(&g,0,0);assert(g.reason==DRAW&&g.winner==-1);
 start(400);game_tick(&g,IN_BACKSTEP,0);ticks(18,0,0);
 assert(g.f[0].x<-300&&g.f[0].action==IDLE);
 start(400);game_tick(&g,IN_LEFT,0);game_tick(&g,0,0);game_tick(&g,IN_LEFT,0);
 assert(g.f[0].action==BACKSTEP);
 start(400);game_tick(&g,0,IN_RIGHT);game_tick(&g,0,0);game_tick(&g,0,IN_RIGHT);
 assert(g.f[1].action==BACKSTEP);
 start(400);ticks(10,IN_DOWN,0);assert(g.f[0].crouch);game_tick(&g,0,0);assert(!g.f[0].crouch);
 start(150);game_tick(&g,IN_PUNCH,0);ticks(5,0,0);
 assert(g.hitstop==3&&g.f[1].hp==91&&g.effect_life[1]==14);
 {int time=g.remaining,x=g.f[1].x,t=g.f[0].tick;int freeze=g.hitstop;
  ticks(freeze,0,0);assert(g.remaining==time&&g.f[1].x==x&&g.f[0].tick==t);
  game_tick(&g,0,0);assert(g.remaining==time-1&&g.f[1].hp==91);
 }
 /* With fighters aligned in depth, both screen-relative inputs approach. */
 start(400);g.f[0].x=g.f[1].x=0;g.f[0].z=-200;g.f[1].z=200;
 ticks(10,IN_RIGHT,IN_LEFT);assert(game_distance(&g.f[0],&g.f[1])<320);
 start(400);g.f[0].x=g.f[1].x=0;g.f[0].z=-200;g.f[1].z=200;
 {int i;for(i=0;i<60;i++){int a=game_cpu(&g,0,1),b=game_cpu(&g,1,1);game_tick(&g,a,b);}}
 assert(game_distance(&g.f[0],&g.f[1])<300);
 /* Identical seeds and commands must remain bit-identical for long runs. */
 {Game a,b;int i;game_init(&a,0,1,37);game_init(&b,0,1,37);
  for(i=0;i<20000;i++){
   int a0=game_cpu(&a,0,1),a1=game_cpu(&a,1,2);
   int b0=game_cpu(&b,0,1),b1=game_cpu(&b,1,2);
   game_tick(&a,a0,a1);game_tick(&b,b0,b1);assert(!memcmp(&a,&b,sizeof(a)));
   assert(a.f[0].hp>=0&&a.f[0].hp<=100&&a.f[1].hp>=0&&a.f[1].hp<=100);
   if(a.phase==MATCH_OVER){game_init(&a,0,1,(unsigned)i+1);game_init(&b,0,1,(unsigned)i+1);}
  }
 }
 puts("PASS: hit levels, guard, throws, jump, trades, range, KO, rounds, ring-out, timeout, draw, determinism");
 return 0;
}
