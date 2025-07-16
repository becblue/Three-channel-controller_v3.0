/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : log_system.c
  * @brief          : 系统运行日志模块实现文件 - 精简版日志系统
  ******************************************************************************
  * @attention
  *
  * 精简版日志系统实现，只记录核心事件：
  * 1. 异常事件记录
  * 2. 系统保护事件
  * 3. 系统生命周期
  * 4. 系统健康监控
  * 
  * 采用统一存储的循环覆盖策略，保留最新记录
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "log_system.h"
#include "usart.h"
#include "iwdg.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* 全局日志管理器 - 统一存储管理 */
static LogManager_t log_manager = {0};

/* 日志分类名称映射表（精简版） */
static const char* log_category_strings[4] = {
    "INVALID",  // 0 - 无效分类
    "SAFETY",   // LOG_CATEGORY_SAFETY
    "SYSTEM",   // LOG_CATEGORY_SYSTEM
    "MONITOR"   // LOG_CATEGORY_MONITOR
};

/* 全局变量用于OLED显示 */
extern uint8_t oled_show_flash_warning;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* 内部辅助函数 */
static LogSystemStatus_t LogSystem_WriteEntry(const LogEntry_t* entry);
static LogSystemStatus_t LogSystem_PrepareNewSector(uint32_t address);
static uint32_t LogSystem_GetEntryAddress(uint32_t entry_index);
static LogSystemStatus_t LogSystem_HandleCircularOverwrite(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  初始化日志系统 - 统一存储版本
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_Init(void)
{
    DEBUG_Printf("开始初始化统一存储日志系统...\r\n");
    
    /* 喂狗，防止初始化过程中看门狗复位 */
    HAL_IWDG_Refresh(&hiwdg);
    
    /* 初始化W25Q128 Flash */
    DEBUG_Printf("正在初始化W25Q128 Flash驱动...\r\n");
    if (W25Q128_Init() != W25Q128_OK) {
        DEBUG_Printf("日志系统初始化失败：Flash初始化失败\r\n");
        DEBUG_Printf("请检查：\r\n");
        DEBUG_Printf("1. W25Q128芯片连接\r\n");
        DEBUG_Printf("2. SPI2接口配置\r\n");
        DEBUG_Printf("3. CS引脚是否接地\r\n");
        return LOG_SYSTEM_ERROR;
    }
    DEBUG_Printf("Flash驱动初始化成功\r\n");
    
    /* 喂狗 */
    HAL_IWDG_Refresh(&hiwdg);
    
    /* 初始化统一日志管理器 */
    DEBUG_Printf("初始化统一存储管理器...\r\n");
    memset(&log_manager, 0, sizeof(LogManager_t));
    
    log_manager.start_address = LOG_SYSTEM_START_ADDR;
    log_manager.end_address = LOG_SYSTEM_END_ADDR;
    log_manager.total_size = LOG_SYSTEM_SIZE;
    log_manager.max_entries = LOG_MAX_ENTRIES;
    log_manager.write_address = LOG_SYSTEM_START_ADDR;
    log_manager.read_address = LOG_SYSTEM_START_ADDR;
    log_manager.used_size = 0;
    log_manager.total_entries = 0;
    log_manager.oldest_entry_index = 0;
    log_manager.newest_entry_index = 0;
    log_manager.is_full = 0;
    log_manager.is_initialized = 0;
    log_manager.total_erase_count = 0;
    log_manager.total_write_count = 0;
    log_manager.total_reset_count = 0;
    log_manager.health_percentage = 100;
    
    /* 查找最后写入位置 */
    DEBUG_Printf("扫描存储区域，查找最后写入位置...\r\n");
    LogSystemStatus_t status = LogSystem_FindLastWritePosition();
    if (status != LOG_SYSTEM_OK) {
        DEBUG_Printf("定位最后写入位置失败，重置为起始位置\r\n");
        log_manager.write_address = LOG_SYSTEM_START_ADDR;
        log_manager.total_entries = 0;
    }
    
    /* 更新健康度 */
    DEBUG_Printf("正在更新健康度...\r\n");
    LogSystem_UpdateHealthPercentage();
    
    /* 标记初始化完成 */
    log_manager.is_initialized = 1;
    
    /* 输出初始化统计信息 */
    DEBUG_Printf("统一存储日志系统初始化成功:\r\n");
    DEBUG_Printf("- 总容量: %d MB (健康度:%d%%)\r\n", 
                 LOG_SYSTEM_SIZE / (1024 * 1024), 
                 log_manager.health_percentage);
    DEBUG_Printf("- 存储区域: 0x%08X - 0x%08X\r\n", 
                 log_manager.start_address, log_manager.end_address);
    DEBUG_Printf("- 当前写入位置: 0x%08X\r\n", log_manager.write_address);
    DEBUG_Printf("- 已有日志: %d 条\r\n", log_manager.total_entries);
    DEBUG_Printf("- 最大容量: %d 条\r\n", log_manager.max_entries);
    
    /* 记录系统启动日志 */
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "统一存储日志系统启动");
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  记录日志条目 - 新接口
  * @param  category: 日志分类
  * @param  channel: 通道编号
  * @param  event_code: 事件代码
  * @param  message: 日志消息
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_Record(LogCategory_t category, uint8_t channel, uint16_t event_code, const char* message)
{
    LogEntry_t entry = {0};
    
    /* 检查初始化状态 */
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    /* 验证分类 */
    if (!LogSystem_ValidateCategory(category)) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* 填充日志条目 */
    entry.timestamp = HAL_GetTick();
    entry.log_category = category;
    entry.channel = channel;
    entry.event_code = event_code;
    
    /* 安全复制消息 */
    if (message) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
    }
    
    /* 计算校验和 */
    entry.checksum = LogSystem_CalculateChecksum(&entry);
    entry.magic_number = LOG_MAGIC_NUMBER;
    
    /* 写入到对应分类 */
    return LogSystem_WriteEntry(&entry);
}

/**
  * @brief  格式化记录日志条目
  * @param  category: 日志分类
  * @param  channel: 通道编号
  * @param  event_code: 事件代码
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_RecordFormatted(LogCategory_t category, uint8_t channel, uint16_t event_code, const char* format, ...)
{
    char formatted_message[48];
    va_list args;
    
    /* 格式化消息 */
    va_start(args, format);
    vsnprintf(formatted_message, sizeof(formatted_message), format, args);
    va_end(args);
    
    /* 调用标准记录函数 */
    return LogSystem_Record(category, channel, event_code, formatted_message);
}

/**
  * @brief  将日志条目写入指定分类
  * @param  entry: 日志条目指针
  * @retval LogSystemStatus_t 操作状态
  */
static LogSystemStatus_t LogSystem_WriteEntry(const LogEntry_t* entry)
{
    if (!entry) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* 检查是否需要擦除新扇区 */
    if ((log_manager.write_address % LOG_SECTOR_SIZE) == 0) {
        LogSystemStatus_t status = LogSystem_PrepareNewSector(log_manager.write_address);
        if (status != LOG_SYSTEM_OK) {
            DEBUG_Printf("日志系统准备新扇区失败\r\n");
            return status;
        }
    }
    
    /* 写入Flash */
    if (W25Q128_WriteData(log_manager.write_address, (uint8_t*)entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
        DEBUG_Printf("日志系统写入Flash失败，地址:0x%08X\r\n", log_manager.write_address);
        return LOG_SYSTEM_ERROR;
    }
    
    /* 更新管理器状态 */
    log_manager.write_address += LOG_ENTRY_SIZE;
    log_manager.total_write_count++;
    
    /* 检查是否需要循环覆盖 */
    if (log_manager.write_address > log_manager.end_address) {
        log_manager.write_address = log_manager.start_address; // 回到起始位置
        log_manager.is_full = 1;
        
        DEBUG_Printf("日志系统触发循环覆盖，回到起始位置\r\n");
        
        /* 处理循环覆盖逻辑 */
        LogSystem_HandleCircularOverwrite();
    } else if (!log_manager.is_full) {
        log_manager.total_entries++;
        log_manager.used_size += LOG_ENTRY_SIZE;
        log_manager.newest_entry_index = log_manager.total_entries;
    } else {
        /* 已满状态下，更新最新索引 */
        log_manager.newest_entry_index++;
        if (log_manager.newest_entry_index >= log_manager.max_entries) {
            log_manager.newest_entry_index = 0;
        }
        
        /* 处理循环覆盖逻辑 */
        LogSystem_HandleCircularOverwrite();
    }
    
    /* 对于SAFETY分类，输出详细信息 */
    if (entry->log_category == LOG_CATEGORY_SAFETY) {
        DEBUG_Printf("SAFETY日志记录: [%s] %s\r\n", 
                     LogSystem_GetEventCodeString(entry->event_code), 
                     entry->message);
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  为指定分类准备新扇区
  * @param  address: 扇区地址
  * @retval LogSystemStatus_t 操作状态
  */
static LogSystemStatus_t LogSystem_PrepareNewSector(uint32_t address)
{
    /* 检查地址有效性 */
    if (address < log_manager.start_address || address > log_manager.end_address) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* 检查地址是否在扇区边界 */
    if ((address % LOG_SECTOR_SIZE) != 0) {
        address = (address / LOG_SECTOR_SIZE) * LOG_SECTOR_SIZE;
    }
    
    DEBUG_Printf("准备日志系统新扇区: 0x%08X\r\n", address);
    
    /* 擦除扇区 */
    if (W25Q128_EraseSector(address) != W25Q128_OK) {
        DEBUG_Printf("日志系统擦除扇区失败: 0x%08X\r\n", address);
        return LOG_SYSTEM_ERROR;
    }
    
    /* 更新擦除计数 */
    log_manager.total_erase_count++;
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  输出所有日志
  * @param  format: 输出格式
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_OutputAll(LogFormat_t format)
{
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    LogSystem_OutputHeader();
    
    DEBUG_Printf("========== 日志系统完整输出 ==========\r\n");
    
    /* 按时间顺序输出所有日志 */
    uint32_t entries_to_read = log_manager.is_full ? log_manager.max_entries : log_manager.total_entries;
    
    for (uint32_t i = 0; i < entries_to_read; i++) {
        LogEntry_t entry;
        uint32_t actual_index = log_manager.is_full ? 
            ((log_manager.oldest_entry_index + i) % log_manager.max_entries) : i;
        uint32_t entry_addr = LogSystem_GetEntryAddress(actual_index);
        
        if (W25Q128_ReadData(entry_addr, (uint8_t*)&entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
            DEBUG_Printf("读取失败，地址:0x%08X\r\n", entry_addr);
            break;
        }
        
        /* 验证条目 */
        if (LogSystem_VerifyEntry(&entry) && entry.log_category > 0 && entry.log_category <= 4) {
            /* 格式化输出 */
            char time_str[20];
            LogSystem_FormatTimestamp(entry.timestamp, time_str, sizeof(time_str));
            
            switch (format) {
                case LOG_FORMAT_SIMPLE:
                    DEBUG_Printf("[%s] %s\r\n", time_str, entry.message);
                    break;
                    
                case LOG_FORMAT_DETAILED:
                    DEBUG_Printf("[%s] %s|CH%d|0x%04X: %s\r\n",
                                time_str, 
                                log_category_strings[entry.log_category],
                                entry.channel,
                                entry.event_code,
                                entry.message);
                    break;
                    
                case LOG_FORMAT_CSV:
                    DEBUG_Printf("%s,%s,%d,0x%04X,%s\r\n",
                                time_str,
                                log_category_strings[entry.log_category],
                                entry.channel,
                                entry.event_code,
                                entry.message);
                    break;
            }
        }
        
        /* 定期喂狗 */
        if ((i % 100) == 0) {
            HAL_IWDG_Refresh(&hiwdg);
        }
    }
    
    LogSystem_OutputFooter();
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  输出指定分类的日志
  * @param  category: 日志分类
  * @param  format: 输出格式
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_OutputByCategory(LogCategory_t category, LogFormat_t format)
{
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    if (!LogSystem_ValidateCategory(category)) {
        return LOG_SYSTEM_ERROR;
    }
    
    uint32_t entries_to_read = log_manager.is_full ? log_manager.max_entries : log_manager.total_entries;
    
    if (entries_to_read == 0) {
        DEBUG_Printf("分类 %s 无日志记录\r\n", log_category_strings[category]);
        return LOG_SYSTEM_OK;
    }
    
    DEBUG_Printf("输出分类 %s 日志...\r\n", log_category_strings[category]);
    
    for (uint32_t i = 0; i < entries_to_read; i++) {
        LogEntry_t entry;
        uint32_t actual_index = log_manager.is_full ? 
            ((log_manager.oldest_entry_index + i) % log_manager.max_entries) : i;
        uint32_t entry_addr = LogSystem_GetEntryAddress(actual_index);
        
        if (W25Q128_ReadData(entry_addr, (uint8_t*)&entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
            DEBUG_Printf("读取失败，地址:0x%08X\r\n", entry_addr);
            break;
        }
        
        /* 验证条目 */
        if (LogSystem_VerifyEntry(&entry) && entry.log_category == category) {
            /* 格式化输出 */
            char time_str[20];
            LogSystem_FormatTimestamp(entry.timestamp, time_str, sizeof(time_str));
            
            switch (format) {
                case LOG_FORMAT_SIMPLE:
                    DEBUG_Printf("[%s] %s\r\n", time_str, entry.message);
                    break;
                    
                case LOG_FORMAT_DETAILED:
                    DEBUG_Printf("[%s] %s|CH%d|0x%04X: %s\r\n",
                                time_str, 
                                log_category_strings[category],
                                entry.channel,
                                entry.event_code,
                                entry.message);
                    break;
                    
                case LOG_FORMAT_CSV:
                    DEBUG_Printf("%s,%s,%d,0x%04X,%s\r\n",
                                time_str,
                                log_category_strings[category],
                                entry.channel,
                                entry.event_code,
                                entry.message);
                    break;
            }
        }
        
        /* 定期喂狗 */
        if ((i % 100) == 0) {
            HAL_IWDG_Refresh(&hiwdg);
        }
    }
    
    DEBUG_Printf("分类 %s 输出完成，共 %d 条记录\r\n", 
                 log_category_strings[category], entries_to_read);
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  获取日志总数
  * @param  None
  * @retval 日志总数
  */
uint32_t LogSystem_GetEntryCount(void)
{
    return log_manager.total_entries;
}

/**
  * @brief  获取已使用容量
  * @param  None
  * @retval 已使用字节数
  */
uint32_t LogSystem_GetUsedSize(void)
{
    return log_manager.used_size;
}

/**
  * @brief  获取健康度百分比
  * @param  None
  * @retval 健康度百分比
  */
uint8_t LogSystem_GetHealthPercentage(void)
{
    return log_manager.health_percentage;
}

/**
  * @brief  检查是否已满
  * @param  None
  * @retval 1:已满 0:未满
  */
uint8_t LogSystem_IsFull(void)
{
    return log_manager.is_full;
}

/**
  * @brief  检查是否已初始化
  * @param  None
  * @retval 1:已初始化 0:未初始化
  */
uint8_t LogSystem_IsInitialized(void)
{
    return log_manager.is_initialized;
}

/**
  * @brief  更新健康度百分比
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_UpdateHealthPercentage(void)
{
    /* 简化的健康度计算：基于擦除次数 */
    if (log_manager.total_erase_count < LOG_MAX_ERASE_CYCLES / 10) {
        log_manager.health_percentage = 100;
    } else if (log_manager.total_erase_count < LOG_MAX_ERASE_CYCLES / 2) {
        log_manager.health_percentage = 80;
    } else if (log_manager.total_erase_count < LOG_MAX_ERASE_CYCLES) {
        log_manager.health_percentage = 50;
    } else {
        log_manager.health_percentage = 20;
    }
    
    /* 健康度警告 */
    if (log_manager.health_percentage <= LOG_HEALTH_WARNING_THRESHOLD) {
        oled_show_flash_warning = 1;
        DEBUG_Printf("Flash健康度警告: %d%%\r\n", log_manager.health_percentage);
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  计算条目校验和
  * @param  entry: 日志条目指针
  * @retval 校验和
  */
uint32_t LogSystem_CalculateChecksum(const LogEntry_t* entry)
{
    if (!entry) return 0;
    
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)entry;
    
    /* 跳过checksum和magic_number字段 */
    for (uint32_t i = 0; i < sizeof(LogEntry_t) - 8; i++) {
        checksum += data[i];
    }
    
    return checksum;
}

/**
  * @brief  验证条目有效性
  * @param  entry: 日志条目指针
  * @retval 1:有效 0:无效
  */
uint8_t LogSystem_VerifyEntry(const LogEntry_t* entry)
{
    if (!entry) return 0;
    
    /* 检查魔数 */
    if (entry->magic_number != LOG_MAGIC_NUMBER) {
        return 0;
    }
    
    /* 检查校验和 */
    uint32_t calculated_checksum = LogSystem_CalculateChecksum(entry);
    if (calculated_checksum != entry->checksum) {
        return 0;
    }
    
    /* 检查分类有效性 */
    if (!LogSystem_ValidateCategory((LogCategory_t)entry->log_category)) {
        return 0;
    }
    
    return 1;
}

/**
  * @brief  格式化时间戳
  * @param  timestamp: 时间戳
  * @param  buffer: 输出缓冲区
  * @param  buffer_size: 缓冲区大小
  * @retval None
  */
void LogSystem_FormatTimestamp(uint32_t timestamp, char* buffer, uint8_t buffer_size)
{
    if (!buffer || buffer_size < 10) return;
    
    uint32_t seconds = timestamp / 1000;
    uint32_t milliseconds = timestamp % 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    snprintf(buffer, buffer_size, "%02d:%02d:%02d.%03d", 
             (int)hours, (int)minutes, (int)seconds, (int)milliseconds);
}

/**
  * @brief  输出日志头
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_OutputHeader(void)
{
    DEBUG_Printf("\r\n==================== 日志系统输出 ====================\r\n");
    DEBUG_Printf("时间戳        | 分类      | CH | 事件 | 消息\r\n");
    DEBUG_Printf("--------------------------------------------------------\r\n");
    return LOG_SYSTEM_OK;
}

/**
  * @brief  输出日志尾
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_OutputFooter(void)
{
    DEBUG_Printf("--------------------------------------------------------\r\n");
    DEBUG_Printf("总条目数: %d, 已用空间: %d KB, 健康度: %d%%\r\n", 
                 LogSystem_GetEntryCount(),
                 LogSystem_GetUsedSize() / 1024,
                 LogSystem_GetHealthPercentage());
    DEBUG_Printf("=====================================================\r\n\r\n");
    return LOG_SYSTEM_OK;
}

/**
  * @brief  兼容函数：查找最后写入位置
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_FindLastWritePosition(void)
{
    uint32_t address = log_manager.start_address;
    uint32_t entry_count = 0;
    LogEntry_t temp_entry;
    LogResetMarker_t reset_marker;
    
    DEBUG_Printf("扫描日志系统存储区域...\r\n");
    
    /* 首先检查是否存在重置标记 */
    if (W25Q128_ReadData(log_manager.start_address, (uint8_t*)&reset_marker, LOG_RESET_MARKER_SIZE) == W25Q128_OK) {
        if (reset_marker.reset_magic == LOG_RESET_MARKER_MAGIC && reset_marker.final_magic == LOG_MAGIC_NUMBER) {
            DEBUG_Printf("发现重置标记，时间戳:%u，重置次数:%u\r\n", 
                         reset_marker.reset_timestamp, reset_marker.reset_count);
            DEBUG_Printf("重置原因: %s\r\n", reset_marker.reset_reason);
            
            /* 更新管理器状态为空状态 */
            log_manager.write_address = log_manager.start_address;  // 新日志将覆盖重置标记
            log_manager.total_entries = 0;
            log_manager.used_size = 0;
            log_manager.total_reset_count = reset_marker.reset_count;
            
            DEBUG_Printf("已从重置状态恢复，下次写入将覆盖重置标记\r\n");
            return LOG_SYSTEM_OK;
        }
    }
    
    DEBUG_Printf("未发现重置标记，开始完整扫描...\r\n");
    
    /* 扫描日志系统的存储区域 */
    while (address + LOG_ENTRY_SIZE <= log_manager.end_address + 1) {
        /* 定期喂狗 */
        if ((entry_count % 1000) == 0) {
            HAL_IWDG_Refresh(&hiwdg);
        }
        
        /* 读取条目 */
        if (W25Q128_ReadData(address, (uint8_t*)&temp_entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
            DEBUG_Printf("日志系统读取失败，地址:0x%08X\r\n", address);
            break;
        }
        
        /* 检查条目有效性 */
        if (!LogSystem_VerifyEntry(&temp_entry) || temp_entry.log_category < LOG_CATEGORY_SAFETY || temp_entry.log_category > LOG_CATEGORY_MONITOR) {
            /* 找到第一个无效条目，这里就是写入位置 */
            break;
        }
        
        /* 继续下一条目 */
        address += LOG_ENTRY_SIZE;
        entry_count++;
        
        /* 防止扫描超时 */
        if (entry_count > log_manager.max_entries) {
            DEBUG_Printf("日志系统扫描条目数超限，可能存储区已满\r\n");
            log_manager.is_full = 1;
            break;
        }
    }
    
    /* 更新管理器状态 */
    log_manager.write_address = address;
    log_manager.total_entries = entry_count;
    log_manager.used_size = entry_count * LOG_ENTRY_SIZE;
    
    /* 检查是否已满 */
    if (log_manager.write_address >= log_manager.end_address + 1) {
        log_manager.is_full = 1;
        log_manager.write_address = log_manager.start_address; // 循环覆盖，回到起始位置
        DEBUG_Printf("日志系统存储区已满，启用循环覆盖模式\r\n");
    }
    
    DEBUG_Printf("日志系统扫描完成: 找到 %d 条日志，写入位置:0x%08X\r\n",
                 entry_count, log_manager.write_address);
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  获取日志总数
  * @param  None
  * @retval 日志总数
  */
uint32_t LogSystem_GetLogCount(void)
{
    return LogSystem_GetEntryCount();
}

/**
  * @brief  获取指定分类的日志总数
  * @param  category: 日志分类
  * @retval 该分类的日志总数
  */
uint32_t LogSystem_GetCategoryLogCount(LogCategory_t category)
{
    if (!LogSystem_ValidateCategory(category)) {
        return 0;
    }
    
    uint32_t count = 0;
    uint32_t start_index = log_manager.oldest_entry_index;
    uint32_t end_index = log_manager.newest_entry_index;
    
    for (uint32_t i = start_index; i < end_index; i++) {
        LogEntry_t entry;
        uint32_t entry_addr = LogSystem_GetEntryAddress(i);
        
        if (W25Q128_ReadData(entry_addr, (uint8_t*)&entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
            DEBUG_Printf("读取失败，地址:0x%08X\r\n", entry_addr);
            break;
        }
        
        if (entry.log_category == category) {
            count++;
        }
        
        /* 定期喂狗 */
        if ((i % 100) == 0) {
            HAL_IWDG_Refresh(&hiwdg);
        }
    }
    
    return count;
}

/**
  * @brief  读取指定索引的日志条目
  * @param  entry_index: 日志条目索引
  * @param  entry: 日志条目指针
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_ReadEntry(uint32_t entry_index, LogEntry_t* entry)
{
    if (!entry) {
        return LOG_SYSTEM_ERROR;
    }
    
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    /* 检查索引有效性 */
    if (entry_index >= log_manager.total_entries) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* 计算实际地址 */
    uint32_t entry_addr = LogSystem_GetEntryAddress(entry_index);
    
    /* 从Flash读取数据 */
    if (W25Q128_ReadData(entry_addr, (uint8_t*)entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* 验证读取的条目 */
    if (!LogSystem_VerifyEntry(entry)) {
        return LOG_SYSTEM_ERROR;
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  格式化日志系统（完全擦除所有数据）
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_Format(void)
{
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    // 暂停日志写入
    log_manager.is_initialized = 0;
    
    // 计算需要擦除的扇区数量
    uint32_t sector_size = 4096;  // 4KB扇区
    uint32_t start_sector = log_manager.start_address / sector_size;
    uint32_t end_sector = log_manager.end_address / sector_size;
    
    // 逐个擦除扇区
    for (uint32_t sector = start_sector; sector <= end_sector; sector++) {
        uint32_t sector_addr = sector * sector_size;
        
        // 使用W25Q128的扇区擦除命令
        if (W25Q128_EraseSector(sector_addr) != W25Q128_OK) {
            // 恢复初始化状态
            log_manager.is_initialized = 1;
            return LOG_SYSTEM_ERROR;
        }
        
        // 喂狗防止擦除过程中复位
        HAL_IWDG_Refresh(&hiwdg);
    }
    
    // 重新初始化系统
    return LogSystem_Init();
}

/**
  * @brief  输出单条日志记录
  * @param  index: 日志索引
  * @param  format: 输出格式
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_OutputSingle(uint32_t index, LogFormat_t format)
{
    if (!log_manager.is_initialized) {
        DEBUG_Printf("日志系统未初始化\r\n");
        return LOG_SYSTEM_NOT_INIT;
    }
    
    // 检查索引有效性
    uint32_t total_entries = LogSystem_GetEntryCount();
    if (index >= total_entries) {
        DEBUG_Printf("索引超出范围: %d >= %d\r\n", index, total_entries);
        return LOG_SYSTEM_ERROR;
    }
    
    LogEntry_t entry;
    char time_str[32];
    
    // 读取日志条目
    uint32_t entry_addr = log_manager.start_address + (index * LOG_ENTRY_SIZE);
    if (W25Q128_ReadData(entry_addr, (uint8_t*)&entry, sizeof(LogEntry_t)) != W25Q128_OK) {
        DEBUG_Printf("读取日志失败: 索引 %d\r\n", index);
        return LOG_SYSTEM_ERROR;
    }
    
    // 验证日志条目
    if (entry.magic_number != LOG_MAGIC_NUMBER || entry.log_category > 3) {
        DEBUG_Printf("无效日志条目: 索引 %d\r\n", index);
        return LOG_SYSTEM_ERROR;
    }
    
    // 格式化时间戳
    LogSystem_FormatTimestamp(entry.timestamp, time_str, sizeof(time_str));
    
    // 根据格式输出
    if (format == LOG_FORMAT_SIMPLE) {
        DEBUG_Printf("[%d] %s %s\r\n",
                    index, time_str, entry.message);
    } else {
        DEBUG_Printf("[%05d] %s | %s | CH:%d | EVT:0x%04X | %s\r\n",
                    index, time_str, log_category_strings[entry.log_category],
                    entry.channel, entry.event_code, entry.message);
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  重置日志系统（快速清空日志）
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_Reset(void)
{
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    DEBUG_Printf("开始快速清空日志系统...\r\n");
    
    /* 创建重置标记 */
    LogResetMarker_t reset_marker = {0};
    reset_marker.reset_magic = LOG_RESET_MARKER_MAGIC;
    reset_marker.reset_timestamp = HAL_GetTick();
    reset_marker.reset_count = log_manager.total_reset_count + 1;
    reset_marker.reserved1 = 0;
    strncpy(reset_marker.reset_reason, "User manual reset via KEY2", sizeof(reset_marker.reset_reason) - 1);
    reset_marker.reset_reason[sizeof(reset_marker.reset_reason) - 1] = '\0';
    reset_marker.reserved2 = 0;
    reset_marker.reserved3 = 0;
    reset_marker.final_magic = LOG_MAGIC_NUMBER;
    
    /* 计算重置标记的校验和 */
    uint32_t checksum = 0;
    uint8_t* data = (uint8_t*)&reset_marker;
    for (int i = 0; i < sizeof(LogResetMarker_t) - 8; i++) { // 排除checksum和final_magic
        checksum += data[i];
    }
    reset_marker.checksum = checksum;
    
    /* 检查是否需要擦除第一个扇区 */
    if ((log_manager.start_address % LOG_SECTOR_SIZE) == 0) {
        DEBUG_Printf("擦除第一个扇区用于写入重置标记...\r\n");
        if (W25Q128_EraseSector(log_manager.start_address) != W25Q128_OK) {
            DEBUG_Printf("擦除第一个扇区失败\r\n");
            return LOG_SYSTEM_ERROR;
        }
        log_manager.total_erase_count++;
    }
    
    /* 写入重置标记到Flash开头 */
    if (W25Q128_WriteData(log_manager.start_address, (uint8_t*)&reset_marker, LOG_RESET_MARKER_SIZE) != W25Q128_OK) {
        DEBUG_Printf("写入重置标记失败\r\n");
        return LOG_SYSTEM_ERROR;
    }
    
    /* 更新管理结构为空状态 */
    log_manager.write_address = log_manager.start_address;
    log_manager.read_address = log_manager.start_address;
    log_manager.used_size = 0;
    log_manager.total_entries = 0;
    log_manager.oldest_entry_index = 0;
    log_manager.newest_entry_index = 0;
    log_manager.is_full = 0;
    log_manager.total_write_count = 0;
    log_manager.total_reset_count++;
    
    /* 保持初始化状态和健康度不变 */
    
    DEBUG_Printf("日志系统已真正清空：\r\n");
    DEBUG_Printf("- 重置标记已写入Flash，时间戳: %u\r\n", reset_marker.reset_timestamp);
    DEBUG_Printf("- 重置次数: %u\r\n", log_manager.total_reset_count);
    DEBUG_Printf("- 写入位置重置为: 0x%08X\r\n", log_manager.write_address);
    DEBUG_Printf("- 系统重启后将从重置标记恢复空状态\r\n");
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  获取指定索引的日志条目地址
  * @param  entry_index: 日志条目索引
  * @retval 日志条目地址
  */
static uint32_t LogSystem_GetEntryAddress(uint32_t entry_index)
{
    return log_manager.start_address + (entry_index * LOG_ENTRY_SIZE);
}

/**
  * @brief  处理循环覆盖逻辑
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
static LogSystemStatus_t LogSystem_HandleCircularOverwrite(void)
{
    if (!log_manager.is_full) {
        return LOG_SYSTEM_OK;  // 尚未满，无需循环覆盖
    }
    
    /* 在循环覆盖模式下，确保oldest_entry_index始终指向最旧的有效记录 */
    /* 当newest_entry_index向前移动时，oldest_entry_index也要相应调整 */
    
    /* 计算当前写入的条目索引 */
    uint32_t current_entry_index = (log_manager.write_address - log_manager.start_address) / LOG_ENTRY_SIZE;
    
    /* 在循环模式下，oldest_entry_index应该跟随写入位置 */
    log_manager.oldest_entry_index = current_entry_index;
    
    /* 确保索引不会超出范围 */
    if (log_manager.oldest_entry_index >= log_manager.max_entries) {
        log_manager.oldest_entry_index = log_manager.oldest_entry_index % log_manager.max_entries;
    }
    
    /* 在已满状态下，总条目数保持为最大值 */
    log_manager.total_entries = log_manager.max_entries;
    log_manager.used_size = log_manager.max_entries * LOG_ENTRY_SIZE;
    
    DEBUG_Printf("循环覆盖: oldest_index=%d, newest_index=%d, current_pos=0x%08X\r\n",
                 log_manager.oldest_entry_index, 
                 log_manager.newest_entry_index,
                 log_manager.write_address);
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  验证分类有效性
  * @param  category: 日志分类
  * @retval 1:有效 0:无效
  */
uint8_t LogSystem_ValidateCategory(LogCategory_t category)
{
    return (category == LOG_CATEGORY_SAFETY || category == LOG_CATEGORY_SYSTEM || category == LOG_CATEGORY_MONITOR);
}

/**
  * @brief  获取分类字符串
  * @param  category: 日志分类
  * @retval 分类字符串
  */
const char* LogSystem_GetCategoryString(uint8_t category)
{
    if (category >= 1 && category <= 3) {
        return log_category_strings[category];
    }
    return "INVALID";
}

/**
  * @brief  获取事件代码字符串
  * @param  event_code: 事件代码
  * @retval 事件代码字符串
  */
const char* LogSystem_GetEventCodeString(uint16_t event_code)
{
    /* 根据事件代码范围返回描述 */
    if (event_code >= 0x1000 && event_code <= 0x1FFF) {
        if (event_code <= 0x100F) return "安全异常";
        if (event_code <= 0x102F) return "系统保护";
        if (event_code <= 0x103F) return "异常解除";
    } else if (event_code >= 0x3000 && event_code <= 0x3FFF) {
        if (event_code <= 0x300F) return "系统生命周期";
    } else if (event_code >= 0x4000 && event_code <= 0x4FFF) {
        if (event_code <= 0x403F) return "系统健康";
    }
    
    return "未知事件";
}

/**
  * @brief  反初始化日志系统
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_DeInit(void)
{
    /* 记录系统停止日志 */
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_STOP, "统一存储日志系统停止");
    
    /* 等待所有写入完成 */
    SmartDelay(100);
    
    /* 反初始化Flash */
    W25Q128_DeInit();
    
    /* 清除标志 */
    log_manager.is_initialized = 0;
    
    return LOG_SYSTEM_OK;
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */ 


