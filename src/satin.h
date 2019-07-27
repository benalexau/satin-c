/* satin.h by Alan K Stewart
 Saturation Intensity Calculation */

#ifndef SATIN_H
#define SATIN_H

typedef struct Laser {
    char output_file[9];
    float small_signal_gain;
    unsigned int discharge_pressure;
    char carbon_dioxide[3];
} Laser;

typedef struct SatinProcessArgs {
    unsigned int pnum;
    unsigned int *input_powers;
    Laser laser_data;
} SatinProcessArgs;

typedef struct Gaussian {
    unsigned int input_power;
    unsigned int saturation_intensity;
    double output_power;
    double log_output_power_divided_by_input_power;
    double output_power_minus_input_power;
} Gaussian;

void calculate();
unsigned int get_input_powers(unsigned int **input_powers);
unsigned int get_laser_data(Laser **lasers);
void *process(void *arg);
unsigned int gaussian_calculation(unsigned int input_power, float small_signal_gain, Gaussian **gaussians);

#endif
