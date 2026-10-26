#pragma once

#include "stm32c031xx.h"
#include <stdint.h>
#include <stdbool.h>

void enable_i2c_clock();
void i2c_master_init(I2C_TypeDef *i2c, uint8_t presc, uint8_t sclh, uint8_t scll);
bool check_timeout(uint16_t start_tick, uint16_t timeout_ms);
bool i2c_master_readregs(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *rx_buf, uint16_t size);
bool i2c_master_writereg(I2C_TypeDef *i2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t data);   