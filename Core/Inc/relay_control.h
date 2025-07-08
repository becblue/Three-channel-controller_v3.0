#ifndef __RELAY_CONTROL_H__
#define __RELAY_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * 继电器控制模块头文件
 * 1. 通道控制（开启/关闭）
 * 2. 互锁检测、三重检测、状态反馈、异常处理
 * 3. 通道状态、错误码、控制函数声明
 ************************************************************/

// 继电器通道状态枚举
typedef enum {
    RELAY_STATE_OFF = 0,    // 继电器关闭状态
    RELAY_STATE_ON,         // 继电器打开状态
    RELAY_STATE_ERROR       // 继电器错误状态
} RelayState_t;

// 继电器通道结构体
typedef struct {
    uint8_t channelNum;         // 通道号(1-3)
    RelayState_t state;         // 当前状态
    uint32_t lastActionTime;    // 最后一次动作时间
    uint8_t errorCode;          // 错误代码
} RelayChannel_t;

// 初始化函数
void RelayControl_Init(void);

// 通道控制函数
uint8_t RelayControl_OpenChannel(uint8_t channelNum);   // 开启指定通道
uint8_t RelayControl_CloseChannel(uint8_t channelNum);  // 关闭指定通道

// 状态检测函数
RelayState_t RelayControl_GetChannelState(uint8_t channelNum); // 获取通道状态
uint8_t RelayControl_CheckChannelFeedback(uint8_t channelNum); // 检查通道反馈

// 互锁检查函数
uint8_t RelayControl_CheckInterlock(uint8_t channelNum); // 检查通道互锁

// 错误处理函数
uint8_t RelayControl_GetLastError(uint8_t channelNum);   // 获取最后一次错误
void RelayControl_ClearError(uint8_t channelNum);        // 清除错误状态

// 使能信号处理函数（在中断中调用）
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state);

// 错误代码定义
#define RELAY_ERR_NONE              0x00    // 无错误
#define RELAY_ERR_INTERLOCK         0x01    // 互锁错误
#define RELAY_ERR_FEEDBACK          0x02    // 反馈错误
#define RELAY_ERR_TIMEOUT           0x03    // 超时错误
#define RELAY_ERR_INVALID_CHANNEL   0x04    // 无效通道
#define RELAY_ERR_INVALID_STATE     0x05    // 无效状态
#define RELAY_ERR_POWER_FAILURE     0x06    // 电源异常错误
#define RELAY_ERR_HARDWARE_FAILURE  0x07    // 硬件反馈异常错误

// 时间常量定义
#define RELAY_CHECK_INTERVAL        50      // 使能信号检测间隔(ms)
#define RELAY_PULSE_WIDTH           500     // 控制脉冲宽度(ms)
#define RELAY_FEEDBACK_DELAY        500     // 反馈检测延时(ms)
#define RELAY_CHECK_COUNT           3       // 使能信号检测次数

// ========== 函数声明 ==========
void RelayControl_ProcessPendingActions(void); // 处理待处理的动作

#ifdef __cplusplus
}
#endif

#endif /* __RELAY_CONTROL_H__ */ 



