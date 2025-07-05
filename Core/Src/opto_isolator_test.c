#include "opto_isolator_test.h"
#include "gpio_control.h"
#include "usart.h"
#include "main.h"

/************************************************************
 * ��������·����ģ��
 * ר�����ڲ��Ժ����K1_EN��K2_EN��K3_EN�źŵĹ�������·����״̬
 * �������ź�״̬��⡢��ѹ���������ż�⡢ͳ�Ʒ����ȹ���
 ************************************************************/

// ����ͳ�����ݽṹ
typedef struct {
    uint32_t high_count;        // �ߵ�ƽ����
    uint32_t low_count;         // �͵�ƽ����
    uint32_t transition_count;  // ��ƽ�������
    uint32_t test_duration;     // ���Գ���ʱ��(ms)
    uint8_t last_state;         // ��һ��״̬
    uint32_t last_change_time;  // �ϴ�״̬�仯ʱ��
} OptoTestStats_t;

// ����ͨ���Ĳ���ͳ������
static OptoTestStats_t optoStats[3];
static uint32_t test_start_time;
static uint8_t test_running = 0;

/**
  * @brief  ��ʼ����������·����
  * @retval ��
  */
void OptoTest_Init(void)
{
    // ��ʼ������ͳ������
    for(uint8_t i = 0; i < 3; i++) {
        optoStats[i].high_count = 0;
        optoStats[i].low_count = 0;
        optoStats[i].transition_count = 0;
        optoStats[i].test_duration = 0;
        optoStats[i].last_state = 0xFF; // ��ʼ״̬��Ϊδ֪
        optoStats[i].last_change_time = 0;
    }
    
    test_start_time = HAL_GetTick();
    test_running = 0;
    
    DEBUG_Printf("=== ��������·����ģ���ʼ����� ===\r\n");
    DEBUG_Printf("���Թ��ܣ�\r\n");
    DEBUG_Printf("1. ʵʱ���K1_EN��K2_EN��K3_EN�ź�״̬\r\n");
    DEBUG_Printf("2. ͳ�Ƹ�/�͵�ƽռ�Ⱥ��������\r\n");
    DEBUG_Printf("3. ����ź��ȶ��ԺͿ���������\r\n");
    DEBUG_Printf("4. �����ϸ�������Ϣ\r\n");
}

/**
  * @brief  ��ʼ��������·����
  * @param  test_duration_sec: ���Գ���ʱ��(��)
  * @retval ��
  */
void OptoTest_Start(uint16_t test_duration_sec)
{
    if(test_running) {
        DEBUG_Printf("[����] ��������������\r\n");
        return;
    }
    
    // ���ò�������
    for(uint8_t i = 0; i < 3; i++) {
        optoStats[i].high_count = 0;
        optoStats[i].low_count = 0;
        optoStats[i].transition_count = 0;
        optoStats[i].test_duration = test_duration_sec * 1000;
        optoStats[i].last_state = 0xFF;
        optoStats[i].last_change_time = 0;
    }
    
    test_start_time = HAL_GetTick();
    test_running = 1;
    
    DEBUG_Printf("=== ��ʼ��������·���� ===\r\n");
    DEBUG_Printf("����ʱ����%d��\r\n", test_duration_sec);
    DEBUG_Printf("���ڲ����ڼ䣺\r\n");
    DEBUG_Printf("1. ��SW1��GND_WBʩ��/�Ƴ�DC24V\r\n");
    DEBUG_Printf("2. �۲�K1_EN�źű仯\r\n");
    DEBUG_Printf("3. ��������ͨ���Ĺ����·\r\n");
    DEBUG_Printf("��ʼ���...\r\n");
}

/**
  * @brief  ֹͣ��������·����
  * @retval ��
  */
void OptoTest_Stop(void)
{
    if(!test_running) {
        DEBUG_Printf("[��ʾ] ����δ������\r\n");
        return;
    }
    
    test_running = 0;
    uint32_t actual_duration = HAL_GetTick() - test_start_time;
    
    DEBUG_Printf("=== ��������·���Խ��� ===\r\n");
    DEBUG_Printf("ʵ�ʲ���ʱ����%lu����\r\n", actual_duration);
    
    // ������Խ��
    OptoTest_PrintResults();
}

/**
  * @brief  ��������·������ѭ��������ѭ���е��ã�
  * @retval ��
  */
void OptoTest_Process(void)
{
    if(!test_running) return;
    
    uint32_t current_time = HAL_GetTick();
    
    // �������Ƿ�Ӧ�ý���
    if((current_time - test_start_time) >= optoStats[0].test_duration) {
        OptoTest_Stop();
        return;
    }
    
    // ��ȡ����ͨ����K_EN�ź�״̬
    uint8_t k_en_states[3] = {
        GPIO_ReadK1_EN(),
        GPIO_ReadK2_EN(),
        GPIO_ReadK3_EN()
    };
    
    // ����ͳ������
    for(uint8_t i = 0; i < 3; i++) {
        uint8_t current_state = k_en_states[i];
        
        // ͳ�Ƹ�/�͵�ƽ
        if(current_state == GPIO_PIN_SET) {
            optoStats[i].high_count++;
        } else {
            optoStats[i].low_count++;
        }
        
        // ���״̬����
        if(optoStats[i].last_state != 0xFF && optoStats[i].last_state != current_state) {
            optoStats[i].transition_count++;
            uint32_t time_diff = current_time - optoStats[i].last_change_time;
            DEBUG_Printf("[K%d_EN] ״̬�仯: %s -> %s (���:%lums)\r\n", 
                i+1, 
                optoStats[i].last_state ? "��" : "��",
                current_state ? "��" : "��",
                time_diff);
            optoStats[i].last_change_time = current_time;
        }
        
        optoStats[i].last_state = current_state;
    }
    
    // ÿ5�����һ��ʵʱ״̬
    static uint32_t last_status_time = 0;
    if((current_time - last_status_time) >= 5000) {
        OptoTest_PrintRealTimeStatus();
        last_status_time = current_time;
    }
}

/**
  * @brief  ���ʵʱ״̬��Ϣ
  * @retval ��
  */
void OptoTest_PrintRealTimeStatus(void)
{
    uint32_t elapsed_time = (HAL_GetTick() - test_start_time) / 1000;
    
    DEBUG_Printf("=== ʵʱ״̬ (������%lus) ===\r\n", elapsed_time);
    DEBUG_Printf("K1_EN=%d, K2_EN=%d, K3_EN=%d\r\n", 
        GPIO_ReadK1_EN(), GPIO_ReadK2_EN(), GPIO_ReadK3_EN());
    
    // ͬʱ��ȡ״̬�����ź�
    DEBUG_Printf("״̬����: K1_1=%d K1_2=%d SW1=%d | K2_1=%d K2_2=%d SW2=%d | K3_1=%d K3_2=%d SW3=%d\r\n",
        GPIO_ReadK1_1_STA(), GPIO_ReadK1_2_STA(), GPIO_ReadSW1_STA(),
        GPIO_ReadK2_1_STA(), GPIO_ReadK2_2_STA(), GPIO_ReadSW2_STA(),
        GPIO_ReadK3_1_STA(), GPIO_ReadK3_2_STA(), GPIO_ReadSW3_STA());
    
    // �������ͳ��
    for(uint8_t i = 0; i < 3; i++) {
        uint32_t total_samples = optoStats[i].high_count + optoStats[i].low_count;
        if(total_samples > 0) {
            float high_percentage = (float)optoStats[i].high_count / total_samples * 100.0f;
            DEBUG_Printf("K%d_EN: �ߵ�ƽ%.1f%% ����%lu��\r\n", 
                i+1, high_percentage, optoStats[i].transition_count);
        }
    }
}

/**
  * @brief  �����ϸ���Խ��
  * @retval ��
  */
void OptoTest_PrintResults(void)
{
    DEBUG_Printf("=== ��������·���Խ�� ===\r\n");
    
    for(uint8_t i = 0; i < 3; i++) {
        uint32_t total_samples = optoStats[i].high_count + optoStats[i].low_count;
        if(total_samples == 0) continue;
        
        float high_percentage = (float)optoStats[i].high_count / total_samples * 100.0f;
        float low_percentage = (float)optoStats[i].low_count / total_samples * 100.0f;
        
        DEBUG_Printf("--- ͨ��%d (K%d_EN) ---\r\n", i+1, i+1);
        DEBUG_Printf("��������: %lu\r\n", total_samples);
        DEBUG_Printf("�ߵ�ƽ: %lu�� (%.1f%%)\r\n", optoStats[i].high_count, high_percentage);
        DEBUG_Printf("�͵�ƽ: %lu�� (%.1f%%)\r\n", optoStats[i].low_count, low_percentage);
        DEBUG_Printf("״̬����: %lu��\r\n", optoStats[i].transition_count);
        
        // �������
        if(optoStats[i].transition_count == 0) {
            if(high_percentage > 95.0f) {
                DEBUG_Printf("���: �źų���Ϊ�ߵ�ƽ - �������δ��ͨ\r\n");
                DEBUG_Printf("����: ���24V��Դ������LED��������ѹ����ֵ\r\n");
            } else if(low_percentage > 95.0f) {
                DEBUG_Printf("���: �źų���Ϊ�͵�ƽ - �������һֱ��ͨ\r\n");
                DEBUG_Printf("����: ����������ˡ��������衢EMI����\r\n");
            }
        } else if(optoStats[i].transition_count > 100) {
            DEBUG_Printf("���: �ź�����Ƶ�� - ���ܴ���EMI���Ż�Ӵ�����\r\n");
            DEBUG_Printf("����: ����������Ρ��ӵء��˲�����\r\n");
        } else {
            DEBUG_Printf("���: �źŹ������� - ������Ӧ����\r\n");
        }
    }
    
    // ����Ľ�����
    DEBUG_Printf("\n=== �����·�Ż����� ===\r\n");
    DEBUG_Printf("1. ��������: ȷ��LED������5-15mA��Χ��\r\n");
    DEBUG_Printf("2. ��������: ����ʹ��10k�����ṩ���ٹضϺͿ�����\r\n");
    DEBUG_Printf("3. �˲�����: ������˲���100nF�մɵ�������EMI\r\n");
    DEBUG_Printf("4. TVS����: ����������TVS�����ܱ���\r\n");
    DEBUG_Printf("5. CTR����: ���ʱʹ�ñ���CTRֵ(��25%)\r\n");
    DEBUG_Printf("6. ��������: �����봫��ʱʹ�����ε���\r\n");
}

/**
  * @brief  �ֶ���������״̬���
  * @retval ��
  */
void OptoTest_ManualCheck(void)
{
    DEBUG_Printf("=== �ֶ�״̬��� ===\r\n");
    
    // ������ȡ10�Σ�����ȶ���
    uint8_t readings[3][10];
    for(uint8_t i = 0; i < 10; i++) {
        readings[0][i] = GPIO_ReadK1_EN();
        readings[1][i] = GPIO_ReadK2_EN();
        readings[2][i] = GPIO_ReadK3_EN();
        HAL_Delay(10); // ���10ms
    }
    
    // �����ȶ���
    for(uint8_t ch = 0; ch < 3; ch++) {
        uint8_t stable = 1;
        uint8_t first_value = readings[ch][0];
        
        for(uint8_t i = 1; i < 10; i++) {
            if(readings[ch][i] != first_value) {
                stable = 0;
                break;
            }
        }
        
        DEBUG_Printf("K%d_EN: ��ǰֵ=%d, �ȶ���=%s\r\n", 
            ch+1, first_value, stable ? "�ȶ�" : "���ȶ�");
            
        if(!stable) {
            DEBUG_Printf("  ��ϸ����: ");
            for(uint8_t i = 0; i < 10; i++) {
                DEBUG_Printf("%d ", readings[ch][i]);
            }
            DEBUG_Printf("\r\n");
        }
    }
    
    // ������״̬
    DEBUG_Printf("��ǰ״̬����:\r\n");
    DEBUG_Printf("  K1: STA1=%d STA2=%d SW1=%d\r\n", 
        GPIO_ReadK1_1_STA(), GPIO_ReadK1_2_STA(), GPIO_ReadSW1_STA());
    DEBUG_Printf("  K2: STA1=%d STA2=%d SW2=%d\r\n", 
        GPIO_ReadK2_1_STA(), GPIO_ReadK2_2_STA(), GPIO_ReadSW2_STA());
    DEBUG_Printf("  K3: STA1=%d STA2=%d SW3=%d\r\n", 
        GPIO_ReadK3_1_STA(), GPIO_ReadK3_2_STA(), GPIO_ReadSW3_STA());
}

/**
  * @brief  ��ȡ��ǰ����״̬
  * @retval 1:���������� 0:����ֹͣ
  */
uint8_t OptoTest_IsRunning(void)
{
    return test_running;
}

/**
  * @brief  ��ȡ���Ծ���ʱ��
  * @retval ���Ծ���ʱ��(����)
  */
uint32_t OptoTest_GetElapsedTime(void)
{
    return HAL_GetTick() - test_start_time;
} 