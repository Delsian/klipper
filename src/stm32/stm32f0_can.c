/*
 * Serial over CAN emulation for STM32F042 boards.
 *
 *  Copyright (C) 2019 Eug Krashtan <eug.krashtan@gmail.com>
 *  This file may be distributed under the terms of the GNU GPLv3 license.
 *
 */

#include "autoconf.h" // CONFIG_SERIAL_BAUD
#include "board/armcm_boot.h" // armcm_enable_irq
#include "board/serial_irq.h" // serial_rx_byte
#include "command.h" // DECL_CONSTANT_STR
#include "internal.h" // enable_pclock
#include "sched.h" // DECL_INIT
#include <string.h>
#include "stm32f0_can.h"

static uint16_t MyCanId = 0;

void CanTransmit(uint32_t id, uint32_t dlc, uint8_t *pkt)
{

}


void CanInit(void)
{

}
DECL_INIT(CanInit);

void HAL_CAN_RxCpltCallback(void* h) {

}

void HAL_CAN_ErrorCallback()
{

}

/**
  * @brief This function handles HDMI-CEC and CAN global interrupts /
  *  HDMI-CEC wake-up interrupt through EXTI line 27.
  */
void CEC_CAN_IRQHandler(void)
{
    //HAL_CAN_IRQHandler(&hcan);
}

void
serial_enable_tx_irq(void)
{
    if(MyCanId == 0)
        // Serial port not initialized
        return;

    int i=0;
    for (;i<8;)
    {

        i++;
    }
    if (i>0) {

    }
}

