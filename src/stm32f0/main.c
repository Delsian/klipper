/*
 * Main starting point for STM32F042 boards.
 *
 *  Copyright (C) 2019 Eug Krashtan <eug.krashtan@gmail.com>
 *  This file may be distributed under the terms of the GNU GPLv3 license.
 *
 */

#include "stm32f0xx_hal.h"
#include "autoconf.h"
#include "command.h" // DECL_CONSTANT
#include "board/misc.h" // timer_read_time
#include "sched.h" // sched_main
#include "internal.h"
#include "can.h"

DECL_CONSTANT(MCU, "stm32f042");

/****************************************************************
 * dynamic memory pool
 ****************************************************************/

static char dynmem_pool[4 * 1024];

// Return the start of memory available for dynamic allocations
void *
dynmem_start(void)
{
    return dynmem_pool;
}

// Return the end of memory available for dynamic allocations
void *
dynmem_end(void)
{
    return &dynmem_pool[sizeof(dynmem_pool)];
}

/****************************************************************
 * misc functions
 ****************************************************************/

void
command_reset(uint32_t *args)
{
    NVIC_SystemReset();
}
DECL_COMMAND_FLAGS(command_reset, HF_IN_SHUTDOWN, "reset");

void clock_config(void)
{
	  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	  /**Initializes the CPU, AHB and APB busses clocks
	  */
	  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14|RCC_OSCILLATORTYPE_HSI48
	                              |RCC_OSCILLATORTYPE_LSI;
	  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
	  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	  RCC_OscInitStruct.HSI14CalibrationValue = 16;
	  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	  HAL_RCC_OscConfig(&RCC_OscInitStruct);

	  /**Initializes the CPU, AHB and APB busses clocks
	  */
	  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
	                              |RCC_CLOCKTYPE_PCLK1;
	  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
	  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
	  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);

	  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RTC;
	  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
	  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

// Implement simple early-boot delay mechanism
void
udelay(uint32_t usecs)
{
    uint32_t end = timer_read_time() + timer_from_us(usecs);
    while (timer_is_before(timer_read_time(), end))
        ;
}

static void rtc_init(void)
{
	RTC_HandleTypeDef hrtc;
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 127;
	hrtc.Init.SynchPrediv = 255;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	HAL_RTC_Init(&hrtc);
}

// Main entry point
int
main(void)
{
	HAL_Init();

    clock_config();
    gpio_init();
    CanInit();
    rtc_init();
    sched_main();
    return 0;
}



