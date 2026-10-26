#pragma once

#include "stm32c031xx.h"
#include <stdint.h>
#include <stdbool.h>

void enable_i2c_clock();
void i2c_master_init(I2C_TypeDef *i2c, uint8_t presc, uint8_t sclh, uint8_t scll);
bool check_timeout(uint16_t start_tick, uint16_t timeout_ms);
bool i2c_master_readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size);
// i2c_master_writereg return codes
#define I2C_OK               0
#define I2C_ERR_BUSY         1  // bus never went idle
#define I2C_ERR_ADDR_NACK    2  // device didn't ACK its address (wiring / address)
#define I2C_ERR_ADDR_TIMEOUT 3  // START/address never completed (pull-ups, clock, PE)
#define I2C_ERR_REG_NACK     4
#define I2C_ERR_REG_TIMEOUT  5
#define I2C_ERR_STOP_TIMEOUT 6
#define I2C_ERR_WHO_AM_I     7  // write worked but WHO_AM_I read failed or wasn't 0x68

// i2c_read_error values (set by i2c_master_readregs)
#define I2C_RD_ERR_BUSY          11 // bus never went idle
#define I2C_RD_ERR_ADDR_NACK     12 // no ACK to address (write phase)
#define I2C_RD_ERR_ADDR_TIMEOUT  13 // address never completed (write phase)
#define I2C_RD_ERR_WRITE_STOP    14 // register-address write never finished
#define I2C_RD_ERR_BUSY_BETWEEN  15 // bus stayed busy between write and read
#define I2C_RD_ERR_RX_NACK       16 // NACK while reading
#define I2C_RD_ERR_RX_TIMEOUT    17 // no byte received
#define I2C_RD_ERR_READ_STOP     18 // read never finished

int i2c_master_writereg(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
void delay_ms(uint16_t ms);   