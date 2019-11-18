#include "fast_ica.h"
#include <stdio.h>
#include "arm_math.h"
#include "stm32l475e_iot01_qspi.h"

extern UART_HandleTypeDef huart1;
extern int _write(int fd, char *ptr, int len);

float id[4] = {1, 0, 0, 1};
arm_matrix_instance_f32 idmat;

float lbuf[32000], rbuf[32000];

void generateCovMat(float result[2][2], SineWave *centeredL, SineWave *centeredR) {
    float l, r;
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        BSP_QSPI_Read((uint8_t *) &lbuf[i], centeredL->base_addr + i * 4, 4);
        BSP_QSPI_Read((uint8_t *) &rbuf[i], centeredR->base_addr + i * 4, 4);
    }
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        result[0][0] += lbuf[i] * lbuf[i];
        result[0][1] += rbuf[i] * lbuf[i];
        result[1][0] += lbuf[i] * rbuf[i];
        result[1][1] += rbuf[i] * rbuf[i];
    }

    result[0][0] = result[0][0] / (SAMPLE_RATE * DURATION - 1);
    result[0][1] = result[0][1] / (SAMPLE_RATE * DURATION - 1);
    result[1][0] = result[1][0] / (SAMPLE_RATE * DURATION - 1);
    result[1][1] = result[1][1] / (SAMPLE_RATE * DURATION - 1);
}

float generateCenteredWaves(SineWave *centeredL, SineWave *centeredR, float meanL, float meanR, SineWave *baseL, SineWave *baseR) {
    for (int i = 0; i < 2; i++) {
        BSP_QSPI_Erase_Block(centeredL->base_addr + i * MX25R6435F_BLOCK_SIZE);
        BSP_QSPI_Erase_Block(centeredR->base_addr + i * MX25R6435F_BLOCK_SIZE);
    }

    float x;
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        BSP_QSPI_Read((uint8_t *) &x, baseL->base_addr + i * 4, 4);
        x = x - meanL;
        BSP_QSPI_Write((uint8_t *) &x, centeredL->base_addr + i * 4, 4);

        BSP_QSPI_Read((uint8_t *) &x, baseR->base_addr + i * 4, 4);
        x = x - meanR;
        BSP_QSPI_Write((uint8_t *) &x, centeredR->base_addr + i * 4, 4);
    }
}

float computeMean(SineWave *wave) {
    float result = 0.0;
    float x;
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        BSP_QSPI_Read((uint8_t *)&x, wave->base_addr + i * 4, 4);
        result = result + x;
    }
    return result / SAMPLE_RATE * DURATION;
}

void eigen2by2(float eigval[2][2], float eigvec[2][2], float mat[2][2]) {
    float a = mat[0][0];
    float b = mat[0][1];
    float c = mat[1][0];
    float d = mat[1][1];

    float tr = a + d;
    float det = a * d - b * c;

    float tmp;
    arm_sqrt_f32(tr * tr - 4 * det, &tmp);
    float eigval1 = (tr + tmp) / 2;
    float eigval2 = (tr - tmp) / 2;

    printf("eigval1 = %f\neigval2 = %f\n", eigval1, eigval2);

    eigvec[0][0] = -(a - eigval1) / b;
    eigvec[1][0] = 1;
    eigvec[0][1] = -(a - eigval2) / b;
    eigvec[1][1] = 1;

    printf("eigenvectors:\n%f %f\n%f %f\n", eigvec[0][0], eigvec[0][1], eigvec[1][0], eigvec[1][1]);
}

int fast_ica(SineWave *resultL, SineWave *resultR, SineWave *mixedL, SineWave *mixedR) {
    arm_mat_init_f32(&idmat, 2, 2, id);

    SineWave centeredL = {
        .amplitude = 1,
        .freq = 0,
        .base_addr = 20 * MX25R6435F_BLOCK_SIZE};

    SineWave centeredR = {
        .amplitude = 1,
        .freq = 0,
        .base_addr = 22 * MX25R6435F_BLOCK_SIZE};

    float mean1 = computeMean(mixedL);
    float mean2 = computeMean(mixedR);

    generateCenteredWaves(&centeredL, &centeredR, mean1, mean2, mixedL, mixedR);

    float covmat[2][2] = {{0, 0}, {0, 0}};

    generateCovMatrix(covmat, &centeredL, &centeredR);

    float eigval[2][2];
    float eigvec[2][2];

    eigen2by2(eigval, eigvec, covmat);

    float invsqrtmat[2][2] = {{0, 0}, {0, 0}};
    arm_sqrt_f32(1 / eigval[0][0], &invsqrtmat[0][0]);
    arm_sqrt_f32(1 / eigval[1][1], &invsqrtmat[1][1]);

    float whitening[2][2] = {{invsqrtmat[0][0] * eigvec[0][0], invsqrtmat[1][1] * eigvec[0][1]},
                             {invsqrtmat[0][0] * eigvec[1][0], invsqrtmat[1][1] * eigvec[1][1]}};
                            
    float sqrtmat[2][2] = {{0, 0}, {0, 0}};
    arm_sqrt_f32(eigval[0][0], &sqrtmat[0][0]);
    arm_sqrt_f32(eigval[1][1], &sqrtmat[1][1]);

    float dewhitening[2][2] = {{eigvec[0][0] * sqrtmat[0][0], eigvec[0][1] * sqrt[1][1]},
                               {eigvec[1][0] * sqrtmat[1][1], eigvec[1][1] * sqrt[1][1]}};
}