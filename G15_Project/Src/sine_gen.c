#include "stm32l475e_iot01_qspi.h"
#include "arm_math.h"
#include <stdio.h>
#include "sine_gen.h"

extern TIM_HandleTypeDef htim3;
extern DAC_HandleTypeDef hdac1;

extern int tim3flag;

extern UART_HandleTypeDef huart1;
extern int _write (int fd, char * ptr, int len);

const float mix_matrix[2][2] = {
  {0.5, 0.5},
  {0.2, 0.8}
};

int sine_gen_init(void) {
  if (BSP_QSPI_Init() == QSPI_OK) {
    printf("QSPI init success!\n");
    // printf("Another print\n");
    return 1;
  } else {
    printf("QSPI init error :(\n");
    return 0;
  }
}

int sine_wave_gen(SineWave *sine) {

  uint8_t status;
  for (int i = 0; i < 4; i++) {
    status = BSP_QSPI_Erase_Block(i * MX25R6435F_BLOCK_SIZE + sine->base_addr);
    if (status != QSPI_OK) {
      printf("Block erase error\n");
    }
  }

  float f;
  int x;
  for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
    f = AMPLITUDE_MAX * sine->amplitude * arm_sin_f32(2 * PI * sine->freq * i / SAMPLE_RATE);
    status = BSP_QSPI_Write((uint8_t *) &f, sine->base_addr + i * 4, 4);
    
    x = (int) (f + 2048);
    BSP_QSPI_Write((uint8_t *) &x, sine->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
    // printf("%d - %.5f", i, f);
    // if (status == QSPI_OK) {
    //   printf(" - OK\n");
    // } else {
    //   printf(" - ERROR\n");
    // }
  }
  return 0;
}

void play_sine_wave(SineWave *left, SineWave *right) {
  printf("Playing sound!\n");
  uint8_t status;
  int l, r;
  for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
    while (tim3flag == 0);
    status = BSP_QSPI_Read((uint8_t *) &l, left->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
    if (status != QSPI_OK) printf("Read Error\n");
    status = BSP_QSPI_Read((uint8_t *) &r, right->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
    if (status != QSPI_OK) printf("Read Error\n");
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, l);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, r);
    tim3flag = 0;
  }
}

void print_sine_wave(SineWave *left, SineWave *right) {
  float lf, rf;
  int l, r;
  for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
    // while (tim3flag == 0);
    BSP_QSPI_Read((uint8_t *) &lf, left->base_addr + i * 4, 4);
    BSP_QSPI_Read((uint8_t *) &rf, left->base_addr + i * 4, 4);
    BSP_QSPI_Read((uint8_t *) &l, left->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
    BSP_QSPI_Read((uint8_t *) &r, right->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
    
    printf("%d - %f - %f - %d - %d\n", i, lf, rf, l, r);
    // tim3flag = 0;
  }
}

int combine_sine_waves(SineWave *resultL, SineWave *resultR, SineWave *wav1, SineWave *wav2) {
  uint8_t status;
  for (int i = 0; i < 4; i++) {
    status = BSP_QSPI_Erase_Block(resultL->base_addr + i * MX25R6435F_BLOCK_SIZE);
    if (status != QSPI_OK) printf("Erase Error\n");
    status = BSP_QSPI_Erase_Block(resultR->base_addr + i * MX25R6435F_BLOCK_SIZE);
    if (status != QSPI_OK) printf("Erase Error\n");
  }
  printf("Mix matrix:\n");
  printf("%f, %f\n", mix_matrix[0][0], mix_matrix[0][1]);
  printf("%f, %f\n", mix_matrix[1][0], mix_matrix[1][1]);
  float w1, w2;
  float x1, x2;
  int y1, y2;
  
  for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
    BSP_QSPI_Read((uint8_t *) &w1, wav1->base_addr + i * 4, 4);
    BSP_QSPI_Read((uint8_t *) &w2, wav2->base_addr + i * 4, 4);
    x1 = w1 * mix_matrix[0][0] + w2 * mix_matrix[0][1];
    x2 = w1 * mix_matrix[1][0] + w2 * mix_matrix[1][1];
    // printf("%d - %f - %f\n", i, x1, x2);
    status = BSP_QSPI_Write((uint8_t *) &x1, resultL->base_addr + i * 4, 4);
    if (status != QSPI_OK) printf("Write Error\n");
    status = BSP_QSPI_Write((uint8_t *) &x2, resultR->base_addr + i * 4, 4);
    if (status != QSPI_OK) printf("Write Error\n");
    y1 = (int) (x1 + 2048);
    y2 = (int) (x2 + 2048);
    status = BSP_QSPI_Write((uint8_t *) &y1, resultL->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
    if (status != QSPI_OK) printf("Write Error\n");
    status = BSP_QSPI_Write((uint8_t *) &y2, resultR->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
    if (status != QSPI_OK) printf("Write Error\n");
  }
  return 0;
}
