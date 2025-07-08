/**
  ******************************************************************************
  * @file    iwdg_control.c
  * @brief   IWDG���Ź�����ģ��Դ�ļ�
  * @author  ��ͨ���л������ϵͳ
  * @version V3.0
  * @date    2024
  ******************************************************************************
  * @attention
  * 
  * ��ģ��ʵ��IWDG�������Ź����ܣ�Ϊ�߰�ȫ����Ĺ�ҵ����ϵͳ�ṩ�ɿ���
  * ϵͳ��λ�����������������ԣ�
  * 1. 3�볬ʱʱ�䣬�ʺϹ�ҵ����Ӧ��
  * 2. ����ι�����ƣ�ֻ��ϵͳ����ʱ��ι��
  * 3. �밲ȫ���ϵͳ���ɣ��쳣ʱֹͣι��
  * 4. ������״̬��غ͵��Խӿ�
  * 5. ��λ���״̬�ָ����쳣��¼
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "iwdg_control.h"
#include "iwdg.h"
#include "safety_monitor.h"
#include "system_control.h"
#include "log_system.h"
#include "usart.h"
#include "smart_delay.h"  // ������ʱ����
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define IWDG_MAGIC_NUMBER       0x5A5A5A5A  // ���Ź�������־
#define IWDG_RESET_COUNT_ADDR   0x20000000  // ��λ�����洢��ַ������SRAM��ʼ��ַ��

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static IwdgControl_t g_iwdg_control;           // ���Ź����ƽṹ��
static uint32_t g_iwdg_last_feed_time = 0;    // �ϴ�ι��ʱ��
static uint32_t g_iwdg_reset_count = 0;       // ��λ����

/* Private function prototypes -----------------------------------------------*/
static void IwdgControl_InitStructure(void);
static void IwdgControl_AnalyzeResetReason(void);
// ͳ����Ϣ���º������Ƴ���ͳ����Ϣ��ʵ�ʲ�����ֱ�Ӹ���
static uint8_t IwdgControl_CheckSafetyConditions(void);
static void IwdgControl_HandleResetRecovery(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  ���Ź�����ģ���ʼ��
  * @retval ��
  */
void IwdgControl_Init(void)
{
    // ��ʼ�����ƽṹ��
    IwdgControl_InitStructure();
    
    // ������λԭ��
    IwdgControl_AnalyzeResetReason();
    
    // ϵͳ�������
    IwdgControl_SystemStartupCheck();
    
    // ����λ�ָ�
    IwdgControl_HandleResetRecovery();
    
    DEBUG_Printf("=== IWDG���Ź�����ģ���ʼ����� ===\r\n");
    DEBUG_Printf("��ʱʱ��: %lums, ι�����: %lums\r\n", 
                g_iwdg_control.timeout_value, g_iwdg_control.feed_interval);
    
    // ��ӡ��λԭ��
    IwdgControl_PrintResetReason();
}

/**
  * @brief  ���Ź�����ģ�鷴��ʼ��
  * @retval ��
  */
void IwdgControl_DeInit(void)
{
    // ֹͣ���Ź�
    IwdgControl_Stop();
    
    // ������ƽṹ��
    memset(&g_iwdg_control, 0, sizeof(g_iwdg_control));
    
    DEBUG_Printf("IWDG���Ź�����ģ�鷴��ʼ�����\r\n");
}

/**
  * @brief  ���ÿ��Ź�����
  * @param  timeout_ms: ��ʱʱ�䣨���룩
  * @param  feed_interval_ms: ι����������룩
  * @retval ��
  */
void IwdgControl_Config(uint32_t timeout_ms, uint32_t feed_interval_ms)
{
    g_iwdg_control.timeout_value = timeout_ms;
    g_iwdg_control.feed_interval = feed_interval_ms;
    
    DEBUG_Printf("IWDG���ø���: ��ʱ=%lums, ι�����=%lums\r\n", 
                timeout_ms, feed_interval_ms);
}

/**
  * @brief  �������Ź�
  * @retval ��
  */
void IwdgControl_Start(void)
{
    if(g_iwdg_control.state == IWDG_STATE_DISABLED) {
        // ��ʼ��IWDG����
        MX_IWDG_Init();
        
        // ����״̬
        g_iwdg_control.state = IWDG_STATE_ENABLED;
        g_iwdg_control.start_time = HAL_GetTick();
        g_iwdg_control.auto_feed_enabled = 1;
        
        // ����ι��һ��
        IwdgControl_Feed();
        
        DEBUG_Printf("=== IWDG���Ź������ɹ� ===\r\n");
        DEBUG_Printf("���Ź���������״̬: ENABLED\r\n");
    } else {
        DEBUG_Printf("IWDG���Ź��Ѿ���������ǰ״̬: %d\r\n", g_iwdg_control.state);
    }
}

/**
  * @brief  ֹͣ���Ź���ע�⣺IWDGһ�������޷�ֹͣ��
  * @retval ��
  */
void IwdgControl_Stop(void)
{
    DEBUG_Printf("���棺IWDG���Ź�һ�������޷�ֹͣ��\r\n");
    DEBUG_Printf("ֻ��ֹͣ�Զ�ι�������Ź�Ӳ����������\r\n");
    
    // ֹͣ�Զ�ι��
    g_iwdg_control.auto_feed_enabled = 0;
    g_iwdg_control.state = IWDG_STATE_SUSPENDED;
    
    DEBUG_Printf("�Զ�ι����ֹͣ��ϵͳ����%lums��λ\r\n", g_iwdg_control.timeout_value);
}

/**
  * @brief  �ֶ�ι��
  * @retval ��
  */
void IwdgControl_Feed(void)
{
    // ��鰲ȫ����
    if(!IwdgControl_CheckSafetyConditions()) {
        DEBUG_Printf("��ȫ���������㣬�ܾ�ι����\r\n");
        return;
    }
    
    // ִ��ι������
    IWDG_Refresh();
    
    // ����״̬
    g_iwdg_control.state = IWDG_STATE_FEEDING;
    g_iwdg_control.statistics.feed_count++;
    g_iwdg_control.statistics.last_feed_time = HAL_GetTick();
    g_iwdg_last_feed_time = HAL_GetTick();
    
    if(g_iwdg_control.debug_mode) {
        DEBUG_Printf("[IWDG] ι���ɹ�������: %lu\r\n", g_iwdg_control.statistics.feed_count);
    }
    
    // ������ʱ��ָ���������״̬
    HAL_Delay(1);
    g_iwdg_control.state = IWDG_STATE_ENABLED;
}

/**
  * @brief  ��ͣ�Զ�ι��
  * @retval ��
  */
void IwdgControl_Suspend(void)
{
    g_iwdg_control.auto_feed_enabled = 0;
    g_iwdg_control.state = IWDG_STATE_SUSPENDED;
    g_iwdg_control.statistics.suspend_count++;
    g_iwdg_control.statistics.last_suspend_time = HAL_GetTick();
    
    DEBUG_Printf("=== IWDG�Զ�ι������ͣ ===\r\n");
    DEBUG_Printf("��ͣ����: %lu, ϵͳ����%lums��λ\r\n", 
                g_iwdg_control.statistics.suspend_count, g_iwdg_control.timeout_value);
    
    // ��¼ϵͳ�����¼�����־ϵͳ
    if(LogSystem_IsInitialized()) {
        char msg[48];
        snprintf(msg, sizeof(msg), "���Ź���ͣ��ϵͳ������������ͣ����:%lu", g_iwdg_control.statistics.suspend_count);
        LOG_SYSTEM_LOCK(msg);
    }
}

/**
  * @brief  �ָ��Զ�ι��
  * @retval ��
  */
void IwdgControl_Resume(void)
{
    g_iwdg_control.auto_feed_enabled = 1;
    g_iwdg_control.state = IWDG_STATE_ENABLED;
    g_iwdg_control.statistics.resume_count++;
    
    // ����ι��һ��
    IwdgControl_Feed();
    
    DEBUG_Printf("=== IWDG�Զ�ι���ѻָ� ===\r\n");
    DEBUG_Printf("�ָ�����: %lu\r\n", g_iwdg_control.statistics.resume_count);
}

/**
  * @brief  ��ȡ���Ź���ǰ״̬
  * @retval ���Ź�״̬
  */
IwdgState_t IwdgControl_GetState(void)
{
    return g_iwdg_control.state;
}

/**
  * @brief  ����Ƿ��������Զ�ι��
  * @retval 1:���� 0:����
  */
uint8_t IwdgControl_IsAutoFeedEnabled(void)
{
    return g_iwdg_control.auto_feed_enabled;
}

/**
  * @brief  ����Ƿ�Ϊ����ģʽ
  * @retval 1:����ģʽ 0:����ģʽ
  */
uint8_t IwdgControl_IsDebugMode(void)
{
    return g_iwdg_control.debug_mode;
}

/**
  * @brief  ��ȡ�ϴθ�λԭ��
  * @retval ��λԭ��
  */
IwdgResetReason_t IwdgControl_GetLastResetReason(void)
{
    return g_iwdg_control.statistics.last_reset_reason;
}

/**
  * @brief  ��ȡͳ����Ϣ
  * @retval ͳ����Ϣ�ṹ��
  */
IwdgStatistics_t IwdgControl_GetStatistics(void)
{
    // ��������ʱ��
    if(g_iwdg_control.start_time > 0) {
        g_iwdg_control.statistics.total_run_time = 
            HAL_GetTick() - g_iwdg_control.start_time;
    }
    
    return g_iwdg_control.statistics;
}

/**
  * @brief  ����ͳ����Ϣ
  * @retval ��
  */
void IwdgControl_ResetStatistics(void)
{
    memset(&g_iwdg_control.statistics, 0, sizeof(IwdgStatistics_t));
    g_iwdg_control.start_time = HAL_GetTick();
    
    DEBUG_Printf("IWDGͳ����Ϣ������\r\n");
}

/**
  * @brief  ��ӡͳ����Ϣ
  * @retval ��
  */
void IwdgControl_PrintStatistics(void)
{
    IwdgStatistics_t stats = IwdgControl_GetStatistics();
    
    DEBUG_Printf("=== IWDG���Ź�ͳ����Ϣ ===\r\n");
    DEBUG_Printf("ι������: %lu\r\n", stats.feed_count);
    DEBUG_Printf("��ͣ����: %lu\r\n", stats.suspend_count);
    DEBUG_Printf("�ָ�����: %lu\r\n", stats.resume_count);
    DEBUG_Printf("������ʱ��: %lums\r\n", stats.total_run_time);
    DEBUG_Printf("�ϴ�ι��ʱ��: %lums\r\n", stats.last_feed_time);
    DEBUG_Printf("�ϴ���ͣʱ��: %lums\r\n", stats.last_suspend_time);
    DEBUG_Printf("�ϴθ�λԭ��: %d\r\n", stats.last_reset_reason);
}

/**
  * @brief  �����Զ�ι��
  * @retval ��
  */
void IwdgControl_EnableAutoFeed(void)
{
    g_iwdg_control.auto_feed_enabled = 1;
    DEBUG_Printf("IWDG�Զ�ι��������\r\n");
}

/**
  * @brief  �����Զ�ι��
  * @retval ��
  */
void IwdgControl_DisableAutoFeed(void)
{
    g_iwdg_control.auto_feed_enabled = 0;
    DEBUG_Printf("IWDG�Զ�ι���ѽ���\r\n");
}

/**
  * @brief  �����Զ�ι��������ѭ���е��ã�
  * @retval ��
  */
void IwdgControl_ProcessAutoFeed(void)
{
    // ����Ƿ������Զ�ι��
    if(!g_iwdg_control.auto_feed_enabled) {
        return;
    }
    
    // ����Ƿ񵽴�ι�����ʱ��
    uint32_t current_time = HAL_GetTick();
    if(current_time - g_iwdg_last_feed_time >= g_iwdg_control.feed_interval) {
        // ���ϵͳ�Ƿ�ȫ
        if(IwdgControl_IsSystemSafeToFeed()) {
            IwdgControl_Feed();
        } else {
            // ϵͳ����ȫ����ͣι��
            if(g_iwdg_control.state != IWDG_STATE_SUSPENDED) {
                DEBUG_Printf("ϵͳ�쳣����ͣIWDGι��\r\n");
                IwdgControl_Suspend();
            }
        }
    }
}

/**
  * @brief  ���õ���ģʽ
  * @retval ��
  */
void IwdgControl_EnableDebugMode(void)
{
    g_iwdg_control.debug_mode = 1;
    DEBUG_Printf("IWDG����ģʽ������\r\n");
}

/**
  * @brief  ���õ���ģʽ
  * @retval ��
  */
void IwdgControl_DisableDebugMode(void)
{
    g_iwdg_control.debug_mode = 0;
    DEBUG_Printf("IWDG����ģʽ�ѽ���\r\n");
}

/**
  * @brief  ��ӡ��ǰ״̬
  * @retval ��
  */
void IwdgControl_PrintStatus(void)
{
    const char* state_str[] = {"DISABLED", "ENABLED", "FEEDING", "SUSPENDED"};
    
    DEBUG_Printf("=== IWDG���Ź���ǰ״̬ ===\r\n");
    DEBUG_Printf("״̬: %s\r\n", state_str[g_iwdg_control.state]);
    DEBUG_Printf("�Զ�ι��: %s\r\n", g_iwdg_control.auto_feed_enabled ? "����" : "����");
    DEBUG_Printf("����ģʽ: %s\r\n", g_iwdg_control.debug_mode ? "����" : "����");
    DEBUG_Printf("��ʱʱ��: %lums\r\n", g_iwdg_control.timeout_value);
    DEBUG_Printf("ι�����: %lums\r\n", g_iwdg_control.feed_interval);
    DEBUG_Printf("����ʱ��: %lums\r\n", HAL_GetTick() - g_iwdg_control.start_time);
    DEBUG_Printf("�����ϴ�ι��: %lums\r\n", HAL_GetTick() - g_iwdg_last_feed_time);
}

/**
  * @brief  ��ӡ��λԭ��
  * @retval ��
  */
void IwdgControl_PrintResetReason(void)
{
    const char* reason_str[] = {
        "�޸�λ", "δ֪��λ", "�ϵ縴λ", "�����λ", 
        "���Ź���λ", "���ڿ��Ź���λ", "�͹��ĸ�λ", "���Ÿ�λ"
    };
    
    DEBUG_Printf("=== ϵͳ��λԭ����� ===\r\n");
    DEBUG_Printf("�ϴθ�λԭ��: %s\r\n", reason_str[g_iwdg_control.statistics.last_reset_reason]);
    DEBUG_Printf("��ʷ��λ����: %lu\r\n", g_iwdg_reset_count);
}

/**
  * @brief  ϵͳ�������
  * @retval ��
  */
void IwdgControl_SystemStartupCheck(void)
{
    DEBUG_Printf("=== IWDGϵͳ������� ===\r\n");
    
    // ����Ƿ�Ϊ���Ź���λ
    if(g_iwdg_control.statistics.last_reset_reason == IWDG_RESET_WATCHDOG) {
        DEBUG_Printf("��⵽���Ź���λ��ϵͳ���ڻָ�...\r\n");
        g_iwdg_reset_count++;
        
        // ���Ƶ����λ��������Ҫ���⴦��
        if(g_iwdg_reset_count > 3) {
            DEBUG_Printf("���棺Ƶ�����Ź���λ����λ����: %lu\r\n", g_iwdg_reset_count);
        }
    }
    
    DEBUG_Printf("ϵͳ����������\r\n");
}

/**
  * @brief  �밲ȫ���ϵͳ����
  * @retval ��
  */
void IwdgControl_SafetyMonitorIntegration(void)
{
    // ��鰲ȫ���ϵͳ״̬
    uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
    
    if(alarm_flags != 0) {
        // �б������ڣ���ͣι��
        if(g_iwdg_control.auto_feed_enabled) {
            DEBUG_Printf("��⵽��ȫ����(0x%04X)����ͣIWDGι��\r\n", alarm_flags);
            IwdgControl_Suspend();
        }
    } else {
        // �ޱ������ָ�ι��
        if(!g_iwdg_control.auto_feed_enabled && g_iwdg_control.state == IWDG_STATE_SUSPENDED) {
            DEBUG_Printf("��ȫ������������ָ�IWDGι��\r\n");
            IwdgControl_Resume();
        }
    }
}

/**
  * @brief  ���ϵͳ�Ƿ�ȫ����ι��
  * @retval 1:��ȫ 0:����ȫ
  */
uint8_t IwdgControl_IsSystemSafeToFeed(void)
{
    // ���ϵͳ״̬
    SystemState_t system_state = SystemControl_GetState();
    
    // ���Լ�׶���ͣι��
    if(system_state == SYSTEM_STATE_SELF_TEST || 
       system_state == SYSTEM_STATE_LOGO_DISPLAY ||
       system_state == SYSTEM_STATE_ERROR) {
        return 0;
    }
    
    // ��鰲ȫ���ϵͳ
    uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
    
    // ��������ر�����K~M���¶��쳣����ֹͣι��
    for(int i = 10; i <= 12; i++) {  // K~M���쳣������10-12��
        if(alarm_flags & (1 << i)) {
            return 0;  // �¶��쳣��ֹͣι��
        }
    }
    
    // �����쳣����¼���ι������ֹϵͳ��ν��λ
    return 1;
}

/**
  * @brief  ���Կ��Ź�����
  * @retval ��
  */
void IwdgControl_TestWatchdog(void)
{
    DEBUG_Printf("=== IWDG���Ź����Կ�ʼ ===\r\n");
    DEBUG_Printf("���棺���Խ�ֹͣι����ϵͳ����%lums��λ\r\n", g_iwdg_control.timeout_value);
    
    // ֹͣι��
    IwdgControl_Suspend();
    
    // �ȴ���λ
    DEBUG_Printf("�ȴ����Ź���λ...\r\n");
    while(1) {
        HAL_Delay(100);
        DEBUG_Printf(".");
    }
}

/**
  * @brief  ���Ը�λ�ָ�����
  * @retval ��
  */
void IwdgControl_TestResetRecovery(void)
{
    DEBUG_Printf("=== IWDG��λ�ָ����� ===\r\n");
    IwdgControl_PrintResetReason();
    IwdgControl_PrintStatistics();
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  ��ʼ�����ƽṹ��
  * @retval ��
  */
static void IwdgControl_InitStructure(void)
{
    memset(&g_iwdg_control, 0, sizeof(g_iwdg_control));
    
    // ����Ĭ�ϲ���
    g_iwdg_control.state = IWDG_STATE_DISABLED;
    g_iwdg_control.auto_feed_enabled = 0;
    g_iwdg_control.debug_mode = 0;
    g_iwdg_control.timeout_value = IWDG_TIMEOUT_MS;
    g_iwdg_control.feed_interval = IWDG_FEED_INTERVAL_MS;
    g_iwdg_control.start_time = 0;
    
    // ��ʼ��ʱ��
    g_iwdg_last_feed_time = HAL_GetTick();
}

/**
  * @brief  ������λԭ��
  * @retval ��
  */
static void IwdgControl_AnalyzeResetReason(void)
{
    // ��鸴λ��־
    if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_WATCHDOG;
        __HAL_RCC_CLEAR_RESET_FLAGS();
        // ��¼���Ź���λ����־ϵͳ
        if(LogSystem_IsInitialized()) {
            LOG_WATCHDOG_RESET();
        }
    } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_WINDOW_WATCHDOG;
        __HAL_RCC_CLEAR_RESET_FLAGS();
        // ��¼���ڿ��Ź���λ����־ϵͳ
        if(LogSystem_IsInitialized()) {
            LOG_WATCHDOG_RESET();
        }
    } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_POWER_ON;
        __HAL_RCC_CLEAR_RESET_FLAGS();
    } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_SOFTWARE;
        __HAL_RCC_CLEAR_RESET_FLAGS();
    } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_PIN;
        __HAL_RCC_CLEAR_RESET_FLAGS();
    } else {
        g_iwdg_control.statistics.last_reset_reason = IWDG_RESET_UNKNOWN;
    }
}

// ͳ����Ϣ���º������Ƴ���ͳ����Ϣ��ʵ�ʲ�����ֱ�Ӹ���

/**
  * @brief  ��鰲ȫ����
  * @retval 1:��ȫ 0:����ȫ
  */
static uint8_t IwdgControl_CheckSafetyConditions(void)
{
    // ������ȫ���
    if(g_iwdg_control.state == IWDG_STATE_DISABLED) {
        return 0;
    }
    
    // ����Ƿ��ڰ�ȫ��ϵͳ״̬��
    return IwdgControl_IsSystemSafeToFeed();
}

/**
  * @brief  ����λ�ָ�
  * @retval ��
  */
static void IwdgControl_HandleResetRecovery(void)
{
    // ����ǿ��Ź���λ���������⴦��
    if(g_iwdg_control.statistics.last_reset_reason == IWDG_RESET_WATCHDOG) {
        DEBUG_Printf("���Ź���λ�ָ�������...\r\n");
        
        // �ӳ�һ��ʱ�����������Ź�����ϵͳ�ȶ�ʱ��
        SmartDelayWithForceFeed(1000);
        
        DEBUG_Printf("���Ź���λ�ָ����\r\n");
    }
} 


