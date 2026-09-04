#include "game.h"
#include "render.h"
#include "sound.h"
#include <stdio.h>
#include <stdint.h>
#include <psxapi.h>
#include <psxpad.h>

enum ScreenMode { TITLE, SELECT, PLAY, RESULT, HELP };
static uint8_t pads[2][34];
static uint16_t read_pad(int n){PADTYPE*p=(PADTYPE*)pads[n];return p->stat==0?(uint16_t)~p->btn:0;}
static uint16_t commands(uint16_t p){
 uint16_t i=0;
 if(p&PAD_LEFT)i|=IN_LEFT;if(p&PAD_RIGHT)i|=IN_RIGHT;
 if(p&PAD_DOWN)i|=IN_DOWN;if(p&PAD_UP)i|=IN_JUMP;
 if(p&PAD_SQUARE)i|=IN_PUNCH;if(p&PAD_TRIANGLE)i|=IN_KICK;
 if(p&PAD_CROSS)i|=IN_GUARD;
 if(p&PAD_CIRCLE)i|=IN_PUNCH|IN_GUARD;
 if(p&PAD_L1)i|=IN_STEP_L;if(p&PAD_R1)i|=IN_STEP_R;
 return i;
}
static void timer_digit(int x,int y,int digit,int urgent){
 static const unsigned mask[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
 static const int segment[7][4]={{2,0,8,3},{10,2,3,9},{10,12,3,9},
  {2,20,8,3},{0,12,3,9},{0,2,3,9},{2,10,8,3}};
 int n;for(n=0;n<7;n++)if(mask[digit]&(1u<<n))
  render_rect(x+segment[n][0],y+segment[n][1],segment[n][2],segment[n][3],245,urgent?86:231,urgent?54:172);
}
static void hud(const Game*g,int versus,int stage){
 char text[64];int i;
 int seconds=(g->remaining+59)/60;
 timer_digit(144,14,seconds/10,seconds<=10);timer_digit(161,14,seconds%10,seconds<=10);
 for(i=0;i<2;i++){
  int x=i?188:16,fill=g->f[i].hp*116/100;
  render_text(x,10,game_character_name(g->f[i].character));
  render_rect(x+(i?116-fill:0),25,fill,11,i?40:222,i?176:84,i?203:58);
  render_rect(x-1,24,118,13,69,76,91);
  render_rect(x,41,9,4,g->f[i].wins>0?244:60,g->f[i].wins>0?190:68,60);
  render_rect(x+14,41,9,4,g->f[i].wins>1?244:60,g->f[i].wins>1?190:68,60);
 }
 sprintf(text,"ROUND %d   %s",g->round,versus?"2P VERSUS":"ARCADE");render_center(218,text);
 if(!versus){sprintf(text,"STAGE %d/3",stage);render_center(204,text);}
 render_rect(8,5,304,44,12,20,33);
 if(g->phase==INTRO){sprintf(text,g->tick<60?"ROUND %d":"FIGHT!",g->round);render_center(88,text);}
 if(g->phase==ROUND_OVER){
  static const char*reason[]={"","K.O.","RING OUT","TIME UP","DRAW"};
  render_center(84,reason[g->reason]);
  if(g->winner>=0){sprintf(text,"%s WINS",game_character_name(g->f[g->winner].character));render_center(98,text);}
 }
}
int main(void){
 Game game;int screen=TITLE,choice=0,versus=0,frame=0,idle=0,paused=0,stage=1,demo=0;
 int chars[2]={0,1};uint16_t previous[2]={0,0};
 render_init();sound_init();InitPAD(pads[0],34,pads[1],34);StartPAD();ChangeClearPAD(0);
 game_init(&game,0,1,1);printf("FACET boot: native PS1\n");
 for(;;){
  uint16_t p[2]={read_pad(0),read_pad(1)},edge[2]={p[0]&~previous[0],p[1]&~previous[1]};
  int i,oldphase=game.phase;previous[0]=p[0];previous[1]=p[1];frame=(frame+1)&32767;
  if(screen==TITLE){
   if(edge[0]&PAD_DOWN)choice=(choice+1)%3;
   if(edge[0]&PAD_UP)choice=(choice+2)%3;
   if(edge[0]&(PAD_START|PAD_CROSS)){screen=choice==2?HELP:SELECT;versus=choice==1;idle=0;}
   if(p[0]||p[1])idle=0;
   if(++idle>600){demo=1;screen=PLAY;game_init(&game,0,1,(unsigned)frame+1);}
  }else if(screen==HELP){if(edge[0]&(PAD_START|PAD_CROSS|PAD_SELECT))screen=TITLE;}
  else if(screen==SELECT){
   for(i=0;i<(versus?2:1);i++)if(edge[i]&(PAD_LEFT|PAD_RIGHT))chars[i]^=1;
   if(edge[0]&PAD_SELECT)screen=TITLE;
   if(edge[0]&(PAD_START|PAD_CROSS)){
    stage=1;paused=0;demo=0;game_init(&game,chars[0],versus?chars[1]:chars[0]^1,(unsigned)frame+1);screen=PLAY;
   }
  }else if(screen==PLAY){
   if(demo&&(edge[0]||edge[1])){demo=0;screen=TITLE;idle=0;}
   else{
    if(!demo&&((edge[0]|edge[1])&PAD_START))paused=!paused;
    if(paused){if(edge[0]&PAD_SELECT){paused=0;screen=TITLE;idle=0;}}
    else{
     game_tick(&game,demo?game_cpu(&game,0,1):commands(p[0]),versus&&!demo?commands(p[1]):game_cpu(&game,1,stage));
     sound_event(game.events);
     if(oldphase!=game.phase)printf("FACET phase=%d round=%d hp=%d,%d winner=%d reason=%d\n",game.phase,game.round,game.f[0].hp,game.f[1].hp,game.winner,game.reason);
     if(game.phase==MATCH_OVER){
      if(demo){screen=TITLE;idle=0;demo=0;}
      else if(!versus&&game.winner==0&&stage<3){stage++;game_init(&game,chars[0],stage==2?chars[0]:chars[0]^1,(unsigned)frame+1);}
      else screen=RESULT;
     }
    }
   }
  }else if(screen==RESULT){
   if(edge[0]&PAD_START){stage=1;game_init(&game,chars[0],versus?chars[1]:chars[0]^1,(unsigned)frame+1);screen=PLAY;}
   if(edge[0]&PAD_SELECT){screen=TITLE;idle=0;}
  }
  if(screen==SELECT){game.f[0].character=chars[0];game.f[1].character=versus?chars[1]:chars[0]^1;}
  render_begin(&game,frame,screen==TITLE||screen==HELP?0:screen==SELECT?1:2);render_scene(&game,frame);
  if(screen==TITLE){
   render_center(44,"F A C E T   F I G H T E R");render_center(61,"POLYGON COMBAT / PLAYSTATION");
   render_center(169,choice==0?"> ARCADE <":"  ARCADE  ");
   render_center(182,choice==1?"> 2P VERSUS <":"  2P VERSUS  ");
   render_center(195,choice==2?"> HOW TO PLAY <":"  HOW TO PLAY  ");
   render_center(222,"UP/DOWN  -  START TO SELECT");
  }else if(screen==SELECT){
   render_center(24,"CHOOSE YOUR FIGHTER");
   render_text(24,62,game_character_name(chars[0]));render_text(206,62,game_character_name(versus?chars[1]:chars[0]^1));
   render_center(187,"EMBER: SPEED / BASALT: POWER");
   render_center(202,"LEFT/RIGHT TO CHANGE");render_center(218,"START: FIGHT   SELECT: BACK");
  }else if(screen==HELP){
   render_center(26,"HOW TO PLAY");
   render_text(24,52,"LEFT/RIGHT   MOVE\nDOWN         CROUCH\nUP           JUMP\nSQUARE       PUNCH (HIGH)\nTRIANGLE     KICK (MID)\nCROSS        GUARD\nCIRCLE       THROW (P+G)\nDOWN+KICK    LOW KICK\nPUNCH+KICK   HEAVY KICK\nL1 / R1      CIRCLE RIVAL\nSTART        PAUSE");
   render_center(154,"KO / RING OUT / 30 SECOND LIMIT");
   render_center(170,"FIRST TO TWO ROUNDS WINS");render_center(186,"GUARD LOW: DOWN+CROSS");
   render_center(218,"START TO RETURN");render_rect(12,18,296,212,12,20,33);
  }else{
   hud(&game,versus,stage);
   if(demo)render_center(62,"DEMO - PRESS ANY BUTTON");
   if(paused){render_center(94,"PAUSED");render_center(110,"START: RESUME");render_center(124,"SELECT: TITLE");render_rect(72,84,176,54,12,20,33);}
   if(screen==RESULT){
    render_center(82,!versus&&game.winner==0?"ARCADE CHAMPION":game.winner==0?"PLAYER 1 WINS":"PLAYER 2 WINS");
    render_center(99,"START: REMATCH");render_center(112,"SELECT: TITLE");render_rect(64,73,192,53,12,20,33);
   }
  }
  render_end();
 }
 return 0;
}
