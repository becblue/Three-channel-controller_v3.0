/**
  ******************************************************************************
  * @file    iwdg_control.h
  * @brief   IWDG���Ź�����ģ��ͷ�ļ�
  * @author  ��ͨ���л������ϵͳ
  * @version V3.0
  * @date    2024
  ******************************************************************************
  * @attention
  * 
  * ��ģ��ʵ��IWDG�������Ź����ܣ�Ϊ�߰�ȫ����Ĺ�ҵ����ϵͳ�ṩ�ɿ���
  * ϵͳ��λ��������Ҫ���ԣ�
  * 1. 3�볬ʱʱ�䣬�ʺϹ�ҵ����Ӧ��
  * 2. ����ι�����ƣ�ֻ��ϵͳ����ʱ��ι��
  * 3. �밲ȫ���ϵͳ���ɣ��쳣ʱֹͣι��
  * 4. ������״̬��غ͵��Խӿ�
  * 5. ��λ���״̬�ָ����쳣��¼
  * 
  ******************************************************************************
  */

#ifndef __IWDG_CONTROL_H
#define __IWDG_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  ���Ź�״̬ö��
  */
typedef enum {
    IWDG_STATE_DISABLED = 0,    // ���Ź�δ����
    IWDG_STATE_ENABLED,         // ���Ź�������
    IWDG_STATE_FEEDING,         // ����ι��
    IWDG_STATE_SUSPENDED        // ι������ͣ���쳣״̬��
} IwdgState_t;

/**
  * @brief  ���Ź���λԭ��ö��
  */
typedef enum {
    IWDG_RESET_NONE = 0,        // �޸�λ
    IWDG_RESET_UNKNOWN,         // δ֪��λԭ��
    IWDG_RESET_POWER_ON,        // �ϵ縴λ
    IWDG_RESET_SOFTWARE,        // �����λ
    IWDG_RESET_WATCHDOG,        // ���Ź���λ
    IWDG_RESET_WINDOW_WATCHDOG, // ���ڿ��Ź���λ
    IWDG_RESET_LOW_POWER,       // �͹��ĸ�λ
    IWDG_RESET_PIN              // ���Ÿ�λ
} IwdgResetReason_t;

/**
  * @brief  ���Ź�����ͳ����Ϣ
  */
typedef struct {
    uint32_t feed_count;            // ι������
    uint32_t suspend_count;         // ��ͣ����
    uint32_t resume_count;          // �ָ�����
    uint32_t last_feed_time;        // �ϴ�ι��ʱ��
    uint32_t last_suspend_time;     // �ϴ���ͣʱ��
    uint32_t total_run_time;        // ������ʱ��
    uint32_t total_suspend_time;    // ����ͣʱ��
    IwdgResetReason_t last_reset_reason;  // �ϴθ�λԭ��
} IwdgStatistics_t;

/**
  * @brief  ���Ź����ƽṹ��
  */
typedef struct {
    IwdgState_t state;              // ��ǰ״̬
    uint8_t auto_feed_enabled;      // �Զ�ι��ʹ�ܱ�־
    uint8_t debug_mode;             // ����ģʽ��־
    uint32_t feed_interval;         // ι�����ʱ�䣨ms��
    uint32_t timeout_value;         // ��ʱʱ�䣨ms��
    uint32_t start_time;            // ����ʱ��
    IwdgStatistics_t statistics;    // ����ͳ����Ϣ
} IwdgControl_t;

/* Exported constants --------------------------------------------------------*/

// ���Ź����ò���
#define IWDG_TIMEOUT_MS         3000        // ��ʱʱ�䣺3��
#define IWDG_FEED_INTERVAL_MS   500         // ι�������500ms
#define IWDG_PRESCALER          IWDG_PRESCALER_64  // Ԥ��Ƶ����64
#define IWDG_RELOAD_VALUE       1875        // ��װ��ֵ��1875 (3�볬ʱ)

// ���Ź�״̬��־
#define IWDG_FLAG_ENABLED       0x01        // ���Ź�������
#define IWDG_FLAG_FEEDING       0x02        // ����ι��
#define IWDG_FLAG_SUSPENDED     0x04        // ι������ͣ
#define IWDG_FLAG_DEBUG         0x08        // ����ģʽ

/* Exported macro ------------------------------------------------------------*/

/**
  * @brief  ��鿴�Ź��Ƿ���ָ��״̬
  */
#define IWDG_IS_STATE(state)    (IwdgControl_GetState() == (state))

/**
  * @brief  ��鿴�Ź��Ƿ�������
  */
#define IWDG_IS_ENABLED()       (IwdgControl_GetState() != IWDG_STATE_DISABLED)

/**
  * @brief  ����Ƿ������Զ�ι��
  */
#define IWDG_IS_AUTO_FEED_ENABLED() (IwdgControl_IsAutoFeedEnabled())

/* Exported functions prototypes ---------------------------------------------*/

/* ��ʼ�������ú��� */
void IwdgControl_Init(void);
void IwdgControl_DeInit(void);
void IwdgControl_Config(uint32_t timeout_ms, uint32_t feed_interval_ms);

/* ���ƺ��� */
void IwdgControl_Start(void);
void IwdgControl_Stop(void);
void IwdgControl_Feed(void);
void IwdgControl_Suspend(void);
void IwdgControl_Resume(void);

/* ״̬��ѯ���� */
IwdgState_t IwdgControl_GetState(void);
uint8_t IwdgControl_IsAutoFeedEnabled(void);
uint8_t IwdgControl_IsDebugMode(void);
IwdgResetReason_t IwdgControl_GetLastResetReason(void);

/* ͳ����Ϣ���� */
IwdgStatistics_t IwdgControl_GetStatistics(void);
void IwdgControl_ResetStatistics(void);
void IwdgControl_PrintStatistics(void);

/* �Զ�ι�����ƺ��� */
void IwdgControl_EnableAutoFeed(void);
void IwdgControl_DisableAutoFeed(void);
void IwdgControl_ProcessAutoFeed(void);

/* ���Ժͼ�غ��� */
void IwdgControl_EnableDebugMode(void);
void IwdgControl_DisableDebugMode(void);
void IwdgControl_PrintStatus(void);
void IwdgControl_PrintResetReason(void);

/* ϵͳ���ɺ��� */
void IwdgControl_SystemStartupCheck(void);
void IwdgControl_SafetyMonitorIntegration(void);
uint8_t IwdgControl_IsSystemSafeToFeed(void);

/* ��λԭ������� */
void IwdgControl_SetResetReason(IwdgResetReason_t reason);
void IwdgControl_SetResetCount(uint32_t count);

/* ���Ժ��� */
void IwdgControl_TestWatchdog(void);
void IwdgControl_TestResetRecovery(void);

#ifdef __cplusplus
}
#endif

#endif /* __IWDG_CONTROL_H */


