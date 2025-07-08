#ifndef __RELAY_CONTROL_H__
#define __RELAY_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * �̵�������ģ��ͷ�ļ�
 * 1. ͨ�����ƣ�����/�رգ�
 * 2. ������⡢���ؼ�⡢״̬�������쳣����
 * 3. ͨ��״̬�������롢���ƺ�������
 ************************************************************/

// �̵���ͨ��״̬ö��
typedef enum {
    RELAY_STATE_OFF = 0,    // �̵����ر�״̬
    RELAY_STATE_ON,         // �̵�����״̬
    RELAY_STATE_ERROR       // �̵�������״̬
} RelayState_t;

// �̵���ͨ���ṹ��
typedef struct {
    uint8_t channelNum;         // ͨ����(1-3)
    RelayState_t state;         // ��ǰ״̬
    uint32_t lastActionTime;    // ���һ�ζ���ʱ��
    uint8_t errorCode;          // �������
} RelayChannel_t;

// ��ʼ������
void RelayControl_Init(void);

// ͨ�����ƺ���
uint8_t RelayControl_OpenChannel(uint8_t channelNum);   // ����ָ��ͨ��
uint8_t RelayControl_CloseChannel(uint8_t channelNum);  // �ر�ָ��ͨ��

// ״̬��⺯��
RelayState_t RelayControl_GetChannelState(uint8_t channelNum); // ��ȡͨ��״̬
uint8_t RelayControl_CheckChannelFeedback(uint8_t channelNum); // ���ͨ������

// ������麯��
uint8_t RelayControl_CheckInterlock(uint8_t channelNum); // ���ͨ������

// ��������
uint8_t RelayControl_GetLastError(uint8_t channelNum);   // ��ȡ���һ�δ���
void RelayControl_ClearError(uint8_t channelNum);        // �������״̬

// ʹ���źŴ����������ж��е��ã�
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state);

// ������붨��
#define RELAY_ERR_NONE              0x00    // �޴���
#define RELAY_ERR_INTERLOCK         0x01    // ��������
#define RELAY_ERR_FEEDBACK          0x02    // ��������
#define RELAY_ERR_TIMEOUT           0x03    // ��ʱ����
#define RELAY_ERR_INVALID_CHANNEL   0x04    // ��Чͨ��
#define RELAY_ERR_INVALID_STATE     0x05    // ��Ч״̬
#define RELAY_ERR_POWER_FAILURE     0x06    // ��Դ�쳣����
#define RELAY_ERR_HARDWARE_FAILURE  0x07    // Ӳ�������쳣����

// ʱ�䳣������
#define RELAY_CHECK_INTERVAL        50      // ʹ���źż����(ms)
#define RELAY_PULSE_WIDTH           500     // ����������(ms)
#define RELAY_FEEDBACK_DELAY        500     // ���������ʱ(ms)
#define RELAY_CHECK_COUNT           3       // ʹ���źż�����

// ========== �������� ==========
void RelayControl_ProcessPendingActions(void); // ���������Ķ���

#ifdef __cplusplus
}
#endif

#endif /* __RELAY_CONTROL_H__ */ 



