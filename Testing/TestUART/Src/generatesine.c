#include "arm_math.h"
#include "stm32l475e_iot01_qspi.h"
#include <stdio.h>

extern int fputc(int ch, FILE *f);
extern int fgetc(FILE *f);

void writeSineFloat(float freq, int volume, int baseAddr, int sampleFreq, int duration) {
	float val;
	for (int i = 0; i < sampleFreq * duration; i++) {
		val = volume * arm_sin_f32(2 * PI * freq * i / sampleFreq);
		BSP_QSPI_Write((uint8_t *) &val, i * sizeof(float), sizeof(float));
	}
}

void writeSineInt(float freq, int volume, int center, int baseAddr, int sampleFreq, int duration) {
	float val;
	uint32_t intval;
	for (int i = 0; i < sampleFreq * duration; i++) {
		val = volume * arm_sin_f32(2 * PI * freq * i / sampleFreq);
		intval = intval + center;
		if (intval < 0) {
			intval = 0;
		}
		if (intval >= center * 2) {
			intval = center * 2 - 1;
		}
		BSP_QSPI_Write((uint8_t *) &intval, i * sizeof(int), sizeof(int));
	}
}

void printSine(int baseAddr, int length, char type) {
	int idata;
	float fdata;
			
	if (type == 'i') {
		for (int i = 0; i < length; i++) {
			BSP_QSPI_Read((uint8_t *) &idata, baseAddr + i * sizeof(int), sizeof(int));
			printf("%d - %d\n", i, idata);
		}
	} else if (type == 'f') {
		for (int i = 0; i < length; i++) {
			BSP_QSPI_Read((uint8_t *) &fdata, baseAddr + i * sizeof(float), sizeof(float));
			printf("%d - %.5f\n", i, fdata);
		}
	}
}
