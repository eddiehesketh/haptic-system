#include "angles.h"
#include <math.h>

#define ACCEL_ADDRESS (uint8_t)(0x3B) // ACCEL_XOUT_H
#define NO_BYTES 6               // ACCEL_XOUT_H .. ACCEL_ZOUT_L
#define RAD_TO_DEG (180.0f / 3.14159265f)

float roll = 0;
float pitch = 0;
float tilt = 0;

// Eqs. (1)-(3), returned in degrees. atan2f only uses the ratio of its arguments,
// so the raw accelerometer counts can be used directly (no unit conversion needed).
void calculate_angles(int16_t a_x, int16_t a_y, int16_t a_z, float *phi, float *theta, float *alpha) {
    float ax = (float)a_x, ay = (float)a_y, az = (float)a_z;

    *phi   = atan2f(ay, az) * RAD_TO_DEG; // (1) roll
    *theta = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG; // (2) pitch
    *alpha = sqrtf((*theta) * (*theta) + (*phi) * (*phi)); // (3) tilt
}

bool read_mpu(void) {
    uint8_t buf[NO_BYTES];

    if (!i2c_master_readregs(I2C1, MPU1_ADDR, ACCEL_ADDRESS, buf, NO_BYTES)) {
        I2C1->CR1 &= ~(1 << 0); // reset I2C after a failed read
        I2C1->CR1 |=  (1 << 0);
        return false;
    }

    int16_t ax = (int16_t)((buf[0] << 8) | buf[1]); // X: 0x3B high, 0x3C low
    int16_t ay = (int16_t)((buf[2] << 8) | buf[3]); // Y: 0x3D, 0x3E
    int16_t az = (int16_t)((buf[4] << 8) | buf[5]); // Z: 0x3F, 0x40

    calculate_angles(ax, ay, az, &roll, &pitch, &tilt);
    return true;
}
