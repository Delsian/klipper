/*
 *  GPIO functions on STM32F042 boards.
 *
 *  Copyright (C) 2019 Eug Krashtan <eug.krashtan@gmail.com>
 *  This file may be distributed under the terms of the GNU GPLv3 license.
 *
 */

#include "stm32f0xx_hal.h"
#include "gpio.h" // gpio_out_setup
#include "internal.h" // GPIO


/****************************************************************
 * General Purpose Input Output (GPIO) pins
 ****************************************************************/

struct gpio_out
gpio_out_setup(uint8_t pin, uint8_t val)
{
    struct gpio_out g = { .regs=0, .bit=0 };
    return g;
}

void
gpio_out_write(struct gpio_out g, uint8_t val)
{

}

struct gpio_in
gpio_in_setup(uint8_t pin, int8_t pull_up)
{
    struct gpio_in g = { .regs=0, .bit=0 };
    return g;
}

uint8_t
gpio_in_read(struct gpio_in g)
{
    return 0;
}

void
gpio_out_toggle_noirq(struct gpio_out g)
{
}

void gpio_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : Endstop_Pin * / ToDo: Move it
  GPIO_InitStruct.Pin = Endstop_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Endstop_GPIO_Port, &GPIO_InitStruct); */
}
