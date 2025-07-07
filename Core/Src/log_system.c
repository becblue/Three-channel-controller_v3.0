/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : log_system.c
  * @brief          : ϵͳ������־ģ��ʵ���ļ�
  ******************************************************************************
  * @attention
  *
  * ����Ϊ��ͨ���л������ϵͳ��Ƶ���־��¼ϵͳʵ��
  * ֧�ֻ��θ��Ǵ洢�������ȼ�ء���������ȹ���
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

/* Flash������ʵ�� */
static FlashManager_t flash_manager = {0};

/* ��־�����ַ������� */
static const char* log_type_strings[] = {
    "SYS",      // LOG_TYPE_SYSTEM
    "CHN",      // LOG_TYPE_CHANNEL
    "ERR",      // LOG_TYPE_ERROR
    "WRN",      // LOG_TYPE_WARNING
    "INF",      // LOG_TYPE_INFO
    "DBG"       // LOG_TYPE_DEBUG
};

/* ȫ�ֱ�������OLED��ʾ */
extern uint8_t oled_show_flash_warning;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* �ڲ��������� */
static LogSystemStatus_t LogSystem_WriteEntry(const LogEntry_t* entry);
static LogSystemStatus_t LogSystem_PrepareNewSector(uint32_t address);
static uint32_t LogSystem_GetEntryAddress(uint32_t entry_index);
/* static LogSystemStatus_t LogSystem_ScanForLastEntry(void); */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  ��ʼ����־ϵͳ
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_Init(void)
{
    DEBUG_Printf("��ʼ��ʼ����־ϵͳ...\r\n");
    
    /* ��ʼ��W25Q128 Flash */
    DEBUG_Printf("���ڳ�ʼ��W25Q128 Flash����...\r\n");
    if (W25Q128_Init() != W25Q128_OK) {
        DEBUG_Printf("��־ϵͳ��ʼ��ʧ�ܣ�Flash��ʼ��ʧ��\r\n");
        DEBUG_Printf("���飺\r\n");
        DEBUG_Printf("1. W25Q128оƬ����\r\n");
        DEBUG_Printf("2. SPI2�ӿ�����\r\n");
        DEBUG_Printf("3. CS�����Ƿ�ӵ�\r\n");
        return LOG_SYSTEM_ERROR;
    }
    DEBUG_Printf("Flash������ʼ���ɹ�\r\n");
    
    /* ����Flash���������� */
    DEBUG_Printf("��ʼ��Flash������...\r\n");
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
    
    DEBUG_Printf("Flash���������ã�\r\n");
    DEBUG_Printf("- ��ʼ��ַ: 0x%08X\r\n", flash_manager.start_address);
    DEBUG_Printf("- ������ַ: 0x%08X\r\n", flash_manager.end_address);
    DEBUG_Printf("- ������: %d MB\r\n", flash_manager.total_size / (1024 * 1024));
    
    /* �������д��λ�� */
    DEBUG_Printf("���ڲ������д��λ��...\r\n");
    LogSystemStatus_t status = LogSystem_FindLastWritePosition();
    if (status != LOG_SYSTEM_OK) {
        DEBUG_Printf("��־ϵͳ��ʼ�����棺�޷��ҵ����д��λ�ã���ͷ��ʼ\r\n");
        flash_manager.write_address = LOG_SYSTEM_START_ADDR;
        flash_manager.total_entries = 0;
    } else {
        DEBUG_Printf("�ҵ����д��λ��: 0x%08X\r\n", flash_manager.write_address);
    }
    
    /* ���½����� */
    DEBUG_Printf("���ڸ��½�����...\r\n");
    LogSystem_UpdateHealthPercentage();
    
    /* ��ǳ�ʼ����� */
    flash_manager.is_initialized = 1;
    
    DEBUG_Printf("��־ϵͳ��ʼ���ɹ� (����:%dMB, ������:%d%%, ������־:%d��)\r\n", 
                 flash_manager.total_size / (1024 * 1024), 
                 flash_manager.health_percentage,
                 flash_manager.total_entries);
    
    /* ��¼ϵͳ������־ */
    LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "Log system initialized");
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ����ʼ����־ϵͳ
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_DeInit(void)
{
    /* ��¼ϵͳֹͣ��־ */
    LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_STOP, "Log system deinitialized");
    
    /* �ȴ�����д����� */
    SmartDelay(100);
    
    /* ����ʼ��Flash */
    W25Q128_DeInit();
    
    /* �����־ */
    flash_manager.is_initialized = 0;
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��¼��־��Ŀ
  * @param  log_type: ��־����
  * @param  channel: ͨ�����
  * @param  event_code: �¼�����
  * @param  message: ��־��Ϣ
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_Record(LogType_t log_type, uint8_t channel, uint16_t event_code, const char* message)
{
    LogEntry_t entry = {0};
    
    /* ����ʼ��״̬ */
    if (!flash_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    /* �����־��Ŀ */
    entry.timestamp = HAL_GetTick();
    entry.log_type = log_type;
    entry.channel = channel;
    entry.event_code = event_code;
    entry.magic_number = LOG_MAGIC_NUMBER;
    
    /* ��ȫ������Ϣ */
    if (message != NULL) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
    }
    
    /* ����У��� */
    entry.checksum = LogSystem_CalculateChecksum(&entry);
    
    /* д����־ */
    return LogSystem_WriteEntry(&entry);
}

/**
  * @brief  ��¼��ʽ����־��Ŀ
  * @param  log_type: ��־����
  * @param  channel: ͨ�����
  * @param  event_code: �¼�����
  * @param  format: ��ʽ���ַ���
  * @param  ...: �ɱ����
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_RecordFormatted(LogType_t log_type, uint8_t channel, uint16_t event_code, const char* format, ...)
{
    char message[48];
    va_list args;
    
    /* ��ʽ����Ϣ */
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    return LogSystem_Record(log_type, channel, event_code, message);
}

/**
  * @brief  ���������־
  * @param  format: �����ʽ
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_OutputAll(LogFormat_t format)
{
    uint32_t entry_count = flash_manager.total_entries;
    uint32_t entries_per_batch = 50;  // ÿ������50����־
    uint32_t current_batch = 0;
    LogEntry_t entries[50];
    
    /* ����ʼ��״̬ */
    if (!flash_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    /* ���ͷ����Ϣ */
    DEBUG_Printf("========== ϵͳ������־ ==========\r\n");
    DEBUG_Printf("����־����: %d\r\n", entry_count);
    DEBUG_Printf("�洢ʹ��: %.1f%%\r\n", (float)flash_manager.used_size * 100.0f / flash_manager.total_size);
    DEBUG_Printf("Flash������: %d%%\r\n", flash_manager.health_percentage);
    DEBUG_Printf("========================================\r\n");
    
    /* ���û����־��ֱ�ӷ��� */
    if (entry_count == 0) {
        DEBUG_Printf("������־��¼\r\n");
        return LOG_SYSTEM_OK;
    }
    
    /* ���������־ */
    while (current_batch * entries_per_batch < entry_count) {
        uint32_t batch_start = current_batch * entries_per_batch;
        uint32_t batch_size = entries_per_batch;
        
        /* �������һ���Ĵ�С */
        if (batch_start + batch_size > entry_count) {
            batch_size = entry_count - batch_start;
        }
        
        /* ��ȡһ����־ */
        for (uint32_t i = 0; i < batch_size; i++) {
            uint32_t entry_index = batch_start + i;
            uint32_t entry_address = LogSystem_GetEntryAddress(entry_index);
            
            if (W25Q128_ReadData(entry_address, (uint8_t*)&entries[i], sizeof(LogEntry_t)) != W25Q128_OK) {
                DEBUG_Printf("��ȡ��־ʧ�ܣ���ַ 0x%08X\r\n", entry_address);
                continue;
            }
            
            /* ��֤��־��Ŀ */
            if (!LogSystem_VerifyEntry(&entries[i])) {
                DEBUG_Printf("��־��֤ʧ�ܣ���Ŀ %d\r\n", entry_index);
                continue;
            }
        }
        
        /* ���������־ */
        for (uint32_t i = 0; i < batch_size; i++) {
            uint32_t entry_index = batch_start + i;
            LogEntry_t* entry = &entries[i];
            
            /* ��ʽ��ʱ��� */
            char time_str[16];
            LogSystem_FormatTimestamp(entry->timestamp, time_str, sizeof(time_str));
            
            /* ���ݸ�ʽ��� */
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
        
        /* ÿ���һ����������ʱ�����⴮�ڻ�������� */
        SmartDelay(50);
    }
    
    DEBUG_Printf("========================================\r\n");
    DEBUG_Printf("��־�����ɣ��� %d ��\r\n", entry_count);
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��ȡ��־��Ŀ����
  * @param  None
  * @retval uint32_t ��־��Ŀ����
  */
uint32_t LogSystem_GetEntryCount(void)
{
    return flash_manager.total_entries;
}

/**
  * @brief  ��ȡ��ʹ�ô�С
  * @param  None
  * @retval uint32_t ��ʹ���ֽ���
  */
uint32_t LogSystem_GetUsedSize(void)
{
    return flash_manager.used_size;
}

/**
  * @brief  ��ȡ�����Ȱٷֱ�
  * @param  None
  * @retval uint8_t �����Ȱٷֱ�
  */
uint8_t LogSystem_GetHealthPercentage(void)
{
    return flash_manager.health_percentage;
}

/**
  * @brief  ���洢���Ƿ�����
  * @param  None
  * @retval uint8_t 1-������0-δ��
  */
uint8_t LogSystem_IsFull(void)
{
    return flash_manager.is_full;
}

/**
  * @brief  ����Ƿ��ѳ�ʼ��
  * @param  None
  * @retval uint8_t 1-�ѳ�ʼ����0-δ��ʼ��
  */
uint8_t LogSystem_IsInitialized(void)
{
    return flash_manager.is_initialized;
}

/**
  * @brief  ���½����Ȱٷֱ�
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_UpdateHealthPercentage(void)
{
    /* ���ڲ����������㽡���� */
    uint32_t sectors_count = flash_manager.total_size / LOG_SECTOR_SIZE;
    uint32_t avg_erase_per_sector = flash_manager.total_erase_count / sectors_count;
    
    if (avg_erase_per_sector >= LOG_MAX_ERASE_CYCLES) {
        flash_manager.health_percentage = 0;
    } else {
        flash_manager.health_percentage = 100 - (avg_erase_per_sector * 100 / LOG_MAX_ERASE_CYCLES);
    }
    
    /* ����OLED��ʾ��־ */
    if (flash_manager.health_percentage <= LOG_HEALTH_WARNING_THRESHOLD) {
        oled_show_flash_warning = 1;
    } else {
        oled_show_flash_warning = 0;
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  �������д��λ��
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_FindLastWritePosition(void)
{
    uint32_t search_address = LOG_SYSTEM_START_ADDR;
    uint32_t entry_count = 0;
    LogEntry_t entry;
    
    /* ɨ��������־���� */
    while (search_address < LOG_SYSTEM_END_ADDR) {
        /* ��ȡһ����־��Ŀ */
        if (W25Q128_ReadData(search_address, (uint8_t*)&entry, sizeof(LogEntry_t)) != W25Q128_OK) {
            break;
        }
        
        /* ����Ƿ�����Ч����־��Ŀ */
        if (entry.magic_number == LOG_MAGIC_NUMBER && LogSystem_VerifyEntry(&entry)) {
            entry_count++;
            search_address += LOG_ENTRY_SIZE;
        } else {
            /* �ҵ��հ��������������һ��д��λ�� */
            break;
        }
    }
    
    /* ����Flash������״̬ */
    flash_manager.write_address = search_address;
    flash_manager.total_entries = entry_count;
    flash_manager.used_size = entry_count * LOG_ENTRY_SIZE;
    
    /* ����Ƿ����� */
    if (search_address >= LOG_SYSTEM_END_ADDR) {
        flash_manager.is_full = 1;
        flash_manager.write_address = LOG_SYSTEM_START_ADDR;  // ���θ���
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ����У���
  * @param  entry: ��־��Ŀָ��
  * @retval uint32_t У���ֵ
  */
uint32_t LogSystem_CalculateChecksum(const LogEntry_t* entry)
{
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)entry;
    
    /* �����У����ֶ���������ֶ� */
    for (uint32_t i = 0; i < sizeof(LogEntry_t) - 8; i++) {  // �ų�checksum��magic_number
        checksum += data[i];
    }
    
    return checksum;
}

/**
  * @brief  ��֤��־��Ŀ
  * @param  entry: ��־��Ŀָ��
  * @retval uint8_t 1-��Ч��0-��Ч
  */
uint8_t LogSystem_VerifyEntry(const LogEntry_t* entry)
{
    if (entry == NULL) {
        return 0;
    }
    
    /* ���ħ�� */
    if (entry->magic_number != LOG_MAGIC_NUMBER) {
        return 0;
    }
    
    /* ���У��� */
    uint32_t calculated_checksum = LogSystem_CalculateChecksum(entry);
    if (calculated_checksum != entry->checksum) {
        return 0;
    }
    
    /* �����־���� */
    if (entry->log_type >= sizeof(log_type_strings) / sizeof(log_type_strings[0])) {
        return 0;
    }
    
    return 1;
}

/**
  * @brief  ��ʽ��ʱ���
  * @param  timestamp: ʱ��������룩
  * @param  buffer: ���������
  * @param  buffer_size: ��������С
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
  * @brief  ��ȡ��־�����ַ���
  * @param  log_type: ��־����
  * @retval const char* �����ַ���
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
  * @brief  д����־��Ŀ��Flash
  * @param  entry: ��־��Ŀָ��
  * @retval LogSystemStatus_t ����״̬
  */
static LogSystemStatus_t LogSystem_WriteEntry(const LogEntry_t* entry)
{
    /* ����Ƿ���Ҫ���������� */
    if ((flash_manager.write_address % LOG_SECTOR_SIZE) == 0) {
        if (LogSystem_PrepareNewSector(flash_manager.write_address) != LOG_SYSTEM_OK) {
            return LOG_SYSTEM_ERROR;
        }
    }
    
    /* д����־��Ŀ */
    if (W25Q128_WriteData(flash_manager.write_address, (uint8_t*)entry, sizeof(LogEntry_t)) != W25Q128_OK) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* ���¹�����״̬ */
    flash_manager.write_address += LOG_ENTRY_SIZE;
    flash_manager.total_entries++;
    flash_manager.used_size += LOG_ENTRY_SIZE;
    flash_manager.total_write_count++;
    
    /* ����Ƿ���Ҫ���θ��� */
    if (flash_manager.write_address >= LOG_SYSTEM_END_ADDR) {
        flash_manager.write_address = LOG_SYSTEM_START_ADDR;
        flash_manager.is_full = 1;
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ׼����������������
  * @param  address: ������ַ
  * @retval LogSystemStatus_t ����״̬
  */
static LogSystemStatus_t LogSystem_PrepareNewSector(uint32_t address)
{
    /* �������� */
    if (W25Q128_EraseSector(address) != W25Q128_OK) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* ���²������� */
    flash_manager.total_erase_count++;
    
    /* ���½����� */
    LogSystem_UpdateHealthPercentage();
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��ȡ��־��Ŀ��ַ
  * @param  entry_index: ��Ŀ����
  * @retval uint32_t ��ַ
  */
static uint32_t LogSystem_GetEntryAddress(uint32_t entry_index)
{
    uint32_t address = LOG_SYSTEM_START_ADDR + (entry_index * LOG_ENTRY_SIZE);
    
    /* ���ε�ַ���� */
    if (address >= LOG_SYSTEM_END_ADDR) {
        address = LOG_SYSTEM_START_ADDR + ((address - LOG_SYSTEM_START_ADDR) % LOG_SYSTEM_SIZE);
    }
    
    return address;
}

/**
  * @brief  ��ʽ����־ϵͳ�����������־��
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_Format(void)
{
    /* ����ʼ��״̬ */
    if (!flash_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    DEBUG_Printf("��ʼ��ʽ����־ϵͳ�����������־��...\r\n");
    
    /* ����������־�����ǰ����������Ϊʾ����Ϊ��ȫ������������У� */
    uint32_t sectors_to_erase = 10; // ֻ����ǰ10��������Ϊ��ʾ
    uint32_t current_address = LOG_SYSTEM_START_ADDR;
    
    for (uint32_t i = 0; i < sectors_to_erase && current_address < LOG_SYSTEM_END_ADDR; i++) {
        DEBUG_Printf("���ڲ������� %d (��ַ: 0x%08X)...\r\n", i + 1, current_address);
        
        if (W25Q128_EraseSector(current_address) != W25Q128_OK) {
            DEBUG_Printf("��������ʧ�ܣ���ַ 0x%08X\r\n", current_address);
            return LOG_SYSTEM_ERROR;
        }
        
        current_address += LOG_SECTOR_SIZE;
        
        /* �����ʱ�����⿴�Ź���ʱ */
        SmartDelay(10);
    }
    
    /* ����Flash������״̬ */
    flash_manager.write_address = LOG_SYSTEM_START_ADDR;
    flash_manager.used_size = 0;
    flash_manager.total_entries = 0;
    flash_manager.total_erase_count += sectors_to_erase;
    flash_manager.is_full = 0;
    
    /* ���½����� */
    LogSystem_UpdateHealthPercentage();
    
    DEBUG_Printf("��־ϵͳ��ʽ�����\r\n");
    DEBUG_Printf("- �Ѳ���������: %d\r\n", sectors_to_erase);
    DEBUG_Printf("- ��ǰ��־����: %d\r\n", flash_manager.total_entries);
    DEBUG_Printf("- Flash������: %d%%\r\n", flash_manager.health_percentage);
    
    /* ��¼��ʽ����־ */
    LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "Log system formatted");
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ������־ϵͳ��������գ�������ָ�룩
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_Reset(void)
{
    /* ����ʼ��״̬ */
    if (!flash_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    DEBUG_Printf("����������־ϵͳ...\r\n");
    
    /* ����Flash������״̬����ʵ�ʲ���Flash��ֻ����ָ�룩 */
    flash_manager.write_address = LOG_SYSTEM_START_ADDR;
    flash_manager.used_size = 0;
    flash_manager.total_entries = 0;
    flash_manager.is_full = 0;
    
    DEBUG_Printf("��־ϵͳ������ɣ��߼���գ�\r\n");
    DEBUG_Printf("- ��ǰ��־����: %d\r\n", flash_manager.total_entries);
    DEBUG_Printf("- д���ַ����Ϊ: 0x%08X\r\n", flash_manager.write_address);
    
    /* ��¼������־ */
    LogSystem_Record(LOG_TYPE_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "Log system reset");
    
    return LOG_SYSTEM_OK;
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */ 


