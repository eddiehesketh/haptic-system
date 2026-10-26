#pragma once

#include "angles.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define DEADBAND_DEG 5.0f   // hysteresis applied when leaving clamped / compliant

// ---------- Axis FSM (re-used for roll, pitch and tilt) ----------

typedef enum { ZONE_FREE, ZONE_COMPLIANT, ZONE_CLAMPED } fsm_state;

typedef struct {
    fsm_state state;
    // Thresholds on |angle| (degrees). Exits are DEADBAND_DEG inside the entries.
    float enter_compliant;   // free -> compliant
    float enter_clamped;     // compliant -> clamped
    // Servo output (servo degrees, 0-180)
    float neutral_deg;       // position in free motion (90 roll/pitch, 0 tilt)
    float gain;              // servo deg per detected deg (1 roll/pitch, 2 tilt)
    float clamp_deg;         // offset from neutral when clamped (50 roll/pitch, 140 tilt)
} axis_data;

// Advance the FSM with a new angle (degrees) and write the servo pulse (us) to *position
void axis_fsm_update(axis_data *axis, volatile uint16_t *position, float angle_deg);

// ---------- Mode FSM (autopilot / manual) ----------

typedef enum { MODE_AUTOPILOT, MODE_MANUAL } mode_state;

// Advance the mode FSM. button_pressed = a button press happened since the last call.
// Drives the autopilot (PA2) and manual (PB11) LEDs.
mode_state mode_fsm_update(mode_state mode, const axis_data *roll_axis,
                           const axis_data *pitch_axis, bool button_pressed);
