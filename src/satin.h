/* satin.h by Alan K Stewart
 Saturation Intensity Calculation */

#ifndef SATIN_H
#define SATIN_H

#ifdef REGEX
#include <regex.h>
#endif

typedef struct laser {
    char output_file[9];
    double small_signal_gain;
    int discharge_pressure;
    char carbon_dioxide[3];
} laser;

typedef struct satin_process_args {
    int pnum;
    int *input_powers;
    laser laser_data;
} satin_process_args;

typedef struct gaussian {
    int input_power;
    int saturation_intensity;
    double output_power;
    double log_output_power_divided_by_input_power;
    double output_power_minus_input_power;
} gaussian;

void calculate();
void calculate_concurrently();
int get_input_powers(int **input_powers);
int get_laser_data(laser **lasers);
#ifdef REGEX
char *get_regerror(int errcode, regex_t *preg);
#endif
void *process(void *arg);
int gaussian_calculation(int input_power, float small_signal_gain, gaussian **gaussians);

#endif
