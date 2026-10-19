#pragma once

#include "stm32c0xx.h"

// Clock TIMx, set PSC/ARR, enable update IRQ, start the counter.
// Must provide TIMx_IRQHandler. NVIC priority left at reset.
void enable_timer(TIM_TypeDef * tim, uint16_t arr, uint16_t psc);
