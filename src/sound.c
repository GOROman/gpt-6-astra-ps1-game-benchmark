#include "sound.h"
#include "game.h"
#include "sound_synth.h"
#include <stdint.h>
#include <psxspu.h>
#define SAMPLE_BYTES SOUND_SAMPLE_BYTES
#define SAMPLE_BASE 0x1100
static uint32_t sample[SAMPLE_BYTES/4];
static int channel;
void sound_init(void){
 int kind;
 SpuInit();SpuSetCommonMasterVolume(0x3fff,0x3fff);
 for(kind=0;kind<4;kind++){
  sound_generate(kind,(uint8_t*)sample);
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
 if(events&EV_ROUND)play(3,2048,0x2000);
 else if(events&EV_THROW)play(1,1700,0x3000);
 else if(events&EV_HIT)play(1,2048+(channel%3-1)*75,0x3000);
 else if(events&EV_BLOCK)play(2,2200,0x2800);
 else if(events&EV_SWING)play(0,2400,0x1c00);
}
