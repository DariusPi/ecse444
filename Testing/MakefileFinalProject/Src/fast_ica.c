#include "fast_ica.h"
#include "arm_math.h"
#include <math.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;
extern int _write (int fd, char * ptr, int len);

float id[4] = {1, 0, 0, 1};
arm_matrix_instance_f32 idmat;

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

    
}

int fast_ica(SineWave *resultL, SineWave *resultR, SineWave *mixedL, SineWave *mixedR) {
    arm_mat_init_f32(&idmat, 2, 2, id);
}