/**
  ******************************************************************************
  * @file    smart_delay.c
  * @brief   智能延时函数实现
  *          在延时过程中自动喂狗，彻底解决看门狗复位问题
  ******************************************************************************
  * @attention
  *
  * 智能延时系统实现：
  * 1. 将长延时分割为多个小延时段
  * 2. 在每个延时段之间智能喂狗
  * 3. 根据系统状态决定是否喂狗
  * 4. 保持延时精度和系统稳定性
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "smart_delay.h"
#include "iwdg.h"
#include "usart.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint32_t last_feed_time = 0;  // 上次喂狗时间

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  检查是否需要喂狗
  * @retval 1:需要喂狗 0:不需要喂狗
  * @note   根据当前系统状态判断是否需要喂狗
  */
uint8_t SmartDelay_ShouldFeedWatchdog(void)
{
    SystemState_t current_state = SystemControl_GetState();
    
    // 在正常状态、报警状态和自检阶段都需要喂狗
    if(current_state == SYSTEM_STATE_NORMAL || 
       current_state == SYSTEM_STATE_ALARM ||
       current_state == SYSTEM_STATE_SELF_TEST ||
       current_state == SYSTEM_STATE_SELF_TEST_CHECK) {
        return 1;
    }
    
    // 其他状态下不喂狗（如ERROR状态，让看门狗复位系统）
    return 0;
}

/**
  * @brief  执行智能喂狗操作
  * @retval 无
  * @note   根据系统状态智能决定是否喂狗
  */
void SmartDelay_FeedWatchdog(void)
{
    uint32_t current_time = HAL_GetTick();
    
    // 检查是否需要喂狗
    if(SmartDelay_ShouldFeedWatchdog()) {
        // 防止喂狗过于频繁
        if(current_time - last_feed_time >= SMART_DELAY_MIN_FEED_TIME) {
            IWDG_Refresh();
            last_feed_time = current_time;
        }
    }
}

/**
  * @brief  智能延时函数（推荐使用）
  * @param  delay_ms: 延时时间（毫秒）
  * @retval 无
  * @note   在延时过程中会根据系统状态自动喂狗
  */
void SmartDelay(uint32_t delay_ms)
{
    uint32_t remaining_time = delay_ms;
    uint32_t feed_interval = SMART_DELAY_FEED_INTERVAL_MS;
    
    // 如果延时时间很短，直接使用HAL_Delay
    if(delay_ms <= SMART_DELAY_MIN_FEED_TIME) {
        HAL_Delay(delay_ms);
        return;
    }
    
    // 在延时过程中定期喂狗
    while(remaining_time > 0) {
        uint32_t current_delay = (remaining_time > feed_interval) ? feed_interval : remaining_time;
        
        // 执行延时
        HAL_Delay(current_delay);
        remaining_time -= current_delay;
        
        // 如果还有剩余时间，尝试喂狗
        if(remaining_time > 0) {
            SmartDelay_FeedWatchdog();
        }
    }
    
    // 延时结束后再喂一次狗
    SmartDelay_FeedWatchdog();
}

/**
  * @brief  带调试信息的智能延时函数
  * @param  delay_ms: 延时时间（毫秒）
  * @param  context: 延时上下文描述（用于调试）
  * @retval 无
  * @note   在延时过程中会输出调试信息并自动喂狗
  */
void SmartDelayWithDebug(uint32_t delay_ms, const char* context)
{
    uint32_t remaining_time = delay_ms;
    uint32_t feed_interval = SMART_DELAY_FEED_INTERVAL_MS;
    uint32_t elapsed_time = 0;
    
    // 注释智能延时调试信息，保持串口输出简洁
    // DEBUG_Printf("[智能延时] 开始: %s, 总时间: %lums\r\n", context, delay_ms);
    
    // 如果延时时间很短，直接使用HAL_Delay
    if(delay_ms <= SMART_DELAY_MIN_FEED_TIME) {
        HAL_Delay(delay_ms);
        // DEBUG_Printf("[智能延时] 完成: %s\r\n", context);
        return;
    }
    
    // 在延时过程中定期喂狗（注释进度输出）
    while(remaining_time > 0) {
        uint32_t current_delay = (remaining_time > feed_interval) ? feed_interval : remaining_time;
        
        // 执行延时
        HAL_Delay(current_delay);
        remaining_time -= current_delay;
        elapsed_time += current_delay;
        
        // 如果还有剩余时间，尝试喂狗（注释进度输出）
        if(remaining_time > 0) {
            SmartDelay_FeedWatchdog();
            // uint32_t progress = (elapsed_time * 100) / delay_ms;
            // DEBUG_Printf("[智能延时] %s: 进度 %lu%% (%lums/%lums)\r\n", 
            //             context, progress, elapsed_time, delay_ms);
        }
    }
    
    // 延时结束后再喂一次狗
    SmartDelay_FeedWatchdog();
    // DEBUG_Printf("[智能延时] 完成: %s, 总用时: %lums\r\n", context, delay_ms);
}

/**
  * @brief  强制喂狗延时函数（特殊场景使用）
  * @param  delay_ms: 延时时间（毫秒）
  * @retval 无
  * @note   无论系统状态如何都会喂狗，用于关键初始化阶段
  */
void SmartDelayWithForceFeed(uint32_t delay_ms)
{
    uint32_t remaining_time = delay_ms;
    uint32_t feed_interval = SMART_DELAY_FEED_INTERVAL_MS;
    
    // 如果延时时间很短，直接使用HAL_Delay
    if(delay_ms <= SMART_DELAY_MIN_FEED_TIME) {
        HAL_Delay(delay_ms);
        return;
    }
    
    // 在延时过程中强制喂狗
    while(remaining_time > 0) {
        uint32_t current_delay = (remaining_time > feed_interval) ? feed_interval : remaining_time;
        
        // 执行延时
        HAL_Delay(current_delay);
        remaining_time -= current_delay;
        
        // 如果还有剩余时间，强制喂狗
        if(remaining_time > 0) {
            uint32_t current_time = HAL_GetTick();
            if(current_time - last_feed_time >= SMART_DELAY_MIN_FEED_TIME) {
                IWDG_Refresh();  // 强制喂狗，不检查系统状态
                last_feed_time = current_time;
            }
        }
    }
    
    // 延时结束后再强制喂一次狗
    IWDG_Refresh();
    last_feed_time = HAL_GetTick();
}

/**
  * @brief  简单延时函数（不喂狗）
  * @param  delay_ms: 延时时间（毫秒）
  * @retval 无
  * @note   完全等同于HAL_Delay，用于不需要喂狗的场景
  */
void SimpleDelay(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
} 


