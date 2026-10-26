#include "convert.h"

#define LSB_SENSITIVITY 2048
#define GRAVITY_ACCEl (double)(9.81)

double convert_accel(int16_t a) {
    return ((double)a / LSB_SENSITIVITY) * GRAVITY_ACCEl;
}
