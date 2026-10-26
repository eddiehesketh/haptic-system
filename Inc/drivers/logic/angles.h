#pragma once

#include <math.h>
#include "stm32c031xx.h"
#include "convert.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

// Latest orientation angles in DEGREES, updated by read_mpu()
extern float roll;  // phi   = atan2(a_y, a_z)                  Eq. (1)
extern float pitch; // theta = atan2(-a_x, sqrt(a_y^2 + a_z^2)) Eq. (2)
extern float tilt;  // alpha = sqrt(theta^2 + phi^2)            Eq. (3)

// Subroutine that calculates roll, pitch and tilt (degrees) from the accelerometer components
void calculate_angles(int16_t a_x, int16_t a_y, int16_t a_z, float *phi, float *theta, float *alpha);

// Read the accelerometer over I2C and update roll, pitch, tilt. Returns false if the read failed.
bool read_mpu(void);
