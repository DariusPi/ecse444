#ifndef _SINE_GEN
#define _SINE_GEN

#include "arm_math.h"

#define SAMPLE_RATE 16000
#define DURATION 2
#define AMPLITUDE_MAX 1200

typedef struct {
    float32_t amplitude;
    float32_t freq;
    uint32_t base_addr;
} SineWave;

uint32_t get_base_address(void);
int sine_gen_init(void);
int sine_wave_gen(SineWave *sine);
void play_sine_wave(SineWave *left, SineWave *right);
void print_sine_wave(SineWave *left, SineWave *right);
int combine_sine_waves(SineWave *resultL, SineWave *resultR, SineWave *wav1, SineWave *wav2);

#endif