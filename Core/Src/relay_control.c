#include "relay_control.h"
#include "gpio_control.h"
#include "safety_monitor.h"
#include "smart_delay.h"  // 智能延时函数
#include "usart.h"
#include "log_system.h"   // 日志系统
#include <stdio.h>

/************************************************************
 * 继电器控制模块源文件 - 异步状态机版本
 * 实现三通道继电器的异步开启/关闭、中断防抖、干扰检测、优先级处理
 * 彻底消除主循环阻塞，提高系统响应性和稳定性
 ************************************************************/

// 三个继电器通道实例
static RelayChannel_t relayChannels[3];

// ================== 异步状态机变量 ===================
// 异步操作控制数组
static RelayAsyncOperation_t g_async_operations[MAX_ASYNC_OPERATIONS];

// 异步操作统计
static uint32_t g_async_total_operations = 0;
static uint32_t g_async_completed_operations = 0;
static uint32_t g_async_failed_operations = 0;

// ================== 增强防抖和干扰检测变量（轮询模式） ===================
// 防抖计时器（用于轮询模式）
static uint32_t g_last_poll_time[3] = {0, 0, 0};
static uint32_t g_debounce_count[3] = {0, 0, 0};

// 干扰检测过滤器
static InterferenceFilter_t g_interference_filter = {0};

// 干扰检测统计
static uint32_t g_interference_total_count = 0;
static uint32_t g_filtered_interrupts_count = 0;

/**
  * @brief  继电器控制模块初始化
  * @retval 无
  */
void RelayControl_Init(void)
{
    for(uint8_t i = 0; i < 3; i++) {
        relayChannels[i].channelNum = i + 1;
        relayChannels[i].state = RELAY_STATE_OFF;
        relayChannels[i].lastActionTime = 0;
        relayChannels[i].errorCode = RELAY_ERR_NONE;
        
        // 初始化所有继电器控制信号为高电平（三极管截止，继电器不动作）
        switch(i+1) {
            case 1:
                GPIO_SetK1_1_ON(1); GPIO_SetK1_1_OFF(1);
                GPIO_SetK1_2_ON(1); GPIO_SetK1_2_OFF(1);
                break;
            case 2:
                GPIO_SetK2_1_ON(1); GPIO_SetK2_1_OFF(1);
                GPIO_SetK2_2_ON(1); GPIO_SetK2_2_OFF(1);
                break;
            case 3:
                GPIO_SetK3_1_ON(1); GPIO_SetK3_1_OFF(1);
                GPIO_SetK3_2_ON(1); GPIO_SetK3_2_OFF(1);
                break;
        }
    }
    
    // ================== 初始化异步状态机 ===================
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        g_async_operations[i].state = RELAY_ASYNC_IDLE;
        g_async_operations[i].channel = 0;
        g_async_operations[i].operation = 0;
        g_async_operations[i].start_time = 0;
        g_async_operations[i].phase_start_time = 0;
        g_async_operations[i].result = RELAY_ERR_NONE;
        g_async_operations[i].error_code = RELAY_ERR_NONE;
    }
    
    // ================== 初始化防抖和干扰检测 ===================
    for(uint8_t i = 0; i < 3; i++) {
        g_last_poll_time[i] = 0;
        g_debounce_count[i] = 0;
        g_interference_filter.last_interrupt_time[i] = 0;
    }
    g_interference_filter.simultaneous_count = 0;
    g_interference_filter.interference_detected = 0;
    g_interference_filter.last_interference_time = 0;
    
    // 重置统计计数器
    g_async_total_operations = 0;
    g_async_completed_operations = 0;
    g_async_failed_operations = 0;
    g_interference_total_count = 0;
    g_filtered_interrupts_count = 0;
    
    DEBUG_Printf("继电器控制模块初始化完成 - 异步状态机版本\r\n");
    DEBUG_Printf("支持功能: 异步操作、中断防抖、干扰检测、优先级处理\r\n");
    
    // 继电器模块初始化完成，日志记录功能已删除
}

/**
  * @brief  检查通道互锁状态
  * @param  channelNum: 通道号(1-3)
  * @retval 0:无互锁 1:有互锁
  */
uint8_t RelayControl_CheckInterlock(uint8_t channelNum)
{
    uint8_t result = 0;
    switch(channelNum) {
        case 1:
            if(!GPIO_ReadK2_EN() || !GPIO_ReadK3_EN() ||
               GPIO_ReadK2_1_STA() || GPIO_ReadK2_2_STA() || GPIO_ReadSW2_STA() ||
               GPIO_ReadK3_1_STA() || GPIO_ReadK3_2_STA() || GPIO_ReadSW3_STA())
                result = 1;
                break;
        case 2:
            if(!GPIO_ReadK1_EN() || !GPIO_ReadK3_EN() ||
               GPIO_ReadK1_1_STA() || GPIO_ReadK1_2_STA() || GPIO_ReadSW1_STA() ||
               GPIO_ReadK3_1_STA() || GPIO_ReadK3_2_STA() || GPIO_ReadSW3_STA())
                result = 1;
                break;
        case 3:
            if(!GPIO_ReadK1_EN() || !GPIO_ReadK2_EN() ||
               GPIO_ReadK1_1_STA() || GPIO_ReadK1_2_STA() || GPIO_ReadSW1_STA() ||
               GPIO_ReadK2_1_STA() || GPIO_ReadK2_2_STA() || GPIO_ReadSW2_STA())
                result = 1;
                break;
            default:
            result = 1;
            break;
    }
    return result;
}

/**
  * @brief  检查通道反馈状态（已废弃 - 改为直接检查硬件状态）
  * @param  channelNum: 通道号(1-3)
  * @retval 0:正常 1:异常
  * @note   此函数已废弃，现在使用直接硬件状态检查，避免依赖内部状态导致的逻辑混乱
  */
uint8_t RelayControl_CheckChannelFeedback(uint8_t channelNum)
{
    // 此函数已废弃，保留以保持兼容性
    (void)channelNum;
    return 0;  // 始终返回正常，避免影响其他模块
}





/**
  * @brief  获取通道状态
  * @param  channelNum: 通道号(1-3)
  * @retval 通道状态
  */
RelayState_t RelayControl_GetChannelState(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_STATE_ERROR;
    return relayChannels[channelNum - 1].state;
}

/**
  * @brief  获取最后一次错误代码
  * @param  channelNum: 通道号(1-3)
  * @retval 错误代码
  */
uint8_t RelayControl_GetLastError(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_ERR_INVALID_CHANNEL;
    return relayChannels[channelNum - 1].errorCode;
}

/**
  * @brief  清除错误状态
  * @param  channelNum: 通道号(1-3)
  * @retval 无
  */
void RelayControl_ClearError(uint8_t channelNum)
{
    if(channelNum >= 1 && channelNum <= 3) {
        relayChannels[channelNum - 1].errorCode = RELAY_ERR_NONE;
        if(relayChannels[channelNum - 1].state == RELAY_STATE_ERROR)
            relayChannels[channelNum - 1].state = RELAY_STATE_OFF;
    }
}

/**
  * @brief  处理使能信号变化（轮询模式：直接执行操作）
  * @param  channelNum: 通道号(1-3)
  * @param  state: 信号状态（0=低电平=开启，1=高电平=关闭）
  * @retval 无
  * @note   轮询模式：防抖+干扰检测+直接执行继电器操作
  */
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state)
{
    uint32_t current_time = HAL_GetTick();
    
    // 参数验证
    if(channelNum < 1 || channelNum > 3) {
        g_filtered_interrupts_count++;
        return;
    }
    
    uint8_t ch_idx = channelNum - 1; // 转换为0-2索引
    
    // ================== 防抖检查 ===================
    // 50ms内的重复调用直接丢弃
    if(current_time - g_last_poll_time[ch_idx] < INTERRUPT_DEBOUNCE_TIME_MS) {
        g_filtered_interrupts_count++;
        g_debounce_count[ch_idx]++;
        return;
    }
    
    // ================== 干扰检测 ===================
    // 检查是否在很短时间内多个通道同时变化（疑似干扰）
    uint8_t simultaneous_count = 0;
    for(uint8_t i = 0; i < 3; i++) {
        if(current_time - g_interference_filter.last_interrupt_time[i] < INTERFERENCE_DETECT_TIME_MS) {
            simultaneous_count++;
        }
    }
    
    // 如果检测到多个通道同时变化，可能是干扰
    if(simultaneous_count >= 2) {
        g_interference_filter.interference_detected = 1;
        g_interference_filter.simultaneous_count = simultaneous_count + 1;
        g_interference_filter.last_interference_time = current_time;
        g_interference_total_count++;
        g_filtered_interrupts_count++;
        
        DEBUG_Printf("?? [干扰过滤] 通道%d状态变化被过滤：检测到%d个通道同时变化\r\n", 
                    channelNum, simultaneous_count + 1);
        return;
    }
    
    // ================== 系统安全检查 ===================
    // 检查系统状态，如果处于报警状态则停止处理
    extern SystemState_t SystemControl_GetState(void);
    SystemState_t system_state = SystemControl_GetState();
    if(system_state == SYSTEM_STATE_ALARM) {
        DEBUG_Printf("?? [安全阻止] 通道%d操作被阻止：系统处于报警状态\r\n", channelNum);
        return;
    }
    
    // 检查是否存在关键异常
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_B) ||  // K1_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_C) ||  // K2_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_D) ||  // K3_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_E) ||  // K1_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_F) ||  // K2_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_G) ||  // K3_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {  // 外部电源异常（DC_CTRL）
        DEBUG_Printf("?? [安全阻止] 通道%d操作被阻止：检测到关键异常\r\n", channelNum);
        return;
    }
    
    // ================== 更新时间记录 ===================
    g_last_poll_time[ch_idx] = current_time;
    g_interference_filter.last_interrupt_time[ch_idx] = current_time;
    
    // ================== 直接执行继电器操作 ===================
    // 读取当前硬件状态
    uint8_t k_sta_all;
    switch(channelNum) {
        case 1:
            k_sta_all = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
            break;
        case 2:
            k_sta_all = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
            break;
        case 3:
            k_sta_all = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
            break;
        default:
            return;
    }
    
    DEBUG_Printf("? [轮询触发] 通道%d: K_EN=%d, STA=%d\r\n", channelNum, state, k_sta_all);
    
    // 执行操作逻辑：检测K_EN为0且STA也为0，则启动异步开启操作
    if(state == 0 && k_sta_all == 0) {
        DEBUG_Printf("? [异步开启] 通道%d: K_EN=0且STA=0，启动异步开启\r\n", channelNum);
        uint8_t result = RelayControl_StartOpenChannel(channelNum);
        if(result == RELAY_ERR_NONE) {
            DEBUG_Printf("? 通道%d异步开启操作已启动\r\n", channelNum);
        } else {
            DEBUG_Printf("? 通道%d异步开启启动失败，错误码=%d\r\n", channelNum, result);
        }
    }
    // 执行操作逻辑：检测K_EN为1且STA也为1，则启动异步关闭操作
    else if(state == 1 && k_sta_all == 1) {
        DEBUG_Printf("? [异步关闭] 通道%d: K_EN=1且STA=1，启动异步关闭\r\n", channelNum);
        uint8_t result = RelayControl_StartCloseChannel(channelNum);
        if(result == RELAY_ERR_NONE) {
            DEBUG_Printf("? 通道%d异步关闭操作已启动\r\n", channelNum);
        } else {
            DEBUG_Printf("? 通道%d异步关闭启动失败，错误码=%d\r\n", channelNum, result);
        }
    } else {
        DEBUG_Printf("?? [状态检查] 通道%d: K_EN=%d, STA=%d，条件不满足，无操作\r\n", 
                    channelNum, state, k_sta_all);
    }
}

// ================== 状态验证和优先级处理函数 ===================

/**
  * @brief  验证状态变化的合理性（检测干扰）
  * @param  k1_en: K1_EN当前状态
  * @param  k2_en: K2_EN当前状态  
  * @param  k3_en: K3_EN当前状态
  * @retval 1:状态变化合理 0:疑似干扰
  */
uint8_t RelayControl_ValidateStateChange(uint8_t k1_en, uint8_t k2_en, uint8_t k3_en)
{
    // 检查是否符合互锁逻辑：正常情况下最多只有一个通道的EN为低电平（使能）
    uint8_t en_active_count = (!k1_en) + (!k2_en) + (!k3_en);
    
    if(en_active_count > 1) {
        return 0; // 多通道同时使能，疑似干扰
    }
    
    // 可以添加更多验证逻辑，比如检查EN信号与STA反馈的一致性等
    return 1; // 状态变化合理
}

/**
  * @brief  获取最高优先级的中断（按时间顺序）
  * @retval 通道号(1-3)，如果没有中断则返回0
  */
uint8_t RelayControl_GetHighestPriorityInterrupt(void)
{
    // 删除中断标志位和时间变量，此函数不再适用
    return 0;
}

/**
  * @brief  处理K_EN中断标志并执行继电器动作（已废弃：改为轮询模式）
  * @retval 无
  * @note   此函数已废弃，K_EN信号现在通过轮询模式在gpio_control.c中处理
  */
void RelayControl_ProcessPendingActions(void)
{
    // 此函数已废弃，K_EN信号现在通过轮询模式处理
    // 保留空函数以保持兼容性
    return;
} 

// ================== 异步操作辅助函数 ===================

/**
  * @brief  查找空闲的异步操作槽位
  * @retval 槽位索引，如果没有空闲槽位则返回MAX_ASYNC_OPERATIONS
  */
static uint8_t FindFreeAsyncSlot(void)
{
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        if(g_async_operations[i].state == RELAY_ASYNC_IDLE) {
            return i;
        }
    }
    return MAX_ASYNC_OPERATIONS; // 没有空闲槽位
}

/**
  * @brief  查找指定通道的异步操作
  * @param  channelNum: 通道号(1-3)
  * @retval 槽位索引，如果没有找到则返回MAX_ASYNC_OPERATIONS
  */
static uint8_t FindAsyncOperationByChannel(uint8_t channelNum)
{
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        if(g_async_operations[i].state != RELAY_ASYNC_IDLE && 
           g_async_operations[i].channel == channelNum) {
            return i;
        }
    }
    return MAX_ASYNC_OPERATIONS;
}

// ================== 异步操作启动函数 ===================

/**
  * @brief  启动异步开启通道操作（非阻塞）
  * @param  channelNum: 通道号(1-3)
  * @retval 0:成功启动 其他:错误代码
  */
uint8_t RelayControl_StartOpenChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3) {
        return RELAY_ERR_INVALID_CHANNEL;
    }
    
    // 检查是否存在O类异常（电源异常）
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
        DEBUG_Printf("通道%d异步开启被阻止：检测到O类异常（电源异常）\r\n", channelNum);
        return RELAY_ERR_POWER_FAILURE;
    }
    
    // 检查该通道是否已有正在进行的操作
    if(FindAsyncOperationByChannel(channelNum) != MAX_ASYNC_OPERATIONS) {
        DEBUG_Printf("通道%d异步开启失败：操作正在进行中\r\n", channelNum);
        return RELAY_ERR_OPERATION_BUSY;
    }
    
    // 检查互锁
    if(RelayControl_CheckInterlock(channelNum)) {
        DEBUG_Printf("通道%d异步开启失败：互锁错误\r\n", channelNum);
        return RELAY_ERR_INTERLOCK;
    }
    
    // 查找空闲的异步操作槽位
    uint8_t slot = FindFreeAsyncSlot();
    if(slot == MAX_ASYNC_OPERATIONS) {
        DEBUG_Printf("通道%d异步开启失败：无可用操作槽位\r\n", channelNum);
        return RELAY_ERR_OPERATION_BUSY;
    }
    
    // 初始化异步操作
    g_async_operations[slot].state = RELAY_ASYNC_PULSE_START;
    g_async_operations[slot].channel = channelNum;
    g_async_operations[slot].operation = 1; // 1=开启
    g_async_operations[slot].start_time = HAL_GetTick();
    g_async_operations[slot].phase_start_time = HAL_GetTick();
    g_async_operations[slot].result = RELAY_ERR_NONE;
    g_async_operations[slot].error_code = RELAY_ERR_NONE;
    
    // 更新统计
    g_async_total_operations++;
    
    DEBUG_Printf("? 通道%d异步开启操作已启动（槽位%d）\r\n", channelNum, slot);
    
    return RELAY_ERR_NONE;
}

/**
  * @brief  启动异步关闭通道操作（非阻塞）
  * @param  channelNum: 通道号(1-3)
  * @retval 0:成功启动 其他:错误代码
  */
uint8_t RelayControl_StartCloseChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3) {
        return RELAY_ERR_INVALID_CHANNEL;
    }
    
    // 检查该通道是否已有正在进行的操作
    if(FindAsyncOperationByChannel(channelNum) != MAX_ASYNC_OPERATIONS) {
        DEBUG_Printf("通道%d异步关闭失败：操作正在进行中\r\n", channelNum);
        return RELAY_ERR_OPERATION_BUSY;
    }
    
    // 查找空闲的异步操作槽位
    uint8_t slot = FindFreeAsyncSlot();
    if(slot == MAX_ASYNC_OPERATIONS) {
        DEBUG_Printf("通道%d异步关闭失败：无可用操作槽位\r\n", channelNum);
        return RELAY_ERR_OPERATION_BUSY;
    }
    
    // 初始化异步操作
    g_async_operations[slot].state = RELAY_ASYNC_PULSE_START;
    g_async_operations[slot].channel = channelNum;
    g_async_operations[slot].operation = 0; // 0=关闭
    g_async_operations[slot].start_time = HAL_GetTick();
    g_async_operations[slot].phase_start_time = HAL_GetTick();
    g_async_operations[slot].result = RELAY_ERR_NONE;
    g_async_operations[slot].error_code = RELAY_ERR_NONE;
    
    // 更新统计
    g_async_total_operations++;
    
    DEBUG_Printf("? 通道%d异步关闭操作已启动（槽位%d）\r\n", channelNum, slot);
    
    return RELAY_ERR_NONE;
}

// ================== 异步状态机轮询处理 ===================

/**
  * @brief  异步状态机轮询处理（在主循环中高频调用）
  * @retval 无
  * @note   这是整个异步架构的核心，负责推进所有异步操作的状态机
  */
void RelayControl_ProcessAsyncOperations(void)
{
    uint32_t current_time = HAL_GetTick();
    
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        RelayAsyncOperation_t* op = &g_async_operations[i];
        
        // 跳过空闲槽位
        if(op->state == RELAY_ASYNC_IDLE) {
            continue;
        }
        
        uint8_t channelNum = op->channel;
        uint32_t elapsed = current_time - op->phase_start_time;
        uint8_t hardware_state_correct = 0; // 预先声明，避免switch中声明变量的警告
        
        switch(op->state) {
            case RELAY_ASYNC_PULSE_START:
                // 开始脉冲输出
                DEBUG_Printf("? 通道%d %s脉冲开始\r\n", channelNum, op->operation ? "开启" : "关闭");
                
                if(op->operation) {
                    // 开启操作：输出ON脉冲
                    switch(channelNum) {
                        case 1: GPIO_SetK1_1_ON(0); GPIO_SetK1_2_ON(0); break;
                        case 2: GPIO_SetK2_1_ON(0); GPIO_SetK2_2_ON(0); break;
                        case 3: GPIO_SetK3_1_ON(0); GPIO_SetK3_2_ON(0); break;
                    }
                } else {
                    // 关闭操作：输出OFF脉冲
                    switch(channelNum) {
                        case 1: GPIO_SetK1_1_OFF(0); GPIO_SetK1_2_OFF(0); break;
                        case 2: GPIO_SetK2_1_OFF(0); GPIO_SetK2_2_OFF(0); break;
                        case 3: GPIO_SetK3_1_OFF(0); GPIO_SetK3_2_OFF(0); break;
                    }
                }
                
                op->state = RELAY_ASYNC_PULSE_ACTIVE;
                op->phase_start_time = current_time;
                break;
                
            case RELAY_ASYNC_PULSE_ACTIVE:
                // 脉冲进行中，等待脉冲宽度时间
                if(elapsed >= RELAY_ASYNC_PULSE_WIDTH) {
                    DEBUG_Printf("? 通道%d脉冲宽度达到，结束脉冲\r\n", channelNum);
                    op->state = RELAY_ASYNC_PULSE_END;
                    op->phase_start_time = current_time;
                }
                break;
                
            case RELAY_ASYNC_PULSE_END:
                // 结束脉冲输出，恢复高电平
                if(op->operation) {
                    // 开启操作：恢复ON控制信号为高电平
                    switch(channelNum) {
                        case 1: GPIO_SetK1_1_ON(1); GPIO_SetK1_2_ON(1); break;
                        case 2: GPIO_SetK2_1_ON(1); GPIO_SetK2_2_ON(1); break;
                        case 3: GPIO_SetK3_1_ON(1); GPIO_SetK3_2_ON(1); break;
                    }
                } else {
                    // 关闭操作：恢复OFF控制信号为高电平
                    switch(channelNum) {
                        case 1: GPIO_SetK1_1_OFF(1); GPIO_SetK1_2_OFF(1); break;
                        case 2: GPIO_SetK2_1_OFF(1); GPIO_SetK2_2_OFF(1); break;
                        case 3: GPIO_SetK3_1_OFF(1); GPIO_SetK3_2_OFF(1); break;
                    }
                }
                
                DEBUG_Printf("? 通道%d脉冲结束，等待反馈\r\n", channelNum);
                op->state = RELAY_ASYNC_FEEDBACK_WAIT;
                op->phase_start_time = current_time;
                break;
                
            case RELAY_ASYNC_FEEDBACK_WAIT:
                // 等待反馈延时
                if(elapsed >= RELAY_ASYNC_FEEDBACK_DELAY) {
                    DEBUG_Printf("? 通道%d开始检查反馈\r\n", channelNum);
                    op->state = RELAY_ASYNC_FEEDBACK_CHECK;
                    op->phase_start_time = current_time;
                }
                break;
                
            case RELAY_ASYNC_FEEDBACK_CHECK:
                // 检查硬件反馈
                hardware_state_correct = 0;
                
                if(op->operation) {
                    // 开启操作：检查是否所有状态都为高
                    switch(channelNum) {
                        case 1:
                            hardware_state_correct = (GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA());
                            break;
                        case 2:
                            hardware_state_correct = (GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA());
                            break;
                        case 3:
                            hardware_state_correct = (GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA());
                            break;
                    }
                } else {
                    // 关闭操作：检查是否所有状态都为低
                    switch(channelNum) {
                        case 1:
                            hardware_state_correct = (!GPIO_ReadK1_1_STA() && !GPIO_ReadK1_2_STA() && !GPIO_ReadSW1_STA());
                            break;
                        case 2:
                            hardware_state_correct = (!GPIO_ReadK2_1_STA() && !GPIO_ReadK2_2_STA() && !GPIO_ReadSW2_STA());
                            break;
                        case 3:
                            hardware_state_correct = (!GPIO_ReadK3_1_STA() && !GPIO_ReadK3_2_STA() && !GPIO_ReadSW3_STA());
                            break;
                    }
                }
                
                if(hardware_state_correct) {
                    // 操作成功
                    relayChannels[channelNum-1].state = op->operation ? RELAY_STATE_ON : RELAY_STATE_OFF;
                    relayChannels[channelNum-1].errorCode = RELAY_ERR_NONE;
                    relayChannels[channelNum-1].lastActionTime = current_time;
                    
                    op->result = RELAY_ERR_NONE;
                    op->state = RELAY_ASYNC_COMPLETE;
                    g_async_completed_operations++;
                    
                    DEBUG_Printf("? 通道%d异步%s操作成功完成\r\n", channelNum, op->operation ? "开启" : "关闭");
                } else {
                    // 操作失败
                    relayChannels[channelNum-1].state = RELAY_STATE_ERROR;
                    relayChannels[channelNum-1].errorCode = RELAY_ERR_HARDWARE_FAILURE;
                    
                    op->result = RELAY_ERR_HARDWARE_FAILURE;
                    op->error_code = RELAY_ERR_HARDWARE_FAILURE;
                    op->state = RELAY_ASYNC_ERROR;
                    g_async_failed_operations++;
                    
                    DEBUG_Printf("? 通道%d异步%s操作失败 - 硬件反馈异常\r\n", channelNum, op->operation ? "开启" : "关闭");
                }
                break;
                
            case RELAY_ASYNC_COMPLETE:
            case RELAY_ASYNC_ERROR:
                // 操作完成或出错，重置槽位
                op->state = RELAY_ASYNC_IDLE;
                op->channel = 0;
                break;
                
            default:
                // 未知状态，重置
                DEBUG_Printf("?? 通道%d异步操作遇到未知状态%d，重置\r\n", channelNum, op->state);
                op->state = RELAY_ASYNC_IDLE;
                op->channel = 0;
                g_async_failed_operations++;
                break;
        }
    }
} 

// ================== 异步状态查询函数 ===================

/**
  * @brief  检查指定通道是否有异步操作正在进行
  * @param  channelNum: 通道号(1-3)
  * @retval 1:有操作进行中 0:无操作
  */
uint8_t RelayControl_IsOperationInProgress(uint8_t channelNum)
{
    return (FindAsyncOperationByChannel(channelNum) != MAX_ASYNC_OPERATIONS);
}

/**
  * @brief  获取指定通道的异步操作结果
  * @param  channelNum: 通道号(1-3)
  * @retval 操作结果错误代码
  */
uint8_t RelayControl_GetOperationResult(uint8_t channelNum)
{
    uint8_t slot = FindAsyncOperationByChannel(channelNum);
    if(slot != MAX_ASYNC_OPERATIONS) {
        return g_async_operations[slot].result;
    }
    return RELAY_ERR_INVALID_CHANNEL;
}

// ================== 统计和调试函数 ===================

/**
  * @brief  获取异步操作统计信息
  * @param  total_ops: 总操作数
  * @param  completed_ops: 完成操作数
  * @param  failed_ops: 失败操作数
  * @retval 无
  */
void RelayControl_GetAsyncStatistics(uint32_t* total_ops, uint32_t* completed_ops, uint32_t* failed_ops)
{
    if(total_ops) *total_ops = g_async_total_operations;
    if(completed_ops) *completed_ops = g_async_completed_operations;
    if(failed_ops) *failed_ops = g_async_failed_operations;
}

/**
  * @brief  获取干扰检测统计
  * @param  interference_count: 检测到的干扰次数
  * @param  filtered_interrupts: 被过滤的中断次数
  * @retval 无
  */
void RelayControl_GetInterferenceStatistics(uint32_t* interference_count, uint32_t* filtered_interrupts)
{
    if(interference_count) *interference_count = g_interference_total_count;
    if(filtered_interrupts) *filtered_interrupts = g_filtered_interrupts_count;
}

// ================== 智能屏蔽接口实现（用于安全监控） ===================

/**
  * @brief  检查指定通道是否正在进行异步操作
  * @param  channelNum: 通道号(1-3)
  * @retval 1:正在操作 0:空闲状态
  * @note   用于安全监控模块判断是否需要屏蔽相关异常检测
  */
uint8_t RelayControl_IsChannelBusy(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3) {
        return 0; // 无效通道号
    }
    
    // 检查该通道是否有正在进行的异步操作
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        RelayAsyncOperation_t* op = &g_async_operations[i];
        if(op->state != RELAY_ASYNC_IDLE && op->channel == channelNum) {
            return 1; // 找到正在进行的操作
        }
    }
    
    return 0; // 该通道空闲
}

/**
  * @brief  获取指定通道当前的异步操作类型
  * @param  channelNum: 通道号(1-3)
  * @retval 1:开启操作 0:关闭操作 255:无操作
  * @note   用于安全监控模块精确屏蔽相关异常类型
  */
uint8_t RelayControl_GetChannelOperationType(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3) {
        return 255; // 无效通道号
    }
    
    // 查找该通道正在进行的异步操作
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        RelayAsyncOperation_t* op = &g_async_operations[i];
        if(op->state != RELAY_ASYNC_IDLE && op->channel == channelNum) {
            return op->operation ? 1 : 0; // 1:开启操作 0:关闭操作
        }
    }
    
    return 255; // 无操作
}

// ================== 兼容性函数（阻塞版本） ===================

/**
  * @brief  开启指定通道（阻塞版本，保持向后兼容）
  * @param  channelNum: 通道号(1-3)
  * @retval 0:成功 其他:错误代码
  * @note   内部使用异步操作，但等待操作完成后返回，保持与原有代码的兼容性
  */
uint8_t RelayControl_OpenChannel(uint8_t channelNum)
{
    // 启动异步操作
    uint8_t result = RelayControl_StartOpenChannel(channelNum);
    if(result != RELAY_ERR_NONE) {
        return result;
    }
    
    DEBUG_Printf("? 通道%d阻塞开启等待异步操作完成\r\n", channelNum);
    
    // 等待异步操作完成
    uint32_t start_time = HAL_GetTick();
    const uint32_t timeout = 2000; // 2秒超时
    
    while(RelayControl_IsOperationInProgress(channelNum)) {
        // 在等待期间继续轮询异步操作
        RelayControl_ProcessAsyncOperations();
        
        // 检查超时
        if(HAL_GetTick() - start_time > timeout) {
            DEBUG_Printf("? 通道%d阻塞开启操作超时\r\n", channelNum);
            return RELAY_ERR_TIMEOUT;
        }
        
        // 短暂延时，避免CPU过度占用
        HAL_Delay(1);
    }
    
    // 获取最终结果
    return RelayControl_GetOperationResult(channelNum);
}

/**
  * @brief  关闭指定通道（阻塞版本，保持向后兼容）
  * @param  channelNum: 通道号(1-3)
  * @retval 0:成功 其他:错误代码
  * @note   内部使用异步操作，但等待操作完成后返回，保持与原有代码的兼容性
  */
uint8_t RelayControl_CloseChannel(uint8_t channelNum)
{
    // 启动异步操作
    uint8_t result = RelayControl_StartCloseChannel(channelNum);
    if(result != RELAY_ERR_NONE) {
        return result;
    }
    
    DEBUG_Printf("? 通道%d阻塞关闭等待异步操作完成\r\n", channelNum);
    
    // 等待异步操作完成
    uint32_t start_time = HAL_GetTick();
    const uint32_t timeout = 2000; // 2秒超时
    
    while(RelayControl_IsOperationInProgress(channelNum)) {
        // 在等待期间继续轮询异步操作
        RelayControl_ProcessAsyncOperations();
        
        // 检查超时
        if(HAL_GetTick() - start_time > timeout) {
            DEBUG_Printf("? 通道%d阻塞关闭操作超时\r\n", channelNum);
            return RELAY_ERR_TIMEOUT;
        }
        
        // 短暂延时，避免CPU过度占用
        HAL_Delay(1);
    }
    
    // 获取最终结果
    return RelayControl_GetOperationResult(channelNum);
} 





