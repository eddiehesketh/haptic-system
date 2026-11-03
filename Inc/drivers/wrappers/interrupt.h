#pragma once

#include <stdint.h>
#include "stm32c031xx.h"
#include "reset_and_control_clock.h"

typedef enum {
    FALLING, // EXTI_FTSR1
    RISING, // EXTI_RTSR1
    BOTH
} edge_trigger_mode;

// Configure EXTI + NVIC for this pin. NVIC priority left at reset (0).
// Must provide EXTI0_1 / EXTI2_3 / EXTI4_15_IRQHandler
void enable_interrupt(gpio_port port, edge_trigger_mode mode, uint8_t pin);
