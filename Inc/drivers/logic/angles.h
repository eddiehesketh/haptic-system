#include <math.h>
#include "stm32c031xx.h"
#include "convert.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

// typedef struct {
//     double roll;
//     double pitch;
//     double tilt;
// } haptic_data;

extern double roll;
extern double pitch;
extern double tilt;

double calculate_roll(int16_t a_y, int16_t a_z);
double calculate_pitch(int16_t a_x, int16_t a_y, int16_t a_z);
double calculate_tilt(double roll, double pitch);
bool read_mpu();

