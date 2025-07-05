#include "system_control.h"
#include "relay_control.h"
#include "temperature_monitor.h"
#include "oled_display.h"
#include "gpio_control.h"
#include "safety_monitor.h"
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
    
    // ��ʼ����ȫ���ģ��
    SafetyMonitor_Init();
    
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
    
    // ���Լ쿪ʼǰ������ִ��ͨ���ض�ȷ��
    SystemControl_ExecuteChannelShutdown();
    
    DEBUG_Printf("��ʼ�Լ�׶Σ�����ʱ��: %d��\r\n", SELF_TEST_TIME_MS/1000);
}

/**
  * @brief  ִ��ͨ���ض�ȷ�ϣ��Լ�ǰ�İ�ȫ������
  * @retval ��
  */
void SystemControl_ExecuteChannelShutdown(void)
{
    DEBUG_Printf("=== ִ��ͨ���ض�ȷ�ϣ��Լ�ǰ��ȫ��飩 ===\r\n");
    
    // ��ȡ��ǰ����ͨ����״̬����
    uint8_t k1_sta_all = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
    uint8_t k2_sta_all = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
    uint8_t k3_sta_all = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
    
    DEBUG_Printf("��ǰͨ��״̬: CH1=%s, CH2=%s, CH3=%s\r\n", 
                k1_sta_all ? "ON" : "OFF", 
                k2_sta_all ? "ON" : "OFF", 
                k3_sta_all ? "ON" : "OFF");
    
    // ����Ƿ���ͨ�����ڿ���״̬���������ǿ�ƹض�
    uint8_t need_shutdown = 0;
    
    if(k1_sta_all == 1) {
        DEBUG_Printf("ͨ��1���ڿ���״̬��ִ��ǿ�ƹض�\r\n");
        RelayControl_CloseChannel(1);
        need_shutdown = 1;
    }
    
    if(k2_sta_all == 1) {
        DEBUG_Printf("ͨ��2���ڿ���״̬��ִ��ǿ�ƹض�\r\n");
        RelayControl_CloseChannel(2);
        need_shutdown = 1;
    }
    
    if(k3_sta_all == 1) {
        DEBUG_Printf("ͨ��3���ڿ���״̬��ִ��ǿ�ƹض�\r\n");
        RelayControl_CloseChannel(3);
        need_shutdown = 1;
    }
    
    if(need_shutdown) {
        // ���ִ���˹ضϲ������ȴ�1���ü̵����������
        DEBUG_Printf("�ȴ��̵����ض϶������...\r\n");
        HAL_Delay(1000);
        
        // ���¼��״̬ȷ�Ϲضϳɹ�
        k1_sta_all = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
        k2_sta_all = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
        k3_sta_all = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
        
        DEBUG_Printf("�ضϺ�ͨ��״̬: CH1=%s, CH2=%s, CH3=%s\r\n", 
                    k1_sta_all ? "ON" : "OFF", 
                    k2_sta_all ? "ON" : "OFF", 
                    k3_sta_all ? "ON" : "OFF");
                    
        if(k1_sta_all || k2_sta_all || k3_sta_all) {
            DEBUG_Printf("����: ����ͨ���ض�ʧ�ܣ����ܴ���Ӳ������\r\n");
        } else {
            DEBUG_Printf("����ͨ����ȷ�Ϲضϣ����԰�ȫ�����Լ�\r\n");
        }
    } else {
        DEBUG_Printf("����ͨ���Ѵ��ڹض�״̬������ضϲ���\r\n");
    }
    
    DEBUG_Printf("=== ͨ���ض�ȷ����� ===\r\n");
}

/**
  * @brief  ִ���Լ��⣨�µ��Ĳ����̣�
  * @retval ��
  */
void SystemControl_ExecuteSelfTest(void)
{
    DEBUG_Printf("=== ��ʼ�°�ϵͳ�Լ��⣨�Ĳ����̣� ===\r\n");
    
    // ��ʼ���Լ���
    g_system_control.self_test.overall_result = 1;
    memset(g_system_control.self_test.error_info, 0, sizeof(g_system_control.self_test.error_info));
    g_system_control.self_test.current_step = 0;
    
    // ��һ����ʶ��ǰ����״̬������25%��
    SystemControl_UpdateSelfTestProgress(1, 25);
    g_system_control.self_test.step1_expected_state_ok = SystemControl_SelfTest_Step1_ExpectedState();
    if(!g_system_control.self_test.step1_expected_state_ok) {
        strcat(g_system_control.self_test.error_info, "����״̬ʶ���쳣;");
        g_system_control.self_test.overall_result = 0;
    }
    HAL_Delay(500); // ÿ��֮��ͣ��500ms�����û���������
    
    // �ڶ������̵���״̬���������������ȫ�ȹغ󿪣�
    SystemControl_UpdateSelfTestProgress(2, 50);
    g_system_control.self_test.step2_relay_correction_ok = SystemControl_SelfTest_Step2_RelayCorrection();
    if(!g_system_control.self_test.step2_relay_correction_ok) {
        strcat(g_system_control.self_test.error_info, "�̵��������쳣;");
        g_system_control.self_test.overall_result = 0;
    }
    HAL_Delay(500);
    
    // ���������Ӵ���״̬����뱨������75%��
    SystemControl_UpdateSelfTestProgress(3, 75);
    g_system_control.self_test.step3_contactor_check_ok = SystemControl_SelfTest_Step3_ContactorCheck();
    if(!g_system_control.self_test.step3_contactor_check_ok) {
        strcat(g_system_control.self_test.error_info, "�Ӵ�������쳣;");
        g_system_control.self_test.overall_result = 0;
    }
    HAL_Delay(500);
    
    // ���Ĳ����¶Ȱ�ȫ��⣨����100%��
    SystemControl_UpdateSelfTestProgress(4, 100);
    g_system_control.self_test.step4_temperature_safety_ok = SystemControl_SelfTest_Step4_TemperatureSafety();
    if(!g_system_control.self_test.step4_temperature_safety_ok) {
        strcat(g_system_control.self_test.error_info, "�¶Ȱ�ȫ����쳣;");
        g_system_control.self_test.overall_result = 0;
    }
    HAL_Delay(500);
    
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
    
    // ��ϸ���ԣ��������ÿ���¶�ֵ
    DEBUG_Printf("[�¶ȵ���] T1=%.1f��, T2=%.1f��, T3=%.1f��\r\n", 
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
  * @brief  ��ѭ������������������ʱ��������ȣ�
  * @retval ��
  */
void SystemControl_MainLoopScheduler(void)
{
    static uint32_t lastRelayTime = 0;
    static uint32_t lastTempTime = 0;
    static uint32_t lastOledTime = 0;
    static uint32_t lastSafetyTime = 0;
    static uint32_t lastDiagTime = 0;
    static uint32_t lastFanSpeedTime = 0;  // ��ӷ���ת��ͳ�ƶ�ʱ��
    
    uint32_t currentTime = HAL_GetTick();
    
    // ÿ10ms����̵�������������������ȼ���
    if(currentTime - lastRelayTime >= 10) {
        lastRelayTime = currentTime;
        RelayControl_ProcessPendingActions();
    }
    
    // ÿ1000ms���K_EN״̬�����Ϣ����ע�� - ���ڲ鿴�쳣��Ϣ��
    // if(currentTime - lastDiagTime >= 1000) {
    //     lastDiagTime = currentTime;
    //     SystemControl_PrintKEnDiagnostics();
    // }
    
    // ÿ1000msͳ�Ʒ���ת�٣��������ת��ʼ����ʾ0RPM���⣩
    if(currentTime - lastFanSpeedTime >= 1000) {
        lastFanSpeedTime = currentTime;
        TemperatureMonitor_FanSpeed1sHandler();
        // ע�ͷ��ȵ�����Ϣ��ר�Ĳ鿴��ȫ��ع���
        // DEBUG_Printf("[���ȵ���] ת��ͳ����ɣ���ǰRPM: %d\\r\\n", (int)TemperatureMonitor_GetFanSpeed().rpm);
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
    
    // ÿ100msִ�а�ȫ���ģ�飨�����Ӧ�ٶȣ�
    if(currentTime - lastSafetyTime >= 100) {
        lastSafetyTime = currentTime;
        SafetyMonitor_Process();
        
        // ===== �޸�BUG����ⱨ��״̬�仯���л�ϵͳ״̬ =====
        uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
        
        if(alarm_flags != 0) {
            // �б������ڣ�ȷ��ϵͳ����ALARM״̬
            if(g_system_control.current_state == SYSTEM_STATE_NORMAL) {
                DEBUG_Printf("=== ��⵽��������־:0x%04X�����л���ALARM״̬ ===\\r\\n", alarm_flags);
                SystemControl_EnterAlarmState();
            }
        } else {
            // �ޱ�����ȷ��ϵͳ����NORMAL״̬
            if(g_system_control.current_state == SYSTEM_STATE_ALARM) {
                DEBUG_Printf("=== ���б����ѽ�����л���NORMAL״̬ ===\\r\\n");
                SystemControl_EnterNormalState();
            }
        }
    }
}

/**
  * @brief  ������ʾ���ݣ���������ʾ��
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
        
        // ===== �޸�BUG����ȷ��ȡ������Ϣ =====
        if(g_system_control.current_state == SYSTEM_STATE_ALARM) {
            // ����״̬����ȡ��ǰ����ı�����Ϣ
            uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
            
            if(alarm_flags != 0) {
                // �ҵ�������ȼ��ı������Ӹߵ��ͣ�K~M > B~J > A��N��
                AlarmFlag_t active_alarm = ALARM_FLAG_COUNT;  // ��ʼ��Ϊ��Чֵ
                
                // ���ȼ�1��K~M���쳣���¶��쳣��������ȼ���
                for(AlarmFlag_t flag = ALARM_FLAG_K; flag <= ALARM_FLAG_M; flag++) {
                    if(SafetyMonitor_IsAlarmActive(flag)) {
                        active_alarm = flag;
                        break;  // �ҵ��¶��쳣������ʹ��
                    }
                }
                
                // ���ȼ�2��B~J���쳣��״̬�����쳣��
                if(active_alarm == ALARM_FLAG_COUNT) {
                    for(AlarmFlag_t flag = ALARM_FLAG_B; flag <= ALARM_FLAG_J; flag++) {
                        if(SafetyMonitor_IsAlarmActive(flag)) {
                            active_alarm = flag;
                            break;  // �ҵ�״̬�쳣������ʹ��
                        }
                    }
                }
                
                // ���ȼ�3��A��N��O���쳣��ʹ�ܳ�ͻ���Լ��쳣����Դ�쳣��
                if(active_alarm == ALARM_FLAG_COUNT) {
                    if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_A)) {
                        active_alarm = ALARM_FLAG_A;
                    } else if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_N)) {
                        active_alarm = ALARM_FLAG_N;
                    } else if(SafetyMonitor_IsAlarmActive(ALARM_FLAG_O)) {
                        active_alarm = ALARM_FLAG_O;
                    }
                }
                
                // ���ɱ�����ʾ��Ϣ
                if(active_alarm < ALARM_FLAG_COUNT) {
                    AlarmInfo_t alarm_detail = SafetyMonitor_GetAlarmInfo(active_alarm);
                    const char* alarm_desc = SafetyMonitor_GetAlarmDescription(active_alarm);
                    
                    // ��ʽ���쳣��־+���������磺"A�쳣:ʹ�ܳ�ͻ" �� "K�쳣:NTC1����"
                    char alarm_letter = 'A' + active_alarm;  // ת��Ϊ��ĸ
                    snprintf(alarm_info, sizeof(alarm_info), "%c�쳣:%s", alarm_letter, 
                            alarm_detail.description[0] ? alarm_detail.description : alarm_desc);
                } else {
                    // ���û���ҵ������쳣����ʾͨ�ñ�����Ϣ
                    strncpy(alarm_info, "ALARM ACTIVE", sizeof(alarm_info) - 1);
                }
            } else {
                // ״̬ΪALARM��û�м����쳣���������쳣�ս��������ʾ����
                strncpy(alarm_info, "System Normal", sizeof(alarm_info) - 1);
            }
        } else {
            // ����״̬����ʾϵͳ����
            strncpy(alarm_info, "System Normal", sizeof(alarm_info) - 1);
        }
        
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
        
        // ʹ�øĽ����¶���ʾ��ʽ��Temp��**.*|**.*|**.*
        sprintf(temp_info, "Temp:%.1f|%.1f|%.1f", 
               temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
        
        // ע�͵�����Ϣ��ר�Ĳ鿴��ȫ��ع���
        // DEBUG_Printf("[�¶ȵ���] T1=%.1f��, T2=%.1f��, T3=%.1f��\r\n", 
        //             temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
        // DEBUG_Printf("[��ʾ����] �¸�ʽ: \"%s\" (����:%d)\r\n", temp_info, (int)strlen(temp_info));
        
        // ��ȡ������Ϣ
        uint8_t fan_pwm = TemperatureMonitor_GetFanPWM();
        FanSpeedInfo_t fan_speed = TemperatureMonitor_GetFanSpeed();
        sprintf(fan_info, "FAN:%d%% %dRPM", fan_pwm, (int)fan_speed.rpm);
        
        // ע�ͷ��ȵ�����Ϣ��ר�Ĳ鿴��ȫ��ع���
        // DEBUG_Printf("[���ȵ���] PWMռ�ձ�: %d%%, ת��: %dRPM, �������: %d\\r\\n", 
        //             fan_pwm, (int)fan_speed.rpm, (int)fan_speed.pulse_count);
        
        // ��ʾ�����棨ʹ������ˢ�£����׽����˸���⣩
        OLED_ShowMainScreenDirty(alarm_info, status1, status2, status3, temp_info, fan_info);
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
    
    // ������ʾ���棬ȷ��״̬�л�ʱǿ��ˢ��
    OLED_ResetDisplayCache();
    OLED_ClearDirtyRegions();  // ���������¼
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
    
    // ������ʾ���棬ȷ��״̬�л�ʱǿ��ˢ��
    OLED_ResetDisplayCache();
    OLED_ClearDirtyRegions();  // ���������¼
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
    
    // ������ʾ���棬ȷ��״̬�л�ʱǿ��ˢ��
    OLED_ResetDisplayCache();
    OLED_ClearDirtyRegions();  // ���������¼
}

/**
  * @brief  �����Լ��쳣������N�쳣��־��
  * @retval ��
  */
void SystemControl_HandleSelfTestError(void)
{
    DEBUG_Printf("=== �Լ��쳣������N�쳣��־ ===\r\n");
    
    // ʹ�ð�ȫ���ģ������N�쳣��־
    SafetyMonitor_SetAlarmFlag(ALARM_FLAG_N, g_system_control.self_test.error_info);
    
    // �������״̬
    SystemControl_EnterErrorState("�Լ�ʧ��");
    
    // ������ʾ���棬ȷ���쳣��Ϣ����ȷ��ʾ
    OLED_ResetDisplayCache();
    
    // ��OLED����ʾ�Լ��쳣��Ϣ
    OLED_Clear();
    // ���������ʾ������쳣��Ϣ����ʽ�磺"N�쳣:����ԭ��"
    char display_msg[64];
    sprintf(display_msg, "N�쳣:%s", g_system_control.self_test.error_info);
    OLED_DrawString6x8(0, 3, display_msg); // ʹ��6x8������ʾ
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

/**
  * @brief  ���K_EN״̬�����Ϣ��ÿ�����һ�Σ�
  * @retval ��
  */
void SystemControl_PrintKEnDiagnostics(void)
{
    // ֻ��ϵͳ�������л򱨾�״̬����������Ϣ
    if(g_system_control.current_state != SYSTEM_STATE_NORMAL && 
       g_system_control.current_state != SYSTEM_STATE_ALARM) {
        return;
    }
    
    // ��ȡK_EN�ź�״̬
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // ��ȡSTA�����ź�״̬
    uint8_t k1_sta = GPIO_ReadK1_1_STA() && GPIO_ReadK1_2_STA() && GPIO_ReadSW1_STA();
    uint8_t k2_sta = GPIO_ReadK2_1_STA() && GPIO_ReadK2_2_STA() && GPIO_ReadSW2_STA();
    uint8_t k3_sta = GPIO_ReadK3_1_STA() && GPIO_ReadK3_2_STA() && GPIO_ReadSW3_STA();
    
    // ��������Ϣ
    DEBUG_Printf("[K_EN���] K1_EN=%d K2_EN=%d K3_EN=%d | STA: K1=%d K2=%d K3=%d\r\n", 
                k1_en, k2_en, k3_en, k1_sta, k2_sta, k3_sta);
}

// ================== �°��Լ����̺��� ===================

/**
  * @brief  �����Լ��������ʾ
  * @param  step: ��ǰ����(1-4)
  * @param  percent: ��ɰٷֱ�(0-100)
  * @retval ��
  */
void SystemControl_UpdateSelfTestProgress(uint8_t step, uint8_t percent)
{
    g_system_control.self_test.current_step = step;
    
    // ��ʾ������
    OLED_ShowSelfTestBar(percent);
    
    // ���������Ϣ
    const char* step_names[] = {
        "",
        "����״̬ʶ��",
        "�̵�������",
        "�Ӵ������", 
        "�¶Ȱ�ȫ���"
    };
    
    if(step >= 1 && step <= 4) {
        DEBUG_Printf("�Լ첽��%d��%s��%d%%��\r\n", step, step_names[step], percent);
    }
}

/**
  * @brief  ��һ����ʶ��ǰ����״̬
  * @retval 1:���� 0:�쳣
  */
uint8_t SystemControl_SelfTest_Step1_ExpectedState(void)
{
    DEBUG_Printf("=== ��һ����ʶ��ǰ����״̬ ===\r\n");
    
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    DEBUG_Printf("K_EN�ź�״̬: K1_EN=%d, K2_EN=%d, K3_EN=%d\r\n", k1_en, k2_en, k3_en);
    
    // ������ֵ��ʶ������״̬
    if(k1_en == 1 && k2_en == 1 && k3_en == 1) {
        DEBUG_Printf("ʶ������ȫ���ض�״̬\r\n");
        return 1;
    } else if(k1_en == 0 && k2_en == 1 && k3_en == 1) {
        DEBUG_Printf("ʶ������Channel_1��״̬\r\n");
        return 1;
    } else if(k1_en == 1 && k2_en == 0 && k3_en == 1) {
        DEBUG_Printf("ʶ������Channel_2��״̬\r\n");
        return 1;
    } else if(k1_en == 1 && k2_en == 1 && k3_en == 0) {
        DEBUG_Printf("ʶ������Channel_3��״̬\r\n");
        return 1;
    } else {
        DEBUG_Printf("ʶ������״̬�쳣����������ֵ��\r\n");
        return 0;
    }
}

/**
  * @brief  �ڶ������̵���״̬���������������ȫ�ȹغ󿪣�
  * @retval 1:���� 0:�쳣
  */
uint8_t SystemControl_SelfTest_Step2_RelayCorrection(void)
{
    DEBUG_Printf("=== �ڶ������̵���״̬���������������ȫ�ȹغ󿪣� ===\r\n");
    
    uint8_t result = 1;
    uint8_t correction_needed = 0;
    
    // ��ȡ����״̬
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // ��ȡ�̵�������״̬
    uint8_t k1_1_sta = GPIO_ReadK1_1_STA();
    uint8_t k1_2_sta = GPIO_ReadK1_2_STA();
    uint8_t k2_1_sta = GPIO_ReadK2_1_STA();
    uint8_t k2_2_sta = GPIO_ReadK2_2_STA();
    uint8_t k3_1_sta = GPIO_ReadK3_1_STA();
    uint8_t k3_2_sta = GPIO_ReadK3_2_STA();
    
    DEBUG_Printf("�̵���״̬: K1_1=%d K1_2=%d, K2_1=%d K2_2=%d, K3_1=%d K3_2=%d\\r\\n", 
                k1_1_sta, k1_2_sta, k2_1_sta, k2_2_sta, k3_1_sta, k3_2_sta);
    
    // ȷ������״̬
    uint8_t expected_k1_1 = 0, expected_k1_2 = 0;
    uint8_t expected_k2_1 = 0, expected_k2_2 = 0;
    uint8_t expected_k3_1 = 0, expected_k3_2 = 0;
    
    if(k1_en == 0 && k2_en == 1 && k3_en == 1) {
        // Channel_1��
        expected_k1_1 = 1; expected_k1_2 = 1;
        DEBUG_Printf("Ŀ��״̬��Channel_1��\\r\\n");
    } else if(k1_en == 1 && k2_en == 0 && k3_en == 1) {
        // Channel_2��
        expected_k2_1 = 1; expected_k2_2 = 1;
        DEBUG_Printf("Ŀ��״̬��Channel_2��\\r\\n");
    } else if(k1_en == 1 && k2_en == 1 && k3_en == 0) {
        // Channel_3��
        expected_k3_1 = 1; expected_k3_2 = 1;
        DEBUG_Printf("Ŀ��״̬��Channel_3��\\r\\n");
    } else {
        DEBUG_Printf("Ŀ��״̬��ȫ���ض�\\r\\n");
    }
    // ȫ���ض�״̬������ֵ��Ϊ0���ѳ�ʼ����
    
    // ����Ƿ���Ҫ����
    uint8_t k1_need_correction = (k1_1_sta != expected_k1_1 || k1_2_sta != expected_k1_2);
    uint8_t k2_need_correction = (k2_1_sta != expected_k2_1 || k2_2_sta != expected_k2_2);
    uint8_t k3_need_correction = (k3_1_sta != expected_k3_1 || k3_2_sta != expected_k3_2);
    
    if(k1_need_correction || k2_need_correction || k3_need_correction) {
        correction_needed = 1;
        DEBUG_Printf("��⵽�̵���״̬���󣬿�ʼ��ȫ��������\\r\\n");
        
        // ��һ�׶Σ���ȫ�ض����в���Ҫ������ͨ��
        DEBUG_Printf("[��ȫ����] ��һ�׶Σ��ض����в���Ҫ��ͨ��\\r\\n");
        
        // �ض�K1�����Ŀ�겻�ǿ���K1��
        if(expected_k1_1 == 0 && (k1_1_sta == 1 || k1_2_sta == 1)) {
            DEBUG_Printf("[��ȫ����] �ض�Channel_1�̵���\\r\\n");
            GPIO_SetK1_1_OFF(0); GPIO_SetK1_2_OFF(0);
            HAL_Delay(50);  // ���ݵ��źŽ���ʱ��
            GPIO_SetK1_1_OFF(1); GPIO_SetK1_2_OFF(1);
            HAL_Delay(500); // �ȴ��̵����������
        }
        
        // �ض�K2�����Ŀ�겻�ǿ���K2��
        if(expected_k2_1 == 0 && (k2_1_sta == 1 || k2_2_sta == 1)) {
            DEBUG_Printf("[��ȫ����] �ض�Channel_2�̵���\\r\\n");
            GPIO_SetK2_1_OFF(0); GPIO_SetK2_2_OFF(0);
            HAL_Delay(50);
            GPIO_SetK2_1_OFF(1); GPIO_SetK2_2_OFF(1);
            HAL_Delay(500);
        }
        
        // �ض�K3�����Ŀ�겻�ǿ���K3��
        if(expected_k3_1 == 0 && (k3_1_sta == 1 || k3_2_sta == 1)) {
            DEBUG_Printf("[��ȫ����] �ض�Channel_3�̵���\\r\\n");
            GPIO_SetK3_1_OFF(0); GPIO_SetK3_2_OFF(0);
            HAL_Delay(50);
            GPIO_SetK3_1_OFF(1); GPIO_SetK3_2_OFF(1);
            HAL_Delay(500);
        }
        
        // �ڶ��׶Σ�����Ŀ��ͨ��
        DEBUG_Printf("[��ȫ����] �ڶ��׶Σ�����Ŀ��ͨ��\\r\\n");
        
        // ����K1�����Ŀ���ǿ���K1��
        if(expected_k1_1 == 1) {
            DEBUG_Printf("[��ȫ����] ����Channel_1�̵���\\r\\n");
            GPIO_SetK1_1_ON(0); GPIO_SetK1_2_ON(0);
            HAL_Delay(50);
            GPIO_SetK1_1_ON(1); GPIO_SetK1_2_ON(1);
            HAL_Delay(500);
        }
        
        // ����K2�����Ŀ���ǿ���K2��
        if(expected_k2_1 == 1) {
            DEBUG_Printf("[��ȫ����] ����Channel_2�̵���\\r\\n");
            GPIO_SetK2_1_ON(0); GPIO_SetK2_2_ON(0);
            HAL_Delay(50);
            GPIO_SetK2_1_ON(1); GPIO_SetK2_2_ON(1);
            HAL_Delay(500);
        }
        
        // ����K3�����Ŀ���ǿ���K3��
        if(expected_k3_1 == 1) {
            DEBUG_Printf("[��ȫ����] ����Channel_3�̵���\\r\\n");
            GPIO_SetK3_1_ON(0); GPIO_SetK3_2_ON(0);
            HAL_Delay(50);
            GPIO_SetK3_1_ON(1); GPIO_SetK3_2_ON(1);
            HAL_Delay(500);
        }
        
        // �����׶Σ���֤������
        DEBUG_Printf("[��ȫ����] �����׶Σ���֤������\\r\\n");
        HAL_Delay(200); // ����ȴ���ȷ��״̬�ȶ�
        
        // ���¶�ȡ�̵���״̬
        k1_1_sta = GPIO_ReadK1_1_STA();
        k1_2_sta = GPIO_ReadK1_2_STA();
        k2_1_sta = GPIO_ReadK2_1_STA();
        k2_2_sta = GPIO_ReadK2_2_STA();
        k3_1_sta = GPIO_ReadK3_1_STA();
        k3_2_sta = GPIO_ReadK3_2_STA();
        
        DEBUG_Printf("�����״̬: K1_1=%d K1_2=%d, K2_1=%d K2_2=%d, K3_1=%d K3_2=%d\\r\\n", 
                    k1_1_sta, k1_2_sta, k2_1_sta, k2_2_sta, k3_1_sta, k3_2_sta);
        
        // ��֤�����Ƿ�ɹ�
        if(k1_1_sta != expected_k1_1 || k1_2_sta != expected_k1_2) {
            DEBUG_Printf("Channel_1�̵�������ʧ��\\r\\n");
            result = 0;
        }
        if(k2_1_sta != expected_k2_1 || k2_2_sta != expected_k2_2) {
            DEBUG_Printf("Channel_2�̵�������ʧ��\\r\\n");
            result = 0;
        }
        if(k3_1_sta != expected_k3_1 || k3_2_sta != expected_k3_2) {
            DEBUG_Printf("Channel_3�̵�������ʧ��\\r\\n");
            result = 0;
        }
        
        if(result) {
            DEBUG_Printf("[��ȫ����] ���м̵��������ɹ�\\r\\n");
        } else {
            DEBUG_Printf("[��ȫ����] ���̵ּ�������ʧ��\\r\\n");
        }
        
    } else {
        DEBUG_Printf("�̵���״̬�������������\\r\\n");
    }
    
    return result;
}

/**
  * @brief  ���������Ӵ���״̬����뱨��
  * @retval 1:���� 0:�쳣
  */
uint8_t SystemControl_SelfTest_Step3_ContactorCheck(void)
{
    DEBUG_Printf("=== ���������Ӵ���״̬����뱨�� ===\r\n");
    
    uint8_t result = 1;
    
    // ��ȡ����״̬
    uint8_t k1_en = GPIO_ReadK1_EN();
    uint8_t k2_en = GPIO_ReadK2_EN();
    uint8_t k3_en = GPIO_ReadK3_EN();
    
    // ��ȡ�Ӵ�������״̬
    uint8_t sw1_sta = GPIO_ReadSW1_STA();
    uint8_t sw2_sta = GPIO_ReadSW2_STA();
    uint8_t sw3_sta = GPIO_ReadSW3_STA();
    
    DEBUG_Printf("�Ӵ���״̬: SW1=%d SW2=%d SW3=%d\r\n", sw1_sta, sw2_sta, sw3_sta);
    
    // ��������״̬���Ӵ���״̬
    uint8_t expected_sw1 = 0, expected_sw2 = 0, expected_sw3 = 0;
    
    if(k1_en == 0 && k2_en == 1 && k3_en == 1) {
        // Channel_1��
        expected_sw1 = 1;
    } else if(k1_en == 1 && k2_en == 0 && k3_en == 1) {
        // Channel_2��
        expected_sw2 = 1;
    } else if(k1_en == 1 && k2_en == 1 && k3_en == 0) {
        // Channel_3��
        expected_sw3 = 1;
    }
    // ȫ���ض�״̬������ֵ��Ϊ0���ѳ�ʼ����
    
    // ���Ӵ���״̬�����������о�����
    if(sw1_sta != expected_sw1) {
        DEBUG_Printf("Channel_1�Ӵ���״̬�쳣\r\n");
        result = 0;
    }
    
    if(sw2_sta != expected_sw2) {
        DEBUG_Printf("Channel_2�Ӵ���״̬�쳣\r\n");
        result = 0;
    }
    
    if(sw3_sta != expected_sw3) {
        DEBUG_Printf("Channel_3�Ӵ���״̬�쳣\r\n");
        result = 0;
    }
    
    if(result) {
        DEBUG_Printf("�Ӵ���״̬��飺����\r\n");
    } else {
        DEBUG_Printf("�Ӵ���״̬��飺�쳣��ϵͳ�޷�����������\r\n");
    }
    
    return result;
}

/**
  * @brief  ���Ĳ����¶Ȱ�ȫ���
  * @retval 1:���� 0:�쳣
  */
uint8_t SystemControl_SelfTest_Step4_TemperatureSafety(void)
{
    DEBUG_Printf("=== ���Ĳ����¶Ȱ�ȫ��� ===\r\n");
    
    // �����¶�����
    TemperatureMonitor_UpdateAll();
    
    TempInfo_t temp1 = TemperatureMonitor_GetInfo(TEMP_CH1);
    TempInfo_t temp2 = TemperatureMonitor_GetInfo(TEMP_CH2);
    TempInfo_t temp3 = TemperatureMonitor_GetInfo(TEMP_CH3);
    
    DEBUG_Printf("�¶Ȱ�ȫ���: NTC1=%.1f��, NTC2=%.1f��, NTC3=%.1f��\r\n", 
                temp1.value_celsius, temp2.value_celsius, temp3.value_celsius);
    
    // �����·�¶��Ƿ����60�����£�ȷ���Ȱ�ȫ��
    if(temp1.value_celsius < SELF_TEST_TEMP_THRESHOLD && 
       temp2.value_celsius < SELF_TEST_TEMP_THRESHOLD && 
       temp3.value_celsius < SELF_TEST_TEMP_THRESHOLD) {
        DEBUG_Printf("�¶Ȱ�ȫ��⣺�����������¶Ⱦ���60�����£�\r\n");
        return 1;
    } else {
        DEBUG_Printf("�¶Ȱ�ȫ��⣺�쳣�������¶ȡ�60��Ĵ�������\r\n");
        
        // ��ϸ���������Ϣ
        if(temp1.value_celsius >= SELF_TEST_TEMP_THRESHOLD) {
            DEBUG_Printf("NTC1�¶ȳ��ޣ�%.1f�� >= 60��\r\n", temp1.value_celsius);
        }
        if(temp2.value_celsius >= SELF_TEST_TEMP_THRESHOLD) {
            DEBUG_Printf("NTC2�¶ȳ��ޣ�%.1f�� >= 60��\r\n", temp2.value_celsius);
        }
        if(temp3.value_celsius >= SELF_TEST_TEMP_THRESHOLD) {
            DEBUG_Printf("NTC3�¶ȳ��ޣ�%.1f�� >= 60��\r\n", temp3.value_celsius);
        }
        
        return 0;
    }
}

