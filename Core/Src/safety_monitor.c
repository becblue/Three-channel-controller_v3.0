#include "safety_monitor.h"
#include "gpio_control.h"
#include "relay_control.h"
#include "temperature_monitor.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

/************************************************************
 * 安全监控模块源文件
 * 实现异常标志管理、报警输出、蜂鸣器控制、电源监控
 * 严格按照README要求实现所有报警逻辑和解除条件
 ************************************************************/

// 全局安全监控结构体
static SafetyMonitor_t g_safety_monitor;

// 异常标志描述字符串（英文版本，适配OLED显示）
static const char* g_alarm_descriptions[ALARM_FLAG_COUNT] = {
    "EN Conflict",           // A - K1_EN/K2_EN/K3_EN使能冲突 (11字符)
    "K1_1_STA Error",        // B - K1_1_STA工作异常 (13字符)
    "K2_1_STA Error",        // C - K2_1_STA工作异常 (13字符)
    "K3_1_STA Error",        // D - K3_1_STA工作异常 (13字符)
    "K1_2_STA Error",        // E - K1_2_STA工作异常 (13字符)
    "K2_2_STA Error",        // F - K2_2_STA工作异常 (13字符)
    "K3_2_STA Error",        // G - K3_2_STA工作异常 (13字符)
    "SW1_STA Error",         // H - SW1_STA工作异常 (12字符)
    "SW2_STA Error",         // I - SW2_STA工作异常 (12字符)
    "SW3_STA Error",         // J - SW3_STA工作异常 (12字符)
    "NTC1 Overheat",         // K - NTC_1温度异常 (12字符)
    "NTC2 Overheat",         // L - NTC_2温度异常 (12字符)
    "NTC3 Overheat",         // M - NTC_3温度异常 (12字符)
    "Self-Test Fail",        // N - 自检异常 (13字符)
    "Power Failure"          // O - 外部电源异常 (12字符)
};

// 获取异常类型（根据异常标志确定报警类型）
static AlarmType_t SafetyMonitor_GetAlarmType(AlarmFlag_t flag);

// ================== 核心管理功能 ===================

/**
  * @brief  安全监控模块初始化
  * @retval 无
  */
void SafetyMonitor_Init(void)
{
    // 初始化安全监控结构体
    memset(&g_safety_monitor, 0, sizeof(SafetyMonitor_t));
    
    // 初始化异常信息
    for(uint8_t i = 0; i < ALARM_FLAG_COUNT; i++) {
        g_safety_monitor.alarm_info[i].is_active = 0;
        g_safety_monitor.alarm_info[i].type = SafetyMonitor_GetAlarmType((AlarmFlag_t)i);
        g_safety_monitor.alarm_info[i].trigger_time = 0;
        strncpy(g_safety_monitor.alarm_info[i].description, 
                g_alarm_descriptions[i], 
                sizeof(g_safety_monitor.alarm_info[i].description) - 1);
    }
    
    // 初始化报警输出和蜂鸣器状态
    g_safety_monitor.alarm_output_active = 0;
    g_safety_monitor.beep_state = BEEP_STATE_OFF;
    g_safety_monitor.beep_current_level = 0;
    g_safety_monitor.beep_last_toggle_time = HAL_GetTick();
    
    // 启用电源监控
    g_safety_monitor.power_monitor_enabled = 1;
    
    // 初始化硬件引脚状态
    GPIO_SetAlarmOutput(0);  // ALARM引脚初始化为高电平（正常状态）
    GPIO_SetBeepOutput(0);   // 蜂鸣器初始化为高电平（关闭状态）
    
    DEBUG_Printf("安全监控模块初始化完成\r\n");
}

/**
  * @brief  安全监控主处理函数（主循环中调用）
  * @retval 无
  */
void SafetyMonitor_Process(void)
{
    // 1. 检查并更新所有异常状态
    SafetyMonitor_UpdateAllAlarmStatus();
    
    // 2. 更新ALARM引脚输出
    SafetyMonitor_UpdateAlarmOutput();
    
    // 3. 更新蜂鸣器状态
    SafetyMonitor_UpdateBeepState();
    
    // 4. 处理蜂鸣器脉冲
    SafetyMonitor_ProcessBeep();
}

/**
  * @brief  获取当前异常状态（返回异常标志位图）
  * @retval 异常标志位图
  */
uint16_t SafetyMonitor_GetAlarmFlags(void)
{
    return g_safety_monitor.alarm_flags;
}

/**
  * @brief  检查指定异常是否激活
  * @param  flag: 异常标志
  * @retval 1:激活 0:未激活
  */
uint8_t SafetyMonitor_IsAlarmActive(AlarmFlag_t flag)
{
    if(flag >= ALARM_FLAG_COUNT) return 0;
    return (g_safety_monitor.alarm_flags & (1 << flag)) ? 1 : 0;
}

/**
  * @brief  获取异常信息
  * @param  flag: 异常标志
  * @retval 异常信息结构体
  */
AlarmInfo_t SafetyMonitor_GetAlarmInfo(AlarmFlag_t flag)
{
    AlarmInfo_t empty_info = {0};
    if(flag >= ALARM_FLAG_COUNT) return empty_info;
    return g_safety_monitor.alarm_info[flag];
}

// ================== 异常标志管理 ===================

/**
  * @brief  设置异常标志
  * @param  flag: 异常标志
  * @param  description: 异常描述（可选，NULL使用默认描述）
  * @retval 无
  */
void SafetyMonitor_SetAlarmFlag(AlarmFlag_t flag, const char* description)
{
    if(flag >= ALARM_FLAG_COUNT) return;
    
    // 如果异常已经激活，不重复设置
    if(SafetyMonitor_IsAlarmActive(flag)) return;
    
    // 设置异常标志位
    g_safety_monitor.alarm_flags |= (1 << flag);
    
    // 更新异常信息
    g_safety_monitor.alarm_info[flag].is_active = 1;
    g_safety_monitor.alarm_info[flag].trigger_time = HAL_GetTick();
    
    if(description) {
        strncpy(g_safety_monitor.alarm_info[flag].description, 
                description, 
                sizeof(g_safety_monitor.alarm_info[flag].description) - 1);
    }
    
    DEBUG_Printf("[安全监控] 异常标志设置: %c类异常 - %s\r\n", 
                'A' + flag, g_safety_monitor.alarm_info[flag].description);
}

/**
  * @brief  清除异常标志
  * @param  flag: 异常标志
  * @retval 无
  */
void SafetyMonitor_ClearAlarmFlag(AlarmFlag_t flag)
{
    if(flag >= ALARM_FLAG_COUNT) return;
    
    // 如果异常未激活，无需清除
    if(!SafetyMonitor_IsAlarmActive(flag)) return;
    
    // 清除异常标志位
    g_safety_monitor.alarm_flags &= ~(1 << flag);
    
    // 更新异常信息
    g_safety_monitor.alarm_info[flag].is_active = 0;
    
    DEBUG_Printf("[安全监控] 异常标志清除: %c类异常解除\r\n", 'A' + flag);
}

/**
  * @brief  清除所有异常标志
  * @retval 无
  */
void SafetyMonitor_ClearAllAlarmFlags(void)
{
    for(uint8_t i = 0; i < ALARM_FLAG_COUNT; i++) {
        if(SafetyMonitor_IsAlarmActive((AlarmFlag_t)i)) {
            SafetyMonitor_ClearAlarmFlag((AlarmFlag_t)i);
        }
    }
    DEBUG_Printf("[安全监控] 所有异常标志已清除\r\n");
}

/**
  * @brief  检查并更新所有异常状态
  * @retval 无
  */
void SafetyMonitor_UpdateAllAlarmStatus(void)
{
    // 1. 检测使能信号冲突（A类异常）
    SafetyMonitor_CheckEnableConflict();
    
    // 2. 检测继电器状态异常（B~G类异常）
    SafetyMonitor_CheckRelayStatus();
    
    // 3. 检测接触器状态异常（H~J类异常）
    SafetyMonitor_CheckContactorStatus();
    
    // 4. 检测温度异常（K~M类异常）
    SafetyMonitor_CheckTemperatureAlarm();
    
    // 5. 检测电源监控异常（O类异常）
    SafetyMonitor_CheckPowerMonitor();
    
    // 检查异常解除条件
    for(uint8_t i = 0; i < ALARM_FLAG_COUNT; i++) {
        if(SafetyMonitor_IsAlarmActive((AlarmFlag_t)i)) {
            uint8_t can_clear = 0;
            
            if(i == ALARM_FLAG_A) {
                can_clear = SafetyMonitor_CheckAlarmA_ClearCondition();
            } else if((i >= ALARM_FLAG_B && i <= ALARM_FLAG_J) || i == ALARM_FLAG_N) {
                can_clear = SafetyMonitor_CheckAlarmBJN_ClearCondition();
            } else if(i >= ALARM_FLAG_K && i <= ALARM_FLAG_M) {
                can_clear = SafetyMonitor_CheckAlarmKM_ClearCondition();
            } else if(i == ALARM_FLAG_O) {
                can_clear = SafetyMonitor_CheckAlarmO_ClearCondition();
            }
            
            if(can_clear) {
                SafetyMonitor_ClearAlarmFlag((AlarmFlag_t)i);
            }
        }
    }
}

// ================== 报警输出控制 ===================

/**
  * @brief  更新ALARM引脚输出（有任何异常时输出低电平）
  * @retval 无
  */
void SafetyMonitor_UpdateAlarmOutput(void)
{
    uint8_t should_alarm = (g_safety_monitor.alarm_flags != 0) ? 1 : 0;
    
    if(g_safety_monitor.alarm_output_active != should_alarm) {
        g_safety_monitor.alarm_output_active = should_alarm;
        GPIO_SetAlarmOutput(should_alarm);  // 1:低电平输出 0:高电平输出
        
        DEBUG_Printf("[安全监控] ALARM引脚输出: %s\r\n", 
                    should_alarm ? "低电平（报警）" : "高电平（正常）");
    }
}

/**
  * @brief  强制设置ALARM引脚状态
  * @param  active: 1:低电平输出 0:高电平输出
  * @retval 无
  */
void SafetyMonitor_ForceAlarmOutput(uint8_t active)
{
    g_safety_monitor.alarm_output_active = active;
    GPIO_SetAlarmOutput(active);
    DEBUG_Printf("[安全监控] 强制设置ALARM引脚: %s\r\n", 
                active ? "低电平" : "高电平");
}

// ================== 蜂鸣器控制 ===================

/**
  * @brief  更新蜂鸣器状态（根据异常类型决定脉冲方式）
  * @retval 无
  */
void SafetyMonitor_UpdateBeepState(void)
{
    BeepState_t new_state = BEEP_STATE_OFF;
    
    // 按优先级检查异常类型（K~M类最高优先级）
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_K) || 
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_L) || 
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_M)) {
        new_state = BEEP_STATE_CONTINUOUS;  // 持续低电平
    } else if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_B) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_C) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_D) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_E) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_F) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_G) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_H) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_I) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_J)) {
        new_state = BEEP_STATE_PULSE_50MS;  // 50ms间隔脉冲
    } else if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_A) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_N) ||
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {  // 添加O类异常处理
        new_state = BEEP_STATE_PULSE_1S;    // 1秒间隔脉冲
    }
    
    // 如果状态改变，更新蜂鸣器
    if(g_safety_monitor.beep_state != new_state) {
        g_safety_monitor.beep_state = new_state;
        g_safety_monitor.beep_last_toggle_time = HAL_GetTick();
        g_safety_monitor.beep_current_level = 0;
        
        DEBUG_Printf("[安全监控] 蜂鸣器状态切换: %s\r\n", 
                    SafetyMonitor_GetBeepStateDescription(new_state));
        
        // 立即更新蜂鸣器输出
        if(new_state == BEEP_STATE_CONTINUOUS) {
            GPIO_SetBeepOutput(1);  // 持续低电平
            g_safety_monitor.beep_current_level = 1;
        } else if(new_state == BEEP_STATE_OFF) {
            GPIO_SetBeepOutput(0);  // 关闭
            g_safety_monitor.beep_current_level = 0;
        }
    }
}

/**
  * @brief  蜂鸣器脉冲处理（定时调用）
  * @retval 无
  */
void SafetyMonitor_ProcessBeep(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - g_safety_monitor.beep_last_toggle_time;
    
    switch(g_safety_monitor.beep_state) {
        case BEEP_STATE_PULSE_1S:
            // 1秒间隔脉冲：25ms低电平，975ms高电平
            if(g_safety_monitor.beep_current_level == 1 && elapsed_time >= BEEP_PULSE_DURATION) {
                // 结束低电平脉冲，切换到高电平
                GPIO_SetBeepOutput(0);
                g_safety_monitor.beep_current_level = 0;
                g_safety_monitor.beep_last_toggle_time = current_time;
            } else if(g_safety_monitor.beep_current_level == 0 && 
                     elapsed_time >= (BEEP_PULSE_1S_INTERVAL - BEEP_PULSE_DURATION)) {
                // 开始下一个低电平脉冲
                GPIO_SetBeepOutput(1);
                g_safety_monitor.beep_current_level = 1;
                g_safety_monitor.beep_last_toggle_time = current_time;
            }
            break;
            
        case BEEP_STATE_PULSE_50MS:
            // 50ms间隔脉冲：25ms低电平，25ms高电平
            if(elapsed_time >= BEEP_PULSE_DURATION) {
                g_safety_monitor.beep_current_level = !g_safety_monitor.beep_current_level;
                GPIO_SetBeepOutput(g_safety_monitor.beep_current_level);
                g_safety_monitor.beep_last_toggle_time = current_time;
            }
            break;
            
        case BEEP_STATE_CONTINUOUS:
            // 持续低电平，无需处理
            break;
            
        case BEEP_STATE_OFF:
        default:
            // 关闭状态，无需处理
            break;
    }
}

/**
  * @brief  强制设置蜂鸣器状态
  * @param  state: 蜂鸣器状态
  * @retval 无
  */
void SafetyMonitor_ForceBeepState(BeepState_t state)
{
    g_safety_monitor.beep_state = state;
    g_safety_monitor.beep_last_toggle_time = HAL_GetTick();
    g_safety_monitor.beep_current_level = 0;
    
    DEBUG_Printf("[安全监控] 强制设置蜂鸣器状态: %s\r\n", 
                SafetyMonitor_GetBeepStateDescription(state));
}

// ================== 电源监控 ===================

/**
  * @brief  启用电源监控
  * @retval 无
  */
void SafetyMonitor_EnablePowerMonitor(void)
{
    g_safety_monitor.power_monitor_enabled = 1;
    DEBUG_Printf("[安全监控] 电源监控已启用\r\n");
}

/**
  * @brief  禁用电源监控
  * @retval 无
  */
void SafetyMonitor_DisablePowerMonitor(void)
{
    g_safety_monitor.power_monitor_enabled = 0;
    DEBUG_Printf("[安全监控] 电源监控已禁用\r\n");
}

/**
  * @brief  电源异常中断回调（DC_CTRL引脚中断时调用）
  * @retval 无
  */
void SafetyMonitor_PowerFailureCallback(void)
{
    if(!g_safety_monitor.power_monitor_enabled) return;
    
    // 读取DC_CTRL当前状态
    uint8_t dc_ctrl_state = GPIO_ReadDC_CTRL();
    
    DEBUG_Printf("[安全监控] DC_CTRL中断触发，当前状态: %s\r\n", dc_ctrl_state ? "高电平" : "低电平");
    
    // 修正逻辑：根据实际硬件设计
    // 有DC24V供电时：DC_CTRL为低电平（正常状态）
    // 没有DC24V供电时：DC_CTRL为高电平（异常状态）
    if(dc_ctrl_state == 1) {
        // 检测到电源异常（高电平 = 没有DC24V供电）
        if(!SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
            DEBUG_Printf("[安全监控] 检测到外部电源异常！DC24V供电中断\r\n");
            SafetyMonitor_SetAlarmFlag(ALARM_FLAG_O, "DC24V Loss");  // 英文版本，10字符
            
            // 电源异常时保持当前通道状态，不强制关闭
            // 继电器在DC24V断电时会自然断开，无需主动控制
            DEBUG_Printf("[安全监控] 电源异常时保持当前通道状态，继电器将自然断开\r\n");
            
            // 立即更新报警输出和蜂鸣器状态
            SafetyMonitor_UpdateAlarmOutput();
            SafetyMonitor_UpdateBeepState();
            
            // 强制立即处理蜂鸣器脉冲（确保蜂鸣器立即响起）
            SafetyMonitor_ProcessBeep();
        }
    } else {
        // 电源恢复正常（低电平 = 有DC24V供电）
        DEBUG_Printf("[安全监控] 外部电源恢复正常，准备解除O类异常\r\n");
        // 电源恢复后会在主循环中通过CheckAlarmO_ClearCondition自动解除异常
    }
}

// ================== 异常检测函数 ===================

/**
  * @brief  检测使能信号冲突（A类异常）
  * @retval 无
  */
void SafetyMonitor_CheckEnableConflict(void)
{
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // 计算低电平信号的数量
    uint8_t low_count = 0;
    if(k1_en == 0) low_count++;
    if(k2_en == 0) low_count++;
    if(k3_en == 0) low_count++;
    
    // 如果有多于1个信号为低电平，则产生A类异常
    if(low_count > 1) {
        char conflict_desc[64];
        sprintf(conflict_desc, "Multi EN Active");  // 简化版本，14字符，加上"A:"总共16字符
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_A, conflict_desc);
    }
}

/**
  * @brief  检测继电器状态异常（B~G类异常）
  * @retval 无
  */
void SafetyMonitor_CheckRelayStatus(void)
{
    // 读取继电器状态反馈
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    
    // 获取继电器期望状态（从继电器控制模块获取）
    RelayState_t ch1_state = RelayControl_GetChannelState(1);
    RelayState_t ch2_state = RelayControl_GetChannelState(2);
    RelayState_t ch3_state = RelayControl_GetChannelState(3);
    
    // 检查K1_1_STA异常（B类）
    uint8_t k1_1_expected = (ch1_state == RELAY_STATE_ON) ? 1 : 0;
    if(k1_1_sta != k1_1_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_B, NULL);
    }
    
    // 检查K2_1_STA异常（C类）
    uint8_t k2_1_expected = (ch2_state == RELAY_STATE_ON) ? 1 : 0;
    if(k2_1_sta != k2_1_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_C, NULL);
    }
    
    // 检查K3_1_STA异常（D类）
    uint8_t k3_1_expected = (ch3_state == RELAY_STATE_ON) ? 1 : 0;
    if(k3_1_sta != k3_1_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_D, NULL);
    }
    
    // 检查K1_2_STA异常（E类）
    uint8_t k1_2_expected = (ch1_state == RELAY_STATE_ON) ? 1 : 0;
    if(k1_2_sta != k1_2_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_E, NULL);
    }
    
    // 检查K2_2_STA异常（F类）
    uint8_t k2_2_expected = (ch2_state == RELAY_STATE_ON) ? 1 : 0;
    if(k2_2_sta != k2_2_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_F, NULL);
    }
    
    // 检查K3_2_STA异常（G类）
    uint8_t k3_2_expected = (ch3_state == RELAY_STATE_ON) ? 1 : 0;
    if(k3_2_sta != k3_2_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_G, NULL);
    }
}

/**
  * @brief  检测接触器状态异常（H~J类异常）
  * @retval 无
  */
void SafetyMonitor_CheckContactorStatus(void)
{
    // 读取接触器状态反馈
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    // 获取继电器期望状态
    RelayState_t ch1_state = RelayControl_GetChannelState(1);
    RelayState_t ch2_state = RelayControl_GetChannelState(2);
    RelayState_t ch3_state = RelayControl_GetChannelState(3);
    
    // 检查SW1_STA异常（H类）
    uint8_t sw1_expected = (ch1_state == RELAY_STATE_ON) ? 1 : 0;
    if(sw1_sta != sw1_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_H, NULL);
    }
    
    // 检查SW2_STA异常（I类）
    uint8_t sw2_expected = (ch2_state == RELAY_STATE_ON) ? 1 : 0;
    if(sw2_sta != sw2_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_I, NULL);
    }
    
    // 检查SW3_STA异常（J类）
    uint8_t sw3_expected = (ch3_state == RELAY_STATE_ON) ? 1 : 0;
    if(sw3_sta != sw3_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_J, NULL);
    }
}

/**
  * @brief  检测温度异常（K~M类异常）
  * @retval 无
  */
void SafetyMonitor_CheckTemperatureAlarm(void)
{
    // 获取温度信息
    TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
    TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
    TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
    
    // 检查NTC_1温度异常（K类）
    if(temp1.value_celsius >= TEMP_ALARM_THRESHOLD) {
        char temp_desc[32];
        sprintf(temp_desc, "NTC1 %.1fC", temp1.value_celsius);  // 英文版本，约10字符
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_K, temp_desc);
    }
    
    // 检查NTC_2温度异常（L类）
    if(temp2.value_celsius >= TEMP_ALARM_THRESHOLD) {
        char temp_desc[32];
        sprintf(temp_desc, "NTC2 %.1fC", temp2.value_celsius);  // 英文版本，约10字符
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_L, temp_desc);
    }
    
    // 检查NTC_3温度异常（M类）
    if(temp3.value_celsius >= TEMP_ALARM_THRESHOLD) {
        char temp_desc[32];
        sprintf(temp_desc, "NTC3 %.1fC", temp3.value_celsius);  // 英文版本，约10字符
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_M, temp_desc);
    }
}

/**
  * @brief  检测电源监控异常（O类异常）
  * @retval 无
  */
void SafetyMonitor_CheckPowerMonitor(void)
{
    if(!g_safety_monitor.power_monitor_enabled) return;
    
    // 读取DC_CTRL状态
    uint8_t dc_ctrl_state = GPIO_ReadDC_CTRL();
    
    // 修正逻辑：根据实际硬件设计
    // 有DC24V供电时：DC_CTRL为低电平（正常状态）
    // 没有DC24V供电时：DC_CTRL为高电平（异常状态）
    if(dc_ctrl_state == 1) {
        // 检测到电源异常（高电平 = 没有DC24V供电）
        if(!SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
            DEBUG_Printf("[安全监控] 电源监控检测到DC24V异常\r\n");
            SafetyMonitor_SetAlarmFlag(ALARM_FLAG_O, "DC24V Loss");  // 英文版本，10字符
            
            // 电源异常时保持当前通道状态，不强制关闭
            // 继电器在DC24V断电时会自然断开，无需主动控制
            DEBUG_Printf("[安全监控] 电源异常时保持当前通道状态，继电器将自然断开\r\n");
            
            // 立即更新报警输出和蜂鸣器状态
            SafetyMonitor_UpdateAlarmOutput();
            SafetyMonitor_UpdateBeepState();
            SafetyMonitor_ProcessBeep();
        }
    }
}

// ================== 异常解除检查 ===================

/**
  * @brief  检查A类异常解除条件
  * @retval 1:可以解除 0:不能解除
  */
uint8_t SafetyMonitor_CheckAlarmA_ClearCondition(void)
{
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // 计算低电平信号的数量
    uint8_t low_count = 0;
    if(k1_en == 0) low_count++;
    if(k2_en == 0) low_count++;
    if(k3_en == 0) low_count++;
    
    // A异常解除条件：只能有一路处于低电平或三路均处于高电平
    return (low_count <= 1) ? 1 : 0;
}

/**
  * @brief  检查B~J、N类异常解除条件
  * @retval 1:可以解除 0:不能解除
  */
uint8_t SafetyMonitor_CheckAlarmBJN_ClearCondition(void)
{
    // 读取所有相关状态
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    // 检查是否符合触发条件真值表的三种状态之一
    
    // Channel_1打开状态：K1_EN=0,K2_EN=1,K3_EN=1,K1_1_STA=1,K1_2_STA=1,其他STA=0,SW1_STA=1,其他SW=0
    if(k1_en == 0 && k2_en == 1 && k3_en == 1 && 
       k1_1_sta == 1 && k1_2_sta == 1 && k2_1_sta == 0 && k2_2_sta == 0 && k3_1_sta == 0 && k3_2_sta == 0 &&
       sw1_sta == 1 && sw2_sta == 0 && sw3_sta == 0) {
        return 1;
    }
    
    // Channel_2打开状态：K2_EN=0,K1_EN=1,K3_EN=1,K2_1_STA=1,K2_2_STA=1,其他STA=0,SW2_STA=1,其他SW=0
    if(k2_en == 0 && k1_en == 1 && k3_en == 1 && 
       k2_1_sta == 1 && k2_2_sta == 1 && k1_1_sta == 0 && k1_2_sta == 0 && k3_1_sta == 0 && k3_2_sta == 0 &&
       sw2_sta == 1 && sw1_sta == 0 && sw3_sta == 0) {
        return 1;
    }
    
    // Channel_3打开状态：K3_EN=0,K1_EN=1,K2_EN=1,K3_1_STA=1,K3_2_STA=1,其他STA=0,SW3_STA=1,其他SW=0
    if(k3_en == 0 && k1_en == 1 && k2_en == 1 && 
       k3_1_sta == 1 && k3_2_sta == 1 && k1_1_sta == 0 && k1_2_sta == 0 && k2_1_sta == 0 && k2_2_sta == 0 &&
       sw3_sta == 1 && sw1_sta == 0 && sw2_sta == 0) {
        return 1;
    }
    
    // 全部关闭状态：所有EN=1，所有STA=0
    if(k1_en == 1 && k2_en == 1 && k3_en == 1 &&
       k1_1_sta == 0 && k1_2_sta == 0 && k2_1_sta == 0 && k2_2_sta == 0 && k3_1_sta == 0 && k3_2_sta == 0 &&
       sw1_sta == 0 && sw2_sta == 0 && sw3_sta == 0) {
        return 1;
    }
    
    return 0;  // 不符合任何正常状态，不能解除异常
}

/**
  * @brief  检查K~M类异常解除条件
  * @retval 1:可以解除 0:不能解除
  */
uint8_t SafetyMonitor_CheckAlarmKM_ClearCondition(void)
{
    // 获取温度信息
    TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
    TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
    TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
    
    // K~M异常解除条件：温度回降到58℃以下（考虑到2℃回差）
    uint8_t can_clear = 1;
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_K) && temp1.value_celsius >= TEMP_ALARM_CLEAR_THRESHOLD) {
        can_clear = 0;
    }
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_L) && temp2.value_celsius >= TEMP_ALARM_CLEAR_THRESHOLD) {
        can_clear = 0;
    }
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_M) && temp3.value_celsius >= TEMP_ALARM_CLEAR_THRESHOLD) {
        can_clear = 0;
    }
    
    return can_clear;
}

/**
  * @brief  检查O类异常解除条件
  * @retval 1:可以解除 0:不能解除
  */
uint8_t SafetyMonitor_CheckAlarmO_ClearCondition(void)
{
    if(!g_safety_monitor.power_monitor_enabled) return 1;
    
    // 读取DC_CTRL状态
    uint8_t dc_ctrl_state = GPIO_ReadDC_CTRL();
    
    // 修正逻辑：O类异常解除条件是DC_CTRL恢复到低电平（电源正常）
    // 有DC24V供电时：DC_CTRL为低电平（正常状态）
    // 没有DC24V供电时：DC_CTRL为高电平（异常状态）
    
    // 简化逻辑：只要DC_CTRL为低电平就可以解除异常，不需要延时确认
    // 因为中断已经能够及时检测到状态变化
    if(dc_ctrl_state == 0) {
        DEBUG_Printf("[安全监控] 电源恢复正常，解除O类异常\r\n");
        return 1;
    }
    
    return 0;
}

// ================== 调试和状态查询 ===================

/**
  * @brief  输出当前安全监控状态
  * @retval 无
  */
void SafetyMonitor_PrintStatus(void)
{
    DEBUG_Printf("=== 安全监控状态 ===\r\n");
    DEBUG_Printf("异常标志位图: 0x%04X\r\n", g_safety_monitor.alarm_flags);
    DEBUG_Printf("ALARM引脚输出: %s\r\n", 
                g_safety_monitor.alarm_output_active ? "低电平（报警）" : "高电平（正常）");
    DEBUG_Printf("蜂鸣器状态: %s\r\n", 
                SafetyMonitor_GetBeepStateDescription(g_safety_monitor.beep_state));
    DEBUG_Printf("电源监控: %s\r\n", 
                g_safety_monitor.power_monitor_enabled ? "启用" : "禁用");
    
    // 输出激活的异常信息
    for(uint8_t i = 0; i < ALARM_FLAG_COUNT; i++) {
        if(SafetyMonitor_IsAlarmActive((AlarmFlag_t)i)) {
            DEBUG_Printf("异常 %c: %s (触发时间: %lu)\r\n", 
                        'A' + i, 
                        g_safety_monitor.alarm_info[i].description,
                        g_safety_monitor.alarm_info[i].trigger_time);
        }
    }
}

/**
  * @brief  获取异常描述字符串
  * @param  flag: 异常标志
  * @retval 异常描述字符串
  */
const char* SafetyMonitor_GetAlarmDescription(AlarmFlag_t flag)
{
    if(flag >= ALARM_FLAG_COUNT) return "未知异常";
    return g_alarm_descriptions[flag];
}

/**
  * @brief  获取蜂鸣器状态描述
  * @param  state: 蜂鸣器状态
  * @retval 状态描述字符串
  */
const char* SafetyMonitor_GetBeepStateDescription(BeepState_t state)
{
    switch(state) {
        case BEEP_STATE_OFF:         return "关闭";
        case BEEP_STATE_PULSE_1S:    return "1秒间隔脉冲";
        case BEEP_STATE_PULSE_50MS:  return "50ms间隔脉冲";
        case BEEP_STATE_CONTINUOUS:  return "持续输出";
        default:                     return "未知状态";
    }
}

/**
  * @brief  安全监控调试输出
  * @retval 无
  */
void SafetyMonitor_DebugPrint(void)
{
    SafetyMonitor_PrintStatus();
}

// ================== 私有函数 ===================

/**
  * @brief  获取异常类型（根据异常标志确定报警类型）
  * @param  flag: 异常标志
  * @retval 异常类型
  */
static AlarmType_t SafetyMonitor_GetAlarmType(AlarmFlag_t flag)
{
    if(flag == ALARM_FLAG_A || flag == ALARM_FLAG_N || flag == ALARM_FLAG_O) {
        return ALARM_TYPE_AN;  // A、N、O类异常：1秒间隔脉冲
    } else if(flag >= ALARM_FLAG_B && flag <= ALARM_FLAG_J) {
        return ALARM_TYPE_BJ;  // B~J类异常：50ms间隔脉冲
    } else if(flag >= ALARM_FLAG_K && flag <= ALARM_FLAG_M) {
        return ALARM_TYPE_KM;  // K~M类异常：持续低电平
    }
    return ALARM_TYPE_AN;  // 默认类型
} 

