#include "init.h"
#include "gpio.h"
#include "i2c.h"
#include "interrupt.h"
#include "pwm.h"
#include "stm32c031xx.h"
#include "timer.h"

#define REG_PWR_MGMT_1 0x6B
#define ACCEL_CONFIG_ADDR (uint8_t)(0x1C)
#define ACCEL_CONFIG (uint8_t)(0x18)

// Pin allocations:
//   Servos       PB4 roll (TIM3 CH1), PB5 pitch (TIM3 CH2), PB0 tilt (TIM3 CH3)
//   LEDs         PA2 autopilot, PB11 manual
//   MPU6050      PB8 SCL, PB9 SDA (I2C1)
//   Button       PA12 autopilot button (input, pull-down)

// Enable the clocks for GPIO ports A and B and for I2C1
static void initialise_clocks() {
    enable_port_clock(A);
    enable_port_clock(B);
    enable_i2c_clock();
}

// Configure I2C1 pins: PB8 SCL and PB9 SDA (AF6, pull-up, open-drain)
static void initialise_i2c_pins() {
    init_pin(GPIOB, 8, MODE_ALT, PULL_UP); // I2C1 SCL
    init_pin_af(GPIOB, 8, 6);
    init_pin(GPIOB, 9, MODE_ALT, PULL_UP); // I2C1 SDA
    init_pin_af(GPIOB, 9, 6);
    GPIOB->OTYPER |= (1 << 8) | (1 << 9);  // I2C needs open-drain
}

// Configure inputs: PA12 autopilot button (pull-down)
static void initialise_inputs() {
    init_pin(GPIOA, 12, MODE_INPUT, PULL_DOWN);
}

// Configure PWM: PB4/PB5/PB0 on TIM3 CH1/CH2/CH3 (AF1), idle at 90 deg
static void initialise_pwm() {
    init_pin(GPIOB, 4, MODE_ALT, NONE); // PB4 -> Roll -> Channel 1
    init_pin(GPIOB, 5, MODE_ALT, NONE); // PB5 -> Pitch -> Channel 2
    init_pin(GPIOB, 0, MODE_ALT, NONE); // PB0 -> Tilt -> Channel 3

    enable_cap_com(GPIOB, 4, 1, 1, SERVO_90_DEG); // AF1, CCR1 = 1500 us
    enable_cap_com(GPIOB, 5, 2, 1, SERVO_90_DEG); // AF1, CCR2 = 1500 us
    enable_cap_com(GPIOB, 0, 3, 1, SERVO_90_DEG); // AF1, CCR3 = 1500 us
}

// Configure outputs: PA2 autopilot LED, PB11 manual LED
static void initialise_outputs() {
    init_pin(GPIOB, 11, MODE_OUTPUT, NONE); // manual LED
    init_pin(GPIOA, 2, MODE_OUTPUT, NONE);  // autopilot LED
}

// Enable the autopilot button interrupt: PA12 rising edge -> EXTI4_15_IRQn
static void initialise_interrupts() {
    enable_interrupt(A, RISING, 12);
}

// Configure timers:
//   TIM3  PSC=47 ARR=19999 -> 1 MHz tick, 50 Hz update (also enables TIM3_IRQn)
//   TIM16 PSC=47999 -> 1 ms tick, free-running timebase for I2C timeouts
static void initialise_timer() {
    enable_timer(TIM3, 19999, 47);
    enable_timer_no_interrupt(TIM16, 0xFFFF, 47999);
}

// Initialise I2C1 (PB8 SCL, PB9 SDA) and configure the MPU6050.
// Returns false if either write to the MPU6050 fails.
static bool initialise_i2c() {
    // f_scl = 100 kHz, f_i2c = 4 MHz
    i2c_master_init(I2C1, 11, 19, 19);
    bool ok = i2c_master_writereg(I2C1, MPU1_ADDR, REG_PWR_MGMT_1, 0x00);               // wake MPU6050
    return ok && i2c_master_writereg(I2C1, MPU1_ADDR, ACCEL_CONFIG_ADDR, ACCEL_CONFIG); // +-16 g
}

// Initialisation block of the main flow chart
void initialise_registers(void) {
    initialise_clocks();
    initialise_i2c_pins();
    initialise_inputs();
    initialise_pwm();
    initialise_outputs();
    initialise_timer();
    initialise_interrupts();

    // Start in autopilot (autopilot LED on, manual LED off) once the MPU6050 responds
    if (initialise_i2c()) {
        pin_write(GPIOB, 11, PIN_LOW);
        pin_write(GPIOA, 2, PIN_HIGH);
    }
}
