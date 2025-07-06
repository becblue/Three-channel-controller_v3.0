/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// 调试信息输出函数声明
void DEBUG_Printf(const char *format, ...);

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY1_Pin GPIO_PIN_13
#define KEY1_GPIO_Port GPIOC
#define KEY1_EXTI_IRQn EXTI15_10_IRQn
#define KEY2_Pin GPIO_PIN_14
#define KEY2_GPIO_Port GPIOC
#define KEY2_EXTI_IRQn EXTI15_10_IRQn
#define K1_1_ON_Pin GPIO_PIN_0
#define K1_1_ON_GPIO_Port GPIOC
#define K1_1_OFF_Pin GPIO_PIN_1
#define K1_1_OFF_GPIO_Port GPIOC
#define K2_1_ON_Pin GPIO_PIN_2
#define K2_1_ON_GPIO_Port GPIOC
#define K2_1_OFF_Pin GPIO_PIN_3
#define K2_1_OFF_GPIO_Port GPIOC
#define K1_2_OFF_Pin GPIO_PIN_3
#define K1_2_OFF_GPIO_Port GPIOA
#define K2_2_ON_Pin GPIO_PIN_4
#define K2_2_ON_GPIO_Port GPIOA
#define K2_2_OFF_Pin GPIO_PIN_5
#define K2_2_OFF_GPIO_Port GPIOA
#define K3_2_OFF_Pin GPIO_PIN_7
#define K3_2_OFF_GPIO_Port GPIOA
#define K1_1_STA_Pin GPIO_PIN_4
#define K1_1_STA_GPIO_Port GPIOC
#define K2_1_STA_Pin GPIO_PIN_5
#define K2_1_STA_GPIO_Port GPIOC
#define K3_1_STA_Pin GPIO_PIN_0
#define K3_1_STA_GPIO_Port GPIOB
#define K1_2_STA_Pin GPIO_PIN_1
#define K1_2_STA_GPIO_Port GPIOB
#define K2_2_STA_Pin GPIO_PIN_10
#define K2_2_STA_GPIO_Port GPIOB
#define K3_2_STA_Pin GPIO_PIN_11
#define K3_2_STA_GPIO_Port GPIOB
#define K1_2_ON_Pin GPIO_PIN_12
#define K1_2_ON_GPIO_Port GPIOB
#define K3_1_OFF_Pin GPIO_PIN_6
#define K3_1_OFF_GPIO_Port GPIOC
#define K3_1_ON_Pin GPIO_PIN_7
#define K3_1_ON_GPIO_Port GPIOC
#define SW3_STA_Pin GPIO_PIN_8
#define SW3_STA_GPIO_Port GPIOC
#define SW2_STA_Pin GPIO_PIN_9
#define SW2_STA_GPIO_Port GPIOC
#define SW1_STA_Pin GPIO_PIN_8
#define SW1_STA_GPIO_Port GPIOA
#define RS485_EN_Pin GPIO_PIN_11
#define RS485_EN_GPIO_Port GPIOA
#define K3_EN_Pin GPIO_PIN_15
#define K3_EN_GPIO_Port GPIOA
#define K3_EN_EXTI_IRQn EXTI15_10_IRQn
#define FAN_SEN_Pin GPIO_PIN_12
#define FAN_SEN_GPIO_Port GPIOC
#define FAN_SEN_EXTI_IRQn EXTI15_10_IRQn
#define K3_2_ON_Pin GPIO_PIN_2
#define K3_2_ON_GPIO_Port GPIOD
#define BEEP_Pin GPIO_PIN_3
#define BEEP_GPIO_Port GPIOB
#define ALARM_Pin GPIO_PIN_4
#define ALARM_GPIO_Port GPIOB
#define DC_CTRL_Pin GPIO_PIN_5
#define DC_CTRL_GPIO_Port GPIOB
#define DC_CTRL_EXTI_IRQn EXTI9_5_IRQn
#define K2_EN_Pin GPIO_PIN_8
#define K2_EN_GPIO_Port GPIOB
#define K2_EN_EXTI_IRQn EXTI9_5_IRQn
#define K1_EN_Pin GPIO_PIN_9
#define K1_EN_GPIO_Port GPIOB
#define K1_EN_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
