#include "system_control.h"
#include "relay_control.h"
#include "temperature_monitor.h"
#include "oled_display.h"
#include "gpio_control.h"
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
    
    DEBUG_Printf("开始自检阶段，持续时间: %d秒\r\n", SELF_TEST_TIME_MS/1000);
}

/**
  * @brief  执行自检检测
  * @retval 无
  */
void SystemControl_ExecuteSelfTest(void)
{
    DEBUG_Printf("=== 开始系统自检检测 ===\r\n");
    
    // 初始化自检结果
    g_system_control.self_test.overall_result = 1;
    memset(g_system_control.self_test.error_info, 0, sizeof(g_system_control.self_test.error_info));
    
    // 1. 检测三路输入信号
    g_system_control.self_test.input_signals_ok = SystemControl_CheckInputSignals();
    if(!g_system_control.self_test.input_signals_ok) {
        strcat(g_system_control.self_test.error_info, "输入信号异常;");
        g_system_control.self_test.overall_result = 0;
    }
    
    // 2. 检测状态反馈信号
    g_system_control.self_test.feedback_signals_ok = SystemControl_CheckFeedbackSignals();
    if(!g_system_control.self_test.feedback_signals_ok) {
        strcat(g_system_control.self_test.error_info, "反馈信号异常;");
        g_system_control.self_test.overall_result = 0;
    }
    
    // 3. 检测NTC温度
    g_system_control.self_test.temperature_ok = SystemControl_CheckTemperature();
    if(!g_system_control.self_test.temperature_ok) {
        strcat(g_system_control.self_test.error_info, "温度异常;");
        g_system_control.self_test.overall_result = 0;
    }
    
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
  * @brief  主循环调度器（在正常运行状态调用）
  * @retval 无
  */
void SystemControl_MainLoopScheduler(void)
{
    static uint32_t lastRelayTime = 0;
    static uint32_t lastTempTime = 0;
    static uint32_t lastOledTime = 0;
    static uint32_t lastSafetyTime = 0;
    
    uint32_t currentTime = HAL_GetTick();
    
    // 每10ms处理继电器待处理动作（最高优先级）
    if(currentTime - lastRelayTime >= 10) {
        lastRelayTime = currentTime;
        RelayControl_ProcessPendingActions();
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
    
    // 每500ms执行安全监控模块（待3.2阶段实现）
    if(currentTime - lastSafetyTime >= 500) {
        lastSafetyTime = currentTime;
        // TODO: 3.2阶段 - SafetyMonitor_Process();
    }
}

/**
  * @brief  更新显示内容（在主循环调度器中调用）
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
        
        sprintf(temp_info, "T1:%.1f T2:%.1f T3:%.1f", 
               temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
        
        // 获取风扇信息
        uint8_t fan_pwm = TemperatureMonitor_GetFanPWM();
        FanSpeedInfo_t fan_speed = TemperatureMonitor_GetFanSpeed();
        sprintf(fan_info, "FAN:%d%% %dRPM", fan_pwm, (int)fan_speed.rpm);
        
        // 显示主界面
        OLED_ShowMainScreen(alarm_info, status1, status2, status3, temp_info, fan_info);
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
}

/**
  * @brief  处理自检异常（产生N异常标志）
  * @retval 无
  */
void SystemControl_HandleSelfTestError(void)
{
    DEBUG_Printf("=== 自检异常，产生N异常标志 ===\r\n");
    
    // 设置N异常标志
    g_system_control.error_flags |= (1 << 13); // N对应第13位
    
    // 进入错误状态
    SystemControl_EnterErrorState("自检失败");
    
    // 在OLED上显示自检异常信息
    OLED_Clear();
    // 这里可以显示具体的异常信息，格式如："N异常:具体原因"
    char display_msg[64];
    sprintf(display_msg, "N异常:%s", g_system_control.self_test.error_info);
    OLED_DrawString(0, 20, display_msg, 8); // 使用8x16字体显示
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

