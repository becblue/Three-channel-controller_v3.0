#include "relay_control.h"
#include "gpio_control.h"
#include "safety_monitor.h"
#include "smart_delay.h"  // ������ʱ����
#include "usart.h"
#include "log_system.h"   // ��־ϵͳ
#include <stdio.h>

/************************************************************
 * �̵�������ģ��Դ�ļ� - �첽״̬���汾
 * ʵ����ͨ���̵������첽����/�رա��жϷ��������ż�⡢���ȼ�����
 * ����������ѭ�����������ϵͳ��Ӧ�Ժ��ȶ���
 ************************************************************/

// �����̵���ͨ��ʵ��
static RelayChannel_t relayChannels[3];

// ================== �첽״̬������ ===================
// �첽������������
static RelayAsyncOperation_t g_async_operations[MAX_ASYNC_OPERATIONS];

// �첽����ͳ��
static uint32_t g_async_total_operations = 0;
static uint32_t g_async_completed_operations = 0;
static uint32_t g_async_failed_operations = 0;

// ================== ��ǿ�����͸��ż���������ѯģʽ�� ===================
// ������ʱ����������ѯģʽ��
static uint32_t g_last_poll_time[3] = {0, 0, 0};
static uint32_t g_debounce_count[3] = {0, 0, 0};

// ���ż�������
static InterferenceFilter_t g_interference_filter = {0};

// ���ż��ͳ��
static uint32_t g_interference_total_count = 0;
static uint32_t g_filtered_interrupts_count = 0;

/**
  * @brief  �̵�������ģ���ʼ��
  * @retval ��
  */
void RelayControl_Init(void)
{
    for(uint8_t i = 0; i < 3; i++) {
        relayChannels[i].channelNum = i + 1;
        relayChannels[i].state = RELAY_STATE_OFF;
        relayChannels[i].lastActionTime = 0;
        relayChannels[i].errorCode = RELAY_ERR_NONE;
        
        // ��ʼ�����м̵��������ź�Ϊ�ߵ�ƽ�������ܽ�ֹ���̵�����������
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
    
    // ================== ��ʼ���첽״̬�� ===================
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        g_async_operations[i].state = RELAY_ASYNC_IDLE;
        g_async_operations[i].channel = 0;
        g_async_operations[i].operation = 0;
        g_async_operations[i].start_time = 0;
        g_async_operations[i].phase_start_time = 0;
        g_async_operations[i].result = RELAY_ERR_NONE;
        g_async_operations[i].error_code = RELAY_ERR_NONE;
    }
    
    // ================== ��ʼ�������͸��ż�� ===================
    for(uint8_t i = 0; i < 3; i++) {
        g_last_poll_time[i] = 0;
        g_debounce_count[i] = 0;
        g_interference_filter.last_interrupt_time[i] = 0;
    }
    g_interference_filter.simultaneous_count = 0;
    g_interference_filter.interference_detected = 0;
    g_interference_filter.last_interference_time = 0;
    
    // ����ͳ�Ƽ�����
    g_async_total_operations = 0;
    g_async_completed_operations = 0;
    g_async_failed_operations = 0;
    g_interference_total_count = 0;
    g_filtered_interrupts_count = 0;
    
    DEBUG_Printf("�̵�������ģ���ʼ����� - �첽״̬���汾\r\n");
    DEBUG_Printf("֧�ֹ���: �첽�������жϷ��������ż�⡢���ȼ�����\r\n");
    
    // �̵���ģ���ʼ����ɣ���־��¼������ɾ��
}

/**
  * @brief  ���ͨ������״̬
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:�޻��� 1:�л���
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
  * @brief  ���ͨ������״̬���ѷ��� - ��Ϊֱ�Ӽ��Ӳ��״̬��
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:���� 1:�쳣
  * @note   �˺����ѷ���������ʹ��ֱ��Ӳ��״̬��飬���������ڲ�״̬���µ��߼�����
  */
uint8_t RelayControl_CheckChannelFeedback(uint8_t channelNum)
{
    // �˺����ѷ����������Ա��ּ�����
    (void)channelNum;
    return 0;  // ʼ�շ�������������Ӱ������ģ��
}





/**
  * @brief  ��ȡͨ��״̬
  * @param  channelNum: ͨ����(1-3)
  * @retval ͨ��״̬
  */
RelayState_t RelayControl_GetChannelState(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_STATE_ERROR;
    return relayChannels[channelNum - 1].state;
}

/**
  * @brief  ��ȡ���һ�δ������
  * @param  channelNum: ͨ����(1-3)
  * @retval �������
  */
uint8_t RelayControl_GetLastError(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_ERR_INVALID_CHANNEL;
    return relayChannels[channelNum - 1].errorCode;
}

/**
  * @brief  �������״̬
  * @param  channelNum: ͨ����(1-3)
  * @retval ��
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
  * @brief  ����ʹ���źű仯����ѯģʽ��ֱ��ִ�в�����
  * @param  channelNum: ͨ����(1-3)
  * @param  state: �ź�״̬��0=�͵�ƽ=������1=�ߵ�ƽ=�رգ�
  * @retval ��
  * @note   ��ѯģʽ������+���ż��+ֱ��ִ�м̵�������
  */
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state)
{
    uint32_t current_time = HAL_GetTick();
    
    // ������֤
    if(channelNum < 1 || channelNum > 3) {
        g_filtered_interrupts_count++;
        return;
    }
    
    uint8_t ch_idx = channelNum - 1; // ת��Ϊ0-2����
    
    // ================== ������� ===================
    // 50ms�ڵ��ظ�����ֱ�Ӷ���
    if(current_time - g_last_poll_time[ch_idx] < INTERRUPT_DEBOUNCE_TIME_MS) {
        g_filtered_interrupts_count++;
        g_debounce_count[ch_idx]++;
        return;
    }
    
    // ================== ���ż�� ===================
    // ����Ƿ��ںܶ�ʱ���ڶ��ͨ��ͬʱ�仯�����Ƹ��ţ�
    uint8_t simultaneous_count = 0;
    for(uint8_t i = 0; i < 3; i++) {
        if(current_time - g_interference_filter.last_interrupt_time[i] < INTERFERENCE_DETECT_TIME_MS) {
            simultaneous_count++;
        }
    }
    
    // �����⵽���ͨ��ͬʱ�仯�������Ǹ���
    if(simultaneous_count >= 2) {
        g_interference_filter.interference_detected = 1;
        g_interference_filter.simultaneous_count = simultaneous_count + 1;
        g_interference_filter.last_interference_time = current_time;
        g_interference_total_count++;
        g_filtered_interrupts_count++;
        
        DEBUG_Printf("?? [���Ź���] ͨ��%d״̬�仯�����ˣ���⵽%d��ͨ��ͬʱ�仯\r\n", 
                    channelNum, simultaneous_count + 1);
        return;
    }
    
    // ================== ϵͳ��ȫ��� ===================
    // ���ϵͳ״̬��������ڱ���״̬��ֹͣ����
    extern SystemState_t SystemControl_GetState(void);
    SystemState_t system_state = SystemControl_GetState();
    if(system_state == SYSTEM_STATE_ALARM) {
        DEBUG_Printf("?? [��ȫ��ֹ] ͨ��%d��������ֹ��ϵͳ���ڱ���״̬\r\n", channelNum);
        return;
    }
    
    // ����Ƿ���ڹؼ��쳣
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_B) ||  // K1_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_C) ||  // K2_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_D) ||  // K3_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_E) ||  // K1_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_F) ||  // K2_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_G) ||  // K3_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {  // �ⲿ��Դ�쳣��DC_CTRL��
        DEBUG_Printf("?? [��ȫ��ֹ] ͨ��%d��������ֹ����⵽�ؼ��쳣\r\n", channelNum);
        return;
    }
    
    // ================== ����ʱ���¼ ===================
    g_last_poll_time[ch_idx] = current_time;
    g_interference_filter.last_interrupt_time[ch_idx] = current_time;
    
    // ================== ֱ��ִ�м̵������� ===================
    // ��ȡ��ǰӲ��״̬
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
    
    DEBUG_Printf("? [��ѯ����] ͨ��%d: K_EN=%d, STA=%d\r\n", channelNum, state, k_sta_all);
    
    // ִ�в����߼������K_ENΪ0��STAҲΪ0���������첽��������
    if(state == 0 && k_sta_all == 0) {
        DEBUG_Printf("? [�첽����] ͨ��%d: K_EN=0��STA=0�������첽����\r\n", channelNum);
        uint8_t result = RelayControl_StartOpenChannel(channelNum);
        if(result == RELAY_ERR_NONE) {
            DEBUG_Printf("? ͨ��%d�첽��������������\r\n", channelNum);
        } else {
            DEBUG_Printf("? ͨ��%d�첽��������ʧ�ܣ�������=%d\r\n", channelNum, result);
        }
    }
    // ִ�в����߼������K_ENΪ1��STAҲΪ1���������첽�رղ���
    else if(state == 1 && k_sta_all == 1) {
        DEBUG_Printf("? [�첽�ر�] ͨ��%d: K_EN=1��STA=1�������첽�ر�\r\n", channelNum);
        uint8_t result = RelayControl_StartCloseChannel(channelNum);
        if(result == RELAY_ERR_NONE) {
            DEBUG_Printf("? ͨ��%d�첽�رղ���������\r\n", channelNum);
        } else {
            DEBUG_Printf("? ͨ��%d�첽�ر�����ʧ�ܣ�������=%d\r\n", channelNum, result);
        }
    } else {
        DEBUG_Printf("?? [״̬���] ͨ��%d: K_EN=%d, STA=%d�����������㣬�޲���\r\n", 
                    channelNum, state, k_sta_all);
    }
}

// ================== ״̬��֤�����ȼ������� ===================

/**
  * @brief  ��֤״̬�仯�ĺ����ԣ������ţ�
  * @param  k1_en: K1_EN��ǰ״̬
  * @param  k2_en: K2_EN��ǰ״̬  
  * @param  k3_en: K3_EN��ǰ״̬
  * @retval 1:״̬�仯���� 0:���Ƹ���
  */
uint8_t RelayControl_ValidateStateChange(uint8_t k1_en, uint8_t k2_en, uint8_t k3_en)
{
    // ����Ƿ���ϻ����߼���������������ֻ��һ��ͨ����ENΪ�͵�ƽ��ʹ�ܣ�
    uint8_t en_active_count = (!k1_en) + (!k2_en) + (!k3_en);
    
    if(en_active_count > 1) {
        return 0; // ��ͨ��ͬʱʹ�ܣ����Ƹ���
    }
    
    // ������Ӹ�����֤�߼���������EN�ź���STA������һ���Ե�
    return 1; // ״̬�仯����
}

/**
  * @brief  ��ȡ������ȼ����жϣ���ʱ��˳��
  * @retval ͨ����(1-3)�����û���ж��򷵻�0
  */
uint8_t RelayControl_GetHighestPriorityInterrupt(void)
{
    // ɾ���жϱ�־λ��ʱ��������˺�����������
    return 0;
}

/**
  * @brief  ����K_EN�жϱ�־��ִ�м̵����������ѷ�������Ϊ��ѯģʽ��
  * @retval ��
  * @note   �˺����ѷ�����K_EN�ź�����ͨ����ѯģʽ��gpio_control.c�д���
  */
void RelayControl_ProcessPendingActions(void)
{
    // �˺����ѷ�����K_EN�ź�����ͨ����ѯģʽ����
    // �����պ����Ա��ּ�����
    return;
} 

// ================== �첽������������ ===================

/**
  * @brief  ���ҿ��е��첽������λ
  * @retval ��λ���������û�п��в�λ�򷵻�MAX_ASYNC_OPERATIONS
  */
static uint8_t FindFreeAsyncSlot(void)
{
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        if(g_async_operations[i].state == RELAY_ASYNC_IDLE) {
            return i;
        }
    }
    return MAX_ASYNC_OPERATIONS; // û�п��в�λ
}

/**
  * @brief  ����ָ��ͨ�����첽����
  * @param  channelNum: ͨ����(1-3)
  * @retval ��λ���������û���ҵ��򷵻�MAX_ASYNC_OPERATIONS
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

// ================== �첽������������ ===================

/**
  * @brief  �����첽����ͨ����������������
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:�ɹ����� ����:�������
  */
uint8_t RelayControl_StartOpenChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3) {
        return RELAY_ERR_INVALID_CHANNEL;
    }
    
    // ����Ƿ����O���쳣����Դ�쳣��
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
        DEBUG_Printf("ͨ��%d�첽��������ֹ����⵽O���쳣����Դ�쳣��\r\n", channelNum);
        return RELAY_ERR_POWER_FAILURE;
    }
    
    // ����ͨ���Ƿ��������ڽ��еĲ���
    if(FindAsyncOperationByChannel(channelNum) != MAX_ASYNC_OPERATIONS) {
        DEBUG_Printf("ͨ��%d�첽����ʧ�ܣ��������ڽ�����\r\n", channelNum);
        return RELAY_ERR_OPERATION_BUSY;
    }
    
    // ��黥��
    if(RelayControl_CheckInterlock(channelNum)) {
        DEBUG_Printf("ͨ��%d�첽����ʧ�ܣ���������\r\n", channelNum);
        return RELAY_ERR_INTERLOCK;
    }
    
    // ���ҿ��е��첽������λ
    uint8_t slot = FindFreeAsyncSlot();
    if(slot == MAX_ASYNC_OPERATIONS) {
        DEBUG_Printf("ͨ��%d�첽����ʧ�ܣ��޿��ò�����λ\r\n", channelNum);
        return RELAY_ERR_OPERATION_BUSY;
    }
    
    // ��ʼ���첽����
    g_async_operations[slot].state = RELAY_ASYNC_PULSE_START;
    g_async_operations[slot].channel = channelNum;
    g_async_operations[slot].operation = 1; // 1=����
    g_async_operations[slot].start_time = HAL_GetTick();
    g_async_operations[slot].phase_start_time = HAL_GetTick();
    g_async_operations[slot].result = RELAY_ERR_NONE;
    g_async_operations[slot].error_code = RELAY_ERR_NONE;
    
    // ����ͳ��
    g_async_total_operations++;
    
    DEBUG_Printf("? ͨ��%d�첽������������������λ%d��\r\n", channelNum, slot);
    
    return RELAY_ERR_NONE;
}

/**
  * @brief  �����첽�ر�ͨ����������������
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:�ɹ����� ����:�������
  */
uint8_t RelayControl_StartCloseChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3) {
        return RELAY_ERR_INVALID_CHANNEL;
    }
    
    // ����ͨ���Ƿ��������ڽ��еĲ���
    if(FindAsyncOperationByChannel(channelNum) != MAX_ASYNC_OPERATIONS) {
        DEBUG_Printf("ͨ��%d�첽�ر�ʧ�ܣ��������ڽ�����\r\n", channelNum);
        return RELAY_ERR_OPERATION_BUSY;
    }
    
    // ���ҿ��е��첽������λ
    uint8_t slot = FindFreeAsyncSlot();
    if(slot == MAX_ASYNC_OPERATIONS) {
        DEBUG_Printf("ͨ��%d�첽�ر�ʧ�ܣ��޿��ò�����λ\r\n", channelNum);
        return RELAY_ERR_OPERATION_BUSY;
    }
    
    // ��ʼ���첽����
    g_async_operations[slot].state = RELAY_ASYNC_PULSE_START;
    g_async_operations[slot].channel = channelNum;
    g_async_operations[slot].operation = 0; // 0=�ر�
    g_async_operations[slot].start_time = HAL_GetTick();
    g_async_operations[slot].phase_start_time = HAL_GetTick();
    g_async_operations[slot].result = RELAY_ERR_NONE;
    g_async_operations[slot].error_code = RELAY_ERR_NONE;
    
    // ����ͳ��
    g_async_total_operations++;
    
    DEBUG_Printf("? ͨ��%d�첽�رղ�������������λ%d��\r\n", channelNum, slot);
    
    return RELAY_ERR_NONE;
}

// ================== �첽״̬����ѯ���� ===================

/**
  * @brief  �첽״̬����ѯ��������ѭ���и�Ƶ���ã�
  * @retval ��
  * @note   ���������첽�ܹ��ĺ��ģ������ƽ������첽������״̬��
  */
void RelayControl_ProcessAsyncOperations(void)
{
    uint32_t current_time = HAL_GetTick();
    
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        RelayAsyncOperation_t* op = &g_async_operations[i];
        
        // �������в�λ
        if(op->state == RELAY_ASYNC_IDLE) {
            continue;
        }
        
        uint8_t channelNum = op->channel;
        uint32_t elapsed = current_time - op->phase_start_time;
        uint8_t hardware_state_correct = 0; // Ԥ������������switch�����������ľ���
        
        switch(op->state) {
            case RELAY_ASYNC_PULSE_START:
                // ��ʼ�������
                DEBUG_Printf("? ͨ��%d %s���忪ʼ\r\n", channelNum, op->operation ? "����" : "�ر�");
                
                if(op->operation) {
                    // �������������ON����
                    switch(channelNum) {
                        case 1: GPIO_SetK1_1_ON(0); GPIO_SetK1_2_ON(0); break;
                        case 2: GPIO_SetK2_1_ON(0); GPIO_SetK2_2_ON(0); break;
                        case 3: GPIO_SetK3_1_ON(0); GPIO_SetK3_2_ON(0); break;
                    }
                } else {
                    // �رղ��������OFF����
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
                // ��������У��ȴ�������ʱ��
                if(elapsed >= RELAY_ASYNC_PULSE_WIDTH) {
                    DEBUG_Printf("? ͨ��%d�����ȴﵽ����������\r\n", channelNum);
                    op->state = RELAY_ASYNC_PULSE_END;
                    op->phase_start_time = current_time;
                }
                break;
                
            case RELAY_ASYNC_PULSE_END:
                // ��������������ָ��ߵ�ƽ
                if(op->operation) {
                    // �����������ָ�ON�����ź�Ϊ�ߵ�ƽ
                    switch(channelNum) {
                        case 1: GPIO_SetK1_1_ON(1); GPIO_SetK1_2_ON(1); break;
                        case 2: GPIO_SetK2_1_ON(1); GPIO_SetK2_2_ON(1); break;
                        case 3: GPIO_SetK3_1_ON(1); GPIO_SetK3_2_ON(1); break;
                    }
                } else {
                    // �رղ������ָ�OFF�����ź�Ϊ�ߵ�ƽ
                    switch(channelNum) {
                        case 1: GPIO_SetK1_1_OFF(1); GPIO_SetK1_2_OFF(1); break;
                        case 2: GPIO_SetK2_1_OFF(1); GPIO_SetK2_2_OFF(1); break;
                        case 3: GPIO_SetK3_1_OFF(1); GPIO_SetK3_2_OFF(1); break;
                    }
                }
                
                DEBUG_Printf("? ͨ��%d����������ȴ�����\r\n", channelNum);
                op->state = RELAY_ASYNC_FEEDBACK_WAIT;
                op->phase_start_time = current_time;
                break;
                
            case RELAY_ASYNC_FEEDBACK_WAIT:
                // �ȴ�������ʱ
                if(elapsed >= RELAY_ASYNC_FEEDBACK_DELAY) {
                    DEBUG_Printf("? ͨ��%d��ʼ��鷴��\r\n", channelNum);
                    op->state = RELAY_ASYNC_FEEDBACK_CHECK;
                    op->phase_start_time = current_time;
                }
                break;
                
            case RELAY_ASYNC_FEEDBACK_CHECK:
                // ���Ӳ������
                hardware_state_correct = 0;
                
                if(op->operation) {
                    // ��������������Ƿ�����״̬��Ϊ��
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
                    // �رղ���������Ƿ�����״̬��Ϊ��
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
                    // �����ɹ�
                    relayChannels[channelNum-1].state = op->operation ? RELAY_STATE_ON : RELAY_STATE_OFF;
                    relayChannels[channelNum-1].errorCode = RELAY_ERR_NONE;
                    relayChannels[channelNum-1].lastActionTime = current_time;
                    
                    op->result = RELAY_ERR_NONE;
                    op->state = RELAY_ASYNC_COMPLETE;
                    g_async_completed_operations++;
                    
                    DEBUG_Printf("? ͨ��%d�첽%s�����ɹ����\r\n", channelNum, op->operation ? "����" : "�ر�");
                } else {
                    // ����ʧ��
                    relayChannels[channelNum-1].state = RELAY_STATE_ERROR;
                    relayChannels[channelNum-1].errorCode = RELAY_ERR_HARDWARE_FAILURE;
                    
                    op->result = RELAY_ERR_HARDWARE_FAILURE;
                    op->error_code = RELAY_ERR_HARDWARE_FAILURE;
                    op->state = RELAY_ASYNC_ERROR;
                    g_async_failed_operations++;
                    
                    DEBUG_Printf("? ͨ��%d�첽%s����ʧ�� - Ӳ�������쳣\r\n", channelNum, op->operation ? "����" : "�ر�");
                }
                break;
                
            case RELAY_ASYNC_COMPLETE:
            case RELAY_ASYNC_ERROR:
                // ������ɻ�������ò�λ
                op->state = RELAY_ASYNC_IDLE;
                op->channel = 0;
                break;
                
            default:
                // δ֪״̬������
                DEBUG_Printf("?? ͨ��%d�첽��������δ֪״̬%d������\r\n", channelNum, op->state);
                op->state = RELAY_ASYNC_IDLE;
                op->channel = 0;
                g_async_failed_operations++;
                break;
        }
    }
} 

// ================== �첽״̬��ѯ���� ===================

/**
  * @brief  ���ָ��ͨ���Ƿ����첽�������ڽ���
  * @param  channelNum: ͨ����(1-3)
  * @retval 1:�в��������� 0:�޲���
  */
uint8_t RelayControl_IsOperationInProgress(uint8_t channelNum)
{
    return (FindAsyncOperationByChannel(channelNum) != MAX_ASYNC_OPERATIONS);
}

/**
  * @brief  ��ȡָ��ͨ�����첽�������
  * @param  channelNum: ͨ����(1-3)
  * @retval ��������������
  */
uint8_t RelayControl_GetOperationResult(uint8_t channelNum)
{
    uint8_t slot = FindAsyncOperationByChannel(channelNum);
    if(slot != MAX_ASYNC_OPERATIONS) {
        return g_async_operations[slot].result;
    }
    return RELAY_ERR_INVALID_CHANNEL;
}

// ================== ͳ�ƺ͵��Ժ��� ===================

/**
  * @brief  ��ȡ�첽����ͳ����Ϣ
  * @param  total_ops: �ܲ�����
  * @param  completed_ops: ��ɲ�����
  * @param  failed_ops: ʧ�ܲ�����
  * @retval ��
  */
void RelayControl_GetAsyncStatistics(uint32_t* total_ops, uint32_t* completed_ops, uint32_t* failed_ops)
{
    if(total_ops) *total_ops = g_async_total_operations;
    if(completed_ops) *completed_ops = g_async_completed_operations;
    if(failed_ops) *failed_ops = g_async_failed_operations;
}

/**
  * @brief  ��ȡ���ż��ͳ��
  * @param  interference_count: ��⵽�ĸ��Ŵ���
  * @param  filtered_interrupts: �����˵��жϴ���
  * @retval ��
  */
void RelayControl_GetInterferenceStatistics(uint32_t* interference_count, uint32_t* filtered_interrupts)
{
    if(interference_count) *interference_count = g_interference_total_count;
    if(filtered_interrupts) *filtered_interrupts = g_filtered_interrupts_count;
}

// ================== �������νӿ�ʵ�֣����ڰ�ȫ��أ� ===================

/**
  * @brief  ���ָ��ͨ���Ƿ����ڽ����첽����
  * @param  channelNum: ͨ����(1-3)
  * @retval 1:���ڲ��� 0:����״̬
  * @note   ���ڰ�ȫ���ģ���ж��Ƿ���Ҫ��������쳣���
  */
uint8_t RelayControl_IsChannelBusy(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3) {
        return 0; // ��Чͨ����
    }
    
    // ����ͨ���Ƿ������ڽ��е��첽����
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        RelayAsyncOperation_t* op = &g_async_operations[i];
        if(op->state != RELAY_ASYNC_IDLE && op->channel == channelNum) {
            return 1; // �ҵ����ڽ��еĲ���
        }
    }
    
    return 0; // ��ͨ������
}

/**
  * @brief  ��ȡָ��ͨ����ǰ���첽��������
  * @param  channelNum: ͨ����(1-3)
  * @retval 1:�������� 0:�رղ��� 255:�޲���
  * @note   ���ڰ�ȫ���ģ�龫ȷ��������쳣����
  */
uint8_t RelayControl_GetChannelOperationType(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3) {
        return 255; // ��Чͨ����
    }
    
    // ���Ҹ�ͨ�����ڽ��е��첽����
    for(uint8_t i = 0; i < MAX_ASYNC_OPERATIONS; i++) {
        RelayAsyncOperation_t* op = &g_async_operations[i];
        if(op->state != RELAY_ASYNC_IDLE && op->channel == channelNum) {
            return op->operation ? 1 : 0; // 1:�������� 0:�رղ���
        }
    }
    
    return 255; // �޲���
}

// ================== �����Ժ����������汾�� ===================

/**
  * @brief  ����ָ��ͨ���������汾�����������ݣ�
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:�ɹ� ����:�������
  * @note   �ڲ�ʹ���첽���������ȴ�������ɺ󷵻أ�������ԭ�д���ļ�����
  */
uint8_t RelayControl_OpenChannel(uint8_t channelNum)
{
    // �����첽����
    uint8_t result = RelayControl_StartOpenChannel(channelNum);
    if(result != RELAY_ERR_NONE) {
        return result;
    }
    
    DEBUG_Printf("? ͨ��%d���������ȴ��첽�������\r\n", channelNum);
    
    // �ȴ��첽�������
    uint32_t start_time = HAL_GetTick();
    const uint32_t timeout = 2000; // 2�볬ʱ
    
    while(RelayControl_IsOperationInProgress(channelNum)) {
        // �ڵȴ��ڼ������ѯ�첽����
        RelayControl_ProcessAsyncOperations();
        
        // ��鳬ʱ
        if(HAL_GetTick() - start_time > timeout) {
            DEBUG_Printf("? ͨ��%d��������������ʱ\r\n", channelNum);
            return RELAY_ERR_TIMEOUT;
        }
        
        // ������ʱ������CPU����ռ��
        HAL_Delay(1);
    }
    
    // ��ȡ���ս��
    return RelayControl_GetOperationResult(channelNum);
}

/**
  * @brief  �ر�ָ��ͨ���������汾�����������ݣ�
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:�ɹ� ����:�������
  * @note   �ڲ�ʹ���첽���������ȴ�������ɺ󷵻أ�������ԭ�д���ļ�����
  */
uint8_t RelayControl_CloseChannel(uint8_t channelNum)
{
    // �����첽����
    uint8_t result = RelayControl_StartCloseChannel(channelNum);
    if(result != RELAY_ERR_NONE) {
        return result;
    }
    
    DEBUG_Printf("? ͨ��%d�����رյȴ��첽�������\r\n", channelNum);
    
    // �ȴ��첽�������
    uint32_t start_time = HAL_GetTick();
    const uint32_t timeout = 2000; // 2�볬ʱ
    
    while(RelayControl_IsOperationInProgress(channelNum)) {
        // �ڵȴ��ڼ������ѯ�첽����
        RelayControl_ProcessAsyncOperations();
        
        // ��鳬ʱ
        if(HAL_GetTick() - start_time > timeout) {
            DEBUG_Printf("? ͨ��%d�����رղ�����ʱ\r\n", channelNum);
            return RELAY_ERR_TIMEOUT;
        }
        
        // ������ʱ������CPU����ռ��
        HAL_Delay(1);
    }
    
    // ��ȡ���ս��
    return RelayControl_GetOperationResult(channelNum);
} 





