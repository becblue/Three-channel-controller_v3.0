#include "gpio_control.h"
#include "usart.h"
#include "relay_control.h"
#include "safety_monitor.h"

/************************************************************
 * GPIO控制模块源文件
 * 实现所有输入/输出信号的读取和控制函数，
 * 包括中断回调、去抖动、蜂鸣器、报警、风扇等相关实现。
 ************************************************************/

//---------------- 输入信号读取实现 ----------------//
// 使能信号读取
uint8_t GPIO_ReadK1_EN(void) {
    // 读取K1_EN输入信号（PB9）
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);
}
uint8_t GPIO_ReadK2_EN(void) {
    // 读取K2_EN输入信号（PB8）
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8);
}
uint8_t GPIO_ReadK3_EN(void) {
    // 读取K3_EN输入信号（PA15）
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15);
}

// 状态反馈信号读取
uint8_t GPIO_ReadK1_1_STA(void) { 
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4); ;
}
uint8_t GPIO_ReadK1_2_STA(void) { 
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1); 
}
uint8_t GPIO_ReadK2_1_STA(void) { 
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5); 
}
uint8_t GPIO_ReadK2_2_STA(void) { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10); }
uint8_t GPIO_ReadK3_1_STA(void) { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0); }
uint8_t GPIO_ReadK3_2_STA(void) { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11); }
uint8_t GPIO_ReadSW1_STA(void)  {
     return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8); 
    
     }
uint8_t GPIO_ReadSW2_STA(void)  { return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9); }
uint8_t GPIO_ReadSW3_STA(void)  { return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8); }

// 电源监控信号读取
uint8_t GPIO_ReadDC_CTRL(void) {
    // 读取外部电源监控信号（PB5）
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5);
}

// 风扇转速信号读取
uint8_t GPIO_ReadFAN_SEN(void) {
    // 读取风扇转速信号（PC12）
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12);
}

//---------------- 输出信号控制实现 ----------------//
// 继电器控制信号
void GPIO_SetK1_1_ON(uint8_t state)  { HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, (GPIO_PinState)state); }
void GPIO_SetK1_1_OFF(uint8_t state) { HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, (GPIO_PinState)state); }
void GPIO_SetK1_2_ON(uint8_t state)  { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, (GPIO_PinState)state); }
void GPIO_SetK1_2_OFF(uint8_t state) { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, (GPIO_PinState)state); }
void GPIO_SetK2_1_ON(uint8_t state)  { HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, (GPIO_PinState)state); }
void GPIO_SetK2_1_OFF(uint8_t state) { HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, (GPIO_PinState)state); }
void GPIO_SetK2_2_ON(uint8_t state)  { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, (GPIO_PinState)state); }
void GPIO_SetK2_2_OFF(uint8_t state) { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, (GPIO_PinState)state); }
void GPIO_SetK3_1_ON(uint8_t state)  { HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, (GPIO_PinState)state); }
void GPIO_SetK3_1_OFF(uint8_t state) { HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, (GPIO_PinState)state); }
void GPIO_SetK3_2_ON(uint8_t state)  { HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, (GPIO_PinState)state); }
void GPIO_SetK3_2_OFF(uint8_t state) { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, (GPIO_PinState)state); }

// 报警输出控制
void GPIO_SetAlarmOutput(uint8_t state) {
    // 控制ALARM引脚输出（PB4）
    // state: 1=低电平输出（报警），0=高电平输出（正常）
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// 蜂鸣器控制
void GPIO_SetBeepOutput(uint8_t state) {
    // 控制蜂鸣器输出（PB3）
    // state: 1=低电平输出（蜂鸣），0=高电平输出（静音）
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// 风扇PWM控制（如需直接控制PWM引脚）
void GPIO_SetFAN_PWM(uint8_t state) {
    // 控制风扇PWM输出（PA6，TIM3_CH1）
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, (GPIO_PinState)state);
}

// RS485方向控制（如后续扩展）
void GPIO_SetRS485_EN(uint8_t state) {
    // 控制RS485方向使能（PA11）
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, (GPIO_PinState)state);
}

//---------------- 中断与去抖动实现 ----------------//
// 去抖动相关变量
static uint32_t lastDebounceTime_K1 = 0;
static uint32_t lastDebounceTime_K2 = 0;
static uint32_t lastDebounceTime_K3 = 0;
static uint32_t lastDebounceTime_DC = 0;
static const uint32_t debounceDelay = 50; // 50ms去抖

// 使能信号中断回调（在EXTI中断服务函数中调用）
void GPIO_K1_EN_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - lastDebounceTime_K1) > debounceDelay) {
        lastDebounceTime_K1 = now;
        uint8_t state = GPIO_ReadK1_EN();
        DEBUG_Printf("K1_EN状态变化: %s\r\n", state ? "高电平" : "低电平");
        // 这里可扩展：通知上层或设置标志
    }
}
void GPIO_K2_EN_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - lastDebounceTime_K2) > debounceDelay) {
        lastDebounceTime_K2 = now;
        uint8_t state = GPIO_ReadK2_EN();
        DEBUG_Printf("K2_EN状态变化: %s\r\n", state ? "高电平" : "低电平");
    }
}
void GPIO_K3_EN_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - lastDebounceTime_K3) > debounceDelay) {
        lastDebounceTime_K3 = now;
        uint8_t state = GPIO_ReadK3_EN();
        DEBUG_Printf("K3_EN状态变化: %s\r\n", state ? "高电平" : "低电平");
    }
}
void GPIO_DC_CTRL_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - lastDebounceTime_DC) > debounceDelay) {
        lastDebounceTime_DC = now;
        uint8_t state = GPIO_ReadDC_CTRL();
        DEBUG_Printf("DC_CTRL状态变化: %s\r\n", state ? "高电平" : "低电平");
    }
}

// ================== GPIO轮询系统实现 ===================

// 轮询状态数组：[0]=K1_EN, [1]=K2_EN, [2]=K3_EN, [3]=DC_CTRL
static GPIO_PollState_t g_poll_states[4];
static uint8_t g_polling_enabled = 0;
static uint32_t g_last_poll_time = 0;

// 轮询统计信息
static uint32_t g_poll_total_count = 0;
static uint32_t g_poll_state_changes = 0;
static uint32_t g_poll_debounce_rejects = 0;

/**
  * @brief  初始化GPIO轮询系统
  * @retval 无
  */
void GPIO_InitPollingSystem(void)
{
    // 初始化轮询状态
    for(uint8_t i = 0; i < 4; i++) {
        g_poll_states[i].current_state = 1;     // 默认高电平
        g_poll_states[i].last_state = 1;
        g_poll_states[i].stable_state = 1;
        g_poll_states[i].state_change_time = 0;
        g_poll_states[i].last_poll_time = 0;
        g_poll_states[i].debounce_counter = 0;
    }
    
    // 读取当前实际状态
    g_poll_states[0].current_state = GPIO_ReadK1_EN();
    g_poll_states[0].stable_state = g_poll_states[0].current_state;
    g_poll_states[0].last_state = g_poll_states[0].current_state;
    
    g_poll_states[1].current_state = GPIO_ReadK2_EN();
    g_poll_states[1].stable_state = g_poll_states[1].current_state;
    g_poll_states[1].last_state = g_poll_states[1].current_state;
    
    g_poll_states[2].current_state = GPIO_ReadK3_EN();
    g_poll_states[2].stable_state = g_poll_states[2].current_state;
    g_poll_states[2].last_state = g_poll_states[2].current_state;
    
    g_poll_states[3].current_state = GPIO_ReadDC_CTRL();
    g_poll_states[3].stable_state = g_poll_states[3].current_state;
    g_poll_states[3].last_state = g_poll_states[3].current_state;
    
    g_last_poll_time = HAL_GetTick();
    g_polling_enabled = 1;
    
    DEBUG_Printf("GPIO轮询系统初始化完成\r\n");
    DEBUG_Printf("初始状态: K1_EN=%d, K2_EN=%d, K3_EN=%d, DC_CTRL=%d\r\n", 
                g_poll_states[0].stable_state, g_poll_states[1].stable_state, 
                g_poll_states[2].stable_state, g_poll_states[3].stable_state);
}

/**
  * @brief  禁用相关引脚的中断
  * @retval 无
  */
void GPIO_DisableInterrupts(void)
{
    // 注意：K1_EN、K2_EN、K3_EN、DC_CTRL引脚已在gpio.c中配置为普通输入模式
    // 无需再禁用EXTI中断，此函数保留仅用于日志输出
    DEBUG_Printf("K1_EN、K2_EN、K3_EN、DC_CTRL已配置为普通输入模式，无中断功能\r\n");
}

/**
  * @brief  获取指定引脚的稳定状态
  * @param  pin_index: 引脚索引 0=K1_EN, 1=K2_EN, 2=K3_EN, 3=DC_CTRL
  * @retval 稳定状态
  */
uint8_t GPIO_GetStableState(uint8_t pin_index)
{
    if(pin_index >= 4) return 1;
    return g_poll_states[pin_index].stable_state;
}

/**
  * @brief  处理GPIO轮询（主循环中高频调用）
  * @retval 无
  */
void GPIO_ProcessPolling(void)
{
    if(!g_polling_enabled) return;
    
    uint32_t current_time = HAL_GetTick();
    
    // 轮询间隔控制
    if(current_time - g_last_poll_time < GPIO_POLL_INTERVAL_MS) {
        return;
    }
    g_last_poll_time = current_time;
    g_poll_total_count++; // 统计轮询次数
    
    // 读取当前引脚状态
    uint8_t new_states[4];
    new_states[0] = GPIO_ReadK1_EN();
    new_states[1] = GPIO_ReadK2_EN();
    new_states[2] = GPIO_ReadK3_EN();
    new_states[3] = GPIO_ReadDC_CTRL();
    
    // 对每个引脚进行防抖处理
    for(uint8_t i = 0; i < 4; i++) {
        GPIO_PollState_t *state = &g_poll_states[i];
        
        // 检查状态是否发生变化
        if(new_states[i] != state->last_state) {
            // 状态发生变化，记录变化时间并重置防抖计数器
            state->current_state = new_states[i];
            state->state_change_time = current_time;
            state->debounce_counter = 0;
            g_poll_state_changes++; // 统计状态变化次数
        } else {
            // 状态未变化，增加防抖计数器
            if(state->debounce_counter < GPIO_DEBOUNCE_SAMPLES) {
                state->debounce_counter++;
            }
        }
        
        // 检查是否达到防抖要求
        if(state->debounce_counter >= GPIO_DEBOUNCE_SAMPLES && 
           current_time - state->state_change_time >= GPIO_DEBOUNCE_TIME_MS) {
            
            // 状态稳定，检查是否需要触发事件
            if(state->current_state != state->stable_state) {
                uint8_t old_stable_state = state->stable_state;
                state->stable_state = state->current_state;
                
                // 触发相应的处理函数
                switch(i) {
                    case 0: // K1_EN
                        DEBUG_Printf("? [轮询检测] K1_EN状态变化: %d->%d (轮询计数:%lu)\r\n", 
                                    old_stable_state, state->stable_state, g_poll_total_count);
                        RelayControl_HandleEnableSignal(1, state->stable_state);
                        break;
                        
                    case 1: // K2_EN
                        DEBUG_Printf("? [轮询检测] K2_EN状态变化: %d->%d (轮询计数:%lu)\r\n", 
                                    old_stable_state, state->stable_state, g_poll_total_count);
                        RelayControl_HandleEnableSignal(2, state->stable_state);
                        break;
                        
                    case 2: // K3_EN
                        DEBUG_Printf("? [轮询检测] K3_EN状态变化: %d->%d (轮询计数:%lu)\r\n", 
                                    old_stable_state, state->stable_state, g_poll_total_count);
                        RelayControl_HandleEnableSignal(3, state->stable_state);
                        break;
                        
                    case 3: // DC_CTRL
                        DEBUG_Printf("? [轮询检测] DC_CTRL状态变化: %d->%d (轮询计数:%lu)\r\n", 
                                    old_stable_state, state->stable_state, g_poll_total_count);
                        if(state->stable_state == 0) { // 下降沿表示电源异常
                            SafetyMonitor_PowerFailureCallback();
                        }
                        break;
                }
            }
        } else if(state->current_state != state->stable_state && 
                  current_time - state->state_change_time < GPIO_DEBOUNCE_TIME_MS) {
            // 状态变化但未通过防抖，记录统计
            g_poll_debounce_rejects++;
        }
        
        // 更新上次状态
        state->last_state = new_states[i];
        state->last_poll_time = current_time;
    }
    
    // 每30秒输出一次轮询统计信息
    static uint32_t last_stats_time = 0;
    if(current_time - last_stats_time >= 30000) {
        last_stats_time = current_time;
        DEBUG_Printf("? [轮询统计] 总轮询:%lu次, 状态变化:%lu次, 防抖拒绝:%lu次\r\n", 
                    g_poll_total_count, g_poll_state_changes, g_poll_debounce_rejects);
    }
}

/**
  * @brief  输出轮询系统统计信息
  * @retval 无
  */
void GPIO_PrintPollingStats(void)
{
    DEBUG_Printf("=== GPIO轮询系统状态 ===\r\n");
    DEBUG_Printf("轮询使能: %s\r\n", g_polling_enabled ? "是" : "否");
    DEBUG_Printf("总轮询次数: %lu\r\n", g_poll_total_count);
    DEBUG_Printf("状态变化次数: %lu\r\n", g_poll_state_changes);
    DEBUG_Printf("防抖拒绝次数: %lu\r\n", g_poll_debounce_rejects);
    DEBUG_Printf("当前稳定状态: K1_EN=%d, K2_EN=%d, K3_EN=%d, DC_CTRL=%d\r\n",
                g_poll_states[0].stable_state, g_poll_states[1].stable_state,
                g_poll_states[2].stable_state, g_poll_states[3].stable_state);
    
    // 输出每个引脚的详细状态
    const char* pin_names[] = {"K1_EN", "K2_EN", "K3_EN", "DC_CTRL"};
    for(uint8_t i = 0; i < 4; i++) {
        GPIO_PollState_t *state = &g_poll_states[i];
        DEBUG_Printf("%s: 当前=%d, 稳定=%d, 防抖计数=%d\r\n", 
                    pin_names[i], state->current_state, state->stable_state, state->debounce_counter);
    }
}

/**
  * @brief  检查轮询系统是否已启用
  * @retval 1:已启用 0:未启用
  */
uint8_t GPIO_IsPollingEnabled(void)
{
    return g_polling_enabled;
}

