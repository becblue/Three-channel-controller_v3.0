#ifndef __SAFETY_MONITOR_H__
#define __SAFETY_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * ��ȫ���ģ��ͷ�ļ�
 * 1. �쳣��־��A~N��KLM�ȣ���������
 * 2. ���������ALARM���š��������������巽ʽ��
 * 3. ��Դ��أ�DC_CTRL�쳣��⣩
 * 4. �쳣���ȼ��ͽ������
 * 
 * �ϸ���READMEҪ��
 * - A��N���쳣��������1��������
 * - B~J���쳣��������50ms�������
 * - K~M���쳣�������������͵�ƽ
 * - ALARM���ţ������쳣����������͵�ƽ
 ************************************************************/

// �쳣��־ö�٣�����README���壩
typedef enum {
    ALARM_FLAG_A = 0,       // K1_EN��K2_EN��K3_ENʹ�ܳ�ͻ
    ALARM_FLAG_B,           // K1_1_STA�����쳣
    ALARM_FLAG_C,           // K2_1_STA�����쳣
    ALARM_FLAG_D,           // K3_1_STA�����쳣
    ALARM_FLAG_E,           // K1_2_STA�����쳣
    ALARM_FLAG_F,           // K2_2_STA�����쳣
    ALARM_FLAG_G,           // K3_2_STA�����쳣
    ALARM_FLAG_H,           // SW1_STA�����쳣
    ALARM_FLAG_I,           // SW2_STA�����쳣
    ALARM_FLAG_J,           // SW3_STA�����쳣
    ALARM_FLAG_K,           // NTC_1�¶��쳣
    ALARM_FLAG_L,           // NTC_2�¶��쳣
    ALARM_FLAG_M,           // NTC_3�¶��쳣
    ALARM_FLAG_N,           // �Լ��쳣
    ALARM_FLAG_COUNT        // �쳣��־����
} AlarmFlag_t;

// �쳣����ö��
typedef enum {
    ALARM_TYPE_AN = 0,      // A��N���쳣��1��������
    ALARM_TYPE_BJ,          // B~J���쳣��50ms�������
    ALARM_TYPE_KM           // K~M���쳣�������͵�ƽ
} AlarmType_t;

// ������״̬ö��
typedef enum {
    BEEP_STATE_OFF = 0,     // �������ر�
    BEEP_STATE_PULSE_1S,    // 1�������壨A��N���쳣��
    BEEP_STATE_PULSE_50MS,  // 50ms������壨B~J���쳣��
    BEEP_STATE_CONTINUOUS   // ���������K~M���쳣��
} BeepState_t;

// �쳣��Ϣ�ṹ��
typedef struct {
    uint8_t is_active;      // �쳣�Ƿ񼤻1:���� 0:�ѽ����
    AlarmType_t type;       // �쳣����
    uint32_t trigger_time;  // �쳣����ʱ��
    char description[32];   // �쳣������Ϣ
} AlarmInfo_t;

// ��ȫ��ؽṹ��
typedef struct {
    uint16_t alarm_flags;               // �쳣��־λ��λͼ��֧��16���쳣��
    AlarmInfo_t alarm_info[ALARM_FLAG_COUNT];  // ��ϸ�쳣��Ϣ
    uint8_t alarm_output_active;        // ALARM�������״̬��1:�͵�ƽ��� 0:�ߵ�ƽ��
    BeepState_t beep_state;             // ��ǰ������״̬
    uint32_t beep_last_toggle_time;     // �������ϴ��л�ʱ��
    uint8_t beep_current_level;         // ��������ǰ��ƽ״̬
    uint8_t power_monitor_enabled;      // ��Դ���ʹ�ܱ�־
} SafetyMonitor_t;

// ʱ�䳣������
#define BEEP_PULSE_1S_INTERVAL      1000    // 1�����������ڣ�ms��
#define BEEP_PULSE_50MS_INTERVAL    50      // 50ms����������ڣ�ms��
#define BEEP_PULSE_DURATION         25      // �������ʱ�䣨ms��

// �¶��쳣��ֵ����temperature_monitorģ�鱣��һ�£�
#define TEMP_ALARM_THRESHOLD        60.0f   // �¶ȱ�����ֵ��60�棩
#define TEMP_ALARM_CLEAR_THRESHOLD  58.0f   // �¶ȱ��������ֵ��58�棬2��ز

// ================== ���Ĺ����� ===================

// ��ȫ���ģ���ʼ��
void SafetyMonitor_Init(void);

// ��ȫ���������������ѭ���е��ã�
void SafetyMonitor_Process(void);

// ��ȡ��ǰ�쳣״̬�������쳣��־λͼ��
uint16_t SafetyMonitor_GetAlarmFlags(void);

// ���ָ���쳣�Ƿ񼤻�
uint8_t SafetyMonitor_IsAlarmActive(AlarmFlag_t flag);

// ��ȡ�쳣��Ϣ
AlarmInfo_t SafetyMonitor_GetAlarmInfo(AlarmFlag_t flag);

// ================== �쳣��־���� ===================

// �����쳣��־
void SafetyMonitor_SetAlarmFlag(AlarmFlag_t flag, const char* description);

// ����쳣��־
void SafetyMonitor_ClearAlarmFlag(AlarmFlag_t flag);

// ��������쳣��־
void SafetyMonitor_ClearAllAlarmFlags(void);

// ��鲢���������쳣״̬
void SafetyMonitor_UpdateAllAlarmStatus(void);

// ================== ����������� ===================

// ����ALARM������������κ��쳣ʱ����͵�ƽ��
void SafetyMonitor_UpdateAlarmOutput(void);

// ǿ������ALARM����״̬
void SafetyMonitor_ForceAlarmOutput(uint8_t active);

// ================== ���������� ===================

// ���·�����״̬�������쳣���;������巽ʽ��
void SafetyMonitor_UpdateBeepState(void);

// ���������崦����ʱ���ã�
void SafetyMonitor_ProcessBeep(void);

// ǿ�����÷�����״̬
void SafetyMonitor_ForceBeepState(BeepState_t state);

// ================== ��Դ��� ===================

// ���õ�Դ���
void SafetyMonitor_EnablePowerMonitor(void);

// ���õ�Դ���
void SafetyMonitor_DisablePowerMonitor(void);

// ��Դ�쳣�жϻص���DC_CTRL�����ж�ʱ���ã�
void SafetyMonitor_PowerFailureCallback(void);

// ================== �쳣��⺯�� ===================

// ���ʹ���źų�ͻ��A���쳣��
void SafetyMonitor_CheckEnableConflict(void);

// ���̵���״̬�쳣��B~G���쳣��
void SafetyMonitor_CheckRelayStatus(void);

// ���Ӵ���״̬�쳣��H~J���쳣��
void SafetyMonitor_CheckContactorStatus(void);

// ����¶��쳣��K~M���쳣��
void SafetyMonitor_CheckTemperatureAlarm(void);

// ================== �쳣������ ===================

// ���A���쳣�������
uint8_t SafetyMonitor_CheckAlarmA_ClearCondition(void);

// ���B~J��N���쳣�������  
uint8_t SafetyMonitor_CheckAlarmBJN_ClearCondition(void);

// ���K~M���쳣�������
uint8_t SafetyMonitor_CheckAlarmKM_ClearCondition(void);

// ================== ���Ժ�״̬��ѯ ===================

// �����ǰ��ȫ���״̬
void SafetyMonitor_PrintStatus(void);

// ��ȡ�쳣�����ַ���
const char* SafetyMonitor_GetAlarmDescription(AlarmFlag_t flag);

// ��ȡ������״̬����
const char* SafetyMonitor_GetBeepStateDescription(BeepState_t state);

// ��ȫ��ص������
void SafetyMonitor_DebugPrint(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAFETY_MONITOR_H__ */ 

