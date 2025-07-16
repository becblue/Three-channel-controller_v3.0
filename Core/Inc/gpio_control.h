#ifndef __GPIO_CONTROL_H__
#define __GPIO_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * GPIO控制模块头文件
 * 1. 输入信号读取接口（如K1_EN、K2_EN、K3_EN、状态反馈、按键等）
 * 2. 输出信号控制接口（如继电器、蜂鸣器、报警、风扇PWM等）
 * 3. 中断回调、去抖动处理、蜂鸣器、报警、风扇等相关接口
 ************************************************************/

//---------------- 输入信号读取 ----------------//
// 使能信号读取
uint8_t GPIO_ReadK1_EN(void);   // 读取K1_EN输入信号
uint8_t GPIO_ReadK2_EN(void);   // 读取K2_EN输入信号
uint8_t GPIO_ReadK3_EN(void);   // 读取K3_EN输入信号

// 状态反馈信号读取
uint8_t GPIO_ReadK1_1_STA(void); // 读取K1_1状态反馈
uint8_t GPIO_ReadK1_2_STA(void); // 读取K1_2状态反馈
uint8_t GPIO_ReadK2_1_STA(void); // 读取K2_1状态反馈
uint8_t GPIO_ReadK2_2_STA(void); // 读取K2_2状态反馈
uint8_t GPIO_ReadK3_1_STA(void); // 读取K3_1状态反馈
uint8_t GPIO_ReadK3_2_STA(void); // 读取K3_2状态反馈
uint8_t GPIO_ReadSW1_STA(void);  // 读取SW1状态反馈
uint8_t GPIO_ReadSW2_STA(void);  // 读取SW2状态反馈
uint8_t GPIO_ReadSW3_STA(void);  // 读取SW3状态反馈

// 电源监控信号读取
uint8_t GPIO_ReadDC_CTRL(void);  // 读取外部电源监控信号

// 风扇转速信号读取
uint8_t GPIO_ReadFAN_SEN(void);  // 读取风扇转速信号

//---------------- 输出信号控制 ----------------//
// 继电器控制信号
void GPIO_SetK1_1_ON(uint8_t state);   // 控制K1_1_ON输出
void GPIO_SetK1_1_OFF(uint8_t state);  // 控制K1_1_OFF输出
void GPIO_SetK1_2_ON(uint8_t state);   // 控制K1_2_ON输出
void GPIO_SetK1_2_OFF(uint8_t state);  // 控制K1_2_OFF输出
void GPIO_SetK2_1_ON(uint8_t state);   // 控制K2_1_ON输出
void GPIO_SetK2_1_OFF(uint8_t state);  // 控制K2_1_OFF输出
void GPIO_SetK2_2_ON(uint8_t state);   // 控制K2_2_ON输出
void GPIO_SetK2_2_OFF(uint8_t state);  // 控制K2_2_OFF输出
void GPIO_SetK3_1_ON(uint8_t state);   // 控制K3_1_ON输出
void GPIO_SetK3_1_OFF(uint8_t state);  // 控制K3_1_OFF输出
void GPIO_SetK3_2_ON(uint8_t state);   // 控制K3_2_ON输出
void GPIO_SetK3_2_OFF(uint8_t state);  // 控制K3_2_OFF输出

// 报警输出控制
void GPIO_SetAlarmOutput(uint8_t state);     // 控制ALARM引脚输出（1:低电平 0:高电平）

// 蜂鸣器控制
void GPIO_SetBeepOutput(uint8_t state);      // 控制蜂鸣器输出（1:低电平 0:高电平）

// 风扇PWM控制（如需直接控制PWM引脚）
void GPIO_SetFAN_PWM(uint8_t state);   // 控制风扇PWM输出（如有需要）

// RS485方向控制（如后续扩展）
void GPIO_SetRS485_EN(uint8_t state);  // 控制RS485方向使能

//---------------- 中断与去抖动 ----------------//
// 使能信号中断回调（在EXTI中断服务函数中调用）
void GPIO_K1_EN_Callback(uint16_t GPIO_Pin);
void GPIO_K2_EN_Callback(uint16_t GPIO_Pin);
void GPIO_K3_EN_Callback(uint16_t GPIO_Pin);
void GPIO_DC_CTRL_Callback(uint16_t GPIO_Pin);

// 去抖动处理相关（如有需要可扩展）

// ================== GPIO轮询系统（替代中断方式） ===================

// 轮询状态结构体
typedef struct {
    uint8_t current_state;      // 当前引脚状态
    uint8_t last_state;         // 上次引脚状态  
    uint8_t stable_state;       // 稳定状态（防抖后的状态）
    uint32_t state_change_time; // 状态变化时间
    uint32_t last_poll_time;    // 上次轮询时间
    uint8_t debounce_counter;   // 防抖计数器
} GPIO_PollState_t;

// 防抖配置参数
#define GPIO_DEBOUNCE_TIME_MS       50   // 防抖时间50ms
#define GPIO_DEBOUNCE_SAMPLES       5    // 防抖采样次数
#define GPIO_POLL_INTERVAL_MS       10   // 轮询间隔10ms

// GPIO轮询系统函数声明
void GPIO_InitPollingSystem(void);
void GPIO_ProcessPolling(void);
void GPIO_DisableInterrupts(void);
uint8_t GPIO_GetStableState(uint8_t pin_index);

// 轮询系统诊断函数
void GPIO_PrintPollingStats(void);
uint8_t GPIO_IsPollingEnabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_CONTROL_H__ */ 

