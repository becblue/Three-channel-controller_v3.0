#include "gpio_control.h"
#include "usart.h"

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
void GPIO_SetK1_2_ON(uint8_t state)  { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, (GPIO_PinState)state); }
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

