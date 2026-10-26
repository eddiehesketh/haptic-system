#include "gpio.h"
#include <stdint.h>

// GPIOx_MODER: 2 bits per pin (00 input, 01 output, 10 AF, 11 analog)
static void set_pin_mode(GPIO_TypeDef * port, uint8_t pin, pin_mode mode) {
    uint8_t shift = pin * 2;
    port->MODER &= ~(3 << shift);
    port->MODER |= (mode << shift);
}

// GPIOx_MODER: set this pin to alternate function
static void set_alt_func(GPIO_TypeDef * port, uint8_t pin) {
    uint8_t shift = 2 * pin;
    port->MODER &= ~(3 << shift);
    port->MODER |= (MODE_ALT << shift);
}

// GPIOx_AFR: AFRL for pins 0-7, AFRH for pins 8-15, 4 bits per pin
static void set_af_alt_func(GPIO_TypeDef * port, uint8_t pin, uint8_t af) {
    uint8_t idx = pin / 8;
    uint8_t shift = (pin % 8) * 4;

    port->AFR[idx] &= ~(0xF << shift);
    port->AFR[idx] |= (af << shift);
}

// Set or clear one bit in GPIOx_ODR
void pin_write(GPIO_TypeDef * port, uint8_t pin, pin_state state) {
    if (state == PIN_HIGH) {
        port->ODR |= (1 << pin);
    } else {
        port->ODR &= ~(1 << pin);
    }
}

// Toggle one bit in GPIOx_ODR
void toggle_output(GPIO_TypeDef * port, uint8_t pin) {
    port->ODR ^= (1 << pin);
}

// Read one bit from GPIOx_IDR
uint8_t pin_read(GPIO_TypeDef * port, uint8_t pin) {
    return (uint8_t)((port->IDR >> pin) & 1);
}

// GPIOx_PUPDR: 2 bits per pin (00 none, 01 pull-up, 10 pull-down)
static void set_pupdr_mode(GPIO_TypeDef * port, uint8_t pin, pupdr_mode mode) {
    uint8_t shift = 2 * pin;
    port->PUPDR &= ~(3 << shift);
    port->PUPDR |= (mode << shift);
}

// Set MODER. Set PUPDR only if p_mode is not NONE.
void init_pin(GPIO_TypeDef * port, uint8_t pin, pin_mode mode, pupdr_mode p_mode) {
    set_pin_mode(port, pin, mode);
    if (p_mode != NONE) {
        set_pupdr_mode(port, pin, p_mode);
    }
}
void init_pin_af(GPIO_TypeDef * port, uint8_t pin, uint8_t af) {
    // GPIOx_MODER: set this pin to alternate function
    set_alt_func(port, pin);
    set_af_alt_func(port, pin, af);

}

