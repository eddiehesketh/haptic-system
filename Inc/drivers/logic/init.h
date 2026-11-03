#pragma once

#include <stdint.h>

// Servo pulse widths in microseconds
#define SERVO_0_DEG  (uint16_t)500  // 0.5 ms = ~0 degrees
#define SERVO_90_DEG (uint16_t)1500 // 1.5 ms = ~90 degrees

// Initialisation block of the main flow chart (clocks, GPIO, PWM, TIM3, NVIC)
void initialise_registers(void);
