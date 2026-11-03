#pragma once

#include <stdint.h>

// Servo pulse widths in microseconds
#define SERVO_0_DEG (uint16_t)500 // 0.5 ms = ~0 degrees
#define SERVO_90_DEG (uint16_t)1500 // 1.5 ms = ~90 degrees

// Hardware initialisation (clocks, GPIO, PWM, timers, NVIC, I2C / MPU6050)
void initialise_registers(void);
