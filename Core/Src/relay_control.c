#include "relay_control.h"
#include "gpio_control.h"
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
  * @brief  ���ͨ������״̬
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:���� 1:�쳣
  */
uint8_t RelayControl_CheckChannelFeedback(uint8_t channelNum)
{
    uint8_t result = 0;
    // ������ϸ�����������ӡ��ǰͨ��״̬�ͷ������ƽ
    DEBUG_Printf("[����] ͨ��%d ״̬=%d K1_1=%d K1_2=%d SW1=%d K2_1=%d K2_2=%d SW2=%d K3_1=%d K3_2=%d SW3=%d\r\n",
        channelNum,
        relayChannels[channelNum-1].state,
        GPIO_ReadK1_1_STA(), GPIO_ReadK1_2_STA(), GPIO_ReadSW1_STA(),
        GPIO_ReadK2_1_STA(), GPIO_ReadK2_2_STA(), GPIO_ReadSW2_STA(),
        GPIO_ReadK3_1_STA(), GPIO_ReadK3_2_STA(), GPIO_ReadSW3_STA());
    switch(channelNum) {
        case 1:
            if(relayChannels[0].state == RELAY_STATE_ON) {
                if(GPIO_ReadK1_1_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] K1_1_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadK1_2_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] K1_2_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW1_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] SW1_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
            } else if(relayChannels[0].state == RELAY_STATE_OFF) {
                if(GPIO_ReadK1_1_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] K1_1_STAδ�ص��͵�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadK1_2_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] K1_2_STAδ�ص��͵�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW1_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] SW1_STAδ�ص��͵�ƽ\r\n");
                    result = 1;
                }
            }
            break;
        case 2:
            if(relayChannels[1].state == RELAY_STATE_ON) {
                if(GPIO_ReadK2_1_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] K2_1_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadK2_2_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] K2_2_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW2_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] SW2_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
            } else if(relayChannels[1].state == RELAY_STATE_OFF) {
                if(GPIO_ReadK2_1_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] K2_1_STAδ�ص��͵�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadK2_2_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] K2_2_STAδ�ص��͵�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW2_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] SW2_STAδ�ص��͵�ƽ\r\n");
                    result = 1;
                }
            }
            break;
        case 3:
            if(relayChannels[2].state == RELAY_STATE_ON) {
                if(GPIO_ReadK3_1_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] K3_1_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadK3_2_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] K3_2_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW3_STA() != GPIO_PIN_SET) {
                    DEBUG_Printf("[�����쳣] SW3_STAδ���ߵ�ƽ\r\n");
                    result = 1;
                }
            } else if(relayChannels[2].state == RELAY_STATE_OFF) {
                if(GPIO_ReadK3_1_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] K3_1_STAδ�ص��͵�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadK3_2_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] K3_2_STAδ�ص��͵�ƽ\r\n");
                    result = 1;
                }
                if(GPIO_ReadSW3_STA() != GPIO_PIN_RESET) {
                    DEBUG_Printf("[�����쳣] SW3_STAδ�ص��͵�ƽ\r\n");
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
  * @brief  �򿪼̵���ͨ��
  * @param  channelNum: ͨ����(1-3)
  * @retval 0:�ɹ� ����:�������
  */
uint8_t RelayControl_OpenChannel(uint8_t channelNum)
{
    if(channelNum < 1 || channelNum > 3)
        return RELAY_ERR_INVALID_CHANNEL;
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
    
    // ��ʱ500ms���ȸ���״̬�ټ�鷴��
    HAL_Delay(RELAY_FEEDBACK_DELAY);
    relayChannels[idx].state = RELAY_STATE_ON; // �ȸ���״̬
    if(RelayControl_CheckChannelFeedback(channelNum)) {
        relayChannels[idx].errorCode = RELAY_ERR_FEEDBACK;
        relayChannels[idx].state = RELAY_STATE_ERROR;
        DEBUG_Printf("ͨ��%d��������\r\n", channelNum);
        return RELAY_ERR_FEEDBACK;
    }
    relayChannels[idx].errorCode = RELAY_ERR_NONE;
    DEBUG_Printf("ͨ��%d�����ɹ�\r\n", channelNum);
    return RELAY_ERR_NONE;
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
    
    // ��ʱ500ms���ȸ���״̬�ټ�鷴��
    HAL_Delay(RELAY_FEEDBACK_DELAY);
    relayChannels[idx].state = RELAY_STATE_OFF; // �ȸ���״̬
    if(RelayControl_CheckChannelFeedback(channelNum)) {
        relayChannels[idx].errorCode = RELAY_ERR_FEEDBACK;
        relayChannels[idx].state = RELAY_STATE_ERROR;
        DEBUG_Printf("ͨ��%d��������\r\n", channelNum);
        return RELAY_ERR_FEEDBACK;
    }
    relayChannels[idx].errorCode = RELAY_ERR_NONE;
    DEBUG_Printf("ͨ��%d�رճɹ�\r\n", channelNum);
    return RELAY_ERR_NONE;
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
  * @brief  ����ʹ���źű仯�����ж��е��ã����ؼ�⣬��������
  * @param  channelNum: ͨ����(1-3)
  * @param  state: �ź�״̬��0=�͵�ƽ=������1=�ߵ�ƽ=�رգ�
  * @retval ��
  */
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state)
{
    if(channelNum < 1 || channelNum > 3)
        return;

    relayChannels[channelNum - 1].lastActionTime = HAL_GetTick();

    if(state == 0) {
        pendingAction[channelNum - 1] = 1;
        DEBUG_Printf("[�ж�] ͨ��%d��⵽�½��أ�������\r\n", channelNum);
    } else {
        pendingAction[channelNum - 1] = 2;
        DEBUG_Printf("[�ж�] ͨ��%d��⵽�����أ����ر�\r\n", channelNum);
    }
}

/**
  * @brief  ���������ļ̵�������������ѭ���е��ã����жϻ�����
  * @retval ��
  */
void RelayControl_ProcessPendingActions(void)
{
    for(uint8_t i = 0; i < 3; i++) {
        if(pendingAction[i] == 1) { // ����ͨ��
            pendingAction[i] = 0;
            uint8_t result = RelayControl_OpenChannel(i + 1);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��%d�����ɹ�\r\n", i + 1);
            } else {
                DEBUG_Printf("ͨ��%d����ʧ�ܣ�������=%d\r\n", i + 1, result);
            }
        } else if(pendingAction[i] == 2) { // �ر�ͨ��
            pendingAction[i] = 0;
            uint8_t result = RelayControl_CloseChannel(i + 1);
            if(result == RELAY_ERR_NONE) {
                DEBUG_Printf("ͨ��%d�رճɹ�\r\n", i + 1);
            } else {
                DEBUG_Printf("ͨ��%d�ر�ʧ�ܣ�������=%d\r\n", i + 1, result);
            }
        }
    }
} 





