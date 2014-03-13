/* Header satin.h by Alan K Stewart
 Saturation Intensity Calculation */

#ifndef SATIN_H
#define SATIN_H

#include <regex.h>

#define N   8
#define BUF 25
#define ERR "Failed to allocate memory"

typedef struct {
    char outputFile[9];
    double smallSignalGain;
    int dischargePressure;
    char carbonDioxide[3];
} laser;

typedef struct {
    int pNum;
    int *inputPowers;
    laser laserData;
} satin_process_args;

typedef struct {
    int inputPower;
    int saturationIntensity;
    double outputPower;
} gaussian;

void calculateConcurrently();
void calculate();
int getInputPowers(int **inputPowers);
int getLaserData(laser **laserData);
char *get_regerror(int errcode, regex_t *preg);
void *process(void *arg);
int gaussianCalculation(int inputPower, float smallSignalGain, gaussian **gaussianData);

#endif
