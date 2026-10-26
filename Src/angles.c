#include "angles.h"
#include <math.h>

#define MPU1_ADDR (uint8_t)(0x68)
#define ACCEL_BYTE_SIZE 6
#define ACCEL_ADDRESS (uint8_t)(0x3B)
#define NO_BYTES 6

double roll = 0;
double pitch = 0;
double tilt = 0;


double calculate_roll(int16_t a_y, int16_t a_z) {
    return atan2((double)a_y, (double)a_z);
}

double calculate_pitch(int16_t a_x, int16_t a_y, int16_t a_z) {
    return atan2((double)(-a_x), sqrt(pow((double)a_y, 2) + pow((double)a_y, 2)));
}

double calculate_tilt(double roll, double pitch) {
    return sqrt(pow(roll, 2) + pow(pitch, 2));
}

bool read_mpu() {
    uint8_t buf[NO_BYTES];

    if (i2c_master_readregs(I2C1, MPU1_ADDR, ACCEL_ADDRESS, buf, 6)) {
        int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);   // X: 0x3B high, 0x3C low
        int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);   // Y: 0x3D, 0x3E
        int16_t az = (int16_t)((buf[4] << 8) | buf[5]);   // Z: 0x3F, 0x40

        roll =  calculate_roll(convert_accel(ay), convert_accel(az));
        pitch = calculate_pitch(convert_accel(ax),convert_accel(ay), convert_accel(az));
        tilt =  calculate_tilt(roll, pitch);
    } else {
        I2C1->CR1 &= ~(1 << 0);   // reset I2C after a failed read
        I2C1->CR1 |=  (1 << 0);
        return false;
    }

    return true;


}
