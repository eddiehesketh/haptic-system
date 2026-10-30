/**
 * @file    main.c
 * @brief   Haptic system main loop and interrupt handlers.
 *
 * TIM3 fires at 50 Hz: the ISR applies the latest servo pulses and flags the
 * main loop, which reads the MPU6050, updates the roll / pitch / tilt axis
 * FSMs and runs the autopilot / manual mode FSM.
 */

#include <stdint.h>
#include "gpio.h"
#include "interrupt.h"
#include "stm32c031xx.h"
#include "init.h"
#include "angles.h"
#include "fsm.h"

// Servo pulses in microseconds, written to TIM3 CCR1..3 by TIM3_IRQHandler
volatile uint16_t roll_servo_pos = SERVO_90_DEG;
volatile uint16_t pitch_servo_pos = SERVO_90_DEG;
volatile uint16_t tilt_servo_pos = SERVO_0_DEG;

volatile bool read_imu = false;       // set by TIM3_IRQHandler at 50 Hz
volatile bool button_pressed = false; // set by EXTI4_15_IRQHandler on the autopilot button

// One FSM instance per axis (same code, different parameters)
static axis_data roll_axis = {
    .state = ZONE_FREE, .enter_compliant = 30, .enter_clamped = 50,
    .neutral_deg = 90, .gain = 1, .clamp_deg = 50, .direction = -1,
};

static axis_data pitch_axis = {
    .state = ZONE_FREE, .enter_compliant = 30, .enter_clamped = 50,
    .neutral_deg = 90, .gain = 1, .clamp_deg = 50, .direction = -1,
};

static axis_data tilt_axis = {
    .state = ZONE_FREE, .enter_compliant = 20, .enter_clamped = 70,
    .neutral_deg = 0, .gain = 2, .clamp_deg = 140, .direction = 1,   // servo angle = 2 x tilt, clamp at 140
};

int main(void)
{
    initialise_registers();
    mode_state mode = MODE_AUTOPILOT;

    /* Loop forever */
    for (;;) {
        // TIM3 sets read_imu every 20 ms; do the (blocking) I2C read here, not in the ISR
        while (!read_imu);
        read_imu = false;

        if (read_mpu()) {
            axis_fsm_update(&roll_axis,  &roll_servo_pos,  roll);
            axis_fsm_update(&pitch_axis, &pitch_servo_pos, pitch);
            axis_fsm_update(&tilt_axis,  &tilt_servo_pos,  tilt);
        }

        // Consume the button press (if any) and run the autopilot/manual FSM
        bool pressed = button_pressed;
        button_pressed = false;
        mode = mode_fsm_update(mode, &roll_axis, &pitch_axis, pressed);
    }
}

// Autopilot button (PA12, rising edge)
void EXTI4_15_IRQHandler() {
    if (EXTI->RPR1 & (1 << 12)) {
        EXTI->RPR1 = (1 << 12); // write 1 to clear the PA12 rising-edge pending bit
        button_pressed = true;  // handled by the mode FSM in the main loop
    }
}

// TIM3 update event (50 Hz)
void TIM3_IRQHandler() {
    if (TIM3->SR & (1 << 0)) { // UIF set?
        TIM3->SR &= ~(1 << 0); // clear UIF
        read_imu = true;       // tell the main loop to read the MPU6050
        TIM3->CCR1 = roll_servo_pos;  // PB4 roll
        TIM3->CCR2 = pitch_servo_pos; // PB5 pitch
        TIM3->CCR3 = tilt_servo_pos;  // PB0 tilt
    }
}
