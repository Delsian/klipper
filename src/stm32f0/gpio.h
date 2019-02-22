#ifndef __STM32F0_GPIO_H
#define __STM32F0_GPIO_H

#include <stdint.h>
#include "stm32f0xx_hal.h"

void gpio_peripheral(char bank, uint32_t bit, char ptype, uint32_t pull_up);

struct gpio_out {
    GPIO_TypeDef *regs;
    uint32_t bit;
};
struct gpio_out gpio_out_setup(uint8_t pin, uint8_t val);
void gpio_out_reset(struct gpio_out g, uint8_t val);
void gpio_out_toggle_noirq(struct gpio_out g);
void gpio_out_toggle(struct gpio_out g);
void gpio_out_write(struct gpio_out g, uint8_t val);

struct gpio_in {
    GPIO_TypeDef *regs;
    uint32_t bit;
};
struct gpio_in gpio_in_setup(uint8_t pin, int8_t pull_up);
void gpio_in_reset(struct gpio_in g, int8_t pull_up);
uint8_t gpio_in_read(struct gpio_in g);


#endif // gpio.h
