#include "system_control.h"
#include "relay_control.h"
#include "temperature_monitor.h"
#include "oled_display.h"
#include "gpio_control.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

/************************************************************
 * ϵͳ����ģ��Դ�ļ�
 * ʵ��ϵͳ�Լ졢��ѭ�����ȡ�״̬������
 * �ϸ���READMEҪ��2��LOGO �� 3���Լ������ �� ��� �� ��ѭ��
 ************************************************************/

// ȫ��ϵͳ���ƽṹ��
static SystemControl_t g_system_control;

/**
  * @brief  ϵͳ����ģ���ʼ��
  * @retval ��
  */
void SystemControl_Init(void)
{
    // ��ʼ��ϵͳ���ƽṹ��
    memset(&g_system_control, 0, sizeof(SystemControl_t));
    g_system_control.current_state = SYSTEM_STATE_INIT;
    g_system_control.last_state = SYSTEM_STATE_INIT;
    g_system_control.state_start_time = HAL_GetTick();
    
    DEBUG_Printf("ϵͳ����ģ���ʼ�����\r\n");
    
    // ��ʼLOGO��ʾ�׶�
    SystemControl_StartLogoDisplay();
}

/**
  * @brief  ϵͳ״̬��������ѭ���е��ã�
  * @retval ��
  */
void SystemControl_Process(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - g_system_control.state_start_time;
    
    switch(g_system_control.current_state) {
        case SYSTEM_STATE_INIT:
            // ��ʼ��״̬����������LOGO��ʾ
            SystemControl_StartLogoDisplay();
            break;
            
        case SYSTEM_STATE_LOGO_DISPLAY:
            // LOGO��ʾ�׶Σ�ά��2��
            if(elapsed_time >= LOGO_DISPLAY_TIME_MS) {
                SystemControl_StartSelfTest();
            }
            break;
            
        case SYSTEM_STATE_SELF_TEST:
            // �Լ��������ʾ�׶Σ�����3��
            if(elapsed_time < SELF_TEST_TIME_MS) {
                // ���½�����
                uint8_t percent = (elapsed_time * 100) / SELF_TEST_TIME_MS;
                OLED_ShowSelfTestBar(percent);
                DEBUG_Printf("�Լ����: %d%%\r\n", percent);
            } else {
                // �����Լ���׶�
                g_system_control.current_state = SYSTEM_STATE_SELF_TEST_CHECK;
                g_system_control.state_start_time = current_time;
                DEBUG_Printf("��ʼִ���Լ���\r\n");
            }
            break;
            
        case SYSTEM_STATE_SELF_TEST_CHECK:
            // ִ�о�����Լ���
            SystemControl_ExecuteSelfTest();
            break;
            
        case SYSTEM_STATE_NORMAL:
            // ��������״̬��ִ����ѭ������
            SystemControl_MainLoopScheduler();
            break;
            
        case SYSTEM_STATE_ERROR:
            // ����״̬����
            DEBUG_Printf("ϵͳ���ڴ���״̬: %s\r\n", g_system_control.self_test.error_info);
            // ������������Ӵ���ָ��߼�
            break;
            
        case SYSTEM_STATE_ALARM:
            // ����״̬����
            DEBUG_Printf("ϵͳ���ڱ���״̬\r\n");
            // ����ִ����ѭ���������ֱ���״̬
            SystemControl_MainLoopScheduler();
            break;
            
        default:
            // δ֪״̬������ϵͳ
            DEBUG_Printf("δ֪ϵͳ״̬������ϵͳ\r\n");
            SystemControl_Reset();
            break;
    }
}

/**
  * @brief  ��ʼLOGO��ʾ�׶�
  * @retval ��
  */
void SystemControl_StartLogoDisplay(void)
{
    g_system_control.current_state = SYSTEM_STATE_LOGO_DISPLAY;
    g_system_control.state_start_time = HAL_GetTick();
    
    // ��ʾLOGO
    OLED_ShowLogo();
    
    DEBUG_Printf("��ʼLOGO��ʾ�׶Σ�����ʱ��: %d��\r\n", LOGO_DISPLAY_TIME_MS/1000);
}

/**
  * @brief  ��ʼ�Լ�׶�
  * @retval ��
  */
void SystemControl_StartSelfTest(void)
{
    g_system_control.current_state = SYSTEM_STATE_SELF_TEST;
    g_system_control.state_start_time = HAL_GetTick();
    
    // ����׼����ʾ������
    OLED_Clear();
    
    DEBUG_Printf("��ʼ�Լ�׶Σ�����ʱ��: %d��\r\n", SELF_TEST_TIME_MS/1000);
}

/**
  * @brief  ִ���Լ���
  * @retval ��
  */
void SystemControl_ExecuteSelfTest(void)
{
    DEBUG_Printf("=== ��ʼϵͳ�Լ��� ===\r\n");
    
    // ��ʼ���Լ���
    g_system_control.self_test.overall_result = 1;
    memset(g_system_control.self_test.error_info, 0, sizeof(g_system_control.self_test.error_info));
    
    // 1. �����·�����ź�
    g_system_control.self_test.input_signals_ok = SystemControl_CheckInputSignals();
    if(!g_system_control.self_test.input_signals_ok) {
        strcat(g_system_control.self_test.error_info, "�����ź��쳣;");
        g_system_control.self_test.overall_result = 0;
    }
    
    // 2. ���״̬�����ź�
    g_system_control.self_test.feedback_signals_ok = SystemControl_CheckFeedbackSignals();
    if(!g_system_control.self_test.feedback_signals_ok) {
        strcat(g_system_control.self_test.error_info, "�����ź��쳣;");
        g_system_control.self_test.overall_result = 0;
    }
    
    // 3. ���NTC�¶�
    g_system_control.self_test.temperature_ok = SystemControl_CheckTemperature();
    if(!g_system_control.self_test.temperature_ok) {
        strcat(g_system_control.self_test.error_info, "�¶��쳣;");
        g_system_control.self_test.overall_result = 0;
    }
    
    // ����Լ���
    DEBUG_Printf("�Լ���: %s\r\n", g_system_control.self_test.overall_result ? "ͨ��" : "ʧ��");
    if(!g_system_control.self_test.overall_result) {
        DEBUG_Printf("�Լ������Ϣ: %s\r\n", g_system_control.self_test.error_info);
    }
    
    // �����Լ����л�״̬
    if(g_system_control.self_test.overall_result) {
        SystemControl_EnterNormalState();
    } else {
        SystemControl_HandleSelfTestError();
    }
}

/**
  * @brief  ��������źţ�K1_EN��K2_EN��K3_ENӦΪ�ߵ�ƽ��
  * @retval 1:���� 0:�쳣
  */
uint8_t SystemControl_CheckInputSignals(void)
{
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN(); 
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    DEBUG_Printf("�����źż��: K1_EN=%d, K2_EN=%d, K3_EN=%d\r\n", k1_en, k2_en, k3_en);
    
    // ����READMEҪ���Լ�ʱ��·�����źž�ӦΪ�ߵ�ƽ
    if(k1_en == 1 && k2_en == 1 && k3_en == 1) {
        DEBUG_Printf("�����źż��: ����\r\n");
        return 1;
    } else {
        DEBUG_Printf("�����źż��: �쳣\r\n");
        return 0;
    }
}

/**
  * @brief  ��ⷴ���źţ�����״̬����ӦΪ�͵�ƽ��
  * @retval 1:���� 0:�쳣
  */
uint8_t SystemControl_CheckFeedbackSignals(void)
{
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    DEBUG_Printf("�����źż��: K1_1=%d, K1_2=%d, K2_1=%d, K2_2=%d, K3_1=%d, K3_2=%d\r\n", 
                k1_1_sta, k1_2_sta, k2_1_sta, k2_2_sta, k3_1_sta, k3_2_sta);
    DEBUG_Printf("�Ӵ���״̬: SW1=%d, SW2=%d, SW3=%d\r\n", sw1_sta, sw2_sta, sw3_sta);
    
    // ����READMEҪ���Լ�ʱ����״̬������ӦΪ�͵�ƽ
    if(k1_1_sta == 0 && k1_2_sta == 0 && k2_1_sta == 0 && k2_2_sta == 0 && 
       k3_1_sta == 0 && k3_2_sta == 0 && sw1_sta == 0 && sw2_sta == 0 && sw3_sta == 0) {
        DEBUG_Printf("�����źż��: ����\r\n");
        return 1;
    } else {
        DEBUG_Printf("�����źż��: �쳣\r\n");
        return 0;
    }
}

/**
  * @brief  ����¶ȣ�����NTC�¶�Ӧ��60�����£�
  * @retval 1:���� 0:�쳣
  */
uint8_t SystemControl_CheckTemperature(void)
{
    // �����¶�����
    TemperatureMonitor_UpdateAll();
    
    TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
    TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
    TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
    
    DEBUG_Printf("�¶ȼ��: NTC1=%.1f��, NTC2=%.1f��, NTC3=%.1f��\r\n", 
                temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
    
    // ����READMEҪ���Լ�ʱNTC�¶Ⱦ�Ӧ��60������
    if(temp1.value_celsius < SELF_TEST_TEMP_THRESHOLD && 
       temp2.value_celsius < SELF_TEST_TEMP_THRESHOLD && 
       temp3.value_celsius < SELF_TEST_TEMP_THRESHOLD) {
        DEBUG_Printf("�¶ȼ��: ����\r\n");
        return 1;
    } else {
        DEBUG_Printf("�¶ȼ��: �쳣\r\n");
        return 0;
    }
}

/**
  * @brief  ��ѭ��������������������״̬���ã�
  * @retval ��
  */
void SystemControl_MainLoopScheduler(void)
{
    static uint32_t lastRelayTime = 0;
    static uint32_t lastTempTime = 0;
    static uint32_t lastOledTime = 0;
    static uint32_t lastSafetyTime = 0;
    
    uint32_t currentTime = HAL_GetTick();
    
    // ÿ10ms����̵�������������������ȼ���
    if(currentTime - lastRelayTime >= 10) {
        lastRelayTime = currentTime;
        RelayControl_ProcessPendingActions();
    }
    
    // ÿ100msִ���¶ȼ��
    if(currentTime - lastTempTime >= 100) {
        lastTempTime = currentTime;
        TemperatureMonitor_UpdateAll();
        TemperatureMonitor_CheckAndHandleAlarm();
    }
    
    // ÿ200ms����OLED��ʾ
    if(currentTime - lastOledTime >= 200) {
        lastOledTime = currentTime;
        SystemControl_UpdateDisplay();
    }
    
    // ÿ500msִ�а�ȫ���ģ�飨��3.2�׶�ʵ�֣�
    if(currentTime - lastSafetyTime >= 500) {
        lastSafetyTime = currentTime;
        // TODO: 3.2�׶� - SafetyMonitor_Process();
    }
}

/**
  * @brief  ������ʾ���ݣ�����ѭ���������е��ã�
  * @retval ��
  */
void SystemControl_UpdateDisplay(void)
{
    // ������״̬�¸�����������ʾ
    if(g_system_control.current_state == SYSTEM_STATE_NORMAL || 
       g_system_control.current_state == SYSTEM_STATE_ALARM) {
        
        // ׼����ʾ����
        char alarm_info[32] = "";
        char status1[16], status2[16], status3[16];
        char temp_info[32], fan_info[16];
        
        // ��ȡͨ��״̬
        RelayState_t ch1_state = RelayControl_GetChannelState(1);
        RelayState_t ch2_state = RelayControl_GetChannelState(2);
        RelayState_t ch3_state = RelayControl_GetChannelState(3);
        
        sprintf(status1, "CH1:%s", (ch1_state == RELAY_STATE_ON) ? "ON" : "OFF");
        sprintf(status2, "CH2:%s", (ch2_state == RELAY_STATE_ON) ? "ON" : "OFF");
        sprintf(status3, "CH3:%s", (ch3_state == RELAY_STATE_ON) ? "ON" : "OFF");
        
        // ��ȡ�¶���Ϣ
        TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
        TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
        TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
        
        sprintf(temp_info, "T1:%.1f T2:%.1f T3:%.1f", 
               temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
        
        // ��ȡ������Ϣ
        uint8_t fan_pwm = TemperatureMonitor_GetFanPWM();
        FanSpeedInfo_t fan_speed = TemperatureMonitor_GetFanSpeed();
        sprintf(fan_info, "FAN:%d%% %dRPM", fan_pwm, (int)fan_speed.rpm);
        
        // ��ʾ������
        OLED_ShowMainScreen(alarm_info, status1, status2, status3, temp_info, fan_info);
    }
}

/**
  * @brief  ������������״̬
  * @retval ��
  */
void SystemControl_EnterNormalState(void)
{
    g_system_control.current_state = SYSTEM_STATE_NORMAL;
    g_system_control.state_start_time = HAL_GetTick();
    
    DEBUG_Printf("=== ϵͳ������������״̬ ===\r\n");
    
    // ����׼����ʾ������
    OLED_Clear();
}

/**
  * @brief  �������״̬
  * @param  error_msg: ������Ϣ
  * @retval ��
  */
void SystemControl_EnterErrorState(const char* error_msg)
{
    g_system_control.current_state = SYSTEM_STATE_ERROR;
    g_system_control.state_start_time = HAL_GetTick();
    
    if(error_msg) {
        strncpy(g_system_control.self_test.error_info, error_msg, 
                sizeof(g_system_control.self_test.error_info) - 1);
    }
    
    DEBUG_Printf("=== ϵͳ�������״̬: %s ===\r\n", error_msg ? error_msg : "δ֪����");
}

/**
  * @brief  ���뱨��״̬
  * @retval ��
  */
void SystemControl_EnterAlarmState(void)
{
    g_system_control.current_state = SYSTEM_STATE_ALARM;
    g_system_control.state_start_time = HAL_GetTick();
    
    DEBUG_Printf("=== ϵͳ���뱨��״̬ ===\r\n");
}

/**
  * @brief  �����Լ��쳣������N�쳣��־��
  * @retval ��
  */
void SystemControl_HandleSelfTestError(void)
{
    DEBUG_Printf("=== �Լ��쳣������N�쳣��־ ===\r\n");
    
    // ����N�쳣��־
    g_system_control.error_flags |= (1 << 13); // N��Ӧ��13λ
    
    // �������״̬
    SystemControl_EnterErrorState("�Լ�ʧ��");
    
    // ��OLED����ʾ�Լ��쳣��Ϣ
    OLED_Clear();
    // ���������ʾ������쳣��Ϣ����ʽ�磺"N�쳣:����ԭ��"
    char display_msg[64];
    sprintf(display_msg, "N�쳣:%s", g_system_control.self_test.error_info);
    OLED_DrawString(0, 20, display_msg, 8); // ʹ��8x16������ʾ
    OLED_Refresh();
}

/**
  * @brief  ��ȡ��ǰϵͳ״̬
  * @retval ϵͳ״̬
  */
SystemState_t SystemControl_GetState(void)
{
    return g_system_control.current_state;
}

/**
  * @brief  ǿ���л�ϵͳ״̬
  * @param  new_state: ��״̬
  * @retval ��
  */
void SystemControl_SetState(SystemState_t new_state)
{
    g_system_control.last_state = g_system_control.current_state;
    g_system_control.current_state = new_state;
    g_system_control.state_start_time = HAL_GetTick();
    
    DEBUG_Printf("ϵͳ״̬�л�: %d -> %d\r\n", g_system_control.last_state, new_state);
}

/**
  * @brief  ��ȡ�Լ���
  * @retval �Լ����ṹ��
  */
SelfTestResult_t SystemControl_GetSelfTestResult(void)
{
    return g_system_control.self_test;
}

/**
  * @brief  ϵͳ��λ
  * @retval ��
  */
void SystemControl_Reset(void)
{
    DEBUG_Printf("ϵͳ��λ\r\n");
    
    // ����ϵͳ���ƽṹ��
    memset(&g_system_control, 0, sizeof(SystemControl_t));
    g_system_control.current_state = SYSTEM_STATE_INIT;
    g_system_control.state_start_time = HAL_GetTick();
    
    // ���¿�ʼLOGO��ʾ
    SystemControl_StartLogoDisplay();
}

/**
  * @brief  ϵͳ״̬�������
  * @retval ��
  */
void SystemControl_DebugPrint(void)
{
    DEBUG_Printf("=== ϵͳ״̬������Ϣ ===\r\n");
    DEBUG_Printf("��ǰ״̬: %d\r\n", g_system_control.current_state);
    DEBUG_Printf("״̬����ʱ��: %lu ms\r\n", HAL_GetTick() - g_system_control.state_start_time);
    DEBUG_Printf("�Լ���: %s\r\n", g_system_control.self_test.overall_result ? "ͨ��" : "ʧ��");
    if(!g_system_control.self_test.overall_result) {
        DEBUG_Printf("������Ϣ: %s\r\n", g_system_control.self_test.error_info);
    }
    DEBUG_Printf("�����־: 0x%02X\r\n", g_system_control.error_flags);
}

