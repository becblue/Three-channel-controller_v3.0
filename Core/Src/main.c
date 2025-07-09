/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *�˴���������ʱּ�ڲ���cubemx�ı����Ƿ���cursoe�Լ�keil����
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>         // ������׼��������⣬����printf����
#include "gpio.h"          // ����GPIO����ģ��ͷ�ļ�������GPIO�ײ����
#include "gpio_control.h"  // ����GPIO����ģ��
#include "relay_control.h"
#include "temperature_monitor.h"
#include "oled_display.h"
#include "system_control.h"
#include "usart.h"
#include "iwdg_control.h"  // ����IWDG���Ź�����ģ��
#include "iwdg.h"          // ����IWDG��������
#include "log_system.h"    // ������־ϵͳģ��
#include "w25q128_driver.h" // ����W25Q128 Flash����ģ��
#include <string.h>        // �����ַ�����������
#include "safety_monitor.h" // ������ȫ���ģ��
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// �ⲿ������������Щ�����������ļ��ж��壩
extern volatile uint8_t key1_pressed;
extern volatile uint8_t key2_pressed;
extern volatile uint32_t key1_press_start_time;
extern volatile uint32_t key2_press_start_time;
extern volatile uint8_t key1_long_press_triggered;
extern volatile uint8_t key2_long_press_triggered;
extern volatile uint32_t fan_pulse_count;

// ��������־���״̬����
typedef struct {
    uint8_t is_outputting;      // �Ƿ����������־
    uint32_t current_index;     // ��ǰ�������
    uint32_t total_logs;        // ����־����
    uint32_t last_output_time;  // �ϴ����ʱ��
} LogOutputState_t;

static LogOutputState_t log_output_state = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  �ض���printf������USART3�����ڵ�����Ϣ���
  * @param  ch: Ҫ���͵��ַ�
  * @param  f: �ļ�ָ�루δʹ�ã�
  * @retval ���͵��ַ�
  */
// int fputc(int ch, FILE *f)
// {
//   // ͨ��USART3���͵����ַ����ȴ��������
//   HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
//   return ch;  // ���ط��͵��ַ�
// }



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_IWDG_Init();
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  
  // ��ʱ200ms�ȴ�ϵͳ�ȶ�
  HAL_Delay(200);
  
  // ���ڳ�ʼ������
  DEBUG_Printf("\r\n=== ��ͨ���л������ϵͳ���� ===\r\n");
  DEBUG_Printf("MCU: STM32F103RCT6\r\n");
  DEBUG_Printf("ϵͳʱ��: %luMHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
  DEBUG_Printf("����ʱ��: %s %s\r\n", __DATE__, __TIME__);
  DEBUG_Printf("USART3���Դ��ڳ�ʼ�����\r\n");

  // ��ʼ��������ģ��
  DEBUG_Printf("��ʼ��ʼ��������ģ��...\r\n");
  
  // ��������PWM���
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  DEBUG_Printf("����PWM����������\r\n");
  
  // OLED��ʾģ���ʼ��
  if(OLED_TestConnection()) {
    DEBUG_Printf("OLED������������ʼ��OLEDģ��\r\n");
    OLED_Init();
  } else {
    DEBUG_Printf("OLED����ʧ�ܣ�\r\n");
  }
  
  // �¶ȼ��ģ���ʼ��
  TemperatureMonitor_Init();
  DEBUG_Printf("�¶ȼ��ģ���ʼ�����\r\n");
  
  // �̵�������ģ���ʼ��
  RelayControl_Init();
  DEBUG_Printf("�̵�������ģ���ʼ�����\r\n");
  
  // ϵͳ����ģ���ʼ������ʼ2��LOGO��ʾ��
  SystemControl_Init();
  DEBUG_Printf("ϵͳ����ģ���ʼ����ɣ���ʼִ���Լ�����\r\n");
  
  // IWDG���Ź�����ģ���ʼ��
  IwdgControl_Init();
  DEBUG_Printf("IWDG���Ź�����ģ���ʼ�����\r\n");
  
  // ��ӡ��λԭ��ͳ����Ϣ���ڸ�λԭ��������ɺ�
  IwdgControl_PrintResetReason();
  
  // ͳһ���ϵͳ����ԭ�򲢼�¼����־ϵͳ
  DEBUG_Printf("=== ͳһ���ϵͳ����ԭ�� ===\r\n");
  uint8_t reset_reason_detected = 0;
  char reset_reason_msg[48] = {0};
  IwdgResetReason_t iwdg_reset_reason = IWDG_RESET_UNKNOWN;
  static uint32_t reset_count = 0;  // �����������򻯰棬�洢�ھ�̬�����У�
  
  // ��⿴�Ź���λ
  if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
    DEBUG_Printf("��⵽���Ź���λ��ϵͳ���ڻָ�...\r\n");
    strcpy(reset_reason_msg, "Watchdog reset detected");
    reset_reason_detected = 1;
    iwdg_reset_reason = IWDG_RESET_WATCHDOG;
    reset_count++;  // ���Ź���λ��������
  }
  // ��ⴰ�ڿ��Ź���λ
  else if(__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
    DEBUG_Printf("��⵽���ڿ��Ź���λ\r\n");
    strcpy(reset_reason_msg, "Window watchdog reset");
    reset_reason_detected = 6;
    iwdg_reset_reason = IWDG_RESET_WINDOW_WATCHDOG;
    reset_count++;
  }
  // ��������λ
  else if(__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
    DEBUG_Printf("��⵽�����λ\r\n");
    strcpy(reset_reason_msg, "Software reset");
    reset_reason_detected = 2;
    iwdg_reset_reason = IWDG_RESET_SOFTWARE;
  }
  // ����ϵ縴λ
  else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
    DEBUG_Printf("��⵽�ϵ縴λ\r\n");
    strcpy(reset_reason_msg, "Power on reset");
    reset_reason_detected = 3;
    iwdg_reset_reason = IWDG_RESET_POWER_ON;
  }
  // ����ⲿ��λ/���Ÿ�λ
  else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) {
    DEBUG_Printf("��⵽���Ÿ�λ\r\n");
    strcpy(reset_reason_msg, "Pin reset");
    reset_reason_detected = 4;
    iwdg_reset_reason = IWDG_RESET_PIN;
  }
  else {
    DEBUG_Printf("��������\r\n");
    strcpy(reset_reason_msg, "Normal startup");
    reset_reason_detected = 5;
    iwdg_reset_reason = IWDG_RESET_NONE;
  }
  
  // ����λԭ��ͼ������ݸ�IWDGģ��
  IwdgControl_SetResetReason(iwdg_reset_reason);
  IwdgControl_SetResetCount(reset_count);
  
  // �����λ��־��ͳһ�ڴ˴�����������ظ���⣩
  __HAL_RCC_CLEAR_RESET_FLAGS();
  
  DEBUG_Printf("��λԭ�����: %d, IWDGԭ��: %d, ��λ����: %lu\r\n", 
               reset_reason_detected, iwdg_reset_reason, reset_count);
  
  // Flash���Ӳ��ԣ������ã�
  DEBUG_Printf("��ʼFlash���Ӳ���...\r\n");
  if(W25Q128_TestConnection() == W25Q128_OK) {
    DEBUG_Printf("Flash���Ӳ���ͨ��\r\n");
  } else {
    DEBUG_Printf("Flash���Ӳ���ʧ��\r\n");
  }
  
  // ��־ϵͳģ���ʼ��
  if(LogSystem_Init() == LOG_SYSTEM_OK) {
    DEBUG_Printf("��־ϵͳ��ʼ�����\r\n");
    
    // ��¼����ԭ����־ϵͳ������֧�����и�λ���ͣ�
    char log_msg[48];
    switch(reset_reason_detected) {
      case 1:  // ���Ź���λ
        snprintf(log_msg, sizeof(log_msg), "���Ź���λ(��%lu��)", reset_count);
        LOG_WATCHDOG_RESET();
        // ͬʱ��¼��ϸ��Ϣ
        LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_WATCHDOG_RESET, log_msg);
        break;
        
      case 6:  // ���ڿ��Ź���λ
        snprintf(log_msg, sizeof(log_msg), "���ڿ��Ź���λ(��%lu��)", reset_count);
        LOG_WATCHDOG_RESET();
        LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_WATCHDOG_RESET, log_msg);
        break;
        
      case 2:  // �����λ
        LOG_SOFTWARE_RESET("���������λ");
        break;
        
      case 3:  // �ϵ縴λ
        LOG_POWER_ON_RESET();
        break;
        
      case 4:  // ���Ÿ�λ
        LOG_SYSTEM_RESTART("���Ÿ�λ(����λ��ť)");
        break;
        
      default:  // ��������
        LOG_SYSTEM_START();
        break;
    }
  } else {
    DEBUG_Printf("��־ϵͳ��ʼ��ʧ��\r\n");
    // ��¼��ʼ��ʧ�ܵ�����������޷���¼��Flash
  }
  
  // ע�⣺IWDG����ϵͳ��������״̬���Զ������������ڴ˴�������ʱ
  DEBUG_Printf("ϵͳ��ʼ����ɣ���ʼ״̬��ѭ��\r\n");
  
  // �����λԭ���޸���֤��Ϣ
  DEBUG_Printf("\r\n=== ��λԭ���¼�޸���֤ ===\r\n");
  DEBUG_Printf("? ��λ�����ͳһ��main.c����\r\n");
  DEBUG_Printf("? ������iwdg_control.c���ظ����\r\n");
  DEBUG_Printf("? ȷ����λԭ������־ϵͳ��ʼ�����¼\r\n");
  DEBUG_Printf("? ֧�������ĸ�λ���ͼ�����־��¼\r\n");
  DEBUG_Printf("��֤����: ����ϵͳ��ʹ��KEY1����3��鿴��־\r\n");



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    // ? ���ȴ���DC_CTRL�жϱ�־�����⿴�Ź���λ��
    SafetyMonitor_ProcessDcCtrlInterrupt();
    
    // ϵͳ����ģ��״̬�����������Լ졢��ѭ�����ȵȣ�
    SystemControl_Process();
    
    // PC13����״̬��⣨�����ɿ�ʱִ�й��ܣ�
    static uint8_t key1_was_pressed = 0;
    static uint32_t key1_saved_start_time = 0;  // ���水����ʼʱ��
    
    if (key1_pressed) {
        if (!key1_was_pressed) {
            key1_was_pressed = 1;  // ��ǰ�������������
            key1_saved_start_time = key1_press_start_time;  // ���濪ʼʱ��
        }
        
        // ���������ڼ䣬ֻ��ʾ��ʾ��Ϣ����ִ�й���
        uint32_t current_time = HAL_GetTick();
        uint32_t press_duration = current_time - key1_saved_start_time;
        
        // ��ʾ������ʾ��ÿ�����һ�Σ�
        static uint32_t last_hint_time = 0;
        if (current_time - last_hint_time >= 1000) {
            last_hint_time = current_time;
            
            if (key2_pressed) {
                if (press_duration >= 3000) {
                    DEBUG_Printf("KEY1+KEY2�Ѱ��� %.1f �룬�ɿ���ִ��FLASH��ȡ����\r\n", press_duration/1000.0f);
                } else {
                    DEBUG_Printf("KEY1+KEY2�Ѱ��� %.1f �룬��3��ִ��FLASH��ȡ����\r\n", press_duration/1000.0f);
                }
            } else {
                if (press_duration >= 8000) {
                    DEBUG_Printf("KEY1�Ѱ��� %.1f �룬�ɿ���ִ��FLASH״̬�鿴\r\n", press_duration/1000.0f);
                } else if (press_duration >= 3000) {
                    DEBUG_Printf("KEY1�Ѱ��� %.1f �룬�ɿ��������־����������8��ɲ鿴FLASH״̬\r\n", press_duration/1000.0f);
                } else {
                    DEBUG_Printf("KEY1�Ѱ��� %.1f �룬��3�������־��8��鿴FLASH״̬\r\n", press_duration/1000.0f);
                }
            }
        }
    } else if (key1_was_pressed && !key1_long_press_triggered) {
        // �����ɿ��������ܰ�ѹʱ��ִ�ж�Ӧ����
        uint32_t total_press_duration = HAL_GetTick() - key1_saved_start_time;
        
        // ����Ƿ���˫�����
        static uint8_t key2_was_pressed_with_key1 = 0;
        if (key2_pressed) {
            key2_was_pressed_with_key1 = 1;
        }
        
        if (key2_was_pressed_with_key1 && total_press_duration >= 3000) {
            // KEY1+KEY2��ϣ�FLASH��ȡ����
            key1_long_press_triggered = 1;
            key2_long_press_triggered = 1;
            DEBUG_Printf("\r\n=== KEY1+KEY2��ϣ�%.1f�룩����ʼFLASH��ȡ���� ===\r\n", total_press_duration/1000.0f);
            SystemControl_FlashReadTest();
            DEBUG_Printf("=== FLASH��ȡ������� ===\r\n");
        } else if (!key2_was_pressed_with_key1) {
            // ����KEY1
            if (total_press_duration >= 8000) {
                // KEY1����8�룺FLASH״̬�鿴
                key1_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY1����%.1f�룬�鿴FLASH״̬ ===\r\n", total_press_duration/1000.0f);
                SystemControl_PrintFlashStatus();
                DEBUG_Printf("=== FLASH״̬�鿴��� ===\r\n");
            } else if (total_press_duration >= 3000) {
                // KEY1����3�룺�����־
                key1_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY1����%.1f�룬��ʼ�����־ ===\r\n", total_press_duration/1000.0f);
            
            // ����������־���
            if (LogSystem_IsInitialized() && !log_output_state.is_outputting) {
                log_output_state.is_outputting = 1;
                log_output_state.current_index = 0;
                log_output_state.total_logs = LogSystem_GetLogCount();
                log_output_state.last_output_time = HAL_GetTick();
                
                // �����־ͷ��Ϣ
                LogSystem_OutputHeader();
                
                DEBUG_Printf("��ʼ������� %lu ����־...\r\n", log_output_state.total_logs);
            } else if (!LogSystem_IsInitialized()) {
                DEBUG_Printf("��־ϵͳδ��ʼ�����޷������־\r\n");
                DEBUG_Printf("=== ��־������ ===\r\n");
                }
            } else {
                DEBUG_Printf("KEY1��ѹʱ�䲻��3�루%.1f�룩���޲���\r\n", total_press_duration/1000.0f);
            }
        }
        
        // �������б�־
        key1_was_pressed = 0;
        key2_was_pressed_with_key1 = 0;
    }
    
    // ������־���������������
    if (log_output_state.is_outputting) {
        uint32_t current_time = HAL_GetTick();
        
        // ÿ10ms���һ����־������������ѭ��
        if (current_time - log_output_state.last_output_time >= 10) {
            if (log_output_state.current_index < log_output_state.total_logs) {
                // ���������־
                LogSystem_OutputSingle(log_output_state.current_index, LOG_FORMAT_DETAILED);
                log_output_state.current_index++;
                log_output_state.last_output_time = current_time;
                
                // ÿ10����־���һ�ν���
                if (log_output_state.current_index % 10 == 0) {
                    DEBUG_Printf("�������: %lu/%lu\r\n", 
                                log_output_state.current_index, 
                                log_output_state.total_logs);
                }
            } else {
                // ������
                LogSystem_OutputFooter();
                DEBUG_Printf("=== ��־������ ===\r\n");
                log_output_state.is_outputting = 0;
            }
        }
    }
    
    // PC14����״̬��⣨�����ɿ�ʱִ�й��ܣ�
    static uint8_t key2_was_pressed = 0;
    static uint32_t key2_saved_start_time = 0;  // ���水����ʼʱ��
    
    if (key2_pressed) {
        if (!key2_was_pressed) {
            key2_was_pressed = 1;  // ��ǰ�������������
            key2_saved_start_time = key2_press_start_time;  // ���濪ʼʱ��
        }
        
        // ���������ڼ䣬ֻ��ʾ��ʾ��Ϣ����ִ�й���
        uint32_t current_time = HAL_GetTick();
        uint32_t press_duration = current_time - key2_saved_start_time;
        
        // ��ʾ������ʾ��ÿ�����һ�Σ�
        static uint32_t last_hint_time2 = 0;
        if (current_time - last_hint_time2 >= 1000) {
            last_hint_time2 = current_time;
            
            // ֻ����û��ͬʱ����KEY1ʱ����ʾ����KEY2��ʾ
            if (!key1_pressed) {
                if (press_duration >= 15000) {
                    DEBUG_Printf("KEY2�Ѱ��� %.1f �룬�ɿ�����ȫ����FLASH��Σ�գ���\r\n", press_duration/1000.0f);
                } else if (press_duration >= 10000) {
                    DEBUG_Printf("KEY2�Ѱ��� %.1f �룬�ɿ���ִ������д�����ԣ���������15�����ȫ����\r\n", press_duration/1000.0f);
                } else if (press_duration >= 8000) {
                    DEBUG_Printf("KEY2�Ѱ��� %.1f �룬�ɿ���ִ�п���д�����ԣ���������10������д��\r\n", press_duration/1000.0f);
                } else if (press_duration >= 3000) {
                    DEBUG_Printf("KEY2�Ѱ��� %.1f �룬�ɿ������������־����������8�����д��\r\n", press_duration/1000.0f);
                } else {
                    DEBUG_Printf("KEY2�Ѱ��� %.1f �룬��3����գ�8�����д����10������д����15����ȫ����\r\n", press_duration/1000.0f);
                }
            }
        }
    } else if (key2_was_pressed && !key2_long_press_triggered) {
        // �����ɿ��������ܰ�ѹʱ��ִ�ж�Ӧ����
        uint32_t total_press_duration = HAL_GetTick() - key2_saved_start_time;
        
        // ֻ����û��ͬʱ����KEY1ʱ��ִ�е���KEY2����
        static uint8_t key1_was_pressed_with_key2 = 0;
        if (key1_pressed) {
            key1_was_pressed_with_key2 = 1;
        }
        
        if (!key1_was_pressed_with_key2) {
            // ����KEY2����
            if (total_press_duration >= 15000) {
                // KEY2����15�룺��ȫ����FLASH����Σ�ղ�����
                key2_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY2����%.1f�룬��ʼ��ȫ����FLASH ===\r\n", total_press_duration/1000.0f);
                DEBUG_Printf("���棺�⽫����ɾ��������־���ݣ�\r\n");
                SystemControl_FlashCompleteErase();
                DEBUG_Printf("=== FLASH��ȫ����������� ===\r\n");
            } else if (total_press_duration >= 10000) {
                // KEY2����10�룺����FLASHд�����ԣ�����д��15MB��
            key2_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY2����%.1f�룬��ʼ����FLASHд������ ===\r\n", total_press_duration/1000.0f);
                SystemControl_FlashFillTest();
                DEBUG_Printf("=== ����FLASHд��������� ===\r\n");
            } else if (total_press_duration >= 8000) {
                // KEY2����8�룺����FLASHд�����ԣ�д��10000����
                key2_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY2����%.1f�룬��ʼ����FLASHд������ ===\r\n", total_press_duration/1000.0f);
                SystemControl_FlashQuickFillTest();
                DEBUG_Printf("=== ����FLASHд��������� ===\r\n");
            } else if (total_press_duration >= 3000) {
                // KEY2����3�룺���������־��ԭ���ܣ�
                key2_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY2����%.1f�룬��ʼ���������־ ===\r\n", total_press_duration/1000.0f);
            
            // ���������־
            if (LogSystem_IsInitialized()) {
                LogSystemStatus_t status = LogSystem_Reset();  // ʹ�ÿ������÷�ʽ
                if (status == LOG_SYSTEM_OK) {
                    DEBUG_Printf("�����\r\n");  // �û�Ҫ��ķ�����Ϣ
                } else {
                    DEBUG_Printf("��־���ʧ��\r\n");
                }
            } else {
                DEBUG_Printf("��־ϵͳδ��ʼ�����޷������־\r\n");
            }
            
            DEBUG_Printf("=== ��־��ղ������ ===\r\n");
            } else {
                DEBUG_Printf("KEY2��ѹʱ�䲻��3�루%.1f�룩���޲���\r\n", total_press_duration/1000.0f);
            }
        }
        
        // �������б�־
        key2_was_pressed = 0;
        key1_was_pressed_with_key2 = 0;
    }
    
    // ������־λ��ʱ���ã���ֹ�ظ�ִ�У�
    static uint32_t key_reset_time = 0;
    if ((key1_long_press_triggered || key2_long_press_triggered) && 
        (HAL_GetTick() - key_reset_time > 2000)) {  // 2����Զ�����
        key1_long_press_triggered = 0;
        key2_long_press_triggered = 0;
        key_reset_time = HAL_GetTick();
    }
    
    // ��������ʱ�䣨���κΰ����������ʱ��
    if (key1_long_press_triggered || key2_long_press_triggered) {
        if (key_reset_time == 0) {
            key_reset_time = HAL_GetTick();  // �������ÿ�ʼʱ��
        }
    }
    
    // ���Ź�ι������ʵ�֣����ٵ������
    SystemState_t current_state = SystemControl_GetState();
    uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
    
    // ע�Ϳ��Ź�������Ϣ�����ִ���������
    // static uint32_t last_watchdog_debug_time = 0;
    // uint32_t current_time = HAL_GetTick();
    // if(current_time - last_watchdog_debug_time >= 500) {
    //     last_watchdog_debug_time = current_time;
    //     DEBUG_Printf("[���Ź�����] ʱ��:%lu, ϵͳ״̬:%d, �쳣��־:0x%04X\r\n", 
    //                 current_time, current_state, alarm_flags);
    //     DEBUG_Printf("[���Ź�����] IWDG�Զ�ι��:%s, ��ȫ���:%s\r\n", 
    //                 iwdg_auto_feed ? "����" : "����", 
    //                 iwdg_safe_to_feed ? "ͨ��" : "��ֹ");
    // }
    
    // ע�͵�����صı�����������뾯��
    // static uint32_t last_feed_time = 0;
    // static uint32_t feed_count = 0;
    
    // �򻯵Ŀ��Ź�ι����������״̬������״̬���Լ�׶ζ�ι��
    if(current_state == SYSTEM_STATE_NORMAL || 
       current_state == SYSTEM_STATE_ALARM ||
       current_state == SYSTEM_STATE_SELF_TEST ||
       current_state == SYSTEM_STATE_SELF_TEST_CHECK) {
        // ����״̬������״̬���Լ�׶ζ���Ҫι����ֻ�д���״̬��ֹͣι��
        IWDG_Refresh();
        
        // ע��ι��ͳ�Ƶ�����Ϣ�����ִ���������
        // feed_count++;
        // uint32_t current_time = HAL_GetTick();
        // uint32_t feed_interval = current_time - last_feed_time;
        // last_feed_time = current_time;
        // 
        // if(feed_count % 100 == 0) {
        //     DEBUG_Printf("[���Ź�����] ��ι��%lu�Σ����μ��:%lums\r\n", feed_count, feed_interval);
        // }
        // 
        // // �������O���쳣��ÿ��ι�������ȷ����Ϣ
        // if(alarm_flags & (1 << 14)) {
        //     static uint32_t last_o_feed_debug = 0;
        //     if(current_time - last_o_feed_debug >= 1000) {  // ÿ�����һ��
        //         last_o_feed_debug = current_time;
        //         DEBUG_Printf("[���Ź�����] ? O���쳣�ڼ����ι����״̬:%d������:%lu\r\n", 
        //                     current_state, feed_count);
        //     }
        // }
    } else {
        // ��ι���������������Ϣ��ע�ͣ�
        // static uint32_t last_no_feed_debug = 0;
        // uint32_t current_time = HAL_GetTick();
        // if(current_time - last_no_feed_debug >= 1000) {
        //     last_no_feed_debug = current_time;
        //     DEBUG_Printf("[���Ź�����] ? ֹͣι����ϵͳ״̬:%d������״̬��\r\n", current_state);
        // }
    }
    
    // ��ѭ����ʱ1ms������CPUռ�ù���
    HAL_Delay(1);
    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
