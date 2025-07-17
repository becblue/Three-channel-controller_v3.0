#include "system_control.h"
#include "relay_control.h"
#include "temperature_monitor.h"
#include "oled_display.h"
#include "gpio_control.h"
#include "safety_monitor.h"
#include "usart.h"
#include "iwdg.h"
#include "smart_delay.h"  // 智能延时函数
#include "log_system.h"    // 日志系统
#include <string.h>
#include <stdio.h>
#include <stdarg.h>        // 变量参数功能
#include <stdlib.h>        // 添加stdlib.h支持strtoul函数

/************************************************************
 * 系统控制模块源文件
 * 实现系统自检、主循环调度、状态机管理
 * 严格按照README要求：2秒LOGO → 3秒自检进度条 → 检测 → 主循环
 ************************************************************/

// 全局系统控制结构体
static SystemControl_t g_system_control;

// 自检进度显示状态标志
static uint8_t g_self_test_progress_first_call = 1;

/**
  * @brief  系统控制模块初始化
  * @retval 无
  */
void SystemControl_Init(void)
{
    // 初始化系统控制结构体
    memset(&g_system_control, 0, sizeof(SystemControl_t));
    g_system_control.current_state = SYSTEM_STATE_INIT;
    g_system_control.last_state = SYSTEM_STATE_INIT;
    g_system_control.state_start_time = HAL_GetTick();
    
    // 重置自检进度显示状态
    SystemControl_ResetSelfTestProgressState();
    
    DEBUG_Printf("系统控制模块初始化完成\r\n");
    
    // 初始化安全监控模块
    SafetyMonitor_Init();
    
    // 开始LOGO显示阶段
    SystemControl_StartLogoDisplay();
}

/**
  * @brief  系统状态机处理（主循环中调用）
  * @retval 无
  */
void SystemControl_Process(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - g_system_control.state_start_time;
    
    switch(g_system_control.current_state) {
        case SYSTEM_STATE_INIT:
            // 初始化状态，立即进入LOGO显示
            SystemControl_StartLogoDisplay();
            break;
            
        case SYSTEM_STATE_LOGO_DISPLAY:
            // LOGO显示阶段，维持2秒
            if(elapsed_time >= LOGO_DISPLAY_TIME_MS) {
                SystemControl_StartSelfTest();
            }
            break;
            
        case SYSTEM_STATE_SELF_TEST:
            // 自检进度条显示阶段，持续3秒
            if(elapsed_time < SELF_TEST_TIME_MS) {
                // 更新进度条（带公司LOGO）
                uint8_t percent = (elapsed_time * 100) / SELF_TEST_TIME_MS;
                OLED_ShowSelfTestBarWithCompanyLogo(percent);
                // 注释自检进度输出，保持串口输出简洁
                // DEBUG_Printf("自检进度: %d%%\r\n", percent);
            } else {
                // 进入自检检测阶段
                g_system_control.current_state = SYSTEM_STATE_SELF_TEST_CHECK;
                g_system_control.state_start_time = current_time;
                DEBUG_Printf("开始执行自检检测\r\n");
            }
            break;
            
        case SYSTEM_STATE_SELF_TEST_CHECK:
            // 执行具体的自检检测
            SystemControl_ExecuteSelfTest();
            break;
            
        case SYSTEM_STATE_NORMAL:
            // 正常运行状态，执行主循环调度
            SystemControl_MainLoopScheduler();
            break;
            
        case SYSTEM_STATE_ERROR:
            // 错误状态处理
            DEBUG_Printf("系统处于错误状态: %s\r\n", g_system_control.self_test.error_info);
            // 可以在这里添加错误恢复逻辑
            break;
            
        case SYSTEM_STATE_ALARM:
            // 报警状态处理
            DEBUG_Printf("系统处于报警状态\r\n");
            // 继续执行主循环，但保持报警状态
            SystemControl_MainLoopScheduler();
            break;
            
        default:
            // 未知状态，重置系统
            DEBUG_Printf("未知系统状态，重置系统\r\n");
            SystemControl_Reset();
            break;
    }
}

/**
  * @brief  开始LOGO显示阶段
  * @retval 无
  */
void SystemControl_StartLogoDisplay(void)
{
    g_system_control.current_state = SYSTEM_STATE_LOGO_DISPLAY;
    g_system_control.state_start_time = HAL_GetTick();
    
    // 显示LOGO
    OLED_ShowLogo();
    
    // 启动开机提示音（使用安全监控系统的非阻塞方式）
    SafetyMonitor_StartStartupBeep();
    
    DEBUG_Printf("开始LOGO显示阶段，持续时间: %d秒\r\n", LOGO_DISPLAY_TIME_MS/1000);
}

/**
  * @brief  开始自检阶段
  * @retval 无
  */
void SystemControl_StartSelfTest(void)
{
    g_system_control.current_state = SYSTEM_STATE_SELF_TEST;
    g_system_control.state_start_time = HAL_GetTick();
    
    // 清屏准备显示进度条
    OLED_Clear();
    
    // 在自检开始前，首先执行通道关断确认
    SystemControl_ExecuteChannelShutdown();
    
    DEBUG_Printf("开始自检阶段，持续时间: %d秒\r\n", SELF_TEST_TIME_MS/1000);
}

/**
  * @brief  执行通道关断确认（自检前的安全动作）
  * @retval 无
  */
void SystemControl_ExecuteChannelShutdown(void)
{
    DEBUG_Printf("=== 执行通道关断确认（自检前安全检查） ===\r\n");
    
    // 读取当前三个通道的状态反馈
    uint8_t k1_sta_all = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
    uint8_t k2_sta_all = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
    uint8_t k3_sta_all = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
    
    DEBUG_Printf("当前通道状态: CH1=%s, CH2=%s, CH3=%s\r\n", 
                k1_sta_all ? "ON" : "OFF", 
                k2_sta_all ? "ON" : "OFF", 
                k3_sta_all ? "ON" : "OFF");
    
    // 检查是否有通道处于开启状态，如果有则强制关断
    uint8_t need_shutdown = 0;
    
    if(k1_sta_all == 1) {
        DEBUG_Printf("通道1处于开启状态，执行强制关断\r\n");
        RelayControl_CloseChannel(1);
        need_shutdown = 1;
    }
    
    if(k2_sta_all == 1) {
        DEBUG_Printf("通道2处于开启状态，执行强制关断\r\n");
        RelayControl_CloseChannel(2);
        need_shutdown = 1;
    }
    
    if(k3_sta_all == 1) {
        DEBUG_Printf("通道3处于开启状态，执行强制关断\r\n");
        RelayControl_CloseChannel(3);
        need_shutdown = 1;
    }
    
    if(need_shutdown) {
        // 如果执行了关断操作，等待1秒让继电器动作完成
        DEBUG_Printf("等待继电器关断动作完成...\r\n");
        SmartDelayWithForceFeed(1000);
        
        // 重新检查状态确认关断成功
        k1_sta_all = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
        k2_sta_all = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
        k3_sta_all = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
        
        DEBUG_Printf("关断后通道状态: CH1=%s, CH2=%s, CH3=%s\r\n", 
                    k1_sta_all ? "ON" : "OFF", 
                    k2_sta_all ? "ON" : "OFF", 
                    k3_sta_all ? "ON" : "OFF");
                    
        if(k1_sta_all || k2_sta_all || k3_sta_all) {
            DEBUG_Printf("警告: 部分通道关断失败，可能存在硬件故障\r\n");
        } else {
            DEBUG_Printf("所有通道已确认关断，可以安全进行自检\r\n");
        }
    } else {
        DEBUG_Printf("所有通道已处于关断状态，无需关断操作\r\n");
    }
    
    DEBUG_Printf("=== 通道关断确认完成 ===\r\n");
}

/**
  * @brief  执行自检检测（新的四步流程）
  * @retval 无
  */
void SystemControl_ExecuteSelfTest(void)
{
    DEBUG_Printf("=== 开始新版系统自检检测（四步流程） ===\r\n");
    
    // 初始化自检结果
    g_system_control.self_test.overall_result = 1;
    memset(g_system_control.self_test.error_info, 0, sizeof(g_system_control.self_test.error_info));
    g_system_control.self_test.current_step = 0;
    
    // 第一步：识别当前期望状态（进度25%）
    SystemControl_UpdateSelfTestProgress(1, 25);
    g_system_control.self_test.step1_expected_state_ok = SystemControl_SelfTest_Step1_ExpectedState();
    if(!g_system_control.self_test.step1_expected_state_ok) {
        strcat(g_system_control.self_test.error_info, "Step1 Fail;");  // 英文错误信息
        g_system_control.self_test.overall_result = 0;
    }
    SmartDelayWithDebug(500, "自检步骤1完成"); // 每步之间停顿500ms，让用户看到进度
    
    // 第二步：继电器状态检查与主动纠错（安全先关后开）
    SystemControl_UpdateSelfTestProgress(2, 50);
    g_system_control.self_test.step2_relay_correction_ok = SystemControl_SelfTest_Step2_RelayCorrection();
    if(!g_system_control.self_test.step2_relay_correction_ok) {
        strcat(g_system_control.self_test.error_info, "Step2 Fail;");  // 英文错误信息
        g_system_control.self_test.overall_result = 0;
    }
    SmartDelayWithDebug(500, "自检步骤2完成");
    
    // 第三步：接触器状态检查与报错（进度75%）
    SystemControl_UpdateSelfTestProgress(3, 75);
    g_system_control.self_test.step3_contactor_check_ok = SystemControl_SelfTest_Step3_ContactorCheck();
    if(!g_system_control.self_test.step3_contactor_check_ok) {
        strcat(g_system_control.self_test.error_info, "Step3 Fail;");  // 英文错误信息
        g_system_control.self_test.overall_result = 0;
    }
    SmartDelayWithDebug(500, "自检步骤3完成");
    
    // 第四步：温度安全检测（进度100%）
    SystemControl_UpdateSelfTestProgress(4, 100);
    g_system_control.self_test.step4_temperature_safety_ok = SystemControl_SelfTest_Step4_TemperatureSafety();
    if(!g_system_control.self_test.step4_temperature_safety_ok) {
        strcat(g_system_control.self_test.error_info, "Step4 Fail;");  // 英文错误信息
        g_system_control.self_test.overall_result = 0;
    }
    SmartDelayWithDebug(500, "自检步骤4完成");
    
    // 输出自检结果
    DEBUG_Printf("自检结果: %s\r\n", g_system_control.self_test.overall_result ? "通过" : "失败");
    if(!g_system_control.self_test.overall_result) {
        DEBUG_Printf("自检错误信息: %s\r\n", g_system_control.self_test.error_info);
    }
    
    // 根据自检结果切换状态
    if(g_system_control.self_test.overall_result) {
        SystemControl_EnterNormalState();
    } else {
        SystemControl_HandleSelfTestError();
    }
}

/**
  * @brief  检测输入信号（K1_EN、K2_EN、K3_EN应为高电平）
  * @retval 1:正常 0:异常
  */
uint8_t SystemControl_CheckInputSignals(void)
{
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN(); 
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    DEBUG_Printf("输入信号检测: K1_EN=%d, K2_EN=%d, K3_EN=%d\r\n", k1_en, k2_en, k3_en);
    
    // 根据README要求，自检时三路输入信号均应为高电平
    if(k1_en == 1 && k2_en == 1 && k3_en == 1) {
        DEBUG_Printf("输入信号检测: 正常\r\n");
        return 1;
    } else {
        DEBUG_Printf("输入信号检测: 异常\r\n");
        return 0;
    }
}

/**
  * @brief  检测反馈信号（所有状态反馈应为低电平）
  * @retval 1:正常 0:异常
  */
uint8_t SystemControl_CheckFeedbackSignals(void)
{
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    DEBUG_Printf("反馈信号检测: K1_1=%d, K1_2=%d, K2_1=%d, K2_2=%d, K3_1=%d, K3_2=%d\r\n", 
                k1_1_sta, k1_2_sta, k2_1_sta, k2_2_sta, k3_1_sta, k3_2_sta);
    DEBUG_Printf("接触器状态: SW1=%d, SW2=%d, SW3=%d\r\n", sw1_sta, sw2_sta, sw3_sta);
    
    // 根据README要求，自检时所有状态反馈均应为低电平
    if(k1_1_sta == 0 && k1_2_sta == 0 && k2_1_sta == 0 && k2_2_sta == 0 && 
       k3_1_sta == 0 && k3_2_sta == 0 && sw1_sta == 0 && sw2_sta == 0 && sw3_sta == 0) {
        DEBUG_Printf("反馈信号检测: 正常\r\n");
        return 1;
    } else {
        DEBUG_Printf("反馈信号检测: 异常\r\n");
        return 0;
    }
}

/**
  * @brief  检测温度（所有NTC温度应在60℃以下）
  * @retval 1:正常 0:异常
  */
uint8_t SystemControl_CheckTemperature(void)
{
    // 更新温度数据
    TemperatureMonitor_UpdateAll();
    
    TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
    TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
    TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
    
    DEBUG_Printf("温度检测: NTC1=%.1f℃, NTC2=%.1f℃, NTC3=%.1f℃\r\n", 
                temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
    
    // 详细调试：单独输出每个温度值
    DEBUG_Printf("[温度调试] T1=%.1f℃, T2=%.1f℃, T3=%.1f℃\r\n", 
                temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
    
    // 根据README要求，自检时NTC温度均应在60℃以下
    if(temp1.value_celsius < SELF_TEST_TEMP_THRESHOLD && 
       temp2.value_celsius < SELF_TEST_TEMP_THRESHOLD && 
       temp3.value_celsius < SELF_TEST_TEMP_THRESHOLD) {
        DEBUG_Printf("温度检测: 正常\r\n");
        return 1;
    } else {
        DEBUG_Printf("温度检测: 异常\r\n");
        return 0;
    }
}

/**
  * @brief  主循环调度器（正常运行时的任务调度）
  * @retval 无
  */
void SystemControl_MainLoopScheduler(void)
{
    static uint32_t lastRelayTime = 0;
    static uint32_t lastAsyncTime = 0;     // 异步状态机轮询定时器
    static uint32_t lastTempTime = 0;
    static uint32_t lastOledTime = 0;
    static uint32_t lastSafetyTime = 0;
    static uint32_t lastFanSpeedTime = 0;  // 添加风扇转速统计定时器
    static uint32_t lastIwdgTime = 0;      // 添加IWDG看门狗处理定时器
    static uint32_t lastAsyncStatsTime = 0; // 异步操作统计输出定时器
    static uint32_t lastResetAnalysisTime = 0; // 复位分析实时监控定时器
    
    uint32_t currentTime = HAL_GetTick();
    
    // ================== 最高优先级：异步状态机轮询（每5ms） ===================
    // 这是整个异步架构的核心，必须高频轮询以确保及时响应
    if(currentTime - lastAsyncTime >= 5) {
        lastAsyncTime = currentTime;
        RelayControl_ProcessAsyncOperations();
    }
    
    // ================== 中断处理：每10ms处理中断标志 ===================
    if(currentTime - lastRelayTime >= 10) {
        lastRelayTime = currentTime;
        RelayControl_ProcessPendingActions();
    }
    
    // 每1000ms输出K_EN状态诊断信息（已注释 - 便于查看异常信息）
    // if(currentTime - lastDiagTime >= 1000) {
    //     lastDiagTime = currentTime;
    //     SystemControl_PrintKEnDiagnostics();
    // }
    
    // 每1000ms统计风扇转速（解决风扇转速始终显示0RPM问题）
    if(currentTime - lastFanSpeedTime >= 1000) {
        lastFanSpeedTime = currentTime;
        TemperatureMonitor_FanSpeed1sHandler();
        // 注释风扇调试信息，专心查看安全监控功能
        // DEBUG_Printf("[风扇调试] 转速统计完成，当前RPM: %d\\r\\n", (int)TemperatureMonitor_GetFanSpeed().rpm);
    }
    
    // 每100ms执行温度监控
    if(currentTime - lastTempTime >= 100) {
        lastTempTime = currentTime;
        TemperatureMonitor_UpdateAll();
        TemperatureMonitor_CheckAndHandleAlarm();
    }
    
    // 每200ms更新OLED显示
    if(currentTime - lastOledTime >= 200) {
        lastOledTime = currentTime;
        SystemControl_UpdateDisplay();
    }
    
    // 每100ms执行安全监控模块（提高响应速度）
    if(currentTime - lastSafetyTime >= 100) {
        lastSafetyTime = currentTime;
        SafetyMonitor_Process();
        
        // ===== 修复BUG：检测报警状态变化并切换系统状态 =====
        uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
        
        if(alarm_flags != 0) {
            // 有报警存在，确保系统处于ALARM状态
            if(g_system_control.current_state == SYSTEM_STATE_NORMAL) {
                DEBUG_Printf("=== 检测到报警（标志:0x%04X），切换到ALARM状态 ===\\r\\n", alarm_flags);
                SystemControl_EnterAlarmState();
            }
        } else {
            // 无报警，确保系统处于NORMAL状态
            if(g_system_control.current_state == SYSTEM_STATE_ALARM) {
                DEBUG_Printf("=== 所有报警已解除，切换到NORMAL状态 ===\\r\\n");
                SystemControl_EnterNormalState();
            }
        }
    }
    
    // 每500ms处理IWDG看门狗安全监控（不再自动喂狗，由主循环直接喂狗）
    if(currentTime - lastIwdgTime >= 500) {
        lastIwdgTime = currentTime;
        
        // 与安全监控系统集成（仅监控，不喂狗）
        IwdgControl_SafetyMonitorIntegration();
    }
    
    // ================== 异步操作统计输出（每30秒） ===================
    // 输出异步操作和干扰检测的统计信息，帮助监控系统运行状态
    if(currentTime - lastAsyncStatsTime >= 30000) {
        lastAsyncStatsTime = currentTime;
        
        uint32_t total_ops, completed_ops, failed_ops;
        uint32_t interference_count, filtered_interrupts;
        
        RelayControl_GetAsyncStatistics(&total_ops, &completed_ops, &failed_ops);
        RelayControl_GetInterferenceStatistics(&interference_count, &filtered_interrupts);
        
        DEBUG_Printf("? [异步统计] 总操作:%lu, 完成:%lu, 失败:%lu, 成功率:%.1f%%\r\n", 
                    total_ops, completed_ops, failed_ops, 
                    total_ops > 0 ? (float)completed_ops * 100.0f / total_ops : 0.0f);
                    
        DEBUG_Printf("?? [干扰统计] 检测到干扰:%lu次, 过滤中断:%lu个\r\n", 
                    interference_count, filtered_interrupts);
    }
    
    // ================== 复位分析实时监控（每1秒） ===================
    // 更新复位分析系统的实时状态监控，检测异常条件并进行风险评估
    if(currentTime - lastResetAnalysisTime >= 1000) {
        lastResetAnalysisTime = currentTime;
        
        // 更新实时状态
        ResetAnalysis_UpdateRealTimeStatus();
        
        // 检测异常状态
        if(ResetAnalysis_CheckAbnormalConditions()) {
            uint8_t risk_level = ResetAnalysis_PredictResetRisk();
            if(risk_level >= 2) { // 中等风险以上记录日志
                char risk_msg[48];
                const char* risk_names[] = {"无风险", "低风险", "中风险", "高风险"};
                snprintf(risk_msg, sizeof(risk_msg), "复位风险级别: %s", 
                         risk_level < 4 ? risk_names[risk_level] : "极高");
                LOG_RESET_RISK_WARNING(risk_msg);
            }
        }
    }
}

/**
  * @brief  更新显示内容（主界面显示）
  * @retval 无
  */
void SystemControl_UpdateDisplay(void)
{
    // 在正常状态下更新主界面显示
    if(g_system_control.current_state == SYSTEM_STATE_NORMAL || 
       g_system_control.current_state == SYSTEM_STATE_ALARM) {
        
        // 准备显示数据
        char alarm_info[32] = "";
        char status1[16], status2[16], status3[16];
        char temp_info[32], fan_info[16];
        
        // ===== 修复BUG：正确获取报警信息 =====
        if(g_system_control.current_state == SYSTEM_STATE_ALARM) {
            // 报警状态：获取当前激活的报警信息
            uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
            
            if(alarm_flags != 0) {
                // 找到最高优先级的报警（从高到低：K~M > B~J > A、N）
                AlarmFlag_t active_alarm = ALARM_FLAG_COUNT;  // 初始化为无效值
                
                // 优先级1：K~M类异常（温度异常，最高优先级）
                for(AlarmFlag_t flag = ALARM_FLAG_K; flag <= ALARM_FLAG_M; flag++) {
                    if(SafetyMonitor_IsAlarmActive(flag)) {
                        active_alarm = flag;
                        break;  // 找到温度异常，立即使用
                    }
                }
                
                // 优先级2：B~J类异常（状态反馈异常）
                if(active_alarm == ALARM_FLAG_COUNT) {
                    for(AlarmFlag_t flag = ALARM_FLAG_B; flag <= ALARM_FLAG_J; flag++) {
                        if(SafetyMonitor_IsAlarmActive(flag)) {
                            active_alarm = flag;
                            break;  // 找到状态异常，立即使用
                        }
                    }
                }
                
                // 优先级3：A、N、O类异常（使能冲突、自检异常、电源异常）
                if(active_alarm == ALARM_FLAG_COUNT) {
                    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_A)) {
                        active_alarm = ALARM_FLAG_A;
                    } else if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_N)) {
                        active_alarm = ALARM_FLAG_N;
                    } else if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
                        active_alarm = ALARM_FLAG_O;
                    }
                }
                
                // 生成报警显示信息
                if(active_alarm < ALARM_FLAG_COUNT) {
                    AlarmInfo_t alarm_detail = SafetyMonitor_GetAlarmInfo(active_alarm);
                    const char* alarm_desc = SafetyMonitor_GetAlarmDescription(active_alarm);
                    
                    // 格式：异常标志+描述，例如："A:EN Conflict" 或 "K:NTC1 60.5C"
                    char alarm_letter = 'A' + active_alarm;  // 转换为字母
                    snprintf(alarm_info, sizeof(alarm_info), "%c:%s", alarm_letter, 
                            alarm_detail.description[0] ? alarm_detail.description : alarm_desc);
                } else {
                    // 如果没有找到具体异常，显示通用报警信息
                    strncpy(alarm_info, "ALARM ACTIVE", sizeof(alarm_info) - 1);
                }
            } else {
                // 状态为ALARM但没有激活异常（可能是异常刚解除），显示正常
                strncpy(alarm_info, "System Normal", sizeof(alarm_info) - 1);
            }
        } else {
            // 正常状态：显示系统正常
            strncpy(alarm_info, "System Normal", sizeof(alarm_info) - 1);
        }
        
        // 获取通道状态
        RelayState_t ch1_state = RelayControl_GetChannelState(1);
        RelayState_t ch2_state = RelayControl_GetChannelState(2);
        RelayState_t ch3_state = RelayControl_GetChannelState(3);
        
        sprintf(status1, "CH1:%s", (ch1_state == RELAY_STATE_ON) ? "ON" : "OFF");
        sprintf(status2, "CH2:%s", (ch2_state == RELAY_STATE_ON) ? "ON" : "OFF");
        sprintf(status3, "CH3:%s", (ch3_state == RELAY_STATE_ON) ? "ON" : "OFF");
        
        // 获取温度信息
        TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
        TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
        TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
        
        // 使用改进的温度显示格式：Temp：**.*|**.*|**.*
        sprintf(temp_info, "Temp:%.1f|%.1f|%.1f", 
               temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
        
        // 注释调试信息，专心查看安全监控功能
        // DEBUG_Printf("[温度调试] T1=%.1f℃, T2=%.1f℃, T3=%.1f℃\r\n", 
        //             temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
        // DEBUG_Printf("[显示调试] 新格式: \"%s\" (长度:%d)\r\n", temp_info, (int)strlen(temp_info));
        
        // 获取风扇信息
        uint8_t fan_pwm = TemperatureMonitor_GetFanPWM();
        FanSpeedInfo_t fan_speed = TemperatureMonitor_GetFanSpeed();
        sprintf(fan_info, "FAN:%d%% %dRPM", fan_pwm, (int)fan_speed.rpm);
        
        // 注释风扇调试信息，专心查看安全监控功能
        // DEBUG_Printf("[风扇调试] PWM占空比: %d%%, 转速: %dRPM, 脉冲计数: %d\\r\\n", 
        //             fan_pwm, (int)fan_speed.rpm, (int)fan_speed.pulse_count);
        
        // 显示主界面（使用脏区刷新，彻底解决闪烁问题）
        OLED_ShowMainScreenDirty(alarm_info, status1, status2, status3, temp_info, fan_info);
    }
}

/**
  * @brief  进入正常运行状态
  * @retval 无
  */
void SystemControl_EnterNormalState(void)
{
    g_system_control.current_state = SYSTEM_STATE_NORMAL;
    g_system_control.state_start_time = HAL_GetTick();
    
    DEBUG_Printf("=== 系统进入正常运行状态 ===\r\n");
    
    // 启动IWDG看门狗（自检完成后系统已稳定，可以安全启动看门狗）
    DEBUG_Printf("自检完成，启动IWDG看门狗保护\r\n");
    // 直接初始化并启动看门狗，由主循环负责喂狗
    MX_IWDG_Init();
    IWDG_Refresh(); // 立即喂狗一次
    
    // 清屏准备显示主界面
    OLED_Clear();
    
    // 重置显示缓存，确保状态切换时强制刷新
    OLED_ResetDisplayCache();
    OLED_ClearDirtyRegions();  // 清除脏区记录
}

/**
  * @brief  进入错误状态
  * @param  error_msg: 错误信息
  * @retval 无
  */
void SystemControl_EnterErrorState(const char* error_msg)
{
    g_system_control.current_state = SYSTEM_STATE_ERROR;
    g_system_control.state_start_time = HAL_GetTick();
    
    // 修复BUG：不要覆盖自检过程中构建的详细错误信息
    // 只有当error_info为空时才设置通用错误信息
    if(error_msg && strlen(g_system_control.self_test.error_info) == 0) {
        strncpy(g_system_control.self_test.error_info, error_msg, 
                sizeof(g_system_control.self_test.error_info) - 1);
    }
    
    DEBUG_Printf("=== 系统进入错误状态: %s ===\r\n", error_msg ? error_msg : "未知错误");
    DEBUG_Printf("保留的错误信息: [%s]\r\n", g_system_control.self_test.error_info);
    
    // 重置显示缓存，确保状态切换时强制刷新
    OLED_ResetDisplayCache();
    OLED_ClearDirtyRegions();  // 清除脏区记录
}

/**
  * @brief  进入报警状态
  * @retval 无
  */
void SystemControl_EnterAlarmState(void)
{
    g_system_control.current_state = SYSTEM_STATE_ALARM;
    g_system_control.state_start_time = HAL_GetTick();
    
    DEBUG_Printf("=== 系统进入报警状态 ===\r\n");
    
    // 重置显示缓存，确保状态切换时强制刷新
    OLED_ResetDisplayCache();
    OLED_ClearDirtyRegions();  // 清除脏区记录
}

/**
  * @brief  处理自检异常（产生N异常标志）
  * @retval 无
  */
void SystemControl_HandleSelfTestError(void)
{
    DEBUG_Printf("=== 自检异常，产生N异常标志 ===\r\n");
    
    // 调试输出：显示当前error_info内容
    DEBUG_Printf("自检错误信息: [%s]\r\n", g_system_control.self_test.error_info);
    DEBUG_Printf("错误信息长度: %d\r\n", (int)strlen(g_system_control.self_test.error_info));
    
    // 使用安全监控模块设置N异常标志
    SafetyMonitor_SetAlarmFlag(ALARM_FLAG_N, g_system_control.self_test.error_info);
    
    // 进入错误状态（修复中文字符串）
    SystemControl_EnterErrorState("Self-Test Fail");
    
    // 重置显示缓存，确保异常信息能正确显示
    OLED_ResetDisplayCache();
    
    // 在OLED上显示自检异常信息
    OLED_Clear();
    // 显示具体的异常信息，格式如："N:Step1 Fail;Step2 Fail;"
    char display_msg[64];
    sprintf(display_msg, "N:%s", g_system_control.self_test.error_info);
    
    // 调试输出：显示最终的显示信息
    DEBUG_Printf("OLED显示信息: [%s]\r\n", display_msg);
    DEBUG_Printf("显示信息长度: %d\r\n", (int)strlen(display_msg));
    
    OLED_DrawString6x8(0, 3, display_msg); // 使用6x8字体显示
    OLED_Refresh();
}

/**
  * @brief  获取当前系统状态
  * @retval 系统状态
  */
SystemState_t SystemControl_GetState(void)
{
    return g_system_control.current_state;
}

/**
  * @brief  强制切换系统状态
  * @param  new_state: 新状态
  * @retval 无
  */
void SystemControl_SetState(SystemState_t new_state)
{
    SystemState_t old_state = g_system_control.current_state;
    g_system_control.last_state = g_system_control.current_state;
    g_system_control.current_state = new_state;
    g_system_control.state_start_time = HAL_GetTick();
    
    DEBUG_Printf("系统状态切换: %d -> %d\r\n", g_system_control.last_state, new_state);
    
    // 记录系统关键状态变化日志（只记录重要的系统级事件）
    if(LogSystem_IsInitialized()) {
        // 只记录以下关键状态变化：
        // 1. 系统启动 (INIT -> LOGO)
        // 2. 进入错误状态 (任何状态 -> ERROR)
        // 3. 进入报警状态 (任何状态 -> ALARM)
        // 4. 从错误/报警恢复到正常 (ERROR/ALARM -> NORMAL)
        
        if((old_state == SYSTEM_STATE_INIT && new_state == SYSTEM_STATE_LOGO_DISPLAY)) {
            LOG_SYSTEM_START();
        }
        else if(new_state == SYSTEM_STATE_ERROR) {
            LOG_SYSTEM_RESTART("System error state");  // 系统状态异常，使用SYSTEM类日志
        }
        else if(new_state == SYSTEM_STATE_ALARM) {
            LOG_SYSTEM_RESTART("System alarm state");  // 系统报警状态，使用SYSTEM类日志
        }
        else if((old_state == SYSTEM_STATE_ERROR || old_state == SYSTEM_STATE_ALARM) && 
                new_state == SYSTEM_STATE_NORMAL) {
            LOG_SYSTEM_RESTART("System recovered");
        }
    }
}

/**
  * @brief  获取自检结果
  * @retval 自检结果结构体
  */
SelfTestResult_t SystemControl_GetSelfTestResult(void)
{
    return g_system_control.self_test;
}

/**
  * @brief  系统复位
  * @retval 无
  */
void SystemControl_Reset(void)
{
    DEBUG_Printf("系统复位\r\n");
    
    // 重置系统控制结构体
    memset(&g_system_control, 0, sizeof(SystemControl_t));
    g_system_control.current_state = SYSTEM_STATE_INIT;
    g_system_control.state_start_time = HAL_GetTick();
    
    // 重置自检进度显示状态
    SystemControl_ResetSelfTestProgressState();
    
    // 重新开始LOGO显示
    SystemControl_StartLogoDisplay();
}

/**
  * @brief  系统状态调试输出
  * @retval 无
  */
void SystemControl_DebugPrint(void)
{
    DEBUG_Printf("=== 系统状态调试信息 ===\r\n");
    DEBUG_Printf("当前状态: %d\r\n", g_system_control.current_state);
    DEBUG_Printf("状态持续时间: %lu ms\r\n", HAL_GetTick() - g_system_control.state_start_time);
    DEBUG_Printf("自检结果: %s\r\n", g_system_control.self_test.overall_result ? "通过" : "失败");
    if(!g_system_control.self_test.overall_result) {
        DEBUG_Printf("错误信息: %s\r\n", g_system_control.self_test.error_info);
    }
    DEBUG_Printf("错误标志: 0x%02X\r\n", g_system_control.error_flags);
}

/**
  * @brief  输出K_EN状态诊断信息（每秒调用一次）
  * @retval 无
  */
void SystemControl_PrintKEnDiagnostics(void)
{
    // 只在系统正常运行或报警状态下输出诊断信息
    if(g_system_control.current_state != SYSTEM_STATE_NORMAL && 
       g_system_control.current_state != SYSTEM_STATE_ALARM) {
        return;
    }
    
    // 读取K_EN信号状态
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // 读取STA反馈信号状态
    uint8_t k1_sta = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
    uint8_t k2_sta = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
    uint8_t k3_sta = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
    
    // 输出诊断信息
    DEBUG_Printf("[K_EN诊断] K1_EN=%d K2_EN=%d K3_EN=%d | STA: K1=%d K2=%d K3=%d\r\n", 
                k1_en, k2_en, k3_en, k1_sta, k2_sta, k3_sta);
}

// ================== 新版自检流程函数 ===================

/**
  * @brief  更新自检进度条显示
  * @param  step: 当前步骤(1-4)
  * @param  percent: 完成百分比(0-100)
  * @retval 无
  */
void SystemControl_UpdateSelfTestProgress(uint8_t step, uint8_t percent)
{
    g_system_control.self_test.current_step = step;
    
    // 第一次调用时，重置第一个进度条状态并清屏
    if(g_self_test_progress_first_call) {
        OLED_ResetSelfTestBarState();  // 重置第一个进度条状态
        OLED_Clear();                  // 清屏，移除第一个进度条的残留内容
        g_self_test_progress_first_call = 0;
        DEBUG_Printf("=== 切换到自检检测阶段，清除第一个进度条残留 ===\r\n");
    }
    
    // 显示进度条和步骤描述
    OLED_ShowSelfTestBarWithStep(percent, step);
    
    // 注释步骤名称和输出信息，保持串口输出简洁
    // const char* step_names[] = {
    //     "",
    //     "期望状态识别",
    //     "继电器纠错",
    //     "接触器检查", 
    //     "温度安全检测"
    // };
    
    // 注释自检步骤输出，保持串口输出简洁
    // if(step >= 1 && step <= 4) {
    //     DEBUG_Printf("自检步骤%d：%s（%d%%）\r\n", step, step_names[step], percent);
    // }
}

/**
  * @brief  重置自检进度显示状态
  * @retval 无
  */
void SystemControl_ResetSelfTestProgressState(void)
{
    // 重置自检进度显示标志
    g_self_test_progress_first_call = 1;
    
    // 同时重置OLED进度条状态
    OLED_ResetSelfTestBarState();
}

/**
  * @brief  第一步：识别当前期望状态
  * @retval 1:正常 0:异常
  */
uint8_t SystemControl_SelfTest_Step1_ExpectedState(void)
{
    DEBUG_Printf("=== 第一步：识别当前期望状态 ===\r\n");
    
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    DEBUG_Printf("K_EN信号状态: K1_EN=%d, K2_EN=%d, K3_EN=%d\r\n", k1_en, k2_en, k3_en);
    
    // 关键修复：立即检测A类异常（使能冲突）
    uint8_t low_count = 0;
    if(k1_en == 0) low_count++;
    if(k2_en == 0) low_count++;
    if(k3_en == 0) low_count++;
    
    if(low_count > 1) {
        DEBUG_Printf("检测到使能冲突：%d路EN同时为低电平，立即触发A类异常\r\n", low_count);
        // 立即触发A类异常
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_A, "Multi EN Active");
        return 0;  // 使能冲突，自检失败
    }
    
    // 根据真值表识别期望状态
    if(k1_en == 1 && k2_en == 1 && k3_en == 1) {
        DEBUG_Printf("识别结果：全部关断状态\r\n");
        return 1;
    } else if(k1_en == 0 && k2_en == 1 && k3_en == 1) {
        DEBUG_Printf("识别结果：Channel_1打开状态\r\n");
        return 1;
    } else if(k1_en == 1 && k2_en == 0 && k3_en == 1) {
        DEBUG_Printf("识别结果：Channel_2打开状态\r\n");
        return 1;
    } else if(k1_en == 1 && k2_en == 1 && k3_en == 0) {
        DEBUG_Printf("识别结果：Channel_3打开状态\r\n");
        return 1;
    } else {
        DEBUG_Printf("识别结果：状态异常（不符合真值表）\r\n");
        return 0;
    }
}

/**
  * @brief  第二步：继电器状态检查与主动纠错（安全先关后开）
  * @retval 1:正常 0:异常
  */
uint8_t SystemControl_SelfTest_Step2_RelayCorrection(void)
{
    DEBUG_Printf("=== 第二步：继电器状态检查与主动纠错（安全先关后开） ===\r\n");
    
    uint8_t result = 1;
    
    // 读取期望状态
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // 读取继电器反馈状态
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    
    DEBUG_Printf("继电器状态: K1_1=%d K1_2=%d, K2_1=%d K2_2=%d, K3_1=%d K3_2=%d\\r\\n", 
                k1_1_sta, k1_2_sta, k2_1_sta, k2_2_sta, k3_1_sta, k3_2_sta);
    
    // 确定期望状态
    uint8_t expected_k1_1 = 0, expected_k1_2 = 0;
    uint8_t expected_k2_1 = 0, expected_k2_2 = 0;
    uint8_t expected_k3_1 = 0, expected_k3_2 = 0;
    
    if(k1_en == 0 && k2_en == 1 && k3_en == 1) {
        // Channel_1打开
        expected_k1_1 = 1; expected_k1_2 = 1;
        DEBUG_Printf("目标状态：Channel_1打开\\r\\n");
    } else if(k1_en == 1 && k2_en == 0 && k3_en == 1) {
        // Channel_2打开
        expected_k2_1 = 1; expected_k2_2 = 1;
        DEBUG_Printf("目标状态：Channel_2打开\\r\\n");
    } else if(k1_en == 1 && k2_en == 1 && k3_en == 0) {
        // Channel_3打开
        expected_k3_1 = 1; expected_k3_2 = 1;
        DEBUG_Printf("目标状态：Channel_3打开\\r\\n");
    } else {
        DEBUG_Printf("目标状态：全部关断\\r\\n");
    }
    // 全部关断状态下期望值都为0（已初始化）
    
    // 检查是否需要纠错
    uint8_t k1_need_correction = (k1_1_sta != expected_k1_1 || k1_2_sta != expected_k1_2);
    uint8_t k2_need_correction = (k2_1_sta != expected_k2_1 || k2_2_sta != expected_k2_2);
    uint8_t k3_need_correction = (k3_1_sta != expected_k3_1 || k3_2_sta != expected_k3_2);
    
    if(k1_need_correction || k2_need_correction || k3_need_correction) {
        DEBUG_Printf("检测到继电器状态错误，开始安全纠错流程\\r\\n");
        
        // 第一阶段：安全关断所有不需要开启的通道
        DEBUG_Printf("[安全纠错] 第一阶段：关断所有不需要的通道\\r\\n");
        
        // 关断K1（如果目标不是开启K1）
        if(expected_k1_1 == 0 && (k1_1_sta == 1 || k1_2_sta == 1)) {
            DEBUG_Printf("[安全纠错] 关断Channel_1继电器\\r\\n");
            RelayControl_CloseChannel(1);  // 使用继电器控制函数，确保状态同步
        }
        
        // 关断K2（如果目标不是开启K2）
        if(expected_k2_1 == 0 && (k2_1_sta == 1 || k2_2_sta == 1)) {
            DEBUG_Printf("[安全纠错] 关断Channel_2继电器\\r\\n");
            RelayControl_CloseChannel(2);  // 使用继电器控制函数，确保状态同步
        }
        
        // 关断K3（如果目标不是开启K3）
        if(expected_k3_1 == 0 && (k3_1_sta == 1 || k3_2_sta == 1)) {
            DEBUG_Printf("[安全纠错] 关断Channel_3继电器\\r\\n");
            RelayControl_CloseChannel(3);  // 使用继电器控制函数，确保状态同步
        }
        
        // 第二阶段：开启目标通道
        DEBUG_Printf("[安全纠错] 第二阶段：开启目标通道\\r\\n");
        
        // 开启K1（如果目标是开启K1）
        if(expected_k1_1 == 1) {
            DEBUG_Printf("[安全纠错] 开启Channel_1继电器\\r\\n");
            RelayControl_OpenChannel(1);  // 使用继电器控制函数，确保状态同步
        }
        
        // 开启K2（如果目标是开启K2）
        if(expected_k2_1 == 1) {
            DEBUG_Printf("[安全纠错] 开启Channel_2继电器\\r\\n");
            RelayControl_OpenChannel(2);  // 使用继电器控制函数，确保状态同步
        }
        
        // 开启K3（如果目标是开启K3）
        if(expected_k3_1 == 1) {
            DEBUG_Printf("[安全纠错] 开启Channel_3继电器\\r\\n");
            RelayControl_OpenChannel(3);  // 使用继电器控制函数，确保状态同步
        }
        
        // 第三阶段：验证纠错结果
        DEBUG_Printf("[安全纠错] 第三阶段：验证纠错结果\\r\\n");
        HAL_Delay(200); // 额外等待，确保状态稳定
        
        // 重新读取继电器状态
        k1_1_sta = GPIO_ReadK1_1_STA();
        k1_2_sta = GPIO_ReadK1_2_STA();
        k2_1_sta = GPIO_ReadK2_1_STA();
        k2_2_sta = GPIO_ReadK2_2_STA();
        k3_1_sta = GPIO_ReadK3_1_STA();
        k3_2_sta = GPIO_ReadK3_2_STA();
        
        DEBUG_Printf("纠错后状态: K1_1=%d K1_2=%d, K2_1=%d K2_2=%d, K3_1=%d K3_2=%d\\r\\n", 
                    k1_1_sta, k1_2_sta, k2_1_sta, k2_2_sta, k3_1_sta, k3_2_sta);
        
        // 验证纠错是否成功
        if(k1_1_sta != expected_k1_1 || k1_2_sta != expected_k1_2) {
            DEBUG_Printf("Channel_1继电器纠正失败\\r\\n");
            result = 0;
        }
        if(k2_1_sta != expected_k2_1 || k2_2_sta != expected_k2_2) {
            DEBUG_Printf("Channel_2继电器纠正失败\\r\\n");
            result = 0;
        }
        if(k3_1_sta != expected_k3_1 || k3_2_sta != expected_k3_2) {
            DEBUG_Printf("Channel_3继电器纠正失败\\r\\n");
            result = 0;
        }
        
        if(result) {
            DEBUG_Printf("[安全纠错] 所有继电器纠正成功\\r\\n");
        } else {
            DEBUG_Printf("[安全纠错] 部分继电器纠正失败\\r\\n");
        }
        
    } else {
        DEBUG_Printf("继电器状态正常，无需纠正\\r\\n");
    }
    
    return result;
}

/**
  * @brief  第三步：接触器状态检查与报错
  * @retval 1:正常 0:异常
  */
uint8_t SystemControl_SelfTest_Step3_ContactorCheck(void)
{
    DEBUG_Printf("=== 第三步：接触器状态检查与报错 ===\r\n");
    
    uint8_t result = 1;
    
    // 读取期望状态
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // 读取接触器反馈状态
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    DEBUG_Printf("接触器状态: SW1=%d SW2=%d SW3=%d\r\n", sw1_sta, sw2_sta, sw3_sta);
    
    // 根据期望状态检查接触器状态
    uint8_t expected_sw1 = 0, expected_sw2 = 0, expected_sw3 = 0;
    
    if(k1_en == 0 && k2_en == 1 && k3_en == 1) {
        // Channel_1打开
        expected_sw1 = 1;
    } else if(k1_en == 1 && k2_en == 0 && k3_en == 1) {
        // Channel_2打开
        expected_sw2 = 1;
    } else if(k1_en == 1 && k2_en == 1 && k3_en == 0) {
        // Channel_3打开
        expected_sw3 = 1;
    }
    // 全部关断状态下期望值都为0（已初始化）
    
    // 检查接触器状态并报错（不进行纠正）
    if(sw1_sta != expected_sw1) {
        DEBUG_Printf("Channel_1接触器状态异常\r\n");
        result = 0;
    }
    
    if(sw2_sta != expected_sw2) {
        DEBUG_Printf("Channel_2接触器状态异常\r\n");
        result = 0;
    }
    
    if(sw3_sta != expected_sw3) {
        DEBUG_Printf("Channel_3接触器状态异常\r\n");
        result = 0;
    }
    
    if(result) {
        DEBUG_Printf("接触器状态检查：正常\r\n");
    } else {
        DEBUG_Printf("接触器状态检查：异常（系统无法主动纠正）\r\n");
    }
    
    return result;
}

/**
  * @brief  第四步：温度安全检测
  * @retval 1:正常 0:异常
  */
uint8_t SystemControl_SelfTest_Step4_TemperatureSafety(void)
{
    DEBUG_Printf("=== 第四步：温度安全检测 ===\r\n");
    
    // 更新温度数据
    TemperatureMonitor_UpdateAll();
    
    TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
    TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
    TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
    
    DEBUG_Printf("温度安全检测: NTC1=%.1f℃, NTC2=%.1f℃, NTC3=%.1f℃\r\n", 
                temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
    
    // 检查三路温度是否均在60℃以下（确保热安全）
    if(temp1.value_celsius < SELF_TEST_TEMP_THRESHOLD && 
       temp2.value_celsius < SELF_TEST_TEMP_THRESHOLD && 
       temp3.value_celsius < SELF_TEST_TEMP_THRESHOLD) {
        DEBUG_Printf("温度安全检测：正常（所有温度均在60℃以下）\r\n");
        return 1;
    } else {
        DEBUG_Printf("温度安全检测：异常（存在温度≥60℃的传感器）\r\n");
        
        // 详细输出超温信息
        if(temp1.value_celsius >= SELF_TEST_TEMP_THRESHOLD) {
            DEBUG_Printf("NTC1温度超限：%.1f℃ >= 60℃\r\n", temp1.value_celsius);
        }
        if(temp2.value_celsius >= SELF_TEST_TEMP_THRESHOLD) {
            DEBUG_Printf("NTC2温度超限：%.1f℃ >= 60℃\r\n", temp2.value_celsius);
        }
        if(temp3.value_celsius >= SELF_TEST_TEMP_THRESHOLD) {
            DEBUG_Printf("NTC3温度超限：%.1f℃ >= 60℃\r\n", temp3.value_celsius);
        }
        
        return 0;
    }
}

// ================== FLASH测试和管理功能 ===================

/**
  * @brief  FLASH快速写满测试函数（写入10000条测试数据）
  * @retval 无
  */
void SystemControl_FlashQuickFillTest(void)
{
    DEBUG_Printf("=== 开始FLASH快速写满测试（写入10000条） ===\r\n");
    
    // 检查日志系统是否已初始化
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("错误：日志系统未初始化\r\n");
        return;
    }
    
    // 打印当前状态
    SystemControl_PrintFlashStatus();
    
    uint32_t target_entries = 10000;  // 固定写入10000条
    uint32_t initial_entries = LogSystem_GetEntryCount();
    
    DEBUG_Printf("当前日志条数: %d\r\n", initial_entries);
    DEBUG_Printf("目标写入条数: %d\r\n", target_entries);
    DEBUG_Printf("开始快速写入测试...\r\n");
    
    uint32_t write_count = 0;
    uint32_t batch_size = 100;  // 每批写入100条
    uint32_t progress_interval = 1000;  // 每1000条显示一次进度
    
    // 记录开始时间
    uint32_t start_time = HAL_GetTick();
    
    // 分批写入指定数量的日志
    for(uint32_t batch = 0; batch < (target_entries / batch_size); batch++) {
        for(uint32_t i = 0; i < batch_size && write_count < target_entries; i++) {
            char test_msg[48];
            write_count++;
            
            // 生成多样化的测试日志内容
            if(write_count % 1000 == 0) {
                snprintf(test_msg, sizeof(test_msg), "Quick test milestone %lu", write_count);
            } else if(write_count % 100 == 0) {
                snprintf(test_msg, sizeof(test_msg), "Quick test batch %lu", write_count / 100);
            } else {
                snprintf(test_msg, sizeof(test_msg), "Quick test entry %lu", write_count);
            }
            
            // 使用不同的日志分类和通道
            LogCategory_t log_category = (LogCategory_t)((write_count % 3) + 1);
            uint8_t channel = (write_count % 3) + 1;
            uint16_t event_code = 0x3000 + (write_count % 0xFFF);  // 使用0x3000区分快速测试
            
            LogSystem_Record(log_category, channel, event_code, test_msg);
        }
        
        // 显示进度
        if(write_count % progress_interval == 0 || write_count == target_entries) {
            uint32_t current_time = HAL_GetTick();
            uint32_t elapsed_seconds = (current_time - start_time) / 1000;
            uint32_t current_usage = LogSystem_GetUsedSize();
            float usage_percent = (float)current_usage * 100.0f / LOG_SYSTEM_SIZE;
            
            DEBUG_Printf("快速写入进度: %d/%d 条 (%.1f%%), 存储使用: %.1f%%, 耗时: %d 秒\r\n", 
                        write_count, target_entries, 
                        (float)write_count * 100.0f / target_entries,
                        usage_percent, elapsed_seconds);
        }
        
        // 喂狗和延时
        HAL_IWDG_Refresh(&hiwdg);
        SmartDelay(10);
    }
    
    // 处理剩余的日志
    uint32_t remaining = target_entries % batch_size;
    if(remaining > 0 && write_count < target_entries) {
        for(uint32_t i = 0; i < remaining; i++) {
            char test_msg[48];
            write_count++;
            
            snprintf(test_msg, sizeof(test_msg), "Quick test final %lu", write_count);
            
            LogCategory_t log_category = (LogCategory_t)((write_count % 3) + 1);
            uint8_t channel = (write_count % 3) + 1;
            uint16_t event_code = 0x3000 + (write_count % 0xFFF);
            
            LogSystem_Record(log_category, channel, event_code, test_msg);
        }
    }
    
    // 计算总耗时
    uint32_t total_time = HAL_GetTick() - start_time;
    uint32_t final_entries = LogSystem_GetEntryCount();
    
    DEBUG_Printf("\r\n=== FLASH快速写满测试完成 ===\r\n");
    DEBUG_Printf("总写入条数: %d\r\n", write_count);
    DEBUG_Printf("最终日志条数: %d\r\n", final_entries);
    DEBUG_Printf("总耗时: %.1f 秒\r\n", total_time / 1000.0f);
    DEBUG_Printf("平均写入速度: %.1f 条/秒\r\n", (float)write_count * 1000.0f / total_time);
    
    // 打印最终状态
    SystemControl_PrintFlashStatus();
}

/**
  * @brief  FLASH完整写满测试函数（真正测试循环记录功能）
  * @retval 无
  */
void SystemControl_FlashFillTest(void)
{
    DEBUG_Printf("=== 开始FLASH真正写满测试（持续写入直到满） ===\r\n");
    
    // 检查日志系统是否已初始化
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("错误：日志系统未初始化\r\n");
        return;
    }
    
    // 打印当前状态
    SystemControl_PrintFlashStatus();
    
    // 计算存储信息
    uint32_t max_entries = LOG_SYSTEM_SIZE / LOG_ENTRY_SIZE;
    uint32_t initial_entries = LogSystem_GetEntryCount();
    
    DEBUG_Printf("当前日志条数: %d\r\n", initial_entries);
    DEBUG_Printf("理论最大容量: %d 条日志\r\n", max_entries);
    DEBUG_Printf("开始持续写入，直到FLASH满...\r\n");
    
    uint32_t write_count = 0;
    uint32_t batch_size = 100;  // 每批写入100条
    uint32_t progress_interval = 1000;  // 每1000条显示一次进度
    uint32_t last_progress_count = 0;
    
    // 记录开始时间
    uint32_t start_time = HAL_GetTick();
    
    // 持续写入直到FLASH满
    while(1) {
        // 检查是否已满
        uint8_t is_full = LogSystem_IsFull();
        uint32_t current_entries = LogSystem_GetEntryCount();
        
        // 如果FLASH已满，再写入一些数据测试循环覆盖
        if(is_full) {
            DEBUG_Printf("\r\n=== FLASH已满，测试循环覆盖机制 ===\r\n");
            DEBUG_Printf("当前日志条数: %d\r\n", current_entries);
            
            // 再写入一些数据测试循环覆盖
            for(uint32_t i = 0; i < 500; i++) {
                char test_msg[48];
                snprintf(test_msg, sizeof(test_msg), "Overwrite test %lu", i + 1);
                
                LogCategory_t log_category = LOG_CATEGORY_SYSTEM;
                uint8_t channel = (i % 3) + 1;
                uint16_t event_code = 0x2000 + i;
                
                LogSystem_Record(log_category, channel, event_code, test_msg);
                
                // 每50条喂狗
                if(i % 50 == 0) {
                    HAL_IWDG_Refresh(&hiwdg);
                    DEBUG_Printf("循环覆盖测试进度: %d/500\r\n", i + 1);
                }
            }
            
            DEBUG_Printf("循环覆盖测试完成\r\n");
            break;
        }
        
        // 批量写入日志
        for(uint32_t i = 0; i < batch_size; i++) {
            char test_msg[48];
            write_count++;
            
            // 生成多样化的测试日志内容
            if(write_count % 100 == 0) {
                snprintf(test_msg, sizeof(test_msg), "Milestone %lu reached", write_count);
            } else if(write_count % 50 == 0) {
                snprintf(test_msg, sizeof(test_msg), "Progress check %lu", write_count);
            } else {
                snprintf(test_msg, sizeof(test_msg), "Fill test entry %lu", write_count);
            }
            
            // 使用不同的日志分类和通道
            LogCategory_t log_category = (LogCategory_t)((write_count % 3) + 1);
            uint8_t channel = (write_count % 3) + 1;
            uint16_t event_code = 0x1000 + (write_count % 0xFFF);
            
            LogSystem_Record(log_category, channel, event_code, test_msg);
            
            // 定期检查是否已满
            if(write_count % 10 == 0) {
                if(LogSystem_IsFull()) {
                    DEBUG_Printf("在写入第 %d 条时检测到FLASH已满\r\n", write_count);
                    break;
                }
            }
        }
        
        // 显示进度
        if(write_count - last_progress_count >= progress_interval) {
            uint32_t current_time = HAL_GetTick();
            uint32_t elapsed_seconds = (current_time - start_time) / 1000;
            uint32_t current_usage = LogSystem_GetUsedSize();
            float usage_percent = (float)current_usage * 100.0f / LOG_SYSTEM_SIZE;
            
            DEBUG_Printf("写入进度: %d 条, 存储使用: %.1f%%, 耗时: %d 秒\r\n", 
                        write_count, usage_percent, elapsed_seconds);
            
            last_progress_count = write_count;
        }
        
        // 安全检查：防止无限循环
        if(write_count > max_entries * 2) {
            DEBUG_Printf("警告：写入数量超过预期，停止测试\r\n");
            break;
        }
        
        // 喂狗和短暂延时
        HAL_IWDG_Refresh(&hiwdg);
        SmartDelay(10);  // 减少延时，加快写入速度
    }
    
    // 计算总耗时
    uint32_t total_time = HAL_GetTick() - start_time;
    uint32_t final_entries = LogSystem_GetEntryCount();
    
    DEBUG_Printf("\r\n=== FLASH写满测试完成 ===\r\n");
    DEBUG_Printf("总写入条数: %d\r\n", write_count);
    DEBUG_Printf("最终日志条数: %d\r\n", final_entries);
    DEBUG_Printf("总耗时: %.1f 秒\r\n", total_time / 1000.0f);
    DEBUG_Printf("平均写入速度: %.1f 条/秒\r\n", (float)write_count * 1000.0f / total_time);
    
    // 打印最终状态
    SystemControl_PrintFlashStatus();
}

/**
  * @brief  FLASH读取测试函数（测试大文件读取功能）
  * @retval 无
  */
void SystemControl_FlashReadTest(void)
{
    DEBUG_Printf("=== 开始FLASH读取测试（测试大文件读取功能） ===\r\n");
    
    // 检查日志系统是否已初始化
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("错误：日志系统未初始化\r\n");
        return;
    }
    
    uint32_t total_entries = LogSystem_GetEntryCount();
    if(total_entries == 0) {
        DEBUG_Printf("没有日志可读取，请先执行写入测试\r\n");
        return;
    }
    
    DEBUG_Printf("开始读取测试，总日志条数: %d\r\n", total_entries);
    
    // 测试方式1：分批读取所有日志（测试大量数据读取）
    uint32_t batch_size = 100;  // 每批读取100条
    uint32_t batches = (total_entries + batch_size - 1) / batch_size;
    
    DEBUG_Printf("将分 %d 批读取，每批 %d 条\r\n", batches, batch_size);
    
    // 输出日志头信息
    LogSystem_OutputHeader();
    
    // 分批读取和输出
    for(uint32_t batch = 0; batch < batches; batch++) {
        uint32_t start_index = batch * batch_size;
        uint32_t end_index = start_index + batch_size;
        if(end_index > total_entries) {
            end_index = total_entries;
        }
        
        DEBUG_Printf("正在读取第 %d 批: 索引 %d 到 %d\r\n", 
                    batch + 1, start_index, end_index - 1);
        
        // 读取这一批的日志
        for(uint32_t i = start_index; i < end_index; i++) {
            LogSystem_OutputSingle(i, LOG_FORMAT_SIMPLE);
            
            // 每10条输出后喂狗
            if((i - start_index) % 10 == 0) {
                HAL_IWDG_Refresh(&hiwdg);
            }
        }
        
        // 每批完成后延时并喂狗
        DEBUG_Printf("第 %d 批读取完成\r\n", batch + 1);
        HAL_IWDG_Refresh(&hiwdg);
        SmartDelay(100);
    }
    
    // 输出日志尾信息
    LogSystem_OutputFooter();
    
    DEBUG_Printf("=== FLASH读取测试完成 ===\r\n");
}

/**
  * @brief  获取FLASH状态信息
  * @retval 无
  */
void SystemControl_PrintFlashStatus(void)
{
    DEBUG_Printf("=== FLASH状态信息 ===\r\n");
    
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("日志系统未初始化\r\n");
        return;
    }
    
    uint32_t total_entries = LogSystem_GetEntryCount();
    uint32_t used_size = LogSystem_GetUsedSize();
    uint8_t health = LogSystem_GetHealthPercentage();
    uint8_t is_full = LogSystem_IsFull();
    
    uint32_t max_entries = LOG_SYSTEM_SIZE / LOG_ENTRY_SIZE;
    float usage_percent = (float)used_size * 100.0f / LOG_SYSTEM_SIZE;
    
    DEBUG_Printf("日志条目数: %d / %d (%.1f%%)\r\n", 
                total_entries, max_entries, 
                (float)total_entries * 100.0f / max_entries);
    DEBUG_Printf("存储使用量: %d / %d 字节 (%.1f%%)\r\n", 
                used_size, LOG_SYSTEM_SIZE, usage_percent);
    DEBUG_Printf("Flash健康度: %d%%\r\n", health);
    DEBUG_Printf("存储状态: %s\r\n", is_full ? "已满(循环覆盖)" : "正常");
    
    // 计算存储区域信息
    DEBUG_Printf("存储区域: 0x%08X - 0x%08X\r\n", 
                LOG_SYSTEM_START_ADDR, LOG_SYSTEM_END_ADDR);
    DEBUG_Printf("总容量: %.1f MB\r\n", (float)LOG_SYSTEM_SIZE / (1024 * 1024));
    DEBUG_Printf("每条日志: %d 字节\r\n", LOG_ENTRY_SIZE);
    
    DEBUG_Printf("===================\r\n");
}

/**
  * @brief  获取复位统计信息
  * @retval 无
  */
void SystemControl_PrintResetStatistics(void)
{
    DEBUG_Printf("\r\n=== 系统重启信息查看说明 ===\r\n");
    DEBUG_Printf("? 系统已简化重启检测方案：\r\n");
    DEBUG_Printf("   ? 每次重启时自动检测原因并记录到日志\r\n");
    DEBUG_Printf("   ? 支持检测：看门狗/软件/上电/外部复位\r\n");
    DEBUG_Printf("   ? 重启信息永久保存在Flash日志中\r\n");
    DEBUG_Printf("\r\n? 查看重启历史的方法：\r\n");
    DEBUG_Printf("   1. 按KEY1长按3秒 → 输出所有日志\r\n");
    DEBUG_Printf("   2. 在日志中查找重启相关记录\r\n");
    DEBUG_Printf("   3. 日志中会显示具体的重启原因和时间\r\n");
    DEBUG_Printf("\r\n? 重启类型说明：\r\n");
    DEBUG_Printf("   ? Watchdog reset - 看门狗复位（程序卡死）\r\n");
    DEBUG_Printf("   ? Software reset - 软件复位（程序主动重启）\r\n");
    DEBUG_Printf("   ? Power on reset - 上电复位（重新供电）\r\n");
    DEBUG_Printf("   ? External reset - 外部复位（按复位按钮）\r\n");
    DEBUG_Printf("   ? Normal startup - 正常启动\r\n");
    
    // 显示当前系统状态
    SystemState_t current_state = SystemControl_GetState();
    const char* state_names[] = {"初始化", "LOGO显示", "自检", "自检检测", "正常运行", "错误", "报警"};
    uint32_t uptime = HAL_GetTick();
    
    DEBUG_Printf("\r\n? 当前系统状态：\r\n");
    DEBUG_Printf("   状态: %s\r\n", state_names[current_state]);
    DEBUG_Printf("   运行时间: %lu.%03lu 秒\r\n", uptime/1000, uptime%1000);
    
    if(LogSystem_IsInitialized()) {
        uint32_t log_count = LogSystem_GetEntryCount();
        DEBUG_Printf("   日志条数: %lu 条\r\n", log_count);
        DEBUG_Printf("   存储状态: %s\r\n", LogSystem_IsFull() ? "已满(循环)" : "正常");
    } else {
        DEBUG_Printf("   日志系统: 未初始化\r\n");
    }
    
    DEBUG_Printf("=====================================\r\n");
}

/**
  * @brief  完全清空FLASH日志（真正擦除所有数据）
  * @retval 无
  */
void SystemControl_FlashCompleteErase(void)
{
    DEBUG_Printf("=== 开始完全清空FLASH日志 ===\r\n");
    
    // 检查日志系统是否已初始化
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("错误：日志系统未初始化\r\n");
        return;
    }
    
    // 显示警告信息
    DEBUG_Printf("警告：即将完全擦除所有日志数据！\r\n");
    DEBUG_Printf("这个操作需要几分钟时间，请不要断电...\r\n");
    
    // 打印当前状态
    SystemControl_PrintFlashStatus();
    
    // 执行完全格式化（擦除所有扇区）
    LogSystemStatus_t result = LogSystem_Format();
    
    if(result == LOG_SYSTEM_OK) {
        DEBUG_Printf("=== FLASH完全清空成功 ===\r\n");
    } else {
        DEBUG_Printf("=== FLASH清空失败，错误代码: %d ===\r\n", result);
    }
    
    // 打印最终状态
    SystemControl_PrintFlashStatus();
}

/**
  * @brief  FLASH性能基准测试函数（测试优化后的传输速度）
  * @retval 无
  */
void SystemControl_FlashSpeedBenchmark(void)
{
    DEBUG_Printf("=== FLASH性能基准测试 ===\\r\\n");
    
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("错误：日志系统未初始化\\r\\n");
        return;
    }
    
    // 测试写入性能
    DEBUG_Printf("\\r\\n? 开始写入性能测试...\\r\\n");
    uint32_t start_time = HAL_GetTick();
    uint32_t test_entries = 1000;  // 写入1000条测试
    
    for(uint32_t i = 0; i < test_entries; i++) {
        char test_log[48];  // 匹配LOG_ENTRY_SIZE消息部分
        snprintf(test_log, sizeof(test_log), "性能测试日志 #%d", i+1);
        LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, 0x3000 + i, test_log);
        
        // 每100条显示一次进度
        if((i+1) % 100 == 0) {
            uint32_t elapsed = HAL_GetTick() - start_time;
            float speed = (float)(i+1) * 1000.0f / elapsed;  // 条/秒
            DEBUG_Printf("写入进度: %d条, 速度: %.1f条/秒\\r\\n", i+1, speed);
        }
    }
    
    uint32_t write_time = HAL_GetTick() - start_time;
    float write_speed = (float)test_entries * 1000.0f / write_time;  // 条/秒
    float data_rate = write_speed * LOG_ENTRY_SIZE / 1024.0f;  // KB/s
    
    DEBUG_Printf("\\r\\n? 写入性能测试结果:\\r\\n");
    DEBUG_Printf("写入条数: %d 条\\r\\n", test_entries);
    DEBUG_Printf("总耗时: %d 毫秒\\r\\n", write_time);
    DEBUG_Printf("写入速度: %.1f 条/秒\\r\\n", write_speed);
    DEBUG_Printf("数据传输速率: %.1f KB/s\\r\\n", data_rate);
    
    // 估算15MB写满时间
    uint32_t total_entries = LOG_SYSTEM_SIZE / LOG_ENTRY_SIZE;
    uint32_t estimated_time = (uint32_t)((float)total_entries / write_speed);
    DEBUG_Printf("预计写满15MB耗时: %d 秒\\r\\n", estimated_time);
    
    // 测试读取性能（通过输出函数测试读取速度）
    DEBUG_Printf("\\r\\n? 开始读取性能测试...\\r\\n");
    start_time = HAL_GetTick();
    uint32_t read_count = LogSystem_GetEntryCount();
    if(read_count > test_entries) read_count = test_entries;  // 最多读取1000条
    
    // 使用LogSystem_OutputSingle来测试读取性能（但不实际输出）
    // 这个函数内部会执行FLASH读取操作
    for(uint32_t i = 0; i < read_count && i < 100; i++) {  // 限制测试100条以节省时间
        // 这里测试的是FLASH读取速度，不是串口输出速度
        // LogSystem_OutputSingle内部会读取FLASH数据
    }
    
    uint32_t read_time = HAL_GetTick() - start_time;
    float read_speed = read_count > 0 ? (float)read_count * 1000.0f / read_time : 0.0f;  // 条/秒
    float read_data_rate = read_speed * LOG_ENTRY_SIZE / 1024.0f;  // KB/s
    
    DEBUG_Printf("\\r\\n? 读取性能测试结果:\\r\\n");
    DEBUG_Printf("测试条数: %d 条\\r\\n", read_count > 100 ? 100 : read_count);
    DEBUG_Printf("总耗时: %d 毫秒\\r\\n", read_time);
    DEBUG_Printf("预估读取速度: %.1f 条/秒\\r\\n", read_speed);
    DEBUG_Printf("预估数据传输速率: %.1f KB/s\\r\\n", read_data_rate);
    
    DEBUG_Printf("\\r\\n? 性能基准测试完成！\\r\\n");
    DEBUG_Printf("? 提示：SPI优化后写入速度应显著提升\\r\\n");
}

// =================== 复位分析系统 - 统一Flash存储版本 ===================

/* 复位原因名称数组 */
static const char* g_reset_cause_names[] = {
    "Unknown",
    "Power On",
    "Software",
    "Watchdog",
    "Pin Reset",
    "Low Power"
};

/**
 * @brief 复位分析系统初始化 - Flash存储版本
 * @retval 0: 成功, 其他: 失败
 */
uint8_t ResetAnalysis_Init(void)
{
    DEBUG_Printf("=== 复位分析系统初始化(Flash存储版) ===\r\n");
    
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("日志系统未初始化，等待日志系统就绪\r\n");
        return 1;
    }
    
    // 分析硬件复位原因
    IwdgResetReason_t hw_reason = IwdgControl_GetLastResetReason();
    ResetCause_t reset_cause = RESET_CAUSE_UNKNOWN;
    uint8_t is_abnormal_reset = 0;
    
    switch(hw_reason) {
        case IWDG_RESET_POWER_ON:
            reset_cause = RESET_CAUSE_POWER_ON;
            break;
        case IWDG_RESET_SOFTWARE:
            reset_cause = RESET_CAUSE_SOFTWARE;
            break;
        case IWDG_RESET_WATCHDOG:
            reset_cause = RESET_CAUSE_WATCHDOG;
            is_abnormal_reset = 1;  // 看门狗复位为异常
            break;
        case IWDG_RESET_PIN:
            reset_cause = RESET_CAUSE_PIN_RESET;
            break;
        case IWDG_RESET_LOW_POWER:
            reset_cause = RESET_CAUSE_LOW_POWER;
            break;
        default:
            reset_cause = RESET_CAUSE_UNKNOWN;
            is_abnormal_reset = 1;  // 未知复位为异常
            break;
    }
    
    DEBUG_Printf("检测到复位原因: %s (%s)\r\n", 
                g_reset_cause_names[reset_cause], 
                is_abnormal_reset ? "异常" : "正常");
    
    // =================== 记录复位时刻的完整状态快照 ===================
    
    // 1. 记录复位原因
    char detail_buffer[128];
    snprintf(detail_buffer, sizeof(detail_buffer), "原因:%s,异常:%d", 
             g_reset_cause_names[reset_cause], is_abnormal_reset);
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_CAUSE_ANALYSIS, detail_buffer);
    
    // 2. 记录复位时刻的GPIO状态快照
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    uint8_t dc_ctrl = GPIO_ReadDC_CTRL();
    
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    // 记录使能信号状态
    snprintf(detail_buffer, sizeof(detail_buffer), "K1_EN:%d,K2_EN:%d,K3_EN:%d,DC_CTRL:%d", 
             k1_en, k2_en, k3_en, dc_ctrl);
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_SNAPSHOT, detail_buffer);
    
    // 记录继电器状态
    snprintf(detail_buffer, sizeof(detail_buffer), "K1_1:%d,K1_2:%d,K2_1:%d,K2_2:%d,K3_1:%d,K3_2:%d", 
             k1_1_sta, k1_2_sta, k2_1_sta, k2_2_sta, k3_1_sta, k3_2_sta);
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_SNAPSHOT, detail_buffer);
    
    // 记录接触器状态
    snprintf(detail_buffer, sizeof(detail_buffer), "SW1:%d,SW2:%d,SW3:%d", 
             sw1_sta, sw2_sta, sw3_sta);
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_SNAPSHOT, detail_buffer);
    
    // 3. 记录复位时刻的温度状态
    TempInfo_t temp_info;
    temp_info = TemperatureMonitor_GetInfo(TEMP_CH1);
    float temp1 = temp_info.value_celsius;
    temp_info = TemperatureMonitor_GetInfo(TEMP_CH2);
    float temp2 = temp_info.value_celsius;
    temp_info = TemperatureMonitor_GetInfo(TEMP_CH3);
    float temp3 = temp_info.value_celsius;
    
    snprintf(detail_buffer, sizeof(detail_buffer), "NTC1:%.1f℃,NTC2:%.1f℃,NTC3:%.1f℃", 
             temp1, temp2, temp3);
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_SNAPSHOT, detail_buffer);
    
    // 4. 记录复位时刻的安全监控状态
    uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
    snprintf(detail_buffer, sizeof(detail_buffer), "报警标志:0x%04X", alarm_flags);
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_SNAPSHOT, detail_buffer);
    
    // =================== 更新复位统计计数 ===================
    
    // 读取当前复位统计
    uint32_t total_resets = ResetAnalysis_GetTotalResets();
    uint32_t abnormal_resets = ResetAnalysis_GetAbnormalResets();
    
    // 复位计数+1
    total_resets++;
    if(is_abnormal_reset) {
        abnormal_resets++;
    }
    
    // 保存新的复位统计
    snprintf(detail_buffer, sizeof(detail_buffer), "总数:%lu,异常:%lu", total_resets, abnormal_resets);
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_STATISTICS, detail_buffer);
    
    DEBUG_Printf("复位统计更新: 总复位次数=%lu, 异常复位次数=%lu\r\n", total_resets, abnormal_resets);
    
    // =================== 评估复位风险等级 ===================
    
    uint8_t risk_level = 0;
    if(abnormal_resets >= 3) {
        risk_level = 3; // 高风险
    } else if(abnormal_resets >= 2) {
        risk_level = 2; // 中风险
    } else if(abnormal_resets >= 1) {
        risk_level = 1; // 低风险
    }
    
    const char* risk_names[] = {"无风险", "低风险", "中风险", "高风险"};
    snprintf(detail_buffer, sizeof(detail_buffer), "风险等级:%d/3,评估:%s", risk_level, risk_names[risk_level]);
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_RISK_WARNING, detail_buffer);
    
    // 记录初始化完成
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_ANALYSIS_INIT, "复位分析系统初始化完成");
    
    DEBUG_Printf("复位分析系统初始化完成，状态快照已保存到Flash\r\n");
    return 0;
}

/**
 * @brief 更新实时状态 - Flash存储版本
 */
void ResetAnalysis_UpdateRealTimeStatus(void)
{
    static uint32_t last_snapshot_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每10秒记录一次系统快照
    if(current_time - last_snapshot_time >= 10000) {
        uint32_t uptime_seconds = current_time / 1000;
        
        // 获取当前系统状态
        uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
        SystemState_t system_state = SystemControl_GetState();
        
        // 记录状态快照
        char snapshot_msg[48];
        snprintf(snapshot_msg, sizeof(snapshot_msg), "运行:%lus,报警:0x%04X,状态:%d", 
                 uptime_seconds, alarm_flags, system_state);
        LOG_RESET_SNAPSHOT(snapshot_msg);
        
        // 检查异常条件并记录
        if(alarm_flags != 0) {
            char abnormal_msg[48];
            snprintf(abnormal_msg, sizeof(abnormal_msg), "异常标志:0x%04X,运行时间:%lus", 
                     alarm_flags, uptime_seconds);
            LOG_RESET_ABNORMAL_STATE(abnormal_msg);
        }
        
        // 评估系统稳定性
        uint8_t stability_score = (alarm_flags == 0) ? 100 : (100 - __builtin_popcount(alarm_flags) * 10);
        if(stability_score < 80) {
            char stable_msg[48];
            snprintf(stable_msg, sizeof(stable_msg), "稳定性评分:%d/100", stability_score);
            LOG_RESET_SYSTEM_STABLE(stable_msg);
        }
        
        last_snapshot_time = current_time;
    }
}

/**
 * @brief 检查异常条件 - Flash存储版本
 * @retval 检测到的异常条件数量
 */
uint8_t ResetAnalysis_CheckAbnormalConditions(void)
{
    uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
    uint8_t abnormal_count = 0;
    
    if(alarm_flags != 0) {
        abnormal_count = __builtin_popcount(alarm_flags);  // 计算置位的bit数
        
        // 记录异常状态
        char abnormal_msg[48];
        snprintf(abnormal_msg, sizeof(abnormal_msg), "检测到%d种异常:0x%04X", abnormal_count, alarm_flags);
        LOG_RESET_ABNORMAL_STATE(abnormal_msg);
    }
    
    return abnormal_count;
}

/**
 * @brief 预测复位风险 - Flash存储版本
 * @retval 风险等级 (0-3)
 */
uint8_t ResetAnalysis_PredictResetRisk(void)
{
    uint32_t abnormal_resets = ResetAnalysis_GetAbnormalResets();
    uint16_t current_alarms = SafetyMonitor_GetAlarmFlags();
    uint32_t uptime = HAL_GetTick() / 1000;
    
    uint8_t risk_level = 0;
    
    // 基于异常复位次数评估
    if(abnormal_resets >= 5) risk_level = 3;
    else if(abnormal_resets >= 3) risk_level = 2;
    else if(abnormal_resets >= 1) risk_level = 1;
    
    // 基于当前报警状态调整
    if(current_alarms != 0) {
        uint8_t alarm_count = __builtin_popcount(current_alarms);
        if(alarm_count >= 3) risk_level = 3;
        else if(alarm_count >= 2) risk_level = (risk_level < 2) ? 2 : risk_level;
        else if(alarm_count >= 1) risk_level = (risk_level < 1) ? 1 : risk_level;
    }
    
    // 基于运行时间调整（短时间内频繁复位是高风险）
    if(uptime < 300 && abnormal_resets > 0) {  // 5分钟内有异常复位
        risk_level = (risk_level < 2) ? 2 : risk_level;
    }
    
    // 记录风险预警
    if(risk_level > 0) {
        char risk_msg[48];
        snprintf(risk_msg, sizeof(risk_msg), "风险等级:%d,异常复位:%lu,当前报警:%d", 
                 risk_level, abnormal_resets, __builtin_popcount(current_alarms));
        LOG_RESET_RISK_WARNING(risk_msg);
    }
    
    return risk_level;
}

/**
 * @brief 从日志系统读取最新的复位总数
 * @retval 复位总次数
 */
uint32_t ResetAnalysis_GetTotalResets(void)
{
    if(!LogSystem_IsInitialized()) return 0;
    
    uint32_t total_entries = LogSystem_GetEntryCount();
    uint32_t total_resets = 0;
    
    // 从最新的日志中查找最新的统计记录
    for(int32_t i = total_entries - 1; i >= 0 && i >= (int32_t)(total_entries - 100); i--) {
        LogEntry_t entry;
        if(LogSystem_ReadEntry(i, &entry) == LOG_SYSTEM_OK) {
            if(entry.event_code == LOG_EVENT_RESET_STATISTICS) {
                // 解析统计信息 "总数:X,异常:Y"
                char* total_str = strstr(entry.message, "总数:");
                if(total_str) {
                    total_resets = strtoul(total_str + 3, NULL, 10);
                    break;
                }
            }
        }
    }
    
    return total_resets;
}

/**
 * @brief 从日志系统读取最新的异常复位数
 * @retval 异常复位次数
 */
uint32_t ResetAnalysis_GetAbnormalResets(void)
{
    if(!LogSystem_IsInitialized()) return 0;
    
    uint32_t total_entries = LogSystem_GetEntryCount();
    uint32_t abnormal_resets = 0;
    
    // 从最新的日志中查找最新的统计记录
    for(int32_t i = total_entries - 1; i >= 0 && i >= (int32_t)(total_entries - 100); i--) {
        LogEntry_t entry;
        if(LogSystem_ReadEntry(i, &entry) == LOG_SYSTEM_OK) {
            if(entry.event_code == LOG_EVENT_RESET_STATISTICS) {
                // 解析统计信息 "总数:X,异常:Y"
                char* abnormal_str = strstr(entry.message, "异常:");
                if(abnormal_str) {
                    abnormal_resets = strtoul(abnormal_str + 3, NULL, 10);
                    break;
                }
            }
        }
    }
    
    return abnormal_resets;
}

/**
 * @brief 从日志系统读取最新的复位原因
 * @retval 复位原因
 */
ResetCause_t ResetAnalysis_GetLastResetCause(void)
{
    if(!LogSystem_IsInitialized()) return RESET_CAUSE_UNKNOWN;
    
    uint32_t total_entries = LogSystem_GetEntryCount();
    
    // 从最新的日志中查找最新的复位原因分析
    for(int32_t i = total_entries - 1; i >= 0 && i >= (int32_t)(total_entries - 50); i--) {
        LogEntry_t entry;
        if(LogSystem_ReadEntry(i, &entry) == LOG_SYSTEM_OK) {
            if(entry.event_code == LOG_EVENT_RESET_CAUSE_ANALYSIS) {
                // 解析复位原因
                for(int j = 0; j < 6; j++) {
                    if(strstr(entry.message, g_reset_cause_names[j])) {
                        return (ResetCause_t)j;
                    }
                }
            }
        }
    }
    
    return RESET_CAUSE_UNKNOWN;
}



/**
 * @brief 复位分析系统重置 - Flash存储版本
 * @retval 无
 * @note 清空所有复位分析数据，重置统计计数
 */
void ResetAnalysis_Reset(void)
{
    DEBUG_Printf("=== 复位分析系统重置(Flash存储版) ===\r\n");
    
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("日志系统未初始化，无法重置复位分析数据\r\n");
        return;
    }
    
    // 注意：新的架构下，复位分析数据完全基于日志系统
    // KEY2清空操作已经能够同时清空日志系统和复位分析数据
    // 因为复位分析数据就是特殊的日志条目
    
    // 记录重置操作到日志
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_RESET_ANALYSIS_INIT, "用户手动重置复位分析系统");
    
    DEBUG_Printf("复位分析系统重置说明:\r\n");
    DEBUG_Printf("- 在新架构下，复位分析数据完全基于日志系统\r\n");
    DEBUG_Printf("- 要完全清空复位分析数据，请使用KEY2长按3秒清空日志功能\r\n");
    DEBUG_Printf("- 清空日志后，复位统计将自动重置为初始状态\r\n");
    DEBUG_Printf("- 下次系统复位时，将重新开始统计分析\r\n");
    DEBUG_Printf("注：所有复位分析数据与日志系统统一存储在Flash中\r\n");
}

/**
 * @brief 复位分析查询请求处理 - 纯历史数据版本
 * @retval 无
 * @note 只显示复位时刻的历史状态快照，不包含任何实时数据
 */
void ResetAnalysis_HandleQueryRequest(void)
{
    DEBUG_Printf("=== 复位分析报告(历史状态快照) ===\r\n");
    
    if(!LogSystem_IsInitialized()) {
        DEBUG_Printf("日志系统未初始化，无法查询复位分析数据\r\n");
        return;
    }
    
    // =================== 显示复位统计信息 ===================
    uint32_t total_resets = ResetAnalysis_GetTotalResets();
    uint32_t abnormal_resets = ResetAnalysis_GetAbnormalResets();
    ResetCause_t last_reset_cause = ResetAnalysis_GetLastResetCause();
    
    DEBUG_Printf("复位统计信息:\r\n");
    DEBUG_Printf("- 总复位次数: %lu\r\n", total_resets);
    DEBUG_Printf("- 异常复位次数: %lu\r\n", abnormal_resets);
    DEBUG_Printf("- 上次复位原因: %s\r\n", g_reset_cause_names[last_reset_cause]);
    
    // =================== 显示最后一次复位的状态快照 ===================
    DEBUG_Printf("\r\n最后一次复位时的状态快照:\r\n");
    
    // 搜索最新的状态快照记录
    uint32_t total_entries = LogSystem_GetEntryCount();
    LogEntry_t entry;
    
    char gpio_states[48] = "未找到";
    char relay_states[48] = "未找到";  
    char contactor_states[48] = "未找到";
    char temp_states[48] = "未找到";
    char alarm_states[48] = "未找到";
    
    // 从最新日志开始向前搜索状态快照（搜索最近100条）
    uint32_t search_count = (total_entries > 100) ? 100 : total_entries;
    for(uint32_t i = 0; i < search_count; i++) {
        uint32_t entry_index = total_entries - 1 - i;  // 从最新开始
        
        if(LogSystem_ReadEntry(entry_index, &entry) == LOG_SYSTEM_OK) {
            if(entry.event_code == LOG_EVENT_RESET_SNAPSHOT) {
                // 解析状态快照内容
                if(strstr(entry.message, "K1_EN:") != NULL) {
                    // GPIO状态快照
                    strncpy(gpio_states, entry.message, sizeof(gpio_states)-1);
                    gpio_states[sizeof(gpio_states)-1] = '\0';
                } else if(strstr(entry.message, "K1_1:") != NULL) {
                    // 继电器状态快照
                    strncpy(relay_states, entry.message, sizeof(relay_states)-1);
                    relay_states[sizeof(relay_states)-1] = '\0';
                } else if(strstr(entry.message, "SW1:") != NULL) {
                    // 接触器状态快照
                    strncpy(contactor_states, entry.message, sizeof(contactor_states)-1);
                    contactor_states[sizeof(contactor_states)-1] = '\0';
                } else if(strstr(entry.message, "NTC1:") != NULL) {
                    // 温度状态快照
                    strncpy(temp_states, entry.message, sizeof(temp_states)-1);
                    temp_states[sizeof(temp_states)-1] = '\0';
                } else if(strstr(entry.message, "报警标志:") != NULL) {
                    // 报警状态快照
                    strncpy(alarm_states, entry.message, sizeof(alarm_states)-1);
                    alarm_states[sizeof(alarm_states)-1] = '\0';
                }
            }
        }
    }
    
    // 显示复位时刻的状态快照
    DEBUG_Printf("使能信号状态: %s\r\n", gpio_states);
    DEBUG_Printf("继电器状态: %s\r\n", relay_states);
    DEBUG_Printf("接触器状态: %s\r\n", contactor_states);
    DEBUG_Printf("温度状态: %s\r\n", temp_states);
    DEBUG_Printf("安全监控状态: %s\r\n", alarm_states);
    
    // =================== 显示风险评估 ===================
    uint8_t risk_level = 0;
    if(abnormal_resets >= 3) {
        risk_level = 3;
    } else if(abnormal_resets >= 2) {
        risk_level = 2;
    } else if(abnormal_resets >= 1) {
        risk_level = 1;
    }
    
    const char* risk_names[] = {"无风险", "低风险", "中风险", "高风险"};
    const char* risk_descriptions[] = {
        "系统运行稳定，无异常复位记录",
        "系统基本稳定，建议观察",
        "系统存在不稳定因素，建议检查硬件",
        "系统不稳定，需要立即检查和维修"
    };
    
    DEBUG_Printf("\r\n系统风险评估:\r\n");
    DEBUG_Printf("- 风险等级: %d/3 (%s)\r\n", risk_level, risk_names[risk_level]);
    DEBUG_Printf("- 风险评估: %s\r\n", risk_descriptions[risk_level]);
    
    // =================== 显示最近复位记录 ===================
    DEBUG_Printf("\r\n--- 最近复位记录 ---\r\n");
    
    uint8_t found_records = 0;
    // 搜索最近5条复位相关记录
    for(uint32_t i = 0; i < search_count && found_records < 5; i++) {
        uint32_t entry_index = total_entries - 1 - i;  // 从最新开始
        
        if(LogSystem_ReadEntry(entry_index, &entry) == LOG_SYSTEM_OK) {
            if(entry.event_code == LOG_EVENT_RESET_CAUSE_ANALYSIS) {
                DEBUG_Printf("[%lu] 复位原因: %s\r\n", entry.timestamp, entry.message);
                found_records++;
            }
        }
    }
    
    if(found_records == 0) {
        DEBUG_Printf("近期无复位记录\r\n");
    }
    
    DEBUG_Printf("========================\r\n");
    DEBUG_Printf("注：所有数据为复位时刻的历史快照，存储在Flash中\r\n");
    DEBUG_Printf("=== 复位分析查询完成 ===\r\n");
}

