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

#define RAD    0.18
#define RAD2   (RAD * RAD)
#define W1     0.3
#define DR     0.002
#define DZ     0.04
#define LAMBDA 0.0106
#define PI     3.14159265358979323846
#define AREA   (PI * RAD2)
#define Z1     (PI * (W1 * W1) / LAMBDA)
#define Z12    (Z1 * Z1)
#define EXPR   (2 * PI * DR)
#define INCR   8001

int main(int argc, char** argv)
{
    struct timeval t1;
    struct timeval t2;

    gettimeofday(&t1, NULL);
    calculate();
    gettimeofday(&t2, NULL);

    double elapsed_time = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) / 1E6;
    printf("The time was %.3f seconds.\n", elapsed_time);
    pthread_exit(NULL);
}

void calculate()
{
    unsigned int i;
    int rc;
    unsigned int *input_powers;
    Laser *lasers;
    pthread_t *threads;
    SatinProcessArgs *process_args;

    unsigned int pnum = get_input_powers(&input_powers);
    unsigned int lnum = get_laser_data(&lasers);

    if ((threads = malloc(lnum * sizeof(pthread_t))) == NULL) {
        perror("Failed to allocate memory for threads");
        exit(EXIT_FAILURE);
    }

    if ((process_args = malloc(lnum * sizeof(SatinProcessArgs))) == NULL) {
        perror("Failed to allocate memory for process_args");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < lnum; i++) {
        process_args[i].pnum = pnum;
        process_args[i].input_powers = input_powers;
        process_args[i].laser_data = lasers[i];
        rc = pthread_create(&threads[i], NULL, process, &process_args[i]);
        if (rc) {
            printf("Failed to create thread %u: %d\n", i, rc);
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < lnum; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            printf("Failed to join thread %u: %d\n", i, rc);
            exit(EXIT_FAILURE);
        }
    }

    free(input_powers);
    free(lasers);
    free(process_args);
    free(threads);
}

unsigned int get_input_powers(unsigned int **input_powers)
{
    unsigned int i = 0;
    unsigned int j = 6;
    unsigned int *input_powers_ptr;
    char *input_power_file = "pin.dat";
    FILE *fp;

    if ((fp = fopen(input_power_file, "r")) == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", input_power_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((input_powers_ptr = malloc(j * sizeof(unsigned int))) == NULL) {
        perror("Failed to allocate memory for input_powers_ptr");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%u\n", &input_powers_ptr[i]) != EOF) {
        i++;
        if (i == j) {
            if ((input_powers_ptr = realloc(input_powers_ptr, (j *= 2) * sizeof(unsigned int))) == NULL) {
                perror("Failed to reallocate memory for input_powers_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *input_powers = input_powers_ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Failed to close %s: %s\n", input_power_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return i;
}

unsigned int get_laser_data(Laser **lasers)
{
    unsigned int i = 0;
    unsigned int j = 9;
    char *laser_data_file = "laser.dat";
    Laser *lasers_ptr;
    FILE *fp;

    if ((fp = fopen(laser_data_file, "r")) == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", laser_data_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((lasers_ptr = malloc(j * sizeof(Laser))) == NULL) {
        perror("Failed to allocate memory for lasers_ptr");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%s %f %u %s\n", lasers_ptr[i].output_file, &lasers_ptr[i].small_signal_gain,
                  &lasers_ptr[i].discharge_pressure, lasers_ptr[i].carbon_dioxide) != EOF) {
        i++;
        if (i == j) {
            if ((lasers_ptr = realloc(lasers_ptr, (j *= 2) * sizeof(Laser))) == NULL) {
                perror("Failed to reallocate memory for lasers_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *lasers = lasers_ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Failed to close %s: %s\n", laser_data_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return i;
}

void *process(void *arg)
{
    time_t the_time;
    SatinProcessArgs *process_args = (SatinProcessArgs*) arg;
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

    for (unsigned int i = 0; i < process_args->pnum; i++) {
        unsigned int gaussians_size = gaussian_calculation(process_args->input_powers[i], laser_data.small_signal_gain, &gaussians);
        for (unsigned int j = 0; j < gaussians_size; j++) {
            fprintf(fp, "%u\t\t%7.3f\t\t%u\t\t%5.3f\t\t%7.3f\n", gaussians[j].input_power, gaussians[j].output_power,
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
    printf("Created %s\n", output_file);
    return NULL;
}

unsigned int gaussian_calculation(unsigned int input_power, float small_signal_gain, Gaussian **gaussians)
{
    unsigned int k = 17;
    double *expr1;
    Gaussian *gaussians_ptr;

    if ((gaussians_ptr = malloc(k * sizeof(Gaussian))) == NULL) {
        perror("Failed to allocate memory for gaussians_ptr");
        exit(EXIT_FAILURE);
    }

    if ((expr1 = malloc(INCR * sizeof(double))) == NULL) {
        perror("Failed to allocate memory for expr1");
        exit(EXIT_FAILURE);
    }

    for (unsigned int i = 0; i < INCR; i++) {
        double z_inc = ((double) i - INCR / 2) / 25;
        expr1[i] = z_inc * 2 * DZ / (Z12 + pow(z_inc, 2));
    }

    double input_intensity = 2 * input_power / AREA;
    double expr2 = small_signal_gain / 32000 * DZ;

    unsigned int i = 0;
    for (unsigned int saturation_intensity = 10000; saturation_intensity <= 25000; saturation_intensity += 1000) {
        double expr3 = saturation_intensity * expr2;
        double output_power = 0.0;
        for (unsigned int r = 0; r <= 250; r++) {
            double r1 = r / 500.0;
            double output_intensity = input_intensity * exp(-2 * pow(r1, 2) / RAD2);
            for (unsigned int j = 0; j < INCR; j++) {
                output_intensity *= (1 + expr3 / (saturation_intensity + output_intensity) - expr1[j]);
            }
            output_power += output_intensity * EXPR * r1;
        }
        gaussians_ptr[i].input_power = input_power;
        gaussians_ptr[i].saturation_intensity = saturation_intensity;
        gaussians_ptr[i].output_power = output_power;
        gaussians_ptr[i].log_output_power_divided_by_input_power = log(output_power / input_power);
        gaussians_ptr[i].output_power_minus_input_power = output_power - input_power;
        i++;
        if (i == k) {
            if ((gaussians_ptr = realloc(gaussians_ptr, (k *= 2) * sizeof(Gaussian))) == NULL) {
                perror("Failed to reallocate memory for gaussians_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *gaussians = gaussians_ptr;

    free(expr1);
    return i;
}

