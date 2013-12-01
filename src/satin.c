/* Program satin.c by Alan K Stewart
 Saturation Intensity Calculation */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#define N     10
#define RAD   18E-2
#define W1    3E-1
#define DR    2E-3
#define DZ    4E-2
#define LAMDA 10.6E-3
#define AREA  (M_PI * (RAD * RAD))
#define Z1    (M_PI * (W1 * W1) / LAMDA)
#define Z12   Z1 * Z1
#define EXPR  2 * M_PI * DR
#define INCR  8001

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
    int pNum, inputPowers[N], count;
    laser laserData;
} satin_process_args;

_Bool calculate(_Bool concurrent);
int getInputPowers(int inputPowers[]);
int getLaserData(laser laserData[]);
void *process(void *arg);
void gaussianCalculation(int inputPower, float smallSignalGain, gaussian *gaussianData);

int main(int argc, char* argv[]) {

    int rc = 0;
    struct timeval tp;
    double start, end;

    gettimeofday(&tp, NULL);
    start = tp.tv_sec + (tp.tv_usec / 1E6);
    if (!calculate(argc > 1 && strcmp(argv[1], "-concurrent") == 0)) {
        printf("Failed to complete\n");
        rc = 1;
    }
    gettimeofday(&tp, NULL);
    end = tp.tv_sec + (tp.tv_usec / 1E6);
    printf("The time was %6.3f seconds.\n", end - start);
    return rc;
}

_Bool calculate(_Bool concurrent) {

    int i, pNum, lNum, inputPowers[N], total;
    laser laserData[N];

    pNum = getInputPowers(inputPowers);
    lNum = getLaserData(laserData);

    pthread_t threads[lNum];
    satin_process_args process_args[lNum];

    total = 0;
    for (i = 0; i < lNum; i++) {
        process_args[i].pNum = pNum;
        memcpy(process_args[i].inputPowers, inputPowers, sizeof(inputPowers));
        process_args[i].laserData = laserData[i];

        if (concurrent) {
            pthread_create(&threads[i], NULL, process, &process_args[i]);
        } else {
            process(&process_args[i]);
            total += process_args[i].count;
        }
    }

    if (concurrent) {
        for (i = 0; i < lNum; i++) {
            if (pthread_join(threads[i], NULL) == 0) {
                total += process_args[i].count;
            } else {
                exit(EXIT_FAILURE);
            }
        }
    }

    return total == pNum * lNum;
}

int getInputPowers(int inputPowers[]) {

    int i = 0;
    char *inputPowerFile = "pin.dat";
    FILE *fd;

    if ((fd = fopen(inputPowerFile, "r")) == NULL) {
        printf("Error opening %s\n", inputPowerFile);
        exit(EXIT_FAILURE);
    }

    while (fscanf(fd, "%d \n", &inputPowers[i]) != EOF) {
        i++;
    }

    if (fclose(fd) == EOF) {
        printf("Error closing %s\n", inputPowerFile);
        exit(EXIT_FAILURE);
    }

    return i;
}

int getLaserData(laser laserData[]) {

    int i = 0;
    char *gainMediumDataFile = "laser.dat";
    FILE *fd;

    if ((fd = fopen(gainMediumDataFile, "r")) == NULL) {
        printf("Error opening %s\n", gainMediumDataFile);
        exit(EXIT_FAILURE);
    }

    while (fscanf(fd, "%s %f %d %s\n", laserData[i].outputFile, &laserData[i].smallSignalGain,
            &laserData[i].dischargePressure, laserData[i].carbonDioxide) != EOF) {
        i++;
    }

    if (fclose(fd) == EOF) {
        printf("Error closing %s\n", gainMediumDataFile);
        exit(EXIT_FAILURE);
    }

    return i;
}

void *process(void *arg) {

    int i, j, count;
    time_t the_time;
    satin_process_args* process_args = (satin_process_args*) arg;
    laser laserData = process_args->laserData;
    char *outputFile = laserData.outputFile;
    gaussian *gaussianData = malloc(16 * sizeof(gaussian));
    FILE *fd;

    if ((fd = fopen(outputFile, "w+")) == NULL) {
        printf("Error opening %s\n", outputFile);
        exit(EXIT_FAILURE);
    }

    time(&the_time);
    fprintf(fd,
            "Start date: %s\nGaussian Beam\n\nPressure in Main Discharge = %dkPa\nSmall-signal Gain = %4.1f\nCO2 via %s\n\nPin\t\tPout\t\tSat. Int\tln(Pout/Pin)\tPout-Pin\n(watts)\t\t(watts)\t\t(watts/cm2)\t\t\t(watts)\n",
            ctime(&the_time), laserData.dischargePressure, laserData.smallSignalGain, laserData.carbonDioxide);

    count = 0;
    for (i = 0; i < process_args->pNum; i++) {
        gaussianCalculation(process_args->inputPowers[i], laserData.smallSignalGain, gaussianData);
        for (j = 0; j < sizeof(*gaussianData); j++) {
            int inputPower = gaussianData[j].inputPower;
            double outputPower = gaussianData[j].outputPower;
            fprintf(fd, "%d\t\t%7.3f\t\t%d\t\t%5.3f\t\t%7.3f\n", inputPower, outputPower,
                    gaussianData[j].saturationIntensity, log(outputPower / inputPower), outputPower - inputPower);
        }
        count++;
    }
    process_args->count = count;

    free(gaussianData);
    gaussianData = NULL;

    time(&the_time);
    fprintf(fd, "\nEnd date: %s\n", ctime(&the_time));
    fflush(fd);

    if (fclose(fd) == EOF) {
        printf("Error closing %s\n", outputFile);
        exit(EXIT_FAILURE);
    }

    return NULL;
}

void gaussianCalculation(int inputPower, float smallSignalGain, gaussian *gaussianData) {

    int i, j, saturationIntensity;
    float r;
    double *expr, *exprtemp;

    if ((exprtemp = expr = (double*) malloc(8 * INCR)) == NULL) {
        printf("Not enough memory to allocate buffer\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < INCR; i++) {
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
        for (r = 0.0; r <= 0.5f; r += DR) {
            double outputIntensity = inputIntensity * exp(-2 * pow(r, 2) / pow(RAD, 2));
            exprtemp = expr;
            for (j = 0; j < INCR; j++) {
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

    free(expr);
    expr = exprtemp = NULL;
}
