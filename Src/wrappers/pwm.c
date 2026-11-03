#include "pwm.h"
#include "gpio.h"
#include "stm32c031xx.h"

// TIM3 PWM: CH1/2 in CCMR1, CH3/4 in CCMR2. Then CCR pulse and CCER enable.
static void set_ccmr_register_tim3(uint8_t channel, uint16_t duty_us, cc_mode cc, pwm_mode pwm) {
    switch (channel) {
        case 1:
            TIM3->CCMR1 &= ~(3 << 0 | 7 << 4); // CC1S, OC1M
            TIM3->CCMR1 |= (cc << 0 | pwm << 4);
            TIM3->CCR1 = duty_us;
            TIM3->CCER |= (1 << 0); // CC1E
            break;
        case 2:
            TIM3->CCMR1 &= ~(3 << 8 | 7 << 12); // CC2S, OC2M
            TIM3->CCMR1 |= (cc << 8 | pwm << 12);
            TIM3->CCR2 = duty_us;
            TIM3->CCER |= (1 << 4); // CC2E
            break;
        case 3:
            TIM3->CCMR2 &= ~(3 << 0 | 7 << 4); // CC3S, OC3M
            TIM3->CCMR2 |= (cc << 0 | pwm << 4);
            TIM3->CCR3 = duty_us;
            TIM3->CCER |= (1 << 8); // CC3E
            break;
        case 4:
            TIM3->CCMR2 &= ~(3 << 8 | 7 << 12); // CC4S, OC4M
            TIM3->CCMR2 |= (cc << 8 | pwm << 12);
            TIM3->CCR4 = duty_us;
            TIM3->CCER |= (1 << 12); // CC4E
            break;
        default:
            break;
    }
}

// Put this pin on TIM3 PWM and load the first CCR value
void enable_cap_com(GPIO_TypeDef * port, uint8_t pin, uint8_t channel, uint8_t af, uint16_t duty_us) {
    init_pin_af(port, pin, af);
    set_ccmr_register_tim3(channel, duty_us, CCM_OUTPUT, PWM_MODE_1);
    TIM3->EGR |= (1 << 0); // load PSC/ARR/CCR into the live registers
}
