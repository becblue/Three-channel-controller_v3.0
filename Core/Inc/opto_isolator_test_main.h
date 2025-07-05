#ifndef __OPTO_ISOLATOR_TEST_MAIN_H__
#define __OPTO_ISOLATOR_TEST_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * 光耦隔离电路测试主程序头文件
 * 提供完整的测试流程和用户交互界面
 * 可以通过串口命令控制测试过程
 ************************************************************/

/**
  * @brief  光耦隔离电路测试主程序初始化
  * @retval 无
  */
void OptoTestMain_Init(void);

/**
  * @brief  处理串口接收到的字符
  * @param  ch: 接收到的字符
  * @retval 无
  */
void OptoTestMain_ProcessChar(char ch);

/**
  * @brief  处理用户输入的命令
  * @param  cmd: 命令字符串
  * @retval 无
  */
void OptoTestMain_ProcessCommand(char* cmd);

/**
  * @brief  显示详细帮助信息
  * @retval 无
  */
void OptoTestMain_ShowHelp(void);

/**
  * @brief  自动化测试流程
  * @retval 无
  */
void OptoTestMain_AutoTest(void);

/**
  * @brief  光耦隔离电路测试主循环
  * @retval 无
  */
void OptoTestMain_Process(void);

/**
  * @brief  检查光耦隔离电路的理论设计
  * @retval 无
  */
void OptoTestMain_CheckDesign(void);

#ifdef __cplusplus
}
#endif

#endif /* __OPTO_ISOLATOR_TEST_MAIN_H__ */ 