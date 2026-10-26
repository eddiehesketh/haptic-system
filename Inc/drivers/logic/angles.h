#include <math.h>
#include <stdint.h>

double calculate_roll(int16_t a_y, int16_t a_z);
double calculate_pitch(int16_t ax, int16_t a_y, int16_t a_z);
double calculate_tilt(double roll, double pitch);