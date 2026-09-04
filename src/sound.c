#include "sound.h"
#include "game.h"
#include <stdint.h>
#include <psxspu.h>
#define SAMPLE_BYTES 2048
#define SAMPLE_BASE 0x1100
static uint32_t sample[SAMPLE_BYTES/4];
static int channel;
void sound_init(void){
 int kind,block,n;uint32_t rng=0x13579;
 SpuInit();SpuSetCommonMasterVolume(0x3fff,0x3fff);
 for(kind=0;kind<4;kind++){
  uint8_t*out=(uint8_t*)sample;
  for(block=0;block<128;block++){
   out[block*16]=1;out[block*16+1]=block==127?1:0;
   for(n=0;n<28;n++){
    int index=block*28+n,amp=7*(127-block)/127,value;
    rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
    if(kind==0)value=((int)(rng%15)-7)*amp/7;
    else if(kind==1)value=((index/(10+block/3))&1)?amp:-amp;
    else if(kind==2)value=((index/8)&1)?amp/2:-amp/2;
    else{int phase=index%64;value=(phase<32?phase:64-phase)-16;value=value*amp/16;}
    if(n&1)out[block*16+2+n/2]|=(value&15)<<4;
    else out[block*16+2+n/2]=value&15;
   }
  }
  SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
  SpuSetTransferStartAddr(SAMPLE_BASE+kind*SAMPLE_BYTES);
  SpuWrite(sample,SAMPLE_BYTES);SpuIsTransferCompleted(SPU_TRANSFER_WAIT);
 }
}
static void play(int kind,int pitch,int volume){
 int ch=channel;channel=(channel+1)%8;
 SpuSetKey(0,1<<ch);SpuSetVoiceStartAddr(ch,SAMPLE_BASE+kind*SAMPLE_BYTES);
 SpuSetVoicePitch(ch,pitch);SpuSetVoiceVolume(ch,volume,volume);
 SPU_CH_ADSR1(ch)=0x00ff;SPU_CH_ADSR2(ch)=0;
 SpuSetKey(1,1<<ch);
}
void sound_event(int events){
 if(events&EV_ROUND)play(3,1900,0x3000);
 else if(events&EV_THROW)play(1,1100,0x3000);
 else if(events&EV_HIT)play(1,2000,0x2800);
 else if(events&EV_BLOCK)play(2,2500,0x2000);
 else if(events&EV_SWING)play(0,3000,0x1500);
}
