#ifndef __RELAY_CONTROL_H__
#define __RELAY_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * �̵�������ģ��ͷ�ļ�
 * 1. ͨ�����ƣ�����/�رգ�- �����첽״̬���ܹ�
 * 2. ������⡢���ؼ�⡢״̬�������쳣����
 * 3. �жϷ����͸��ż�⹦��
 * 4. �ж����ȼ��������
 ************************************************************/

// �̵���ͨ��״̬ö��
typedef enum {
    RELAY_STATE_OFF = 0,    // �̵����ر�״̬
    RELAY_STATE_ON,         // �̵�����״̬
    RELAY_STATE_ERROR       // �̵�������״̬
} RelayState_t;

// ================== �첽״̬���ܹ� ===================

// �̵����첽����״̬ö��
typedef enum {
    RELAY_ASYNC_IDLE = 0,           // ����״̬
    RELAY_ASYNC_PULSE_START,        // ���忪ʼ
    RELAY_ASYNC_PULSE_ACTIVE,       // ���������
    RELAY_ASYNC_PULSE_END,          // �������
    RELAY_ASYNC_FEEDBACK_WAIT,      // �ȴ�����
    RELAY_ASYNC_FEEDBACK_CHECK,     // ��鷴��
    RELAY_ASYNC_COMPLETE,           // �������
    RELAY_ASYNC_ERROR               // ����ʧ��
} RelayAsyncState_t;

// �첽�������ƽṹ��
typedef struct {
    RelayAsyncState_t state;        // ��ǰ״̬
    uint8_t channel;                // ͨ����(1-3)
    uint8_t operation;              // ��������(0=�ر�, 1=����)
    uint32_t start_time;            // ������ʼʱ��
    uint32_t phase_start_time;      // ��ǰ�׶ο�ʼʱ��
    uint8_t result;                 // �������
    uint8_t error_code;             // �������
} RelayAsyncOperation_t;

// ================== �жϷ����͸��ż�� ===================

// �ж����ȼ�����ṹ��
typedef struct {
    uint8_t channel;
    uint32_t interrupt_time;
    uint8_t priority;
} InterruptPriority_t;

// ���ż��ṹ��
typedef struct {
    uint32_t last_interrupt_time[3];  // ��ͨ������ж�ʱ��
    uint8_t simultaneous_count;       // ͬʱ�жϼ���
    uint8_t interference_detected;    // ���ż���־
    uint32_t last_interference_time;  // ���һ�θ��ż��ʱ��
} InterferenceFilter_t;

// �̵���ͨ���ṹ��
typedef struct {
    uint8_t channelNum;         // ͨ����(1-3)
    RelayState_t state;         // ��ǰ״̬
    uint32_t lastActionTime;    // ���һ�ζ���ʱ��
    uint8_t errorCode;          // �������
} RelayChannel_t;

// ================== �����͸��ż����� ===================
#define INTERRUPT_DEBOUNCE_TIME_MS    50    // ����ʱ��50ms
#define INTERFERENCE_DETECT_TIME_MS   10    // ͬʱ�жϼ�ⴰ��10ms
#define STATE_VALIDATION_DELAY_MS     100   // ״̬��֤�ӳ�100ms
#define MAX_ASYNC_OPERATIONS         3      // ���ͬʱ�첽������

// ================== �������� ===================

// ��ʼ������
void RelayControl_Init(void);

// ================== �첽�������� ===================

// �����첽��������������
uint8_t RelayControl_StartOpenChannel(uint8_t channelNum);   // ������������
uint8_t RelayControl_StartCloseChannel(uint8_t channelNum);  // �����رղ���

// �첽״̬����ѯ����
void RelayControl_ProcessAsyncOperations(void);

// ����첽����״̬
uint8_t RelayControl_IsOperationInProgress(uint8_t channelNum);
uint8_t RelayControl_GetOperationResult(uint8_t channelNum);

// ================== �����Ժ����������汾�����������ݣ� ===================

// ͨ�����ƺ����������汾�����ּ����ԣ�
uint8_t RelayControl_OpenChannel(uint8_t channelNum);   // ����ָ��ͨ��
uint8_t RelayControl_CloseChannel(uint8_t channelNum);  // �ر�ָ��ͨ��

// ================== ״̬��⺯�� ===================

// ״̬��⺯��
RelayState_t RelayControl_GetChannelState(uint8_t channelNum); // ��ȡͨ��״̬
uint8_t RelayControl_CheckChannelFeedback(uint8_t channelNum); // ���ͨ������

// ������麯��
uint8_t RelayControl_CheckInterlock(uint8_t channelNum); // ���ͨ������

// ��������
uint8_t RelayControl_GetLastError(uint8_t channelNum);   // ��ȡ���һ�δ���
void RelayControl_ClearError(uint8_t channelNum);        // �������״̬

// ================== �жϴ����� ===================

// ��ǿ��ʹ���źŴ�����������+���ż�⣩
void RelayControl_HandleEnableSignal(uint8_t channelNum, uint8_t state);

// ���ȼ��жϴ���
void RelayControl_ProcessPendingActions(void);

// ���ż���״̬��֤
uint8_t RelayControl_ValidateStateChange(uint8_t k1_en, uint8_t k2_en, uint8_t k3_en);
uint8_t RelayControl_GetHighestPriorityInterrupt(void);

// ================== ���Ժ�ͳ�ƺ��� ===================

// ��ȡ�첽����ͳ����Ϣ
void RelayControl_GetAsyncStatistics(uint32_t* total_ops, uint32_t* completed_ops, uint32_t* failed_ops);

// ��ȡ���ż��ͳ��
void RelayControl_GetInterferenceStatistics(uint32_t* interference_count, uint32_t* filtered_interrupts);

// ������붨��
#define RELAY_ERR_NONE              0x00    // �޴���
#define RELAY_ERR_INTERLOCK         0x01    // ��������
#define RELAY_ERR_FEEDBACK          0x02    // ��������
#define RELAY_ERR_TIMEOUT           0x03    // ��ʱ����
#define RELAY_ERR_INVALID_CHANNEL   0x04    // ��Чͨ��
#define RELAY_ERR_INVALID_STATE     0x05    // ��Ч״̬
#define RELAY_ERR_POWER_FAILURE     0x06    // ��Դ�쳣����
#define RELAY_ERR_HARDWARE_FAILURE  0x07    // Ӳ�������쳣����
#define RELAY_ERR_OPERATION_BUSY    0x08    // ����æ���첽���������У�
#define RELAY_ERR_INTERFERENCE      0x09    // ��⵽����

// ʱ�䳣������
#define RELAY_PULSE_WIDTH           500     // ����������(ms)
#define RELAY_FEEDBACK_DELAY        500     // ���������ʱ(ms)
#define RELAY_ASYNC_PULSE_WIDTH     100     // �첽ģʽ������(ms) - ��������ʱ��
#define RELAY_ASYNC_FEEDBACK_DELAY  100     // �첽ģʽ���������ʱ(ms)

// ================== �첽����״̬��ѯ ===================
uint8_t RelayControl_IsOperationInProgress(uint8_t channelNum);
uint8_t RelayControl_GetOperationResult(uint8_t channelNum);
void RelayControl_GetAsyncStatistics(uint32_t* total_ops, uint32_t* completed_ops, uint32_t* failed_ops);

// ================== �������νӿڣ����ڰ�ȫ��أ� ===================
/**
  * @brief  ���ָ��ͨ���Ƿ����ڽ����첽����
  * @param  channelNum: ͨ����(1-3)
  * @retval 1:���ڲ��� 0:����״̬
  * @note   ���ڰ�ȫ���ģ���ж��Ƿ���Ҫ��������쳣���
  */
uint8_t RelayControl_IsChannelBusy(uint8_t channelNum);

/**
  * @brief  ��ȡָ��ͨ����ǰ���첽��������
  * @param  channelNum: ͨ����(1-3)
  * @retval 1:�������� 0:�رղ��� 255:�޲���
  * @note   ���ڰ�ȫ���ģ�龫ȷ��������쳣����
  */
uint8_t RelayControl_GetChannelOperationType(uint8_t channelNum);

#ifdef __cplusplus
}
#endif

#endif /* __RELAY_CONTROL_H__ */ 



