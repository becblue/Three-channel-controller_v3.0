#ifndef __SYSTEM_CONTROL_H__
#define __SYSTEM_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * ϵͳ����ģ��ͷ�ļ�
 * 1. ϵͳ״̬�������Լ졢�������쳣�������ȣ�
 * 2. �ϵ��Լ����̣�2��LOGO��3�������������/����/�¶ȼ�⣩
 * 3. ��ѭ�������������ȸ�����ģ�飩
 * 4. ϵͳ״̬�л����쳣����
 ************************************************************/

// ϵͳ״̬ö��
typedef enum {
    SYSTEM_STATE_INIT = 0,          // ϵͳ��ʼ��״̬
    SYSTEM_STATE_LOGO_DISPLAY,      // LOGO��ʾ״̬��2�룩
    SYSTEM_STATE_SELF_TEST,         // �Լ�״̬��3���������
    SYSTEM_STATE_SELF_TEST_CHECK,   // �Լ���׶�
    SYSTEM_STATE_NORMAL,            // ��������״̬
    SYSTEM_STATE_ERROR,             // ϵͳ����״̬
    SYSTEM_STATE_ALARM              // ����״̬
} SystemState_t;

// �Լ���Ŀö��
typedef enum {
    SELF_TEST_INPUT_SIGNALS = 0,    // �����·�����ź�K1_EN��K2_EN��K3_EN
    SELF_TEST_FEEDBACK_SIGNALS,     // ���״̬�����ź�
    SELF_TEST_TEMPERATURE,          // ���NTC�¶�
    SELF_TEST_COMPLETE              // �Լ����
} SelfTestItem_t;

// �Լ����ṹ��
typedef struct {
    uint8_t input_signals_ok;       // �����źż������1:���� 0:�쳣��
    uint8_t feedback_signals_ok;    // �����źż������1:���� 0:�쳣��
    uint8_t temperature_ok;         // �¶ȼ������1:���� 0:�쳣��
    uint8_t overall_result;         // �����Լ�����1:ͨ�� 0:ʧ�ܣ�
    char error_info[64];            // ������Ϣ�ַ���
} SelfTestResult_t;

// ϵͳ���ƽṹ��
typedef struct {
    SystemState_t current_state;    // ��ǰϵͳ״̬
    SystemState_t last_state;       // ��һ��ϵͳ״̬
    uint32_t state_start_time;      // ��ǰ״̬��ʼʱ��
    SelfTestResult_t self_test;     // �Լ���
    uint8_t error_flags;            // ϵͳ�����־λ
} SystemControl_t;

// ʱ�䳣������
#define LOGO_DISPLAY_TIME_MS        2000    // LOGO��ʾʱ�䣨2�룩
#define SELF_TEST_TIME_MS           3000    // �Լ�ʱ�䣨3�룩
#define MAIN_LOOP_INTERVAL_MS       10      // ��ѭ�������10ms��

// �Լ���ֵ����
#define SELF_TEST_TEMP_THRESHOLD    60.0f   // �Լ��¶���ֵ��60�棩

// ================== ϵͳ���ƽӿں��� ===================

// ϵͳ����ģ���ʼ��
void SystemControl_Init(void);

// ϵͳ״̬��������ѭ���е��ã�
void SystemControl_Process(void);

// ��ȡ��ǰϵͳ״̬
SystemState_t SystemControl_GetState(void);

// ǿ���л�ϵͳ״̬
void SystemControl_SetState(SystemState_t new_state);

// ��ȡ�Լ���
SelfTestResult_t SystemControl_GetSelfTestResult(void);

// ϵͳ��λ
void SystemControl_Reset(void);

// ================== �Լ����̺��� ===================

// ��ʼLOGO��ʾ�׶�
void SystemControl_StartLogoDisplay(void);

// ��ʼ�Լ�׶�
void SystemControl_StartSelfTest(void);

// ִ���Լ���
void SystemControl_ExecuteSelfTest(void);

// ��������źţ�K1_EN��K2_EN��K3_ENӦΪ�ߵ�ƽ��
uint8_t SystemControl_CheckInputSignals(void);

// ��ⷴ���źţ�����״̬����ӦΪ�͵�ƽ��
uint8_t SystemControl_CheckFeedbackSignals(void);

// ����¶ȣ�����NTC�¶�Ӧ��60�����£�
uint8_t SystemControl_CheckTemperature(void);

// ================== ��ѭ�����Ⱥ��� ===================

// ��ѭ�������������ȸ�����ģ�飩
void SystemControl_MainLoop(void);

// ���ȼ̵�������ģ��
void SystemControl_ScheduleRelayControl(void);

// �����¶ȼ��ģ��
void SystemControl_ScheduleTemperatureMonitor(void);

// ����OLED��ʾģ��
void SystemControl_ScheduleOLEDDisplay(void);

// ���Ȱ�ȫ���ģ�飨Ԥ����
void SystemControl_ScheduleSafetyMonitor(void);

// ================== ״̬�л����쳣���� ===================

// ������������״̬
void SystemControl_EnterNormalState(void);

// �������״̬
void SystemControl_EnterErrorState(const char* error_msg);

// ���뱨��״̬
void SystemControl_EnterAlarmState(void);

// �����Լ��쳣������N�쳣��־��
void SystemControl_HandleSelfTestError(void);

// ϵͳ״̬�������
void SystemControl_DebugPrint(void);

// ========== �������� ==========
void SystemControl_MainLoopScheduler(void);
void SystemControl_UpdateDisplay(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CONTROL_H__ */

