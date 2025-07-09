#include "relay_control.h"
#include "gpio_control.h"
#include "safety_monitor.h"
#include "smart_delay.h"  // 智能延时函数
#include "usart.h"
#include "log_system.h"   // 日志系统
#include <stdio.h>

/************************************************************
 * 继电器控制模块源文件
 * 实现三通道继电器的开启/关闭、三重检测、互锁检查、500ms脉冲、状态反馈检测、异常处理与状态管理
 ************************************************************/

// 三个继电器通道实例
static RelayChannel_t relayChannels[3];

// ================== 中断标志位变量 ===================
// K_EN信号中断标志位（类似DC_CTRL中断处理方式）
static volatile uint8_t g_k1_en_interrupt_flag = 0;
static volatile uint8_t g_k2_en_interrupt_flag = 0;
static volatile uint8_t g_k3_en_interrupt_flag = 0;
static volatile uint32_t g_k1_en_interrupt_time = 0;
static volatile uint32_t g_k2_en_interrupt_time = 0;
static volatile uint32_t g_k3_en_interrupt_time = 0;

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
    DEBUG_Printf("继电器控制模块初始化完成，所有控制信号设为高电平（三极管截止）\r\n");
    
    // 记录继电器模块初始化日志
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "继电器控制模块初始化完成");
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
  * @brief  打开继电器通道
  * @param  channelNum: 通道号(1-3)
  * @retval 0:成功 其他:错误代码
  */
uint8_t RelayControl_OpenChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_ERR_INVALID_CHANNEL;
    
    // 检查是否存在O类异常（电源异常）
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
        DEBUG_Printf("通道%d开启被阻止：检测到O类异常（电源异常）\r\n", channelNum);
        
        // 记录电源异常阻止开启日志
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "通道%d开启被阻止-电源异常", channelNum);
        LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_POWER_FAILURE, log_msg);
        
        return RELAY_ERR_POWER_FAILURE;
    }
    
    uint8_t idx = channelNum - 1;
    // 检查互锁
    if(RelayControl_CheckInterlock(channelNum)) {
        relayChannels[idx].errorCode = RELAY_ERR_INTERLOCK;
        DEBUG_Printf("通道%d互锁错误\r\n", channelNum);
        
        // 记录互锁错误日志（这是安全关键异常）
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "通道%d互锁错误", channelNum);
        LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_A, log_msg);
        
        return RELAY_ERR_INTERLOCK;
    }
    
    DEBUG_Printf("通道%d开启中 - 输出500ms低电平脉冲（三极管导通）\r\n", channelNum);
    
    // 输出500ms低电平脉冲（三极管导通，继电器动作）
    switch(channelNum) {
        case 1:
            GPIO_SetK1_1_ON(0); GPIO_SetK1_2_ON(0);
            break;
        case 2:
            GPIO_SetK2_1_ON(0); GPIO_SetK2_2_ON(0);
            break;
        case 3:
            GPIO_SetK3_1_ON(0); GPIO_SetK3_2_ON(0);
            break;
    }
    relayChannels[idx].lastActionTime = HAL_GetTick();
    SmartDelayWithDebug(RELAY_PULSE_WIDTH, "继电器开启脉冲");
    
    // 脉冲结束，恢复高电平（三极管截止）
    switch(channelNum) {
        case 1:
            GPIO_SetK1_1_ON(1); GPIO_SetK1_2_ON(1);
            break;
        case 2:
            GPIO_SetK2_1_ON(1); GPIO_SetK2_2_ON(1);
            break;
        case 3:
            GPIO_SetK3_1_ON(1); GPIO_SetK3_2_ON(1);
            break;
    }
    
    DEBUG_Printf("通道%d脉冲输出完成，等待反馈\r\n", channelNum);
    
    // 延时500ms后，检查硬件反馈再决定内部状态
    SmartDelayWithDebug(RELAY_FEEDBACK_DELAY, "继电器开启反馈等待");
    
    // 检查硬件是否真正开启（直接检查硬件状态，不依赖内部状态）
    uint8_t hardware_opened = 0;
    switch(channelNum) {
        case 1:
            hardware_opened = (GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA());
            break;
        case 2:
            hardware_opened = (GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA());
            break;
        case 3:
            hardware_opened = (GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA());
            break;
    }
    
    if(hardware_opened) {
        // 硬件成功开启，设置内部状态为ON
        relayChannels[idx].state = RELAY_STATE_ON;
        relayChannels[idx].errorCode = RELAY_ERR_NONE;
        DEBUG_Printf("通道%d开启成功\r\n", channelNum);
        
        return RELAY_ERR_NONE;
    } else {
        // 硬件开启失败，记录错误
        relayChannels[idx].state = RELAY_STATE_ERROR;
        relayChannels[idx].errorCode = RELAY_ERR_HARDWARE_FAILURE;
        DEBUG_Printf("通道%d开启失败 - 硬件反馈异常\r\n", channelNum);
        
        // 精确检测每个继电器和接触器的异常状态并记录相应日志
        char log_msg[64];
        uint8_t k1_sta = 0, k2_sta = 0, sw_sta = 0;
        
        switch(channelNum) {
            case 1:
                k1_sta = GPIO_ReadK1_1_STA();
                k2_sta = GPIO_ReadK1_2_STA();
                sw_sta = GPIO_ReadSW1_STA();
                
                if(!k1_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d K1_1继电器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_B, log_msg);
                }
                if(!k2_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d K1_2继电器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_E, log_msg);
                }
                if(!sw_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d SW1接触器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_H, log_msg);
                }
                break;
                
            case 2:
                k1_sta = GPIO_ReadK2_1_STA();
                k2_sta = GPIO_ReadK2_2_STA();
                sw_sta = GPIO_ReadSW2_STA();
                
                if(!k1_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d K2_1继电器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_C, log_msg);
                }
                if(!k2_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d K2_2继电器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_F, log_msg);
                }
                if(!sw_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d SW2接触器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_I, log_msg);
                }
                break;
                
            case 3:
                k1_sta = GPIO_ReadK3_1_STA();
                k2_sta = GPIO_ReadK3_2_STA();
                sw_sta = GPIO_ReadSW3_STA();
                
                if(!k1_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d K3_1继电器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_D, log_msg);
                }
                if(!k2_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d K3_2继电器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_G, log_msg);
                }
                if(!sw_sta) {
                    snprintf(log_msg, sizeof(log_msg), "通道%d SW3接触器反馈异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_J, log_msg);
                }
                break;
        }
        
        return RELAY_ERR_HARDWARE_FAILURE;
    }
}

/**
  * @brief  关闭继电器通道
  * @param  channelNum: 通道号(1-3)
  * @retval 0:成功 其他:错误代码
  */
uint8_t RelayControl_CloseChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_ERR_INVALID_CHANNEL;
    
    uint8_t idx = channelNum - 1;
    DEBUG_Printf("通道%d关闭中 - 输出500ms低电平脉冲（三极管导通）\r\n", channelNum);
    
    // 输出500ms低电平脉冲（三极管导通，继电器动作）
    switch(channelNum) {
        case 1:
            GPIO_SetK1_1_OFF(0); GPIO_SetK1_2_OFF(0);
            break;
        case 2:
            GPIO_SetK2_1_OFF(0); GPIO_SetK2_2_OFF(0);
            break;
        case 3:
            GPIO_SetK3_1_OFF(0); GPIO_SetK3_2_OFF(0);
            break;
    }
    relayChannels[idx].lastActionTime = HAL_GetTick();
    SmartDelayWithDebug(RELAY_PULSE_WIDTH, "继电器关闭脉冲");
    
    // 脉冲结束，恢复高电平（三极管截止）
    switch(channelNum) {
        case 1:
            GPIO_SetK1_1_OFF(1); GPIO_SetK1_2_OFF(1);
            break;
        case 2:
            GPIO_SetK2_1_OFF(1); GPIO_SetK2_2_OFF(1);
            break;
        case 3:
            GPIO_SetK3_1_OFF(1); GPIO_SetK3_2_OFF(1);
            break;
    }
    
    DEBUG_Printf("通道%d脉冲输出完成，等待反馈\r\n", channelNum);
    
    // 延时500ms后，检查硬件反馈再决定内部状态
    SmartDelayWithDebug(RELAY_FEEDBACK_DELAY, "继电器关闭反馈等待");
    
    // 检查硬件是否真正关闭（直接检查硬件状态，不依赖内部状态）
    uint8_t hardware_closed = 0;
    switch(channelNum) {
        case 1:
            hardware_closed = (!GPIO_ReadK1_1_STA() && !GPIO_ReadK1_2_STA() && !GPIO_ReadSW1_STA());
            break;
        case 2:
            hardware_closed = (!GPIO_ReadK2_1_STA() && !GPIO_ReadK2_2_STA() && !GPIO_ReadSW2_STA());
            break;
        case 3:
            hardware_closed = (!GPIO_ReadK3_1_STA() && !GPIO_ReadK3_2_STA() && !GPIO_ReadSW3_STA());
            break;
    }
    
    if(hardware_closed) {
        // 硬件成功关闭，设置内部状态为OFF
        relayChannels[idx].state = RELAY_STATE_OFF;
        relayChannels[idx].errorCode = RELAY_ERR_NONE;
        DEBUG_Printf("通道%d关闭成功\r\n", channelNum);
        
        return RELAY_ERR_NONE;
    } else {
        // 硬件关闭失败，记录错误
        relayChannels[idx].state = RELAY_STATE_ERROR;
        relayChannels[idx].errorCode = RELAY_ERR_HARDWARE_FAILURE;
        DEBUG_Printf("通道%d关闭失败 - 硬件反馈异常\r\n", channelNum);
        
        // 精确检测每个继电器和接触器的异常状态并记录相应日志
        char log_msg[64];
        uint8_t k1_sta = 0, k2_sta = 0, sw_sta = 0;
        
        switch(channelNum) {
            case 1:
                k1_sta = GPIO_ReadK1_1_STA();
                k2_sta = GPIO_ReadK1_2_STA();
                sw_sta = GPIO_ReadSW1_STA();
                
                if(k1_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d K1_1继电器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_B, log_msg);
                }
                if(k2_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d K1_2继电器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_E, log_msg);
                }
                if(sw_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d SW1接触器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_H, log_msg);
                }
                break;
                
            case 2:
                k1_sta = GPIO_ReadK2_1_STA();
                k2_sta = GPIO_ReadK2_2_STA();
                sw_sta = GPIO_ReadSW2_STA();
                
                if(k1_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d K2_1继电器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_C, log_msg);
                }
                if(k2_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d K2_2继电器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_F, log_msg);
                }
                if(sw_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d SW2接触器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_I, log_msg);
                }
                break;
                
            case 3:
                k1_sta = GPIO_ReadK3_1_STA();
                k2_sta = GPIO_ReadK3_2_STA();
                sw_sta = GPIO_ReadSW3_STA();
                
                if(k1_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d K3_1继电器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_D, log_msg);
                }
                if(k2_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d K3_2继电器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_G, log_msg);
                }
                if(sw_sta) {  // 关闭失败时应该为0，如果为1则异常
                    snprintf(log_msg, sizeof(log_msg), "通道%d SW3接触器关闭异常", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_J, log_msg);
                }
                break;
        }
        
        return RELAY_ERR_HARDWARE_FAILURE;
    }
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
  * @brief  处理使能信号变化（中断方式：设置标志位，主循环处理具体逻辑）
  * @param  channelNum: 通道号(1-3)
  * @param  state: 信号状态（0=低电平=开启，1=高电平=关闭）
  * @retval 无
  * @note   中断中仅设置标志位，具体处理逻辑在主循环中执行，避免中断阻塞
  */
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state)
{
    // ? 中断处理简化：只设置标志位，避免复杂操作导致看门狗复位
    // 具体的继电器控制逻辑将在主循环中处理
    uint32_t current_time = HAL_GetTick();
    
    switch(channelNum) {
        case 1:
            g_k1_en_interrupt_flag = 1;
            g_k1_en_interrupt_time = current_time;
            break;
        case 2:
            g_k2_en_interrupt_flag = 1;
            g_k2_en_interrupt_time = current_time;
            break;
        case 3:
            g_k3_en_interrupt_flag = 1;
            g_k3_en_interrupt_time = current_time;
            break;
        default:
            // 无效通道号，忽略
            break;
    }
}

/**
  * @brief  处理K_EN中断标志并执行继电器动作（在主循环中调用）
  * @retval 无
  * @note   处理中断标志位，避免轮询延迟，提高响应速度
  */
void RelayControl_ProcessPendingActions(void)
{
    // 检查系统状态，如果处于报警状态则停止处理
    extern SystemState_t SystemControl_GetState(void);
    SystemState_t system_state = SystemControl_GetState();
    if(system_state == SYSTEM_STATE_ALARM) {
        // 报警状态下停止继电器操作，避免阻塞主循环导致看门狗复位
        return;
    }
    
    // 检查是否存在关键异常，如果存在则停止处理
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_B) ||  // K1_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_C) ||  // K2_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_D) ||  // K3_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_E) ||  // K1_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_F) ||  // K2_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_G) ||  // K3_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {  // 外部电源异常（DC_CTRL）
        // 发生关键异常时停止处理，保持当前通道状态
        return;
    }
    
    uint32_t current_time = HAL_GetTick();
    
    // ================== 处理K1_EN中断标志 ===================
    if(g_k1_en_interrupt_flag) {
        g_k1_en_interrupt_flag = 0;  // 清除标志位
        
        uint8_t k1_en = GPIO_ReadK1_EN();
        uint8_t k1_sta_all = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
        uint32_t interrupt_time = g_k1_en_interrupt_time;
        
        DEBUG_Printf("[中断处理] K1_EN中断: 时间=%lu, 中断时间=%lu, K1_EN=%d, STA=%d\r\n", 
                    current_time, interrupt_time, k1_en, k1_sta_all);
        
        // 中断逻辑：检测K_EN为0且STA也为0，则执行开启动作
        if(k1_en == 0 && k1_sta_all == 0) {
            DEBUG_Printf("[中断处理] 通道1: K_EN=0且STA=0，执行开启动作\r\n");
            uint8_t result = RelayControl_OpenChannel(1);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("通道1开启成功\r\n");
            } else {
                DEBUG_Printf("通道1开启失败，错误码=%d\r\n", result);
            }
        }
        // 中断逻辑：检测K_EN为1且STA也为1，则执行关闭动作
        else if(k1_en == 1 && k1_sta_all == 1) {
            DEBUG_Printf("[中断处理] 通道1: K_EN=1且STA=1，执行关闭动作\r\n");
            uint8_t result = RelayControl_CloseChannel(1);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("通道1关闭成功\r\n");
            } else {
                DEBUG_Printf("通道1关闭失败，错误码=%d\r\n", result);
            }
        }
    }
    
    // ================== 处理K2_EN中断标志 ===================
    if(g_k2_en_interrupt_flag) {
        g_k2_en_interrupt_flag = 0;  // 清除标志位
        
        uint8_t k2_en = GPIO_ReadK2_EN();
        uint8_t k2_sta_all = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
        uint32_t interrupt_time = g_k2_en_interrupt_time;
        
        DEBUG_Printf("[中断处理] K2_EN中断: 时间=%lu, 中断时间=%lu, K2_EN=%d, STA=%d\r\n", 
                    current_time, interrupt_time, k2_en, k2_sta_all);
        
        // 中断逻辑：检测K_EN为0且STA也为0，则执行开启动作
        if(k2_en == 0 && k2_sta_all == 0) {
            DEBUG_Printf("[中断处理] 通道2: K_EN=0且STA=0，执行开启动作\r\n");
            uint8_t result = RelayControl_OpenChannel(2);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("通道2开启成功\r\n");
            } else {
                DEBUG_Printf("通道2开启失败，错误码=%d\r\n", result);
            }
        }
        // 中断逻辑：检测K_EN为1且STA也为1，则执行关闭动作
        else if(k2_en == 1 && k2_sta_all == 1) {
            DEBUG_Printf("[中断处理] 通道2: K_EN=1且STA=1，执行关闭动作\r\n");
            uint8_t result = RelayControl_CloseChannel(2);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("通道2关闭成功\r\n");
            } else {
                DEBUG_Printf("通道2关闭失败，错误码=%d\r\n", result);
            }
        }
    }
    
    // ================== 处理K3_EN中断标志 ===================
    if(g_k3_en_interrupt_flag) {
        g_k3_en_interrupt_flag = 0;  // 清除标志位
        
        uint8_t k3_en = GPIO_ReadK3_EN();
        uint8_t k3_sta_all = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
        uint32_t interrupt_time = g_k3_en_interrupt_time;
        
        DEBUG_Printf("[中断处理] K3_EN中断: 时间=%lu, 中断时间=%lu, K3_EN=%d, STA=%d\r\n", 
                    current_time, interrupt_time, k3_en, k3_sta_all);
        
        // 中断逻辑：检测K_EN为0且STA也为0，则执行开启动作
        if(k3_en == 0 && k3_sta_all == 0) {
            DEBUG_Printf("[中断处理] 通道3: K_EN=0且STA=0，执行开启动作\r\n");
            uint8_t result = RelayControl_OpenChannel(3);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("通道3开启成功\r\n");
            } else {
                DEBUG_Printf("通道3开启失败，错误码=%d\r\n", result);
            }
        }
        // 中断逻辑：检测K_EN为1且STA也为1，则执行关闭动作
        else if(k3_en == 1 && k3_sta_all == 1) {
            DEBUG_Printf("[中断处理] 通道3: K_EN=1且STA=1，执行关闭动作\r\n");
            uint8_t result = RelayControl_CloseChannel(3);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("通道3关闭成功\r\n");
            } else {
                DEBUG_Printf("通道3关闭失败，错误码=%d\r\n", result);
            }
        }
    }
} 





