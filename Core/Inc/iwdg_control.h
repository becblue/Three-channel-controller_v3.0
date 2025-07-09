/**
  ******************************************************************************
  * @file    iwdg_control.h
  * @brief   IWDG看门狗控制模块头文件
  * @author  三通道切换箱控制系统
  * @version V3.0
  * @date    2024
  ******************************************************************************
  * @attention
  * 
  * 本模块实现IWDG独立看门狗功能，为高安全级别的工业控制系统提供可靠的
  * 系统复位保护。主要特性：
  * 1. 3秒超时时间，适合工业控制应用
  * 2. 智能喂狗机制，只有系统正常时才喂狗
  * 3. 与安全监控系统集成，异常时停止喂狗
  * 4. 完整的状态监控和调试接口
  * 5. 复位后的状态恢复和异常记录
  * 
  ******************************************************************************
  */

#ifndef __IWDG_CONTROL_H
#define __IWDG_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  看门狗状态枚举
  */
typedef enum {
    IWDG_STATE_DISABLED = 0,    // 看门狗未启用
    IWDG_STATE_ENABLED,         // 看门狗已启用
    IWDG_STATE_FEEDING,         // 正在喂狗
    IWDG_STATE_SUSPENDED        // 喂狗被暂停（异常状态）
} IwdgState_t;

/**
  * @brief  看门狗复位原因枚举
  */
typedef enum {
    IWDG_RESET_NONE = 0,        // 无复位
    IWDG_RESET_UNKNOWN,         // 未知复位原因
    IWDG_RESET_POWER_ON,        // 上电复位
    IWDG_RESET_SOFTWARE,        // 软件复位
    IWDG_RESET_WATCHDOG,        // 看门狗复位
    IWDG_RESET_WINDOW_WATCHDOG, // 窗口看门狗复位
    IWDG_RESET_LOW_POWER,       // 低功耗复位
    IWDG_RESET_PIN              // 引脚复位
} IwdgResetReason_t;

/**
  * @brief  看门狗运行统计信息
  */
typedef struct {
    uint32_t feed_count;            // 喂狗次数
    uint32_t suspend_count;         // 暂停次数
    uint32_t resume_count;          // 恢复次数
    uint32_t last_feed_time;        // 上次喂狗时间
    uint32_t last_suspend_time;     // 上次暂停时间
    uint32_t total_run_time;        // 总运行时间
    uint32_t total_suspend_time;    // 总暂停时间
    IwdgResetReason_t last_reset_reason;  // 上次复位原因
} IwdgStatistics_t;

/**
  * @brief  看门狗控制结构体
  */
typedef struct {
    IwdgState_t state;              // 当前状态
    uint8_t auto_feed_enabled;      // 自动喂狗使能标志
    uint8_t debug_mode;             // 调试模式标志
    uint32_t feed_interval;         // 喂狗间隔时间（ms）
    uint32_t timeout_value;         // 超时时间（ms）
    uint32_t start_time;            // 启动时间
    IwdgStatistics_t statistics;    // 运行统计信息
} IwdgControl_t;

/* Exported constants --------------------------------------------------------*/

// 看门狗配置参数
#define IWDG_TIMEOUT_MS         3000        // 超时时间：3秒
#define IWDG_FEED_INTERVAL_MS   500         // 喂狗间隔：500ms
#define IWDG_PRESCALER          IWDG_PRESCALER_64  // 预分频器：64
#define IWDG_RELOAD_VALUE       1875        // 重装载值：1875 (3秒超时)

// 看门狗状态标志
#define IWDG_FLAG_ENABLED       0x01        // 看门狗已启用
#define IWDG_FLAG_FEEDING       0x02        // 正在喂狗
#define IWDG_FLAG_SUSPENDED     0x04        // 喂狗被暂停
#define IWDG_FLAG_DEBUG         0x08        // 调试模式

/* Exported macro ------------------------------------------------------------*/

/**
  * @brief  检查看门狗是否处于指定状态
  */
#define IWDG_IS_STATE(state)    (IwdgControl_GetState() == (state))

/**
  * @brief  检查看门狗是否已启用
  */
#define IWDG_IS_ENABLED()       (IwdgControl_GetState() != IWDG_STATE_DISABLED)

/**
  * @brief  检查是否允许自动喂狗
  */
#define IWDG_IS_AUTO_FEED_ENABLED() (IwdgControl_IsAutoFeedEnabled())

/* Exported functions prototypes ---------------------------------------------*/

/* 初始化和配置函数 */
void IwdgControl_Init(void);
void IwdgControl_DeInit(void);
void IwdgControl_Config(uint32_t timeout_ms, uint32_t feed_interval_ms);

/* 控制函数 */
void IwdgControl_Start(void);
void IwdgControl_Stop(void);
void IwdgControl_Feed(void);
void IwdgControl_Suspend(void);
void IwdgControl_Resume(void);

/* 状态查询函数 */
IwdgState_t IwdgControl_GetState(void);
uint8_t IwdgControl_IsAutoFeedEnabled(void);
uint8_t IwdgControl_IsDebugMode(void);
IwdgResetReason_t IwdgControl_GetLastResetReason(void);

/* 统计信息函数 */
IwdgStatistics_t IwdgControl_GetStatistics(void);
void IwdgControl_ResetStatistics(void);
void IwdgControl_PrintStatistics(void);

/* 自动喂狗控制函数 */
void IwdgControl_EnableAutoFeed(void);
void IwdgControl_DisableAutoFeed(void);
void IwdgControl_ProcessAutoFeed(void);

/* 调试和监控函数 */
void IwdgControl_EnableDebugMode(void);
void IwdgControl_DisableDebugMode(void);
void IwdgControl_PrintStatus(void);
void IwdgControl_PrintResetReason(void);

/* 系统集成函数 */
void IwdgControl_SystemStartupCheck(void);
void IwdgControl_SafetyMonitorIntegration(void);
uint8_t IwdgControl_IsSystemSafeToFeed(void);

/* 复位原因管理函数 */
void IwdgControl_SetResetReason(IwdgResetReason_t reason);
void IwdgControl_SetResetCount(uint32_t count);

/* 测试函数 */
void IwdgControl_TestWatchdog(void);
void IwdgControl_TestResetRecovery(void);

#ifdef __cplusplus
}
#endif

#endif /* __IWDG_CONTROL_H */


