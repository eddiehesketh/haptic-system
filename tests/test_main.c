// Host-side unit tests for the haptic-system logic (angles + FSMs).
// Run from the repo root:   sh tests/run_tests.sh
// No board or Wokwi needed: the hardware calls (GPIO, I2C) are replaced by fakes below.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "fsm.h"
#include "angles.h"
#include "gpio.h"

// ---------- Fakes for the hardware the logic touches ----------

GPIO_TypeDef fake_gpioa, fake_gpiob;
I2C_TypeDef  fake_i2c1;
TIM_TypeDef  fake_tim16;

static int led_autopilot = -1, led_manual = -1;   // last value written to PA2 / PB11

void pin_write(GPIO_TypeDef *port, uint8_t pin, pin_state state) {
    if (port == GPIOA && pin == 2)  led_autopilot = (state == PIN_HIGH);
    if (port == GPIOB && pin == 11) led_manual    = (state == PIN_HIGH);
}

static uint8_t fake_rx[6];
static bool    fake_read_ok = true;

bool i2c_master_readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size) {
    (void)i2c; (void)dev_addr; (void)reg_addr;
    if (!fake_read_ok) return false;
    memcpy(rx_buf, fake_rx, size);
    return true;
}

// Load raw accelerometer counts the way the MPU6050 sends them (big-endian)
static void set_fake_accel(int16_t ax, int16_t ay, int16_t az) {
    int16_t v[3] = { ax, ay, az };
    for (int i = 0; i < 3; i++) {
        fake_rx[2 * i]     = (uint8_t)((uint16_t)v[i] >> 8);
        fake_rx[2 * i + 1] = (uint8_t)((uint16_t)v[i] & 0xFF);
    }
}

// ---------- Tiny test framework ----------

static int tests_run = 0, tests_failed = 0;

#define CHECK(cond, ...) do {                                         \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL %s:%d: ", __FILE__, __LINE__);                  \
        printf(__VA_ARGS__); printf("\n");                            \
    }                                                                 \
} while (0)

#define CHECK_NEAR(actual, expected, tol, what) \
    CHECK(fabsf((float)(actual) - (float)(expected)) <= (tol), \
          "%s: got %.3f, expected %.3f", what, (double)(actual), (double)(expected))

// Servo pulse for a servo angle, same mapping as fsm.c (500 us = 0 deg, 2500 us = 180 deg)
static uint16_t us(float servo_deg) { return (uint16_t)(500.0f + servo_deg * (2000.0f / 180.0f)); }

static axis_data make_roll(void) {
    return (axis_data){ .state = ZONE_FREE, .enter_compliant = 30, .enter_clamped = 50,
                        .neutral_deg = 90, .gain = 1, .clamp_deg = 50, .direction = -1 };
}
static axis_data make_tilt(void) {
    return (axis_data){ .state = ZONE_FREE, .enter_compliant = 20, .enter_clamped = 70,
                        .neutral_deg = 0, .gain = 2, .clamp_deg = 140, .direction = 1 };
}

// Feed one angle, check resulting state and pulse
static void step(axis_data *a, float angle, fsm_state want_state, float want_servo_deg) {
    volatile uint16_t pos = 0;
    axis_fsm_update(a, &pos, angle);
    CHECK(a->state == want_state, "angle %.1f: state %d, expected %d", (double)angle, a->state, want_state);
    CHECK(abs((int)pos - (int)us(want_servo_deg)) <= 1, "angle %.1f: pulse %u us, expected %u us (%.1f deg)",
          (double)angle, (unsigned)pos, (unsigned)us(want_servo_deg), (double)want_servo_deg);
}

// ---------- Tests ----------

static void test_calculate_angles(void) {
    float r, p, t;

    calculate_angles(0, 0, 2048, &r, &p, &t);                       // level
    CHECK_NEAR(r, 0, 0.01f, "level roll");
    CHECK_NEAR(p, 0, 0.01f, "level pitch");
    CHECK_NEAR(t, 0, 0.01f, "level tilt");

    calculate_angles(0, 1024, 1774, &r, &p, &t);                    // roll +30
    CHECK_NEAR(r, 30, 0.1f, "roll +30");
    CHECK_NEAR(p, 0, 0.01f, "pitch with pure roll");
    CHECK_NEAR(t, 30, 0.1f, "tilt with pure roll");

    calculate_angles(0, -1024, 1774, &r, &p, &t);                   // roll -30
    CHECK_NEAR(r, -30, 0.1f, "roll -30");

    calculate_angles(-1024, 0, 1774, &r, &p, &t);                   // pitch +30 (note -a_x)
    CHECK_NEAR(p, 30, 0.1f, "pitch +30");
    CHECK_NEAR(r, 0, 0.01f, "roll with pure pitch");

    calculate_angles(0, 2048, 0, &r, &p, &t);                       // on its side
    CHECK_NEAR(r, 90, 0.01f, "roll 90 (a_z = 0)");

    calculate_angles(-1024, 1024, 1448, &r, &p, &t);                // roll and pitch together
    CHECK_NEAR(t, sqrtf(r * r + p * p), 0.001f, "tilt = sqrt(theta^2 + phi^2)");
}

static void test_read_mpu(void) {
    fake_read_ok = true;
    set_fake_accel(0, 1024, 1774);
    CHECK(read_mpu(), "read_mpu returns true on a good read");
    CHECK_NEAR(roll, 30, 0.1f, "read_mpu roll from bytes");
    CHECK_NEAR(pitch, 0, 0.01f, "read_mpu pitch from bytes");

    set_fake_accel(-1024, 0, 1774);                                 // negative a_x: byte order + sign
    read_mpu();
    CHECK_NEAR(pitch, 30, 0.1f, "read_mpu handles negative counts");

    fake_read_ok = false;
    fake_i2c1.CR1 = 1;
    float before = pitch;
    CHECK(!read_mpu(), "read_mpu returns false when I2C fails");
    CHECK(pitch == before, "angles unchanged after a failed read");
    CHECK(fake_i2c1.CR1 & 1, "I2C re-enabled (PE = 1) after the reset");
    fake_read_ok = true;
}

static void test_axis_zones(void) {
    axis_data a = make_roll();
    step(&a,   0, ZONE_FREE,      90);
    step(&a,  29, ZONE_FREE,      90);
    step(&a,  30, ZONE_COMPLIANT, 60);      // 30 is compliant; direction -1: 90 - 30
    step(&a,  40, ZONE_COMPLIANT, 50);      // tracks one-to-one
    step(&a,  50, ZONE_COMPLIANT, 40);      // 50 is still compliant (clamp is > 50)
    step(&a,  51, ZONE_CLAMPED,   40);      // clamped at 90 - 50
    step(&a,  60, ZONE_CLAMPED,   40);      // a command of 60 is held at 50
}

static void test_axis_deadband(void) {
    axis_data a = make_roll();
    step(&a,  60, ZONE_CLAMPED,   40);      // big jump: free -> clamped in one sample
    step(&a,  47, ZONE_CLAMPED,   40);      // deadband: stays clamped above 45
    step(&a,  44, ZONE_COMPLIANT, 46);
    step(&a,  27, ZONE_COMPLIANT, 63);      // deadband: stays compliant above 25
    step(&a,  24, ZONE_FREE,      90);
    step(&a,  29, ZONE_FREE,      90);      // re-entering needs 30 again, not 25
}

static void test_axis_negative(void) {
    axis_data a = make_roll();
    step(&a, -35, ZONE_COMPLIANT, 125);     // negative angle moves the other way: 90 + 35
    step(&a, -55, ZONE_CLAMPED,   140);     // clamped at 90 + 50
    step(&a, -46, ZONE_CLAMPED,   140);
    step(&a, -44, ZONE_COMPLIANT, 134);
    step(&a, -26, ZONE_COMPLIANT, 116);
    step(&a, -10, ZONE_FREE,      90);
}

static void test_tilt(void) {
    axis_data t = make_tilt();
    step(&t,  10, ZONE_FREE,        0);
    step(&t,  20, ZONE_COMPLIANT,  40);     // servo = 2 x tilt
    step(&t,  35, ZONE_COMPLIANT,  70);
    step(&t,  71, ZONE_CLAMPED,   140);     // clamp bound 140
    step(&t,  66, ZONE_CLAMPED,   140);     // deadband: stays clamped above 65
    step(&t,  64, ZONE_COMPLIANT, 128);
    step(&t,  17, ZONE_COMPLIANT,  34);     // deadband: stays compliant above 15
    step(&t,  14, ZONE_FREE,        0);
}

static void test_mode(void) {
    axis_data roll_a = make_roll(), pitch_a = make_roll();
    mode_state m = MODE_AUTOPILOT;

    m = mode_fsm_update(m, &roll_a, &pitch_a, false);
    CHECK(m == MODE_AUTOPILOT && led_autopilot == 1 && led_manual == 0, "starts in autopilot, LEDs match");

    m = mode_fsm_update(m, &roll_a, &pitch_a, true);
    CHECK(m == MODE_MANUAL && led_manual == 1 && led_autopilot == 0, "button + both free -> manual");

    m = mode_fsm_update(m, &roll_a, &pitch_a, true);
    CHECK(m == MODE_AUTOPILOT, "button in manual -> autopilot");

    roll_a.state = ZONE_COMPLIANT;
    m = mode_fsm_update(m, &roll_a, &pitch_a, true);
    CHECK(m == MODE_AUTOPILOT, "button ignored in autopilot unless both axes free");

    roll_a.state = ZONE_FREE;
    m = mode_fsm_update(m, &roll_a, &pitch_a, true);
    CHECK(m == MODE_MANUAL, "back to manual once free");

    roll_a.state = ZONE_CLAMPED;
    m = mode_fsm_update(m, &roll_a, &pitch_a, false);
    CHECK(m == MODE_MANUAL, "only one axis clamped: stays manual");

    pitch_a.state = ZONE_CLAMPED;
    m = mode_fsm_update(m, &roll_a, &pitch_a, false);
    CHECK(m == MODE_AUTOPILOT, "both clamped -> autopilot automatically");
}

int main(void) {
    test_calculate_angles();
    test_read_mpu();
    test_axis_zones();
    test_axis_deadband();
    test_axis_negative();
    test_tilt();
    test_mode();

    printf("%d checks, %d failed\n", tests_run, tests_failed);
    return tests_failed ? 1 : 0;
}
