/*
 * Serial over CAN emulation for STM32F042 boards.
 *
 *  Copyright (C) 2019 Eug Krashtan <eug.krashtan@gmail.com>
 *  This file may be distributed under the terms of the GNU GPLv3 license.
 *
 */

#include "stm32f0xx_hal.h"
#include <string.h>
#include "can.h"
#include "command.h" // encoder

CAN_HandleTypeDef hcan;
static CanTxMsgTypeDef    TxMessage;
static CanRxMsgTypeDef    RxMessage;

static uint16_t MyCanId = 0;

void CanInit(void)
{
	hcan.Instance = CAN;
	hcan.Init.Prescaler = 6;
	hcan.Init.Mode = CAN_MODE_NORMAL;
	hcan.Init.SJW = CAN_SJW_1TQ;
	hcan.Init.BS1 = CAN_BS1_5TQ;
	hcan.Init.BS2 = CAN_BS2_2TQ;
	hcan.Init.TTCM = DISABLE;
	hcan.Init.ABOM = DISABLE;
	hcan.Init.AWUM = DISABLE;
	hcan.Init.NART = ENABLE;
	hcan.Init.RFLM = DISABLE;
	hcan.Init.TXFP = DISABLE;
	HAL_CAN_Init(&hcan);
	hcan.pTxMsg = &TxMessage;
	hcan.pRxMsg = &RxMessage;

	/*##-2- Configure the CAN Filter ###########################################*/
	CAN_FilterConfTypeDef sFilterConfig;
	sFilterConfig.FilterNumber = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = PKT_ID_UUID<<5;
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = PKT_ID_SET<<5;
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterFIFOAssignment = CAN_FIFO0;
	sFilterConfig.FilterActivation = ENABLE;
	HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);

	/*##-3- Configure Transmission process #####################################*/
	hcan.pTxMsg->ExtId = 0;
	HAL_CAN_Receive_IT(&hcan, CAN_FIFO0);
}

void CanTransmit(uint32_t id, uint32_t dlc, uint8_t *pkt)
{
	memcpy(hcan.pTxMsg->Data, pkt, dlc);
	hcan.pTxMsg->StdId = id;
	hcan.pTxMsg->DLC = dlc;
	HAL_CAN_Transmit(&hcan, 1000);
}

// Convert Unique 96-bit value into 48 bit representation
static void PackUuid(uint8_t* u)
{
	for(int i=0; i<SHORT_UUID_LEN; i++) {
		u[i] = *((uint8_t*)(STM32_UUID_ADDR+i)) ^
				*((uint8_t*)(STM32_UUID_ADDR+i+SHORT_UUID_LEN));
	}
}

static void CanUUIDResp(void)
{
	uint8_t short_uuid[SHORT_UUID_LEN];
	PackUuid(short_uuid);
	CanTransmit(PKT_ID_UUID_RESP, SHORT_UUID_LEN, short_uuid);
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* h) {
	if(!MyCanId) { // If serial not assigned yet
		if(h->pRxMsg->StdId == PKT_ID_UUID) {
			// Just inform host about my UUID
			CanUUIDResp();
		} else if (h->pRxMsg->StdId == PKT_ID_SET) {
			// compare my UUID with packet to check if this packet mine
			uint8_t short_uuid[SHORT_UUID_LEN];
			PackUuid(short_uuid);
			if (memcmp(&(h->pRxMsg->Data[2]), short_uuid, SHORT_UUID_LEN) == 0) {
				memcpy(&MyCanId, h->pRxMsg->Data, sizeof(uint16_t));
				CAN_FilterConfTypeDef sFilterConfig;
				sFilterConfig.FilterNumber = 0;
				sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
				sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
				sFilterConfig.FilterIdHigh = MyCanId<<5;
				sFilterConfig.FilterIdLow = 0x0000;
				sFilterConfig.FilterMaskIdHigh = 0xFFFF;
				sFilterConfig.FilterMaskIdLow = 0x0000;
				sFilterConfig.FilterFIFOAssignment = CAN_FIFO0;
				sFilterConfig.FilterActivation = ENABLE;
				// Disable 'set address' filter and enable only my packets
				HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);
			}
		}
	} else {
		if(h->pRxMsg->StdId == MyCanId) {
			memcpy(TxMessage.Data,h->pRxMsg->Data,h->pRxMsg->DLC);
			TxMessage.Data[0]++;
			hcan.pTxMsg->DLC = h->pRxMsg->DLC;
			hcan.pTxMsg->StdId = MyCanId+1;
			HAL_CAN_Transmit(&hcan, 1000);
		}
	}
	HAL_CAN_Receive_IT(&hcan, CAN_FIFO0);
}

/**
  * @brief This function handles HDMI-CEC and CAN global interrupts / HDMI-CEC wake-up interrupt through EXTI line 27.
  */
void CEC_CAN_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan);
}

// Generate messages - only used for ack/nak messages
void
console_sendf(const struct command_encoder *ce, va_list args)
{
    //uint8_t buf[MESSAGE_MIN];
}
