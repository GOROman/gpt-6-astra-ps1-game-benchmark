#ifndef FACET_SOUND_SYNTH_H
#define FACET_SOUND_SYNTH_H
#include <stdint.h>
#define SOUND_SAMPLE_BYTES 4096
#define SOUND_SAMPLE_RATE 22050
/* Generates PS1 filter-zero ADPCM, with block-adaptive quantization. */
void sound_generate(int kind,uint8_t out[SOUND_SAMPLE_BYTES]);
#endif
