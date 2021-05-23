/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Elektronika MK-52 display scanner
  *
  * MIT License
  *
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"

#include "target.h"
#include "lib/ticker.h"

#ifdef __cplusplus
extern "C" {
#endif

void Error_Handler(void);
void SystemClock_Config(void);

#define SCAN_Pin GPIO_PIN_0
#define SCAN_GPIO_Port GPIOB
#define SCAN_EXTI_IRQn EXTI0_IRQn

#define SEG_A   0x01
#define SEG_B   0x02
#define SEG_C   0x04
#define SEG_D   0x08
#define SEG_E   0x10
#define SEG_F   0x20
#define SEG_G   0x40
#define SEG_DOT 0x80

#define SEG_A_Pin 	GPIO_PIN_0
#define SEG_B_Pin 	GPIO_PIN_1
#define SEG_C_Pin 	GPIO_PIN_2
#define SEG_D_Pin 	GPIO_PIN_3
#define SEG_E_Pin 	GPIO_PIN_4
#define SEG_F_Pin 	GPIO_PIN_5
#define SEG_G_Pin 	GPIO_PIN_6
#define SEG_DOT_Pin GPIO_PIN_7

#define SEG_GPIO_Port GPIOA

#define SEG_PINS (SEG_A_Pin | SEG_B_Pin | SEG_C_Pin | SEG_D_Pin | SEG_E_Pin | SEG_F_Pin | SEG_G_Pin | SEG_DOT_Pin)

/* use pulled-up onboard LED (C13) and USB D+ (A12) pins for debugging */
#define LED_PIN_Pin GPIO_PIN_13
#define LED_PIN_GPIO_Port GPIOC

#define DBG_Pin GPIO_PIN_12
#define DBG_GPIO_Port GPIOA

#define SPI_DA_Pin GPIO_PIN_12
#define SPI_CS_Pin GPIO_PIN_14

#define SPI_DA_GPIO_Port GPIOB
#define SPI_CS_GPIO_Port GPIOB

extern volatile uint32_t scan_period; /** interval between scan pin interrupts, in sys clocks */
volatile uint32_t tim_arr;			  /** timer auto reload register */

#define APP_PRINT_HEX_SCAN 0x01 /** print hex scan codes */
#define APP_PRINT_KEY_SCAN 0x02 /** print changes in key scans */

extern uint8_t app_flags;

#ifdef __cplusplus
}
#endif
#endif /* __MAIN_H */