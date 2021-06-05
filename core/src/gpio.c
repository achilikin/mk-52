/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/* Configure GPIO pins Output Levels */
	HAL_GPIO_WritePin(DBG_GPIO_Port, DBG_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_PIN_GPIO_Port, LED_PIN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(OLED_DC_GPIO_Port,  OLED_DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(OLED_CS_GPIO_Port,  OLED_CS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_SET);

	/* Configure GPIO output pins: LED pin and USB D+ pin*/
	GPIO_InitStruct.Pin = LED_PIN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; /* all pins will use High speed */
	HAL_GPIO_Init(LED_PIN_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = DBG_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(DBG_GPIO_Port, &GPIO_InitStruct);

	/* Configure GPIO SEGMENTs pins */
	GPIO_InitStruct.Pin = SEG_A_Pin | SEG_B_Pin | SEG_C_Pin | SEG_D_Pin |
						  SEG_E_Pin | SEG_F_Pin | SEG_G_Pin | SEG_DOT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(SEG_GPIO_Port, &GPIO_InitStruct);

	/* Configure GPIO GRID pin for scanning start interrupt */
	GPIO_InitStruct.Pin = SCAN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(SCAN_GPIO_Port, &GPIO_InitStruct);

	/* Configure GPIO OLED CS, DC and RST pins */
	GPIO_InitStruct.Pin = OLED_DC_Pin | OLED_CS_Pin | OLED_RST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(OLED_DC_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init for GRID_8 pin */
	HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
