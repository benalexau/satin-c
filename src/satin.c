/* Program satin.c by Alan K Stewart
 Saturation Intensity Calculation */

#include "satin.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#define PI    3.14159265358979323846264338327950288L
#define RAD   18E-2
#define RAD2  (RAD * RAD)
#define W1    3E-1
#define DR    2E-3
#define DZ    4E-2
#define LAMDA 10.6E-3
#define AREA  (PI * RAD2)
#define Z1    (PI * (W1 * W1) / LAMDA)
#define Z12   (Z1 * Z1)
#define EXPR  (2 * PI * DR)
#define INCR  8001

int main(int argc, char *argv[])
{
    struct timeval t1;
    struct timeval t2;
    double elapsedTime;

    gettimeofday(&t1, NULL);
    if (argc > 1 && strcmp(argv[1], "-single") == 0) {
        calculate();
    } else {
        calculate_concurrently();
    }
    gettimeofday(&t2, NULL);

    elapsedTime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) / 1E6;
    printf("The time was %.3f seconds.\n", elapsedTime);
    return EXIT_SUCCESS;
}

void calculate()
{
    int i;
    int *inputPowers;
    laser *laserData;
    satin_process_args *process_args;

    int pNum = get_input_powers(&inputPowers);
    int lNum = get_laser_data(&laserData);

    if ((process_args = malloc(lNum * sizeof(satin_process_args))) == NULL) {
        perror("Failed to allocate memory for process_args");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < lNum; i++) {
        process_args[i].pnum = pNum;
        process_args[i].input_powers = inputPowers;
        process_args[i].laser_data = laserData[i];
        process(&process_args[i]);
    }

    free(inputPowers);
    free(laserData);
    free(process_args);
}

void calculate_concurrently()
{
    int i;
    int *input_powers;
    laser *laser_data;
    pthread_t *threads;
    satin_process_args *process_args;

    int pnum = get_input_powers(&input_powers);
    int lnum = get_laser_data(&laser_data);

    if ((threads = malloc(lnum * sizeof(pthread_t))) == NULL) {
        perror("Failed to allocate memory for threads");
        exit(EXIT_FAILURE);
    }

    if ((process_args = malloc(lnum * sizeof(satin_process_args))) == NULL) {
        perror("Failed to allocate memory for process_args");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < lnum; i++) {
        process_args[i].pnum = pnum;
        process_args[i].input_powers = input_powers;
        process_args[i].laser_data = laser_data[i];
        pthread_create(&threads[i], NULL, process, &process_args[i]);
    }

    for (i = 0; i < lnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join threads");
            exit(EXIT_FAILURE);
        }
    }

    free(input_powers);
    free(laser_data);
    free(process_args);
    free(threads);
}

int get_input_powers(int **input_powers)
{
    int i = 0;
    int j = 6;
    int *ptr;
    char *input_power_file = "pin.dat";
    FILE *fp;

    if ((fp = fopen(input_power_file, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", input_power_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ptr = malloc(j * sizeof(int))) == NULL) {
        perror("Failed to allocate memory for input_powers");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%d\n", &ptr[i]) != EOF) {
        i++;
        if (i == j) {
            if ((ptr = realloc(ptr, (j *= 2) * sizeof(int))) == NULL) {
                perror("Failed to reallocate memory");
                exit(EXIT_FAILURE);
            }
        }
    }

    *input_powers = ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", input_power_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return i;
}

#ifdef REGEX
int get_laser_data(laser **laser_data)
{
    int i = 0;
    int j = 9;
    int rc;
    int buf = 25;
    char *laser_data_file = "laser.dat";
    char *pattern = "((md|pi)[a-z]{2}\\.out)[ ]+([0-9]{2}\\.[0-9])[ ]+([0-9]+)[ ]+(MD|PI)";
    char *line;
    regex_t compiled;
    size_t nmatch = 6;
    regmatch_t *matchptr;
    laser *ptr;
    FILE *fp;

    if ((fp = fopen(laser_data_file, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", laser_data_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ptr = malloc(j * sizeof(laser))) == NULL) {
        perror("Failed to allocate memory for laser");
        exit(EXIT_FAILURE);
    }

    if ((matchptr = malloc(nmatch * sizeof(regmatch_t))) == NULL) {
        perror("Failed to allocate memory for regex pointer");
        exit(EXIT_FAILURE);
    }

    rc = regcomp(&compiled, pattern, REG_EXTENDED);
    if (rc != 0) {
        printf("Failed to compile regex: %d (%s)\n", rc, get_regerror(rc, &compiled));
        exit(rc);
    }

    if ((line = malloc(buf * sizeof(char))) == NULL) {
        perror("Failed to allocate memory for line");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, buf, fp) != NULL) {
        rc = regexec(&compiled, line, nmatch, matchptr, 0);
        if (rc != 0) {
            printf("Failed to execute regex: %d (%s)\n", rc, get_regerror(rc, &compiled));
            exit(rc);
        }

        line[matchptr[1].rm_eo] = 0;
        strcpy(ptr[i].output_file, line + matchptr[1].rm_so);

        line[matchptr[3].rm_eo] = 0;
        ptr[i].small_signal_gain = atof(line + matchptr[3].rm_so);

        line[matchptr[4].rm_eo] = 0;
        ptr[i].discharge_pressure = atoi(line + matchptr[4].rm_so);

        line[matchptr[5].rm_eo] = 0;
        strcpy(ptr[i].carbon_dioxide, line + matchptr[5].rm_so);

        i++;
        if (i == j) {
            if ((ptr = realloc(ptr, (j *= 2) * sizeof(laser))) == NULL) {
                perror("Failed to reallocate memory");
                exit(EXIT_FAILURE);
            }
        }
    }

    *laser_data = ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", laser_data_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Parsed %d records of %s with regex\n", i, laser_data_file);

    regfree(&compiled);
    free(line);
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
int get_laser_data(laser **laser_data)
{
    int i = 0;
    int j = 9;
    char *laser_data_file = "laser.dat";
    laser *ptr;
    FILE *fp;

    if ((fp = fopen(laser_data_file, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", laser_data_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ptr = malloc(j * sizeof(laser))) == NULL) {
        perror("Failed to allocate memory for laser");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%s %lf %d %s\n", ptr[i].output_file, &ptr[i].small_signal_gain, &ptr[i].discharge_pressure,
            ptr[i].carbon_dioxide) != EOF) {
        i++;
        if (i == j) {
            if ((ptr = realloc(ptr, (j *= 2) * sizeof(laser))) == NULL) {
                perror("Failed to reallocate memory");
                exit(EXIT_FAILURE);
            }
        }
    }

    *laser_data = ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", laser_data_file, strerror(errno));
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
    laser laser_data = process_args->laser_data;
    char *output_file = laser_data.output_file;
    gaussian *gaussians;
    FILE *fp;

    if ((fp = fopen(output_file, "w+")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", output_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    time(&the_time);
    fprintf(fp,
            "Start date: %s\nGaussian Beam\n\nPressure in Main Discharge = %dkPa\nSmall-signal Gain = %4.1f\nCO2 via %s\n\nPin\t\tPout\t\tSat. Int\tln(Pout/Pin)\tPout-Pin\n(watts)\t\t(watts)\t\t(watts/cm2)\t\t\t(watts)\n",
            ctime(&the_time), laser_data.discharge_pressure, laser_data.small_signal_gain, laser_data.carbon_dioxide);

    for (i = 0; i < process_args->pnum; i++) {
        int gaussiansSize = gaussian_calculation(process_args->input_powers[i], laser_data.small_signal_gain, &gaussians);
        for (j = 0; j < gaussiansSize; j++) {
            fprintf(fp, "%d\t\t%7.3f\t\t%d\t\t%5.3f\t\t%7.3f\n", gaussians[j].input_power, gaussians[j].output_power,
                    gaussians[j].saturation_intensity, gaussians[j].log_output_power_divided_by_input_power,
                    gaussians[j].ouput_power_minus_input_power);
        }
    }

    time(&the_time);
    fprintf(fp, "\nEnd date: %s\n", ctime(&the_time));
    fflush(fp);

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", output_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    free(gaussians);
    return NULL;
}

int gaussian_calculation(int input_power, float small_signal_gain, gaussian **gaussians)
{
    int i;
    int j;
    int k = 17;
    int saturation_intensity;
    double *expr1;
    double input_intensity;
    double expr2;
    double r;
    gaussian *gaussian_data;

    if ((gaussian_data = malloc(k * sizeof(gaussian))) == NULL) {
        perror("Failed to allocate memory for gaussian_data");
        exit(EXIT_FAILURE);
    }

    if ((expr1 = malloc(INCR * sizeof(double))) == NULL) {
        perror("Failed to allocate memory for expr1");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < INCR; i++) {
        double z_inc = ((double) i - INCR / 2) / 25;
        expr1[i] = z_inc * 2 * DZ / (Z12 + pow(z_inc, 2));
    }

    input_intensity = 2 * input_power / AREA;
    expr2 = small_signal_gain / 32E3 * DZ;

    i = 0;
    for (saturation_intensity = 10E3; saturation_intensity <= 25E3; saturation_intensity += 1E3) {
        double expr3 = saturation_intensity * expr2;
        double output_power = 0.0;
        for (r = 0.0; r <= 0.5; r += DR) {
            double output_intensity = input_intensity * exp(-2 * pow(r, 2) / RAD2);
            for (j = 0; j < INCR; j++) {
                output_intensity *= (1 + expr3 / (saturation_intensity + output_intensity) - expr1[j]);
            }
            output_power += output_intensity * EXPR * r;
        }
        gaussian_data[i].input_power = input_power;
        gaussian_data[i].saturation_intensity = saturation_intensity;
        gaussian_data[i].output_power = output_power;
        gaussian_data[i].log_output_power_divided_by_input_power = log(output_power / input_power);
        gaussian_data[i].ouput_power_minus_input_power = output_power - input_power;
        i++;
        if (i == k) {
            if ((gaussian_data = realloc(gaussian_data, (k *= 2) * sizeof(gaussian))) == NULL) {
                perror("Failed to reallocate memory");
                exit(EXIT_FAILURE);
            }
        }
    }

    *gaussians = gaussian_data;

    free(expr1);
    return i;
}
