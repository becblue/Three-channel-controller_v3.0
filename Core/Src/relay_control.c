#include "relay_control.h"
#include "gpio_control.h"
#include "safety_monitor.h"
#include "usart.h"

/************************************************************
 * �̵�������ģ��Դ�ļ�
 * ʵ����ͨ���̵����Ŀ���/�رա����ؼ�⡢������顢500ms���塢״̬������⡢�쳣������״̬����
 ************************************************************/

// �����̵���ͨ��ʵ��
static RelayChannel_t relayChannels[3];
static uint8_t enableCheckCount[3] = {0}; // ���ؼ�������
static uint8_t pendingAction[3] = {0}; // ������Ķ�����0=�޶�����1=������2=�ر�

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
        enableCheckCount[i] = 0;
        pendingAction[i] = 0;
        
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
        return RELAY_ERR_POWER_FAILURE;
    }
    
    uint8_t idx = channelNum - 1;
    // ��黥��
    if(RelayControl_CheckInterlock(channelNum)) {
        relayChannels[idx].errorCode = RELAY_ERR_INTERLOCK;
        DEBUG_Printf("ͨ��%d��������\r\n", channelNum);
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
    HAL_Delay(RELAY_PULSE_WIDTH);
    
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
    HAL_Delay(RELAY_FEEDBACK_DELAY);
    
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
        // Ӳ��δ�ܿ����������ڲ�״̬ΪOFF
        relayChannels[idx].state = RELAY_STATE_OFF;
        relayChannels[idx].errorCode = RELAY_ERR_FEEDBACK;
        DEBUG_Printf("ͨ��%d����ʧ�ܣ�Ӳ�������쳣\r\n", channelNum);
        return RELAY_ERR_FEEDBACK;
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
    HAL_Delay(RELAY_PULSE_WIDTH);
    
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
    HAL_Delay(RELAY_FEEDBACK_DELAY);
    
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
        // Ӳ��δ�ܹرգ������ڲ�״̬ΪON����ӳ��ʵӲ��״̬��
        relayChannels[idx].state = RELAY_STATE_ON;
        relayChannels[idx].errorCode = RELAY_ERR_FEEDBACK;
        DEBUG_Printf("ͨ��%d�ر�ʧ�ܣ�Ӳ�������쳣\r\n", channelNum);
        return RELAY_ERR_FEEDBACK;
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
  * @brief  ����ʹ���źű仯�������ã�ԭ�жϷ�ʽ���ָ�Ϊ��ѯ��ʽ��
  * @param  channelNum: ͨ����(1-3)
  * @param  state: �ź�״̬��0=�͵�ƽ=������1=�ߵ�ƽ=�رգ�
  * @retval ��
  * @note   �˺��������ã�����ʹ����ѯ��ʽ���K_EN�źű仯
  */
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state)
{
    // �˺��������ã�����ʹ����ѯ��ʽ��RelayControl_ProcessPendingActions�д���
    // �������������Ա��ּ����ԣ�����ִ���κβ���
    (void)channelNum;
    (void)state;
    // DEBUG_Printf("[����] RelayControl_HandleEnableSignal�����ã���ʹ����ѯ��ʽ\r\n");  // ��ע�� - �����ظ����
}

/**
  * @brief  ��ѯ���K_EN�źŲ�����̵�������������ѭ���е��ã�
  * @retval ��
  */
void RelayControl_ProcessPendingActions(void)
{
    // ����Ƿ���ڹؼ��쳣�����������ֹͣ��ѯ
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_B) ||  // K1_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_C) ||  // K2_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_D) ||  // K3_1_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_E) ||  // K1_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_F) ||  // K2_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_G) ||  // K3_2_STA�����쳣
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {  // �ⲿ��Դ�쳣��DC_CTRL��
        // �����ؼ��쳣ʱֹͣ��ѯ�����ֵ�ǰͨ��״̬
        DEBUG_Printf("[��ѯ] ��⵽�ؼ��쳣��ֹͣ��ѯ����\r\n");
        return;
    }
    
    // ��ѯ�������ͨ����K_EN��STA�ź�
    for(uint8_t i = 0; i < 3; i++) {
        uint8_t channelNum = i + 1;
        uint8_t k_en = 0;
        uint8_t sta_all = 0;
        
        // ��ȡ��Ӧͨ����K_EN�ź�
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
        
        // ��ѯ�߼������K_ENΪ0��STAҲΪ0�������500ms�͵�ƽ���嵽ON�ź�
        if(k_en == 0 && sta_all == 0) {
            DEBUG_Printf("[��ѯ] ͨ��%d: K_EN=0��STA=0��ִ�п�������\r\n", channelNum);
            uint8_t result = RelayControl_OpenChannel(channelNum);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��%d�����ɹ�\r\n", channelNum);
            } else {
                DEBUG_Printf("ͨ��%d����ʧ�ܣ�������=%d\r\n", channelNum, result);
            }
        }
        // ��ѯ�߼������K_ENΪ1��STAҲΪ1�������500ms�͵�ƽ���嵽OFF�ź�
        else if(k_en == 1 && sta_all == 1) {
            DEBUG_Printf("[��ѯ] ͨ��%d: K_EN=1��STA=1��ִ�йرն���\r\n", channelNum);
            uint8_t result = RelayControl_CloseChannel(channelNum);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��%d�رճɹ�\r\n", channelNum);
            } else {
                DEBUG_Printf("ͨ��%d�ر�ʧ�ܣ�������=%d\r\n", channelNum, result);
            }
        }
    }
} 





