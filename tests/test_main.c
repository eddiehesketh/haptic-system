#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "angles.h"
#include "fsm.h"
#include "gpio.h"

// Host-side unit tests for the haptic system (angle maths, axis FSM, mode FSM).
// Command to run the tests (from the repo root):  sh tests/run_tests.sh
// Note: do NOT compile with -DNDEBUG, which turns every assert() into a no-op.

#define LSB_PER_G  2048.0f   // MPU6050 at +-16 g (ACCEL_CONFIG = 0x18)
#define ANGLE_TOL  0.1f      // degrees
#define SERVO_TOL  0.2f      // servo degrees (1 us of pulse is ~0.09 deg)
#define ONE_G_TOL  0.01f     // allowed error on |a| = 1 g

// ============================================================================
// Fakes: the hardware the logic touches is replaced so it runs on a PC
// ============================================================================

GPIO_TypeDef fake_gpioa, fake_gpiob;
I2C_TypeDef fake_i2c1;
TIM_TypeDef fake_tim16;

bool led_autopilot = false; // last value written to PA2
bool led_manual = false;    // last value written to PB11

void pin_write(GPIO_TypeDef *port, uint8_t pin, pin_state state) {
    if (port == GPIOA && pin == 2) led_autopilot = (state == PIN_HIGH);
    if (port == GPIOB && pin == 11) led_manual = (state == PIN_HIGH);
}

uint8_t fake_rx[6]; // bytes the fake MPU6050 returns

bool i2c_master_readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size) {
    (void)i2c; (void)dev_addr; (void)reg_addr;
    memcpy(rx_buf, fake_rx, size);
    return true;
}

// Load an accelerometer reading (in g) into the fake MPU6050, big-endian like the real one
void set_accel_g(float ax, float ay, float az) {
    float g[3] = { ax, ay, az };
    for (int i = 0; i < 3; i++) {
        int16_t counts = (int16_t)lroundf(g[i] * LSB_PER_G);
        fake_rx[2 * i] = (uint8_t)((uint16_t)counts >> 8);
        fake_rx[2 * i + 1] = (uint8_t)((uint16_t)counts & 0xFF);
    }
}

// ============================================================================
// System under test: same instances and loop body as main.c
// ============================================================================

axis_data roll_axis, pitch_axis, tilt_axis;
volatile uint16_t roll_servo_pos, pitch_servo_pos, tilt_servo_pos;
mode_state mode;

void setup(void) {
    roll_axis = (axis_data){ .state = ZONE_FREE, .enter_compliant = 30, .enter_clamped = 50,
                             .neutral_deg = 90, .gain = 1, .clamp_deg = 50, .direction = -1 };
    pitch_axis = roll_axis;
    tilt_axis = (axis_data){ .state = ZONE_FREE, .enter_compliant = 20, .enter_clamped = 70,
                             .neutral_deg = 0, .gain = 2, .clamp_deg = 140, .direction = 1 };
    mode = MODE_AUTOPILOT;
    led_autopilot = led_manual = false;
}

// Apply an accelerometer reading a = (ax, ay, az) and run one 20 ms cycle, as in the main loop
void apply(float ax, float ay, float az, bool button_pressed) {
    assert(fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f) <= ONE_G_TOL); // |a| = 1 g

    set_accel_g(ax, ay, az);
    assert(read_mpu());
    axis_fsm_update(&roll_axis, &roll_servo_pos, roll);
    axis_fsm_update(&pitch_axis, &pitch_servo_pos, pitch);
    axis_fsm_update(&tilt_axis, &tilt_servo_pos, tilt);
    mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, button_pressed);

    // + 0.0f stops -0.0 being printed
    printf("   a = (%.2f, %.2f, %.2f), roll = %.1f, pitch = %.1f, tilt = %.1f\n",
           ax, ay, az, roll + 0.0f, pitch + 0.0f, tilt + 0.0f);
}

// ============================================================================
// Assertion helpers
// ============================================================================

void assert_near(float actual, float expected, float tol) {
    assert(fabsf(actual - expected) <= tol);
}

// Check a pulse width against a servo angle (500 us = 0 deg, 2500 us = 180 deg)
void assert_servo(uint16_t pulse_us, float expected_deg) {
    float deg = ((float)pulse_us - 500.0f) / (2000.0f / 180.0f);
    assert_near(deg, expected_deg, SERVO_TOL);
}

// Feed one angle straight into an axis FSM and return the servo pulse
uint16_t step(axis_data *axis, float angle) {
    volatile uint16_t pos = 0;
    axis_fsm_update(axis, &pos, angle);
    return pos;
}

// ============================================================================
// 1. Angle calculation, Eqs. (1)-(3)
// ============================================================================

void test_calculate_angles(void) {
    float r, p, t;

    calculate_angles(0, 0, 2048, &r, &p, &t);          // level
    assert_near(r, 0, ANGLE_TOL);
    assert_near(p, 0, ANGLE_TOL);
    assert_near(t, 0, ANGLE_TOL);

    calculate_angles(0, -1024, 1774, &r, &p, &t);      // a_y = -0.5 g, a_z = 0.866 g
    assert_near(r, -30, ANGLE_TOL);

    calculate_angles(-1024, 0, 1774, &r, &p, &t);      // negative a_x gives positive pitch
    assert_near(p, 30, ANGLE_TOL);

    calculate_angles(-1024, 1024, 1448, &r, &p, &t);   // tilt = sqrt(theta^2 + phi^2)
    assert_near(t, sqrtf(r * r + p * p), 0.001f);

    printf("P: test_calculate_angles\n");
}

// ============================================================================
// 2. Axis FSM (roll / pitch): zones, outputs and deadband
// ============================================================================

void test_axis_zones(void) {
    setup();

    assert_servo(step(&roll_axis, 29.9f), 90);       // free: neutral
    assert(roll_axis.state == ZONE_FREE);

    assert_servo(step(&roll_axis, 30), 60);          // compliant from 30, tracks 1:1 (90 - angle)
    assert(roll_axis.state == ZONE_COMPLIANT);
    assert_servo(step(&roll_axis, 50), 40);          // 50 itself is still compliant
    assert(roll_axis.state == ZONE_COMPLIANT);

    assert_servo(step(&roll_axis, 50.1f), 40);       // clamped above 50
    assert(roll_axis.state == ZONE_CLAMPED);
    assert_servo(step(&roll_axis, 60), 40);          // a command of 60 is held at 50

    printf("P: test_axis_zones\n");
}

void test_axis_negative_angles(void) {
    setup();

    assert_servo(step(&roll_axis, -35), 125);        // negative moves the other way: 90 + 35
    assert(roll_axis.state == ZONE_COMPLIANT);
    assert_servo(step(&roll_axis, -60), 140);        // clamped at 90 + 50
    assert(roll_axis.state == ZONE_CLAMPED);

    printf("P: test_axis_negative_angles\n");
}

void test_axis_deadband(void) {
    setup();
    step(&roll_axis, 60);                            // clamped

    assert_servo(step(&roll_axis, 47), 40);          // 45-50: stays clamped
    assert(roll_axis.state == ZONE_CLAMPED);
    assert_servo(step(&roll_axis, 44), 46);          // below 45: compliant
    assert(roll_axis.state == ZONE_COMPLIANT);

    step(&roll_axis, 27);                            // 25-30: stays compliant
    assert(roll_axis.state == ZONE_COMPLIANT);
    step(&roll_axis, 24);                            // below 25: free
    assert(roll_axis.state == ZONE_FREE);
    step(&roll_axis, 29);                            // re-entry needs 30 again, not 25
    assert(roll_axis.state == ZONE_FREE);

    step(&roll_axis, -60);                           // same on the negative side
    step(&roll_axis, -46);
    assert(roll_axis.state == ZONE_CLAMPED);
    step(&roll_axis, -26);
    assert(roll_axis.state == ZONE_COMPLIANT);

    printf("P: test_axis_deadband\n");
}

void test_axis_skip_transitions(void) {
    setup();

    step(&roll_axis, 60);                            // free -> clamped in one sample
    assert(roll_axis.state == ZONE_CLAMPED);
    step(&roll_axis, 10);                            // clamped -> free in one sample
    assert(roll_axis.state == ZONE_FREE);

    printf("P: test_axis_skip_transitions\n");
}

// ============================================================================
// 3. Tilt FSM: 20 / 70 thresholds, servo = 2 x tilt, clamp at 140
// ============================================================================

void test_tilt_zones(void) {
    setup();

    assert_servo(step(&tilt_axis, 19.9f), 0);        // free: neutral at 0
    assert(tilt_axis.state == ZONE_FREE);
    assert_servo(step(&tilt_axis, 20), 40);          // compliant: 2 x tilt
    assert(tilt_axis.state == ZONE_COMPLIANT);
    assert_servo(step(&tilt_axis, 35), 70);
    assert_servo(step(&tilt_axis, 75), 140);         // clamped at 140
    assert(tilt_axis.state == ZONE_CLAMPED);

    printf("P: test_tilt_zones\n");
}

void test_tilt_deadband(void) {
    setup();
    step(&tilt_axis, 75);

    step(&tilt_axis, 66);                            // 65-70: stays clamped
    assert(tilt_axis.state == ZONE_CLAMPED);
    step(&tilt_axis, 64);
    assert(tilt_axis.state == ZONE_COMPLIANT);
    step(&tilt_axis, 17);                            // 15-20: stays compliant
    assert(tilt_axis.state == ZONE_COMPLIANT);
    step(&tilt_axis, 14);
    assert(tilt_axis.state == ZONE_FREE);

    printf("P: test_tilt_deadband\n");
}

// ============================================================================
// 4. Mode FSM: autopilot / manual and the LEDs
// ============================================================================

void test_mode_button(void) {
    setup();

    mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, false);
    assert(mode == MODE_AUTOPILOT);                  // starts in autopilot
    assert(led_autopilot && !led_manual);

    mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, true);
    assert(mode == MODE_MANUAL);                     // button + both free -> manual
    assert(led_manual && !led_autopilot);

    mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, true);
    assert(mode == MODE_AUTOPILOT);                  // button in manual -> autopilot

    printf("P: test_mode_button\n");
}

void test_mode_button_refused_unless_both_free(void) {
    setup();

    roll_axis.state = ZONE_COMPLIANT;
    mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, true);
    assert(mode == MODE_AUTOPILOT);

    roll_axis.state = ZONE_FREE;
    pitch_axis.state = ZONE_CLAMPED;
    mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, true);
    assert(mode == MODE_AUTOPILOT);

    printf("P: test_mode_button_refused_unless_both_free\n");
}

void test_mode_both_clamped_triggers_autopilot(void) {
    setup();
    mode = MODE_MANUAL;

    roll_axis.state = ZONE_CLAMPED;
    mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, false);
    assert(mode == MODE_MANUAL);                     // one axis is not enough

    pitch_axis.state = ZONE_CLAMPED;
    mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, false);
    assert(mode == MODE_AUTOPILOT);
    assert(led_autopilot && !led_manual);

    printf("P: test_mode_both_clamped_triggers_autopilot\n");
}

// ============================================================================
// 5. Documented accelerometer cases, |a| = 1 g, run through read_mpu and all FSMs.
//    These are the triples to use with the Wokwi sliders and in the report.
// ============================================================================

void test_case_level(void) {
    setup();

    apply(0.00f, 0.00f, 1.00f, false);
    assert(roll_axis.state == ZONE_FREE && pitch_axis.state == ZONE_FREE && tilt_axis.state == ZONE_FREE);
    assert_servo(roll_servo_pos, 90);
    assert_servo(pitch_servo_pos, 90);
    assert_servo(tilt_servo_pos, 0);
    printf("P: test_case_level\n");
}

void test_case_roll_only(void) {

    setup();
    apply(0.00f, 0.64f, 0.77f, false);               // compliant (+)
    assert(roll_axis.state == ZONE_COMPLIANT && pitch_axis.state == ZONE_FREE);
    assert_servo(roll_servo_pos, 90 - roll);
    assert_servo(pitch_servo_pos, 90);

    setup();
    apply(0.00f, -0.64f, 0.77f, false);              // compliant (-)
    assert(roll_axis.state == ZONE_COMPLIANT && pitch_axis.state == ZONE_FREE);
    assert_servo(roll_servo_pos, 90 - roll);

    setup();
    apply(0.00f, 0.87f, 0.50f, false);               // clamped (+)
    assert(roll_axis.state == ZONE_CLAMPED && pitch_axis.state == ZONE_FREE);
    assert_servo(roll_servo_pos, 40);

    setup();
    apply(0.00f, -0.87f, 0.50f, false);              // clamped (-)
    assert(roll_axis.state == ZONE_CLAMPED && pitch_axis.state == ZONE_FREE);
    assert_servo(roll_servo_pos, 140);
    printf("P: test_case_roll_only\n");
}

void test_case_pitch_only(void) {

    setup();
    apply(-0.64f, 0.00f, 0.77f, false);              // compliant (+)
    assert(pitch_axis.state == ZONE_COMPLIANT && roll_axis.state == ZONE_FREE);
    assert_servo(pitch_servo_pos, 90 - pitch);
    assert_servo(roll_servo_pos, 90);

    setup();
    apply(0.64f, 0.00f, 0.77f, false);               // compliant (-)
    assert(pitch_axis.state == ZONE_COMPLIANT && roll_axis.state == ZONE_FREE);
    assert_servo(pitch_servo_pos, 90 - pitch);

    setup();
    apply(-0.87f, 0.00f, 0.50f, false);              // clamped (+)
    assert(pitch_axis.state == ZONE_CLAMPED && roll_axis.state == ZONE_FREE);
    assert_servo(pitch_servo_pos, 40);

    setup();
    apply(0.87f, 0.00f, 0.50f, false);               // clamped (-)
    assert(pitch_axis.state == ZONE_CLAMPED && roll_axis.state == ZONE_FREE);
    assert_servo(pitch_servo_pos, 140);
    printf("P: test_case_pitch_only\n");
}

void test_case_tilt_only(void) {

    setup();
    apply(0.00f, 0.34f, 0.94f, false);               // tilt just below 20: free
    assert(tilt_axis.state == ZONE_FREE);
    assert_servo(tilt_servo_pos, 0);

    setup();
    apply(-0.34f, 0.32f, 0.88f, false);              // roll, pitch ~20 (free), tilt ~28: compliant
    assert(roll_axis.state == ZONE_FREE && pitch_axis.state == ZONE_FREE);
    assert(tilt_axis.state == ZONE_COMPLIANT);
    assert_servo(tilt_servo_pos, 2 * tilt);
    assert_servo(roll_servo_pos, 90);
    assert_servo(pitch_servo_pos, 90);
    printf("P: test_case_tilt_only\n");
}

void test_case_tilt_clamped(void) {
    setup();

    apply(-0.64f, 0.66f, 0.38f, false);              // roll ~60, pitch ~40, tilt ~72
    assert(tilt_axis.state == ZONE_CLAMPED);
    assert_servo(tilt_servo_pos, 140);
    printf("P: test_case_tilt_clamped\n");
}

void test_case_deadband(void) {

    setup();
    apply(0.00f, 0.87f, 0.50f, false);               // roll 60.1: clamped
    apply(0.00f, 0.73f, 0.68f, false);               // roll 47.0: stays clamped
    assert(roll_axis.state == ZONE_CLAMPED);
    assert_servo(roll_servo_pos, 40);
    apply(0.00f, 0.69f, 0.72f, false);               // roll 43.8: compliant
    assert(roll_axis.state == ZONE_COMPLIANT);
    apply(0.00f, 0.45f, 0.89f, false);               // roll 26.8: stays compliant
    assert(roll_axis.state == ZONE_COMPLIANT);
    apply(0.00f, 0.39f, 0.92f, false);               // roll 23.0: free
    assert(roll_axis.state == ZONE_FREE);
    assert_servo(roll_servo_pos, 90);

    setup();
    apply(-0.64f, 0.66f, 0.38f, false);              // tilt 72.2: clamped
    apply(-0.50f, 0.75f, 0.43f, false);              // tilt 67.2: stays clamped
    assert(tilt_axis.state == ZONE_CLAMPED);
    apply(-0.50f, 0.71f, 0.50f, false);              // tilt 62.5: compliant
    assert(tilt_axis.state == ZONE_COMPLIANT);
    assert_servo(tilt_servo_pos, 2 * tilt);
    printf("P: test_case_deadband\n");
}

void test_case_mode(void) {
    setup();

    apply(0.00f, 0.64f, 0.77f, true);                // button with roll compliant: refused
    assert(mode == MODE_AUTOPILOT);

    apply(0.00f, 0.00f, 1.00f, true);                // button when level: manual
    assert(mode == MODE_MANUAL);
    assert(led_manual && !led_autopilot);

    apply(-0.87f, 0.43f, 0.25f, false);              // roll ~60, pitch ~60: both clamped -> autopilot
    assert(roll_axis.state == ZONE_CLAMPED && pitch_axis.state == ZONE_CLAMPED);
    assert(mode == MODE_AUTOPILOT);
    assert(led_autopilot && !led_manual);
    printf("P: test_case_mode\n");
}

// ============================================================================
// Run tests
// ============================================================================

int main(void) {
    printf("Running unit tests...\n\n");

    test_calculate_angles();

    test_axis_zones();
    test_axis_negative_angles();
    test_axis_deadband();
    test_axis_skip_transitions();

    test_tilt_zones();
    test_tilt_deadband();

    test_mode_button();
    test_mode_button_refused_unless_both_free();
    test_mode_both_clamped_triggers_autopilot();

    test_case_level();
    test_case_roll_only();
    test_case_pitch_only();
    test_case_tilt_only();
    test_case_tilt_clamped();
    test_case_deadband();
    test_case_mode();

    printf("\nAll tests passed!\n");
    return 0;
}
