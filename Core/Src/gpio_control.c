#include "gpio_control.h"
#include "usart.h"

/************************************************************
 * GPIO����ģ��Դ�ļ�
 * ʵ����������/����źŵĶ�ȡ�Ϳ��ƺ�����
 * �����жϻص���ȥ�����������������������ȵ����ʵ�֡�
 ************************************************************/

//---------------- �����źŶ�ȡʵ�� ----------------//
// ʹ���źŶ�ȡ
uint8_t GPIO_ReadK1_EN(void) {
    // ��ȡK1_EN�����źţ�PB9��
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);
}
uint8_t GPIO_ReadK2_EN(void) {
    // ��ȡK2_EN�����źţ�PB8��
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8);
}
uint8_t GPIO_ReadK3_EN(void) {
    // ��ȡK3_EN�����źţ�PA15��
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15);
}

// ״̬�����źŶ�ȡ
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

// ��Դ����źŶ�ȡ
uint8_t GPIO_ReadDC_CTRL(void) {
    // ��ȡ�ⲿ��Դ����źţ�PB5��
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5);
}

// ����ת���źŶ�ȡ
uint8_t GPIO_ReadFAN_SEN(void) {
    // ��ȡ����ת���źţ�PC12��
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12);
}

//---------------- ����źſ���ʵ�� ----------------//
// �̵��������ź�
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

// �����������
void GPIO_SetAlarmOutput(uint8_t state) {
    // ����ALARM���������PB4��
    // state: 1=�͵�ƽ�������������0=�ߵ�ƽ�����������
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// ����������
void GPIO_SetBeepOutput(uint8_t state) {
    // ���Ʒ����������PB3��
    // state: 1=�͵�ƽ�������������0=�ߵ�ƽ�����������
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// ����PWM���ƣ�����ֱ�ӿ���PWM���ţ�
void GPIO_SetFAN_PWM(uint8_t state) {
    // ���Ʒ���PWM�����PA6��TIM3_CH1��
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, (GPIO_PinState)state);
}

// RS485������ƣ��������չ��
void GPIO_SetRS485_EN(uint8_t state) {
    // ����RS485����ʹ�ܣ�PA11��
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, (GPIO_PinState)state);
}

//---------------- �ж���ȥ����ʵ�� ----------------//
// ȥ������ر���
static uint32_t lastDebounceTime_K1 = 0;
static uint32_t lastDebounceTime_K2 = 0;
static uint32_t lastDebounceTime_K3 = 0;
static uint32_t lastDebounceTime_DC = 0;
static const uint32_t debounceDelay = 50; // 50msȥ��

// ʹ���ź��жϻص�����EXTI�жϷ������е��ã�
void GPIO_K1_EN_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - lastDebounceTime_K1) > debounceDelay) {
        lastDebounceTime_K1 = now;
        uint8_t state = GPIO_ReadK1_EN();
        DEBUG_Printf("K1_EN״̬�仯: %s\r\n", state ? "�ߵ�ƽ" : "�͵�ƽ");
        // �������չ��֪ͨ�ϲ�����ñ�־
    }
}
void GPIO_K2_EN_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - lastDebounceTime_K2) > debounceDelay) {
        lastDebounceTime_K2 = now;
        uint8_t state = GPIO_ReadK2_EN();
        DEBUG_Printf("K2_EN״̬�仯: %s\r\n", state ? "�ߵ�ƽ" : "�͵�ƽ");
    }
}
void GPIO_K3_EN_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - lastDebounceTime_K3) > debounceDelay) {
        lastDebounceTime_K3 = now;
        uint8_t state = GPIO_ReadK3_EN();
        DEBUG_Printf("K3_EN״̬�仯: %s\r\n", state ? "�ߵ�ƽ" : "�͵�ƽ");
    }
}
void GPIO_DC_CTRL_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - lastDebounceTime_DC) > debounceDelay) {
        lastDebounceTime_DC = now;
        uint8_t state = GPIO_ReadDC_CTRL();
        DEBUG_Printf("DC_CTRL״̬�仯: %s\r\n", state ? "�ߵ�ƽ" : "�͵�ƽ");
    }
}

