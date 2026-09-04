#include "sound_synth.h"
static int sine(unsigned phase){
 int x=(int)(phase&65535);if(x>=32768)x-=65536;
 return x*(32768-(x<0?-x:x))/8192;
}
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
void sound_generate(int kind,uint8_t out[SOUND_SAMPLE_BYTES]){
 uint32_t rng=0x13579u+(unsigned)kind*7919u;
 unsigned phase=0;int envelope=24576,attack=24576,filtered=0;
 int block,n;
 for(block=0;block<SOUND_SAMPLE_BYTES/16;block++){
  int pcm[28],peak=0,shift=0,step;
  for(n=0;n<28;n++){
   int i=block*28+n,raw,value=0;
   rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
   raw=(int)(rng&65535)-32768;filtered=(filtered*3+raw)/4;
   if(kind==0){ /* A soft, short air swish; no pitched oscillator. */
    int swell=i<180?i*110:19800;
    if(i>=180)envelope=envelope*4084/4096;
    value=(raw-filtered)/2*(swell*envelope/32768)/32768;
   }else if(kind==1){ /* Broadband attack, falling low body resonance. */
    int hz_step=235+210*envelope/24576;
    phase+=(unsigned)hz_step;
    value=sine(phase)*envelope/32768+raw*attack/32768;
    envelope=envelope*4089/4096;attack=attack*3970/4096;
   }else if(kind==2){ /* Dry palm/forearm contact, brighter than a body hit. */
    phase+=1950;
    value=(raw-filtered)*attack/65536+sine(phase)*envelope/65536;
    envelope=envelope*4020/4096;attack=attack*4040/4096;
   }else{ /* Separate, unobtrusive round cue. */
    phase+=i<1800?1550:2325;
    value=sine(phase)*envelope/32768;
    envelope=envelope*4093/4096;
   }
   /* Silent terminal block prevents wrap-around clicks and endless samples. */
   pcm[n]=block==SOUND_SAMPLE_BYTES/16-1?0:clamp(value,-28000,28000);
   {int a=pcm[n]<0?-pcm[n]:pcm[n];if(a>peak)peak=a;}
  }
  while(shift<12&&peak<=7*(1<<(11-shift)))shift++;
  step=1<<(12-shift);
  out[block*16]=(uint8_t)shift;
  out[block*16+1]=block==SOUND_SAMPLE_BYTES/16-1?1:0;
  for(n=0;n<28;n++){
   int v=clamp(pcm[n]/step,-8,7)&15;
   if(n&1)out[block*16+2+n/2]|=(uint8_t)(v<<4);
   else out[block*16+2+n/2]=(uint8_t)v;
  }
 }
}
