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

// �򻯵�IWDG����ṹ�����STM32F1xx��
IWDG_TypeDef* hiwdg_instance = IWDG;

/* IWDG init function */
void MX_IWDG_Init(void)
{
  /* USER CODE BEGIN IWDG_Init 0 */
  DEBUG_Printf("=== IWDG��ʼ����ʼ ===\r\n");
  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  
  // ��һ��������LSIʱ�ӣ�IWDG���裩
  DEBUG_Printf("����LSIʱ��...\r\n");
  RCC->CSR |= RCC_CSR_LSION;  // ����LSIʱ��
  
  // �ȴ�LSIʱ�Ӿ�������ӳ�ʱ����
  uint32_t timeout = 100000;  // ��ʱ������
  while((RCC->CSR & RCC_CSR_LSIRDY) == 0 && timeout > 0) {
    timeout--;
  }
  
  if(timeout == 0) {
    DEBUG_Printf("LSIʱ������ʧ�ܣ�\r\n");
    return;  // LSI����ʧ�ܣ��˳���ʼ��
  }
  DEBUG_Printf("LSIʱ�������ɹ�\r\n");
  
  // �ڶ���������IWDG�Ĵ���
  DEBUG_Printf("����IWDG�Ĵ���...\r\n");
  
  // ����IWDG�Ĵ���д����
  IWDG->KR = 0x5555;
  
  // ����Ԥ��Ƶ����64 (LSI/64)
  IWDG->PR = 0x04;  // IWDG_PRESCALER_64 = 0x04
  
  // ������װ��ֵ��1875 (Լ3�볬ʱ)
  IWDG->RLR = 1875;
  
  // �ȴ��Ĵ���������ɣ���ӳ�ʱ����
  timeout = 100000;
  while((IWDG->SR != 0) && timeout > 0) {
    timeout--;
  }
  
  if(timeout == 0) {
    DEBUG_Printf("IWDG�Ĵ������³�ʱ������������...\r\n");
  }
  
  // ������������IWDG
  DEBUG_Printf("����IWDG���Ź�...\r\n");
  IWDG->KR = 0xCCCC;
  
  DEBUG_Printf("IWDG��ʼ����ɣ���ʱʱ��3��\r\n");
  
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */
}

/**
  * @brief  ˢ��IWDG���Ź�
  * @param  None
  * @retval None
  */
void IWDG_Refresh(void)
{
  IWDG->KR = 0xAAAA;  // ι��
  // �򻯰汾������ӡ̫�������Ϣ����Ӱ������
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */ 


