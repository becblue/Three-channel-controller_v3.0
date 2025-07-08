/*
 * ��������·���Լ���ʾ��
 * 
 * ���ļ�չʾ�����main.c�м��ɹ�������·����ģ��
 * ���SW1+GND_WBʩ��DC24V��PB9�޷��õ͵�ƽ������
 */

#include "main.h"
#include "opto_isolator_test_main.h"
#include "usart.h"

// ʾ���������main.c�м��ɹ�������·����ģ��

int main(void)
{
    // ϵͳ��ʼ��
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART3_UART_Init();
    
    // ��ʼ����������·����ģ��
    OptoTestMain_Init();
    
    // ��ѭ��
    while (1)
    {
        // ���й�������·������ѭ��
        OptoTestMain_Process();
        
        // ����ҵ���߼�...
        
        HAL_Delay(1);
    }
}

// ���ڽ����жϴ����������ʹ���жϽ��գ�
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        static uint8_t rx_char;
        
        // ��ȡ���յ����ַ�
        if (HAL_UART_Receive(huart, &rx_char, 1, 0) == HAL_OK)
        {
            // ���ݸ��������ģ�鴦��
            OptoTestMain_ProcessChar(rx_char);
        }
        
        // ������������
        HAL_UART_Receive_IT(huart, &rx_char, 1);
    }
}

/*
 * ʹ�ò��裺
 * 
 * 1. ���벢��¼����
 * 2. ���Ӵ��ڹ��ߣ�115200�����ʣ�
 * 3. ����������в��ԣ�
 *    - ���� "autotest" �����Զ�������
 *    - ���� "help" �鿴��������
 *    - ���� "check" �����ֶ����
 * 
 * 4. ������ʾ������
 *    - �ڲ����ڼ��SW1��GND_WBʩ��/�Ƴ�DC24V
 *    - �۲�K1_EN�źű仯
 *    - �鿴��Ͻ���ͽ���
 * 
 * 5. ������Ͻ���޸�Ӳ�����⣺
 *    - �����������ֵ���Ƽ�2.2k����
 *    - ���24V��Դ��ѹ
 *    - ����������CTR����
 *    - ����˲���������EMI
 */ 