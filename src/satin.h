/* Header satin.h by Alan K Stewart
 Saturation Intensity Calculation */

#ifndef SATIN_H
#define SATIN_H

#define N  8

typedef struct {
    int inputPower, saturationIntensity;
    double outputPower;
} gaussian;

typedef struct {
    char outputFile[9];
    float smallSignalGain;
    int dischargePressure;
    char carbonDioxide[3];
} laser;

typedef struct {
    int pNum, *inputPowers;
    laser laserData;
} satin_process_args;

void calculate(_Bool concurrent);
int getInputPowers(int **inputPowers);
int getLaserData(laser **laserData);
void *process(void *arg);
void gaussianCalculation(int inputPower, float smallSignalGain, gaussian **gaussianData);
void error();

#endif
