/**
  ******************************************************************************
  * @file    iwdg.c
  * @brief   This file provides code for the configuration
  *          of the IWDG instances.
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

/* Includes ------------------------------------------------------------------*/
#include "iwdg.h"
#include "usart.h"
#include "stm32f1xx_hal.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

// 简化的IWDG句柄结构（针对STM32F1xx）
IWDG_TypeDef* hiwdg_instance = IWDG;

/* IWDG init function */
void MX_IWDG_Init(void)
{
  /* USER CODE BEGIN IWDG_Init 0 */
  DEBUG_Printf("=== IWDG初始化开始 ===\r\n");
  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  
  // 第一步：启动LSI时钟（IWDG必需）
  DEBUG_Printf("启动LSI时钟...\r\n");
  RCC->CSR |= RCC_CSR_LSION;  // 启动LSI时钟
  
  // 等待LSI时钟就绪，添加超时保护
  uint32_t timeout = 100000;  // 超时计数器
  while((RCC->CSR & RCC_CSR_LSIRDY) == 0 && timeout > 0) {
    timeout--;
  }
  
  if(timeout == 0) {
    DEBUG_Printf("LSI时钟启动失败！\r\n");
    return;  // LSI启动失败，退出初始化
  }
  DEBUG_Printf("LSI时钟启动成功\r\n");
  
  // 第二步：配置IWDG寄存器
  DEBUG_Printf("配置IWDG寄存器...\r\n");
  
  // 解锁IWDG寄存器写保护
  IWDG->KR = 0x5555;
  
  // 设置预分频器：64 (LSI/64)
  IWDG->PR = 0x04;  // IWDG_PRESCALER_64 = 0x04
  
  // 设置重装载值：1875 (约3秒超时)
  IWDG->RLR = 1875;
  
  // 等待寄存器更新完成，添加超时保护
  timeout = 100000;
  while((IWDG->SR != 0) && timeout > 0) {
    timeout--;
  }
  
  if(timeout == 0) {
    DEBUG_Printf("IWDG寄存器更新超时，但继续启动...\r\n");
  }
  
  // 第三步：启动IWDG
  DEBUG_Printf("启动IWDG看门狗...\r\n");
  IWDG->KR = 0xCCCC;
  
  DEBUG_Printf("IWDG初始化完成：超时时间3秒\r\n");
  
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */
}

/**
  * @brief  刷新IWDG看门狗
  * @param  None
  * @retval None
  */
void IWDG_Refresh(void)
{
  IWDG->KR = 0xAAAA;  // 喂狗
  // 简化版本，不打印太多调试信息避免影响性能
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */ 


