#include "init.h"

#define SERVO_90_DEG (uint16_t)1500 // 1.5 ms = ~90 degrees

// Enable GPIO clocks: A, B, C
static void initialise_clocks() {
    enable_port_clock(A);
    enable_port_clock(B);
    enable_port_clock(C); 
}

// Configure inputs: PA2 New Item, PA10 Step Gate, PB5 E-Stop (all pull down)
static void initialise_inputs() {
    init_pin(GPIOA, 2, MODE_INPUT, PULL_DOWN); // New Item PA2
    init_pin(GPIOA, 10, MODE_INPUT, PULL_DOWN); // Step Gate PA10
    init_pin(GPIOB, 5, MODE_INPUT, PULL_DOWN); // E-Stop PB5
}

// Configure PWM: PC7 and PB0 Alt Func AF1, TIM3 CH2/CH3, idle at 90 deg
static void initialise_pwm() {
    init_pin(GPIOC, 7, MODE_ALT, NONE); // servo motor 1 -> PC7 / TIM3 CH2
    init_pin(GPIOB, 0, MODE_ALT, NONE); // servo motor 2 -> PB0 / TIM3 CH3
    enable_cap_com(GPIOC, 7, 2, 1, SERVO_90_DEG); // AF1, CCR2 = 1500 us
    enable_cap_com(GPIOB, 0, 3, 1, SERVO_90_DEG); // AF1, CCR3 = 1500 us
}

// Configure outputs: PA9 Fault LED; PA0/PA1/PA4 Real RGB; PA3/PA11/PA8 Sensed RGB
static void initialise_outputs() {
    init_pin(GPIOA, 9, MODE_OUTPUT, NONE); // Fault LED PA9

    init_pin(GPIOA, 0, MODE_OUTPUT, NONE); // Real RGB -> red
    init_pin(GPIOA, 1, MODE_OUTPUT, NONE); // Real RGB -> green
    init_pin(GPIOA, 4, MODE_OUTPUT, NONE); // Real RGB -> blue

    init_pin(GPIOA, 3, MODE_OUTPUT, NONE); // Sensed RGB -> red 
    init_pin(GPIOA, 11, MODE_OUTPUT, NONE); // Sensed RGB -> green 
    init_pin(GPIOA, 8, MODE_OUTPUT, NONE); // Sensed RGB -> blue 
}

// Enable NVIC: EXTI2_3_IRQn (PA2), EXTI4_15_IRQn (PA10, PB5)
static void initialise_interrupts() {
    enable_interrupt(A, RISING, 2);  // New Item PA2 -> EXTI2_3_IRQn
    enable_interrupt(A, RISING, 10); // Step Gate PA10 -> EXTI4_15_IRQn
    enable_interrupt(B, RISING, 5);  // E-Stop PB5 -> EXTI4_15_IRQn
}

// Configure timer: TIM3 PSC=47 ARR=19999 (50 Hz). Also enables TIM3_IRQn
static void initialise_timer() {
    enable_timer(TIM3, 19999, 47);
}

// Initialisation block of the main flow chart
void initialise_registers() {
    initialise_clocks();
    initialise_inputs();
    initialise_pwm();
    initialise_outputs();
    initialise_timer();
    initialise_interrupts();
}
