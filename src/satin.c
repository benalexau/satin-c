/* Program satin.c by Alan K Stewart
 Saturation Intensity Calculation */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "satin.h"

#define PI    3.14159265358979323846
#define RAD   18E-2
#define W1    3E-1
#define DR    2E-3
#define DZ    4E-2
#define LAMDA 10.6E-3
#define AREA  (PI * (RAD * RAD))
#define Z1    (PI * (W1 * W1) / LAMDA)
#define Z12   Z1 * Z1
#define EXPR  2 * PI * DR
#define INCR  8001

int main(int argc, char* argv[])
{
    struct timeval t1;
    struct timeval t2;
    double elapsedTime;

    gettimeofday(&t1, NULL);
    if (argc > 1 && strcmp(argv[1], "-concurrent") == 0) {
        calculateConcurrently();
    } else {
        calculate();
    }
    gettimeofday(&t2, NULL);

    elapsedTime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) / 1E6;
    printf("The time was %6.3f seconds.\n", elapsedTime);
    return EXIT_SUCCESS;
}

void calculateConcurrently()
{
    int i;
    int pNum;
    int *inputPowers;
    int lNum;
    laser *laserData;
    pthread_t *threads;
    satin_process_args *process_args;

    pNum = getInputPowers(&inputPowers);
    lNum = getLaserData(&laserData);

    if ((threads = malloc(lNum * sizeof(pthread_t))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    if ((process_args = malloc(lNum * sizeof(satin_process_args))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < lNum; i++) {
        process_args[i].pNum = pNum;
        process_args[i].inputPowers = inputPowers;
        process_args[i].laserData = laserData[i];
        pthread_create(&threads[i], NULL, process, &process_args[i]);
    }

    for (i = 0; i < lNum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join threads");
            exit(EXIT_FAILURE);
        }
    }

    free(inputPowers);
    free(laserData);
    free(process_args);
    free(threads);
}

void calculate()
{
    int i;
    int pNum;
    int *inputPowers;
    int lNum;
    laser *laserData;
    satin_process_args *process_args;

    pNum = getInputPowers(&inputPowers);
    lNum = getLaserData(&laserData);

    if ((process_args = malloc(lNum * sizeof(satin_process_args))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < lNum; i++) {
        process_args[i].pNum = pNum;
        process_args[i].inputPowers = inputPowers;
        process_args[i].laserData = laserData[i];
        process(&process_args[i]);
    }

    free(inputPowers);
    free(laserData);
    free(process_args);
}

int getInputPowers(int **inputPowers)
{
    int i = 0;
    int j = N;
    int *ptr;
    char *inputPowerFile = "pin.dat";
    FILE *fd;

    if ((fd = fopen(inputPowerFile, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", inputPowerFile,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ptr = malloc(j * sizeof(int))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    while (fscanf(fd, "%d\n", &ptr[i]) != EOF) {
        i++;
        if (i == j && (ptr = realloc(ptr, (j *= 2) * sizeof(int))) == NULL) {
            perror("Failed to reallocate memory");
            exit(EXIT_FAILURE);
        }
    }

    *inputPowers = ptr;

    if (fclose(fd) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", inputPowerFile,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return i;
}

#ifdef REGEX
int getLaserData(laser **laserData)
{
    int i = 0;
    int j = N;
    int rc = 0;
    char *laserDataFile = "laser.dat";
    char *line;
    char *lineCopy;
    char *pattern =
            "((md|pi)[a-z]{2}\\.out)[ ]+([0-9]{2}\\.[0-9])[ ]+([0-9]+)[ ]+(MD|PI)";
    regex_t compiled;
    size_t nmatch = 6;
    regmatch_t *matchptr;
    laser *ptr;
    FILE *fd;

    if ((fd = fopen(laserDataFile, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", laserDataFile,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ptr = malloc(j * sizeof(laser))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    if ((matchptr = malloc(nmatch * sizeof(regmatch_t))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    rc = regcomp(&compiled, pattern, REG_EXTENDED);
    if (rc != 0) {
        printf("Failed to compile regex: %d (%s)\n", rc,
                get_regerror(rc, &compiled));
        exit(rc);
    }

    if ((line = malloc(BUF * sizeof(char))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    while (fgets(line, BUF, fd) != NULL) {
        rc = regexec(&compiled, line, nmatch, matchptr, 0);
        if (rc != 0) {
            printf("Failed to execute regex: %d (%s)\n", rc,
                    get_regerror(rc, &compiled));
            exit(rc);
        }

        if ((lineCopy = strdup(line)) == NULL) {
            perror(ERR);
            exit(EXIT_FAILURE);
        }

        lineCopy[matchptr[1].rm_eo] = 0;
        strcpy(ptr[i].outputFile, lineCopy + matchptr[1].rm_so);

        lineCopy[matchptr[3].rm_eo] = 0;
        ptr[i].smallSignalGain = atof(lineCopy + matchptr[3].rm_so);

        lineCopy[matchptr[4].rm_eo] = 0;
        ptr[i].dischargePressure = atoi(lineCopy + matchptr[4].rm_so);

        lineCopy[matchptr[5].rm_eo] = 0;
        strcpy(ptr[i].carbonDioxide, lineCopy + matchptr[5].rm_so);

        i++;
        if (i == j && (ptr = realloc(ptr, (j *= 2) * sizeof(laser))) == NULL) {
            perror("Failed to reallocate memory");
            exit(EXIT_FAILURE);
        }
    }

    *laserData = ptr;

    if (fclose(fd) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", laserDataFile,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    regfree(&compiled);
    free(line);
    free(lineCopy);
    free(matchptr);
    return i;
}

char *get_regerror(int errcode, regex_t *compiled)
{
    size_t length = regerror(errcode, compiled, NULL, 0);
    char *buffer = malloc(length);
    (void) regerror(errcode, compiled, buffer, length);
    return buffer;
}

#else
int getLaserData(laser **laserData)
{
    int i = 0;
    int j = N;
    char *laserDataFile = "laser.dat";
    laser *ptr;
    FILE *fd;

    if ((fd = fopen(laserDataFile, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", laserDataFile,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ptr = malloc(j * sizeof(laser))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    while (fscanf(fd, "%s %lf %d %s\n", ptr[i].outputFile,
            &ptr[i].smallSignalGain, &ptr[i].dischargePressure,
            ptr[i].carbonDioxide) != EOF) {
        i++;
        if (i == j && (ptr = realloc(ptr, (j *= 2) * sizeof(laser))) == NULL) {
            perror("Failed to reallocate memory");
            exit(EXIT_FAILURE);
        }
    }

    *laserData = ptr;

    if (fclose(fd) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", laserDataFile,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return i;
}
#endif

void *process(void *arg)
{
    int i;
    int j;
    time_t the_time;
    satin_process_args* process_args = (satin_process_args*) arg;
    laser laserData = process_args->laserData;
    char *outputFile = laserData.outputFile;
    gaussian *gaussianData;
    FILE *fd;

    if ((fd = fopen(outputFile, "w+")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", outputFile, strerror(errno));
        exit(EXIT_FAILURE);
    }

    time(&the_time);
    fprintf(fd,
            "Start date: %s\nGaussian Beam\n\nPressure in Main Discharge = %dkPa\nSmall-signal Gain = %4.1f\nCO2 via %s\n\nPin\t\tPout\t\tSat. Int\tln(Pout/Pin)\tPout-Pin\n(watts)\t\t(watts)\t\t(watts/cm2)\t\t\t(watts)\n",
            ctime(&the_time), laserData.dischargePressure,
            laserData.smallSignalGain, laserData.carbonDioxide);

    for (i = 0; i < process_args->pNum; i++) {
        int gaussianDataSize = gaussianCalculation(process_args->inputPowers[i],
                laserData.smallSignalGain, &gaussianData);
        for (j = 0; j < gaussianDataSize; j++) {
            int inputPower = gaussianData[j].inputPower;
            double outputPower = gaussianData[j].outputPower;
            fprintf(fd, "%d\t\t%7.3f\t\t%d\t\t%5.3f\t\t%7.3f\n", inputPower,
                    outputPower, gaussianData[j].saturationIntensity,
                    log(outputPower / inputPower), outputPower - inputPower);
        }
    }

    time(&the_time);
    fprintf(fd, "\nEnd date: %s\n", ctime(&the_time));
    fflush(fd);

    if (fclose(fd) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", outputFile, strerror(errno));
        exit(EXIT_FAILURE);
    }

    free(gaussianData);
    return NULL;
}

int gaussianCalculation(int inputPower, float smallSignalGain,
        gaussian **gaussianData)
{
    int i;
    int j;
    int saturationIntensity;
    double *expr1;
    double inputIntensity;
    double expr2;
    double r;
    gaussian *gaussians;

    if ((gaussians = malloc(
            16 /* total saturationIntensity increments */* sizeof(gaussian)))
            == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    if ((expr1 = malloc(INCR * sizeof(double))) == NULL) {
        perror(ERR);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < INCR; i++) {
        double zInc = ((double) i - INCR / 2) / 25;
        expr1[i] = zInc * 2 * DZ / (Z12 + pow(zInc, 2));
    }

    inputIntensity = 2 * inputPower / AREA;
    expr2 = (smallSignalGain / 32E3) * DZ;

    i = 0;
    for (saturationIntensity = 10E3; saturationIntensity <= 25E3;
            saturationIntensity += 1E3) {
        double outputPower = 0.0;
        double expr3 = saturationIntensity * expr2;
        for (r = 0.0; r <= 0.5; r += DR) {
            double outputIntensity = inputIntensity
                    * exp(-2 * pow(r, 2) / pow(RAD, 2));
            for (j = 0; j < INCR; j++) {
                outputIntensity *= (1
                        + expr3 / (saturationIntensity + outputIntensity)
                        - expr1[j]);
            }
            outputPower += (outputIntensity * EXPR * r);
        }
        gaussians[i].inputPower = inputPower;
        gaussians[i].saturationIntensity = saturationIntensity;
        gaussians[i].outputPower = outputPower;
        i++;
    }

    *gaussianData = gaussians;

    free(expr1);
    return i;
}
