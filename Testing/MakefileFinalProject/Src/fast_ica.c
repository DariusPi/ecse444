#include "fast_ica.h"
#include <stdio.h>
#include "arm_math.h"
#include "stm32l475e_iot01_qspi.h"
#include "stm32l4xx_hal_rng.h"

extern RNG_HandleTypeDef hrng;
extern UART_HandleTypeDef huart1;
extern int _write(int fd, char *ptr, int len);

// float id[4] = {1, 0, 0, 1};
// arm_matrix_instance_f32 idmat;

#define MAX_ITERATIONS 1000
#define EPSILON 0.0001
// float lbuf[32000], rbuf[32000];

void applyFilter(SineWave *resultL, SineWave *resultR, SineWave *inputL, SineWave *inputR, float ica_filter[2][2]) {
    float l, r;
    float ltmp, rtmp;
    int ltmpi, rtmpi;
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        BSP_QSPI_Read((uint8_t *) &l, inputL->base_addr + i * 4, 4);
        BSP_QSPI_Read((uint8_t *) &r, inputR->base_addr + i * 4, 4);

        ltmp = l * ica_filter[0][0] + r * ica_filter[0][1];
        rtmp = l * ica_filter[1][0] + r * ica_filter[1][1];

        BSP_QSPI_Write((uint8_t *) &ltmp, resultL->base_addr + i * 4, 4);
        BSP_QSPI_Write((uint8_t *) &rtmp, resultR->base_addr + i * 4, 4);

        ltmpi = (int) ((AMPLITUDE_MAX / 3) * ltmp + 2048);
        rtmpi = (int) ((AMPLITUDE_MAX / 3) * rtmp + 2048);

        BSP_QSPI_Write((uint8_t *) &ltmpi, resultR->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
        BSP_QSPI_Write((uint8_t *) &rtmpi, resultL->base_addr + SAMPLE_RATE * DURATION * 4 + i * 4, 4);
    }
}

void updateWeight(SineWave *whiteL, SineWave *whiteR, float weight[2]) {
    float l, r;
    float tmp, w0tmp, w1tmp;
    w0tmp = 0.0;
    w1tmp = 0.0;
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        BSP_QSPI_Read((uint8_t *) &l, whiteL->base_addr + i * 4, 4);
        BSP_QSPI_Read((uint8_t *) &r, whiteR->base_addr + i * 4, 4);

        tmp = l * weight[0] + r * weight[1];
        tmp = tmp * tmp * tmp;

        w0tmp = w0tmp + tmp * l;
        w1tmp = w1tmp + tmp * r;
    }
    w0tmp = w0tmp / (SAMPLE_RATE * DURATION) - 3 * weight[0];
    w1tmp = w1tmp / (SAMPLE_RATE * DURATION) - 3 * weight[1];

    weight[0] = w0tmp;
    weight[1] = w1tmp;

    float absval;
    arm_sqrt_f32(weight[0] * weight[0] + weight[1] * weight[1], &absval);
    weight[0] = weight[0] / absval;
    weight[1] = weight[1] / absval;
}

void calculateWhite(SineWave *whitenedL, SineWave *whitenedR, SineWave *centeredL, SineWave *centeredR, float whitening[2][2]) {
    for (int i = 0; i < 4; i++) {
        BSP_QSPI_Erase_Block(whitenedL->base_addr + i * MX25R6435F_BLOCK_SIZE);
        BSP_QSPI_Erase_Block(whitenedR->base_addr + i * MX25R6435F_BLOCK_SIZE);
    }

    float l, r;
    float ltmp, rtmp;
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        BSP_QSPI_Read((uint8_t *)&l, centeredL->base_addr + i * 4, 4);
        BSP_QSPI_Read((uint8_t *)&r, centeredR->base_addr + i * 4, 4);

        ltmp = whitening[0][0] * l + whitening[0][1] * r;
        rtmp = whitening[1][0] * l + whitening[1][1] * r;

        BSP_QSPI_Write((uint8_t *)&ltmp, whitenedL->base_addr + i * 4, 4);
        BSP_QSPI_Write((uint8_t *)&rtmp, whitenedR->base_addr + i * 4, 4);
    }
}

void generateCovMatrix(float result[2][2], SineWave *centeredL, SineWave *centeredR) {
    float l, r;
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        BSP_QSPI_Read((uint8_t *)&l, centeredL->base_addr + i * 4, 4);
        BSP_QSPI_Read((uint8_t *)&r, centeredR->base_addr + i * 4, 4);

        result[0][0] += l * l;
        result[0][1] += r * l;
        result[1][0] += l * r;
        result[1][1] += r * r;
    }

    result[0][0] = result[0][0] / (SAMPLE_RATE * DURATION - 1);
    result[0][1] = result[0][1] / (SAMPLE_RATE * DURATION - 1);
    result[1][0] = result[1][0] / (SAMPLE_RATE * DURATION - 1);
    result[1][1] = result[1][1] / (SAMPLE_RATE * DURATION - 1);
}

void generateCenteredWaves(SineWave *centeredL, SineWave *centeredR, float meanL, float meanR, SineWave *baseL, SineWave *baseR) {
    for (int i = 0; i < 4; i++) {
        BSP_QSPI_Erase_Block(centeredL->base_addr + i * MX25R6435F_BLOCK_SIZE);
        BSP_QSPI_Erase_Block(centeredR->base_addr + i * MX25R6435F_BLOCK_SIZE);
    }

    float x;
    for (int i = 0; i < SAMPLE_RATE * DURATION; i++) {
        BSP_QSPI_Read((uint8_t *)&x, baseL->base_addr + i * 4, 4);
        x = x - meanL;
        // x = x / AMPLITUDE_MAX;
        BSP_QSPI_Write((uint8_t *)&x, centeredL->base_addr + i * 4, 4);

        BSP_QSPI_Read((uint8_t *)&x, baseR->base_addr + i * 4, 4);
        x = x - meanR;
        // x = x / AMPLITUDE_MAX;
        BSP_QSPI_Write((uint8_t *)&x, centeredR->base_addr + i * 4, 4);
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

    eigval[0][0] = eigval2;
    eigval[1][1] = eigval1;

    eigval[1][0] = 0.0;
    eigval[0][1] = 0.0;

    // printf("eigval1 = %f\neigval2 = %f\n", eigval1, eigval2);

    eigvec[0][0] = a - eigval1;
    eigvec[1][0] = c;
    float len;
    arm_sqrt_f32(eigvec[0][0] * eigvec[0][0] + eigvec[1][0] * eigvec[1][0], &len);
    eigvec[0][0] = eigvec[0][0] / len;
    eigvec[1][0] = eigvec[1][0] / len;

    eigvec[0][1] = a - eigval2;
    eigvec[1][1] = c;
    arm_sqrt_f32(eigvec[0][1] * eigvec[0][1] + eigvec[1][1] * eigvec[1][1], &len);
    eigvec[0][1] = eigvec[0][1] / len;
    eigvec[1][1] = eigvec[1][1] / len;

    printf("eigenvalues:\n%f %f\n%f %f\n", eigval[0][0], eigval[0][1], eigval[1][0], eigval[1][1]);
    printf("eigenvectors:\n%f %f\n%f %f\n", eigvec[0][0], eigvec[0][1], eigvec[1][0], eigvec[1][1]);
}

int fast_ica(SineWave *resultL, SineWave *resultR, SineWave *mixedL, SineWave *mixedR) {
    // arm_mat_init_f32(&idmat, 2, 2, id);

    for (int i = 0; i < 4; i++) {
        BSP_QSPI_Erase_Block(resultL->base_addr + i * MX25R6435F_BLOCK_SIZE);
        BSP_QSPI_Erase_Block(resultR->base_addr + i * MX25R6435F_BLOCK_SIZE);
    }

    SineWave centeredL = {.amplitude = 1, .freq = 0, .base_addr = get_base_address()};
    SineWave centeredR = {.amplitude = 1, .freq = 0, .base_addr = get_base_address()};
    SineWave whitenedL = {.amplitude = 1, .freq = 0, .base_addr = get_base_address()};
    SineWave whitenedR = {.amplitude = 1, .freq = 0, .base_addr = get_base_address()};

    float mean1 = computeMean(mixedL);
    float mean2 = computeMean(mixedR);

    generateCenteredWaves(&centeredL, &centeredR, mean1, mean2, mixedL, mixedR);

    float covmat[2][2] = {{0, 0}, {0, 0}};

    generateCovMatrix(covmat, &centeredL, &centeredR);

    printf("covmat:\n%f %f\n%f %f\n", covmat[0][0], covmat[0][1], covmat[1][0], covmat[1][1]);

    float eigval[2][2];
    float eigvec[2][2];

    eigen2by2(eigval, eigvec, covmat);

    // eigval[0][0] = 59014.48907864;
    // eigval[1][1] = 790585.51092136;
    float invsqrtmat[2][2] = {{0, 0}, {0, 0}};
    arm_sqrt_f32(1 / eigval[0][0], &invsqrtmat[0][0]);
    arm_sqrt_f32(1 / eigval[1][1], &invsqrtmat[1][1]);

    printf("Inverse Sqrt Eigval matrix:\n%f %f\n%f %f\n", invsqrtmat[0][0], invsqrtmat[0][1], invsqrtmat[1][0], invsqrtmat[1][1]);

    // float whitening[2][2] = {{invsqrtmat[0][0] * eigvec[0][0], invsqrtmat[1][1] * eigvec[0][1]}, {invsqrtmat[0][0] * eigvec[1][0], invsqrtmat[1][1] * eigvec[1][1]}};
    float whitening[2][2];
    whitening[0][0] = invsqrtmat[0][0] * eigvec[0][0];
    whitening[0][1] = invsqrtmat[0][0] * eigvec[0][1];
    whitening[1][0] = invsqrtmat[1][1] * eigvec[1][0];
    whitening[1][1] = invsqrtmat[1][1] * eigvec[1][1];

    // float sqrtmat[2][2] = {{0, 0}, {0, 0}};
    // arm_sqrt_f32(eigval[0][0], &sqrtmat[0][0]);
    // arm_sqrt_f32(eigval[1][1], &sqrtmat[1][1]);

    // float dewhitening[2][2] = {{eigvec[0][0] * sqrtmat[0][0], eigvec[0][1] * sqrtmat[1][1]}, {eigvec[1][0] * sqrtmat[1][1], eigvec[1][1] * sqrtmat[1][1]}};
    // float dewhitening[2][2];
    // dewhitening[0][0] = eigvec[0][0] * sqrtmat[0][0];
    // dewhitening[0][1] = eigvec[0][1] * sqrtmat[1][1];
    // dewhitening[1][0] = eigvec[1][0] * sqrtmat[0][0];
    // dewhitening[1][1] = eigvec[1][1] * sqrtmat[1][1];

    printf("Whitening matrix:\n%f %f\n%f %f\n", whitening[0][0], whitening[0][1], whitening[1][0], whitening[1][1]);
    // printf("Dewhitening matrix:\n%f %f\n%f %f\n", dewhitening[0][0], dewhitening[0][1], dewhitening[1][0], dewhitening[1][1]);

    calculateWhite(&whitenedL, &whitenedR, &centeredL, &centeredR, whitening);

    float weight[2] = {1.0, 1.0};
    // uint32_t rand;
    // HAL_RNG_GenerateRandomNumber(&hrng, (uint32_t *)&rand);
    // weight[0] = (float32_t)rand;
    // HAL_RNG_GenerateRandomNumber(&hrng, (uint32_t *)&rand);
    // weight[1] = (float32_t)rand;

    float absval;
    arm_sqrt_f32(weight[0] * weight[0] + weight[1] * weight[1], &absval);
    weight[0] = weight[0] / absval;
    weight[1] = weight[1] / absval;

    float weight_old[2] = {0.0, 0.0};

    printf("Weights:\n%f\n%f\n", weight[0], weight[1]);

    int iteration = 0;
    float diff;
    while (iteration < MAX_ITERATIONS) {
        printf("Iteration %d\n", iteration);
        arm_sqrt_f32((weight[0] - weight_old[0]) * (weight[0] - weight_old[0]) + (weight[1] - weight_old[1]) * (weight[1] - weight_old[1]), &diff);
        if (diff < EPSILON) {
            printf("Converged\n");
            break;
        }
        arm_sqrt_f32((weight[0] + weight_old[0]) * (weight[0] + weight_old[0]) + (weight[1] + weight_old[1]) * (weight[1] + weight_old[1]), &diff);
        if (diff < EPSILON) {
            printf("Converged\n");
            break;
        }

        weight_old[0] = weight[0];
        weight_old[1] = weight[1];
        // SineWave powerTemp = {.amplitude = 1, .freq = 0, .base_addr = get_base_address()};

        updateWeight(&whitenedL, &whitenedR, weight);
        iteration++;
    }

    printf("Weights:\n%f\n%f\n", weight[0], weight[1]);

    float basis_set[2][2] = {{weight[0], -weight[1]}, {weight[1], weight[0]}};
    printf("Basis set:\n%f %f\n%f %f\n", basis_set[0][0], basis_set[0][1], basis_set[1][0], basis_set[1][1]);

    // weight[0] = 0.452487;
    // weight[1] = 0.891770;
    
    // float dewhiten_vec[2][2];
    // dewhiten_vec[0][0] = dewhitening[0][0] * basis_set[0][0] + dewhitening[0][1] * basis_set[1][0];
    // dewhiten_vec[0][1] = dewhitening[0][0] * basis_set[0][1] + dewhitening[0][1] * basis_set[1][1];
    // dewhiten_vec[1][0] = dewhitening[1][0] * basis_set[0][0] + dewhitening[1][1] * basis_set[1][0];
    // dewhiten_vec[1][1] = dewhitening[1][0] * basis_set[0][1] + dewhitening[1][1] * basis_set[1][1];

    float ica_filter[2][2];
    ica_filter[0][0] = basis_set[0][0] * whitening[0][0] + basis_set[1][0] * whitening[1][0];
    ica_filter[0][1] = basis_set[0][0] * whitening[0][1] + basis_set[1][0] * whitening[1][1];
    ica_filter[1][0] = basis_set[0][1] * whitening[0][0] + basis_set[1][1] * whitening[1][0];
    ica_filter[1][1] = basis_set[0][1] * whitening[0][1] + basis_set[1][1] * whitening[1][1];

    printf("ICA Filter:\n%f %f\n%f %f\n", ica_filter[0][0], ica_filter[0][1], ica_filter[1][0], ica_filter[1][1]);
    // ica_filter[0][0] = -0.00078567;
    // ica_filter[0][1] = 0.00196419;
    // ica_filter[1][0] = 0.0031427;
    // ica_filter[1][1] = -0.00196419;
    applyFilter(resultL, resultR, mixedL, mixedR, ica_filter);

    return 0;
}