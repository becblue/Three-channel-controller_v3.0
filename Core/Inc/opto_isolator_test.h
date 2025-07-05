#ifndef __OPTO_ISOLATOR_TEST_H__
#define __OPTO_ISOLATOR_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * 光耦隔离电路测试模块头文件
 * 专门用于测试和诊断K1_EN、K2_EN、K3_EN信号的光耦隔离电路工作状态
 * 提供完整的测试、监控、诊断和优化建议功能
 ************************************************************/

/**
  * @brief  初始化光耦隔离电路测试模块
  * @retval 无
  */
void OptoTest_Init(void);

/**
  * @brief  开始光耦隔离电路测试
  * @param  test_duration_sec: 测试持续时间(秒)
  * @retval 无
  */
void OptoTest_Start(uint16_t test_duration_sec);

/**
  * @brief  停止光耦隔离电路测试
  * @retval 无
  */
void OptoTest_Stop(void);

/**
  * @brief  光耦隔离电路测试主循环（在主循环中调用）
  * @retval 无
  */
void OptoTest_Process(void);

/**
  * @brief  输出实时状态信息
  * @retval 无
  */
void OptoTest_PrintRealTimeStatus(void);

/**
  * @brief  输出详细测试结果
  * @retval 无
  */
void OptoTest_PrintResults(void);

/**
  * @brief  手动触发单次状态检测
  * @retval 无
  */
void OptoTest_ManualCheck(void);

/**
  * @brief  获取当前测试状态
  * @retval 1:测试运行中 0:测试停止
  */
uint8_t OptoTest_IsRunning(void);

/**
  * @brief  获取测试经过时间
  * @retval 测试经过时间(毫秒)
  */
uint32_t OptoTest_GetElapsedTime(void);

#ifdef __cplusplus
}
#endif

#endif /* __OPTO_ISOLATOR_TEST_H__ */ 