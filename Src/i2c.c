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
    i2c->CR1 |= (1<<0);
}

// Busy-wait using TIM16 (1 tick = 1 ms)
void delay_ms(uint16_t ms) {
    uint16_t start = (uint16_t)TIM16->CNT;
    while (!check_timeout(start, ms));
}

bool check_timeout(uint16_t start_tick, uint16_t timeout_ms) {
    uint16_t time = (uint16_t)TIM16->CNT;
    return (uint16_t)(time - start_tick) >= timeout_ms; // cast: stays correct when CNT wraps
}

// Reset the peripheral after a failed transfer so the next call isn't stuck on BUSY
static void i2c_recover(I2C_TypeDef *i2c) {
    i2c->ICR = (1 << 4) | (1 << 5); // clear NACKF, STOPF
    i2c->CR1 &= ~(1 << 0);          // PE = 0 resets the I2C state machine
    i2c->CR1 |= (1 << 0);
}

// Transfer is finished when STOPF sets, or TXDR is empty and the bus is idle.
// (Don't rely on STOPF alone: it never set after AUTOEND in the simulator.)
volatile uint32_t i2c_last_isr = 0;   // ISR captured at the moment wait_for_stop times out

static bool wait_for_stop(I2C_TypeDef *i2c, uint16_t timeout_ms) {
    uint16_t start_tick = (uint16_t)TIM16->CNT;
    while (!(i2c->ISR & (1 << 5))) {                                 // STOPF
        if ((i2c->ISR & (1 << 0)) && !(i2c->ISR & (1 << 15))) break; // TXE && !BUSY
        if (check_timeout(start_tick, timeout_ms)) { i2c_last_isr = i2c->ISR; return false; }
    }
    i2c->ICR = (1 << 5);                                             // clear STOPF if set
    return true;
}

volatile int i2c_read_error = 0; // last i2c_master_readregs failure step (0 = OK), see i2c.h

static bool wait_bus_free(I2C_TypeDef *i2c, uint16_t timeout_ms) {
    uint16_t start_tick = (uint16_t)TIM16->CNT;
    while (i2c->ISR & (1 << 15)) {
        if (check_timeout(start_tick, timeout_ms)) return false;
    }
    return true;
}

static int readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size) {
    const uint16_t timeout_ms = 10;
    uint16_t start_tick;

    if (!wait_bus_free(i2c, timeout_ms)) return I2C_RD_ERR_BUSY;
    i2c->ICR = (1 << 4) | (1 << 5); // clear stale NACKF/STOPF from the previous transfer

    // Phase 1: write the register address (NBYTES = 1, AUTOEND, START)
    i2c->CR2 = (dev_addr << 1) | (1 << 16) | (1 << 25) | (1 << 13);

    start_tick = (uint16_t)TIM16->CNT;
    while (!(i2c->ISR & (1 << 1))) {                                  // TXIS
        if (i2c->ISR & (1 << 4)) return I2C_RD_ERR_ADDR_NACK;
        if (check_timeout(start_tick, timeout_ms)) return I2C_RD_ERR_ADDR_TIMEOUT;
    }
    i2c->TXDR = reg_addr;

    if (!wait_for_stop(i2c, timeout_ms)) return I2C_RD_ERR_WRITE_STOP;
    if (!wait_bus_free(i2c, timeout_ms)) return I2C_RD_ERR_BUSY_BETWEEN;

    // Phase 2: read `size` bytes (RD_WRN = 1, AUTOEND, START)
    i2c->CR2 = (dev_addr << 1) | ((uint32_t)size << 16) | (1 << 10) | (1 << 25) | (1 << 13);

    for (uint16_t i = 0; i < size; ++i) {
        start_tick = (uint16_t)TIM16->CNT;
        while (!(i2c->ISR & (1 << 2))) {                              // RXNE
            if (i2c->ISR & (1 << 4)) return I2C_RD_ERR_RX_NACK;
            if (check_timeout(start_tick, timeout_ms)) return I2C_RD_ERR_RX_TIMEOUT;
        }
        rx_buf[i] = (uint8_t)i2c->RXDR;
    }

    if (!wait_for_stop(i2c, timeout_ms)) return I2C_RD_ERR_READ_STOP;
    return I2C_OK;
}

bool i2c_master_readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size) {
    i2c_read_error = readregs(i2c, dev_addr, reg_addr, rx_buf, size);
    if (i2c_read_error != I2C_OK) {
        i2c_last_isr = i2c->ISR;
        i2c_recover(i2c);
        return false;
    }
    return true;
}

// Returns I2C_OK (0) on success, otherwise the step that failed (see i2c.h)
int i2c_master_writereg(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    const uint16_t timeout_ms = 10;
    uint16_t start_tick = (uint16_t)TIM16->CNT;

    // 1. wait for bus free
    while (i2c->ISR & (1 << 15)) {
        if (check_timeout(start_tick, timeout_ms)) { i2c_recover(i2c); return I2C_ERR_BUSY; }
    }

    i2c->ICR = (1 << 4) | (1 << 5); // clear stale NACKF/STOPF from the previous transfer

    // address, NBYTES = 2, AUTOEND, START (write direction: RD_WRN = 0)
    i2c->CR2 = (dev_addr << 1) | (2 << 16) | (1 << 25) | (1 << 13);

    // 2. wait for address to be ACKed (TXIS)
    start_tick = (uint16_t)TIM16->CNT;
    while (!(i2c->ISR & (1 << 1))) {
        if (i2c->ISR & (1 << 4)) { i2c_recover(i2c); return I2C_ERR_ADDR_NACK; }
        if (check_timeout(start_tick, timeout_ms)) { i2c_recover(i2c); return I2C_ERR_ADDR_TIMEOUT; }
    }
    i2c->TXDR = reg_addr;

    // 3. wait for the hardware to take the register byte before loading the next one.
    // TXIS can still read as set straight after the TXDR write; without this the data
    // byte overwrites reg_addr, only one byte is sent, and the transfer hangs waiting
    // for byte 2 (ISR = 0x800B: BUSY + TXIS + TXE).
    for (volatile uint32_t n = 0; (i2c->ISR & (1 << 1)) && n < 2000; n++);

    start_tick = (uint16_t)TIM16->CNT;
    while (!(i2c->ISR & (1 << 1))) {
        if (i2c->ISR & (1 << 4)) { i2c_recover(i2c); return I2C_ERR_REG_NACK; }
        if (check_timeout(start_tick, timeout_ms)) { i2c_recover(i2c); return I2C_ERR_REG_TIMEOUT; }
    }
    i2c->TXDR = data;

    // 4. wait for STOP
    if (!wait_for_stop(i2c, timeout_ms)) { i2c_recover(i2c); return I2C_ERR_STOP_TIMEOUT; }

    return I2C_OK;
}
