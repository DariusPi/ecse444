#ifndef _GENERATESINE_H
#define _GENERATESINE_H

void writeSineFloat(float freq, int volume, int baseAddr, int sampleFreq, int duration);
void writeSineInt(float freq, int volume, int center, int baseAddr, int sampleFreq, int duration);
void printSine(int baseAddr, int length);

#endif