#pragma once
#include "stm32c0xx.h"

typedef enum {
    A = 0, // RCC_IOPENR GPIOAEN
    B = 1, // RCC_IOPENR GPIOBEN
    C = 2, // RCC_IOPENR GPIOCEN
    D = 3, // RCC_IOPENR GPIODEN
    F = 5  // RCC_IOPENR GPIOFEN; no GPIOE, bit 4 unused
} gpio_port;

// GPIO registers are not accessible until this bit is set (RCC_IOPENR)
void enable_port_clock(gpio_port port);
