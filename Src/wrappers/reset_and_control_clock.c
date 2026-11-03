#include "reset_and_control_clock.h"

// Enable this GPIO port clock through RCC_IOPENR bit
void enable_port_clock(gpio_port port) {
    RCC->IOPENR |= (1 << port);
}
