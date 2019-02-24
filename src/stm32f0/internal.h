#ifndef __STM32F0_INTERNAL_H
#define __STM32F0_INTERNAL_H
// Local definitions for STM32F0 code

#include "stm32f0xx.h"

extern uint8_t const digital_pins[];

void udelay(uint32_t usecs);
void gpio_init(void);

#endif // internal.h
