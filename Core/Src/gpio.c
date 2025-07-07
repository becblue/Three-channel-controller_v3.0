/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
extern volatile uint32_t fan_pulse_count;
#include "relay_control.h"
#include "safety_monitor.h"
#include "usart.h"
#include "log_system.h"

/* PC13按键长按检测变量 */
volatile uint32_t key1_press_start_time = 0;
volatile uint8_t key1_pressed = 0;
volatile uint8_t key1_long_press_triggered = 0;

/* PC14按键长按检测变量 */
volatile uint32_t key2_press_start_time = 0;
volatile uint8_t key2_pressed = 0;
volatile uint8_t key2_long_press_triggered = 0;
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, K1_1_ON_Pin|K1_1_OFF_Pin|K2_1_ON_Pin|K2_1_OFF_Pin
                          |K3_1_OFF_Pin|K3_1_ON_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, K1_2_OFF_Pin|K2_2_ON_Pin|K2_2_OFF_Pin|K3_2_OFF_Pin
                          |K1_2_ON_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, FLASH_CS_Pin|BEEP_Pin|ALARM_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(K3_2_ON_GPIO_Port, K3_2_ON_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PCPin PCPin */
  GPIO_InitStruct.Pin = KEY1_Pin|KEY2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PCPin PCPin PCPin PCPin
                           PCPin PCPin */
  GPIO_InitStruct.Pin = K1_1_ON_Pin|K1_1_OFF_Pin|K2_1_ON_Pin|K2_1_OFF_Pin
                          |K3_1_OFF_Pin|K3_1_ON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PAPin PAPin PAPin PAPin
                           PAPin */
  GPIO_InitStruct.Pin = K1_2_OFF_Pin|K2_2_ON_Pin|K2_2_OFF_Pin|K3_2_OFF_Pin
                          |K1_2_ON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PCPin PCPin PCPin PCPin */
  GPIO_InitStruct.Pin = K1_1_STA_Pin|K2_1_STA_Pin|SW3_STA_Pin|SW2_STA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PBPin PBPin PBPin PBPin */
  GPIO_InitStruct.Pin = K3_1_STA_Pin|K1_2_STA_Pin|K2_2_STA_Pin|K3_2_STA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = FLASH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(FLASH_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = SW1_STA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(SW1_STA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = RS485_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(RS485_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = K3_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(K3_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = FAN_SEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(FAN_SEN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = K3_2_ON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(K3_2_ON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PBPin PBPin */
  GPIO_InitStruct.Pin = BEEP_Pin|ALARM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PBPin PBPin PBPin */
  GPIO_InitStruct.Pin = DC_CTRL_Pin|K2_EN_Pin|K1_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 4, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 4, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 2 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch(GPIO_Pin)
    {
        case K1_EN_Pin:
            // K1_EN中断处理：读取当前状态并传递给继电器控制模块
            RelayControl_HandleEnableSignal(1, HAL_GPIO_ReadPin(K1_EN_GPIO_Port, K1_EN_Pin));
            break;
            
        case K2_EN_Pin:
            // K2_EN中断处理：读取当前状态并传递给继电器控制模块
            RelayControl_HandleEnableSignal(2, HAL_GPIO_ReadPin(K2_EN_GPIO_Port, K2_EN_Pin));
            break;
            
        case K3_EN_Pin:
            // K3_EN中断处理：读取当前状态并传递给继电器控制模块
            RelayControl_HandleEnableSignal(3, HAL_GPIO_ReadPin(K3_EN_GPIO_Port, K3_EN_Pin));
            break;
            
        case DC_CTRL_Pin:
            // 电源监控中断处理：调用安全监控模块的电源异常回调
            DEBUG_Printf("DC_CTRL中断触发\r\n");
            SafetyMonitor_PowerFailureCallback();
            break;
            
        case KEY1_Pin: // PC13按键中断处理
            // 检测按键状态
            if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) {
                // 按键按下（下降沿）
                key1_pressed = 1;
                key1_press_start_time = HAL_GetTick();
                key1_long_press_triggered = 0;
            } else {
                // 按键松开（上升沿）
                key1_pressed = 0;
                key1_long_press_triggered = 0;
            }
            break;
            
        case KEY2_Pin: // PC14按键中断处理
            // 检测按键状态
            if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) {
                // 按键按下（下降沿）
                key2_pressed = 1;
                key2_press_start_time = HAL_GetTick();
                key2_long_press_triggered = 0;
            } else {
                // 按键松开（上升沿）
                key2_pressed = 0;
                key2_long_press_triggered = 0;
            }
            break;
            
        case GPIO_PIN_12: // PC12风扇测速
            fan_pulse_count++;
            break;
            
        default:
            break;
    }
}

/* USER CODE END 2 */
