#ifndef __TEMPERATURE_MONITOR_H__
#define __TEMPERATURE_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * �¶ȼ��ģ��ͷ�ļ�
 * 1. NTC�¶Ȳɼ������¶�ת��
 * 2. ����PWM��������
 * 3. �¶��쳣������ز��
 * 4. �¶�״̬��ѯ��������
 ************************************************************/

// �¶�ͨ��ö��
typedef enum {
    TEMP_CH1 = 0,   // ͨ��1��NTC_1��
    TEMP_CH2,       // ͨ��2��NTC_2��
    TEMP_CH3,       // ͨ��3��NTC_3��
    TEMP_CH_MAX
} TempChannel_t;

// �¶�״̬ö��
typedef enum {
    TEMP_STATE_NORMAL = 0,   // �����¶�
    TEMP_STATE_HIGH,         // ����
    TEMP_STATE_OVERHEAT      // �����쳣
} TempState_t;

// �¶ȼ�ؽṹ��
typedef struct {
    float value_celsius;     // ��ǰ�¶ȣ����϶ȣ�
    TempState_t state;       // ��ǰ�¶�״̬
} TempInfo_t;

// ����ת����Ϣ�ṹ��
typedef struct {
    uint32_t rpm;         // ��ǰ����ת��(RPM)
    uint32_t pulse_count; // 1���ڼ�⵽��������
} FanSpeedInfo_t;

// ================== ADC DMA������� ===================
extern uint16_t adc_dma_buf[TEMP_CH_MAX]; // DMA����������

// �¶ȼ��ģ���ʼ��
void TemperatureMonitor_Init(void);

// �ɼ�����NTC�¶Ȳ�����״̬�����鶨ʱ���ã�
void TemperatureMonitor_UpdateAll(void);

// ��ȡָ��ͨ�����¶���Ϣ
TempInfo_t TemperatureMonitor_GetInfo(TempChannel_t channel);

// ��ȡ����ͨ�����¶�״̬�����ڱ�����������
void TemperatureMonitor_GetAllStates(TempState_t states[TEMP_CH_MAX]);

// ���÷���PWMռ�ձȣ�0~100�����ڲ��Զ������¶�����
void TemperatureMonitor_SetFanPWM(uint8_t duty);

// ��ѯ���ȵ�ǰPWMռ�ձ�
uint8_t TemperatureMonitor_GetFanPWM(void);

// ��鲢�����¶��쳣����KLM��־���ز�ȣ�
void TemperatureMonitor_CheckAndHandleAlarm(void);

// ���ڵ��������ǰ�¶Ⱥͷ���״̬
void TemperatureMonitor_DebugPrint(void);

// ��ȡ��ǰ����ת����Ϣ
FanSpeedInfo_t TemperatureMonitor_GetFanSpeed(void);

// 1�붨ʱ���ص���ÿ�����һ�Σ�ͳ�Ʒ���ת�٣�
void TemperatureMonitor_FanSpeed1sHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEMPERATURE_MONITOR_H__ */ 

