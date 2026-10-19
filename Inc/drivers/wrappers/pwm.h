#pragma once

#include "stm32c0xx.h"

typedef enum {
    CCM_OUTPUT = 0, // CCMRx CCxS 00
    CCM_INPUT_STRAIGHT = 1, // CCMRx CCxS 01
    CCM_INPUT_CROSS = 2 // CCMRx CCxS 10
} cc_mode;

typedef enum {
    PWM_MODE_1 = 0x6, // CCMRx OCxM 110
    PWM_MODE_2 = 0x7 // CCMRx OCxM 111
} pwm_mode;

// TIM3 PWM on this pin. duty_us is the CCR pulse in microseconds (1 MHz tick).
void enable_cap_com(GPIO_TypeDef * port, uint8_t pin, uint8_t channel, uint8_t af, uint16_t duty_us);
