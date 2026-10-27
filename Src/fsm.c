#include "fsm.h"
#include "gpio.h"

// Servo pulse mapping: 500 us = 0 deg, 1500 us = 90 deg, 2500 us = 180 deg
// (matches 1000 us ~ 45 deg and 2000 us ~ 135 deg). Check against Wokwi.
#define SERVO_US_AT_0_DEG   500.0f
#define SERVO_US_PER_DEG    (2000.0f / 180.0f)


static uint16_t servo_deg_to_us(float deg) {
    if (deg < 0.0f)   deg = 0.0f;
    if (deg > 180.0f) deg = 180.0f;
    return (uint16_t)(SERVO_US_AT_0_DEG + deg * SERVO_US_PER_DEG);
}

void axis_fsm_update(axis_data *axis, volatile uint16_t *position, float angle_deg) {
    float a = fabsf(angle_deg);
    float exit_compliant = axis->enter_compliant - DEADBAND_DEG;
    float exit_clamped = axis->enter_clamped - DEADBAND_DEG;

    // 1. Next state
    switch (axis->state) {
        case ZONE_FREE:
            if (a >  axis->enter_clamped) axis->state = ZONE_CLAMPED;
            else if (a >= axis->enter_compliant) axis->state = ZONE_COMPLIANT;
            break;
        case ZONE_COMPLIANT:
            if (a >  axis->enter_clamped) axis->state = ZONE_CLAMPED;
            else if (a < exit_compliant) axis->state = ZONE_FREE;
            break;
        case ZONE_CLAMPED:
            if (a <  exit_compliant) axis->state = ZONE_FREE;
            else if (a < exit_clamped) axis->state = ZONE_COMPLIANT;
            break;
    }

    // 2. Output from state
    float offset;
    switch (axis->state) {
        case ZONE_COMPLIANT: offset = axis->gain * angle_deg; break;
        case ZONE_CLAMPED: offset = (angle_deg < 0.0f) ? -axis->clamp_deg : axis->clamp_deg; break;
        case ZONE_FREE:
        default: offset = 0.0f; break;
    }

    *position = servo_deg_to_us(axis->neutral_deg + axis->direction * offset);
}

mode_state mode_fsm_update(mode_state mode, const axis_data *roll_axis, const axis_data *pitch_axis, bool button_pressed) {
    bool both_clamped = roll_axis->state == ZONE_CLAMPED && pitch_axis->state == ZONE_CLAMPED;
    bool both_free = roll_axis->state == ZONE_FREE    && pitch_axis->state == ZONE_FREE;

    switch (mode) {
        case MODE_MANUAL:
            // Both axes in their impenetrable regions, or button pressed -> autopilot
            if (both_clamped || button_pressed) mode = MODE_AUTOPILOT;
            break;
        case MODE_AUTOPILOT:
            // Only hand back control when the button is pressed AND both axes are free
            if (button_pressed && both_free) mode = MODE_MANUAL;
            break;
    }

    pin_write(GPIOA,  2, (mode == MODE_AUTOPILOT) ? PIN_HIGH : PIN_LOW); // autopilot LED
    pin_write(GPIOB, 11, (mode == MODE_MANUAL)    ? PIN_HIGH : PIN_LOW); // manual LED
    return mode;
}
