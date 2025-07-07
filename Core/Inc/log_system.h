/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : log_system.h
  * @brief          : ϵͳ������־ģ��ͷ�ļ�
  ******************************************************************************
  * @attention
  *
  * ����Ϊ��ͨ���л������ϵͳ��Ƶ���־��¼ϵͳ
  * ֧�ֻ��θ��Ǵ洢�������ȼ�ء���������ȹ���
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

/* ��־���Ͷ��� */
typedef enum {
    LOG_TYPE_SYSTEM = 0,        // ϵͳ����־
    LOG_TYPE_CHANNEL,           // ͨ��������־
    LOG_TYPE_ERROR,             // ������־
    LOG_TYPE_WARNING,           // ������־
    LOG_TYPE_INFO,              // ��Ϣ��־
    LOG_TYPE_DEBUG              // ������־
} LogType_t;

/* ��־��Ŀ�ṹ�壨64�ֽڶ��룩 */
typedef struct {
    uint32_t timestamp;         // ʱ�����ϵͳ����ʱ����룩
    uint8_t  log_type;          // ��־����
    uint8_t  channel;           // ���ͨ����1-3��0��ʾϵͳ����
    uint16_t event_code;        // �¼�����
    char     message[48];       // ��־��Ϣ
    uint32_t checksum;          // У���
    uint32_t magic_number;      // ħ����������֤��Ч�ԣ�
} __attribute__((packed)) LogEntry_t;

/* Flash�������ṹ�� */
typedef struct {
    uint32_t write_address;     // ��ǰд���ַ
    uint32_t start_address;     // ��־������ʼ��ַ
    uint32_t end_address;       // ��־���������ַ
    uint32_t total_size;        // ������
    uint32_t used_size;         // ��ʹ������
    uint32_t total_entries;     // ����־��Ŀ��
    uint32_t total_erase_count; // �ܲ�������
    uint32_t total_write_count; // ��д�����
    uint8_t  health_percentage; // �����Ȱٷֱ�
    uint8_t  is_full;           // �Ƿ�������־
    uint8_t  is_initialized;    // �Ƿ��ѳ�ʼ��
} FlashManager_t;

/* ��־ϵͳ״̬ */
typedef enum {
    LOG_SYSTEM_OK = 0,          // �����ɹ�
    LOG_SYSTEM_ERROR,           // ����ʧ��
    LOG_SYSTEM_FULL,            // �洢������
    LOG_SYSTEM_BUSY,            // ϵͳæµ
    LOG_SYSTEM_NOT_INIT         // δ��ʼ��
} LogSystemStatus_t;

/* ��־�����ʽ */
typedef enum {
    LOG_FORMAT_SIMPLE = 0,      // �򵥸�ʽ
    LOG_FORMAT_DETAILED,        // ��ϸ��ʽ
    LOG_FORMAT_CSV              // CSV��ʽ
} LogFormat_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* Flash�������� */
#define LOG_SYSTEM_START_ADDR       0x100000    // 1MB��ʼ
#define LOG_SYSTEM_END_ADDR         0xFFFFFF    // 16MB����
#define LOG_SYSTEM_SIZE             (LOG_SYSTEM_END_ADDR - LOG_SYSTEM_START_ADDR + 1)

/* ��־��Ŀ���� */
#define LOG_ENTRY_SIZE              64          // ÿ����־64�ֽ�
#define LOG_MAX_ENTRIES             (LOG_SYSTEM_SIZE / LOG_ENTRY_SIZE)  // �����Ŀ��
#define LOG_MAGIC_NUMBER            0xDEADBEEF  // ħ��

/* �����Ȳ��� */
#define LOG_HEALTH_WARNING_THRESHOLD    20      // �����Ⱦ�����ֵ
#define LOG_MAX_ERASE_CYCLES           100000   // ����������
#define LOG_SECTOR_SIZE                4096     // ������С

/* �¼����붨�� */
#define LOG_EVENT_SYSTEM_START      0x0001      // ϵͳ����
#define LOG_EVENT_SYSTEM_STOP       0x0002      // ϵͳֹͣ
#define LOG_EVENT_CHANNEL_OPEN      0x0010      // ͨ������
#define LOG_EVENT_CHANNEL_CLOSE     0x0011      // ͨ���ر�
#define LOG_EVENT_CHANNEL_ERROR     0x0012      // ͨ������
#define LOG_EVENT_TEMP_WARNING      0x0020      // �¶Ⱦ���
#define LOG_EVENT_TEMP_ERROR        0x0021      // �¶ȴ���
#define LOG_EVENT_POWER_FAILURE     0x0030      // ��Դ����
#define LOG_EVENT_EXCEPTION         0x0040      // �쳣�¼�
#define LOG_EVENT_FLASH_WARNING     0x0050      // Flash����
#define LOG_EVENT_FLASH_ERROR       0x0051      // Flash����

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* �����־��¼�� */
#define LOG_SYSTEM(msg)             LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_START, msg)
#define LOG_CHANNEL(ch, msg)        LogSystem_Record(LOG_TYPE_CHANNEL, ch, LOG_EVENT_CHANNEL_OPEN, msg)
#define LOG_ERROR(code, msg)        LogSystem_Record(LOG_TYPE_ERROR, 0, code, msg)
#define LOG_WARNING(code, msg)      LogSystem_Record(LOG_TYPE_WARNING, 0, code, msg)
#define LOG_INFO(msg)               LogSystem_Record(LOG_TYPE_INFO, 0, 0, msg)
#define LOG_DEBUG(msg)              LogSystem_Record(LOG_TYPE_DEBUG, 0, 0, msg)

/* ͨ��������־�� */
#define LOG_CHANNEL_OPEN(ch)        LogSystem_Record(LOG_TYPE_CHANNEL, ch, LOG_EVENT_CHANNEL_OPEN, "Channel opened")
#define LOG_CHANNEL_CLOSE(ch)       LogSystem_Record(LOG_TYPE_CHANNEL, ch, LOG_EVENT_CHANNEL_CLOSE, "Channel closed")
#define LOG_CHANNEL_ERROR(ch, msg)  LogSystem_Record(LOG_TYPE_ERROR, ch, LOG_EVENT_CHANNEL_ERROR, msg)

/* �¶���־�� */
#define LOG_TEMP_WARNING(ch, temp)  do { \
    char temp_msg[48]; \
    sprintf(temp_msg, "Temp warning: %d��C", temp); \
    LogSystem_Record(LOG_TYPE_WARNING, ch, LOG_EVENT_TEMP_WARNING, temp_msg); \
} while(0)

#define LOG_TEMP_ERROR(ch, temp)    do { \
    char temp_msg[48]; \
    sprintf(temp_msg, "Temp error: %d��C", temp); \
    LogSystem_Record(LOG_TYPE_ERROR, ch, LOG_EVENT_TEMP_ERROR, temp_msg); \
} while(0)

/* �쳣��־�� */
#define LOG_EXCEPTION(flags, msg)   LogSystem_Record(LOG_TYPE_ERROR, 0, LOG_EVENT_EXCEPTION, msg)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* ϵͳ������ */
LogSystemStatus_t LogSystem_Init(void);
LogSystemStatus_t LogSystem_DeInit(void);
LogSystemStatus_t LogSystem_Reset(void);
LogSystemStatus_t LogSystem_Format(void);

/* ��־��¼���� */
LogSystemStatus_t LogSystem_Record(LogType_t log_type, uint8_t channel, uint16_t event_code, const char* message);
LogSystemStatus_t LogSystem_RecordFormatted(LogType_t log_type, uint8_t channel, uint16_t event_code, const char* format, ...);

/* ��־��ȡ���� */
LogSystemStatus_t LogSystem_ReadEntry(uint32_t entry_index, LogEntry_t* entry);
LogSystemStatus_t LogSystem_ReadLatestEntries(LogEntry_t* entries, uint32_t max_count, uint32_t* actual_count);
LogSystemStatus_t LogSystem_ReadEntriesByType(LogType_t log_type, LogEntry_t* entries, uint32_t max_count, uint32_t* actual_count);

/* ��־������� */
LogSystemStatus_t LogSystem_OutputAll(LogFormat_t format);
LogSystemStatus_t LogSystem_OutputByType(LogType_t log_type, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputByChannel(uint8_t channel, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputLatest(uint32_t count, LogFormat_t format);

/* �������֧�ֺ��� */
uint32_t LogSystem_GetLogCount(void);
LogSystemStatus_t LogSystem_OutputHeader(void);
LogSystemStatus_t LogSystem_OutputSingle(uint32_t entry_index, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputFooter(void);

/* ϵͳ״̬���� */
uint32_t LogSystem_GetEntryCount(void);
uint32_t LogSystem_GetUsedSize(void);
uint8_t LogSystem_GetHealthPercentage(void);
uint8_t LogSystem_IsFull(void);
uint8_t LogSystem_IsInitialized(void);

/* ά������ */
LogSystemStatus_t LogSystem_UpdateHealthPercentage(void);
LogSystemStatus_t LogSystem_FindLastWritePosition(void);
LogSystemStatus_t LogSystem_VerifyIntegrity(void);

/* ���ߺ��� */
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

