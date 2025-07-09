#include "relay_control.h"
#include "gpio_control.h"
#include "safety_monitor.h"
#include "smart_delay.h"  // ������ʱ����
#include "usart.h"
#include "log_system.h"   // ��־ϵͳ
#include <stdio.h>

/************************************************************
 * �̵�������ģ��Դ�ļ�
 * ʵ����ͨ���̵����Ŀ���/�رա����ؼ�⡢������顢500ms���塢״̬������⡢�쳣������״̬����
 ************************************************************/

// �����̵���ͨ��ʵ��
static RelayChannel_t relayChannels[3];

// ================== �жϱ�־λ���� ===================
// K_EN�ź��жϱ�־λ������DC_CTRL�жϴ���ʽ��
static volatile uint8_t g_k1_en_interrupt_flag = 0;
static volatile uint8_t g_k2_en_interrupt_flag = 0;
static volatile uint8_t g_k3_en_interrupt_flag = 0;
static volatile uint32_t g_k1_en_interrupt_time = 0;
static volatile uint32_t g_k2_en_interrupt_time = 0;
static volatile uint32_t g_k3_en_interrupt_time = 0;

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
    DEBUG_Printf("�̵�������ģ���ʼ����ɣ����п����ź���Ϊ�ߵ�ƽ�������ܽ�ֹ��\r\n");
    
    // ��¼�̵���ģ���ʼ����־
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "�̵�������ģ���ʼ�����");
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
  * @brief  �򿪼̵���ͨ��
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:�ɹ� ����:�������
  */
uint8_t RelayControl_OpenChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_ERR_INVALID_CHANNEL;
    
    // ����Ƿ����O���쳣����Դ�쳣��
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
        DEBUG_Printf("ͨ��%d��������ֹ����⵽O���쳣����Դ�쳣��\r\n", channelNum);
        
        // ��¼��Դ�쳣��ֹ������־
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "ͨ��%d��������ֹ-��Դ�쳣", channelNum);
        LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_POWER_FAILURE, log_msg);
        
        return RELAY_ERR_POWER_FAILURE;
    }
    
    uint8_t idx = channelNum - 1;
    // ��黥��
    if(RelayControl_CheckInterlock(channelNum)) {
        relayChannels[idx].errorCode = RELAY_ERR_INTERLOCK;
        DEBUG_Printf("ͨ��%d��������\r\n", channelNum);
        
        // ��¼����������־�����ǰ�ȫ�ؼ��쳣��
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "ͨ��%d��������", channelNum);
        LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_A, log_msg);
        
        return RELAY_ERR_INTERLOCK;
    }
    
    DEBUG_Printf("ͨ��%d������ - ���500ms�͵�ƽ���壨�����ܵ�ͨ��\r\n", channelNum);
    
    // ���500ms�͵�ƽ���壨�����ܵ�ͨ���̵���������
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
    SmartDelayWithDebug(RELAY_PULSE_WIDTH, "�̵�����������");
    
    // ����������ָ��ߵ�ƽ�������ܽ�ֹ��
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
    
    DEBUG_Printf("ͨ��%d���������ɣ��ȴ�����\r\n", channelNum);
    
    // ��ʱ500ms�󣬼��Ӳ�������پ����ڲ�״̬
    SmartDelayWithDebug(RELAY_FEEDBACK_DELAY, "�̵������������ȴ�");
    
    // ���Ӳ���Ƿ�����������ֱ�Ӽ��Ӳ��״̬���������ڲ�״̬��
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
        // Ӳ���ɹ������������ڲ�״̬ΪON
        relayChannels[idx].state = RELAY_STATE_ON;
        relayChannels[idx].errorCode = RELAY_ERR_NONE;
        DEBUG_Printf("ͨ��%d�����ɹ�\r\n", channelNum);
        
        return RELAY_ERR_NONE;
    } else {
        // Ӳ������ʧ�ܣ���¼����
        relayChannels[idx].state = RELAY_STATE_ERROR;
        relayChannels[idx].errorCode = RELAY_ERR_HARDWARE_FAILURE;
        DEBUG_Printf("ͨ��%d����ʧ�� - Ӳ�������쳣\r\n", channelNum);
        
        // ��ȷ���ÿ���̵����ͽӴ������쳣״̬����¼��Ӧ��־
        char log_msg[64];
        uint8_t k1_sta = 0, k2_sta = 0, sw_sta = 0;
        
        switch(channelNum) {
            case 1:
                k1_sta = GPIO_ReadK1_1_STA();
                k2_sta = GPIO_ReadK1_2_STA();
                sw_sta = GPIO_ReadSW1_STA();
                
                if(!k1_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K1_1�̵��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_B, log_msg);
                }
                if(!k2_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K1_2�̵��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_E, log_msg);
                }
                if(!sw_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d SW1�Ӵ��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_H, log_msg);
                }
                break;
                
            case 2:
                k1_sta = GPIO_ReadK2_1_STA();
                k2_sta = GPIO_ReadK2_2_STA();
                sw_sta = GPIO_ReadSW2_STA();
                
                if(!k1_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K2_1�̵��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_C, log_msg);
                }
                if(!k2_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K2_2�̵��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_F, log_msg);
                }
                if(!sw_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d SW2�Ӵ��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_I, log_msg);
                }
                break;
                
            case 3:
                k1_sta = GPIO_ReadK3_1_STA();
                k2_sta = GPIO_ReadK3_2_STA();
                sw_sta = GPIO_ReadSW3_STA();
                
                if(!k1_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K3_1�̵��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_D, log_msg);
                }
                if(!k2_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K3_2�̵��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_G, log_msg);
                }
                if(!sw_sta) {
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d SW3�Ӵ��������쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_J, log_msg);
                }
                break;
        }
        
        return RELAY_ERR_HARDWARE_FAILURE;
    }
}

/**
  * @brief  �رռ̵���ͨ��
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:�ɹ� ����:�������
  */
uint8_t RelayControl_CloseChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_ERR_INVALID_CHANNEL;
    
    uint8_t idx = channelNum - 1;
    DEBUG_Printf("ͨ��%d�ر��� - ���500ms�͵�ƽ���壨�����ܵ�ͨ��\r\n", channelNum);
    
    // ���500ms�͵�ƽ���壨�����ܵ�ͨ���̵���������
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
    SmartDelayWithDebug(RELAY_PULSE_WIDTH, "�̵����ر�����");
    
    // ����������ָ��ߵ�ƽ�������ܽ�ֹ��
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
    
    DEBUG_Printf("ͨ��%d���������ɣ��ȴ�����\r\n", channelNum);
    
    // ��ʱ500ms�󣬼��Ӳ�������پ����ڲ�״̬
    SmartDelayWithDebug(RELAY_FEEDBACK_DELAY, "�̵����رշ����ȴ�");
    
    // ���Ӳ���Ƿ������رգ�ֱ�Ӽ��Ӳ��״̬���������ڲ�״̬��
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
        // Ӳ���ɹ��رգ������ڲ�״̬ΪOFF
        relayChannels[idx].state = RELAY_STATE_OFF;
        relayChannels[idx].errorCode = RELAY_ERR_NONE;
        DEBUG_Printf("ͨ��%d�رճɹ�\r\n", channelNum);
        
        return RELAY_ERR_NONE;
    } else {
        // Ӳ���ر�ʧ�ܣ���¼����
        relayChannels[idx].state = RELAY_STATE_ERROR;
        relayChannels[idx].errorCode = RELAY_ERR_HARDWARE_FAILURE;
        DEBUG_Printf("ͨ��%d�ر�ʧ�� - Ӳ�������쳣\r\n", channelNum);
        
        // ��ȷ���ÿ���̵����ͽӴ������쳣״̬����¼��Ӧ��־
        char log_msg[64];
        uint8_t k1_sta = 0, k2_sta = 0, sw_sta = 0;
        
        switch(channelNum) {
            case 1:
                k1_sta = GPIO_ReadK1_1_STA();
                k2_sta = GPIO_ReadK1_2_STA();
                sw_sta = GPIO_ReadSW1_STA();
                
                if(k1_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K1_1�̵����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_B, log_msg);
                }
                if(k2_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K1_2�̵����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_E, log_msg);
                }
                if(sw_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d SW1�Ӵ����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_H, log_msg);
                }
                break;
                
            case 2:
                k1_sta = GPIO_ReadK2_1_STA();
                k2_sta = GPIO_ReadK2_2_STA();
                sw_sta = GPIO_ReadSW2_STA();
                
                if(k1_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K2_1�̵����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_C, log_msg);
                }
                if(k2_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K2_2�̵����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_F, log_msg);
                }
                if(sw_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d SW2�Ӵ����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_I, log_msg);
                }
                break;
                
            case 3:
                k1_sta = GPIO_ReadK3_1_STA();
                k2_sta = GPIO_ReadK3_2_STA();
                sw_sta = GPIO_ReadSW3_STA();
                
                if(k1_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K3_1�̵����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_D, log_msg);
                }
                if(k2_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d K3_2�̵����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_G, log_msg);
                }
                if(sw_sta) {  // �ر�ʧ��ʱӦ��Ϊ0�����Ϊ1���쳣
                    snprintf(log_msg, sizeof(log_msg), "ͨ��%d SW3�Ӵ����ر��쳣", channelNum);
                    LogSystem_Record(LOG_CATEGORY_SAFETY, channelNum, LOG_EVENT_SAFETY_ALARM_J, log_msg);
                }
                break;
        }
        
        return RELAY_ERR_HARDWARE_FAILURE;
    }
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
  * @brief  ����ʹ���źű仯���жϷ�ʽ�����ñ�־λ����ѭ����������߼���
  * @param  channelNum: ͨ����(1-3)
  * @param  state: �ź�״̬��0=�͵�ƽ=������1=�ߵ�ƽ=�رգ�
  * @retval ��
  * @note   �ж��н����ñ�־λ�����崦���߼�����ѭ����ִ�У������ж�����
  */
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state)
{
    // ? �жϴ���򻯣�ֻ���ñ�־λ�����⸴�Ӳ������¿��Ź���λ
    // ����ļ̵��������߼�������ѭ���д���
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
            // ��Чͨ���ţ�����
            break;
    }
}

/**
  * @brief  ����K_EN�жϱ�־��ִ�м̵�������������ѭ���е��ã�
  * @retval ��
  * @note   �����жϱ�־λ��������ѯ�ӳ٣������Ӧ�ٶ�
  */
void RelayControl_ProcessPendingActions(void)
{
    // ���ϵͳ״̬��������ڱ���״̬��ֹͣ����
    extern SystemState_t SystemControl_GetState(void);
    SystemState_t system_state = SystemControl_GetState();
    if(system_state == SYSTEM_STATE_ALARM) {
        // ����״̬��ֹͣ�̵�������������������ѭ�����¿��Ź���λ
        return;
    }
    
    // ����Ƿ���ڹؼ��쳣�����������ֹͣ����
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_B) ||  // K1_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_C) ||  // K2_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_D) ||  // K3_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_E) ||  // K1_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_F) ||  // K2_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_G) ||  // K3_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {  // �ⲿ��Դ�쳣��DC_CTRL��
        // �����ؼ��쳣ʱֹͣ�������ֵ�ǰͨ��״̬
        return;
    }
    
    uint32_t current_time = HAL_GetTick();
    
    // ================== ����K1_EN�жϱ�־ ===================
    if(g_k1_en_interrupt_flag) {
        g_k1_en_interrupt_flag = 0;  // �����־λ
        
        uint8_t k1_en = GPIO_ReadK1_EN();
        uint8_t k1_sta_all = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
        uint32_t interrupt_time = g_k1_en_interrupt_time;
        
        DEBUG_Printf("[�жϴ���] K1_EN�ж�: ʱ��=%lu, �ж�ʱ��=%lu, K1_EN=%d, STA=%d\r\n", 
                    current_time, interrupt_time, k1_en, k1_sta_all);
        
        // �ж��߼������K_ENΪ0��STAҲΪ0����ִ�п�������
        if(k1_en == 0 && k1_sta_all == 0) {
            DEBUG_Printf("[�жϴ���] ͨ��1: K_EN=0��STA=0��ִ�п�������\r\n");
            uint8_t result = RelayControl_OpenChannel(1);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��1�����ɹ�\r\n");
            } else {
                DEBUG_Printf("ͨ��1����ʧ�ܣ�������=%d\r\n", result);
            }
        }
        // �ж��߼������K_ENΪ1��STAҲΪ1����ִ�йرն���
        else if(k1_en == 1 && k1_sta_all == 1) {
            DEBUG_Printf("[�жϴ���] ͨ��1: K_EN=1��STA=1��ִ�йرն���\r\n");
            uint8_t result = RelayControl_CloseChannel(1);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��1�رճɹ�\r\n");
            } else {
                DEBUG_Printf("ͨ��1�ر�ʧ�ܣ�������=%d\r\n", result);
            }
        }
    }
    
    // ================== ����K2_EN�жϱ�־ ===================
    if(g_k2_en_interrupt_flag) {
        g_k2_en_interrupt_flag = 0;  // �����־λ
        
        uint8_t k2_en = GPIO_ReadK2_EN();
        uint8_t k2_sta_all = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
        uint32_t interrupt_time = g_k2_en_interrupt_time;
        
        DEBUG_Printf("[�жϴ���] K2_EN�ж�: ʱ��=%lu, �ж�ʱ��=%lu, K2_EN=%d, STA=%d\r\n", 
                    current_time, interrupt_time, k2_en, k2_sta_all);
        
        // �ж��߼������K_ENΪ0��STAҲΪ0����ִ�п�������
        if(k2_en == 0 && k2_sta_all == 0) {
            DEBUG_Printf("[�жϴ���] ͨ��2: K_EN=0��STA=0��ִ�п�������\r\n");
            uint8_t result = RelayControl_OpenChannel(2);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��2�����ɹ�\r\n");
            } else {
                DEBUG_Printf("ͨ��2����ʧ�ܣ�������=%d\r\n", result);
            }
        }
        // �ж��߼������K_ENΪ1��STAҲΪ1����ִ�йرն���
        else if(k2_en == 1 && k2_sta_all == 1) {
            DEBUG_Printf("[�жϴ���] ͨ��2: K_EN=1��STA=1��ִ�йرն���\r\n");
            uint8_t result = RelayControl_CloseChannel(2);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��2�رճɹ�\r\n");
            } else {
                DEBUG_Printf("ͨ��2�ر�ʧ�ܣ�������=%d\r\n", result);
            }
        }
    }
    
    // ================== ����K3_EN�жϱ�־ ===================
    if(g_k3_en_interrupt_flag) {
        g_k3_en_interrupt_flag = 0;  // �����־λ
        
        uint8_t k3_en = GPIO_ReadK3_EN();
        uint8_t k3_sta_all = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
        uint32_t interrupt_time = g_k3_en_interrupt_time;
        
        DEBUG_Printf("[�жϴ���] K3_EN�ж�: ʱ��=%lu, �ж�ʱ��=%lu, K3_EN=%d, STA=%d\r\n", 
                    current_time, interrupt_time, k3_en, k3_sta_all);
        
        // �ж��߼������K_ENΪ0��STAҲΪ0����ִ�п�������
        if(k3_en == 0 && k3_sta_all == 0) {
            DEBUG_Printf("[�жϴ���] ͨ��3: K_EN=0��STA=0��ִ�п�������\r\n");
            uint8_t result = RelayControl_OpenChannel(3);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��3�����ɹ�\r\n");
            } else {
                DEBUG_Printf("ͨ��3����ʧ�ܣ�������=%d\r\n", result);
            }
        }
        // �ж��߼������K_ENΪ1��STAҲΪ1����ִ�йرն���
        else if(k3_en == 1 && k3_sta_all == 1) {
            DEBUG_Printf("[�жϴ���] ͨ��3: K_EN=1��STA=1��ִ�йرն���\r\n");
            uint8_t result = RelayControl_CloseChannel(3);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��3�رճɹ�\r\n");
            } else {
                DEBUG_Printf("ͨ��3�ر�ʧ�ܣ�������=%d\r\n", result);
            }
        }
    }
} 





