#include "angles.h"
#include <math.h>

double calculate_roll(int16_t a_y, int16_t a_z) {
    return atan2((double)a_y, (double)a_z);
}

double calculate_pitch(int16_t a_x, int16_t a_y, int16_t a_z) {
    return atan2((double)(-a_x), sqrt(pow((double)a_y, 2) + pow((double)a_y, 2)));
}

double calculate_tilt(double roll, double pitch) {
    return sqrt(pow(roll, 2) + pow(pitch, 2));
}