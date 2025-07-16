/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : log_system.h
  * @brief          : 系统运行日志模块头文件 - 精简版日志系统
  ******************************************************************************
  * @attention
  *
  * 精简版日志系统，只记录核心事件：
  * 1. 异常事件记录
  * 2. 系统保护事件  
  * 3. 系统生命周期
  * 4. 系统健康监控
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

/* 日志分类定义 - 精简版 */
typedef enum {
    LOG_CATEGORY_SAFETY = 1,    // 安全关键日志（异常事件和系统保护）
    LOG_CATEGORY_SYSTEM,        // 系统状态日志（生命周期）
    LOG_CATEGORY_MONITOR        // 监控数据日志（健康监控）
} LogCategory_t;

/* 日志条目结构体（64字节对齐）- 优化版本 */
typedef struct {
    uint32_t timestamp;         // 时间戳（系统运行时间毫秒）
    uint8_t  log_category;      // 日志分类（1=SAFETY,2=SYSTEM,3=MONITOR）
    uint8_t  channel;           // 相关通道（1-3，0表示系统级）
    uint16_t event_code;        // 事件代码（按分类分段）
    char     message[48];       // 日志消息（支持中英文）
    uint32_t checksum;          // 校验和
    uint32_t magic_number;      // 魔数（用于验证有效性）
} __attribute__((packed)) LogEntry_t;

/* 日志重置标记结构体（64字节对齐）*/
typedef struct {
    uint32_t reset_magic;       // 重置标记魔数（0xRE5E7C1R）
    uint32_t reset_timestamp;   // 重置时间戳
    uint32_t reset_count;       // 重置次数计数
    uint32_t reserved1;         // 保留字段1
    char     reset_reason[32];  // 重置原因描述
    uint32_t reserved2;         // 保留字段2
    uint32_t reserved3;         // 保留字段3
    uint32_t checksum;          // 校验和
    uint32_t final_magic;       // 结束魔数（0xDEADBEEF）
} __attribute__((packed)) LogResetMarker_t;

/* 统一日志管理器结构体 */
typedef struct {
    uint32_t start_address;             // 存储区起始地址
    uint32_t end_address;               // 存储区结束地址
    uint32_t write_address;             // 当前写入地址
    uint32_t read_address;              // 最旧日志地址（用于循环覆盖）
    uint32_t total_size;                // 存储区总容量
    uint32_t used_size;                 // 已使用容量
    uint32_t total_entries;             // 总日志条目数
    uint32_t max_entries;               // 最大条目数
    uint32_t oldest_entry_index;        // 最旧日志索引
    uint32_t newest_entry_index;        // 最新日志索引
    uint8_t  is_full;                   // 是否已满标志（开始循环覆盖）
    uint8_t  is_initialized;            // 是否已初始化
    uint32_t total_erase_count;         // 总擦除次数
    uint32_t total_write_count;         // 总写入次数
    uint32_t total_reset_count;         // 总重置次数
    uint8_t  health_percentage;         // 健康度百分比
} LogManager_t;

/* 日志系统状态 */
typedef enum {
    LOG_SYSTEM_OK = 0,          // 操作成功
    LOG_SYSTEM_ERROR,           // 操作失败
    LOG_SYSTEM_FULL,            // 存储器已满
    LOG_SYSTEM_BUSY,            // 系统忙碌
    LOG_SYSTEM_NOT_INIT,        // 未初始化
    LOG_SYSTEM_CATEGORY_FULL    // 指定分类已满
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

/* Flash分区定义 - 统一存储区管理 */
#define LOG_SYSTEM_START_ADDR       0x100000    // 1MB开始
#define LOG_SYSTEM_END_ADDR         0xFFFFFF    // 16MB结束
#define LOG_SYSTEM_SIZE             (LOG_SYSTEM_END_ADDR - LOG_SYSTEM_START_ADDR + 1)

/* 统一存储区参数 */
#define LOG_ENTRY_SIZE              64          // 每条日志64字节
#define LOG_MAGIC_NUMBER            0xDEADBEEF  // 日志条目魔数
#define LOG_RESET_MARKER_MAGIC      0x5E5E7C1A  // 重置标记魔数（RESET + CLA）
#define LOG_RESET_MARKER_SIZE       64          // 重置标记大小（64字节对齐）
#define LOG_MAX_ENTRIES             (LOG_SYSTEM_SIZE / LOG_ENTRY_SIZE)  // 最大条目数：约245,760条

/* 循环覆盖参数 */
#define LOG_CIRCULAR_BUFFER_SIZE    LOG_SYSTEM_SIZE     // 整个存储区作为循环缓冲区
#define LOG_WRITE_PROTECTION_SIZE   (1024 * 64)         // 64KB写保护区域，防止覆盖最新日志

/* 健康度参数 */
#define LOG_HEALTH_WARNING_THRESHOLD    20      // 健康度警告阈值
#define LOG_MAX_ERASE_CYCLES           100000   // 最大擦除次数
#define LOG_SECTOR_SIZE                4096     // 扇区大小

/* ========================== 安全关键日志事件代码 (0x1000-0x1FFF) ========================== */
/* 异常事件 */
#define LOG_EVENT_SAFETY_ALARM_A    0x1001      // A类异常：使能信号冲突
#define LOG_EVENT_SAFETY_ALARM_B    0x1002      // B类异常：K1_1继电器状态异常
#define LOG_EVENT_SAFETY_ALARM_C    0x1003      // C类异常：K2_1继电器状态异常
#define LOG_EVENT_SAFETY_ALARM_D    0x1004      // D类异常：K3_1继电器状态异常
#define LOG_EVENT_SAFETY_ALARM_E    0x1005      // E类异常：K1_2继电器状态异常
#define LOG_EVENT_SAFETY_ALARM_F    0x1006      // F类异常：K2_2继电器状态异常
#define LOG_EVENT_SAFETY_ALARM_G    0x1007      // G类异常：K3_2继电器状态异常
#define LOG_EVENT_SAFETY_ALARM_H    0x1008      // H类异常：SW1接触器状态异常
#define LOG_EVENT_SAFETY_ALARM_I    0x1009      // I类异常：SW2接触器状态异常
#define LOG_EVENT_SAFETY_ALARM_J    0x100A      // J类异常：SW3接触器状态异常
#define LOG_EVENT_SAFETY_ALARM_K    0x100B      // K类异常：NTC1温度过热
#define LOG_EVENT_SAFETY_ALARM_L    0x100C      // L类异常：NTC2温度过热
#define LOG_EVENT_SAFETY_ALARM_M    0x100D      // M类异常：NTC3温度过热
#define LOG_EVENT_SAFETY_ALARM_N    0x100E      // N类异常：系统自检失败
#define LOG_EVENT_SAFETY_ALARM_O    0x100F      // O类异常：外部电源异常

/* 系统保护事件 */
#define LOG_EVENT_WATCHDOG_RESET    0x1020      // 看门狗复位
#define LOG_EVENT_EMERGENCY_STOP    0x1021      // 紧急停机
#define LOG_EVENT_SYSTEM_LOCK       0x1022      // 系统锁定
#define LOG_EVENT_POWER_FAILURE     0x1023      // 电源故障

/* 异常解除事件 (0x1030-0x103F) */
#define LOG_EVENT_ALARM_RESOLVED_A  0x1030      // A类异常解除
#define LOG_EVENT_ALARM_RESOLVED_B  0x1031      // B类异常解除
#define LOG_EVENT_ALARM_RESOLVED_C  0x1032      // C类异常解除
#define LOG_EVENT_ALARM_RESOLVED_D  0x1033      // D类异常解除
#define LOG_EVENT_ALARM_RESOLVED_E  0x1034      // E类异常解除
#define LOG_EVENT_ALARM_RESOLVED_F  0x1035      // F类异常解除
#define LOG_EVENT_ALARM_RESOLVED_G  0x1036      // G类异常解除
#define LOG_EVENT_ALARM_RESOLVED_H  0x1037      // H类异常解除
#define LOG_EVENT_ALARM_RESOLVED_I  0x1038      // I类异常解除
#define LOG_EVENT_ALARM_RESOLVED_J  0x1039      // J类异常解除
#define LOG_EVENT_ALARM_RESOLVED_K  0x103A      // K类异常解除
#define LOG_EVENT_ALARM_RESOLVED_L  0x103B      // L类异常解除
#define LOG_EVENT_ALARM_RESOLVED_M  0x103C      // M类异常解除
#define LOG_EVENT_ALARM_RESOLVED_N  0x103D      // N类异常解除
#define LOG_EVENT_ALARM_RESOLVED_O  0x103E      // O类异常解除

/* ========================== 系统状态日志事件代码 (0x3000-0x3FFF) ========================== */
/* 系统生命周期 */
#define LOG_EVENT_SYSTEM_START      0x3001      // 系统启动
#define LOG_EVENT_SYSTEM_STOP       0x3002      // 系统停止
#define LOG_EVENT_SYSTEM_RESTART    0x3003      // 系统重启
#define LOG_EVENT_POWER_ON_RESET    0x3004      // 上电复位
#define LOG_EVENT_SOFTWARE_RESET    0x3005      // 软件复位

/* ========================== 监控数据日志事件代码 (0x4000-0x4FFF) ========================== */
/* 系统健康监控 */
#define LOG_EVENT_FLASH_HEALTH      0x4030      // Flash健康度更新
#define LOG_EVENT_WATCHDOG_STATS    0x4031      // 看门狗统计
#define LOG_EVENT_MEMORY_USAGE      0x4032      // 内存使用情况

/* 复位分析系统事件 (0x4040-0x404F) */
#define LOG_EVENT_RESET_ANALYSIS_INIT    0x4040      // 复位分析系统初始化
#define LOG_EVENT_RESET_SNAPSHOT         0x4041      // 复位前状态快照记录
#define LOG_EVENT_RESET_CAUSE_ANALYSIS   0x4042      // 复位原因分析结果
#define LOG_EVENT_RESET_STATISTICS       0x4043      // 复位统计更新
#define LOG_EVENT_RESET_RISK_WARNING     0x4044      // 复位风险预警
#define LOG_EVENT_RESET_ABNORMAL_STATE   0x4045      // 异常状态检测
#define LOG_EVENT_RESET_QUERY_REQUEST    0x4046      // 复位查询请求
#define LOG_EVENT_RESET_SYSTEM_STABLE    0x4047      // 系统稳定性评估

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* ========================== 安全关键日志便捷宏 ========================== */
/* 异常触发 */
#define LOG_SAFETY_ALARM_A(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_SAFETY_ALARM_A, desc)
#define LOG_SAFETY_ALARM_B(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 1, LOG_EVENT_SAFETY_ALARM_B, desc)
#define LOG_SAFETY_ALARM_C(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 2, LOG_EVENT_SAFETY_ALARM_C, desc)
#define LOG_SAFETY_ALARM_D(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 3, LOG_EVENT_SAFETY_ALARM_D, desc)
#define LOG_SAFETY_ALARM_E(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 1, LOG_EVENT_SAFETY_ALARM_E, desc)
#define LOG_SAFETY_ALARM_F(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 2, LOG_EVENT_SAFETY_ALARM_F, desc)
#define LOG_SAFETY_ALARM_G(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 3, LOG_EVENT_SAFETY_ALARM_G, desc)
#define LOG_SAFETY_ALARM_H(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 1, LOG_EVENT_SAFETY_ALARM_H, desc)
#define LOG_SAFETY_ALARM_I(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 2, LOG_EVENT_SAFETY_ALARM_I, desc)
#define LOG_SAFETY_ALARM_J(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 3, LOG_EVENT_SAFETY_ALARM_J, desc)
#define LOG_SAFETY_ALARM_K(ch, temp)   LogSystem_Record(LOG_CATEGORY_SAFETY, ch, LOG_EVENT_SAFETY_ALARM_K, temp)
#define LOG_SAFETY_ALARM_L(ch, temp)   LogSystem_Record(LOG_CATEGORY_SAFETY, ch, LOG_EVENT_SAFETY_ALARM_L, temp)
#define LOG_SAFETY_ALARM_M(ch, temp)   LogSystem_Record(LOG_CATEGORY_SAFETY, ch, LOG_EVENT_SAFETY_ALARM_M, temp)
#define LOG_SAFETY_ALARM_N(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_SAFETY_ALARM_N, desc)
#define LOG_SAFETY_ALARM_O(desc)       LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_SAFETY_ALARM_O, desc)

/* 系统保护 */
#define LOG_WATCHDOG_RESET()           LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_WATCHDOG_RESET, "看门狗复位")
#define LOG_EMERGENCY_STOP(reason)     LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_EMERGENCY_STOP, reason)
#define LOG_SYSTEM_LOCK(reason)        LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_SYSTEM_LOCK, reason)
#define LOG_POWER_FAILURE(desc)        LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_POWER_FAILURE, desc)

/* 异常解除 */
#define LOG_SAFETY_RESOLVED_A(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_ALARM_RESOLVED_A, desc)
#define LOG_SAFETY_RESOLVED_B(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 1, LOG_EVENT_ALARM_RESOLVED_B, desc)
#define LOG_SAFETY_RESOLVED_C(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 2, LOG_EVENT_ALARM_RESOLVED_C, desc)
#define LOG_SAFETY_RESOLVED_D(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 3, LOG_EVENT_ALARM_RESOLVED_D, desc)
#define LOG_SAFETY_RESOLVED_E(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 1, LOG_EVENT_ALARM_RESOLVED_E, desc)
#define LOG_SAFETY_RESOLVED_F(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 2, LOG_EVENT_ALARM_RESOLVED_F, desc)
#define LOG_SAFETY_RESOLVED_G(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 3, LOG_EVENT_ALARM_RESOLVED_G, desc)
#define LOG_SAFETY_RESOLVED_H(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 1, LOG_EVENT_ALARM_RESOLVED_H, desc)
#define LOG_SAFETY_RESOLVED_I(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 2, LOG_EVENT_ALARM_RESOLVED_I, desc)
#define LOG_SAFETY_RESOLVED_J(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 3, LOG_EVENT_ALARM_RESOLVED_J, desc)
#define LOG_SAFETY_RESOLVED_K(ch, temp) LogSystem_Record(LOG_CATEGORY_SAFETY, ch, LOG_EVENT_ALARM_RESOLVED_K, temp)
#define LOG_SAFETY_RESOLVED_L(ch, temp) LogSystem_Record(LOG_CATEGORY_SAFETY, ch, LOG_EVENT_ALARM_RESOLVED_L, temp)
#define LOG_SAFETY_RESOLVED_M(ch, temp) LogSystem_Record(LOG_CATEGORY_SAFETY, ch, LOG_EVENT_ALARM_RESOLVED_M, temp)
#define LOG_SAFETY_RESOLVED_N(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_ALARM_RESOLVED_N, desc)
#define LOG_SAFETY_RESOLVED_O(desc)    LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_ALARM_RESOLVED_O, desc)

/* ========================== 系统状态日志便捷宏 ========================== */
/* 生命周期 */
#define LOG_SYSTEM_START()             LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "系统启动")
#define LOG_SYSTEM_STOP()              LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_STOP, "系统停止")
#define LOG_SYSTEM_RESTART(reason)     LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_RESTART, reason)
#define LOG_POWER_ON_RESET()           LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_POWER_ON_RESET, "上电复位")
#define LOG_SOFTWARE_RESET(reason)     LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SOFTWARE_RESET, reason)

/* ========================== 监控数据日志便捷宏 ========================== */
/* 系统健康 */
#define LOG_FLASH_HEALTH(health)       LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_FLASH_HEALTH, health)
#define LOG_WATCHDOG_STATS(stats)      LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_WATCHDOG_STATS, stats)
#define LOG_MEMORY_USAGE(usage)        LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_MEMORY_USAGE, usage)

/* 复位分析系统 */
#define LOG_RESET_ANALYSIS_INIT(desc)      LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_RESET_ANALYSIS_INIT, desc)
#define LOG_RESET_SNAPSHOT(desc)           LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_RESET_SNAPSHOT, desc)
#define LOG_RESET_CAUSE_ANALYSIS(cause)    LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_RESET_CAUSE_ANALYSIS, cause)
#define LOG_RESET_STATISTICS(desc)         LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_RESET_STATISTICS, desc)
#define LOG_RESET_RISK_WARNING(level)      LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_RESET_RISK_WARNING, level)
#define LOG_RESET_ABNORMAL_STATE(desc)     LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_RESET_ABNORMAL_STATE, desc)
#define LOG_RESET_QUERY_REQUEST(user)      LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_RESET_QUERY_REQUEST, user)
#define LOG_RESET_SYSTEM_STABLE(score)     LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_RESET_SYSTEM_STABLE, score)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* 系统管理函数 */
LogSystemStatus_t LogSystem_Init(void);
LogSystemStatus_t LogSystem_DeInit(void);
LogSystemStatus_t LogSystem_Reset(void);
LogSystemStatus_t LogSystem_Format(void);

/* 日志记录函数 - 统一接口 */
LogSystemStatus_t LogSystem_Record(LogCategory_t category, uint8_t channel, uint16_t event_code, const char* message);
LogSystemStatus_t LogSystem_RecordFormatted(LogCategory_t category, uint8_t channel, uint16_t event_code, const char* format, ...);

/* 日志读取函数 */
LogSystemStatus_t LogSystem_ReadEntry(uint32_t entry_index, LogEntry_t* entry);
LogSystemStatus_t LogSystem_ReadLatestEntries(LogEntry_t* entries, uint32_t max_count, uint32_t* actual_count);
LogSystemStatus_t LogSystem_ReadEntriesByCategory(LogCategory_t category, LogEntry_t* entries, uint32_t max_count, uint32_t* actual_count);

/* 日志输出函数 */
LogSystemStatus_t LogSystem_OutputAll(LogFormat_t format);
LogSystemStatus_t LogSystem_OutputByCategory(LogCategory_t category, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputByChannel(uint8_t channel, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputLatest(uint32_t count, LogFormat_t format);

/* 分批输出支持函数 */
uint32_t LogSystem_GetLogCount(void);
uint32_t LogSystem_GetCategoryLogCount(LogCategory_t category);
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
const char* LogSystem_GetCategoryString(uint8_t category);
const char* LogSystem_GetEventCodeString(uint16_t event_code);

/* 内部验证函数 */
uint8_t LogSystem_ValidateCategory(LogCategory_t category);

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __LOG_SYSTEM_H */ 

