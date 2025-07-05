#include "relay_control.h"
#include "gpio_control.h"
#include "safety_monitor.h"
#include "usart.h"

/************************************************************
 * 继电器控制模块源文件
 * 实现三通道继电器的开启/关闭、三重检测、互锁检查、500ms脉冲、状态反馈检测、异常处理与状态管理
 ************************************************************/

// 三个继电器通道实例
static RelayChannel_t relayChannels[3];
static uint8_t enableCheckCount[3] = {0}; // 三重检测计数器
static uint8_t pendingAction[3] = {0}; // 待处理的动作：0=无动作，1=开启，2=关闭

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
        enableCheckCount[i] = 0;
        pendingAction[i] = 0;
        
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
  * @brief  检查通道反馈状态
  * @param  channelNum: 通道号(1-3)
  * @retval 0:正常 1:异常
  */
uint8_t RelayControl_CheckChannelFeedback(uint8_t channelNum)
{
    uint8_t result = 0;
    // 增加详细调试输出，打印当前通道状态和反馈点电平
    DEBUG_Printf("[调试] 通道%d 状态=%d K1_1=%d K1_2=%d SW1=%d K2_1=%d K2_2=%d SW2=%d K3_1=%d K3_2=%d SW3=%d\r\n",
        channelNum,
        relayChannels[channelNum-1].state,
        GPIO_ReadK1_1_STA(), GPIO_ReadK1_2_STA(), GPIO_ReadSW1_STA(),
        GPIO_ReadK2_1_STA(), GPIO_ReadK2_2_STA(), GPIO_ReadSW2_STA(),
        GPIO_ReadK3_1_STA(), GPIO_ReadK3_2_STA(), GPIO_ReadSW3_STA());
    switch(channelNum) {
        case 1:
            if(relayChannels[0].state == RELAY_STATE_ON) {
                if(GPIO_ReadK1_1_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] K1_1_STA未到高电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadK1_2_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] K1_2_STA未到高电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW1_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] SW1_STA未到高电平\r\n");
                    result = 1;
                }
            } else if(relayChannels[0].state == RELAY_STATE_OFF) {
                if(GPIO_ReadK1_1_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] K1_1_STA未回到低电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadK1_2_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] K1_2_STA未回到低电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW1_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] SW1_STA未回到低电平\r\n");
                    result = 1;
                }
            }
            break;
        case 2:
            if(relayChannels[1].state == RELAY_STATE_ON) {
                if(GPIO_ReadK2_1_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] K2_1_STA未到高电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadK2_2_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] K2_2_STA未到高电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW2_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] SW2_STA未到高电平\r\n");
                    result = 1;
                }
            } else if(relayChannels[1].state == RELAY_STATE_OFF) {
                if(GPIO_ReadK2_1_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] K2_1_STA未回到低电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadK2_2_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] K2_2_STA未回到低电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW2_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] SW2_STA未回到低电平\r\n");
                    result = 1;
                }
            }
            break;
        case 3:
            if(relayChannels[2].state == RELAY_STATE_ON) {
                if(GPIO_ReadK3_1_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] K3_1_STA未到高电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadK3_2_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] K3_2_STA未到高电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW3_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[反馈异常] SW3_STA未到高电平\r\n");
                    result = 1;
                }
            } else if(relayChannels[2].state == RELAY_STATE_OFF) {
                if(GPIO_ReadK3_1_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] K3_1_STA未回到低电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadK3_2_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] K3_2_STA未回到低电平\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW3_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[反馈异常] SW3_STA未回到低电平\r\n");
                    result = 1;
                }
            }
            break;
        default:
            result = 1;
            break;
    }
    return result;
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
        return RELAY_ERR_POWER_FAILURE;
    }
    
    uint8_t idx = channelNum - 1;
    // 检查互锁
    if(RelayControl_CheckInterlock(channelNum)) {
        relayChannels[idx].errorCode = RELAY_ERR_INTERLOCK;
        DEBUG_Printf("通道%d互锁错误\r\n", channelNum);
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
    HAL_Delay(RELAY_PULSE_WIDTH);
    
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
    
    // 延时500ms后，先更新状态再检查反馈
    HAL_Delay(RELAY_FEEDBACK_DELAY);
    relayChannels[idx].state = RELAY_STATE_ON; // 先更新状态
    if(RelayControl_CheckChannelFeedback(channelNum)) {
        relayChannels[idx].errorCode = RELAY_ERR_FEEDBACK;
        relayChannels[idx].state = RELAY_STATE_ERROR;
        DEBUG_Printf("通道%d反馈错误\r\n", channelNum);
        return RELAY_ERR_FEEDBACK;
    }
    relayChannels[idx].errorCode = RELAY_ERR_NONE;
    DEBUG_Printf("通道%d开启成功\r\n", channelNum);
    return RELAY_ERR_NONE;
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
    HAL_Delay(RELAY_PULSE_WIDTH);
    
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
    
    // 延时500ms后，先更新状态再检查反馈
    HAL_Delay(RELAY_FEEDBACK_DELAY);
    relayChannels[idx].state = RELAY_STATE_OFF; // 先更新状态
    if(RelayControl_CheckChannelFeedback(channelNum)) {
        relayChannels[idx].errorCode = RELAY_ERR_FEEDBACK;
        relayChannels[idx].state = RELAY_STATE_ERROR;
        DEBUG_Printf("通道%d反馈错误\r\n", channelNum);
        return RELAY_ERR_FEEDBACK;
    }
    relayChannels[idx].errorCode = RELAY_ERR_NONE;
    DEBUG_Printf("通道%d关闭成功\r\n", channelNum);
    return RELAY_ERR_NONE;
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
  * @brief  处理使能信号变化（已弃用：原中断方式，现改为轮询方式）
  * @param  channelNum: 通道号(1-3)
  * @param  state: 信号状态（0=低电平=开启，1=高电平=关闭）
  * @retval 无
  * @note   此函数已弃用，现在使用轮询方式检测K_EN信号变化
  */
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state)
{
    // 此函数已弃用，现在使用轮询方式在RelayControl_ProcessPendingActions中处理
    // 保留函数定义以保持兼容性，但不执行任何操作
    (void)channelNum;
    (void)state;
    // DEBUG_Printf("[警告] RelayControl_HandleEnableSignal已弃用，请使用轮询方式\r\n");  // 已注释 - 避免重复输出
}

/**
  * @brief  轮询检测K_EN信号并处理继电器动作（在主循环中调用）
  * @retval 无
  */
void RelayControl_ProcessPendingActions(void)
{
    // 检查是否存在关键异常，如果存在则停止轮询
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_B) ||  // K1_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_C) ||  // K2_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_D) ||  // K3_1_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_E) ||  // K1_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_F) ||  // K2_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_G) ||  // K3_2_STA工作异常
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {  // 外部电源异常（DC_CTRL）
        // 发生关键异常时停止轮询，保持当前通道状态
        DEBUG_Printf("[轮询] 检测到关键异常，停止轮询动作\r\n");
        return;
    }
    
    // 轮询检测三个通道的K_EN和STA信号
    for(uint8_t i = 0; i < 3; i++) {
        uint8_t channelNum = i + 1;
        uint8_t k_en = 0;
        uint8_t sta_all = 0;
        
        // 读取对应通道的K_EN信号
        switch(channelNum) {
            case 1:
                k_en = GPIO_ReadK1_EN();
                sta_all = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
                break;
            case 2:
                k_en = GPIO_ReadK2_EN();
                sta_all = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
                break;
            case 3:
                k_en = GPIO_ReadK3_EN();
                sta_all = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
                break;
        }
        
        // 轮询逻辑：检测K_EN为0且STA也为0，则输出500ms低电平脉冲到ON信号
        if(k_en == 0 && sta_all == 0) {
            DEBUG_Printf("[轮询] 通道%d: K_EN=0且STA=0，执行开启动作\r\n", channelNum);
            uint8_t result = RelayControl_OpenChannel(channelNum);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("通道%d开启成功\r\n", channelNum);
            } else {
                DEBUG_Printf("通道%d开启失败，错误码=%d\r\n", channelNum, result);
            }
        }
        // 轮询逻辑：检测K_EN为1且STA也为1，则输出500ms低电平脉冲到OFF信号
        else if(k_en == 1 && sta_all == 1) {
            DEBUG_Printf("[轮询] 通道%d: K_EN=1且STA=1，执行关闭动作\r\n", channelNum);
            uint8_t result = RelayControl_CloseChannel(channelNum);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("通道%d关闭成功\r\n", channelNum);
            } else {
                DEBUG_Printf("通道%d关闭失败，错误码=%d\r\n", channelNum, result);
            }
        }
    }
} 





