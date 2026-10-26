#pragma once

#include <math.h>
#include "stm32c031xx.h"
#include "convert.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

// typedef struct {
//     float roll;
//     float pitch;
//     float tilt;
// } haptic_data;

extern float roll;
extern float pitch;
extern float tilt;

float calculate_roll(int16_t a_y, int16_t a_z);
float calculate_pitch(int16_t a_x, int16_t a_y, int16_t a_z);
float calculate_tilt(float roll, float pitch);
uint16_t convert_to_binary(float);
bool read_mpu();

