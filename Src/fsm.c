#include "fsm.h"

#define CLAMPED_ANGLE 90 // need to work this out

void axis_fsm_update(axis_data *axis, uint16_t *position, float angle) {
    float abs_angle = fabsf(angle);

    switch (axis->state) {
        case ZONE_FREE:
            if (abs_angle > axis->enter_clamped) {
                axis->state = ZONE_COMPLIANT;
                *position = CLAMPED_ANGLE;
            } else if (abs)
            
    }


}