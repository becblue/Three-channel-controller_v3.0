#ifndef __TEMPERATURE_MONITOR_H__
#define __TEMPERATURE_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * 温度监控模块头文件
 * 1. NTC温度采集与查表法温度转换
 * 2. 风扇PWM联动控制
 * 3. 温度异常报警与回差处理
 * 4. 温度状态查询与调试输出
 ************************************************************/

// 温度通道枚举
typedef enum {
    TEMP_CH1 = 0,   // 通道1（NTC_1）
    TEMP_CH2,       // 通道2（NTC_2）
    TEMP_CH3,       // 通道3（NTC_3）
    TEMP_CH_MAX
} TempChannel_t;

// 温度状态枚举
typedef enum {
    TEMP_STATE_NORMAL = 0,   // 正常温度
    TEMP_STATE_HIGH,         // 高温
    TEMP_STATE_OVERHEAT      // 超温异常
} TempState_t;

// 温度监控结构体
typedef struct {
    float value_celsius;     // 当前温度（摄氏度）
    TempState_t state;       // 当前温度状态
} TempInfo_t;

// 风扇转速信息结构体
typedef struct {
    uint32_t rpm;         // 当前风扇转速(RPM)
    uint32_t pulse_count; // 1秒内检测到的脉冲数
} FanSpeedInfo_t;

// ================== ADC DMA采样相关 ===================
extern uint16_t adc_dma_buf[TEMP_CH_MAX]; // DMA采样缓冲区

// 温度监控模块初始化
void TemperatureMonitor_Init(void);

// 采集所有NTC温度并更新状态（建议定时调用）
void TemperatureMonitor_UpdateAll(void);

// 获取指定通道的温度信息
TempInfo_t TemperatureMonitor_GetInfo(TempChannel_t channel);

// 获取所有通道的温度状态（便于报警和联动）
void TemperatureMonitor_GetAllStates(TempState_t states[TEMP_CH_MAX]);

// 设置风扇PWM占空比（0~100），内部自动根据温度联动
void TemperatureMonitor_SetFanPWM(uint8_t duty);

// 查询风扇当前PWM占空比
uint8_t TemperatureMonitor_GetFanPWM(void);

// 检查并处理温度异常（如KLM标志、回差等）
void TemperatureMonitor_CheckAndHandleAlarm(void);

// 串口调试输出当前温度和风扇状态
void TemperatureMonitor_DebugPrint(void);

// 获取当前风扇转速信息
FanSpeedInfo_t TemperatureMonitor_GetFanSpeed(void);

// 1秒定时器回调（每秒调用一次，统计风扇转速）
void TemperatureMonitor_FanSpeed1sHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEMPERATURE_MONITOR_H__ */ 

