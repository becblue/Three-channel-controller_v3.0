#ifndef __SYSTEM_CONTROL_H__
#define __SYSTEM_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "iwdg_control.h"

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
    SELF_TEST_STEP1_EXPECTED_STATE = 0,     // ��һ��������״̬ʶ��
    SELF_TEST_STEP2_RELAY_CORRECTION,      // �ڶ������̵���״̬�������������
    SELF_TEST_STEP3_CONTACTOR_CHECK,       // ���������Ӵ���״̬����뱨��
    SELF_TEST_STEP4_TEMPERATURE_SAFETY,    // ���Ĳ����¶Ȱ�ȫ���
    SELF_TEST_COMPLETE                     // �Լ����
} SelfTestItem_t;

// �Լ����ṹ��
typedef struct {
    uint8_t step1_expected_state_ok;        // ��һ��������״̬ʶ������1:���� 0:�쳣��
    uint8_t step2_relay_correction_ok;      // �ڶ������̵�����������1:���� 0:�쳣��
    uint8_t step3_contactor_check_ok;       // ���������Ӵ����������1:���� 0:�쳣��
    uint8_t step4_temperature_safety_ok;    // ���Ĳ����¶Ȱ�ȫ�������1:���� 0:�쳣��
    uint8_t overall_result;                 // �����Լ�����1:ͨ�� 0:ʧ�ܣ�
    char error_info[128];                   // ������Ϣ�ַ��������������ɸ�����Ϣ��
    uint8_t current_step;                   // ��ǰ�Լ첽�裨���ڽ�������ʾ��
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

// ִ��ͨ���ض�ȷ�ϣ��Լ�ǰ�İ�ȫ������
void SystemControl_ExecuteChannelShutdown(void);

// ִ���Լ��⣨�µ��Ĳ����̣�
void SystemControl_ExecuteSelfTest(void);

// ��һ����ʶ��ǰ����״̬
uint8_t SystemControl_SelfTest_Step1_ExpectedState(void);

// �ڶ������̵���״̬�������������
uint8_t SystemControl_SelfTest_Step2_RelayCorrection(void);

// ���������Ӵ���״̬����뱨��
uint8_t SystemControl_SelfTest_Step3_ContactorCheck(void);

// ���Ĳ����¶Ȱ�ȫ���
uint8_t SystemControl_SelfTest_Step4_TemperatureSafety(void);

// �����Լ��������ʾ
void SystemControl_UpdateSelfTestProgress(uint8_t step, uint8_t percent);
// �����Լ������ʾ״̬
void SystemControl_ResetSelfTestProgressState(void);

// ��������źţ�K1_EN��K2_EN��K3_ENӦΪ�ߵ�ƽ��- ��������
uint8_t SystemControl_CheckInputSignals(void);

// ��ⷴ���źţ�����״̬����ӦΪ�͵�ƽ��- ��������
uint8_t SystemControl_CheckFeedbackSignals(void);

// ����¶ȣ�����NTC�¶�Ӧ��60�����£�- ��������
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

// ���Ȱ�ȫ���ģ��
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

// K_EN״̬�����Ϣ�����ÿ�����һ�Σ�
void SystemControl_PrintKEnDiagnostics(void);

// ================== FLASH���Ժ͹����� ===================

// FLASH����д�����Ժ�����д��10000���������ݣ�
void SystemControl_FlashQuickFillTest(void);

// FLASH����д�����Ժ���������д��15MB������ѭ����¼���ܣ�
void SystemControl_FlashFillTest(void);

// FLASH��ȡ���Ժ��������Դ��ļ���ȡ���ܣ�
void SystemControl_FlashReadTest(void);

// ��ȡFLASH״̬��Ϣ
void SystemControl_PrintFlashStatus(void);

// ��ȡ��λͳ����Ϣ
void SystemControl_PrintResetStatistics(void);

// ��ȫ���FLASH��־�����������������ݣ�
void SystemControl_FlashCompleteErase(void);

// FLASH���ܻ�׼���Ժ����������Ż���Ĵ����ٶȣ�
void SystemControl_FlashSpeedBenchmark(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CONTROL_H__ */

