#include "system_control.h"
#include "relay_control.h"
#include "temperature_monitor.h"
#include "oled_display.h"
#include "gpio_control.h"
#include "safety_monitor.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

/************************************************************
 * 系统控制模块源文件
 * 实现系统自检、主循环调度、状态机管理
 * 严格按照README要求：2秒LOGO → 3秒自检进度条 → 检测 → 主循环
 ************************************************************/

// 全局系统控制结构体
static SystemControl_t g_system_control;

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
                // 更新进度条
                uint8_t percent = (elapsed_time * 100) / SELF_TEST_TIME_MS;
                OLED_ShowSelfTestBar(percent);
                DEBUG_Printf("自检进度: %d%%\r\n", percent);
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
        HAL_Delay(1000);
        
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
        strcat(g_system_control.self_test.error_info, "期望状态识别异常;");
        g_system_control.self_test.overall_result = 0;
    }
    HAL_Delay(500); // 每步之间停顿500ms，让用户看到进度
    
    // 第二步：继电器状态检查与主动纠错（安全先关后开）
    SystemControl_UpdateSelfTestProgress(2, 50);
    g_system_control.self_test.step2_relay_correction_ok = SystemControl_SelfTest_Step2_RelayCorrection();
    if(!g_system_control.self_test.step2_relay_correction_ok) {
        strcat(g_system_control.self_test.error_info, "继电器纠错异常;");
        g_system_control.self_test.overall_result = 0;
    }
    HAL_Delay(500);
    
    // 第三步：接触器状态检查与报错（进度75%）
    SystemControl_UpdateSelfTestProgress(3, 75);
    g_system_control.self_test.step3_contactor_check_ok = SystemControl_SelfTest_Step3_ContactorCheck();
    if(!g_system_control.self_test.step3_contactor_check_ok) {
        strcat(g_system_control.self_test.error_info, "接触器检查异常;");
        g_system_control.self_test.overall_result = 0;
    }
    HAL_Delay(500);
    
    // 第四步：温度安全检测（进度100%）
    SystemControl_UpdateSelfTestProgress(4, 100);
    g_system_control.self_test.step4_temperature_safety_ok = SystemControl_SelfTest_Step4_TemperatureSafety();
    if(!g_system_control.self_test.step4_temperature_safety_ok) {
        strcat(g_system_control.self_test.error_info, "温度安全检测异常;");
        g_system_control.self_test.overall_result = 0;
    }
    HAL_Delay(500);
    
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
    static uint32_t lastTempTime = 0;
    static uint32_t lastOledTime = 0;
    static uint32_t lastSafetyTime = 0;
    static uint32_t lastDiagTime = 0;
    static uint32_t lastFanSpeedTime = 0;  // 添加风扇转速统计定时器
    
    uint32_t currentTime = HAL_GetTick();
    
    // 每10ms处理继电器待处理动作（最高优先级）
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
                    
                    // 格式：异常标志+描述，例如："A异常:使能冲突" 或 "K异常:NTC1超温"
                    char alarm_letter = 'A' + active_alarm;  // 转换为字母
                    snprintf(alarm_info, sizeof(alarm_info), "%c异常:%s", alarm_letter, 
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
    
    if(error_msg) {
        strncpy(g_system_control.self_test.error_info, error_msg, 
                sizeof(g_system_control.self_test.error_info) - 1);
    }
    
    DEBUG_Printf("=== 系统进入错误状态: %s ===\r\n", error_msg ? error_msg : "未知错误");
    
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
    
    // 使用安全监控模块设置N异常标志
    SafetyMonitor_SetAlarmFlag(ALARM_FLAG_N, g_system_control.self_test.error_info);
    
    // 进入错误状态
    SystemControl_EnterErrorState("自检失败");
    
    // 重置显示缓存，确保异常信息能正确显示
    OLED_ResetDisplayCache();
    
    // 在OLED上显示自检异常信息
    OLED_Clear();
    // 这里可以显示具体的异常信息，格式如："N异常:具体原因"
    char display_msg[64];
    sprintf(display_msg, "N异常:%s", g_system_control.self_test.error_info);
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
    g_system_control.last_state = g_system_control.current_state;
    g_system_control.current_state = new_state;
    g_system_control.state_start_time = HAL_GetTick();
    
    DEBUG_Printf("系统状态切换: %d -> %d\r\n", g_system_control.last_state, new_state);
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
    
    // 显示进度条
    OLED_ShowSelfTestBar(percent);
    
    // 输出步骤信息
    const char* step_names[] = {
        "",
        "期望状态识别",
        "继电器纠错",
        "接触器检查", 
        "温度安全检测"
    };
    
    if(step >= 1 && step <= 4) {
        DEBUG_Printf("自检步骤%d：%s（%d%%）\r\n", step, step_names[step], percent);
    }
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
    uint8_t correction_needed = 0;
    
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
        correction_needed = 1;
        DEBUG_Printf("检测到继电器状态错误，开始安全纠错流程\\r\\n");
        
        // 第一阶段：安全关断所有不需要开启的通道
        DEBUG_Printf("[安全纠错] 第一阶段：关断所有不需要的通道\\r\\n");
        
        // 关断K1（如果目标不是开启K1）
        if(expected_k1_1 == 0 && (k1_1_sta == 1 || k1_2_sta == 1)) {
            DEBUG_Printf("[安全纠错] 关断Channel_1继电器\\r\\n");
            GPIO_SetK1_1_OFF(0); GPIO_SetK1_2_OFF(0);
            HAL_Delay(50);  // 短暂的信号建立时间
            GPIO_SetK1_1_OFF(1); GPIO_SetK1_2_OFF(1);
            HAL_Delay(500); // 等待继电器动作完成
        }
        
        // 关断K2（如果目标不是开启K2）
        if(expected_k2_1 == 0 && (k2_1_sta == 1 || k2_2_sta == 1)) {
            DEBUG_Printf("[安全纠错] 关断Channel_2继电器\\r\\n");
            GPIO_SetK2_1_OFF(0); GPIO_SetK2_2_OFF(0);
            HAL_Delay(50);
            GPIO_SetK2_1_OFF(1); GPIO_SetK2_2_OFF(1);
            HAL_Delay(500);
        }
        
        // 关断K3（如果目标不是开启K3）
        if(expected_k3_1 == 0 && (k3_1_sta == 1 || k3_2_sta == 1)) {
            DEBUG_Printf("[安全纠错] 关断Channel_3继电器\\r\\n");
            GPIO_SetK3_1_OFF(0); GPIO_SetK3_2_OFF(0);
            HAL_Delay(50);
            GPIO_SetK3_1_OFF(1); GPIO_SetK3_2_OFF(1);
            HAL_Delay(500);
        }
        
        // 第二阶段：开启目标通道
        DEBUG_Printf("[安全纠错] 第二阶段：开启目标通道\\r\\n");
        
        // 开启K1（如果目标是开启K1）
        if(expected_k1_1 == 1) {
            DEBUG_Printf("[安全纠错] 开启Channel_1继电器\\r\\n");
            GPIO_SetK1_1_ON(0); GPIO_SetK1_2_ON(0);
            HAL_Delay(50);
            GPIO_SetK1_1_ON(1); GPIO_SetK1_2_ON(1);
            HAL_Delay(500);
        }
        
        // 开启K2（如果目标是开启K2）
        if(expected_k2_1 == 1) {
            DEBUG_Printf("[安全纠错] 开启Channel_2继电器\\r\\n");
            GPIO_SetK2_1_ON(0); GPIO_SetK2_2_ON(0);
            HAL_Delay(50);
            GPIO_SetK2_1_ON(1); GPIO_SetK2_2_ON(1);
            HAL_Delay(500);
        }
        
        // 开启K3（如果目标是开启K3）
        if(expected_k3_1 == 1) {
            DEBUG_Printf("[安全纠错] 开启Channel_3继电器\\r\\n");
            GPIO_SetK3_1_ON(0); GPIO_SetK3_2_ON(0);
            HAL_Delay(50);
            GPIO_SetK3_1_ON(1); GPIO_SetK3_2_ON(1);
            HAL_Delay(500);
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

