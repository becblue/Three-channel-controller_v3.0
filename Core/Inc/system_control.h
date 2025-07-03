#ifndef __SYSTEM_CONTROL_H__
#define __SYSTEM_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * 系统控制模块头文件
 * 1. 系统状态机管理（自检、正常、异常、报警等）
 * 2. 上电自检流程（2秒LOGO、3秒进度条、输入/反馈/温度检测）
 * 3. 主循环调度器（调度各功能模块）
 * 4. 系统状态切换与异常处理
 ************************************************************/

// 系统状态枚举
typedef enum {
    SYSTEM_STATE_INIT = 0,          // 系统初始化状态
    SYSTEM_STATE_LOGO_DISPLAY,      // LOGO显示状态（2秒）
    SYSTEM_STATE_SELF_TEST,         // 自检状态（3秒进度条）
    SYSTEM_STATE_SELF_TEST_CHECK,   // 自检检测阶段
    SYSTEM_STATE_NORMAL,            // 正常运行状态
    SYSTEM_STATE_ERROR,             // 系统错误状态
    SYSTEM_STATE_ALARM              // 报警状态
} SystemState_t;

// 自检项目枚举
typedef enum {
    SELF_TEST_INPUT_SIGNALS = 0,    // 检测三路输入信号K1_EN、K2_EN、K3_EN
    SELF_TEST_FEEDBACK_SIGNALS,     // 检测状态反馈信号
    SELF_TEST_TEMPERATURE,          // 检测NTC温度
    SELF_TEST_COMPLETE              // 自检完成
} SelfTestItem_t;

// 自检结果结构体
typedef struct {
    uint8_t input_signals_ok;       // 输入信号检测结果（1:正常 0:异常）
    uint8_t feedback_signals_ok;    // 反馈信号检测结果（1:正常 0:异常）
    uint8_t temperature_ok;         // 温度检测结果（1:正常 0:异常）
    uint8_t overall_result;         // 总体自检结果（1:通过 0:失败）
    char error_info[64];            // 错误信息字符串
} SelfTestResult_t;

// 系统控制结构体
typedef struct {
    SystemState_t current_state;    // 当前系统状态
    SystemState_t last_state;       // 上一个系统状态
    uint32_t state_start_time;      // 当前状态开始时间
    SelfTestResult_t self_test;     // 自检结果
    uint8_t error_flags;            // 系统错误标志位
} SystemControl_t;

// 时间常量定义
#define LOGO_DISPLAY_TIME_MS        2000    // LOGO显示时间（2秒）
#define SELF_TEST_TIME_MS           3000    // 自检时间（3秒）
#define MAIN_LOOP_INTERVAL_MS       10      // 主循环间隔（10ms）

// 自检阈值定义
#define SELF_TEST_TEMP_THRESHOLD    60.0f   // 自检温度阈值（60℃）

// ================== 系统控制接口函数 ===================

// 系统控制模块初始化
void SystemControl_Init(void);

// 系统状态机处理（主循环中调用）
void SystemControl_Process(void);

// 获取当前系统状态
SystemState_t SystemControl_GetState(void);

// 强制切换系统状态
void SystemControl_SetState(SystemState_t new_state);

// 获取自检结果
SelfTestResult_t SystemControl_GetSelfTestResult(void);

// 系统复位
void SystemControl_Reset(void);

// ================== 自检流程函数 ===================

// 开始LOGO显示阶段
void SystemControl_StartLogoDisplay(void);

// 开始自检阶段
void SystemControl_StartSelfTest(void);

// 执行自检检测
void SystemControl_ExecuteSelfTest(void);

// 检测输入信号（K1_EN、K2_EN、K3_EN应为高电平）
uint8_t SystemControl_CheckInputSignals(void);

// 检测反馈信号（所有状态反馈应为低电平）
uint8_t SystemControl_CheckFeedbackSignals(void);

// 检测温度（所有NTC温度应在60℃以下）
uint8_t SystemControl_CheckTemperature(void);

// ================== 主循环调度函数 ===================

// 主循环调度器（调度各功能模块）
void SystemControl_MainLoop(void);

// 调度继电器控制模块
void SystemControl_ScheduleRelayControl(void);

// 调度温度监控模块
void SystemControl_ScheduleTemperatureMonitor(void);

// 调度OLED显示模块
void SystemControl_ScheduleOLEDDisplay(void);

// 调度安全监控模块（预留）
void SystemControl_ScheduleSafetyMonitor(void);

// ================== 状态切换与异常处理 ===================

// 进入正常运行状态
void SystemControl_EnterNormalState(void);

// 进入错误状态
void SystemControl_EnterErrorState(const char* error_msg);

// 进入报警状态
void SystemControl_EnterAlarmState(void);

// 处理自检异常（产生N异常标志）
void SystemControl_HandleSelfTestError(void);

// 系统状态调试输出
void SystemControl_DebugPrint(void);

// ========== 函数声明 ==========
void SystemControl_MainLoopScheduler(void);
void SystemControl_UpdateDisplay(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CONTROL_H__ */

