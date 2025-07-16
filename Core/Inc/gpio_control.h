#ifndef __GPIO_CONTROL_H__
#define __GPIO_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * GPIO����ģ��ͷ�ļ�
 * 1. �����źŶ�ȡ�ӿڣ���K1_EN��K2_EN��K3_EN��״̬�����������ȣ�
 * 2. ����źſ��ƽӿڣ���̵�����������������������PWM�ȣ�
 * 3. �жϻص���ȥ�������������������������ȵ���ؽӿ�
 ************************************************************/

//---------------- �����źŶ�ȡ ----------------//
// ʹ���źŶ�ȡ
uint8_t GPIO_ReadK1_EN(void);   // ��ȡK1_EN�����ź�
uint8_t GPIO_ReadK2_EN(void);   // ��ȡK2_EN�����ź�
uint8_t GPIO_ReadK3_EN(void);   // ��ȡK3_EN�����ź�

// ״̬�����źŶ�ȡ
uint8_t GPIO_ReadK1_1_STA(void); // ��ȡK1_1״̬����
uint8_t GPIO_ReadK1_2_STA(void); // ��ȡK1_2״̬����
uint8_t GPIO_ReadK2_1_STA(void); // ��ȡK2_1״̬����
uint8_t GPIO_ReadK2_2_STA(void); // ��ȡK2_2״̬����
uint8_t GPIO_ReadK3_1_STA(void); // ��ȡK3_1״̬����
uint8_t GPIO_ReadK3_2_STA(void); // ��ȡK3_2״̬����
uint8_t GPIO_ReadSW1_STA(void);  // ��ȡSW1״̬����
uint8_t GPIO_ReadSW2_STA(void);  // ��ȡSW2״̬����
uint8_t GPIO_ReadSW3_STA(void);  // ��ȡSW3״̬����

// ��Դ����źŶ�ȡ
uint8_t GPIO_ReadDC_CTRL(void);  // ��ȡ�ⲿ��Դ����ź�

// ����ת���źŶ�ȡ
uint8_t GPIO_ReadFAN_SEN(void);  // ��ȡ����ת���ź�

//---------------- ����źſ��� ----------------//
// �̵��������ź�
void GPIO_SetK1_1_ON(uint8_t state);   // ����K1_1_ON���
void GPIO_SetK1_1_OFF(uint8_t state);  // ����K1_1_OFF���
void GPIO_SetK1_2_ON(uint8_t state);   // ����K1_2_ON���
void GPIO_SetK1_2_OFF(uint8_t state);  // ����K1_2_OFF���
void GPIO_SetK2_1_ON(uint8_t state);   // ����K2_1_ON���
void GPIO_SetK2_1_OFF(uint8_t state);  // ����K2_1_OFF���
void GPIO_SetK2_2_ON(uint8_t state);   // ����K2_2_ON���
void GPIO_SetK2_2_OFF(uint8_t state);  // ����K2_2_OFF���
void GPIO_SetK3_1_ON(uint8_t state);   // ����K3_1_ON���
void GPIO_SetK3_1_OFF(uint8_t state);  // ����K3_1_OFF���
void GPIO_SetK3_2_ON(uint8_t state);   // ����K3_2_ON���
void GPIO_SetK3_2_OFF(uint8_t state);  // ����K3_2_OFF���

// �����������
void GPIO_SetAlarmOutput(uint8_t state);     // ����ALARM���������1:�͵�ƽ 0:�ߵ�ƽ��

// ����������
void GPIO_SetBeepOutput(uint8_t state);      // ���Ʒ����������1:�͵�ƽ 0:�ߵ�ƽ��

// ����PWM���ƣ�����ֱ�ӿ���PWM���ţ�
void GPIO_SetFAN_PWM(uint8_t state);   // ���Ʒ���PWM�����������Ҫ��

// RS485������ƣ��������չ��
void GPIO_SetRS485_EN(uint8_t state);  // ����RS485����ʹ��

//---------------- �ж���ȥ���� ----------------//
// ʹ���ź��жϻص�����EXTI�жϷ������е��ã�
void GPIO_K1_EN_Callback(uint16_t GPIO_Pin);
void GPIO_K2_EN_Callback(uint16_t GPIO_Pin);
void GPIO_K3_EN_Callback(uint16_t GPIO_Pin);
void GPIO_DC_CTRL_Callback(uint16_t GPIO_Pin);

// ȥ����������أ�������Ҫ����չ��

// ================== GPIO��ѯϵͳ������жϷ�ʽ�� ===================

// ��ѯ״̬�ṹ��
typedef struct {
    uint8_t current_state;      // ��ǰ����״̬
    uint8_t last_state;         // �ϴ�����״̬  
    uint8_t stable_state;       // �ȶ�״̬���������״̬��
    uint32_t state_change_time; // ״̬�仯ʱ��
    uint32_t last_poll_time;    // �ϴ���ѯʱ��
    uint8_t debounce_counter;   // ����������
} GPIO_PollState_t;

// �������ò���
#define GPIO_DEBOUNCE_TIME_MS       50   // ����ʱ��50ms
#define GPIO_DEBOUNCE_SAMPLES       5    // ������������
#define GPIO_POLL_INTERVAL_MS       10   // ��ѯ���10ms

// GPIO��ѯϵͳ��������
void GPIO_InitPollingSystem(void);
void GPIO_ProcessPolling(void);
void GPIO_DisableInterrupts(void);
uint8_t GPIO_GetStableState(uint8_t pin_index);

// ��ѯϵͳ��Ϻ���
void GPIO_PrintPollingStats(void);
uint8_t GPIO_IsPollingEnabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_CONTROL_H__ */ 

