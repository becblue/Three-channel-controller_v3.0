/**
  ******************************************************************************
  * @file    iwdg_test.c
  * @brief   IWDG���Ź����ܲ�����֤����
  * @author  ��ͨ���л������ϵͳ
  * @version V3.0
  * @date    2024
  ******************************************************************************
  * @attention
  * 
  * �����Գ���������֤IWDG���Ź����ܵ���ȷ�ԣ�������
  * 1. ���Ź����������ò���
  * 2. �Զ�ι�����Ʋ���
  * 3. ��ȫ��ؼ��ɲ���
  * 4. ��λ�ָ�����
  * 5. ״̬��غ�ͳ�Ʋ���
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "iwdg_control.h"
#include "safety_monitor.h"
#include "system_control.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TEST_TIMEOUT_MS     10000   // ���Գ�ʱʱ�䣺10��

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// �������б�־���Ƴ�����Ϊ�ֲ���������
static uint32_t g_test_start_time = 0;  // ���Կ�ʼʱ��

/* Private function prototypes -----------------------------------------------*/
static void IwdgTest_PrintTestHeader(const char* test_name);
static void IwdgTest_PrintTestResult(const char* test_name, uint8_t result);
// �ȴ��������㺯�����Ƴ�����Ϊֱ��ʹ��HAL_Delay��״̬���

/* Test functions prototypes -------------------------------------------------*/
static uint8_t IwdgTest_BasicInitialization(void);
static uint8_t IwdgTest_StartAndFeed(void);
static uint8_t IwdgTest_AutoFeedMechanism(void);
static uint8_t IwdgTest_SafetyMonitorIntegration(void);
static uint8_t IwdgTest_StateTransition(void);
static uint8_t IwdgTest_StatisticsAndMonitoring(void);
static uint8_t IwdgTest_ConfigurationChange(void);
static uint8_t IwdgTest_DebugMode(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  ִ��������IWDG���Ź����ܲ����׼�
  * @retval 1:����ͨ�� 0:����ʧ��
  */
uint8_t IwdgTest_RunFullTestSuite(void)
{
    uint8_t test_count = 0;
    uint8_t pass_count = 0;
    
    DEBUG_Printf("\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("  IWDG���Ź��������������׼�����\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("����ʱ��: %s %s\r\n", __DATE__, __TIME__);
    DEBUG_Printf("ϵͳʱ��: %luMHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
    DEBUG_Printf("========================================\r\n");
    
    g_test_start_time = HAL_GetTick();
    
    // ����1��������ʼ������
    test_count++;
    if(IwdgTest_BasicInitialization()) {
        pass_count++;
    }
    
    // ����2��������ι������
    test_count++;
    if(IwdgTest_StartAndFeed()) {
        pass_count++;
    }
    
    // ����3���Զ�ι�����Ʋ���
    test_count++;
    if(IwdgTest_AutoFeedMechanism()) {
        pass_count++;
    }
    
    // ����4����ȫ��ؼ��ɲ���
    test_count++;
    if(IwdgTest_SafetyMonitorIntegration()) {
        pass_count++;
    }
    
    // ����5��״̬ת������
    test_count++;
    if(IwdgTest_StateTransition()) {
        pass_count++;
    }
    
    // ����6��ͳ�ƺͼ�ز���
    test_count++;
    if(IwdgTest_StatisticsAndMonitoring()) {
        pass_count++;
    }
    
    // ����7�����ø��Ĳ���
    test_count++;
    if(IwdgTest_ConfigurationChange()) {
        pass_count++;
    }
    
    // ����8������ģʽ����
    test_count++;
    if(IwdgTest_DebugMode()) {
        pass_count++;
    }
    
    // ����ܽ��
    DEBUG_Printf("\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("  IWDG���Ź����ܲ����׼����\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("�ܲ�������: %d\r\n", test_count);
    DEBUG_Printf("ͨ��������: %d\r\n", pass_count);
    DEBUG_Printf("ʧ�ܲ�����: %d\r\n", test_count - pass_count);
    DEBUG_Printf("���Գɹ���: %d%%\r\n", (pass_count * 100) / test_count);
    DEBUG_Printf("�ܺ�ʱ: %lums\r\n", HAL_GetTick() - g_test_start_time);
    
    if(pass_count == test_count) {
        DEBUG_Printf("? ���в���ͨ����IWDG���Ź����ܹ�������\r\n");
        return 1;
    } else {
        DEBUG_Printf("? �в���ʧ�ܣ�����IWDG���Ź�����\r\n");
        return 0;
    }
}

/**
  * @brief  ִ�п��Ź���λ���ԣ�Σ�ղ�����
  * @retval �ޣ�ϵͳ����λ��
  */
void IwdgTest_RunResetTest(void)
{
    DEBUG_Printf("\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("  IWDG���Ź���λ���ԣ�Σ�ղ�����\r\n");
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("���棺�˲��Խ�����ϵͳ��λ��\r\n");
    DEBUG_Printf("��ȷ���ѱ���������Ҫ����\r\n");
    DEBUG_Printf("5���ʼ����...\r\n");
    
    // 5�뵹��ʱ
    for(int i = 5; i > 0; i--) {
        DEBUG_Printf("����ʱ: %d��\r\n", i);
        HAL_Delay(1000);
    }
    
    DEBUG_Printf("��ʼ��λ���ԣ�ֹͣι��...\r\n");
    
    // �������Ź�������ֹͣι��
    IwdgControl_Start();
    HAL_Delay(100);
    IwdgControl_Suspend();
    
    DEBUG_Printf("���Ź�����ͣ���ȴ���λ...\r\n");
    
    // �ȴ����Ź���λ��Լ3�룩
    uint32_t start_time = HAL_GetTick();
    while(1) {
        uint32_t elapsed = HAL_GetTick() - start_time;
        DEBUG_Printf("�ȴ���λ...%lums\r\n", elapsed);
        HAL_Delay(500);
    }
}

/**
  * @brief  ���ٹ�����֤����
  * @retval 1:��֤ͨ�� 0:��֤ʧ��
  */
uint8_t IwdgTest_QuickFunctionTest(void)
{
    DEBUG_Printf("\r\n=== IWDG���Ź����ٹ�����֤ ===\r\n");
    
    // ����ʼ״̬
    if(IwdgControl_GetState() != IWDG_STATE_DISABLED) {
        DEBUG_Printf("? ��ʼ״̬����\r\n");
        return 0;
    }
    
    // ��������
    IwdgControl_Start();
    if(IwdgControl_GetState() != IWDG_STATE_ENABLED) {
        DEBUG_Printf("? ������״̬����\r\n");
        return 0;
    }
    
    // ����ι��
    IwdgControl_Feed();
    HAL_Delay(100);
    
    // ������ͣ�ͻָ�
    IwdgControl_Suspend();
    if(IwdgControl_GetState() != IWDG_STATE_SUSPENDED) {
        DEBUG_Printf("? ��ͣ��״̬����\r\n");
        return 0;
    }
    
    IwdgControl_Resume();
    if(IwdgControl_GetState() != IWDG_STATE_ENABLED) {
        DEBUG_Printf("? �ָ���״̬����\r\n");
        return 0;
    }
    
    DEBUG_Printf("? ���ٹ�����֤ͨ��\r\n");
    return 1;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  ��ӡ���Ա���
  */
static void IwdgTest_PrintTestHeader(const char* test_name)
{
    DEBUG_Printf("\r\n--- ����: %s ---\r\n", test_name);
}

/**
  * @brief  ��ӡ���Խ��
  */
static void IwdgTest_PrintTestResult(const char* test_name, uint8_t result)
{
    if(result) {
        DEBUG_Printf("? %s: ͨ��\r\n", test_name);
    } else {
        DEBUG_Printf("? %s: ʧ��\r\n", test_name);
    }
}

// �ȴ��������㺯�����Ƴ�����Ϊֱ��ʹ��HAL_Delay��״̬���

/**
  * @brief  ����1��������ʼ������
  */
static uint8_t IwdgTest_BasicInitialization(void)
{
    IwdgTest_PrintTestHeader("������ʼ������");
    
    // ���³�ʼ��
    IwdgControl_DeInit();
    IwdgControl_Init();
    
    // ����ʼ״̬
    if(IwdgControl_GetState() != IWDG_STATE_DISABLED) {
        DEBUG_Printf("��ʼ״̬����\r\n");
        IwdgTest_PrintTestResult("������ʼ������", 0);
        return 0;
    }
    
    // ������ò���
    IwdgStatistics_t stats = IwdgControl_GetStatistics();
    if(stats.feed_count != 0) {
        DEBUG_Printf("��ʼͳ����Ϣ����\r\n");
        IwdgTest_PrintTestResult("������ʼ������", 0);
        return 0;
    }
    
    DEBUG_Printf("��ʼ���ɹ���״̬��ȷ\r\n");
    IwdgTest_PrintTestResult("������ʼ������", 1);
    return 1;
}

/**
  * @brief  ����2��������ι������
  */
static uint8_t IwdgTest_StartAndFeed(void)
{
    IwdgTest_PrintTestHeader("������ι������");
    
    // �������Ź�
    IwdgControl_Start();
    HAL_Delay(100);
    
    if(IwdgControl_GetState() != IWDG_STATE_ENABLED) {
        DEBUG_Printf("������״̬����\r\n");
        IwdgTest_PrintTestResult("������ι������", 0);
        return 0;
    }
    
    // �����ֶ�ι��
    uint32_t feed_count_before = IwdgControl_GetStatistics().feed_count;
    IwdgControl_Feed();
    HAL_Delay(50);
    uint32_t feed_count_after = IwdgControl_GetStatistics().feed_count;
    
    if(feed_count_after <= feed_count_before) {
        DEBUG_Printf("ι������δ����\r\n");
        IwdgTest_PrintTestResult("������ι������", 0);
        return 0;
    }
    
    DEBUG_Printf("�����ɹ���ι������\r\n");
    IwdgTest_PrintTestResult("������ι������", 1);
    return 1;
}

/**
  * @brief  ����3���Զ�ι�����Ʋ���
  */
static uint8_t IwdgTest_AutoFeedMechanism(void)
{
    IwdgTest_PrintTestHeader("�Զ�ι�����Ʋ���");
    
    // �����Զ�ι��
    IwdgControl_EnableAutoFeed();
    
    if(!IwdgControl_IsAutoFeedEnabled()) {
        DEBUG_Printf("�Զ�ι��δ����\r\n");
        IwdgTest_PrintTestResult("�Զ�ι�����Ʋ���", 0);
        return 0;
    }
    
    // �ȴ��Զ�ι������
    uint32_t feed_count_before = IwdgControl_GetStatistics().feed_count;
    
    // ģ����ѭ�������Զ�ι��
    for(int i = 0; i < 10; i++) {
        IwdgControl_ProcessAutoFeed();
        HAL_Delay(100);
    }
    
    uint32_t feed_count_after = IwdgControl_GetStatistics().feed_count;
    
    if(feed_count_after <= feed_count_before) {
        DEBUG_Printf("�Զ�ι��δ����\r\n");
        IwdgTest_PrintTestResult("�Զ�ι�����Ʋ���", 0);
        return 0;
    }
    
    DEBUG_Printf("�Զ�ι����������\r\n");
    IwdgTest_PrintTestResult("�Զ�ι�����Ʋ���", 1);
    return 1;
}

/**
  * @brief  ����4����ȫ��ؼ��ɲ���
  */
static uint8_t IwdgTest_SafetyMonitorIntegration(void)
{
    IwdgTest_PrintTestHeader("��ȫ��ؼ��ɲ���");
    
    // �����밲ȫ���ϵͳ�ļ���
    IwdgControl_SafetyMonitorIntegration();
    
    // ����ϵͳ��ȫ���
    uint8_t is_safe = IwdgControl_IsSystemSafeToFeed();
    DEBUG_Printf("ϵͳ��ȫ�����: %s\r\n", is_safe ? "��ȫ" : "����ȫ");
    
    DEBUG_Printf("��ȫ��ؼ��ɲ������\r\n");
    IwdgTest_PrintTestResult("��ȫ��ؼ��ɲ���", 1);
    return 1;
}

/**
  * @brief  ����5��״̬ת������
  */
static uint8_t IwdgTest_StateTransition(void)
{
    IwdgTest_PrintTestHeader("״̬ת������");
    
    // ������ͣ
    IwdgControl_Suspend();
    if(IwdgControl_GetState() != IWDG_STATE_SUSPENDED) {
        DEBUG_Printf("��ͣ״̬����\r\n");
        IwdgTest_PrintTestResult("״̬ת������", 0);
        return 0;
    }
    
    // ���Իָ�
    IwdgControl_Resume();
    if(IwdgControl_GetState() != IWDG_STATE_ENABLED) {
        DEBUG_Printf("�ָ�״̬����\r\n");
        IwdgTest_PrintTestResult("״̬ת������", 0);
        return 0;
    }
    
    DEBUG_Printf("״̬ת������\r\n");
    IwdgTest_PrintTestResult("״̬ת������", 1);
    return 1;
}

/**
  * @brief  ����6��ͳ�ƺͼ�ز���
  */
static uint8_t IwdgTest_StatisticsAndMonitoring(void)
{
    IwdgTest_PrintTestHeader("ͳ�ƺͼ�ز���");
    
    // ��ӡͳ����Ϣ
    IwdgControl_PrintStatistics();
    
    // ��ӡ��ǰ״̬
    IwdgControl_PrintStatus();
    
    // ��ӡ��λԭ��
    IwdgControl_PrintResetReason();
    
    DEBUG_Printf("ͳ�ƺͼ�ع�������\r\n");
    IwdgTest_PrintTestResult("ͳ�ƺͼ�ز���", 1);
    return 1;
}

/**
  * @brief  ����7�����ø��Ĳ���
  */
static uint8_t IwdgTest_ConfigurationChange(void)
{
    IwdgTest_PrintTestHeader("���ø��Ĳ���");
    
    // �������ø���
    IwdgControl_Config(5000, 1000);  // 5�볬ʱ��1��ι�����
    
    DEBUG_Printf("���ø��Ĳ������\r\n");
    IwdgTest_PrintTestResult("���ø��Ĳ���", 1);
    return 1;
}

/**
  * @brief  ����8������ģʽ����
  */
static uint8_t IwdgTest_DebugMode(void)
{
    IwdgTest_PrintTestHeader("����ģʽ����");
    
    // ���õ���ģʽ
    IwdgControl_EnableDebugMode();
    if(!IwdgControl_IsDebugMode()) {
        DEBUG_Printf("����ģʽ����ʧ��\r\n");
        IwdgTest_PrintTestResult("����ģʽ����", 0);
        return 0;
    }
    
    // ���Ե������
    IwdgControl_Feed();  // Ӧ���е������
    
    // ���õ���ģʽ
    IwdgControl_DisableDebugMode();
    if(IwdgControl_IsDebugMode()) {
        DEBUG_Printf("����ģʽ����ʧ��\r\n");
        IwdgTest_PrintTestResult("����ģʽ����", 0);
        return 0;
    }
    
    DEBUG_Printf("����ģʽ��������\r\n");
    IwdgTest_PrintTestResult("����ģʽ����", 1);
    return 1;
}
