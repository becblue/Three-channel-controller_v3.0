/**
  ******************************************************************************
  * @file    smart_delay.h
  * @brief   智能延时函数头文件
  *          在延时过程中自动喂狗，彻底解决看门狗复位问题
  ******************************************************************************
  * @attention
  *
  * 智能延时系统特点：
  * 1. 延时过程中定期喂狗（默认每100ms一次）
  * 2. 保持原有延时精度
  * 3. 支持不同系统状态下的智能行为
  * 4. 完全替代HAL_Delay()，无缝兼容
  *
  ******************************************************************************
  */

#ifndef __SMART_DELAY_H__
#define __SMART_DELAY_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "system_control.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define SMART_DELAY_FEED_INTERVAL_MS    100     // 喂狗间隔：100ms
#define SMART_DELAY_MIN_FEED_TIME       50      // 最小喂狗时间间隔

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes --------------------------------------------*/

/**
  * @brief  智能延时函数（推荐使用）
  * @param  delay_ms: 延时时间（毫秒）
  * @retval 无
  * @note   在延时过程中会根据系统状态自动喂狗
  */
void SmartDelay(uint32_t delay_ms);

/**
  * @brief  带调试信息的智能延时函数
  * @param  delay_ms: 延时时间（毫秒）
  * @param  context: 延时上下文描述（用于调试）
  * @retval 无
  * @note   在延时过程中会输出调试信息并自动喂狗
  */
void SmartDelayWithDebug(uint32_t delay_ms, const char* context);

/**
  * @brief  强制喂狗延时函数（特殊场景使用）
  * @param  delay_ms: 延时时间（毫秒）
  * @retval 无
  * @note   无论系统状态如何都会喂狗，用于关键初始化阶段
  */
void SmartDelayWithForceFeed(uint32_t delay_ms);

/**
  * @brief  简单延时函数（不喂狗）
  * @param  delay_ms: 延时时间（毫秒）
  * @retval 无
  * @note   完全等同于HAL_Delay，用于不需要喂狗的场景
  */
void SimpleDelay(uint32_t delay_ms);

/**
  * @brief  检查是否需要喂狗
  * @retval 1:需要喂狗 0:不需要喂狗
  * @note   根据当前系统状态判断是否需要喂狗
  */
uint8_t SmartDelay_ShouldFeedWatchdog(void);

/**
  * @brief  执行智能喂狗操作
  * @retval 无
  * @note   根据系统状态智能决定是否喂狗
  */
void SmartDelay_FeedWatchdog(void);

#ifdef __cplusplus
}
#endif

#endif /* __SMART_DELAY_H__ */ 

