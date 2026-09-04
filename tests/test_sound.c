#include "sound_synth.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
int main(void){
 uint8_t a[SOUND_SAMPLE_BYTES],b[SOUND_SAMPLE_BYTES];int kind,i;
 for(kind=0;kind<4;kind++){
  uint64_t early=0,late=0;int nonzero=0;
  sound_generate(kind,a);sound_generate(kind,b);assert(!memcmp(a,b,sizeof(a)));
  for(i=0;i<SOUND_SAMPLE_BYTES/16;i++){
   int j;assert(a[i*16]<=12);assert(a[i*16+1]==(i==SOUND_SAMPLE_BYTES/16-1?1:0));
   for(j=0;j<28;j++){
    int nibble=(a[i*16+2+j/2]>>((j&1)*4))&15;
    int sample=(nibble<8?nibble:nibble-16)*(1<<(12-a[i*16]));
    uint64_t energy=(uint64_t)(sample*sample);
    if(i<16)early+=energy;
    if(i>=240)late+=energy;
    nonzero+=sample!=0;
   }
  }
  assert(nonzero>200);assert(early>late*100);
  for(i=SOUND_SAMPLE_BYTES-14;i<SOUND_SAMPLE_BYTES;i++)assert(a[i]==0);
 }
 puts("PASS: SPU ADPCM headers, termination, deterministic output and transient decay");return 0;
}
