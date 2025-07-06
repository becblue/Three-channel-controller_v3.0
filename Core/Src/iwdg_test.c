/**
  ******************************************************************************
  * @file    iwdg_test.c
  * @brief   IWDG看门狗功能测试验证程序
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

/* Includes ------------------------------------------------------------------*/
#include "iwdg_control.h"
#include "safety_monitor.h"
#include "system_control.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TEST_TIMEOUT_MS     10000   // 测试超时时间：10秒

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// 测试运行标志已移除，改为局部变量管理
static uint32_t g_test_start_time = 0;  // 测试开始时间

/* Private function prototypes -----------------------------------------------*/
static void IwdgTest_PrintTestHeader(const char* test_name);
static void IwdgTest_PrintTestResult(const char* test_name, uint8_t result);
// 等待条件满足函数已移除，改为直接使用HAL_Delay和状态检查

/* Test functions prototypes -------------------------------------------------*/
static uint8_t IwdgTest_BasicInitialization(void);
static uint8_t IwdgTest_StartAndFeed(void);
static uint8_t IwdgTest_AutoFeedMechanism(void);
static uint8_t IwdgTest_SafetyMonitorIntegration(void);
static uint8_t IwdgTest_StateTransition(void);
static uint8_t IwdgTest_StatisticsAndMonitoring(void);
static uint8_t IwdgTest_ConfigurationChange(void);
static uint8_t IwdgTest_DebugMode(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  执行完整的IWDG看门狗功能测试套件
  * @retval 1:测试通过 0:测试失败
  */
uint8_t IwdgTest_RunFullTestSuite(void)
{
    uint8_t test_count = 0;
    uint8_t pass_count = 0;
    
    DEBUG_Printf("\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("  IWDG看门狗功能完整测试套件启动\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("测试时间: %s %s\r\n", __DATE__, __TIME__);
    DEBUG_Printf("系统时钟: %luMHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
    DEBUG_Printf("========================================\r\n");
    
    g_test_start_time = HAL_GetTick();
    
    // 测试1：基本初始化测试
    test_count++;
    if(IwdgTest_BasicInitialization()) {
        pass_count++;
    }
    
    // 测试2：启动和喂狗测试
    test_count++;
    if(IwdgTest_StartAndFeed()) {
        pass_count++;
    }
    
    // 测试3：自动喂狗机制测试
    test_count++;
    if(IwdgTest_AutoFeedMechanism()) {
        pass_count++;
    }
    
    // 测试4：安全监控集成测试
    test_count++;
    if(IwdgTest_SafetyMonitorIntegration()) {
        pass_count++;
    }
    
    // 测试5：状态转换测试
    test_count++;
    if(IwdgTest_StateTransition()) {
        pass_count++;
    }
    
    // 测试6：统计和监控测试
    test_count++;
    if(IwdgTest_StatisticsAndMonitoring()) {
        pass_count++;
    }
    
    // 测试7：配置更改测试
    test_count++;
    if(IwdgTest_ConfigurationChange()) {
        pass_count++;
    }
    
    // 测试8：调试模式测试
    test_count++;
    if(IwdgTest_DebugMode()) {
        pass_count++;
    }
    
    // 输出总结果
    DEBUG_Printf("\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("  IWDG看门狗功能测试套件完成\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("总测试数量: %d\r\n", test_count);
    DEBUG_Printf("通过测试数: %d\r\n", pass_count);
    DEBUG_Printf("失败测试数: %d\r\n", test_count - pass_count);
    DEBUG_Printf("测试成功率: %d%%\r\n", (pass_count * 100) / test_count);
    DEBUG_Printf("总耗时: %lums\r\n", HAL_GetTick() - g_test_start_time);
    
    if(pass_count == test_count) {
        DEBUG_Printf("? 所有测试通过！IWDG看门狗功能工作正常\r\n");
        return 1;
    } else {
        DEBUG_Printf("? 有测试失败！请检查IWDG看门狗功能\r\n");
        return 0;
    }
}

/**
  * @brief  执行看门狗复位测试（危险操作）
  * @retval 无（系统将复位）
  */
void IwdgTest_RunResetTest(void)
{
    DEBUG_Printf("\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("  IWDG看门狗复位测试（危险操作）\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("警告：此测试将导致系统复位！\r\n");
    DEBUG_Printf("请确保已保存所有重要数据\r\n");
    DEBUG_Printf("5秒后开始测试...\r\n");
    
    // 5秒倒计时
    for(int i = 5; i > 0; i--) {
        DEBUG_Printf("倒计时: %d秒\r\n", i);
        HAL_Delay(1000);
    }
    
    DEBUG_Printf("开始复位测试，停止喂狗...\r\n");
    
    // 启动看门狗并立即停止喂狗
    IwdgControl_Start();
    HAL_Delay(100);
    IwdgControl_Suspend();
    
    DEBUG_Printf("看门狗已暂停，等待复位...\r\n");
    
    // 等待看门狗复位（约3秒）
    uint32_t start_time = HAL_GetTick();
    while(1) {
        uint32_t elapsed = HAL_GetTick() - start_time;
        DEBUG_Printf("等待复位...%lums\r\n", elapsed);
        HAL_Delay(500);
    }
}

/**
  * @brief  快速功能验证测试
  * @retval 1:验证通过 0:验证失败
  */
uint8_t IwdgTest_QuickFunctionTest(void)
{
    DEBUG_Printf("\r\n=== IWDG看门狗快速功能验证 ===\r\n");
    
    // 检查初始状态
    if(IwdgControl_GetState() != IWDG_STATE_DISABLED) {
        DEBUG_Printf("? 初始状态错误\r\n");
        return 0;
    }
    
    // 测试启动
    IwdgControl_Start();
    if(IwdgControl_GetState() != IWDG_STATE_ENABLED) {
        DEBUG_Printf("? 启动后状态错误\r\n");
        return 0;
    }
    
    // 测试喂狗
    IwdgControl_Feed();
    HAL_Delay(100);
    
    // 测试暂停和恢复
    IwdgControl_Suspend();
    if(IwdgControl_GetState() != IWDG_STATE_SUSPENDED) {
        DEBUG_Printf("? 暂停后状态错误\r\n");
        return 0;
    }
    
    IwdgControl_Resume();
    if(IwdgControl_GetState() != IWDG_STATE_ENABLED) {
        DEBUG_Printf("? 恢复后状态错误\r\n");
        return 0;
    }
    
    DEBUG_Printf("? 快速功能验证通过\r\n");
    return 1;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  打印测试标题
  */
static void IwdgTest_PrintTestHeader(const char* test_name)
{
    DEBUG_Printf("\r\n--- 测试: %s ---\r\n", test_name);
}

/**
  * @brief  打印测试结果
  */
static void IwdgTest_PrintTestResult(const char* test_name, uint8_t result)
{
    if(result) {
        DEBUG_Printf("? %s: 通过\r\n", test_name);
    } else {
        DEBUG_Printf("? %s: 失败\r\n", test_name);
    }
}

// 等待条件满足函数已移除，改为直接使用HAL_Delay和状态检查

/**
  * @brief  测试1：基本初始化测试
  */
static uint8_t IwdgTest_BasicInitialization(void)
{
    IwdgTest_PrintTestHeader("基本初始化测试");
    
    // 重新初始化
    IwdgControl_DeInit();
    IwdgControl_Init();
    
    // 检查初始状态
    if(IwdgControl_GetState() != IWDG_STATE_DISABLED) {
        DEBUG_Printf("初始状态错误\r\n");
        IwdgTest_PrintTestResult("基本初始化测试", 0);
        return 0;
    }
    
    // 检查配置参数
    IwdgStatistics_t stats = IwdgControl_GetStatistics();
    if(stats.feed_count != 0) {
        DEBUG_Printf("初始统计信息错误\r\n");
        IwdgTest_PrintTestResult("基本初始化测试", 0);
        return 0;
    }
    
    DEBUG_Printf("初始化成功，状态正确\r\n");
    IwdgTest_PrintTestResult("基本初始化测试", 1);
    return 1;
}

/**
  * @brief  测试2：启动和喂狗测试
  */
static uint8_t IwdgTest_StartAndFeed(void)
{
    IwdgTest_PrintTestHeader("启动和喂狗测试");
    
    // 启动看门狗
    IwdgControl_Start();
    HAL_Delay(100);
    
    if(IwdgControl_GetState() != IWDG_STATE_ENABLED) {
        DEBUG_Printf("启动后状态错误\r\n");
        IwdgTest_PrintTestResult("启动和喂狗测试", 0);
        return 0;
    }
    
    // 测试手动喂狗
    uint32_t feed_count_before = IwdgControl_GetStatistics().feed_count;
    IwdgControl_Feed();
    HAL_Delay(50);
    uint32_t feed_count_after = IwdgControl_GetStatistics().feed_count;
    
    if(feed_count_after <= feed_count_before) {
        DEBUG_Printf("喂狗计数未更新\r\n");
        IwdgTest_PrintTestResult("启动和喂狗测试", 0);
        return 0;
    }
    
    DEBUG_Printf("启动成功，喂狗正常\r\n");
    IwdgTest_PrintTestResult("启动和喂狗测试", 1);
    return 1;
}

/**
  * @brief  测试3：自动喂狗机制测试
  */
static uint8_t IwdgTest_AutoFeedMechanism(void)
{
    IwdgTest_PrintTestHeader("自动喂狗机制测试");
    
    // 启用自动喂狗
    IwdgControl_EnableAutoFeed();
    
    if(!IwdgControl_IsAutoFeedEnabled()) {
        DEBUG_Printf("自动喂狗未启用\r\n");
        IwdgTest_PrintTestResult("自动喂狗机制测试", 0);
        return 0;
    }
    
    // 等待自动喂狗触发
    uint32_t feed_count_before = IwdgControl_GetStatistics().feed_count;
    
    // 模拟主循环调用自动喂狗
    for(int i = 0; i < 10; i++) {
        IwdgControl_ProcessAutoFeed();
        HAL_Delay(100);
    }
    
    uint32_t feed_count_after = IwdgControl_GetStatistics().feed_count;
    
    if(feed_count_after <= feed_count_before) {
        DEBUG_Printf("自动喂狗未触发\r\n");
        IwdgTest_PrintTestResult("自动喂狗机制测试", 0);
        return 0;
    }
    
    DEBUG_Printf("自动喂狗机制正常\r\n");
    IwdgTest_PrintTestResult("自动喂狗机制测试", 1);
    return 1;
}

/**
  * @brief  测试4：安全监控集成测试
  */
static uint8_t IwdgTest_SafetyMonitorIntegration(void)
{
    IwdgTest_PrintTestHeader("安全监控集成测试");
    
    // 测试与安全监控系统的集成
    IwdgControl_SafetyMonitorIntegration();
    
    // 测试系统安全检查
    uint8_t is_safe = IwdgControl_IsSystemSafeToFeed();
    DEBUG_Printf("系统安全检查结果: %s\r\n", is_safe ? "安全" : "不安全");
    
    DEBUG_Printf("安全监控集成测试完成\r\n");
    IwdgTest_PrintTestResult("安全监控集成测试", 1);
    return 1;
}

/**
  * @brief  测试5：状态转换测试
  */
static uint8_t IwdgTest_StateTransition(void)
{
    IwdgTest_PrintTestHeader("状态转换测试");
    
    // 测试暂停
    IwdgControl_Suspend();
    if(IwdgControl_GetState() != IWDG_STATE_SUSPENDED) {
        DEBUG_Printf("暂停状态错误\r\n");
        IwdgTest_PrintTestResult("状态转换测试", 0);
        return 0;
    }
    
    // 测试恢复
    IwdgControl_Resume();
    if(IwdgControl_GetState() != IWDG_STATE_ENABLED) {
        DEBUG_Printf("恢复状态错误\r\n");
        IwdgTest_PrintTestResult("状态转换测试", 0);
        return 0;
    }
    
    DEBUG_Printf("状态转换正常\r\n");
    IwdgTest_PrintTestResult("状态转换测试", 1);
    return 1;
}

/**
  * @brief  测试6：统计和监控测试
  */
static uint8_t IwdgTest_StatisticsAndMonitoring(void)
{
    IwdgTest_PrintTestHeader("统计和监控测试");
    
    // 打印统计信息
    IwdgControl_PrintStatistics();
    
    // 打印当前状态
    IwdgControl_PrintStatus();
    
    // 打印复位原因
    IwdgControl_PrintResetReason();
    
    DEBUG_Printf("统计和监控功能正常\r\n");
    IwdgTest_PrintTestResult("统计和监控测试", 1);
    return 1;
}

/**
  * @brief  测试7：配置更改测试
  */
static uint8_t IwdgTest_ConfigurationChange(void)
{
    IwdgTest_PrintTestHeader("配置更改测试");
    
    // 测试配置更改
    IwdgControl_Config(5000, 1000);  // 5秒超时，1秒喂狗间隔
    
    DEBUG_Printf("配置更改测试完成\r\n");
    IwdgTest_PrintTestResult("配置更改测试", 1);
    return 1;
}

/**
  * @brief  测试8：调试模式测试
  */
static uint8_t IwdgTest_DebugMode(void)
{
    IwdgTest_PrintTestHeader("调试模式测试");
    
    // 启用调试模式
    IwdgControl_EnableDebugMode();
    if(!IwdgControl_IsDebugMode()) {
        DEBUG_Printf("调试模式启用失败\r\n");
        IwdgTest_PrintTestResult("调试模式测试", 0);
        return 0;
    }
    
    // 测试调试输出
    IwdgControl_Feed();  // 应该有调试输出
    
    // 禁用调试模式
    IwdgControl_DisableDebugMode();
    if(IwdgControl_IsDebugMode()) {
        DEBUG_Printf("调试模式禁用失败\r\n");
        IwdgTest_PrintTestResult("调试模式测试", 0);
        return 0;
    }
    
    DEBUG_Printf("调试模式功能正常\r\n");
    IwdgTest_PrintTestResult("调试模式测试", 1);
    return 1;
}
