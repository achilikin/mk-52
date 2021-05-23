/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */
/** turn on-board blue led on (ON on LOW)*/
static inline void led_off(void) {
	LED_PIN_GPIO_Port->BSRR = LED_PIN_Pin;
}

/** turn on-board blue led off (OFF on HIGH) */
static inline void led_on(void) {
	LED_PIN_GPIO_Port->BSRR = (uint32_t)(LED_PIN_Pin) << 16u;
}

/** turn on-board blue led for about 30ns (for sys clock 72MHz) */
static inline void led_marker(void) {
	 LED_PIN_GPIO_Port->BSRR = (uint32_t)(LED_PIN_Pin) << 16u;
	 LED_PIN_GPIO_Port->BSRR = LED_PIN_Pin;
}

/** turn on-board blue led for about 100ns (for sys clock 72MHz) */
static inline void led_marker100(void) {
	LED_PIN_GPIO_Port->BSRR = (uint32_t)(LED_PIN_Pin) << 16u;
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	LED_PIN_GPIO_Port->BSRR = LED_PIN_Pin;
}

/* use USB D+ (4.7K pull-up) pin for debugging */
/** turn USB D+ pin HIGH (default) */
static inline void dbg_high(void) {
	DBG_GPIO_Port->BSRR = DBG_Pin;
}

/** turn USB D+ pin LOW */
static inline void dbg_low(void) {
	DBG_GPIO_Port->BSRR = (uint32_t)(DBG_Pin) << 16u;
}

/** turn USB D+ pin LOW for about 30ns (for sys clock 72MHz) */
static inline void dbg_marker(void) {
	DBG_GPIO_Port->BSRR = (uint32_t)(DBG_Pin) << 16u;
	DBG_GPIO_Port->BSRR = DBG_Pin;
}

/** turn USB D+ pin LOW for about 100ns (for sys clock 72MHz) */
static inline void dbg_marker100(void) {
	DBG_GPIO_Port->BSRR = (uint32_t)(DBG_Pin) << 16u;
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	DBG_GPIO_Port->BSRR = DBG_Pin;
}

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
