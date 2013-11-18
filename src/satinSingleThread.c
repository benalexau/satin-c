/* Program satinSingleThread.c by Alan K Stewart
 Saturation Intensity Calculation */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>

#define N     100
#define RAD   18E-2
#define W1    3E-1
#define DR    2E-3
#define DZ    4E-2
#define LAMDA 10.6E-3
#define AREA  (M_PI * (RAD * RAD))
#define Z1    (M_PI * (W1 * W1) / LAMDA)
#define Z12   Z1 * Z1
#define EXPR  2 * M_PI * DR

typedef struct {
    int inputPower, saturationIntensity;
    double outputPower;
} gaussian;

_Bool calculate();
int getInputPowers(int inputPowers[]);
int getLaserData(float smallSignalGain[], char outputFile[][9], int dischargePressure[], char carbonDioxide[][3]);
void gaussianCalculation(int inputPower, float smallSignalGain, gaussian *gaussianData);

int main(int argc, char **argv) {

    int rc = 0;
    clock_t start = clock();
    if (!calculate()) {
        printf("Failed to complete\n");
        rc = 1;
    }
    clock_t end = clock();
    printf("The time was %5.2f seconds.\n", ((double) end - start) / CLOCKS_PER_SEC);
    return rc;
}

_Bool calculate() {

    int i, j, k, pNum, lNum, inputPowerData[N], total, count, dischargePressure[N];
    float smallSignalGain[N];
    char outputFile[N][9], carbonDioxide[N][3];
    time_t the_time;
    FILE *fd;
    gaussian *gaussianData = malloc(16 * sizeof(gaussian));

    pNum = getInputPowers(inputPowerData);
    lNum = getLaserData(smallSignalGain, outputFile, dischargePressure, carbonDioxide);

    total = 0;
    for (i = 0; i < lNum; i++) {
        if ((fd = fopen(outputFile[i], "w+")) == NULL) {
            printf("Error opening %s\n", outputFile[i]);
            exit(EXIT_FAILURE);
        }

        time(&the_time);
        fprintf(fd,
                "Start date: %s\nGaussian Beam\n\nPressure in Main Discharge = %dkPa\nSmall-signal Gain = %4.1f%%\nCO2 via %s\n\nPin\t\tPout\t\tSat. Int\tln(Pout/Pin)\tPout-Pin\n(watts)\t\t(watts)\t\t(watts/cm2)\t\t\t(watts)\n",
                ctime(&the_time), dischargePressure[i], smallSignalGain[i], carbonDioxide[i]);
        count = 0;
        for (j = 0; j < pNum; j++) {
            gaussianCalculation(inputPowerData[j], smallSignalGain[i], gaussianData);
            for (k = 0; k < sizeof(*gaussianData); k++) {
                int inputPower = gaussianData[k].inputPower;
                double outputPower = gaussianData[k].outputPower;
                int saturationIntensity = gaussianData[k].saturationIntensity;
                fprintf(fd, "%d\t\t%7.3f\t\t%d\t\t%5.3f\t\t%7.3f\n", inputPower, outputPower, saturationIntensity,
                        log(outputPower / inputPower), outputPower - inputPower);
            }
            count++;
        }

        time(&the_time);
        fprintf(fd, "\nEnd date: %s\n", ctime(&the_time));
        fflush(fd);

        if (fclose(fd) == EOF) {
            printf("Error closing %s\n", outputFile[i]);
            exit(EXIT_FAILURE);
        }

        total += count;
    }

    free((gaussian*) gaussianData);
    return total == pNum * lNum;
}

int getInputPowers(int inputPowers[]) {

    int i, inputPower;
    char *inputPowerFile = "pin.dat";
    FILE *fd;

    if ((fd = fopen(inputPowerFile, "r")) == NULL) {
        printf("Error opening %s\n", inputPowerFile);
        exit(EXIT_FAILURE);
    }

    for (i = 0; fscanf(fd, "%d \n", &inputPower) != EOF; i++) {
        inputPowers[i] = inputPower;
    }

    if (fclose(fd) == EOF) {
        printf("Error closing %s\n", inputPowerFile);
        exit(EXIT_FAILURE);
    }

    return i;
}

int getLaserData(float smallSignalGain[], char outputFile[][9], int dischargePressure[], char carbonDioxide[][3]) {

    int i, mainDischargePressure;
    float laserGain;
    char *gainMediumDataFile = "laser.dat";
    FILE *fd;

    if ((fd = fopen(gainMediumDataFile, "r")) == NULL) {
        printf("Error opening %s\n", gainMediumDataFile);
        exit(EXIT_FAILURE);
    }

    for (i = 0; fscanf(fd, "%s %f %d %s\n", outputFile[i], &laserGain, &mainDischargePressure, carbonDioxide[i]) != EOF; i++) {
        smallSignalGain[i] = laserGain;
        dischargePressure[i] = mainDischargePressure;
    }

    if (fclose(fd) == EOF) {
        printf("Error closing %s\n", gainMediumDataFile);
        exit(EXIT_FAILURE);
    }

    return i;
}

void gaussianCalculation(int inputPower, float smallSignalGain, gaussian *gaussianData) {

    int i, j, saturationIntensity;
    float r;
    double *expr, *exprtemp;

    if ((exprtemp = expr = (double*) malloc(8 * 8001)) == NULL) {
        printf("Not enough memory to allocate buffer\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 8001; i++) {
        double zInc = ((double) i - 4000) / 25;
        *exprtemp = zInc * 2 * DZ / (Z12 + pow(zInc, 2));
        exprtemp++;
    }

    double inputIntensity = 2 * inputPower / AREA;
    double expr2 = (smallSignalGain / 32E3) * DZ;

    i = 0;
    for (saturationIntensity = 10E3; saturationIntensity <= 25E3; saturationIntensity += 1E3) {
        double outputPower = 0.0;
        double expr3 = saturationIntensity * expr2;
        for (r = 0; r <= 0.5; r += DR) {
            double outputIntensity = inputIntensity * exp(-2 * pow(r, 2) / pow(RAD, 2));
            exprtemp = expr;
            for (j = 0; j < 8001; j++) {
                outputIntensity *= (1 + expr3 / (saturationIntensity + outputIntensity) - *exprtemp);
                exprtemp++;
            }
            outputPower += (outputIntensity * EXPR * r);
        }

        gaussianData[i].inputPower = inputPower;
        gaussianData[i].saturationIntensity = saturationIntensity;
        gaussianData[i].outputPower = outputPower;
        i++;
    }

    free((double*) expr);
}
