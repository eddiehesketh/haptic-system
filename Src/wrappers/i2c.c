#include "i2c.h"
#include "stm32c031xx.h"
#include <stdint.h>

void enable_i2c_clock() {
    RCC->APBENR1 &= ~(1 << 21);
    RCC->APBENR1 |= (1 << 21);
}

bool check_timeout(uint16_t start_tick, uint16_t timeout_ms) {
    uint16_t current_tick = (uint16_t)TIM16->CNT;
    return (uint16_t)(current_tick - start_tick) >= timeout_ms;
}

void i2c_master_init(I2C_TypeDef *i2c, uint8_t presc, uint8_t sclh, uint8_t scll) {
    // Disable I2C peripheral before modifying timing
    i2c->CR1 &= ~(1 << 0);
    // Configure the TIMINGR register
    i2c->TIMINGR = ((uint32_t)presc << 28) | ((uint32_t)sclh << 8) | ((uint32_t)scll << 0);
    // Re-enable the I2C peripheral
    i2c->CR1 |= (1 << 0);
}

bool i2c_master_writereg(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    uint16_t start_tick;
    const uint16_t timeout_ms = 10;

    // Wait for i2c bus free
    start_tick = (uint16_t)TIM16->CNT;                      // use current counter value as start time (1 tick = 1ms)
    while (i2c->ISR & (1 << 15)) {
        // Check Busy flag. If timeout > 10 ms; bus is busy. Exit
        if (check_timeout(start_tick, timeout_ms)) return false;
    }

    // Write the target device register address over I2C and one byte of data
    // device_addrs, num_bytes, autoend = true, start transmission
    // RD_WRN (bit 10) is left clear for a write
    i2c->CR2 = (dev_addr << 1) | (2 << 16) | (1 << 25) | (1 << 13);

    start_tick = (uint16_t)TIM16->CNT;                      // reset start_tick counter
    // Wait for communication of device address to complete
    while (!(i2c->ISR & (1 << 1))) {
        // Check status of TXIS
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    // Device address has been written, TXDR now empty
    i2c->TXDR = reg_addr;                                   // Address of register on peripheral to write

    // TXIS can still read as set straight after the TXDR write. Without this short
    // wait the TXIS loop below passes instantly, the data byte overwrites reg_addr,
    // only one byte is sent, and the transfer hangs waiting for byte 2 (ISR = 0x800B).
    // Wait for the hardware to take reg_addr first.
    for (volatile uint32_t n = 0; (i2c->ISR & (1 << 1)) && n < 2000; n++);

    start_tick = (uint16_t)TIM16->CNT;                      // reset start_tick counter
    // Wait for communication of register address to complete
    while (!(i2c->ISR & (1 << 1))) {
        // Check status of TXIS
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    i2c->TXDR = data;                                       // Actual data to write to peripheral

    start_tick = (uint16_t)TIM16->CNT;                      // Reset Start Tick counter
    while (!(i2c->ISR & (1 << 5))) {
        // Wait for communication with peripheral to be complete (check stop flag)
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    i2c->ICR |= (1 << 5);                                   // Write 1 to clear stop flag

    return true;
}

bool i2c_master_readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size) {
    uint16_t start_tick;
    const uint16_t timeout_ms = 10;

    // Wait for i2c bus free
    start_tick = (uint16_t)TIM16->CNT;                      // use current counter value as start time (1 tick = 1ms)
    while (i2c->ISR & (1 << 15)) {
        // Check Busy flag. If timeout > 10 ms; bus is busy. Exit
        if (check_timeout(start_tick, timeout_ms)) return false;
    }

    // Write the target device register address over I2C
    // device_addrs, num_bytes, autoend = true, start transmission
    // RD_WRN (bit 10) is left clear for a write
    i2c->CR2 = (dev_addr << 1) | (1 << 16) | (1 << 25) | (1 << 13);

    start_tick = (uint16_t)TIM16->CNT;                      // reset start_tick counter
    // Wait for communication of device address to complete
    while (!(i2c->ISR & (1 << 1))) {
        // Check status of TXIS
        if (check_timeout(start_tick, timeout_ms)) return false;
    }

    // Device address has been written, TXDR now empty
    i2c->TXDR = reg_addr;                                   // Address of register on peripheral to read

    start_tick = (uint16_t)TIM16->CNT;                      // Reset Start Tick counter
    while (!(i2c->ISR & (1 << 5))) {
        // Wait for communication with peripheral to be complete
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    i2c->ICR |= (1 << 5);                                   // Write 1 to clear stop flag

    // Small pause to check state of I2C line
    start_tick = (uint16_t)TIM16->CNT;                      // Reset Start Tick counter
    while (i2c->ISR & (1 << 15)) {
        // Check Busy flag. If timeout > 10 ms; bus is busy. Exit
        if (check_timeout(start_tick, timeout_ms)) return false;
    }

    // Read back requested bytes from attached peripheral
    // device_addrs, num_bytes, set to read, autoend = true, start transmission
    i2c->CR2 = (dev_addr << 1) | ((uint32_t)size << 16) | (1 << 10) | (1 << 25) | (1 << 13);

    // Populate buffer with number of bytes that are expected to be received from peripheral to micro
    for (uint16_t i = 0; i < size; i++) {
        start_tick = (uint16_t)TIM16->CNT;                  // Reset Start tick counter
        while (!(i2c->ISR & (1 << 2))) {
            // While no data has been received, check if a NACK transmitted across or timeout
            if (i2c->ISR & (1 << 4)) return false;
            if (check_timeout(start_tick, timeout_ms)) return false;
        }
        // Store received data in software buffer
        rx_buf[i] = (uint8_t)i2c->RXDR;
    }

    start_tick = (uint16_t)TIM16->CNT;                      // Reset Start Tick counter
    while (!(i2c->ISR & (1 << 5))) {
        // Wait for communication with peripheral to be complete
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    i2c->ICR |= (1 << 5);                                   // Write 1 to clear stop flag

    return true;
}
