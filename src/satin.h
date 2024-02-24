/* satin.h by Alan K Stewart
 Saturation Intensity Calculation */

#pragma once

typedef struct Laser {
    char output_file[9];
    float small_signal_gain;
    int discharge_pressure;
    char carbon_dioxide[3];
} Laser;

typedef struct SatinProcessArgs {
    int pnum;
    int *input_powers;
    Laser laser_data;
} SatinProcessArgs;

typedef struct Gaussian {
    int input_power;
    double output_power;
    int saturation_intensity;
} Gaussian;

void calculate();
int get_input_powers(int **input_powers);
int get_laser_data(Laser **lasers);
void *process(void *arg);
int gaussian_calculation(int input_power, float small_signal_gain, Gaussian **gaussians);
double calculate_output_power(int input_power, float small_signal_gain, int saturation_intensity);
