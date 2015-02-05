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
    double elapsed_time;

    gettimeofday(&t1, NULL);
    if (argc > 1 && strcmp(argv[1], "-single") == 0) {
        calculate();
    } else {
        calculate_concurrently();
    }
    gettimeofday(&t2, NULL);

    elapsed_time = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) / 1E6;
    printf("The time was %.3f seconds.\n", elapsed_time);
    pthread_exit(NULL);
    return EXIT_SUCCESS;
}

void calculate()
{
    int i;
    int *input_powers;
    laser *lasers;
    satin_process_args *process_args;

    int pNum = get_input_powers(&input_powers);
    int lNum = get_laser_data(&lasers);

    if ((process_args = malloc(lNum * sizeof(satin_process_args))) == NULL) {
        perror("Failed to allocate memory for process_args");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < lNum; i++) {
        process_args[i].pnum = pNum;
        process_args[i].input_powers = input_powers;
        process_args[i].laser_data = lasers[i];
        process(&process_args[i]);
    }

    free(input_powers);
    free(lasers);
    free(process_args);
}

void calculate_concurrently()
{
    int i;
    int rc;
    int *input_powers;
    laser *lasers;
    pthread_t *threads;
    satin_process_args *process_args;

    int pnum = get_input_powers(&input_powers);
    int lnum = get_laser_data(&lasers);

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
        process_args[i].laser_data = lasers[i];
        rc = pthread_create(&threads[i], NULL, process, &process_args[i]);
        if (rc) {
            printf("Failed to create thread %d, return code is %d\n", i, rc);
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < lnum; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            printf("Failed to join thread %d, error code is %d\n", i, rc);
            exit(EXIT_FAILURE);
        }
    }

    free(input_powers);
    free(lasers);
    free(process_args);
    free(threads);
}

int get_input_powers(int **input_powers)
{
    int i = 0;
    int j = 6;
    int *input_powers_ptr;
    char *input_power_file = "pin.dat";
    FILE *fp;

    if ((fp = fopen(input_power_file, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", input_power_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((input_powers_ptr = malloc(j * sizeof(int))) == NULL) {
        perror("Failed to allocate memory for input_powers_ptr");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%d\n", &input_powers_ptr[i]) != EOF) {
        i++;
        if (i == j) {
            if ((input_powers_ptr = realloc(input_powers_ptr, (j *= 2) * sizeof(int))) == NULL) {
                perror("Failed to reallocate memory for input_powers_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *input_powers = input_powers_ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", input_power_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return i;
}

#ifdef REGEX
int get_laser_data(laser **lasers)
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
    regmatch_t *match_ptr;
    laser *lasers_ptr;
    FILE *fp;

    if ((fp = fopen(laser_data_file, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", laser_data_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((lasers_ptr = malloc(j * sizeof(laser))) == NULL) {
        perror("Failed to allocate memory for lasers_ptr");
        exit(EXIT_FAILURE);
    }

    if ((match_ptr = malloc(nmatch * sizeof(regmatch_t))) == NULL) {
        perror("Failed to allocate memory for regex match pointer");
        exit(EXIT_FAILURE);
    }

    rc = regcomp(&compiled, pattern, REG_EXTENDED);
    if (rc) {
        printf("Failed to compile regex: %d (%s)\n", rc, get_regerror(rc, &compiled));
        exit(EXIT_FAILURE);
    }

    if ((line = malloc(buf * sizeof(char))) == NULL) {
        perror("Failed to allocate memory for line");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, buf, fp) != NULL) {
        rc = regexec(&compiled, line, nmatch, match_ptr, 0);
        if (rc) {
            printf("Failed to execute regex: %d (%s)\n", rc, get_regerror(rc, &compiled));
            exit(rc);
        }

        line[match_ptr[1].rm_eo] = 0;
        strcpy(lasers_ptr[i].output_file, line + match_ptr[1].rm_so);

        line[match_ptr[3].rm_eo] = 0;
        lasers_ptr[i].small_signal_gain = atof(line + match_ptr[3].rm_so);

        line[match_ptr[4].rm_eo] = 0;
        lasers_ptr[i].discharge_pressure = atoi(line + match_ptr[4].rm_so);

        line[match_ptr[5].rm_eo] = 0;
        strcpy(lasers_ptr[i].carbon_dioxide, line + match_ptr[5].rm_so);

        i++;
        if (i == j) {
            if ((lasers_ptr = realloc(lasers_ptr, (j *= 2) * sizeof(laser))) == NULL) {
                perror("Failed to reallocate memory for lasers_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *lasers = lasers_ptr;

    if (fclose(fp) == EOF) {
        fprintf(stderr, "Error closing %s: %s\n", laser_data_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Parsed %d records of %s with regex\n", i, laser_data_file);

    regfree(&compiled);
    free(line);
    free(match_ptr);
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
int get_laser_data(laser **lasers)
{
    int i = 0;
    int j = 9;
    char *laser_data_file = "laser.dat";
    laser *lasers_ptr;
    FILE *fp;

    if ((fp = fopen(laser_data_file, "r")) == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", laser_data_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((lasers_ptr = malloc(j * sizeof(laser))) == NULL) {
        perror("Failed to allocate memory for lasers_ptr");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%s %lf %d %s\n", lasers_ptr[i].output_file, &lasers_ptr[i].small_signal_gain,
            &lasers_ptr[i].discharge_pressure, lasers_ptr[i].carbon_dioxide) != EOF) {
        i++;
        if (i == j) {
            if ((lasers_ptr = realloc(lasers_ptr, (j *= 2) * sizeof(laser))) == NULL) {
                perror("Failed to reallocate memory for lasers_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *lasers = lasers_ptr;

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
    satin_process_args *process_args = (satin_process_args*) arg;
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
        int gaussians_size = gaussian_calculation(process_args->input_powers[i], laser_data.small_signal_gain, &gaussians);
        for (j = 0; j < gaussians_size; j++) {
            fprintf(fp, "%d\t\t%7.3f\t\t%d\t\t%5.3f\t\t%7.3f\n", gaussians[j].input_power, gaussians[j].output_power,
                    gaussians[j].saturation_intensity, gaussians[j].log_output_power_divided_by_input_power,
                    gaussians[j].output_power_minus_input_power);
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
    gaussian *gaussians_ptr;

    if ((gaussians_ptr = malloc(k * sizeof(gaussian))) == NULL) {
        perror("Failed to allocate memory for gaussians_ptr");
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
        gaussians_ptr[i].input_power = input_power;
        gaussians_ptr[i].saturation_intensity = saturation_intensity;
        gaussians_ptr[i].output_power = output_power;
        gaussians_ptr[i].log_output_power_divided_by_input_power = log(output_power / input_power);
        gaussians_ptr[i].output_power_minus_input_power = output_power - input_power;
        i++;
        if (i == k) {
            if ((gaussians_ptr = realloc(gaussians_ptr, (k *= 2) * sizeof(gaussian))) == NULL) {
                perror("Failed to reallocate memory for gaussians_ptr");
                exit(EXIT_FAILURE);
            }
        }
    }

    *gaussians = gaussians_ptr;

    free(expr1);
    return i;
}
