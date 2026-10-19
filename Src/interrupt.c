#include "interrupt.h"
#include "stm32c031xx.h"

// Map this pin to a GPIO port in EXTICR (4 pins per register, 8-bit field)
static void configure_pin(gpio_port port, uint8_t pin) {
    uint8_t reg = pin / 4;
    uint8_t index = (pin % 4) * 8;
    EXTI->EXTICR[reg] &= ~(0xFF << index);
    EXTI->EXTICR[reg] |= (port << index);
}

// Unmask this EXTI line in IMR1 so it can reach NVIC
static void enable_nvic_inform(uint8_t pin) {
    EXTI->IMR1 &= ~(1 << pin);
    EXTI->IMR1 |= (1 << pin);
}

// Enable rising edge detect on this line (RTSR1)
static void enable_rising_edge_interrupt(uint8_t pin) {
    EXTI->RTSR1 &= ~(1 << pin);
    EXTI->RTSR1 |= (1 << pin);
}

// Enable falling edge detect on this line (FTSR1)
static void enable_falling_edge_interrupt(uint8_t pin) {
    EXTI->FTSR1 &= ~(1 << pin);
    EXTI->FTSR1 |= (1 << pin);
}

// RPR1 is write 1 to clear. Clear any rising pending bit.
static void monitor_rising_edge(uint8_t pin) {
    EXTI->RPR1 &= ~(1 << pin);
    EXTI->RPR1 |= (1 << pin);
}

// FPR1 is write 1 to clear. Clear any falling pending bit.
static void monitor_falling_edge(uint8_t pin) {
    EXTI->FPR1 &= ~(1 << pin);
    EXTI->FPR1 |= (1 << pin);
}

// Configure EXTI + NVIC with a set priority
void enable_interrupt_priority(gpio_port port, edge_trigger_mode mode, uint8_t pin, uint8_t priority) {
    configure_pin(port, pin);
    enable_nvic_inform(pin);
    
    if (mode == FALLING || mode == BOTH) {
        enable_falling_edge_interrupt(pin);
        monitor_falling_edge(pin);
    }

    if (mode == RISING || mode == BOTH) {
        enable_rising_edge_interrupt(pin);
        monitor_rising_edge(pin);
    }

    // STM32C0 pins 0-1, 2-3, and 4-15 share one NVIC vector each
    IRQn_Type interrupt_type = pin < 2 ? EXTI0_1_IRQn : (pin < 4 ? EXTI2_3_IRQn : EXTI4_15_IRQn);

    NVIC_SetPriority(interrupt_type, priority);
    NVIC_EnableIRQ(interrupt_type);
}

// Same EXTI setup. NVIC priority left at reset (0).
void enable_interrupt(gpio_port port, edge_trigger_mode mode, uint8_t pin) {
    configure_pin(port, pin);
    enable_nvic_inform(pin);
    
    if (mode == FALLING || mode == BOTH) {
        enable_falling_edge_interrupt(pin);
        monitor_falling_edge(pin);
    }

    if (mode == RISING || mode == BOTH) {
        enable_rising_edge_interrupt(pin);
        monitor_rising_edge(pin);
    }

    // STM32C0 pins 0-1, 2-3, and 4-15 share one NVIC vector each
    IRQn_Type interrupt_type = pin < 2 ? EXTI0_1_IRQn : (pin < 4 ? EXTI2_3_IRQn : EXTI4_15_IRQn);

    NVIC_EnableIRQ(interrupt_type);
}
