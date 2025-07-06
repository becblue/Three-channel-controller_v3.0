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
  *此处的中文秒时旨在测试cubemx的编码是否与cursoe以及keil兼容
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
#include "iwdg.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>         // 包含标准输入输出库，用于printf功能
#include "gpio.h"          // 包含GPIO驱动模块头文件，用于GPIO底层操作
#include "gpio_control.h"  // 包含GPIO控制模块
#include "relay_control.h"
#include "temperature_monitor.h"
#include "oled_display.h"
#include "system_control.h"
#include "usart.h"
#include "iwdg_control.h"  // 包含IWDG看门狗控制模块
#include "iwdg.h"          // 包含IWDG外设驱动
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
  * @brief  重定向printf函数到USART3，用于调试信息输出
  * @param  ch: 要发送的字符
  * @param  f: 文件指针（未使用）
  * @retval 发送的字符
  */
// int fputc(int ch, FILE *f)
// {
//   // 通过USART3发送单个字符，等待发送完成
//   HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
//   return ch;  // 返回发送的字符
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
  
  // 延时200ms等待系统稳定
  HAL_Delay(200);
  
  // 串口初始化测试
  DEBUG_Printf("\r\n=== 三通道切换箱控制系统启动 ===\r\n");
  DEBUG_Printf("MCU: STM32F103RCT6\r\n");
  DEBUG_Printf("系统时钟: %luMHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
  DEBUG_Printf("编译时间: %s %s\r\n", __DATE__, __TIME__);
  DEBUG_Printf("USART3调试串口初始化完成\r\n");

  // 初始化各功能模块
  DEBUG_Printf("开始初始化各功能模块...\r\n");
  
  // 启动风扇PWM输出
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  DEBUG_Printf("风扇PWM输出启动完成\r\n");
  
  // OLED显示模块初始化
  if(OLED_TestConnection()) {
    DEBUG_Printf("OLED连接正常，初始化OLED模块\r\n");
    OLED_Init();
  } else {
    DEBUG_Printf("OLED连接失败！\r\n");
  }
  
  // 温度监控模块初始化
  TemperatureMonitor_Init();
  DEBUG_Printf("温度监控模块初始化完成\r\n");
  
  // 继电器控制模块初始化
  RelayControl_Init();
  DEBUG_Printf("继电器控制模块初始化完成\r\n");
  
  // 系统控制模块初始化（开始2秒LOGO显示）
  SystemControl_Init();
  DEBUG_Printf("系统控制模块初始化完成，开始执行自检流程\r\n");
  
  // IWDG看门狗控制模块初始化
  IwdgControl_Init();
  DEBUG_Printf("IWDG看门狗控制模块初始化完成\r\n");
  
  // 注意：IWDG将在系统进入正常状态后自动启动，无需在此处阻塞延时
  DEBUG_Printf("系统初始化完成，开始状态机循环\r\n");



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    // 系统控制模块状态机处理（包含自检、主循环调度等）
    SystemControl_Process();
    
    // 简化的看门狗喂狗：在正常状态和报警状态下都喂狗
    SystemState_t current_state = SystemControl_GetState();
    if(current_state == SYSTEM_STATE_NORMAL || current_state == SYSTEM_STATE_ALARM) {
        // 正常状态和报警状态都需要喂狗，只有错误状态才停止喂狗
        IWDG_Refresh();
    }
    
    // 主循环延时1ms，避免CPU占用过高
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
