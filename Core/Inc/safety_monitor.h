#ifndef __SAFETY_MONITOR_H__
#define __SAFETY_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * 安全监控模块头文件
 * 1. 异常标志（A~N、KLM等）检测与管理
 * 2. 报警输出（ALARM引脚、蜂鸣器多种脉冲方式）
 * 3. 电源监控（DC_CTRL异常检测）
 * 4. 异常优先级和解除机制
 * 
 * 严格按照README要求：
 * - A、N类异常：蜂鸣器1秒间隔脉冲
 * - B~J类异常：蜂鸣器50ms间隔脉冲
 * - K~M类异常：蜂鸣器持续低电平
 * - ALARM引脚：所有异常均输出持续低电平
 ************************************************************/

// 异常标志枚举（按照README定义）
typedef enum {
    ALARM_FLAG_A = 0,       // K1_EN、K2_EN、K3_EN使能冲突
    ALARM_FLAG_B,           // K1_1_STA工作异常
    ALARM_FLAG_C,           // K2_1_STA工作异常
    ALARM_FLAG_D,           // K3_1_STA工作异常
    ALARM_FLAG_E,           // K1_2_STA工作异常
    ALARM_FLAG_F,           // K2_2_STA工作异常
    ALARM_FLAG_G,           // K3_2_STA工作异常
    ALARM_FLAG_H,           // SW1_STA工作异常
    ALARM_FLAG_I,           // SW2_STA工作异常
    ALARM_FLAG_J,           // SW3_STA工作异常
    ALARM_FLAG_K,           // NTC_1温度异常
    ALARM_FLAG_L,           // NTC_2温度异常
    ALARM_FLAG_M,           // NTC_3温度异常
    ALARM_FLAG_N,           // 自检异常
    ALARM_FLAG_COUNT        // 异常标志总数
} AlarmFlag_t;

// 异常类型枚举
typedef enum {
    ALARM_TYPE_AN = 0,      // A、N类异常：1秒间隔脉冲
    ALARM_TYPE_BJ,          // B~J类异常：50ms间隔脉冲
    ALARM_TYPE_KM           // K~M类异常：持续低电平
} AlarmType_t;

// 蜂鸣器状态枚举
typedef enum {
    BEEP_STATE_OFF = 0,     // 蜂鸣器关闭
    BEEP_STATE_PULSE_1S,    // 1秒间隔脉冲（A、N类异常）
    BEEP_STATE_PULSE_50MS,  // 50ms间隔脉冲（B~J类异常）
    BEEP_STATE_CONTINUOUS   // 持续输出（K~M类异常）
} BeepState_t;

// 异常信息结构体
typedef struct {
    uint8_t is_active;      // 异常是否激活（1:激活 0:已解除）
    AlarmType_t type;       // 异常类型
    uint32_t trigger_time;  // 异常触发时间
    char description[32];   // 异常描述信息
} AlarmInfo_t;

// 安全监控结构体
typedef struct {
    uint16_t alarm_flags;               // 异常标志位（位图，支持16个异常）
    AlarmInfo_t alarm_info[ALARM_FLAG_COUNT];  // 详细异常信息
    uint8_t alarm_output_active;        // ALARM引脚输出状态（1:低电平输出 0:高电平）
    BeepState_t beep_state;             // 当前蜂鸣器状态
    uint32_t beep_last_toggle_time;     // 蜂鸣器上次切换时间
    uint8_t beep_current_level;         // 蜂鸣器当前电平状态
    uint8_t power_monitor_enabled;      // 电源监控使能标志
} SafetyMonitor_t;

// 时间常量定义
#define BEEP_PULSE_1S_INTERVAL      1000    // 1秒间隔脉冲周期（ms）
#define BEEP_PULSE_50MS_INTERVAL    50      // 50ms间隔脉冲周期（ms）
#define BEEP_PULSE_DURATION         25      // 脉冲持续时间（ms）

// 温度异常阈值（与temperature_monitor模块保持一致）
#define TEMP_ALARM_THRESHOLD        60.0f   // 温度报警阈值（60℃）
#define TEMP_ALARM_CLEAR_THRESHOLD  58.0f   // 温度报警解除阈值（58℃，2℃回差）

// ================== 核心管理功能 ===================

// 安全监控模块初始化
void SafetyMonitor_Init(void);

// 安全监控主处理函数（主循环中调用）
void SafetyMonitor_Process(void);

// 获取当前异常状态（返回异常标志位图）
uint16_t SafetyMonitor_GetAlarmFlags(void);

// 检查指定异常是否激活
uint8_t SafetyMonitor_IsAlarmActive(AlarmFlag_t flag);

// 获取异常信息
AlarmInfo_t SafetyMonitor_GetAlarmInfo(AlarmFlag_t flag);

// ================== 异常标志管理 ===================

// 设置异常标志
void SafetyMonitor_SetAlarmFlag(AlarmFlag_t flag, const char* description);

// 清除异常标志
void SafetyMonitor_ClearAlarmFlag(AlarmFlag_t flag);

// 清除所有异常标志
void SafetyMonitor_ClearAllAlarmFlags(void);

// 检查并更新所有异常状态
void SafetyMonitor_UpdateAllAlarmStatus(void);

// ================== 报警输出控制 ===================

// 更新ALARM引脚输出（有任何异常时输出低电平）
void SafetyMonitor_UpdateAlarmOutput(void);

// 强制设置ALARM引脚状态
void SafetyMonitor_ForceAlarmOutput(uint8_t active);

// ================== 蜂鸣器控制 ===================

// 更新蜂鸣器状态（根据异常类型决定脉冲方式）
void SafetyMonitor_UpdateBeepState(void);

// 蜂鸣器脉冲处理（定时调用）
void SafetyMonitor_ProcessBeep(void);

// 强制设置蜂鸣器状态
void SafetyMonitor_ForceBeepState(BeepState_t state);

// ================== 电源监控 ===================

// 启用电源监控
void SafetyMonitor_EnablePowerMonitor(void);

// 禁用电源监控
void SafetyMonitor_DisablePowerMonitor(void);

// 电源异常中断回调（DC_CTRL引脚中断时调用）
void SafetyMonitor_PowerFailureCallback(void);

// ================== 异常检测函数 ===================

// 检测使能信号冲突（A类异常）
void SafetyMonitor_CheckEnableConflict(void);

// 检测继电器状态异常（B~G类异常）
void SafetyMonitor_CheckRelayStatus(void);

// 检测接触器状态异常（H~J类异常）
void SafetyMonitor_CheckContactorStatus(void);

// 检测温度异常（K~M类异常）
void SafetyMonitor_CheckTemperatureAlarm(void);

// ================== 异常解除检查 ===================

// 检查A类异常解除条件
uint8_t SafetyMonitor_CheckAlarmA_ClearCondition(void);

// 检查B~J、N类异常解除条件  
uint8_t SafetyMonitor_CheckAlarmBJN_ClearCondition(void);

// 检查K~M类异常解除条件
uint8_t SafetyMonitor_CheckAlarmKM_ClearCondition(void);

// ================== 调试和状态查询 ===================

// 输出当前安全监控状态
void SafetyMonitor_PrintStatus(void);

// 获取异常描述字符串
const char* SafetyMonitor_GetAlarmDescription(AlarmFlag_t flag);

// 获取蜂鸣器状态描述
const char* SafetyMonitor_GetBeepStateDescription(BeepState_t state);

// 安全监控调试输出
void SafetyMonitor_DebugPrint(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_MONITOR_H__ */ 

