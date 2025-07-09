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
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

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
#include "log_system.h"    // 包含日志系统模块
#include "w25q128_driver.h" // 包含W25Q128 Flash驱动模块
#include <string.h>        // 包含字符串操作函数
#include "safety_monitor.h" // 包含安全监控模块
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
// 外部变量声明（这些变量在其他文件中定义）
extern volatile uint8_t key1_pressed;
extern volatile uint8_t key2_pressed;
extern volatile uint32_t key1_press_start_time;
extern volatile uint32_t key2_press_start_time;
extern volatile uint8_t key1_long_press_triggered;
extern volatile uint8_t key2_long_press_triggered;
extern volatile uint32_t fan_pulse_count;

// 新增：日志输出状态管理
typedef struct {
    uint8_t is_outputting;      // 是否正在输出日志
    uint32_t current_index;     // 当前输出索引
    uint32_t total_logs;        // 总日志数量
    uint32_t last_output_time;  // 上次输出时间
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
  MX_IWDG_Init();
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
  
  // 打印复位原因统计信息（在复位原因设置完成后）
  IwdgControl_PrintResetReason();
  
  // 统一检测系统重启原因并记录到日志系统
  DEBUG_Printf("=== 统一检测系统重启原因 ===\r\n");
  uint8_t reset_reason_detected = 0;
  char reset_reason_msg[48] = {0};
  IwdgResetReason_t iwdg_reset_reason = IWDG_RESET_UNKNOWN;
  static uint32_t reset_count = 0;  // 重启计数（简化版，存储在静态变量中）
  
  // 检测看门狗复位
  if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
    DEBUG_Printf("检测到看门狗复位，系统正在恢复...\r\n");
    strcpy(reset_reason_msg, "Watchdog reset detected");
    reset_reason_detected = 1;
    iwdg_reset_reason = IWDG_RESET_WATCHDOG;
    reset_count++;  // 看门狗复位计数增加
  }
  // 检测窗口看门狗复位
  else if(__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
    DEBUG_Printf("检测到窗口看门狗复位\r\n");
    strcpy(reset_reason_msg, "Window watchdog reset");
    reset_reason_detected = 6;
    iwdg_reset_reason = IWDG_RESET_WINDOW_WATCHDOG;
    reset_count++;
  }
  // 检测软件复位
  else if(__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
    DEBUG_Printf("检测到软件复位\r\n");
    strcpy(reset_reason_msg, "Software reset");
    reset_reason_detected = 2;
    iwdg_reset_reason = IWDG_RESET_SOFTWARE;
  }
  // 检测上电复位
  else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
    DEBUG_Printf("检测到上电复位\r\n");
    strcpy(reset_reason_msg, "Power on reset");
    reset_reason_detected = 3;
    iwdg_reset_reason = IWDG_RESET_POWER_ON;
  }
  // 检测外部复位/引脚复位
  else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) {
    DEBUG_Printf("检测到引脚复位\r\n");
    strcpy(reset_reason_msg, "Pin reset");
    reset_reason_detected = 4;
    iwdg_reset_reason = IWDG_RESET_PIN;
  }
  else {
    DEBUG_Printf("正常启动\r\n");
    strcpy(reset_reason_msg, "Normal startup");
    reset_reason_detected = 5;
    iwdg_reset_reason = IWDG_RESET_NONE;
  }
  
  // 将复位原因和计数传递给IWDG模块
  IwdgControl_SetResetReason(iwdg_reset_reason);
  IwdgControl_SetResetCount(reset_count);
  
  // 清除复位标志（统一在此处清除，避免重复检测）
  __HAL_RCC_CLEAR_RESET_FLAGS();
  
  DEBUG_Printf("复位原因编码: %d, IWDG原因: %d, 复位次数: %lu\r\n", 
               reset_reason_detected, iwdg_reset_reason, reset_count);
  
  // Flash连接测试（调试用）
  DEBUG_Printf("开始Flash连接测试...\r\n");
  if(W25Q128_TestConnection() == W25Q128_OK) {
    DEBUG_Printf("Flash连接测试通过\r\n");
  } else {
    DEBUG_Printf("Flash连接测试失败\r\n");
  }
  
  // 日志系统模块初始化
  if(LogSystem_Init() == LOG_SYSTEM_OK) {
    DEBUG_Printf("日志系统初始化完成\r\n");
    
    // 记录重启原因到日志系统（完整支持所有复位类型）
    char log_msg[48];
    switch(reset_reason_detected) {
      case 1:  // 看门狗复位
        snprintf(log_msg, sizeof(log_msg), "看门狗复位(第%lu次)", reset_count);
        LOG_WATCHDOG_RESET();
        // 同时记录详细信息
        LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_WATCHDOG_RESET, log_msg);
        break;
        
      case 6:  // 窗口看门狗复位
        snprintf(log_msg, sizeof(log_msg), "窗口看门狗复位(第%lu次)", reset_count);
        LOG_WATCHDOG_RESET();
        LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_WATCHDOG_RESET, log_msg);
        break;
        
      case 2:  // 软件复位
        LOG_SOFTWARE_RESET("软件主动复位");
        break;
        
      case 3:  // 上电复位
        LOG_POWER_ON_RESET();
        break;
        
      case 4:  // 引脚复位
        LOG_SYSTEM_RESTART("引脚复位(按复位按钮)");
        break;
        
      default:  // 正常启动
        LOG_SYSTEM_START();
        break;
    }
  } else {
    DEBUG_Printf("日志系统初始化失败\r\n");
    // 记录初始化失败到调试输出，无法记录到Flash
  }
  
  // 注意：IWDG将在系统进入正常状态后自动启动，无需在此处阻塞延时
  DEBUG_Printf("系统初始化完成，开始状态机循环\r\n");
  
  // 输出复位原因修复验证信息
  DEBUG_Printf("\r\n=== 复位原因记录修复验证 ===\r\n");
  DEBUG_Printf("? 复位检测已统一到main.c处理\r\n");
  DEBUG_Printf("? 避免了iwdg_control.c的重复检测\r\n");
  DEBUG_Printf("? 确保复位原因在日志系统初始化后记录\r\n");
  DEBUG_Printf("? 支持完整的复位类型检测和日志记录\r\n");
  DEBUG_Printf("验证方法: 重启系统后使用KEY1长按3秒查看日志\r\n");



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    // ? 优先处理DC_CTRL中断标志（避免看门狗复位）
    SafetyMonitor_ProcessDcCtrlInterrupt();
    
    // 系统控制模块状态机处理（包含自检、主循环调度等）
    SystemControl_Process();
    
    // PC13按键状态检测（按键松开时执行功能）
    static uint8_t key1_was_pressed = 0;
    static uint32_t key1_saved_start_time = 0;  // 保存按键开始时间
    
    if (key1_pressed) {
        if (!key1_was_pressed) {
            key1_was_pressed = 1;  // 标记按键曾经被按下
            key1_saved_start_time = key1_press_start_time;  // 保存开始时间
        }
        
        // 按键按下期间，只显示提示信息，不执行功能
        uint32_t current_time = HAL_GetTick();
        uint32_t press_duration = current_time - key1_saved_start_time;
        
        // 显示按键提示（每秒更新一次）
        static uint32_t last_hint_time = 0;
        if (current_time - last_hint_time >= 1000) {
            last_hint_time = current_time;
            
            if (key2_pressed) {
                if (press_duration >= 3000) {
                    DEBUG_Printf("KEY1+KEY2已按下 %.1f 秒，松开将执行FLASH读取测试\r\n", press_duration/1000.0f);
                } else {
                    DEBUG_Printf("KEY1+KEY2已按下 %.1f 秒，需3秒执行FLASH读取测试\r\n", press_duration/1000.0f);
                }
            } else {
                if (press_duration >= 8000) {
                    DEBUG_Printf("KEY1已按下 %.1f 秒，松开将执行FLASH状态查看\r\n", press_duration/1000.0f);
                } else if (press_duration >= 3000) {
                    DEBUG_Printf("KEY1已按下 %.1f 秒，松开将输出日志，继续按下8秒可查看FLASH状态\r\n", press_duration/1000.0f);
                } else {
                    DEBUG_Printf("KEY1已按下 %.1f 秒，需3秒输出日志，8秒查看FLASH状态\r\n", press_duration/1000.0f);
                }
            }
        }
    } else if (key1_was_pressed && !key1_long_press_triggered) {
        // 按键松开，根据总按压时长执行对应功能
        uint32_t total_press_duration = HAL_GetTick() - key1_saved_start_time;
        
        // 检查是否有双键组合
        static uint8_t key2_was_pressed_with_key1 = 0;
        if (key2_pressed) {
            key2_was_pressed_with_key1 = 1;
        }
        
        if (key2_was_pressed_with_key1 && total_press_duration >= 3000) {
            // KEY1+KEY2组合：FLASH读取测试
            key1_long_press_triggered = 1;
            key2_long_press_triggered = 1;
            DEBUG_Printf("\r\n=== KEY1+KEY2组合（%.1f秒），开始FLASH读取测试 ===\r\n", total_press_duration/1000.0f);
            SystemControl_FlashReadTest();
            DEBUG_Printf("=== FLASH读取测试完成 ===\r\n");
        } else if (!key2_was_pressed_with_key1) {
            // 单独KEY1
            if (total_press_duration >= 8000) {
                // KEY1长按8秒：FLASH状态查看
                key1_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY1长按%.1f秒，查看FLASH状态 ===\r\n", total_press_duration/1000.0f);
                SystemControl_PrintFlashStatus();
                DEBUG_Printf("=== FLASH状态查看完成 ===\r\n");
            } else if (total_press_duration >= 3000) {
                // KEY1长按3秒：输出日志
                key1_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY1长按%.1f秒，开始输出日志 ===\r\n", total_press_duration/1000.0f);
            
            // 启动分批日志输出
            if (LogSystem_IsInitialized() && !log_output_state.is_outputting) {
                log_output_state.is_outputting = 1;
                log_output_state.current_index = 0;
                log_output_state.total_logs = LogSystem_GetLogCount();
                log_output_state.last_output_time = HAL_GetTick();
                
                // 输出日志头信息
                LogSystem_OutputHeader();
                
                DEBUG_Printf("开始分批输出 %lu 条日志...\r\n", log_output_state.total_logs);
            } else if (!LogSystem_IsInitialized()) {
                DEBUG_Printf("日志系统未初始化，无法输出日志\r\n");
                DEBUG_Printf("=== 日志输出完成 ===\r\n");
                }
            } else {
                DEBUG_Printf("KEY1按压时间不足3秒（%.1f秒），无操作\r\n", total_press_duration/1000.0f);
            }
        }
        
        // 重置所有标志
        key1_was_pressed = 0;
        key2_was_pressed_with_key1 = 0;
    }
    
    // 分批日志输出处理（非阻塞）
    if (log_output_state.is_outputting) {
        uint32_t current_time = HAL_GetTick();
        
        // 每10ms输出一条日志，避免阻塞主循环
        if (current_time - log_output_state.last_output_time >= 10) {
            if (log_output_state.current_index < log_output_state.total_logs) {
                // 输出单条日志
                LogSystem_OutputSingle(log_output_state.current_index, LOG_FORMAT_DETAILED);
                log_output_state.current_index++;
                log_output_state.last_output_time = current_time;
                
                // 每10条日志输出一次进度
                if (log_output_state.current_index % 10 == 0) {
                    DEBUG_Printf("输出进度: %lu/%lu\r\n", 
                                log_output_state.current_index, 
                                log_output_state.total_logs);
                }
            } else {
                // 输出完成
                LogSystem_OutputFooter();
                DEBUG_Printf("=== 日志输出完成 ===\r\n");
                log_output_state.is_outputting = 0;
            }
        }
    }
    
    // PC14按键状态检测（按键松开时执行功能）
    static uint8_t key2_was_pressed = 0;
    static uint32_t key2_saved_start_time = 0;  // 保存按键开始时间
    
    if (key2_pressed) {
        if (!key2_was_pressed) {
            key2_was_pressed = 1;  // 标记按键曾经被按下
            key2_saved_start_time = key2_press_start_time;  // 保存开始时间
        }
        
        // 按键按下期间，只显示提示信息，不执行功能
        uint32_t current_time = HAL_GetTick();
        uint32_t press_duration = current_time - key2_saved_start_time;
        
        // 显示按键提示（每秒更新一次）
        static uint32_t last_hint_time2 = 0;
        if (current_time - last_hint_time2 >= 1000) {
            last_hint_time2 = current_time;
            
            // 只有在没有同时按下KEY1时才显示单独KEY2提示
            if (!key1_pressed) {
                if (press_duration >= 15000) {
                    DEBUG_Printf("KEY2已按下 %.1f 秒，松开将完全擦除FLASH（危险！）\r\n", press_duration/1000.0f);
                } else if (press_duration >= 10000) {
                    DEBUG_Printf("KEY2已按下 %.1f 秒，松开将执行完整写满测试，继续按下15秒可完全擦除\r\n", press_duration/1000.0f);
                } else if (press_duration >= 8000) {
                    DEBUG_Printf("KEY2已按下 %.1f 秒，松开将执行快速写满测试，继续按下10秒完整写满\r\n", press_duration/1000.0f);
                } else if (press_duration >= 3000) {
                    DEBUG_Printf("KEY2已按下 %.1f 秒，松开将快速清空日志，继续按下8秒快速写满\r\n", press_duration/1000.0f);
                } else {
                    DEBUG_Printf("KEY2已按下 %.1f 秒，需3秒清空，8秒快速写满，10秒完整写满，15秒完全擦除\r\n", press_duration/1000.0f);
                }
            }
        }
    } else if (key2_was_pressed && !key2_long_press_triggered) {
        // 按键松开，根据总按压时长执行对应功能
        uint32_t total_press_duration = HAL_GetTick() - key2_saved_start_time;
        
        // 只有在没有同时按下KEY1时才执行单独KEY2操作
        static uint8_t key1_was_pressed_with_key2 = 0;
        if (key1_pressed) {
            key1_was_pressed_with_key2 = 1;
        }
        
        if (!key1_was_pressed_with_key2) {
            // 单独KEY2操作
            if (total_press_duration >= 15000) {
                // KEY2长按15秒：完全擦除FLASH（最危险操作）
                key2_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY2长按%.1f秒，开始完全擦除FLASH ===\r\n", total_press_duration/1000.0f);
                DEBUG_Printf("警告：这将永久删除所有日志数据！\r\n");
                SystemControl_FlashCompleteErase();
                DEBUG_Printf("=== FLASH完全擦除操作完成 ===\r\n");
            } else if (total_press_duration >= 10000) {
                // KEY2长按10秒：完整FLASH写满测试（真正写满15MB）
            key2_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY2长按%.1f秒，开始完整FLASH写满测试 ===\r\n", total_press_duration/1000.0f);
                SystemControl_FlashFillTest();
                DEBUG_Printf("=== 完整FLASH写满测试完成 ===\r\n");
            } else if (total_press_duration >= 8000) {
                // KEY2长按8秒：快速FLASH写满测试（写入10000条）
                key2_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY2长按%.1f秒，开始快速FLASH写满测试 ===\r\n", total_press_duration/1000.0f);
                SystemControl_FlashQuickFillTest();
                DEBUG_Printf("=== 快速FLASH写满测试完成 ===\r\n");
            } else if (total_press_duration >= 3000) {
                // KEY2长按3秒：快速清空日志（原功能）
                key2_long_press_triggered = 1;
                DEBUG_Printf("\r\n=== KEY2长按%.1f秒，开始快速清空日志 ===\r\n", total_press_duration/1000.0f);
            
            // 清空所有日志
            if (LogSystem_IsInitialized()) {
                LogSystemStatus_t status = LogSystem_Reset();  // 使用快速重置方式
                if (status == LOG_SYSTEM_OK) {
                    DEBUG_Printf("已清空\r\n");  // 用户要求的反馈信息
                } else {
                    DEBUG_Printf("日志清空失败\r\n");
                }
            } else {
                DEBUG_Printf("日志系统未初始化，无法清空日志\r\n");
            }
            
            DEBUG_Printf("=== 日志清空操作完成 ===\r\n");
            } else {
                DEBUG_Printf("KEY2按压时间不足3秒（%.1f秒），无操作\r\n", total_press_duration/1000.0f);
            }
        }
        
        // 重置所有标志
        key2_was_pressed = 0;
        key1_was_pressed_with_key2 = 0;
    }
    
    // 按键标志位超时重置（防止重复执行）
    static uint32_t key_reset_time = 0;
    if ((key1_long_press_triggered || key2_long_press_triggered) && 
        (HAL_GetTick() - key_reset_time > 2000)) {  // 2秒后自动重置
        key1_long_press_triggered = 0;
        key2_long_press_triggered = 0;
        key_reset_time = HAL_GetTick();
    }
    
    // 设置重置时间（当任何按键操作完成时）
    if (key1_long_press_triggered || key2_long_press_triggered) {
        if (key_reset_time == 0) {
            key_reset_time = HAL_GetTick();  // 设置重置开始时间
        }
    }
    
    // 看门狗喂狗：简化实现，减少调试输出
    SystemState_t current_state = SystemControl_GetState();
    uint16_t alarm_flags = SafetyMonitor_GetAlarmFlags();
    
    // 注释看门狗调试信息，保持串口输出简洁
    // static uint32_t last_watchdog_debug_time = 0;
    // uint32_t current_time = HAL_GetTick();
    // if(current_time - last_watchdog_debug_time >= 500) {
    //     last_watchdog_debug_time = current_time;
    //     DEBUG_Printf("[看门狗调试] 时间:%lu, 系统状态:%d, 异常标志:0x%04X\r\n", 
    //                 current_time, current_state, alarm_flags);
    //     DEBUG_Printf("[看门狗调试] IWDG自动喂狗:%s, 安全检查:%s\r\n", 
    //                 iwdg_auto_feed ? "启用" : "禁用", 
    //                 iwdg_safe_to_feed ? "通过" : "阻止");
    // }
    
    // 注释调试相关的变量，避免编译警告
    // static uint32_t last_feed_time = 0;
    // static uint32_t feed_count = 0;
    
    // 简化的看门狗喂狗：在正常状态、报警状态和自检阶段都喂狗
    if(current_state == SYSTEM_STATE_NORMAL || 
       current_state == SYSTEM_STATE_ALARM ||
       current_state == SYSTEM_STATE_SELF_TEST ||
       current_state == SYSTEM_STATE_SELF_TEST_CHECK) {
        // 正常状态、报警状态和自检阶段都需要喂狗，只有错误状态才停止喂狗
        IWDG_Refresh();
        
        // 注释喂狗统计调试信息，保持串口输出简洁
        // feed_count++;
        // uint32_t current_time = HAL_GetTick();
        // uint32_t feed_interval = current_time - last_feed_time;
        // last_feed_time = current_time;
        // 
        // if(feed_count % 100 == 0) {
        //     DEBUG_Printf("[看门狗调试] 已喂狗%lu次，本次间隔:%lums\r\n", feed_count, feed_interval);
        // }
        // 
        // // 如果存在O类异常，每次喂狗都输出确认信息
        // if(alarm_flags & (1 << 14)) {
        //     static uint32_t last_o_feed_debug = 0;
        //     if(current_time - last_o_feed_debug >= 1000) {  // 每秒输出一次
        //         last_o_feed_debug = current_time;
        //         DEBUG_Printf("[看门狗调试] ? O类异常期间继续喂狗，状态:%d，计数:%lu\r\n", 
        //                     current_state, feed_count);
        //     }
        // }
    } else {
        // 不喂狗的情况（调试信息已注释）
        // static uint32_t last_no_feed_debug = 0;
        // uint32_t current_time = HAL_GetTick();
        // if(current_time - last_no_feed_debug >= 1000) {
        //     last_no_feed_debug = current_time;
        //     DEBUG_Printf("[看门狗调试] ? 停止喂狗，系统状态:%d（错误状态）\r\n", current_state);
        // }
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
