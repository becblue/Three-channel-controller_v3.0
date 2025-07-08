/**
  ******************************************************************************
  * @file    iwdg_control.c
  * @brief   IWDG看门狗控制模块源文件
  * @author  三通道切换箱控制系统
  * @version V3.0
  * @date    2024
  ******************************************************************************
  * @attention
  * 
  * 本模块实现IWDG独立看门狗功能，为高安全级别的工业控制系统提供可靠的
  * 系统复位保护。具有以下特性：
  * 1. 3秒超时时间，适合工业控制应用
  * 2. 智能喂狗机制，只有系统正常时才喂狗
  * 3. 与安全监控系统集成，异常时停止喂狗
  * 4. 完整的状态监控和调试接口
  * 5. 复位后的状态恢复和异常记录
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "iwdg_control.h"
#include "iwdg.h"
#include "safety_monitor.h"
#include "system_control.h"
#include "log_system.h"
#include "usart.h"
#include "smart_delay.h"  // 智能延时函数
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define IWDG_MAGIC_NUMBER       0x5A5A5A5A  // 看门狗启动标志
#define IWDG_RESET_COUNT_ADDR   0x20000000  // 复位计数存储地址（假设SRAM起始地址）

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static IwdgControl_t g_iwdg_control;           // 看门狗控制结构体
static uint32_t g_iwdg_last_feed_time = 0;    // 上次喂狗时间
static uint32_t g_iwdg_reset_count = 0;       // 复位计数

/* Private function prototypes -----------------------------------------------*/
static void IwdgControl_InitStructure(void);
static void IwdgControl_AnalyzeResetReason(void);
// 统计信息更新函数已移除，统计信息在实际操作中直接更新
static uint8_t IwdgControl_CheckSafetyConditions(void);
static void IwdgControl_HandleResetRecovery(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  看门狗控制模块初始化
  * @retval 无
  */
void IwdgControl_Init(void)
{
    // 初始化控制结构体
    IwdgControl_InitStructure();
    
    // 分析复位原因
    IwdgControl_AnalyzeResetReason();
    
    // 系统启动检查
    IwdgControl_SystemStartupCheck();
    
    // 处理复位恢复
    IwdgControl_HandleResetRecovery();
    
    DEBUG_Printf("=== IWDG看门狗控制模块初始化完成 ===\r\n");
    DEBUG_Printf("超时时间: %lums, 喂狗间隔: %lums\r\n", 
                g_iwdg_control.timeout_value, g_iwdg_control.feed_interval);
    
    // 打印复位原因
    IwdgControl_PrintResetReason();
}

/**
  * @brief  看门狗控制模块反初始化
  * @retval 无
  */
void IwdgControl_DeInit(void)
{
    // 停止看门狗
    IwdgControl_Stop();
    
    // 清除控制结构体
    memset(&g_iwdg_control, 0, sizeof(g_iwdg_control));
    
    DEBUG_Printf("IWDG看门狗控制模块反初始化完成\r\n");
}

/**
  * @brief  配置看门狗参数
  * @param  timeout_ms: 超时时间（毫秒）
  * @param  feed_interval_ms: 喂狗间隔（毫秒）
  * @retval 无
  */
void IwdgControl_Config(uint32_t timeout_ms, uint32_t feed_interval_ms)
{
    g_iwdg_control.timeout_value = timeout_ms;
    g_iwdg_control.feed_interval = feed_interval_ms;
    
    DEBUG_Printf("IWDG配置更新: 超时=%lums, 喂狗间隔=%lums\r\n", 
                timeout_ms, feed_interval_ms);
}

/**
  * @brief  启动看门狗
  * @retval 无
  */
void IwdgControl_Start(void)
{
    if(g_iwdg_control.state == IWDG_STATE_DISABLED) {
        // 初始化IWDG外设
        MX_IWDG_Init();
        
        // 更新状态
        g_iwdg_control.state = IWDG_STATE_ENABLED;
        g_iwdg_control.start_time = HAL_GetTick();
        g_iwdg_control.auto_feed_enabled = 1;
        
        // 立即喂狗一次
        IwdgControl_Feed();
        
        DEBUG_Printf("=== IWDG看门狗启动成功 ===\r\n");
        DEBUG_Printf("看门狗已启动，状态: ENABLED\r\n");
    } else {
        DEBUG_Printf("IWDG看门狗已经启动，当前状态: %d\r\n", g_iwdg_control.state);
    }
}

/**
  * @brief  停止看门狗（注意：IWDG一旦启动无法停止）
  * @retval 无
  */
void IwdgControl_Stop(void)
{
    DEBUG_Printf("警告：IWDG看门狗一旦启动无法停止！\r\n");
    DEBUG_Printf("只能停止自动喂狗，看门狗硬件仍在运行\r\n");
    
    // 停止自动喂狗
    g_iwdg_control.auto_feed_enabled = 0;
    g_iwdg_control.state = IWDG_STATE_SUSPENDED;
    
    DEBUG_Printf("自动喂狗已停止，系统将在%lums后复位\r\n", g_iwdg_control.timeout_value);
}

/**
  * @brief  手动喂狗
  * @retval 无
  */
void IwdgControl_Feed(void)
{
    // 检查安全条件
    if(!IwdgControl_CheckSafetyConditions()) {
        DEBUG_Printf("安全条件不满足，拒绝喂狗！\r\n");
        return;
    }
    
    // 执行喂狗操作
    IWDG_Refresh();
    
    // 更新状态
    g_iwdg_control.state = IWDG_STATE_FEEDING;
    g_iwdg_control.statistics.feed_count++;
    g_iwdg_control.statistics.last_feed_time = HAL_GetTick();
    g_iwdg_last_feed_time = HAL_GetTick();
    
    if(g_iwdg_control.debug_mode) {
        DEBUG_Printf("[IWDG] 喂狗成功，计数: %lu\r\n", g_iwdg_control.statistics.feed_count);
    }
    
    // 短暂延时后恢复到已启用状态
    HAL_Delay(1);
    g_iwdg_control.state = IWDG_STATE_ENABLED;
}

/**
  * @brief  暂停自动喂狗
  * @retval 无
  */
void IwdgControl_Suspend(void)
{
    g_iwdg_control.auto_feed_enabled = 0;
    g_iwdg_control.state = IWDG_STATE_SUSPENDED;
    g_iwdg_control.statistics.suspend_count++;
    g_iwdg_control.statistics.last_suspend_time = HAL_GetTick();
    
    DEBUG_Printf("=== IWDG自动喂狗已暂停 ===\r\n");
    DEBUG_Printf("暂停计数: %lu, 系统将在%lums后复位\r\n", 
                g_iwdg_control.statistics.suspend_count, g_iwdg_control.timeout_value);
    
    // 记录系统保护事件到日志系统
    if(LogSystem_IsInitialized()) {
        char msg[48];
        snprintf(msg, sizeof(msg), "看门狗暂停，系统保护启动，暂停次数:%lu", g_iwdg_control.statistics.suspend_count);
        LOG_SYSTEM_LOCK(msg);
    }
}

/**
  * @brief  恢复自动喂狗
  * @retval 无
  */
void IwdgControl_Resume(void)
{
    g_iwdg_control.auto_feed_enabled = 1;
    g_iwdg_control.state = IWDG_STATE_ENABLED;
    g_iwdg_control.statistics.resume_count++;
    
    // 立即喂狗一次
    IwdgControl_Feed();
    
    DEBUG_Printf("=== IWDG自动喂狗已恢复 ===\r\n");
    DEBUG_Printf("恢复计数: %lu\r\n", g_iwdg_control.statistics.resume_count);
}

/**
  * @brief  获取看门狗当前状态
  * @retval 看门狗状态
  */
IwdgState_t IwdgControl_GetState(void)
{
    return g_iwdg_control.state;
}

/**
  * @brief  检查是否启用了自动喂狗
  * @retval 1:启用 0:禁用
  */
uint8_t IwdgControl_IsAutoFeedEnabled(void)
{
    return g_iwdg_control.auto_feed_enabled;
}

/**
  * @brief  检查是否为调试模式
  * @retval 1:调试模式 0:正常模式
  */
uint8_t IwdgControl_IsDebugMode(void)
{
    return g_iwdg_control.debug_mode;
}

/**
  * @brief  获取上次复位原因
  * @retval 复位原因
  */
IwdgResetReason_t IwdgControl_GetLastResetReason(void)
{
    return g_iwdg_control.statistics.last_reset_reason;
}

/**
  * @brief  获取统计信息
  * @retval 统计信息结构体
  */
IwdgStatistics_t IwdgControl_GetStatistics(void)
{
    // 更新运行时间
    if(g_iwdg_control.start_time > 0) {
        g_iwdg_control.statistics.total_run_time = 
            HAL_GetTick() - g_iwdg_control.start_time;
    }
    
    return g_iwdg_control.statistics;
}

/**
  * @brief  重置统计信息
  * @retval 无
  */
void IwdgControl_ResetStatistics(void)
{
    memset(&g_iwdg_control.statistics, 0, sizeof(IwdgStatistics_t));
    g_iwdg_control.start_time = HAL_GetTick();
    
    DEBUG_Printf("IWDG统计信息已重置\r\n");
}

/**
  * @brief  打印统计信息
  * @retval 无
  */
void IwdgControl_PrintStatistics(void)
{
    IwdgStatistics_t stats = IwdgControl_GetStatistics();
    
    DEBUG_Printf("=== IWDG看门狗统计信息 ===\r\n");
    DEBUG_Printf("喂狗次数: %lu\r\n", stats.feed_count);
    DEBUG_Printf("暂停次数: %lu\r\n", stats.suspend_count);
    DEBUG_Printf("恢复次数: %lu\r\n", stats.resume_count);
    DEBUG_Printf("总运行时间: %lums\r\n", stats.total_run_time);
    DEBUG_Printf("上次喂狗时间: %lums\r\n", stats.last_feed_time);
    DEBUG_Printf("上次暂停时间: %lums\r\n", stats.last_suspend_time);
    DEBUG_Printf("上次复位原因: %d\r\n", stats.last_reset_reason);
}

/**
  * @brief  启用自动喂狗
  * @retval 无
  */
void IwdgControl_EnableAutoFeed(void)
{
    g_iwdg_control.auto_feed_enabled = 1;
    DEBUG_Printf("IWDG自动喂狗已启用\r\n");
}

/**
  * @brief  禁用自动喂狗
  * @retval 无
  */
void IwdgControl_DisableAutoFeed(void)
{
    g_iwdg_control.auto_feed_enabled = 0;
    DEBUG_Printf("IWDG自动喂狗已禁用\r\n");
}

/**
  * @brief  处理自动喂狗（在主循环中调用）
  * @retval 无
  */
void IwdgControl_ProcessAutoFeed(void)
{
    // 检查是否启用自动喂狗
    if(!g_iwdg_control.auto_feed_enabled) {
        return;
    }
    
    // 检查是否到达喂狗间隔时间
    uint32_t current_time = HAL_GetTick();
    if(current_time - g_iwdg_last_feed_time >= g_iwdg_control.feed_interval) {
        // 检查系统是否安全
        if(IwdgControl_IsSystemSafeToFeed()) {
            IwdgControl_Feed();
        } else {
            // 系统不安全，暂停喂狗
            if(g_iwdg_control.state != IWDG_STATE_SUSPENDED) {
                DEBUG_Printf("系统异常，暂停IWDG喂狗\r\n");
                IwdgControl_Suspend();
            }
        }
    }
}

/**
  * @brief  启用调试模式
  * @retval 无
  */
void IwdgControl_EnableDebugMode(void)
{
    g_iwdg_control.debug_mode = 1;
    DEBUG_Printf("IWDG调试模式已启用\r\n");
}

/**
  * @brief  禁用调试模式
  * @retval 无
  */
void IwdgControl_DisableDebugMode(void)
{
    g_iwdg_control.debug_mode = 0;
    DEBUG_Printf("IWDG调试模式已禁用\r\n");
}

/**
  * @brief  打印当前状态
  * @retval 无
  */
void IwdgControl_PrintStatus(void)
{
    const char* state_str[] = {"DISABLED", "ENABLED", "FEEDING", "SUSPENDED"};
    
    DEBUG_Printf("=== IWDG看门狗当前状态 ===\r\n");
    DEBUG_Printf("状态: %s\r\n", state_str[g_iwdg_control.state]);
    DEBUG_Printf("自动喂狗: %s\r\n", g_iwdg_control.auto_feed_enabled ? "启用" : "禁用");
    DEBUG_Printf("调试模式: %s\r\n", g_iwdg_control.debug_mode ? "启用" : "禁用");
    DEBUG_Printf("超时时间: %lums\r\n", g_iwdg_control.timeout_value);
    DEBUG_Printf("喂狗间隔: %lums\r\n", g_iwdg_control.feed_interval);
    DEBUG_Printf("运行时间: %lums\r\n", HAL_GetTick() - g_iwdg_control.start_time);
    DEBUG_Printf("距离上次喂狗: %lums\r\n", HAL_GetTick() - g_iwdg_last_feed_time);
}

/**
  * @brief  打印复位原因
  * @retval 无
  */
void IwdgControl_PrintResetReason(void)
{
    const char* reason_str[] = {
        "无复位", "未知复位", "上电复位", "软件复位", 
        "看门狗复位", "窗口看门狗复位", "低功耗复位", "引脚复位"
    };
    
    DEBUG_Printf("=== 系统复位原因分析 ===\r\n");
    DEBUG_Printf("上次复位原因: %s\r\n", reason_str[g_iwdg_control.statistics.last_reset_reason]);
    DEBUG_Printf("历史复位次数: %lu\r\n", g_iwdg_reset_count);
}

/**
  * @brief  系统启动检查
  * @retval 无
  */
void IwdgControl_SystemStartupCheck(void)
{
    DEBUG_Printf("=== IWDG系统启动检查 ===\r\n");
    
    // 检查是否为看门狗复位
    if(g_iwdg_control.statistics.last_reset_reason == IWDG_RESET_WATCHDOG) {
        DEBUG_Printf("检测到看门狗复位，系统正在恢复...\r\n");
        g_iwdg_reset_count++;
        
        // 如果频繁复位，可能需要特殊处理
        if(g_iwdg_reset_count > 3) {
            DEBUG_Printf("警告：频繁看门狗复位，复位次数: %lu\r\n", g_iwdg_reset_count);
        }
    }
    
    DEBUG_Printf("系统启动检查完成\r\n");
}

/**
  * @brief  与安全监控系统集成
  * @retval 无
  */
void IwdgControl_SafetyMonitorIntegration(void)
{
    // 检查安全监控系统状态
    uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
    
    if(alarm_flags != 0) {
        // 有报警存在，暂停喂狗
        if(g_iwdg_control.auto_feed_enabled) {
            DEBUG_Printf("检测到安全报警(0x%04X)，暂停IWDG喂狗\r\n", alarm_flags);
            IwdgControl_Suspend();
        }
    } else {
        // 无报警，恢复喂狗
        if(!g_iwdg_control.auto_feed_enabled && g_iwdg_control.state == IWDG_STATE_SUSPENDED) {
            DEBUG_Printf("安全报警已清除，恢复IWDG喂狗\r\n");
            IwdgControl_Resume();
        }
    }
}

/**
  * @brief  检查系统是否安全可以喂狗
  * @retval 1:安全 0:不安全
  */
uint8_t IwdgControl_IsSystemSafeToFeed(void)
{
    // 检查系统状态
    SystemState_t system_state = SystemControl_GetState();
    
    // 在自检阶段暂停喂狗
    if(system_state == SYSTEM_STATE_SELF_TEST || 
       system_state == SYSTEM_STATE_LOGO_DISPLAY ||
       system_state == SYSTEM_STATE_ERROR) {
        return 0;
    }
    
    // 检查安全监控系统
    uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
    
    // 如果有严重报警（K~M类温度异常），停止喂狗
    for(int i = 10; i <= 12; i++) {  // K~M类异常（索引10-12）
        if(alarm_flags & (1 << i)) {
            return 0;  // 温度异常，停止喂狗
        }
    }
    
    // 其他异常情况下继续喂狗，防止系统无谓复位
    return 1;
}

/**
  * @brief  测试看门狗功能
  * @retval 无
  */
void IwdgControl_TestWatchdog(void)
{
    DEBUG_Printf("=== IWDG看门狗测试开始 ===\r\n");
    DEBUG_Printf("警告：测试将停止喂狗，系统将在%lums后复位\r\n", g_iwdg_control.timeout_value);
    
    // 停止喂狗
    IwdgControl_Suspend();
    
    // 等待复位
    DEBUG_Printf("等待看门狗复位...\r\n");
    while(1) {
        HAL_Delay(100);
        DEBUG_Printf(".");
    }
}

/**
  * @brief  测试复位恢复功能
  * @retval 无
  */
void IwdgControl_TestResetRecovery(void)
{
    DEBUG_Printf("=== IWDG复位恢复测试 ===\r\n");
    IwdgControl_PrintResetReason();
    IwdgControl_PrintStatistics();
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  初始化控制结构体
  * @retval 无
  */
static void IwdgControl_InitStructure(void)
{
    memset(&g_iwdg_control, 0, sizeof(g_iwdg_control));
    
    // 设置默认参数
    g_iwdg_control.state = IWDG_STATE_DISABLED;
    g_iwdg_control.auto_feed_enabled = 0;
    g_iwdg_control.debug_mode = 0;
    g_iwdg_control.timeout_value = IWDG_TIMEOUT_MS;
    g_iwdg_control.feed_interval = IWDG_FEED_INTERVAL_MS;
    g_iwdg_control.start_time = 0;
    
    // 初始化时间
    g_iwdg_last_feed_time = HAL_GetTick();
}

/**
  * @brief  分析复位原因
  * @retval 无
  */
static void IwdgControl_AnalyzeResetReason(void)
{
    // 检查复位标志
    if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_WATCHDOG;
        __HAL_RCC_CLEAR_RESET_FLAGS();
        // 记录看门狗复位到日志系统
        if(LogSystem_IsInitialized()) {
            LOG_WATCHDOG_RESET();
        }
    } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_WINDOW_WATCHDOG;
        __HAL_RCC_CLEAR_RESET_FLAGS();
        // 记录窗口看门狗复位到日志系统
        if(LogSystem_IsInitialized()) {
            LOG_WATCHDOG_RESET();
        }
    } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_POWER_ON;
        __HAL_RCC_CLEAR_RESET_FLAGS();
    } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_SOFTWARE;
        __HAL_RCC_CLEAR_RESET_FLAGS();
    } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_PIN;
        __HAL_RCC_CLEAR_RESET_FLAGS();
    } else {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_UNKNOWN;
    }
}

// 统计信息更新函数已移除，统计信息在实际操作中直接更新

/**
  * @brief  检查安全条件
  * @retval 1:安全 0:不安全
  */
static uint8_t IwdgControl_CheckSafetyConditions(void)
{
    // 基本安全检查
    if(g_iwdg_control.state == IWDG_STATE_DISABLED) {
        return 0;
    }
    
    // 检查是否在安全的系统状态下
    return IwdgControl_IsSystemSafeToFeed();
}

/**
  * @brief  处理复位恢复
  * @retval 无
  */
static void IwdgControl_HandleResetRecovery(void)
{
    // 如果是看门狗复位，进行特殊处理
    if(g_iwdg_control.statistics.last_reset_reason == IWDG_RESET_WATCHDOG) {
        DEBUG_Printf("看门狗复位恢复处理中...\r\n");
        
        // 延迟一段时间再启动看门狗，给系统稳定时间
        SmartDelayWithForceFeed(1000);
        
        DEBUG_Printf("看门狗复位恢复完成\r\n");
    }
} 


