#include "angles.h"
#include <math.h>

#define MPU1_ADDR (uint8_t)(0x68)
#define ACCEL_ADDRESS (uint8_t)(0x3B)
#define NO_BYTES 6

float roll = 0;
float pitch = 0;
float tilt = 0;


float calculate_roll(int16_t a_y, int16_t a_z) {
    return atan2f((float)a_y, (float)a_z);
}

float calculate_pitch(int16_t a_x, int16_t a_y, int16_t a_z) {
    return atan2f((float)(-a_x), sqrt((float)a_y * a_y + (float)a_z * a_z));
}

float calculate_tilt(float roll, float pitch) {
    return sqrt(pow(roll, 2) + pow(pitch, 2));
}

bool read_mpu() {
    uint8_t buf[NO_BYTES];

    if (i2c_master_readregs(I2C1, MPU1_ADDR, ACCEL_ADDRESS, buf, NO_BYTES)) {
        int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);   // X: 0x3B high, 0x3C low
        int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);   // Y: 0x3D, 0x3E
        int16_t az = (int16_t)((buf[4] << 8) | buf[5]);   // Z: 0x3F, 0x40

        // atan2 only uses the ratio, so raw counts work directly (no unit conversion needed)
        roll =  calculate_roll(ay, az);
        pitch = calculate_pitch(ax, ay, az);
        tilt =  calculate_tilt(roll, pitch);
    } else {
        I2C1->CR1 &= ~(1 << 0);   // reset I2C after a failed read
        I2C1->CR1 |=  (1 << 0);
        return false;
    }

    return true;


}
