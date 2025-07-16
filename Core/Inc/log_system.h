/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : log_system.h
  * @brief          : ϵͳ������־ģ��ͷ�ļ� - �������־ϵͳ
  ******************************************************************************
  * @attention
  *
  * �������־ϵͳ��ֻ��¼�����¼���
  * 1. �쳣�¼���¼
  * 2. ϵͳ�����¼�  
  * 3. ϵͳ��������
  * 4. ϵͳ�������
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

/* ��־���ඨ�� - ����� */
typedef enum {
    LOG_CATEGORY_SAFETY = 1,    // ��ȫ�ؼ���־���쳣�¼���ϵͳ������
    LOG_CATEGORY_SYSTEM,        // ϵͳ״̬��־���������ڣ�
    LOG_CATEGORY_MONITOR        // ���������־��������أ�
} LogCategory_t;

/* ��־��Ŀ�ṹ�壨64�ֽڶ��룩- �Ż��汾 */
typedef struct {
    uint32_t timestamp;         // ʱ�����ϵͳ����ʱ����룩
    uint8_t  log_category;      // ��־���ࣨ1=SAFETY,2=SYSTEM,3=MONITOR��
    uint8_t  channel;           // ���ͨ����1-3��0��ʾϵͳ����
    uint16_t event_code;        // �¼����루������ֶΣ�
    char     message[48];       // ��־��Ϣ��֧����Ӣ�ģ�
    uint32_t checksum;          // У���
    uint32_t magic_number;      // ħ����������֤��Ч�ԣ�
} __attribute__((packed)) LogEntry_t;

/* ��־���ñ�ǽṹ�壨64�ֽڶ��룩*/
typedef struct {
    uint32_t reset_magic;       // ���ñ��ħ����0xRE5E7C1R��
    uint32_t reset_timestamp;   // ����ʱ���
    uint32_t reset_count;       // ���ô�������
    uint32_t reserved1;         // �����ֶ�1
    char     reset_reason[32];  // ����ԭ������
    uint32_t reserved2;         // �����ֶ�2
    uint32_t reserved3;         // �����ֶ�3
    uint32_t checksum;          // У���
    uint32_t final_magic;       // ����ħ����0xDEADBEEF��
} __attribute__((packed)) LogResetMarker_t;

/* ͳһ��־�������ṹ�� */
typedef struct {
    uint32_t start_address;             // �洢����ʼ��ַ
    uint32_t end_address;               // �洢��������ַ
    uint32_t write_address;             // ��ǰд���ַ
    uint32_t read_address;              // �����־��ַ������ѭ�����ǣ�
    uint32_t total_size;                // �洢��������
    uint32_t used_size;                 // ��ʹ������
    uint32_t total_entries;             // ����־��Ŀ��
    uint32_t max_entries;               // �����Ŀ��
    uint32_t oldest_entry_index;        // �����־����
    uint32_t newest_entry_index;        // ������־����
    uint8_t  is_full;                   // �Ƿ�������־����ʼѭ�����ǣ�
    uint8_t  is_initialized;            // �Ƿ��ѳ�ʼ��
    uint32_t total_erase_count;         // �ܲ�������
    uint32_t total_write_count;         // ��д�����
    uint32_t total_reset_count;         // �����ô���
    uint8_t  health_percentage;         // �����Ȱٷֱ�
} LogManager_t;

/* ��־ϵͳ״̬ */
typedef enum {
    LOG_SYSTEM_OK = 0,          // �����ɹ�
    LOG_SYSTEM_ERROR,           // ����ʧ��
    LOG_SYSTEM_FULL,            // �洢������
    LOG_SYSTEM_BUSY,            // ϵͳæµ
    LOG_SYSTEM_NOT_INIT,        // δ��ʼ��
    LOG_SYSTEM_CATEGORY_FULL    // ָ����������
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

/* Flash�������� - ͳһ�洢������ */
#define LOG_SYSTEM_START_ADDR       0x100000    // 1MB��ʼ
#define LOG_SYSTEM_END_ADDR         0xFFFFFF    // 16MB����
#define LOG_SYSTEM_SIZE             (LOG_SYSTEM_END_ADDR - LOG_SYSTEM_START_ADDR + 1)

/* ͳһ�洢������ */
#define LOG_ENTRY_SIZE              64          // ÿ����־64�ֽ�
#define LOG_MAGIC_NUMBER            0xDEADBEEF  // ��־��Ŀħ��
#define LOG_RESET_MARKER_MAGIC      0x5E5E7C1A  // ���ñ��ħ����RESET + CLA��
#define LOG_RESET_MARKER_SIZE       64          // ���ñ�Ǵ�С��64�ֽڶ��룩
#define LOG_MAX_ENTRIES             (LOG_SYSTEM_SIZE / LOG_ENTRY_SIZE)  // �����Ŀ����Լ245,760��

/* ѭ�����ǲ��� */
#define LOG_CIRCULAR_BUFFER_SIZE    LOG_SYSTEM_SIZE     // �����洢����Ϊѭ��������
#define LOG_WRITE_PROTECTION_SIZE   (1024 * 64)         // 64KBд�������򣬷�ֹ����������־

/* �����Ȳ��� */
#define LOG_HEALTH_WARNING_THRESHOLD    20      // �����Ⱦ�����ֵ
#define LOG_MAX_ERASE_CYCLES           100000   // ����������
#define LOG_SECTOR_SIZE                4096     // ������С

/* ========================== ��ȫ�ؼ���־�¼����� (0x1000-0x1FFF) ========================== */
/* �쳣�¼� */
#define LOG_EVENT_SAFETY_ALARM_A    0x1001      // A���쳣��ʹ���źų�ͻ
#define LOG_EVENT_SAFETY_ALARM_B    0x1002      // B���쳣��K1_1�̵���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_C    0x1003      // C���쳣��K2_1�̵���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_D    0x1004      // D���쳣��K3_1�̵���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_E    0x1005      // E���쳣��K1_2�̵���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_F    0x1006      // F���쳣��K2_2�̵���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_G    0x1007      // G���쳣��K3_2�̵���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_H    0x1008      // H���쳣��SW1�Ӵ���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_I    0x1009      // I���쳣��SW2�Ӵ���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_J    0x100A      // J���쳣��SW3�Ӵ���״̬�쳣
#define LOG_EVENT_SAFETY_ALARM_K    0x100B      // K���쳣��NTC1�¶ȹ���
#define LOG_EVENT_SAFETY_ALARM_L    0x100C      // L���쳣��NTC2�¶ȹ���
#define LOG_EVENT_SAFETY_ALARM_M    0x100D      // M���쳣��NTC3�¶ȹ���
#define LOG_EVENT_SAFETY_ALARM_N    0x100E      // N���쳣��ϵͳ�Լ�ʧ��
#define LOG_EVENT_SAFETY_ALARM_O    0x100F      // O���쳣���ⲿ��Դ�쳣

/* ϵͳ�����¼� */
#define LOG_EVENT_WATCHDOG_RESET    0x1020      // ���Ź���λ
#define LOG_EVENT_EMERGENCY_STOP    0x1021      // ����ͣ��
#define LOG_EVENT_SYSTEM_LOCK       0x1022      // ϵͳ����
#define LOG_EVENT_POWER_FAILURE     0x1023      // ��Դ����

/* �쳣����¼� (0x1030-0x103F) */
#define LOG_EVENT_ALARM_RESOLVED_A  0x1030      // A���쳣���
#define LOG_EVENT_ALARM_RESOLVED_B  0x1031      // B���쳣���
#define LOG_EVENT_ALARM_RESOLVED_C  0x1032      // C���쳣���
#define LOG_EVENT_ALARM_RESOLVED_D  0x1033      // D���쳣���
#define LOG_EVENT_ALARM_RESOLVED_E  0x1034      // E���쳣���
#define LOG_EVENT_ALARM_RESOLVED_F  0x1035      // F���쳣���
#define LOG_EVENT_ALARM_RESOLVED_G  0x1036      // G���쳣���
#define LOG_EVENT_ALARM_RESOLVED_H  0x1037      // H���쳣���
#define LOG_EVENT_ALARM_RESOLVED_I  0x1038      // I���쳣���
#define LOG_EVENT_ALARM_RESOLVED_J  0x1039      // J���쳣���
#define LOG_EVENT_ALARM_RESOLVED_K  0x103A      // K���쳣���
#define LOG_EVENT_ALARM_RESOLVED_L  0x103B      // L���쳣���
#define LOG_EVENT_ALARM_RESOLVED_M  0x103C      // M���쳣���
#define LOG_EVENT_ALARM_RESOLVED_N  0x103D      // N���쳣���
#define LOG_EVENT_ALARM_RESOLVED_O  0x103E      // O���쳣���

/* ========================== ϵͳ״̬��־�¼����� (0x3000-0x3FFF) ========================== */
/* ϵͳ�������� */
#define LOG_EVENT_SYSTEM_START      0x3001      // ϵͳ����
#define LOG_EVENT_SYSTEM_STOP       0x3002      // ϵͳֹͣ
#define LOG_EVENT_SYSTEM_RESTART    0x3003      // ϵͳ����
#define LOG_EVENT_POWER_ON_RESET    0x3004      // �ϵ縴λ
#define LOG_EVENT_SOFTWARE_RESET    0x3005      // �����λ

/* ========================== ���������־�¼����� (0x4000-0x4FFF) ========================== */
/* ϵͳ������� */
#define LOG_EVENT_FLASH_HEALTH      0x4030      // Flash�����ȸ���
#define LOG_EVENT_WATCHDOG_STATS    0x4031      // ���Ź�ͳ��
#define LOG_EVENT_MEMORY_USAGE      0x4032      // �ڴ�ʹ�����

/* ��λ����ϵͳ�¼� (0x4040-0x404F) */
#define LOG_EVENT_RESET_ANALYSIS_INIT    0x4040      // ��λ����ϵͳ��ʼ��
#define LOG_EVENT_RESET_SNAPSHOT         0x4041      // ��λǰ״̬���ռ�¼
#define LOG_EVENT_RESET_CAUSE_ANALYSIS   0x4042      // ��λԭ��������
#define LOG_EVENT_RESET_STATISTICS       0x4043      // ��λͳ�Ƹ���
#define LOG_EVENT_RESET_RISK_WARNING     0x4044      // ��λ����Ԥ��
#define LOG_EVENT_RESET_ABNORMAL_STATE   0x4045      // �쳣״̬���
#define LOG_EVENT_RESET_QUERY_REQUEST    0x4046      // ��λ��ѯ����
#define LOG_EVENT_RESET_SYSTEM_STABLE    0x4047      // ϵͳ�ȶ�������

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* ========================== ��ȫ�ؼ���־��ݺ� ========================== */
/* �쳣���� */
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

/* ϵͳ���� */
#define LOG_WATCHDOG_RESET()           LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_WATCHDOG_RESET, "���Ź���λ")
#define LOG_EMERGENCY_STOP(reason)     LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_EMERGENCY_STOP, reason)
#define LOG_SYSTEM_LOCK(reason)        LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_SYSTEM_LOCK, reason)
#define LOG_POWER_FAILURE(desc)        LogSystem_Record(LOG_CATEGORY_SAFETY, 0, LOG_EVENT_POWER_FAILURE, desc)

/* �쳣��� */
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

/* ========================== ϵͳ״̬��־��ݺ� ========================== */
/* �������� */
#define LOG_SYSTEM_START()             LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "ϵͳ����")
#define LOG_SYSTEM_STOP()              LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_STOP, "ϵͳֹͣ")
#define LOG_SYSTEM_RESTART(reason)     LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_RESTART, reason)
#define LOG_POWER_ON_RESET()           LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_POWER_ON_RESET, "�ϵ縴λ")
#define LOG_SOFTWARE_RESET(reason)     LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SOFTWARE_RESET, reason)

/* ========================== ���������־��ݺ� ========================== */
/* ϵͳ���� */
#define LOG_FLASH_HEALTH(health)       LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_FLASH_HEALTH, health)
#define LOG_WATCHDOG_STATS(stats)      LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_WATCHDOG_STATS, stats)
#define LOG_MEMORY_USAGE(usage)        LogSystem_Record(LOG_CATEGORY_MONITOR, 0, LOG_EVENT_MEMORY_USAGE, usage)

/* ��λ����ϵͳ */
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

/* ϵͳ������ */
LogSystemStatus_t LogSystem_Init(void);
LogSystemStatus_t LogSystem_DeInit(void);
LogSystemStatus_t LogSystem_Reset(void);
LogSystemStatus_t LogSystem_Format(void);

/* ��־��¼���� - ͳһ�ӿ� */
LogSystemStatus_t LogSystem_Record(LogCategory_t category, uint8_t channel, uint16_t event_code, const char* message);
LogSystemStatus_t LogSystem_RecordFormatted(LogCategory_t category, uint8_t channel, uint16_t event_code, const char* format, ...);

/* ��־��ȡ���� */
LogSystemStatus_t LogSystem_ReadEntry(uint32_t entry_index, LogEntry_t* entry);
LogSystemStatus_t LogSystem_ReadLatestEntries(LogEntry_t* entries, uint32_t max_count, uint32_t* actual_count);
LogSystemStatus_t LogSystem_ReadEntriesByCategory(LogCategory_t category, LogEntry_t* entries, uint32_t max_count, uint32_t* actual_count);

/* ��־������� */
LogSystemStatus_t LogSystem_OutputAll(LogFormat_t format);
LogSystemStatus_t LogSystem_OutputByCategory(LogCategory_t category, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputByChannel(uint8_t channel, LogFormat_t format);
LogSystemStatus_t LogSystem_OutputLatest(uint32_t count, LogFormat_t format);

/* �������֧�ֺ��� */
uint32_t LogSystem_GetLogCount(void);
uint32_t LogSystem_GetCategoryLogCount(LogCategory_t category);
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
const char* LogSystem_GetCategoryString(uint8_t category);
const char* LogSystem_GetEventCodeString(uint16_t event_code);

/* �ڲ���֤���� */
uint8_t LogSystem_ValidateCategory(LogCategory_t category);

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __LOG_SYSTEM_H */ 

