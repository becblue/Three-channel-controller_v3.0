#include "safety_monitor.h"
#include "gpio_control.h"
#include "relay_control.h"
#include "temperature_monitor.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

/************************************************************
 * ��ȫ���ģ��Դ�ļ�
 * ʵ���쳣��־����������������������ơ���Դ���
 * �ϸ���READMEҪ��ʵ�����б����߼��ͽ������
 ************************************************************/

// ȫ�ְ�ȫ��ؽṹ��
static SafetyMonitor_t g_safety_monitor;

// �쳣��־�����ַ�����Ӣ�İ汾������OLED��ʾ��
static const char* g_alarm_descriptions[ALARM_FLAG_COUNT] = {
    "EN Conflict",           // A - K1_EN/K2_EN/K3_ENʹ�ܳ�ͻ (11�ַ�)
    "K1_1_STA Error",        // B - K1_1_STA�����쳣 (13�ַ�)
    "K2_1_STA Error",        // C - K2_1_STA�����쳣 (13�ַ�)
    "K3_1_STA Error",        // D - K3_1_STA�����쳣 (13�ַ�)
    "K1_2_STA Error",        // E - K1_2_STA�����쳣 (13�ַ�)
    "K2_2_STA Error",        // F - K2_2_STA�����쳣 (13�ַ�)
    "K3_2_STA Error",        // G - K3_2_STA�����쳣 (13�ַ�)
    "SW1_STA Error",         // H - SW1_STA�����쳣 (12�ַ�)
    "SW2_STA Error",         // I - SW2_STA�����쳣 (12�ַ�)
    "SW3_STA Error",         // J - SW3_STA�����쳣 (12�ַ�)
    "NTC1 Overheat",         // K - NTC_1�¶��쳣 (12�ַ�)
    "NTC2 Overheat",         // L - NTC_2�¶��쳣 (12�ַ�)
    "NTC3 Overheat",         // M - NTC_3�¶��쳣 (12�ַ�)
    "Self-Test Fail",        // N - �Լ��쳣 (13�ַ�)
    "Power Failure"          // O - �ⲿ��Դ�쳣 (12�ַ�)
};

// ��ȡ�쳣���ͣ������쳣��־ȷ���������ͣ�
static AlarmType_t SafetyMonitor_GetAlarmType(AlarmFlag_t flag);

// ================== ���Ĺ����� ===================

/**
  * @brief  ��ȫ���ģ���ʼ��
  * @retval ��
  */
void SafetyMonitor_Init(void)
{
    // ��ʼ����ȫ��ؽṹ��
    memset(&g_safety_monitor, 0, sizeof(SafetyMonitor_t));
    
    // ��ʼ���쳣��Ϣ
    for(uint8_t i = 0; i < ALARM_FLAG_COUNT; i++) {
        g_safety_monitor.alarm_info[i].is_active = 0;
        g_safety_monitor.alarm_info[i].type = SafetyMonitor_GetAlarmType((AlarmFlag_t)i);
        g_safety_monitor.alarm_info[i].trigger_time = 0;
        strncpy(g_safety_monitor.alarm_info[i].description, 
                g_alarm_descriptions[i], 
                sizeof(g_safety_monitor.alarm_info[i].description) - 1);
    }
    
    // ��ʼ����������ͷ�����״̬
    g_safety_monitor.alarm_output_active = 0;
    g_safety_monitor.beep_state = BEEP_STATE_OFF;
    g_safety_monitor.beep_current_level = 0;
    g_safety_monitor.beep_last_toggle_time = HAL_GetTick();
    
    // ���õ�Դ���
    g_safety_monitor.power_monitor_enabled = 1;
    
    // ��ʼ��Ӳ������״̬
    GPIO_SetAlarmOutput(0);  // ALARM���ų�ʼ��Ϊ�ߵ�ƽ������״̬��
    GPIO_SetBeepOutput(0);   // ��������ʼ��Ϊ�ߵ�ƽ���ر�״̬��
    
    DEBUG_Printf("��ȫ���ģ���ʼ�����\r\n");
}

/**
  * @brief  ��ȫ���������������ѭ���е��ã�
  * @retval ��
  */
void SafetyMonitor_Process(void)
{
    // 1. ��鲢���������쳣״̬
    SafetyMonitor_UpdateAllAlarmStatus();
    
    // 2. ����ALARM�������
    SafetyMonitor_UpdateAlarmOutput();
    
    // 3. ���·�����״̬
    SafetyMonitor_UpdateBeepState();
    
    // 4. �������������
    SafetyMonitor_ProcessBeep();
}

/**
  * @brief  ��ȡ��ǰ�쳣״̬�������쳣��־λͼ��
  * @retval �쳣��־λͼ
  */
uint16_t SafetyMonitor_GetAlarmFlags(void)
{
    return g_safety_monitor.alarm_flags;
}

/**
  * @brief  ���ָ���쳣�Ƿ񼤻�
  * @param  flag: �쳣��־
  * @retval 1:���� 0:δ����
  */
uint8_t SafetyMonitor_IsAlarmActive(AlarmFlag_t flag)
{
    if(flag >= ALARM_FLAG_COUNT) return 0;
    return (g_safety_monitor.alarm_flags & (1 << flag)) ? 1 : 0;
}

/**
  * @brief  ��ȡ�쳣��Ϣ
  * @param  flag: �쳣��־
  * @retval �쳣��Ϣ�ṹ��
  */
AlarmInfo_t SafetyMonitor_GetAlarmInfo(AlarmFlag_t flag)
{
    AlarmInfo_t empty_info = {0};
    if(flag >= ALARM_FLAG_COUNT) return empty_info;
    return g_safety_monitor.alarm_info[flag];
}

// ================== �쳣��־���� ===================

/**
  * @brief  �����쳣��־
  * @param  flag: �쳣��־
  * @param  description: �쳣��������ѡ��NULLʹ��Ĭ��������
  * @retval ��
  */
void SafetyMonitor_SetAlarmFlag(AlarmFlag_t flag, const char* description)
{
    if(flag >= ALARM_FLAG_COUNT) return;
    
    // ����쳣�Ѿ�������ظ�����
    if(SafetyMonitor_IsAlarmActive(flag)) return;
    
    // �����쳣��־λ
    g_safety_monitor.alarm_flags |= (1 << flag);
    
    // �����쳣��Ϣ
    g_safety_monitor.alarm_info[flag].is_active = 1;
    g_safety_monitor.alarm_info[flag].trigger_time = HAL_GetTick();
    
    if(description) {
        strncpy(g_safety_monitor.alarm_info[flag].description, 
                description, 
                sizeof(g_safety_monitor.alarm_info[flag].description) - 1);
    }
    
    DEBUG_Printf("[��ȫ���] �쳣��־����: %c���쳣 - %s\r\n", 
                'A' + flag, g_safety_monitor.alarm_info[flag].description);
}

/**
  * @brief  ����쳣��־
  * @param  flag: �쳣��־
  * @retval ��
  */
void SafetyMonitor_ClearAlarmFlag(AlarmFlag_t flag)
{
    if(flag >= ALARM_FLAG_COUNT) return;
    
    // ����쳣δ����������
    if(!SafetyMonitor_IsAlarmActive(flag)) return;
    
    // ����쳣��־λ
    g_safety_monitor.alarm_flags &= ~(1 << flag);
    
    // �����쳣��Ϣ
    g_safety_monitor.alarm_info[flag].is_active = 0;
    
    DEBUG_Printf("[��ȫ���] �쳣��־���: %c���쳣���\r\n", 'A' + flag);
}

/**
  * @brief  ��������쳣��־
  * @retval ��
  */
void SafetyMonitor_ClearAllAlarmFlags(void)
{
    for(uint8_t i = 0; i < ALARM_FLAG_COUNT; i++) {
        if(SafetyMonitor_IsAlarmActive((AlarmFlag_t)i)) {
            SafetyMonitor_ClearAlarmFlag((AlarmFlag_t)i);
        }
    }
    DEBUG_Printf("[��ȫ���] �����쳣��־�����\r\n");
}

/**
  * @brief  ��鲢���������쳣״̬
  * @retval ��
  */
void SafetyMonitor_UpdateAllAlarmStatus(void)
{
    // 1. ���ʹ���źų�ͻ��A���쳣��
    SafetyMonitor_CheckEnableConflict();
    
    // 2. ���̵���״̬�쳣��B~G���쳣��
    SafetyMonitor_CheckRelayStatus();
    
    // 3. ���Ӵ���״̬�쳣��H~J���쳣��
    SafetyMonitor_CheckContactorStatus();
    
    // 4. ����¶��쳣��K~M���쳣��
    SafetyMonitor_CheckTemperatureAlarm();
    
    // 5. ����Դ����쳣��O���쳣��
    SafetyMonitor_CheckPowerMonitor();
    
    // ����쳣�������
    for(uint8_t i = 0; i < ALARM_FLAG_COUNT; i++) {
        if(SafetyMonitor_IsAlarmActive((AlarmFlag_t)i)) {
            uint8_t can_clear = 0;
            
            if(i == ALARM_FLAG_A) {
                can_clear = SafetyMonitor_CheckAlarmA_ClearCondition();
            } else if((i >= ALARM_FLAG_B && i <= ALARM_FLAG_J) || i == ALARM_FLAG_N) {
                can_clear = SafetyMonitor_CheckAlarmBJN_ClearCondition();
            } else if(i >= ALARM_FLAG_K && i <= ALARM_FLAG_M) {
                can_clear = SafetyMonitor_CheckAlarmKM_ClearCondition();
            } else if(i == ALARM_FLAG_O) {
                can_clear = SafetyMonitor_CheckAlarmO_ClearCondition();
            }
            
            if(can_clear) {
                SafetyMonitor_ClearAlarmFlag((AlarmFlag_t)i);
            }
        }
    }
}

// ================== ����������� ===================

/**
  * @brief  ����ALARM������������κ��쳣ʱ����͵�ƽ��
  * @retval ��
  */
void SafetyMonitor_UpdateAlarmOutput(void)
{
    uint8_t should_alarm = (g_safety_monitor.alarm_flags != 0) ? 1 : 0;
    
    if(g_safety_monitor.alarm_output_active != should_alarm) {
        g_safety_monitor.alarm_output_active = should_alarm;
        GPIO_SetAlarmOutput(should_alarm);  // 1:�͵�ƽ��� 0:�ߵ�ƽ���
        
        DEBUG_Printf("[��ȫ���] ALARM�������: %s\r\n", 
                    should_alarm ? "�͵�ƽ��������" : "�ߵ�ƽ��������");
    }
}

/**
  * @brief  ǿ������ALARM����״̬
  * @param  active: 1:�͵�ƽ��� 0:�ߵ�ƽ���
  * @retval ��
  */
void SafetyMonitor_ForceAlarmOutput(uint8_t active)
{
    g_safety_monitor.alarm_output_active = active;
    GPIO_SetAlarmOutput(active);
    DEBUG_Printf("[��ȫ���] ǿ������ALARM����: %s\r\n", 
                active ? "�͵�ƽ" : "�ߵ�ƽ");
}

// ================== ���������� ===================

/**
  * @brief  ���·�����״̬�������쳣���;������巽ʽ��
  * @retval ��
  */
void SafetyMonitor_UpdateBeepState(void)
{
    BeepState_t new_state = BEEP_STATE_OFF;
    
    // �����ȼ�����쳣���ͣ�K~M��������ȼ���
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_K) || 
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_L) || 
       SafetyMonitor_IsAlarmActive(ALARM_FLAG_M)) {
        new_state = BEEP_STATE_CONTINUOUS;  // �����͵�ƽ
    } else if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_B) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_C) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_D) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_E) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_F) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_G) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_H) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_I) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_J)) {
        new_state = BEEP_STATE_PULSE_50MS;  // 50ms�������
    } else if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_A) || 
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_N) ||
              SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {  // ���O���쳣����
        new_state = BEEP_STATE_PULSE_1S;    // 1��������
    }
    
    // ���״̬�ı䣬���·�����
    if(g_safety_monitor.beep_state != new_state) {
        g_safety_monitor.beep_state = new_state;
        g_safety_monitor.beep_last_toggle_time = HAL_GetTick();
        g_safety_monitor.beep_current_level = 0;
        
        DEBUG_Printf("[��ȫ���] ������״̬�л�: %s\r\n", 
                    SafetyMonitor_GetBeepStateDescription(new_state));
        
        // �������·��������
        if(new_state == BEEP_STATE_CONTINUOUS) {
            GPIO_SetBeepOutput(1);  // �����͵�ƽ
            g_safety_monitor.beep_current_level = 1;
        } else if(new_state == BEEP_STATE_OFF) {
            GPIO_SetBeepOutput(0);  // �ر�
            g_safety_monitor.beep_current_level = 0;
        }
    }
}

/**
  * @brief  ���������崦����ʱ���ã�
  * @retval ��
  */
void SafetyMonitor_ProcessBeep(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - g_safety_monitor.beep_last_toggle_time;
    
    switch(g_safety_monitor.beep_state) {
        case BEEP_STATE_PULSE_1S:
            // 1�������壺25ms�͵�ƽ��975ms�ߵ�ƽ
            if(g_safety_monitor.beep_current_level == 1 && elapsed_time >= BEEP_PULSE_DURATION) {
                // �����͵�ƽ���壬�л����ߵ�ƽ
                GPIO_SetBeepOutput(0);
                g_safety_monitor.beep_current_level = 0;
                g_safety_monitor.beep_last_toggle_time = current_time;
            } else if(g_safety_monitor.beep_current_level == 0 && 
                     elapsed_time >= (BEEP_PULSE_1S_INTERVAL - BEEP_PULSE_DURATION)) {
                // ��ʼ��һ���͵�ƽ����
                GPIO_SetBeepOutput(1);
                g_safety_monitor.beep_current_level = 1;
                g_safety_monitor.beep_last_toggle_time = current_time;
            }
            break;
            
        case BEEP_STATE_PULSE_50MS:
            // 50ms������壺25ms�͵�ƽ��25ms�ߵ�ƽ
            if(elapsed_time >= BEEP_PULSE_DURATION) {
                g_safety_monitor.beep_current_level = !g_safety_monitor.beep_current_level;
                GPIO_SetBeepOutput(g_safety_monitor.beep_current_level);
                g_safety_monitor.beep_last_toggle_time = current_time;
            }
            break;
            
        case BEEP_STATE_CONTINUOUS:
            // �����͵�ƽ�����账��
            break;
            
        case BEEP_STATE_OFF:
        default:
            // �ر�״̬�����账��
            break;
    }
}

/**
  * @brief  ǿ�����÷�����״̬
  * @param  state: ������״̬
  * @retval ��
  */
void SafetyMonitor_ForceBeepState(BeepState_t state)
{
    g_safety_monitor.beep_state = state;
    g_safety_monitor.beep_last_toggle_time = HAL_GetTick();
    g_safety_monitor.beep_current_level = 0;
    
    DEBUG_Printf("[��ȫ���] ǿ�����÷�����״̬: %s\r\n", 
                SafetyMonitor_GetBeepStateDescription(state));
}

// ================== ��Դ��� ===================

/**
  * @brief  ���õ�Դ���
  * @retval ��
  */
void SafetyMonitor_EnablePowerMonitor(void)
{
    g_safety_monitor.power_monitor_enabled = 1;
    DEBUG_Printf("[��ȫ���] ��Դ���������\r\n");
}

/**
  * @brief  ���õ�Դ���
  * @retval ��
  */
void SafetyMonitor_DisablePowerMonitor(void)
{
    g_safety_monitor.power_monitor_enabled = 0;
    DEBUG_Printf("[��ȫ���] ��Դ����ѽ���\r\n");
}

/**
  * @brief  ��Դ�쳣�жϻص���DC_CTRL�����ж�ʱ���ã�
  * @retval ��
  */
void SafetyMonitor_PowerFailureCallback(void)
{
    if(!g_safety_monitor.power_monitor_enabled) return;
    
    // ��ȡDC_CTRL��ǰ״̬
    uint8_t dc_ctrl_state = GPIO_ReadDC_CTRL();
    
    DEBUG_Printf("[��ȫ���] DC_CTRL�жϴ�������ǰ״̬: %s\r\n", dc_ctrl_state ? "�ߵ�ƽ" : "�͵�ƽ");
    
    // �����߼�������ʵ��Ӳ�����
    // ��DC24V����ʱ��DC_CTRLΪ�͵�ƽ������״̬��
    // û��DC24V����ʱ��DC_CTRLΪ�ߵ�ƽ���쳣״̬��
    if(dc_ctrl_state == 1) {
        // ��⵽��Դ�쳣���ߵ�ƽ = û��DC24V���磩
        if(!SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
            DEBUG_Printf("[��ȫ���] ��⵽�ⲿ��Դ�쳣��DC24V�����ж�\r\n");
            SafetyMonitor_SetAlarmFlag(ALARM_FLAG_O, "DC24V Loss");  // Ӣ�İ汾��10�ַ�
            
            // ��Դ�쳣ʱ���ֵ�ǰͨ��״̬����ǿ�ƹر�
            // �̵�����DC24V�ϵ�ʱ����Ȼ�Ͽ���������������
            DEBUG_Printf("[��ȫ���] ��Դ�쳣ʱ���ֵ�ǰͨ��״̬���̵�������Ȼ�Ͽ�\r\n");
            
            // �������±�������ͷ�����״̬
            SafetyMonitor_UpdateAlarmOutput();
            SafetyMonitor_UpdateBeepState();
            
            // ǿ������������������壨ȷ����������������
            SafetyMonitor_ProcessBeep();
        }
    } else {
        // ��Դ�ָ��������͵�ƽ = ��DC24V���磩
        DEBUG_Printf("[��ȫ���] �ⲿ��Դ�ָ�������׼�����O���쳣\r\n");
        // ��Դ�ָ��������ѭ����ͨ��CheckAlarmO_ClearCondition�Զ�����쳣
    }
}

// ================== �쳣��⺯�� ===================

/**
  * @brief  ���ʹ���źų�ͻ��A���쳣��
  * @retval ��
  */
void SafetyMonitor_CheckEnableConflict(void)
{
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // ����͵�ƽ�źŵ�����
    uint8_t low_count = 0;
    if(k1_en == 0) low_count++;
    if(k2_en == 0) low_count++;
    if(k3_en == 0) low_count++;
    
    // ����ж���1���ź�Ϊ�͵�ƽ�������A���쳣
    if(low_count > 1) {
        char conflict_desc[64];
        sprintf(conflict_desc, "Multi EN Active");  // �򻯰汾��14�ַ�������"A:"�ܹ�16�ַ�
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_A, conflict_desc);
    }
}

/**
  * @brief  ���̵���״̬�쳣��B~G���쳣��
  * @retval ��
  */
void SafetyMonitor_CheckRelayStatus(void)
{
    // ��ȡ�̵���״̬����
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    
    // ��ȡ�̵�������״̬���Ӽ̵�������ģ���ȡ��
    RelayState_t ch1_state = RelayControl_GetChannelState(1);
    RelayState_t ch2_state = RelayControl_GetChannelState(2);
    RelayState_t ch3_state = RelayControl_GetChannelState(3);
    
    // ���K1_1_STA�쳣��B�ࣩ
    uint8_t k1_1_expected = (ch1_state == RELAY_STATE_ON) ? 1 : 0;
    if(k1_1_sta != k1_1_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_B, NULL);
    }
    
    // ���K2_1_STA�쳣��C�ࣩ
    uint8_t k2_1_expected = (ch2_state == RELAY_STATE_ON) ? 1 : 0;
    if(k2_1_sta != k2_1_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_C, NULL);
    }
    
    // ���K3_1_STA�쳣��D�ࣩ
    uint8_t k3_1_expected = (ch3_state == RELAY_STATE_ON) ? 1 : 0;
    if(k3_1_sta != k3_1_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_D, NULL);
    }
    
    // ���K1_2_STA�쳣��E�ࣩ
    uint8_t k1_2_expected = (ch1_state == RELAY_STATE_ON) ? 1 : 0;
    if(k1_2_sta != k1_2_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_E, NULL);
    }
    
    // ���K2_2_STA�쳣��F�ࣩ
    uint8_t k2_2_expected = (ch2_state == RELAY_STATE_ON) ? 1 : 0;
    if(k2_2_sta != k2_2_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_F, NULL);
    }
    
    // ���K3_2_STA�쳣��G�ࣩ
    uint8_t k3_2_expected = (ch3_state == RELAY_STATE_ON) ? 1 : 0;
    if(k3_2_sta != k3_2_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_G, NULL);
    }
}

/**
  * @brief  ���Ӵ���״̬�쳣��H~J���쳣��
  * @retval ��
  */
void SafetyMonitor_CheckContactorStatus(void)
{
    // ��ȡ�Ӵ���״̬����
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    // ��ȡ�̵�������״̬
    RelayState_t ch1_state = RelayControl_GetChannelState(1);
    RelayState_t ch2_state = RelayControl_GetChannelState(2);
    RelayState_t ch3_state = RelayControl_GetChannelState(3);
    
    // ���SW1_STA�쳣��H�ࣩ
    uint8_t sw1_expected = (ch1_state == RELAY_STATE_ON) ? 1 : 0;
    if(sw1_sta != sw1_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_H, NULL);
    }
    
    // ���SW2_STA�쳣��I�ࣩ
    uint8_t sw2_expected = (ch2_state == RELAY_STATE_ON) ? 1 : 0;
    if(sw2_sta != sw2_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_I, NULL);
    }
    
    // ���SW3_STA�쳣��J�ࣩ
    uint8_t sw3_expected = (ch3_state == RELAY_STATE_ON) ? 1 : 0;
    if(sw3_sta != sw3_expected) {
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_J, NULL);
    }
}

/**
  * @brief  ����¶��쳣��K~M���쳣��
  * @retval ��
  */
void SafetyMonitor_CheckTemperatureAlarm(void)
{
    // ��ȡ�¶���Ϣ
    TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
    TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
    TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
    
    // ���NTC_1�¶��쳣��K�ࣩ
    if(temp1.value_celsius >= TEMP_ALARM_THRESHOLD) {
        char temp_desc[32];
        sprintf(temp_desc, "NTC1 %.1fC", temp1.value_celsius);  // Ӣ�İ汾��Լ10�ַ�
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_K, temp_desc);
    }
    
    // ���NTC_2�¶��쳣��L�ࣩ
    if(temp2.value_celsius >= TEMP_ALARM_THRESHOLD) {
        char temp_desc[32];
        sprintf(temp_desc, "NTC2 %.1fC", temp2.value_celsius);  // Ӣ�İ汾��Լ10�ַ�
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_L, temp_desc);
    }
    
    // ���NTC_3�¶��쳣��M�ࣩ
    if(temp3.value_celsius >= TEMP_ALARM_THRESHOLD) {
        char temp_desc[32];
        sprintf(temp_desc, "NTC3 %.1fC", temp3.value_celsius);  // Ӣ�İ汾��Լ10�ַ�
        SafetyMonitor_SetAlarmFlag(ALARM_FLAG_M, temp_desc);
    }
}

/**
  * @brief  ����Դ����쳣��O���쳣��
  * @retval ��
  */
void SafetyMonitor_CheckPowerMonitor(void)
{
    if(!g_safety_monitor.power_monitor_enabled) return;
    
    // ��ȡDC_CTRL״̬
    uint8_t dc_ctrl_state = GPIO_ReadDC_CTRL();
    
    // �����߼�������ʵ��Ӳ�����
    // ��DC24V����ʱ��DC_CTRLΪ�͵�ƽ������״̬��
    // û��DC24V����ʱ��DC_CTRLΪ�ߵ�ƽ���쳣״̬��
    if(dc_ctrl_state == 1) {
        // ��⵽��Դ�쳣���ߵ�ƽ = û��DC24V���磩
        if(!SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
            DEBUG_Printf("[��ȫ���] ��Դ��ؼ�⵽DC24V�쳣\r\n");
            SafetyMonitor_SetAlarmFlag(ALARM_FLAG_O, "DC24V Loss");  // Ӣ�İ汾��10�ַ�
            
            // ��Դ�쳣ʱ���ֵ�ǰͨ��״̬����ǿ�ƹر�
            // �̵�����DC24V�ϵ�ʱ����Ȼ�Ͽ���������������
            DEBUG_Printf("[��ȫ���] ��Դ�쳣ʱ���ֵ�ǰͨ��״̬���̵�������Ȼ�Ͽ�\r\n");
            
            // �������±�������ͷ�����״̬
            SafetyMonitor_UpdateAlarmOutput();
            SafetyMonitor_UpdateBeepState();
            SafetyMonitor_ProcessBeep();
        }
    }
}

// ================== �쳣������ ===================

/**
  * @brief  ���A���쳣�������
  * @retval 1:���Խ�� 0:���ܽ��
  */
uint8_t SafetyMonitor_CheckAlarmA_ClearCondition(void)
{
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // ����͵�ƽ�źŵ�����
    uint8_t low_count = 0;
    if(k1_en == 0) low_count++;
    if(k2_en == 0) low_count++;
    if(k3_en == 0) low_count++;
    
    // A�쳣���������ֻ����һ·���ڵ͵�ƽ����·�����ڸߵ�ƽ
    return (low_count <= 1) ? 1 : 0;
}

/**
  * @brief  ���B~J��N���쳣�������
  * @retval 1:���Խ�� 0:���ܽ��
  */
uint8_t SafetyMonitor_CheckAlarmBJN_ClearCondition(void)
{
    // ��ȡ�������״̬
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    // ����Ƿ���ϴ���������ֵ�������״̬֮һ
    
    // Channel_1��״̬��K1_EN=0,K2_EN=1,K3_EN=1,K1_1_STA=1,K1_2_STA=1,����STA=0,SW1_STA=1,����SW=0
    if(k1_en == 0 && k2_en == 1 && k3_en == 1 && 
       k1_1_sta == 1 && k1_2_sta == 1 && k2_1_sta == 0 && k2_2_sta == 0 && k3_1_sta == 0 && k3_2_sta == 0 &&
       sw1_sta == 1 && sw2_sta == 0 && sw3_sta == 0) {
        return 1;
    }
    
    // Channel_2��״̬��K2_EN=0,K1_EN=1,K3_EN=1,K2_1_STA=1,K2_2_STA=1,����STA=0,SW2_STA=1,����SW=0
    if(k2_en == 0 && k1_en == 1 && k3_en == 1 && 
       k2_1_sta == 1 && k2_2_sta == 1 && k1_1_sta == 0 && k1_2_sta == 0 && k3_1_sta == 0 && k3_2_sta == 0 &&
       sw2_sta == 1 && sw1_sta == 0 && sw3_sta == 0) {
        return 1;
    }
    
    // Channel_3��״̬��K3_EN=0,K1_EN=1,K2_EN=1,K3_1_STA=1,K3_2_STA=1,����STA=0,SW3_STA=1,����SW=0
    if(k3_en == 0 && k1_en == 1 && k2_en == 1 && 
       k3_1_sta == 1 && k3_2_sta == 1 && k1_1_sta == 0 && k1_2_sta == 0 && k2_1_sta == 0 && k2_2_sta == 0 &&
       sw3_sta == 1 && sw1_sta == 0 && sw2_sta == 0) {
        return 1;
    }
    
    // ȫ���ر�״̬������EN=1������STA=0
    if(k1_en == 1 && k2_en == 1 && k3_en == 1 &&
       k1_1_sta == 0 && k1_2_sta == 0 && k2_1_sta == 0 && k2_2_sta == 0 && k3_1_sta == 0 && k3_2_sta == 0 &&
       sw1_sta == 0 && sw2_sta == 0 && sw3_sta == 0) {
        return 1;
    }
    
    return 0;  // �������κ�����״̬�����ܽ���쳣
}

/**
  * @brief  ���K~M���쳣�������
  * @retval 1:���Խ�� 0:���ܽ��
  */
uint8_t SafetyMonitor_CheckAlarmKM_ClearCondition(void)
{
    // ��ȡ�¶���Ϣ
    TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
    TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
    TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
    
    // K~M�쳣����������¶Ȼؽ���58�����£����ǵ�2��ز
    uint8_t can_clear = 1;
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_K) && temp1.value_celsius >= TEMP_ALARM_CLEAR_THRESHOLD) {
        can_clear = 0;
    }
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_L) && temp2.value_celsius >= TEMP_ALARM_CLEAR_THRESHOLD) {
        can_clear = 0;
    }
    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_M) && temp3.value_celsius >= TEMP_ALARM_CLEAR_THRESHOLD) {
        can_clear = 0;
    }
    
    return can_clear;
}

/**
  * @brief  ���O���쳣�������
  * @retval 1:���Խ�� 0:���ܽ��
  */
uint8_t SafetyMonitor_CheckAlarmO_ClearCondition(void)
{
    if(!g_safety_monitor.power_monitor_enabled) return 1;
    
    // ��ȡDC_CTRL״̬
    uint8_t dc_ctrl_state = GPIO_ReadDC_CTRL();
    
    // �����߼���O���쳣���������DC_CTRL�ָ����͵�ƽ����Դ������
    // ��DC24V����ʱ��DC_CTRLΪ�͵�ƽ������״̬��
    // û��DC24V����ʱ��DC_CTRLΪ�ߵ�ƽ���쳣״̬��
    
    // ���߼���ֻҪDC_CTRLΪ�͵�ƽ�Ϳ��Խ���쳣������Ҫ��ʱȷ��
    // ��Ϊ�ж��Ѿ��ܹ���ʱ��⵽״̬�仯
    if(dc_ctrl_state == 0) {
        DEBUG_Printf("[��ȫ���] ��Դ�ָ����������O���쳣\r\n");
        return 1;
    }
    
    return 0;
}

// ================== ���Ժ�״̬��ѯ ===================

/**
  * @brief  �����ǰ��ȫ���״̬
  * @retval ��
  */
void SafetyMonitor_PrintStatus(void)
{
    DEBUG_Printf("=== ��ȫ���״̬ ===\r\n");
    DEBUG_Printf("�쳣��־λͼ: 0x%04X\r\n", g_safety_monitor.alarm_flags);
    DEBUG_Printf("ALARM�������: %s\r\n", 
                g_safety_monitor.alarm_output_active ? "�͵�ƽ��������" : "�ߵ�ƽ��������");
    DEBUG_Printf("������״̬: %s\r\n", 
                SafetyMonitor_GetBeepStateDescription(g_safety_monitor.beep_state));
    DEBUG_Printf("��Դ���: %s\r\n", 
                g_safety_monitor.power_monitor_enabled ? "����" : "����");
    
    // ���������쳣��Ϣ
    for(uint8_t i = 0; i < ALARM_FLAG_COUNT; i++) {
        if(SafetyMonitor_IsAlarmActive((AlarmFlag_t)i)) {
            DEBUG_Printf("�쳣 %c: %s (����ʱ��: %lu)\r\n", 
                        'A' + i, 
                        g_safety_monitor.alarm_info[i].description,
                        g_safety_monitor.alarm_info[i].trigger_time);
        }
    }
}

/**
  * @brief  ��ȡ�쳣�����ַ���
  * @param  flag: �쳣��־
  * @retval �쳣�����ַ���
  */
const char* SafetyMonitor_GetAlarmDescription(AlarmFlag_t flag)
{
    if(flag >= ALARM_FLAG_COUNT) return "δ֪�쳣";
    return g_alarm_descriptions[flag];
}

/**
  * @brief  ��ȡ������״̬����
  * @param  state: ������״̬
  * @retval ״̬�����ַ���
  */
const char* SafetyMonitor_GetBeepStateDescription(BeepState_t state)
{
    switch(state) {
        case BEEP_STATE_OFF:         return "�ر�";
        case BEEP_STATE_PULSE_1S:    return "1��������";
        case BEEP_STATE_PULSE_50MS:  return "50ms�������";
        case BEEP_STATE_CONTINUOUS:  return "�������";
        default:                     return "δ֪״̬";
    }
}

/**
  * @brief  ��ȫ��ص������
  * @retval ��
  */
void SafetyMonitor_DebugPrint(void)
{
    SafetyMonitor_PrintStatus();
}

// ================== ˽�к��� ===================

/**
  * @brief  ��ȡ�쳣���ͣ������쳣��־ȷ���������ͣ�
  * @param  flag: �쳣��־
  * @retval �쳣����
  */
static AlarmType_t SafetyMonitor_GetAlarmType(AlarmFlag_t flag)
{
    if(flag == ALARM_FLAG_A || flag == ALARM_FLAG_N || flag == ALARM_FLAG_O) {
        return ALARM_TYPE_AN;  // A��N��O���쳣��1��������
    } else if(flag >= ALARM_FLAG_B && flag <= ALARM_FLAG_J) {
        return ALARM_TYPE_BJ;  // B~J���쳣��50ms�������
    } else if(flag >= ALARM_FLAG_K && flag <= ALARM_FLAG_M) {
        return ALARM_TYPE_KM;  // K~M���쳣�������͵�ƽ
    }
    return ALARM_TYPE_AN;  // Ĭ������
} 

