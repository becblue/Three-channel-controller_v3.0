/**
  ******************************************************************************
  * @file    iwdg_test.h
  * @brief   IWDG看门狗功能测试验证程序头文件
  * @author  三通道切换箱控制系统
  * @version V3.0
  * @date    2024
  ******************************************************************************
  * @attention
  * 
  * 本测试程序用于验证IWDG看门狗功能的正确性，包括：
  * 1. 看门狗启动和配置测试
  * 2. 自动喂狗机制测试
  * 3. 安全监控集成测试
  * 4. 复位恢复测试
  * 5. 状态监控和统计测试
  * 
  ******************************************************************************
  */

#ifndef __IWDG_TEST_H
#define __IWDG_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  执行完整的IWDG看门狗功能测试套件
  * @retval 1:测试通过 0:测试失败
  */
uint8_t IwdgTest_RunFullTestSuite(void);

/**
  * @brief  执行看门狗复位测试（危险操作）
  * @retval 无（系统将复位）
  */
void IwdgTest_RunResetTest(void);

/**
  * @brief  快速功能验证测试
  * @retval 1:验证通过 0:验证失败
  */
uint8_t IwdgTest_QuickFunctionTest(void);

#ifdef __cplusplus
}
#endif

#endif /* __IWDG_TEST_H */ 

