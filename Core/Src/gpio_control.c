#include "gpio_control.h"
#include "usart.h"
#include "relay_control.h"
#include "safety_monitor.h"

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

// ================== GPIO��ѯϵͳʵ�� ===================

// ��ѯ״̬���飺[0]=K1_EN, [1]=K2_EN, [2]=K3_EN, [3]=DC_CTRL
static GPIO_PollState_t g_poll_states[4];
static uint8_t g_polling_enabled = 0;
static uint32_t g_last_poll_time = 0;

// ��ѯͳ����Ϣ
static uint32_t g_poll_total_count = 0;
static uint32_t g_poll_state_changes = 0;
static uint32_t g_poll_debounce_rejects = 0;

/**
  * @brief  ��ʼ��GPIO��ѯϵͳ
  * @retval ��
  */
void GPIO_InitPollingSystem(void)
{
    // ��ʼ����ѯ״̬
    for(uint8_t i = 0; i < 4; i++) {
        g_poll_states[i].current_state = 1;     // Ĭ�ϸߵ�ƽ
        g_poll_states[i].last_state = 1;
        g_poll_states[i].stable_state = 1;
        g_poll_states[i].state_change_time = 0;
        g_poll_states[i].last_poll_time = 0;
        g_poll_states[i].debounce_counter = 0;
    }
    
    // ��ȡ��ǰʵ��״̬
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
    
    DEBUG_Printf("GPIO��ѯϵͳ��ʼ�����\r\n");
    DEBUG_Printf("��ʼ״̬: K1_EN=%d, K2_EN=%d, K3_EN=%d, DC_CTRL=%d\r\n", 
                g_poll_states[0].stable_state, g_poll_states[1].stable_state, 
                g_poll_states[2].stable_state, g_poll_states[3].stable_state);
}

/**
  * @brief  ����������ŵ��ж�
  * @retval ��
  */
void GPIO_DisableInterrupts(void)
{
    // ע�⣺K1_EN��K2_EN��K3_EN��DC_CTRL��������gpio.c������Ϊ��ͨ����ģʽ
    // �����ٽ���EXTI�жϣ��˺���������������־���
    DEBUG_Printf("K1_EN��K2_EN��K3_EN��DC_CTRL������Ϊ��ͨ����ģʽ�����жϹ���\r\n");
}

/**
  * @brief  ��ȡָ�����ŵ��ȶ�״̬
  * @param  pin_index: �������� 0=K1_EN, 1=K2_EN, 2=K3_EN, 3=DC_CTRL
  * @retval �ȶ�״̬
  */
uint8_t GPIO_GetStableState(uint8_t pin_index)
{
    if(pin_index >= 4) return 1;
    return g_poll_states[pin_index].stable_state;
}

/**
  * @brief  ����GPIO��ѯ����ѭ���и�Ƶ���ã�
  * @retval ��
  */
void GPIO_ProcessPolling(void)
{
    if(!g_polling_enabled) return;
    
    uint32_t current_time = HAL_GetTick();
    
    // ��ѯ�������
    if(current_time - g_last_poll_time < GPIO_POLL_INTERVAL_MS) {
        return;
    }
    g_last_poll_time = current_time;
    g_poll_total_count++; // ͳ����ѯ����
    
    // ��ȡ��ǰ����״̬
    uint8_t new_states[4];
    new_states[0] = GPIO_ReadK1_EN();
    new_states[1] = GPIO_ReadK2_EN();
    new_states[2] = GPIO_ReadK3_EN();
    new_states[3] = GPIO_ReadDC_CTRL();
    
    // ��ÿ�����Ž��з�������
    for(uint8_t i = 0; i < 4; i++) {
        GPIO_PollState_t *state = &g_poll_states[i];
        
        // ���״̬�Ƿ����仯
        if(new_states[i] != state->last_state) {
            // ״̬�����仯����¼�仯ʱ�䲢���÷���������
            state->current_state = new_states[i];
            state->state_change_time = current_time;
            state->debounce_counter = 0;
            g_poll_state_changes++; // ͳ��״̬�仯����
        } else {
            // ״̬δ�仯�����ӷ���������
            if(state->debounce_counter < GPIO_DEBOUNCE_SAMPLES) {
                state->debounce_counter++;
            }
        }
        
        // ����Ƿ�ﵽ����Ҫ��
        if(state->debounce_counter >= GPIO_DEBOUNCE_SAMPLES && 
           current_time - state->state_change_time >= GPIO_DEBOUNCE_TIME_MS) {
            
            // ״̬�ȶ�������Ƿ���Ҫ�����¼�
            if(state->current_state != state->stable_state) {
                uint8_t old_stable_state = state->stable_state;
                state->stable_state = state->current_state;
                
                // ������Ӧ�Ĵ�����
                switch(i) {
                    case 0: // K1_EN
                        DEBUG_Printf("? [��ѯ���] K1_EN״̬�仯: %d->%d (��ѯ����:%lu)\r\n", 
                                    old_stable_state, state->stable_state, g_poll_total_count);
                        RelayControl_HandleEnableSignal(1, state->stable_state);
                        break;
                        
                    case 1: // K2_EN
                        DEBUG_Printf("? [��ѯ���] K2_EN״̬�仯: %d->%d (��ѯ����:%lu)\r\n", 
                                    old_stable_state, state->stable_state, g_poll_total_count);
                        RelayControl_HandleEnableSignal(2, state->stable_state);
                        break;
                        
                    case 2: // K3_EN
                        DEBUG_Printf("? [��ѯ���] K3_EN״̬�仯: %d->%d (��ѯ����:%lu)\r\n", 
                                    old_stable_state, state->stable_state, g_poll_total_count);
                        RelayControl_HandleEnableSignal(3, state->stable_state);
                        break;
                        
                    case 3: // DC_CTRL
                        DEBUG_Printf("? [��ѯ���] DC_CTRL״̬�仯: %d->%d (��ѯ����:%lu)\r\n", 
                                    old_stable_state, state->stable_state, g_poll_total_count);
                        if(state->stable_state == 0) { // �½��ر�ʾ��Դ�쳣
                            SafetyMonitor_PowerFailureCallback();
                        }
                        break;
                }
            }
        } else if(state->current_state != state->stable_state && 
                  current_time - state->state_change_time < GPIO_DEBOUNCE_TIME_MS) {
            // ״̬�仯��δͨ����������¼ͳ��
            g_poll_debounce_rejects++;
        }
        
        // �����ϴ�״̬
        state->last_state = new_states[i];
        state->last_poll_time = current_time;
    }
    
    // ÿ30�����һ����ѯͳ����Ϣ
    static uint32_t last_stats_time = 0;
    if(current_time - last_stats_time >= 30000) {
        last_stats_time = current_time;
        DEBUG_Printf("? [��ѯͳ��] ����ѯ:%lu��, ״̬�仯:%lu��, �����ܾ�:%lu��\r\n", 
                    g_poll_total_count, g_poll_state_changes, g_poll_debounce_rejects);
    }
}

/**
  * @brief  �����ѯϵͳͳ����Ϣ
  * @retval ��
  */
void GPIO_PrintPollingStats(void)
{
    DEBUG_Printf("=== GPIO��ѯϵͳ״̬ ===\r\n");
    DEBUG_Printf("��ѯʹ��: %s\r\n", g_polling_enabled ? "��" : "��");
    DEBUG_Printf("����ѯ����: %lu\r\n", g_poll_total_count);
    DEBUG_Printf("״̬�仯����: %lu\r\n", g_poll_state_changes);
    DEBUG_Printf("�����ܾ�����: %lu\r\n", g_poll_debounce_rejects);
    DEBUG_Printf("��ǰ�ȶ�״̬: K1_EN=%d, K2_EN=%d, K3_EN=%d, DC_CTRL=%d\r\n",
                g_poll_states[0].stable_state, g_poll_states[1].stable_state,
                g_poll_states[2].stable_state, g_poll_states[3].stable_state);
    
    // ���ÿ�����ŵ���ϸ״̬
    const char* pin_names[] = {"K1_EN", "K2_EN", "K3_EN", "DC_CTRL"};
    for(uint8_t i = 0; i < 4; i++) {
        GPIO_PollState_t *state = &g_poll_states[i];
        DEBUG_Printf("%s: ��ǰ=%d, �ȶ�=%d, ��������=%d\r\n", 
                    pin_names[i], state->current_state, state->stable_state, state->debounce_counter);
    }
}

/**
  * @brief  �����ѯϵͳ�Ƿ�������
  * @retval 1:������ 0:δ����
  */
uint8_t GPIO_IsPollingEnabled(void)
{
    return g_polling_enabled;
}

