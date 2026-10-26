#include "i2c.h"
#include "stm32c031xx.h"
#include <stdint.h>

void enable_i2c_clock() {
    RCC->APBENR1 &= ~(1 << 21);
    RCC->APBENR1 |= (1 << 21);
}

void i2c_master_init(I2C_TypeDef *i2c, uint8_t presc, uint8_t sclh, uint8_t scll) {
    i2c->CR1 &= ~(1 << 0);
    i2c->TIMINGR = ((uint32_t)presc << 28) | ((uint32_t)sclh << 8) | ((uint32_t)scll << 0);
    i2c->CR1 |= (1 << 0);
}

bool check_timeout(uint16_t start_tick, uint16_t timeout_ms) {
    uint16_t time = (uint16_t)TIM16->CNT;
    return (uint16_t)(time - start_tick) >= timeout_ms; // cast keeps this correct when CNT wraps
}

// Reset the peripheral after a failed transfer so the next call isn't stuck on BUSY
static void i2c_recover(I2C_TypeDef *i2c) {
    i2c->ICR = (1 << 4) | (1 << 5); // clear NACKF, STOPF
    i2c->CR1 &= ~(1 << 0);          // PE = 0 resets the I2C state machine
    i2c->CR1 |= (1 << 0);
}

// Wait for the bus to go idle (BUSY clear)
static bool wait_bus_free(I2C_TypeDef *i2c, uint16_t timeout_ms) {
    uint16_t start_tick = (uint16_t)TIM16->CNT;
    while (i2c->ISR & (1 << 15)) {
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    return true;
}

// Wait for TXIS (ready for next byte), failing on NACK
static bool wait_txis(I2C_TypeDef *i2c, uint16_t timeout_ms) {
    uint16_t start_tick = (uint16_t)TIM16->CNT;
    while (!(i2c->ISR & (1 << 1))) {
        if (i2c->ISR & (1 << 4)) return false;
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    return true;
}

// Transfer is finished when STOPF sets, or TXDR is empty and the bus is idle
static bool wait_for_stop(I2C_TypeDef *i2c, uint16_t timeout_ms) {
    uint16_t start_tick = (uint16_t)TIM16->CNT;
    while (!(i2c->ISR & (1 << 5))) {                                 // STOPF
        if ((i2c->ISR & (1 << 0)) && !(i2c->ISR & (1 << 15))) break; // TXE && !BUSY
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    i2c->ICR = (1 << 5);                                             // clear STOPF if set
    return true;
}

static bool readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size) {
    const uint16_t timeout_ms = 10;

    if (!wait_bus_free(i2c, timeout_ms)) return false;
    i2c->ICR = (1 << 4) | (1 << 5); // clear stale NACKF/STOPF from the previous transfer

    // Phase 1: write the register address (NBYTES = 1, AUTOEND, START)
    i2c->CR2 = (dev_addr << 1) | (1 << 16) | (1 << 25) | (1 << 13);
    if (!wait_txis(i2c, timeout_ms)) return false;
    i2c->TXDR = reg_addr;

    if (!wait_for_stop(i2c, timeout_ms)) return false;
    if (!wait_bus_free(i2c, timeout_ms)) return false;

    // Phase 2: read `size` bytes (RD_WRN = 1, AUTOEND, START)
    i2c->CR2 = (dev_addr << 1) | ((uint32_t)size << 16) | (1 << 10) | (1 << 25) | (1 << 13);

    for (uint16_t i = 0; i < size; ++i) {
        uint16_t start_tick = (uint16_t)TIM16->CNT;
        while (!(i2c->ISR & (1 << 2))) {                              // RXNE
            if (i2c->ISR & (1 << 4)) return false;                    // NACK
            if (check_timeout(start_tick, timeout_ms)) return false;
        }
        rx_buf[i] = (uint8_t)i2c->RXDR;
    }

    return wait_for_stop(i2c, timeout_ms);
}

bool i2c_master_readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size) {
    if (!readregs(i2c, dev_addr, reg_addr, rx_buf, size)) {
        i2c_recover(i2c);
        return false;
    }
    return true;
}

static bool writereg(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    const uint16_t timeout_ms = 10;

    if (!wait_bus_free(i2c, timeout_ms)) return false;
    i2c->ICR = (1 << 4) | (1 << 5); // clear stale NACKF/STOPF from the previous transfer

    // address, NBYTES = 2, AUTOEND, START (write direction: RD_WRN = 0)
    i2c->CR2 = (dev_addr << 1) | (2 << 16) | (1 << 25) | (1 << 13);

    if (!wait_txis(i2c, timeout_ms)) return false; // address ACKed
    i2c->TXDR = reg_addr;

    // Let the hardware take reg_addr before loading the next byte. TXIS can still read
    // as set straight after the TXDR write; without this, data overwrites reg_addr,
    // only one byte is sent, and the transfer hangs waiting for byte 2.
    for (volatile uint32_t n = 0; (i2c->ISR & (1 << 1)) && n < 2000; n++);

    if (!wait_txis(i2c, timeout_ms)) return false; // register byte sent
    i2c->TXDR = data;

    return wait_for_stop(i2c, timeout_ms);
}

bool i2c_master_writereg(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    if (!writereg(i2c, dev_addr, reg_addr, data)) {
        i2c_recover(i2c);
        return false;
    }
    return true;
}
