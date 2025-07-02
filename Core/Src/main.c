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
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "oled_display.h" // ����OLED��ʾͷ�ļ�

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>         // ������׼��������⣬����printf����
#include "gpio.h"          // ����GPIO����ģ��ͷ�ļ�������GPIO�ײ����
#include "relay_control.h"
#include "temperature_monitor.h"
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
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  
  // ��ʱ200ms�ȴ�ϵͳ�ȶ�
  HAL_Delay(200);
  
  // ���ڳ�ʼ������
  printf("\r\n==============================\r\n");
  printf("��ͨ����ѹ�л������� v3.0 ����\r\n");
  printf("���ڵ��Թ��ܲ���\r\n");
  DEBUG_Printf("ϵͳʱ��Ƶ�ʣ�%dMHz\r\n", (int)(HAL_RCC_GetSysClockFreq()/1000000));
  DEBUG_Printf("��ǰ����ʱ�䣺%s %s\r\n", __DATE__, __TIME__);
  printf("==============================\r\n");

  // �����ʼ�����������
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  
  // I2C�豸ɨ�� - ����OLED��ַ
  DEBUG_Printf("[ɨ��] ��ʼI2C�豸ɨ��\r\n");
  uint8_t found_devices = 0;
  for(uint8_t addr = 0x08; addr < 0x78; addr++) {
    if(HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
      DEBUG_Printf("[ɨ��] ����I2C�豸��ַ: 0x%02X (7λ) / 0x%02X (8λ)\r\n", addr, addr << 1);
      found_devices++;
    }
  }
  if(found_devices == 0) {
    DEBUG_Printf("[ɨ��] δ�����κ�I2C�豸������Ӳ������\r\n");
  }
  HAL_Delay(1000);
  
  // OLED���Ӳ���
  DEBUG_Printf("[����] ��ʼOLED���Ӳ���\r\n");
  uint8_t oled_addr = OLED_TestConnection();
  if(oled_addr != 0) {
    DEBUG_Printf("[����] OLED���ӳɹ�����ַ: 0x%02X\r\n", oled_addr);
    
    // OLED��ʾ��ʼ���Ͳ���
    DEBUG_Printf("[����] ��ʼ��OLED��ʾ��\r\n");
    OLED_Init();
    HAL_Delay(500);
    
    DEBUG_Printf("[����] ��ʾLOGO\r\n");
    OLED_ShowLogo();
    HAL_Delay(5000); // ��ʾLOGO 5��
    
    // �ֿ����
    DEBUG_Printf("[����] ��ʼ�ֿ����\r\n");
    
    // ����6x8����
    DEBUG_Printf("[����] ����6x8����\r\n");
    OLED_Clear();
    OLED_DrawString(0, 0, "6x8 Font Test:", 6);
    OLED_DrawString(0, 1, "ABCDEFGHIJKLMNOP", 6);
    OLED_DrawString(0, 2, "QRSTUVWXYZ", 6);
    OLED_DrawString(0, 3, "abcdefghijklmnop", 6);
    OLED_DrawString(0, 4, "qrstuvwxyz", 6);
    OLED_DrawString(0, 5, "0123456789", 6);
    OLED_DrawString(0, 6, "!@#$%^&*()_+-=", 6);
    OLED_DrawString(0, 7, "[]{}|;':\",./<>?", 6);
    DEBUG_Printf("[����] 6x8������ʾ��ɣ�ÿ�����21�ַ�\r\n");
    HAL_Delay(4000); // ��ʾ4��
    
    // ����8x16����
    DEBUG_Printf("[����] ����8x16����\r\n");
    OLED_Clear();
    OLED_DrawString(0, 0, "8x16 Font:", 8);
    OLED_DrawString(0, 2, "ABCDEFGHIJ", 8);
    OLED_DrawString(0, 4, "0123456789", 8);
    OLED_DrawString(0, 6, "!@#$%^&*()", 8);
    DEBUG_Printf("[����] 8x16������ʾ��ɣ�ÿ�����16�ַ�\r\n");
    HAL_Delay(4000); // ��ʾ4��
    
    // ����������
    DEBUG_Printf("[����] ����������\r\n");
    OLED_Clear();
    OLED_DrawString(0, 0, "STM32F103RCT6", 8);   // 8x16�������
    OLED_DrawString(0, 3, "Channel 1: ON  [6x8]", 6);  // 6x8��������
    OLED_DrawString(0, 4, "Channel 2: OFF [6x8]", 6);
    OLED_DrawString(0, 5, "Channel 3: ON  [6x8]", 6);
    OLED_DrawString(0, 7, "Temp:25C Fan:50%", 6);
    DEBUG_Printf("[����] ������������ɣ���֤8x16+6x8���\r\n");
    HAL_Delay(4000); // ��ʾ4��
    
    DEBUG_Printf("[����] �ֿ�������\r\n");
    DEBUG_Printf("[����] OLED�������\r\n");
  } else {
    DEBUG_Printf("[����] OLED����ʧ�ܣ�����Ӳ�����Ӻ͵�ַ����\r\n");
    DEBUG_Printf("[��ʾ] ����OLED��ַ: 0x78, 0x7A (8λ) �� 0x3C, 0x3D (7λ)\r\n");
  }

  // ��������PWM�����ȷ��PA6��PWM�ź����
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // �����У�������PWM����

  // ���� �����¶ȼ�س�ʼ��������ADC+DMA�ɼ� ����
  TemperatureMonitor_Init();

  // �̵�������ģ���ʼ�������ܲ���
  RelayControl_Init();
  HAL_Delay(500);
  // ���β�����ͨ���Ŀ����͹ر�
  DEBUG_Printf("[����] ����ͨ��1\r\n");
  RelayControl_OpenChannel(1);
  HAL_Delay(1000);
  DEBUG_Printf("[����] �ر�ͨ��1\r\n");
  RelayControl_CloseChannel(1);
  HAL_Delay(1000);

  DEBUG_Printf("[����] ����ͨ��2\r\n");
  RelayControl_OpenChannel(2);
  HAL_Delay(1000);
  DEBUG_Printf("[����] �ر�ͨ��2\r\n");
  RelayControl_CloseChannel(2);
  HAL_Delay(1000);

  DEBUG_Printf("[����] ����ͨ��3\r\n");
  RelayControl_OpenChannel(3);
  HAL_Delay(1000);
  DEBUG_Printf("[����] �ر�ͨ��3\r\n");
  RelayControl_CloseChannel(3);
  HAL_Delay(1000);

  // ================== �¶�����ȼ������ ==================
  extern volatile uint32_t fan_pulse_count;
  extern uint32_t fan_rpm;
  fan_pulse_count = 0;
  fan_rpm = 0;
  HAL_Delay(1000); // Ԥ��1�룬ȷ��ͳ�ƴ��ڸɾ�
  // ������һ��ͳ�����ڣ���������˲������ѻ�Ӱ��
  TemperatureMonitor_FanSpeed1sHandler();
  HAL_Delay(1000);
  DEBUG_Printf("[����] ��ʼ����PWM����������\r\n");
  for(int t=0; t<10; t++) {
    uint8_t pwm = 10 + (90 * t) / 9; // 10%~100%
    TemperatureMonitor_SetFanPWM(pwm);
    fan_pulse_count = 0;
    HAL_Delay(1000); // �ȴ������ȶ�
    fan_pulse_count = 0; // �������㣬׼��ͳ��1��
    HAL_Delay(1000);     // ��ȷͳ��1������
    TemperatureMonitor_FanSpeed1sHandler();
    FanSpeedInfo_t info = TemperatureMonitor_GetFanSpeed();
    DEBUG_Printf("[���Ȳ���] PWM: %d%%, ʵ��ת��: %d RPM\r\n", pwm, info.rpm);
  }
  DEBUG_Printf("[����] ��ʼ����PWM�ɿ쵽�����\r\n");
  for(int t=0; t<10; t++) {
    uint8_t pwm = 100 - (90 * t) / 9; // 100%~10%
    TemperatureMonitor_SetFanPWM(pwm);
    fan_pulse_count = 0;
    HAL_Delay(1000); // �ȴ������ȶ�
    fan_pulse_count = 0; // �������㣬׼��ͳ��1��
    HAL_Delay(1000);     // ��ȷͳ��1������
    TemperatureMonitor_FanSpeed1sHandler();
    FanSpeedInfo_t info = TemperatureMonitor_GetFanSpeed();
    DEBUG_Printf("[���Ȳ���] PWM: %d%%, ʵ��ת��: %d RPM\r\n", pwm, info.rpm);
  }
  DEBUG_Printf("[����] ���ȼ�����̽���\r\n");

  // ================== ADC�¶Ȳɼ��������� ==================
  DEBUG_Printf("[����] ��ʼADC�¶Ȳɼ����ԣ���30��\r\n");
  for(int t=0; t<30; t++) {
    TemperatureMonitor_UpdateAll(); // ˢ������ͨ���¶�
    TemperatureMonitor_DebugPrint(); // ���3·�¶�
    HAL_Delay(1000);
  }
  DEBUG_Printf("[����] ADC�¶Ȳɼ����Խ���\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    // ��ѭ�����ƣ��޵������
    // ���Ź��ѽ��ã�ϵͳ�����ڵ���ģʽ��
    
    // ��ѭ����ʱ1ms������CPUռ�ù��ߣ�����ʱ����������ʱ��
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
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
