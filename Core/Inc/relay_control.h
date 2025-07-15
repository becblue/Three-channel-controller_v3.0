#ifndef __RELAY_CONTROL_H__
#define __RELAY_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * 继电器控制模块头文件
 * 1. 通道控制（开启/关闭）- 新增异步状态机架构
 * 2. 互锁检测、三重检测、状态反馈、异常处理
 * 3. 中断防抖和干扰检测功能
 * 4. 中断优先级处理机制
 ************************************************************/

// 继电器通道状态枚举
typedef enum {
    RELAY_STATE_OFF = 0,    // 继电器关闭状态
    RELAY_STATE_ON,         // 继电器打开状态
    RELAY_STATE_ERROR       // 继电器错误状态
} RelayState_t;

// ================== 异步状态机架构 ===================

// 继电器异步操作状态枚举
typedef enum {
    RELAY_ASYNC_IDLE = 0,           // 空闲状态
    RELAY_ASYNC_PULSE_START,        // 脉冲开始
    RELAY_ASYNC_PULSE_ACTIVE,       // 脉冲进行中
    RELAY_ASYNC_PULSE_END,          // 脉冲结束
    RELAY_ASYNC_FEEDBACK_WAIT,      // 等待反馈
    RELAY_ASYNC_FEEDBACK_CHECK,     // 检查反馈
    RELAY_ASYNC_COMPLETE,           // 操作完成
    RELAY_ASYNC_ERROR               // 操作失败
} RelayAsyncState_t;

// 异步操作控制结构体
typedef struct {
    RelayAsyncState_t state;        // 当前状态
    uint8_t channel;                // 通道号(1-3)
    uint8_t operation;              // 操作类型(0=关闭, 1=开启)
    uint32_t start_time;            // 操作开始时间
    uint32_t phase_start_time;      // 当前阶段开始时间
    uint8_t result;                 // 操作结果
    uint8_t error_code;             // 错误代码
} RelayAsyncOperation_t;

// ================== 中断防抖和干扰检测 ===================

// 中断优先级处理结构体
typedef struct {
    uint8_t channel;
    uint32_t interrupt_time;
    uint8_t priority;
} InterruptPriority_t;

// 干扰检测结构体
typedef struct {
    uint32_t last_interrupt_time[3];  // 各通道最后中断时间
    uint8_t simultaneous_count;       // 同时中断计数
    uint8_t interference_detected;    // 干扰检测标志
    uint32_t last_interference_time;  // 最后一次干扰检测时间
} InterferenceFilter_t;

// 继电器通道结构体
typedef struct {
    uint8_t channelNum;         // 通道号(1-3)
    RelayState_t state;         // 当前状态
    uint32_t lastActionTime;    // 最后一次动作时间
    uint8_t errorCode;          // 错误代码
} RelayChannel_t;

// ================== 防抖和干扰检测参数 ===================
#define INTERRUPT_DEBOUNCE_TIME_MS    50    // 防抖时间50ms
#define INTERFERENCE_DETECT_TIME_MS   10    // 同时中断检测窗口10ms
#define STATE_VALIDATION_DELAY_MS     100   // 状态验证延迟100ms
#define MAX_ASYNC_OPERATIONS         3      // 最大同时异步操作数

// ================== 基础函数 ===================

// 初始化函数
void RelayControl_Init(void);

// ================== 异步操作函数 ===================

// 启动异步操作（非阻塞）
uint8_t RelayControl_StartOpenChannel(uint8_t channelNum);   // 启动开启操作
uint8_t RelayControl_StartCloseChannel(uint8_t channelNum);  // 启动关闭操作

// 异步状态机轮询处理
void RelayControl_ProcessAsyncOperations(void);

// 检查异步操作状态
uint8_t RelayControl_IsOperationInProgress(uint8_t channelNum);
uint8_t RelayControl_GetOperationResult(uint8_t channelNum);

// ================== 兼容性函数（阻塞版本，保持向后兼容） ===================

// 通道控制函数（阻塞版本，保持兼容性）
uint8_t RelayControl_OpenChannel(uint8_t channelNum);   // 开启指定通道
uint8_t RelayControl_CloseChannel(uint8_t channelNum);  // 关闭指定通道

// ================== 状态检测函数 ===================

// 状态检测函数
RelayState_t RelayControl_GetChannelState(uint8_t channelNum); // 获取通道状态
uint8_t RelayControl_CheckChannelFeedback(uint8_t channelNum); // 检查通道反馈

// 互锁检查函数
uint8_t RelayControl_CheckInterlock(uint8_t channelNum); // 检查通道互锁

// 错误处理函数
uint8_t RelayControl_GetLastError(uint8_t channelNum);   // 获取最后一次错误
void RelayControl_ClearError(uint8_t channelNum);        // 清除错误状态

// ================== 中断处理函数 ===================

// 增强的使能信号处理函数（防抖+干扰检测）
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state);

// 优先级中断处理
void RelayControl_ProcessPendingActions(void);

// 干扰检测和状态验证
uint8_t RelayControl_ValidateStateChange(uint8_t k1_en, uint8_t k2_en, uint8_t k3_en);
uint8_t RelayControl_GetHighestPriorityInterrupt(void);

// ================== 调试和统计函数 ===================

// 获取异步操作统计信息
void RelayControl_GetAsyncStatistics(uint32_t* total_ops, uint32_t* completed_ops, uint32_t* failed_ops);

// 获取干扰检测统计
void RelayControl_GetInterferenceStatistics(uint32_t* interference_count, uint32_t* filtered_interrupts);

// 错误代码定义
#define RELAY_ERR_NONE              0x00    // 无错误
#define RELAY_ERR_INTERLOCK         0x01    // 互锁错误
#define RELAY_ERR_FEEDBACK          0x02    // 反馈错误
#define RELAY_ERR_TIMEOUT           0x03    // 超时错误
#define RELAY_ERR_INVALID_CHANNEL   0x04    // 无效通道
#define RELAY_ERR_INVALID_STATE     0x05    // 无效状态
#define RELAY_ERR_POWER_FAILURE     0x06    // 电源异常错误
#define RELAY_ERR_HARDWARE_FAILURE  0x07    // 硬件反馈异常错误
#define RELAY_ERR_OPERATION_BUSY    0x08    // 操作忙（异步操作进行中）
#define RELAY_ERR_INTERFERENCE      0x09    // 检测到干扰

// 时间常量定义
#define RELAY_PULSE_WIDTH           500     // 控制脉冲宽度(ms)
#define RELAY_FEEDBACK_DELAY        500     // 反馈检测延时(ms)
#define RELAY_ASYNC_PULSE_WIDTH     100     // 异步模式脉冲宽度(ms) - 减少阻塞时间
#define RELAY_ASYNC_FEEDBACK_DELAY  100     // 异步模式反馈检测延时(ms)

// ================== 异步操作状态查询 ===================
uint8_t RelayControl_IsOperationInProgress(uint8_t channelNum);
uint8_t RelayControl_GetOperationResult(uint8_t channelNum);
void RelayControl_GetAsyncStatistics(uint32_t* total_ops, uint32_t* completed_ops, uint32_t* failed_ops);

// ================== 智能屏蔽接口（用于安全监控） ===================
/**
  * @brief  检查指定通道是否正在进行异步操作
  * @param  channelNum: 通道号(1-3)
  * @retval 1:正在操作 0:空闲状态
  * @note   用于安全监控模块判断是否需要屏蔽相关异常检测
  */
uint8_t RelayControl_IsChannelBusy(uint8_t channelNum);

/**
  * @brief  获取指定通道当前的异步操作类型
  * @param  channelNum: 通道号(1-3)
  * @retval 1:开启操作 0:关闭操作 255:无操作
  * @note   用于安全监控模块精确屏蔽相关异常类型
  */
uint8_t RelayControl_GetChannelOperationType(uint8_t channelNum);

#ifdef __cplusplus
}
#endif

#endif /* __RELAY_CONTROL_H__ */ 



