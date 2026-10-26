#pragma once

#include "reset_and_control_clock.h"
#include "interrupt.h"
#include "gpio.h"
#include "stm32c031xx.h"
#include "timer.h"
#include "pwm.h"
#include "i2c.h"

// Initialisation block of the main flow chart (clocks, GPIO, PWM, TIM3, NVIC)
// Returns 0 if the MPU6050 woke up, else an I2C_ERR_* code
int initialise_registers();