// Host-side stand-in for the CMSIS device header, used ONLY by the unit tests.
// Provides just enough of the register types for the logic code to compile on a PC.
#pragma once
#include <stdint.h>

typedef struct { volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR; } GPIO_TypeDef;
typedef struct { volatile uint32_t CR1, CR2, ISR, ICR, TXDR, RXDR, TIMINGR; } I2C_TypeDef;
typedef struct { volatile uint32_t CNT; } TIM_TypeDef;

extern GPIO_TypeDef fake_gpioa, fake_gpiob;
extern I2C_TypeDef  fake_i2c1;
extern TIM_TypeDef  fake_tim16;

#define GPIOA (&fake_gpioa)
#define GPIOB (&fake_gpiob)
#define I2C1  (&fake_i2c1)
#define TIM16 (&fake_tim16)
