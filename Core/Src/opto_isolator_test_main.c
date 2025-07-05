#include "opto_isolator_test.h"
#include "gpio_control.h"
#include "usart.h"
#include "main.h"
#include <string.h>

/************************************************************
 * ��������·����������
 * �ṩ�����Ĳ������̺��û���������
 * ����ͨ������������Ʋ��Թ���
 ************************************************************/

// ���������
#define CMD_HELP        "help"
#define CMD_INIT        "init"
#define CMD_START       "start"
#define CMD_STOP        "stop"
#define CMD_STATUS      "status"
#define CMD_CHECK       "check"
#define CMD_AUTOTEST    "autotest"

// ���ڽ��ջ�����
static char rx_buffer[64];
static uint8_t rx_index = 0;

/**
  * @brief  ��������·�����������ʼ��
  * @retval ��
  */
void OptoTestMain_Init(void)
{
    // ��ʼ�����ڽ��ջ�����
    memset(rx_buffer, 0, sizeof(rx_buffer));
    rx_index = 0;
    
    // �����ӭ��Ϣ
    DEBUG_Printf("\r\n");
    DEBUG_Printf("****************************************************************\r\n");
    DEBUG_Printf("*           ��������·�������ϵͳ V1.0                       *\r\n");
    DEBUG_Printf("*                                                              *\r\n");
    DEBUG_Printf("* ���ܣ����K1_EN��K2_EN��K3_EN��������·����״̬              *\r\n");
    DEBUG_Printf("* ��;�����SW1+GND_WBʩ��DC24V��PB9�޷��õ͵�ƽ������           *\r\n");
    DEBUG_Printf("*                                                              *\r\n");
    DEBUG_Printf("* ֧�����                                                    *\r\n");
    DEBUG_Printf("*   help     - ��ʾ������Ϣ                                     *\r\n");
    DEBUG_Printf("*   init     - ��ʼ������ģ��                                   *\r\n");
    DEBUG_Printf("*   start <��> - ��ʼ���ԣ�ָ��ʱ����                            *\r\n");
    DEBUG_Printf("*   stop     - ֹͣ����                                         *\r\n");
    DEBUG_Printf("*   status   - ��ʾʵʱ״̬                                     *\r\n");
    DEBUG_Printf("*   check    - �ֶ����                                         *\r\n");
    DEBUG_Printf("*   autotest - �Զ�����������                                   *\r\n");
    DEBUG_Printf("*                                                              *\r\n");
    DEBUG_Printf("****************************************************************\r\n");
    DEBUG_Printf("�������������help�鿴��ϸ��������\r\n");
}

/**
  * @brief  �����ڽ��յ����ַ�
  * @param  ch: ���յ����ַ�
  * @retval ��
  */
void OptoTestMain_ProcessChar(char ch)
{
    if(ch == '\r' || ch == '\n') {
        // �س����У���������
        if(rx_index > 0) {
            rx_buffer[rx_index] = '\0';
            OptoTestMain_ProcessCommand(rx_buffer);
            rx_index = 0;
            memset(rx_buffer, 0, sizeof(rx_buffer));
        }
        DEBUG_Printf("> ");
    } else if(ch == '\b' || ch == 127) {
        // �˸��
        if(rx_index > 0) {
            rx_index--;
            rx_buffer[rx_index] = '\0';
            DEBUG_Printf("\b \b");
        }
    } else if(ch >= ' ' && ch <= '~') {
        // ����ʾ�ַ�
        if(rx_index < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_index++] = ch;
            DEBUG_Printf("%c", ch);
        }
    }
}

/**
  * @brief  �����û����������
  * @param  cmd: �����ַ���
  * @retval ��
  */
void OptoTestMain_ProcessCommand(char* cmd)
{
    DEBUG_Printf("\r\n");
    
    if(strcmp(cmd, CMD_HELP) == 0) {
        OptoTestMain_ShowHelp();
    }
    else if(strcmp(cmd, CMD_INIT) == 0) {
        OptoTest_Init();
    }
    else if(strncmp(cmd, CMD_START, 5) == 0) {
        // ����ʱ�����
        char* param = strchr(cmd, ' ');
        if(param != NULL) {
            int duration = atoi(param + 1);
            if(duration > 0 && duration <= 3600) {
                OptoTest_Start(duration);
            } else {
                DEBUG_Printf("[����] ����ʱ��������1-3600�뷶Χ��\r\n");
            }
        } else {
            DEBUG_Printf("[����] ��ָ������ʱ�������磺start 60\r\n");
        }
    }
    else if(strcmp(cmd, CMD_STOP) == 0) {
        OptoTest_Stop();
    }
    else if(strcmp(cmd, CMD_STATUS) == 0) {
        OptoTest_PrintRealTimeStatus();
    }
    else if(strcmp(cmd, CMD_CHECK) == 0) {
        OptoTest_ManualCheck();
    }
    else if(strcmp(cmd, CMD_AUTOTEST) == 0) {
        OptoTestMain_AutoTest();
    }
    else {
        DEBUG_Printf("[����] δ֪���%s\r\n", cmd);
        DEBUG_Printf("���� help �鿴��������\r\n");
    }
}

/**
  * @brief  ��ʾ��ϸ������Ϣ
  * @retval ��
  */
void OptoTestMain_ShowHelp(void)
{
    DEBUG_Printf("=== ��������·����ϵͳ���� ===\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("����������\r\n");
    DEBUG_Printf("  ��SW1��GND_WB����DC24V��PB9û����Ϊ�͵�ƽ\r\n");
    DEBUG_Printf("  ϵͳ����K1_EN=0ʱ������һͨ����������������������\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("������⣺\r\n");
    DEBUG_Printf("  init          - ��ʼ������ģ�飬��������ͳ������\r\n");
    DEBUG_Printf("  start <��>    - ��ʼ�������ԣ�����źű仯\r\n");
    DEBUG_Printf("                  ���磺start 60 ������60�룩\r\n");
    DEBUG_Printf("  stop          - ֹͣ��ǰ���Բ�������\r\n");
    DEBUG_Printf("  status        - ��ʾ��ǰ�����ź�״̬\r\n");
    DEBUG_Printf("  check         - �ֶ�����ź��ȶ��ԣ�������ȡ10�Σ�\r\n");
    DEBUG_Printf("  autotest      - �Զ����������̣��Ƽ�ʹ�ã�\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("�������̽��飺\r\n");
    DEBUG_Printf("  1. ���� init ��ʼ��\r\n");
    DEBUG_Printf("  2. ���� check �鿴��ǰ״̬\r\n");
    DEBUG_Printf("  3. ���� start 30 ��ʼ30�����\r\n");
    DEBUG_Printf("  4. �ڲ����ڼ��SW1+GND_WBʩ��/�Ƴ�DC24V\r\n");
    DEBUG_Printf("  5. �۲�K1_EN�źű仯����Ͻ��\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("���ܵĹ���ԭ��\r\n");
    DEBUG_Printf("  ? ����LED�������㣨����������裩\r\n");
    DEBUG_Printf("  ? 24V��Դ���⣨��ѹ/�Ʋ�/�ӵأ�\r\n");
    DEBUG_Printf("  ? ����CTR���ͣ������ϻ������������\r\n");
    DEBUG_Printf("  ? EMI���ţ�������/������/���˲���\r\n");
    DEBUG_Printf("  ? Ӳ�����Ӵ��󣨼��ԭ��ͼ��\r\n");
}

/**
  * @brief  �Զ�����������
  * @retval ��
  */
void OptoTestMain_AutoTest(void)
{
    DEBUG_Printf("=== ��ʼ�Զ�����������·���� ===\r\n");
    DEBUG_Printf("\r\n");
    
    // ��һ������ʼ��
    DEBUG_Printf("��1������ʼ������ģ��...\r\n");
    OptoTest_Init();
    HAL_Delay(1000);
    
    // �ڶ�������ʼ״̬���
    DEBUG_Printf("��2��������ʼ״̬...\r\n");
    OptoTest_ManualCheck();
    HAL_Delay(2000);
    
    // ���������û�������ʾ
    DEBUG_Printf("��3�����밴���²��������\r\n");
    DEBUG_Printf("  ׼��������ȷ��SW1��GND_WB֮��δʩ�ӵ�ѹ\r\n");
    DEBUG_Printf("  ���ڿ�ʼ60���������...\r\n");
    DEBUG_Printf("  \r\n");
    DEBUG_Printf("  ���ڲ����ڼ�ִ�����²�����\r\n");
    DEBUG_Printf("  0-10�룺  �����޵�ѹ״̬���۲��׼״̬\r\n");
    DEBUG_Printf("  10-30�룺 ʩ��DC24V��SW1��GND_WB\r\n");
    DEBUG_Printf("  30-50�룺 �Ƴ�DC24V��ѹ\r\n");
    DEBUG_Printf("  50-60�룺 �ٴ�ʩ��DC24V\r\n");
    DEBUG_Printf("  \r\n");
    DEBUG_Printf("  ϵͳ���Զ���¼����״̬�仯...\r\n");
    
    // ���Ĳ�����ʼ����
    DEBUG_Printf("��4������ʼ60���Զ�����...\r\n");
    OptoTest_Start(60);
    
    // �ڲ����ڼ��ṩʵʱָ��
    uint32_t test_start = HAL_GetTick();
    uint32_t last_prompt = 0;
    
    while(OptoTest_IsRunning()) {
        uint32_t elapsed = (HAL_GetTick() - test_start) / 1000;
        
        // ����ʱ�����������ʾ
        if(elapsed == 10 && (HAL_GetTick() - last_prompt) > 1000) {
            DEBUG_Printf("\r\n[������ʾ] ������ʩ��DC24V��SW1��GND_WB\r\n");
            last_prompt = HAL_GetTick();
        }
        else if(elapsed == 30 && (HAL_GetTick() - last_prompt) > 1000) {
            DEBUG_Printf("\r\n[������ʾ] �������Ƴ�DC24V��ѹ\r\n");
            last_prompt = HAL_GetTick();
        }
        else if(elapsed == 50 && (HAL_GetTick() - last_prompt) > 1000) {
            DEBUG_Printf("\r\n[������ʾ] �������ٴ�ʩ��DC24V\r\n");
            last_prompt = HAL_GetTick();
        }
        
        // ���в����߼�
        OptoTest_Process();
        HAL_Delay(10);
    }
    
    // ���岽�����ռ��
    DEBUG_Printf("\r\n��5��������״̬���...\r\n");
    HAL_Delay(1000);
    OptoTest_ManualCheck();
    
    // ���������������
    DEBUG_Printf("\r\n=== �Զ���������� ===\r\n");
    DEBUG_Printf("�������ϲ��Խ���������ԣ�\r\n");
    DEBUG_Printf("1. �鿴��Ͻ��飬ȷ������ԭ��\r\n");
    DEBUG_Printf("2. ���ݽ����޸�Ӳ����·\r\n");
    DEBUG_Printf("3. �������в�����֤�޸�Ч��\r\n");
    DEBUG_Printf("4. �������ʣ�����ϵ����֧��\r\n");
}

/**
  * @brief  ��������·������ѭ��
  * @retval ��
  */
void OptoTestMain_Process(void)
{
    // ���в����߼�
    OptoTest_Process();
    
    // ����Ƿ����µĴ�������
    // ע�⣺������Ҫ�������Ĵ���ʵ�ַ�ʽ����
    // ���ʹ���жϷ�ʽ���գ��������ж��е���OptoTestMain_ProcessChar
    
    // ʾ������ѯ��ʽ����Ҫ����ʵ�����������
    /*
    if(HAL_UART_Receive(&huart3, (uint8_t*)&ch, 1, 0) == HAL_OK) {
        OptoTestMain_ProcessChar(ch);
    }
    */
}

/**
  * @brief  ����������·���������
  * @retval ��
  */
void OptoTestMain_CheckDesign(void)
{
    DEBUG_Printf("=== ��������·��Ƽ�� ===\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("����EL817����ĵ�����Ʋ�����\r\n");
    DEBUG_Printf("  �����ѹ��DC24V\r\n");
    DEBUG_Printf("  LED�����ѹ��Լ1.2V\r\n");
    DEBUG_Printf("  LED����������Ƽ�10mA\r\n");
    DEBUG_Printf("  �������裺R1 = (24V - 1.2V) / 10mA = 2.28k��\r\n");
    DEBUG_Printf("  ʵ��ѡ��2.2k������׼ֵ��\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("  CTR����������ȣ���50%~600%������ֵ100%��\r\n");
    DEBUG_Printf("  ���缫������10mA �� 100% = 10mA\r\n");
    DEBUG_Printf("  �����ѹ��VCC - Ic �� Rc = 3.3V - 10mA �� ��������\r\n");
    DEBUG_Printf("  �������裺����10k��\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("�������������\r\n");
    DEBUG_Printf("  1. ���R1���� �� LED�������� �� �����ͨ\r\n");
    DEBUG_Printf("  2. ���24V��Դ��ѹ���� �� LED��������\r\n");
    DEBUG_Printf("  3. ���CTR���� �� ���缫�������� �� �����ѹ����\r\n");
    DEBUG_Printf("  4. ������������� �� ������� �� ״̬��ȷ��\r\n");
    DEBUG_Printf("  5. ���EMI���� �� �źŲ��ȶ� �� �󴥷�\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("�Ƽ��ĸĽ���ʩ��\r\n");
    DEBUG_Printf("  ? ʹ��2.2k����������ȷ��10mA LED����\r\n");
    DEBUG_Printf("  ? ʹ��10k�����������ṩ�ȶ��ĸߵ�ƽ\r\n");
    DEBUG_Printf("  ? ����100nF�˲���������EMI\r\n");
    DEBUG_Printf("  ? ���TVS�����ܱ��������\r\n");
    DEBUG_Printf("  ? ʹ�����ε��¼��ٳ��ߴ������\r\n");
} 