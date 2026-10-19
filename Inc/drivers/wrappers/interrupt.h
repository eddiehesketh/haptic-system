#pragma once

#include "stm32c0xx.h"
#include "reset_and_control_clock.h"

typedef enum {
    FALLING, // EXTI_FTSR1
    RISING, // EXTI_RTSR1
    BOTH
} edge_trigger_mode;

// Must provide EXTI0_1 / EXTI2_3 / EXTI4_15_IRQHandler
// priority is 0..3 (lower number preempts)
void enable_interrupt_priority(gpio_port port, edge_trigger_mode mode, uint8_t pin, uint8_t priority);

// Same EXTI/NVIC setup. NVIC priority left at reset
void enable_interrupt(gpio_port port, edge_trigger_mode mode, uint8_t pin);
