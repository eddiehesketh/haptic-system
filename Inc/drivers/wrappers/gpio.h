#pragma once

#include "stm32c031xx.h"
#include "stm32c0xx.h"
#include <stdint.h>


typedef enum {
    PIN_LOW = 0,
    PIN_HIGH = 1
} pin_state;

typedef enum {
    MODE_INPUT = 0, // GPIOx_MODER 00
    MODE_OUTPUT = 1, // GPIOx_MODER 01
    MODE_ALT = 2, // GPIOx_MODER 10
    MODE_ANALOG = 3 // GPIOx_MODER 11, reset default
} pin_mode;

typedef enum {
    NONE = 0, // GPIOx_PUPDR 00
    PULL_UP = 1, // GPIOx_PUPDR 01
    PULL_DOWN = 2, // GPIOx_PUPDR 10
    RESERVED = 3 // GPIOx_PUPDR 11
} pupdr_mode;

void pin_write(GPIO_TypeDef * port, uint8_t pin, pin_state state); // GPIOx_ODR
void toggle_output(GPIO_TypeDef * port, uint8_t pin); // GPIOx_ODR toggle
uint8_t pin_read(GPIO_TypeDef * port, uint8_t pin); // GPIOx_IDR

// Set MODER. Leave PUPDR unchanged when p_mode is NONE.
void init_pin(GPIO_TypeDef * port, uint8_t pin, pin_mode mode, pupdr_mode p_mode);
