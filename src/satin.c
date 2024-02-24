/* Program satin.c by Alan K Stewart
 Saturation Intensity Calculation */

#include "satin.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define RAD        0.18
#define RAD2       (RAD * RAD)
#define W1         0.3
#define DR         0.002
#define DZ         0.04
#define LAMBDA     0.0106
#define PI         3.14159265358979323846
#define AREA       (PI * RAD2)
#define Z1         (PI * (W1 * W1) / LAMBDA)
#define Z12        (Z1 * Z1)
#define EXPR       (2 * PI * DR)
#define INCR       8001
#define INPUT_FILE "pin.dat"
#define LASER_FILE "laser.dat"

void calculate();

int get_input_powers(int **input_powers);

int get_laser_data(Laser **lasers);

void *process(void *arg);

int gaussian_calculation(int input_power, float small_signal_gain, Gaussian **gaussians);

int main() {
    struct timespec t1, t2;

    clock_gettime(CLOCK_MONOTONIC, &t1);
    calculate();
    clock_gettime(CLOCK_MONOTONIC, &t2);

    double elapsed_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1E9;
    printf("The time was %.3f seconds.\n", elapsed_time);
    pthread_exit(NULL);
}

void calculate() {
    int i;
    int rc;
    int *input_powers;
    Laser *lasers;
    pthread_t *threads;
    SatinProcessArgs *process_args;

    int pnum = get_input_powers(&input_powers);
    int lnum = get_laser_data(&lasers);

    if ((threads = calloc(lnum, sizeof(pthread_t))) == NULL) {
        perror("Failed to allocate memory for threads");
        exit(EXIT_FAILURE);
    }

    if ((process_args = calloc(lnum, sizeof(SatinProcessArgs))) == NULL) {
        perror("Failed to allocate memory for process_args");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < lnum; i++) {
        process_args[i].pnum = pnum;
        process_args[i].input_powers = input_powers;
        process_args[i].laser_data = lasers[i];
        rc = pthread_create(&threads[i], NULL, process, &process_args[i]);
        if (rc) {
            fprintf(stderr, "Failed to create thread %d: %d\n", i, rc);
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < lnum; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            fprintf(stderr, "Failed to join thread %d: %d\n", i, rc);
            exit(EXIT_FAILURE);
        }
    }

    free(input_powers);
    free(lasers);
    free(process_args);
    free(threads);
}

int get_input_powers(int **input_powers) {
    int i = 0;
    int max_size = 6;
    int *input_powers_ptr;
    FILE *fp;

    if ((fp = fopen(INPUT_FILE, "r")) == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", INPUT_FILE, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((input_powers_ptr = calloc(max_size, sizeof(int))) == NULL) {
        perror("Failed to allocate memory for input_powers_ptr");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%d\n", &input_powers_ptr[i]) != EOF) {
        i++;
        if (i == max_size) {
            if ((input_powers_ptr = realloc(input_powers_ptr, (max_size *= 2) * sizeof(int))) == NULL) {
                perror("Failed to reallocate memory for input_powers_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *input_powers = input_powers_ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Failed to close %s: %s\n", INPUT_FILE, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return i;
}

int get_laser_data(Laser **lasers) {
    int i = 0;
    int max_size = 9;
    Laser *lasers_ptr;
    FILE *fp;

    if ((fp = fopen(LASER_FILE, "r")) == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", LASER_FILE, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((lasers_ptr = calloc(max_size, sizeof(Laser))) == NULL) {
        perror("Failed to allocate memory for lasers_ptr");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%s %f %d %s\n", lasers_ptr[i].output_file, &lasers_ptr[i].small_signal_gain,
                  &lasers_ptr[i].discharge_pressure, lasers_ptr[i].carbon_dioxide) != EOF) {
        i++;
        if (i == max_size) {
            if ((lasers_ptr = realloc(lasers_ptr, (max_size *= 2) * sizeof(Laser))) == NULL) {
                perror("Failed to reallocate memory for lasers_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *lasers = lasers_ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Failed to close %s: %s\n", LASER_FILE, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return i;
}

void *process(void *arg) {
    time_t the_time;
    SatinProcessArgs *process_args = (SatinProcessArgs *) arg;
    Laser laser_data = process_args->laser_data;
    char *output_file = laser_data.output_file;
    Gaussian *gaussians = NULL;
    FILE *fp;

    if ((fp = fopen(output_file, "w+")) == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", output_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    time(&the_time);
    fprintf(fp,
            "Start date: %s\nGaussian Beam\n\nPressure in Main Discharge = %dkPa\nSmall-signal Gain = %4.1f\nCO2 via %s\n\nPin\t\tPout\t\tSat. Int\tln(Pout/Pin)\tPout-Pin\n(watts)\t\t(watts)\t\t(watts/cm2)\t\t\t(watts)\n",
            ctime(&the_time), laser_data.discharge_pressure, laser_data.small_signal_gain, laser_data.carbon_dioxide);

    for (int i = 0; i < process_args->pnum; i++) {
        int gaussians_size = gaussian_calculation(process_args->input_powers[i], laser_data.small_signal_gain, &gaussians);
        for (int j = 0; j < gaussians_size; j++) {
            fprintf(fp, "%u\t\t%7.3f\t\t%u\t\t%5.3f\t\t%7.3f\n", gaussians[j].input_power,
                    gaussians[j].output_power,
                    gaussians[j].saturation_intensity, gaussians[j].log_output_power_divided_by_input_power,
                    gaussians[j].output_power_minus_input_power);
        }
    }

    time(&the_time);
    fprintf(fp, "\nEnd date: %s\n", ctime(&the_time));
    fflush(fp);

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Failed to close %s: %s\n", output_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    free(gaussians);
    return NULL;
}

int gaussian_calculation(int input_power, float small_signal_gain, Gaussian **gaussians) {
    int max_size = 17;
    double *expr1;
    Gaussian *gaussians_ptr;

    if ((gaussians_ptr = calloc(max_size, sizeof(Gaussian))) == NULL) {
        perror("Failed to allocate memory for gaussians_ptr");
        exit(EXIT_FAILURE);
    }

    if ((expr1 = calloc(INCR, sizeof(double))) == NULL) {
        perror("Failed to allocate memory for expr1");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < INCR; i++) {
        double z_inc = ((double) i - INCR / 2) / 25;
        expr1[i] = z_inc * 2 * DZ / (Z12 + pow(z_inc, 2));
    }

    int i = 0;
    for (int saturation_intensity = 10000; saturation_intensity <= 25000; saturation_intensity += 1000) {
        gaussians_ptr[i].input_power = input_power;
        gaussians_ptr[i].saturation_intensity = saturation_intensity;
        gaussians_ptr[i].output_power = calculate_output_power(input_power, small_signal_gain, saturation_intensity);
        gaussians_ptr[i].log_output_power_divided_by_input_power = log(gaussians_ptr[i].output_power / input_power);
        gaussians_ptr[i].output_power_minus_input_power = gaussians_ptr[i].output_power - input_power;
        i++;
        if (i == max_size) {
            if ((gaussians_ptr = realloc(gaussians_ptr, (max_size *= 2) * sizeof(Gaussian))) == NULL) {
                perror("Failed to reallocate memory for gaussians_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *gaussians = gaussians_ptr;

    free(expr1);
    return i;
}

double calculate_output_power(int input_power, float small_signal_gain, int saturation_intensity) {
    double *expr1;
    if ((expr1 = calloc(INCR, sizeof(double))) == NULL) {
        perror("Failed to allocate memory for expr1");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < INCR; i++) {
        double z_inc = ((double) i - INCR / 2) / 25;
        expr1[i] = z_inc * 2 * DZ / (Z12 + pow(z_inc, 2));
    }

    double input_intensity = 2 * input_power / AREA;
    double expr2 = small_signal_gain / 32000 * DZ;

    double output_power = 0.0;
    for (int r = 0; r <= 250; r++) {
        double r1 = r / 500.0;
        double output_intensity = input_intensity * exp(-2 * pow(r1, 2) / RAD2);
        for (int j = 0; j < INCR; j++) {
            output_intensity *= (1 + saturation_intensity * expr2 / (saturation_intensity + output_intensity) - expr1[j]);
        }
        output_power += output_intensity * EXPR * r1;
    }

    free(expr1);
    return output_power;
}
