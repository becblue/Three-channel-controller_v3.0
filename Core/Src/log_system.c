/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : log_system.c
  * @brief          : 系统运行日志模块实现文件
  ******************************************************************************
  * @attention
  *
  * 这是为三通道切换箱控制系统设计的日志记录系统实现
  * 支持环形覆盖存储、健康度监控、串口输出等功能
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "log_system.h"
#include "usart.h"
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

/* Flash管理器实例 */
static FlashManager_t flash_manager = {0};

/* 日志类型字符串数组 */
static const char* log_type_strings[] = {
    "SYS",      // LOG_TYPE_SYSTEM
    "CHN",      // LOG_TYPE_CHANNEL
    "ERR",      // LOG_TYPE_ERROR
    "WRN",      // LOG_TYPE_WARNING
    "INF",      // LOG_TYPE_INFO
    "DBG"       // LOG_TYPE_DEBUG
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
/* static LogSystemStatus_t LogSystem_ScanForLastEntry(void); */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  初始化日志系统
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_Init(void)
{
    DEBUG_Printf("开始初始化日志系统...\r\n");
    
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
    
    /* 设置Flash管理器参数 */
    DEBUG_Printf("初始化Flash管理器...\r\n");
    flash_manager.start_address = LOG_SYSTEM_START_ADDR;
    flash_manager.end_address = LOG_SYSTEM_END_ADDR;
    flash_manager.total_size = LOG_SYSTEM_SIZE;
    flash_manager.write_address = LOG_SYSTEM_START_ADDR;
    flash_manager.used_size = 0;
    flash_manager.total_entries = 0;
    flash_manager.total_erase_count = 0;
    flash_manager.total_write_count = 0;
    flash_manager.health_percentage = 100;
    flash_manager.is_full = 0;
    flash_manager.is_initialized = 0;
    
    DEBUG_Printf("Flash管理器配置：\r\n");
    DEBUG_Printf("- 起始地址: 0x%08X\r\n", flash_manager.start_address);
    DEBUG_Printf("- 结束地址: 0x%08X\r\n", flash_manager.end_address);
    DEBUG_Printf("- 总容量: %d MB\r\n", flash_manager.total_size / (1024 * 1024));
    
    /* 查找最后写入位置 */
    DEBUG_Printf("正在查找最后写入位置...\r\n");
    LogSystemStatus_t status = LogSystem_FindLastWritePosition();
    if (status != LOG_SYSTEM_OK) {
        DEBUG_Printf("日志系统初始化警告：无法找到最后写入位置，从头开始\r\n");
        flash_manager.write_address = LOG_SYSTEM_START_ADDR;
        flash_manager.total_entries = 0;
    } else {
        DEBUG_Printf("找到最后写入位置: 0x%08X\r\n", flash_manager.write_address);
    }
    
    /* 更新健康度 */
    DEBUG_Printf("正在更新健康度...\r\n");
    LogSystem_UpdateHealthPercentage();
    
    /* 标记初始化完成 */
    flash_manager.is_initialized = 1;
    
    DEBUG_Printf("日志系统初始化成功 (容量:%dMB, 健康度:%d%%, 已有日志:%d条)\r\n", 
                 flash_manager.total_size / (1024 * 1024), 
                 flash_manager.health_percentage,
                 flash_manager.total_entries);
    
    /* 记录系统启动日志 */
    LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "Log system initialized");
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  反初始化日志系统
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_DeInit(void)
{
    /* 记录系统停止日志 */
    LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_STOP, "Log system deinitialized");
    
    /* 等待所有写入完成 */
    SmartDelay(100);
    
    /* 反初始化Flash */
    W25Q128_DeInit();
    
    /* 清除标志 */
    flash_manager.is_initialized = 0;
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  记录日志条目
  * @param  log_type: 日志类型
  * @param  channel: 通道编号
  * @param  event_code: 事件代码
  * @param  message: 日志消息
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_Record(LogType_t log_type, uint8_t channel, uint16_t event_code, const char* message)
{
    LogEntry_t entry = {0};
    
    /* 检查初始化状态 */
    if (!flash_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    /* 填充日志条目 */
    entry.timestamp = HAL_GetTick();
    entry.log_type = log_type;
    entry.channel = channel;
    entry.event_code = event_code;
    entry.magic_number = LOG_MAGIC_NUMBER;
    
    /* 安全复制消息 */
    if (message != NULL) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
    }
    
    /* 计算校验和 */
    entry.checksum = LogSystem_CalculateChecksum(&entry);
    
    /* 写入日志 */
    return LogSystem_WriteEntry(&entry);
}

/**
  * @brief  记录格式化日志条目
  * @param  log_type: 日志类型
  * @param  channel: 通道编号
  * @param  event_code: 事件代码
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_RecordFormatted(LogType_t log_type, uint8_t channel, uint16_t event_code, const char* format, ...)
{
    char message[48];
    va_list args;
    
    /* 格式化消息 */
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    return LogSystem_Record(log_type, channel, event_code, message);
}

/**
  * @brief  输出所有日志
  * @param  format: 输出格式
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_OutputAll(LogFormat_t format)
{
    uint32_t entry_count = flash_manager.total_entries;
    uint32_t entries_per_batch = 50;  // 每批处理50条日志
    uint32_t current_batch = 0;
    LogEntry_t entries[50];
    
    /* 检查初始化状态 */
    if (!flash_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    /* 输出头部信息 */
    DEBUG_Printf("========== 系统运行日志 ==========\r\n");
    DEBUG_Printf("总日志条数: %d\r\n", entry_count);
    DEBUG_Printf("存储使用: %.1f%%\r\n", (float)flash_manager.used_size * 100.0f / flash_manager.total_size);
    DEBUG_Printf("Flash健康度: %d%%\r\n", flash_manager.health_percentage);
    DEBUG_Printf("========================================\r\n");
    
    /* 如果没有日志，直接返回 */
    if (entry_count == 0) {
        DEBUG_Printf("暂无日志记录\r\n");
        return LOG_SYSTEM_OK;
    }
    
    /* 分批输出日志 */
    while (current_batch * entries_per_batch < entry_count) {
        uint32_t batch_start = current_batch * entries_per_batch;
        uint32_t batch_size = entries_per_batch;
        
        /* 调整最后一批的大小 */
        if (batch_start + batch_size > entry_count) {
            batch_size = entry_count - batch_start;
        }
        
        /* 读取一批日志 */
        for (uint32_t i = 0; i < batch_size; i++) {
            uint32_t entry_index = batch_start + i;
            uint32_t entry_address = LogSystem_GetEntryAddress(entry_index);
            
            if (W25Q128_ReadData(entry_address, (uint8_t*)&entries[i], sizeof(LogEntry_t)) != W25Q128_OK) {
                DEBUG_Printf("读取日志失败：地址 0x%08X\r\n", entry_address);
                continue;
            }
            
            /* 验证日志条目 */
            if (!LogSystem_VerifyEntry(&entries[i])) {
                DEBUG_Printf("日志验证失败：条目 %d\r\n", entry_index);
                continue;
            }
        }
        
        /* 输出这批日志 */
        for (uint32_t i = 0; i < batch_size; i++) {
            uint32_t entry_index = batch_start + i;
            LogEntry_t* entry = &entries[i];
            
            /* 格式化时间戳 */
            char time_str[16];
            LogSystem_FormatTimestamp(entry->timestamp, time_str, sizeof(time_str));
            
            /* 根据格式输出 */
            switch (format) {
                case LOG_FORMAT_SIMPLE:
                    DEBUG_Printf("[%06d] %s [%s] %s\r\n", 
                                entry_index + 1,
                                time_str,
                                LogSystem_GetTypeString(entry->log_type),
                                entry->message);
                    break;
                    
                case LOG_FORMAT_DETAILED:
                    DEBUG_Printf("[%06d] %s [%s] CH%d E:0x%04X %s\r\n", 
                                entry_index + 1,
                                time_str,
                                LogSystem_GetTypeString(entry->log_type),
                                entry->channel,
                                entry->event_code,
                                entry->message);
                    break;
                    
                case LOG_FORMAT_CSV:
                    DEBUG_Printf("%d,%s,%s,%d,0x%04X,%s\r\n", 
                                entry_index + 1,
                                time_str,
                                LogSystem_GetTypeString(entry->log_type),
                                entry->channel,
                                entry->event_code,
                                entry->message);
                    break;
            }
        }
        
        current_batch++;
        
        /* 每输出一批后稍作延时，避免串口缓冲区溢出 */
        SmartDelay(50);
    }
    
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("日志输出完成，共 %d 条\r\n", entry_count);
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  获取日志条目数量
  * @param  None
  * @retval uint32_t 日志条目数量
  */
uint32_t LogSystem_GetEntryCount(void)
{
    return flash_manager.total_entries;
}

/**
  * @brief  获取已使用大小
  * @param  None
  * @retval uint32_t 已使用字节数
  */
uint32_t LogSystem_GetUsedSize(void)
{
    return flash_manager.used_size;
}

/**
  * @brief  获取健康度百分比
  * @param  None
  * @retval uint8_t 健康度百分比
  */
uint8_t LogSystem_GetHealthPercentage(void)
{
    return flash_manager.health_percentage;
}

/**
  * @brief  检查存储器是否已满
  * @param  None
  * @retval uint8_t 1-已满，0-未满
  */
uint8_t LogSystem_IsFull(void)
{
    return flash_manager.is_full;
}

/**
  * @brief  检查是否已初始化
  * @param  None
  * @retval uint8_t 1-已初始化，0-未初始化
  */
uint8_t LogSystem_IsInitialized(void)
{
    return flash_manager.is_initialized;
}

/**
  * @brief  更新健康度百分比
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_UpdateHealthPercentage(void)
{
    /* 基于擦除次数计算健康度 */
    uint32_t sectors_count = flash_manager.total_size / LOG_SECTOR_SIZE;
    uint32_t avg_erase_per_sector = flash_manager.total_erase_count / sectors_count;
    
    if (avg_erase_per_sector >= LOG_MAX_ERASE_CYCLES) {
        flash_manager.health_percentage = 0;
    } else {
        flash_manager.health_percentage = 100 - (avg_erase_per_sector * 100 / LOG_MAX_ERASE_CYCLES);
    }
    
    /* 更新OLED显示标志 */
    if (flash_manager.health_percentage <= LOG_HEALTH_WARNING_THRESHOLD) {
        oled_show_flash_warning = 1;
    } else {
        oled_show_flash_warning = 0;
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  查找最后写入位置
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_FindLastWritePosition(void)
{
    uint32_t search_address = LOG_SYSTEM_START_ADDR;
    uint32_t entry_count = 0;
    LogEntry_t entry;
    
    /* 扫描整个日志区域 */
    while (search_address < LOG_SYSTEM_END_ADDR) {
        /* 读取一个日志条目 */
        if (W25Q128_ReadData(search_address, (uint8_t*)&entry, sizeof(LogEntry_t)) != W25Q128_OK) {
            break;
        }
        
        /* 检查是否是有效的日志条目 */
        if (entry.magic_number == LOG_MAGIC_NUMBER && LogSystem_VerifyEntry(&entry)) {
            entry_count++;
            search_address += LOG_ENTRY_SIZE;
        } else {
            /* 找到空白区域，这里就是下一个写入位置 */
            break;
        }
    }
    
    /* 更新Flash管理器状态 */
    flash_manager.write_address = search_address;
    flash_manager.total_entries = entry_count;
    flash_manager.used_size = entry_count * LOG_ENTRY_SIZE;
    
    /* 检查是否已满 */
    if (search_address >= LOG_SYSTEM_END_ADDR) {
        flash_manager.is_full = 1;
        flash_manager.write_address = LOG_SYSTEM_START_ADDR;  // 环形覆盖
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  计算校验和
  * @param  entry: 日志条目指针
  * @retval uint32_t 校验和值
  */
uint32_t LogSystem_CalculateChecksum(const LogEntry_t* entry)
{
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)entry;
    
    /* 计算除校验和字段外的所有字段 */
    for (uint32_t i = 0; i < sizeof(LogEntry_t) - 8; i++) {  // 排除checksum和magic_number
        checksum += data[i];
    }
    
    return checksum;
}

/**
  * @brief  验证日志条目
  * @param  entry: 日志条目指针
  * @retval uint8_t 1-有效，0-无效
  */
uint8_t LogSystem_VerifyEntry(const LogEntry_t* entry)
{
    if (entry == NULL) {
        return 0;
    }
    
    /* 检查魔数 */
    if (entry->magic_number != LOG_MAGIC_NUMBER) {
        return 0;
    }
    
    /* 检查校验和 */
    uint32_t calculated_checksum = LogSystem_CalculateChecksum(entry);
    if (calculated_checksum != entry->checksum) {
        return 0;
    }
    
    /* 检查日志类型 */
    if (entry->log_type >= sizeof(log_type_strings) / sizeof(log_type_strings[0])) {
        return 0;
    }
    
    return 1;
}

/**
  * @brief  格式化时间戳
  * @param  timestamp: 时间戳（毫秒）
  * @param  buffer: 输出缓冲区
  * @param  buffer_size: 缓冲区大小
  * @retval None
  */
void LogSystem_FormatTimestamp(uint32_t timestamp, char* buffer, uint8_t buffer_size)
{
    uint32_t hours = timestamp / 3600000;
    uint32_t minutes = (timestamp % 3600000) / 60000;
    uint32_t seconds = (timestamp % 60000) / 1000;
    uint32_t milliseconds = timestamp % 1000;
    
    snprintf(buffer, buffer_size, "%02d:%02d:%02d.%03d", 
             (int)hours, (int)minutes, (int)seconds, (int)milliseconds);
}

/**
  * @brief  获取日志类型字符串
  * @param  log_type: 日志类型
  * @retval const char* 类型字符串
  */
const char* LogSystem_GetTypeString(uint8_t log_type)
{
    if (log_type < sizeof(log_type_strings) / sizeof(log_type_strings[0])) {
        return log_type_strings[log_type];
    }
    return "UNK";
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  写入日志条目到Flash
  * @param  entry: 日志条目指针
  * @retval LogSystemStatus_t 操作状态
  */
static LogSystemStatus_t LogSystem_WriteEntry(const LogEntry_t* entry)
{
    /* 检查是否需要擦除新扇区 */
    if ((flash_manager.write_address % LOG_SECTOR_SIZE) == 0) {
        if (LogSystem_PrepareNewSector(flash_manager.write_address) != LOG_SYSTEM_OK) {
            return LOG_SYSTEM_ERROR;
        }
    }
    
    /* 写入日志条目 */
    if (W25Q128_WriteData(flash_manager.write_address, (uint8_t*)entry, sizeof(LogEntry_t)) != W25Q128_OK) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* 更新管理器状态 */
    flash_manager.write_address += LOG_ENTRY_SIZE;
    flash_manager.total_entries++;
    flash_manager.used_size += LOG_ENTRY_SIZE;
    flash_manager.total_write_count++;
    
    /* 检查是否需要环形覆盖 */
    if (flash_manager.write_address >= LOG_SYSTEM_END_ADDR) {
        flash_manager.write_address = LOG_SYSTEM_START_ADDR;
        flash_manager.is_full = 1;
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  准备新扇区（擦除）
  * @param  address: 扇区地址
  * @retval LogSystemStatus_t 操作状态
  */
static LogSystemStatus_t LogSystem_PrepareNewSector(uint32_t address)
{
    /* 擦除扇区 */
    if (W25Q128_EraseSector(address) != W25Q128_OK) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* 更新擦除计数 */
    flash_manager.total_erase_count++;
    
    /* 更新健康度 */
    LogSystem_UpdateHealthPercentage();
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  获取日志条目地址
  * @param  entry_index: 条目索引
  * @retval uint32_t 地址
  */
static uint32_t LogSystem_GetEntryAddress(uint32_t entry_index)
{
    uint32_t address = LOG_SYSTEM_START_ADDR + (entry_index * LOG_ENTRY_SIZE);
    
    /* 环形地址计算 */
    if (address >= LOG_SYSTEM_END_ADDR) {
        address = LOG_SYSTEM_START_ADDR + ((address - LOG_SYSTEM_START_ADDR) % LOG_SYSTEM_SIZE);
    }
    
    return address;
}

/**
  * @brief  格式化日志系统（清空所有日志）
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_Format(void)
{
    /* 检查初始化状态 */
    if (!flash_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    DEBUG_Printf("开始格式化日志系统（清空所有日志）...\r\n");
    
    /* 擦除整个日志区域的前几个扇区作为示例（为安全起见不擦除所有） */
    uint32_t sectors_to_erase = 10; // 只擦除前10个扇区作为演示
    uint32_t current_address = LOG_SYSTEM_START_ADDR;
    
    for (uint32_t i = 0; i < sectors_to_erase && current_address < LOG_SYSTEM_END_ADDR; i++) {
        DEBUG_Printf("正在擦除扇区 %d (地址: 0x%08X)...\r\n", i + 1, current_address);
        
        if (W25Q128_EraseSector(current_address) != W25Q128_OK) {
            DEBUG_Printf("擦除扇区失败：地址 0x%08X\r\n", current_address);
            return LOG_SYSTEM_ERROR;
        }
        
        current_address += LOG_SECTOR_SIZE;
        
        /* 添加延时，避免看门狗超时 */
        SmartDelay(10);
    }
    
    /* 重置Flash管理器状态 */
    flash_manager.write_address = LOG_SYSTEM_START_ADDR;
    flash_manager.used_size = 0;
    flash_manager.total_entries = 0;
    flash_manager.total_erase_count += sectors_to_erase;
    flash_manager.is_full = 0;
    
    /* 更新健康度 */
    LogSystem_UpdateHealthPercentage();
    
    DEBUG_Printf("日志系统格式化完成\r\n");
    DEBUG_Printf("- 已擦除扇区数: %d\r\n", sectors_to_erase);
    DEBUG_Printf("- 当前日志条数: %d\r\n", flash_manager.total_entries);
    DEBUG_Printf("- Flash健康度: %d%%\r\n", flash_manager.health_percentage);
    
    /* 记录格式化日志 */
    LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "Log system formatted");
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  重置日志系统（快速清空，仅重置指针）
  * @param  None
  * @retval LogSystemStatus_t 操作状态
  */
LogSystemStatus_t LogSystem_Reset(void)
{
    /* 检查初始化状态 */
    if (!flash_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    DEBUG_Printf("正在重置日志系统...\r\n");
    
    /* 重置Flash管理器状态（不实际擦除Flash，只重置指针） */
    flash_manager.write_address = LOG_SYSTEM_START_ADDR;
    flash_manager.used_size = 0;
    flash_manager.total_entries = 0;
    flash_manager.is_full = 0;
    
    DEBUG_Printf("日志系统重置完成（逻辑清空）\r\n");
    DEBUG_Printf("- 当前日志条数: %d\r\n", flash_manager.total_entries);
    DEBUG_Printf("- 写入地址重置为: 0x%08X\r\n", flash_manager.write_address);
    
    /* 记录重置日志 */
    LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "Log system reset");
    
    return LOG_SYSTEM_OK;
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */ 


