/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : log_system.h
  * @brief          : 系统运行日志模块头文件
  ******************************************************************************
  * @attention
  *
  * 这是为三通道切换箱控制系统设计的日志记录系统
  * 支持环形覆盖存储、健康度监控、串口输出等功能
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __LOG_SYSTEM_H
#define __LOG_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "w25q128_driver.h"
#include "smart_delay.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* 日志类型定义 */
typedef enum {
    LOG_TYPE_SYSTEM = 0,        // 系统级日志
    LOG_TYPE_CHANNEL,           // 通道操作日志
    LOG_TYPE_ERROR,             // 错误日志
    LOG_TYPE_WARNING,           // 警告日志
    LOG_TYPE_INFO,              // 信息日志
    LOG_TYPE_DEBUG              // 调试日志
} LogType_t;

/* 日志条目结构体（64字节对齐） */
typedef struct {
    uint32_t timestamp;         // 时间戳（系统运行时间毫秒）
    uint8_t  log_type;          // 日志类型
    uint8_t  channel;           // 相关通道（1-3，0表示系统级）
    uint16_t event_code;        // 事件代码
    char     message[48];       // 日志消息
    uint32_t checksum;          // 校验和
    uint32_t magic_number;      // 魔数（用于验证有效性）
} __attribute__((packed)) LogEntry_t;

/* Flash管理器结构体 */
typedef struct {
    uint32_t write_address;     // 当前写入地址
    uint32_t start_address;     // 日志区域起始地址
    uint32_t end_address;       // 日志区域结束地址
    uint32_t total_size;        // 总容量
    uint32_t used_size;         // 已使用容量
    uint32_t total_entries;     // 总日志条目数
    uint32_t total_erase_count; // 总擦除次数
    uint32_t total_write_count; // 总写入次数
    uint8_t  health_percentage; // 健康度百分比
    uint8_t  is_full;           // 是否已满标志
    uint8_t  is_initialized;    // 是否已初始化
} FlashManager_t;

/* 日志系统状态 */
typedef enum {
    LOG_SYSTEM_OK = 0,          // 操作成功
    LOG_SYSTEM_ERROR,           // 操作失败
    LOG_SYSTEM_FULL,            // 存储器已满
    LOG_SYSTEM_BUSY,            // 系统忙碌
    LOG_SYSTEM_NOT_INIT         // 未初始化
} LogSystemStatus_t;

/* 日志输出格式 */
typedef enum {
    LOG_FORMAT_SIMPLE = 0,      // 简单格式
    LOG_FORMAT_DETAILED,        // 详细格式
    LOG_FORMAT_CSV              // CSV格式
} LogFormat_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* Flash分区定义 */
#define LOG_SYSTEM_START_ADDR       0x100000    // 1MB开始
#define LOG_SYSTEM_END_ADDR         0xFFFFFF    // 16MB结束
#define LOG_SYSTEM_SIZE             (LOG_SYSTEM_END_ADDR - LOG_SYSTEM_START_ADDR + 1)

/* 日志条目参数 */
#define LOG_ENTRY_SIZE              64          // 每条日志64字节
#define LOG_MAX_ENTRIES             (LOG_SYSTEM_SIZE / LOG_ENTRY_SIZE)  // 最大条目数
#define LOG_MAGIC_NUMBER            0xDEADBEEF  // 魔数

/* 健康度参数 */
#define LOG_HEALTH_WARNING_THRESHOLD    20      // 健康度警告阈值
#define LOG_MAX_ERASE_CYCLES           100000   // 最大擦除次数
#define LOG_SECTOR_SIZE                4096     // 扇区大小

/* 事件代码定义 */
#define LOG_EVENT_SYSTEM_START      0x0001      // 系统启动
#define LOG_EVENT_SYSTEM_STOP       0x0002      // 系统停止
#define LOG_EVENT_CHANNEL_OPEN      0x0010      // 通道开启
#define LOG_EVENT_CHANNEL_CLOSE     0x0011      // 通道关闭
#define LOG_EVENT_CHANNEL_ERROR     0x0012      // 通道错误
#define LOG_EVENT_TEMP_WARNING      0x0020      // 温度警告
#define LOG_EVENT_TEMP_ERROR        0x0021      // 温度错误
#define LOG_EVENT_POWER_FAILURE     0x0030      // 电源故障
#define LOG_EVENT_EXCEPTION         0x0040      // 异常事件
#define LOG_EVENT_FLASH_WARNING     0x0050      // Flash警告
#define LOG_EVENT_FLASH_ERROR       0x0051      // Flash错误

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* 便捷日志记录宏 */
#define LOG_SYSTEM(msg)             LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_START, msg)
#define LOG_CHANNEL(ch, msg)        LogSystem_Record(LOG_TYPE_CHANNEL, ch, LOG_EVENT_CHANNEL_OPEN, msg)
#define LOG_ERROR(code, msg)        LogSystem_Record(LOG_TYPE_ERROR, 0, code, msg)
#define LOG_WARNING(code, msg)      LogSystem_Record(LOG_TYPE_WARNING, 0, code, msg)
#define LOG_INFO(msg)               LogSystem_Record(LOG_TYPE_INFO, 0, 0, msg)
#define LOG_DEBUG(msg)              LogSystem_Record(LOG_TYPE_DEBUG, 0, 0, msg)

/* 通道操作日志宏 */
#define LOG_CHANNEL_OPEN(ch)        LogSystem_Record(LOG_TYPE_CHANNEL, ch, LOG_EVENT_CHANNEL_OPEN, "Channel opened")
#define LOG_CHANNEL_CLOSE(ch)       LogSystem_Record(LOG_TYPE_CHANNEL, ch, LOG_EVENT_CHANNEL_CLOSE, "Channel closed")
#define LOG_CHANNEL_ERROR(ch, msg)  LogSystem_Record(LOG_TYPE_ERROR, ch, LOG_EVENT_CHANNEL_ERROR, msg)

/* 温度日志宏 */
#define LOG_TEMP_WARNING(ch, temp)  do { \
    char temp_msg[48]; \
    sprintf(temp_msg, "Temp warning: %d°C", temp); \
    LogSystem_Record(LOG_TYPE_WARNING, ch, LOG_EVENT_TEMP_WARNING, temp_msg); \
} while(0)

#define LOG_TEMP_ERROR(ch, temp)    do { \
    char temp_msg[48]; \
    sprintf(temp_msg, "Temp error: %d°C", temp); \
    LogSystem_Record(LOG_TYPE_ERROR, ch, LOG_EVENT_TEMP_ERROR, temp_msg); \
} while(0)

/* 异常日志宏 */
#define LOG_EXCEPTION(flags, msg)   LogSystem_Record(LOG_TYPE_ERROR, 0, LOG_EVENT_EXCEPTION, msg)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* 系统管理函数 */
LogSystemStatus_t LogSystem_Init(void);
LogSystemStatus_t LogSystem_DeInit(void);
LogSystemStatus_t LogSystem_Reset(void);
LogSystemStatus_t LogSystem_Format(void);

/* 日志记录函数 */
LogSystemStatus_t LogSystem_Record(LogType_t log_type, uint8_t channel, uint16_t event_code, const char* message);
LogSystemStatus_t LogSystem_RecordFormatted(LogType_t log_type, uint8_t channel, uint16_t event_code, const char* format, ...);

/* 日志读取函数 */
LogSystemStatus_t LogSystem_ReadEntry(uint32_t entry_index, LogEntry_t* entry);
LogSystemStatus_t LogSystem_ReadLatestEntries(LogEntry_t* entries, uint32_t max_count, uint32_t* actual_count);
LogSystemStatus_t LogSystem_ReadEntriesByType(LogType_t log_type, LogEntry_t* entries, uint32_t max_count, uint32_t* actual_count);

/* 日志输出函数 */
LogSystemStatus_t LogSystem_OutputAll(LogFormat_t format);
LogSystemStatus_t LogSystem_OutputByType(LogType_t log_type, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputByChannel(uint8_t channel, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputLatest(uint32_t count, LogFormat_t format);

/* 分批输出支持函数 */
uint32_t LogSystem_GetLogCount(void);
LogSystemStatus_t LogSystem_OutputHeader(void);
LogSystemStatus_t LogSystem_OutputSingle(uint32_t entry_index, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputFooter(void);

/* 系统状态函数 */
uint32_t LogSystem_GetEntryCount(void);
uint32_t LogSystem_GetUsedSize(void);
uint8_t LogSystem_GetHealthPercentage(void);
uint8_t LogSystem_IsFull(void);
uint8_t LogSystem_IsInitialized(void);

/* 维护函数 */
LogSystemStatus_t LogSystem_UpdateHealthPercentage(void);
LogSystemStatus_t LogSystem_FindLastWritePosition(void);
LogSystemStatus_t LogSystem_VerifyIntegrity(void);

/* 工具函数 */
uint32_t LogSystem_CalculateChecksum(const LogEntry_t* entry);
uint8_t LogSystem_VerifyEntry(const LogEntry_t* entry);
void LogSystem_FormatTimestamp(uint32_t timestamp, char* buffer, uint8_t buffer_size);
const char* LogSystem_GetTypeString(uint8_t log_type);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __LOG_SYSTEM_H */ 

