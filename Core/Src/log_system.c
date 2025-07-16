/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : log_system.c
  * @brief          : ϵͳ������־ģ��ʵ���ļ� - �������־ϵͳ
  ******************************************************************************
  * @attention
  *
  * �������־ϵͳʵ�֣�ֻ��¼�����¼���
  * 1. �쳣�¼���¼
  * 2. ϵͳ�����¼�
  * 3. ϵͳ��������
  * 4. ϵͳ�������
  * 
  * ����ͳһ�洢��ѭ�����ǲ��ԣ��������¼�¼
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

/* ȫ����־������ - ͳһ�洢���� */
static LogManager_t log_manager = {0};

/* ��־��������ӳ�������棩 */
static const char* log_category_strings[4] = {
    "INVALID",  // 0 - ��Ч����
    "SAFETY",   // LOG_CATEGORY_SAFETY
    "SYSTEM",   // LOG_CATEGORY_SYSTEM
    "MONITOR"   // LOG_CATEGORY_MONITOR
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
static LogSystemStatus_t LogSystem_HandleCircularOverwrite(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  ��ʼ����־ϵͳ - ͳһ�洢�汾
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_Init(void)
{
    DEBUG_Printf("��ʼ��ʼ��ͳһ�洢��־ϵͳ...\r\n");
    
    /* ι������ֹ��ʼ�������п��Ź���λ */
    HAL_IWDG_Refresh(&hiwdg);
    
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
    
    /* ι�� */
    HAL_IWDG_Refresh(&hiwdg);
    
    /* ��ʼ��ͳһ��־������ */
    DEBUG_Printf("��ʼ��ͳһ�洢������...\r\n");
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
    
    /* �������д��λ�� */
    DEBUG_Printf("ɨ��洢���򣬲������д��λ��...\r\n");
    LogSystemStatus_t status = LogSystem_FindLastWritePosition();
    if (status != LOG_SYSTEM_OK) {
        DEBUG_Printf("��λ���д��λ��ʧ�ܣ�����Ϊ��ʼλ��\r\n");
        log_manager.write_address = LOG_SYSTEM_START_ADDR;
        log_manager.total_entries = 0;
    }
    
    /* ���½����� */
    DEBUG_Printf("���ڸ��½�����...\r\n");
    LogSystem_UpdateHealthPercentage();
    
    /* ��ǳ�ʼ����� */
    log_manager.is_initialized = 1;
    
    /* �����ʼ��ͳ����Ϣ */
    DEBUG_Printf("ͳһ�洢��־ϵͳ��ʼ���ɹ�:\r\n");
    DEBUG_Printf("- ������: %d MB (������:%d%%)\r\n", 
                 LOG_SYSTEM_SIZE / (1024 * 1024), 
                 log_manager.health_percentage);
    DEBUG_Printf("- �洢����: 0x%08X - 0x%08X\r\n", 
                 log_manager.start_address, log_manager.end_address);
    DEBUG_Printf("- ��ǰд��λ��: 0x%08X\r\n", log_manager.write_address);
    DEBUG_Printf("- ������־: %d ��\r\n", log_manager.total_entries);
    DEBUG_Printf("- �������: %d ��\r\n", log_manager.max_entries);
    
    /* ��¼ϵͳ������־ */
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_START, "ͳһ�洢��־ϵͳ����");
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��¼��־��Ŀ - �½ӿ�
  * @param  category: ��־����
  * @param  channel: ͨ�����
  * @param  event_code: �¼�����
  * @param  message: ��־��Ϣ
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_Record(LogCategory_t category, uint8_t channel, uint16_t event_code, const char* message)
{
    LogEntry_t entry = {0};
    
    /* ����ʼ��״̬ */
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    /* ��֤���� */
    if (!LogSystem_ValidateCategory(category)) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* �����־��Ŀ */
    entry.timestamp = HAL_GetTick();
    entry.log_category = category;
    entry.channel = channel;
    entry.event_code = event_code;
    
    /* ��ȫ������Ϣ */
    if (message) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
    }
    
    /* ����У��� */
    entry.checksum = LogSystem_CalculateChecksum(&entry);
    entry.magic_number = LOG_MAGIC_NUMBER;
    
    /* д�뵽��Ӧ���� */
    return LogSystem_WriteEntry(&entry);
}

/**
  * @brief  ��ʽ����¼��־��Ŀ
  * @param  category: ��־����
  * @param  channel: ͨ�����
  * @param  event_code: �¼�����
  * @param  format: ��ʽ���ַ���
  * @param  ...: �ɱ����
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_RecordFormatted(LogCategory_t category, uint8_t channel, uint16_t event_code, const char* format, ...)
{
    char formatted_message[48];
    va_list args;
    
    /* ��ʽ����Ϣ */
    va_start(args, format);
    vsnprintf(formatted_message, sizeof(formatted_message), format, args);
    va_end(args);
    
    /* ���ñ�׼��¼���� */
    return LogSystem_Record(category, channel, event_code, formatted_message);
}

/**
  * @brief  ����־��Ŀд��ָ������
  * @param  entry: ��־��Ŀָ��
  * @retval LogSystemStatus_t ����״̬
  */
static LogSystemStatus_t LogSystem_WriteEntry(const LogEntry_t* entry)
{
    if (!entry) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* ����Ƿ���Ҫ���������� */
    if ((log_manager.write_address % LOG_SECTOR_SIZE) == 0) {
        LogSystemStatus_t status = LogSystem_PrepareNewSector(log_manager.write_address);
        if (status != LOG_SYSTEM_OK) {
            DEBUG_Printf("��־ϵͳ׼��������ʧ��\r\n");
            return status;
        }
    }
    
    /* д��Flash */
    if (W25Q128_WriteData(log_manager.write_address, (uint8_t*)entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
        DEBUG_Printf("��־ϵͳд��Flashʧ�ܣ���ַ:0x%08X\r\n", log_manager.write_address);
        return LOG_SYSTEM_ERROR;
    }
    
    /* ���¹�����״̬ */
    log_manager.write_address += LOG_ENTRY_SIZE;
    log_manager.total_write_count++;
    
    /* ����Ƿ���Ҫѭ������ */
    if (log_manager.write_address > log_manager.end_address) {
        log_manager.write_address = log_manager.start_address; // �ص���ʼλ��
        log_manager.is_full = 1;
        
        DEBUG_Printf("��־ϵͳ����ѭ�����ǣ��ص���ʼλ��\r\n");
        
        /* ����ѭ�������߼� */
        LogSystem_HandleCircularOverwrite();
    } else if (!log_manager.is_full) {
        log_manager.total_entries++;
        log_manager.used_size += LOG_ENTRY_SIZE;
        log_manager.newest_entry_index = log_manager.total_entries;
    } else {
        /* ����״̬�£������������� */
        log_manager.newest_entry_index++;
        if (log_manager.newest_entry_index >= log_manager.max_entries) {
            log_manager.newest_entry_index = 0;
        }
        
        /* ����ѭ�������߼� */
        LogSystem_HandleCircularOverwrite();
    }
    
    /* ����SAFETY���࣬�����ϸ��Ϣ */
    if (entry->log_category == LOG_CATEGORY_SAFETY) {
        DEBUG_Printf("SAFETY��־��¼: [%s] %s\r\n", 
                     LogSystem_GetEventCodeString(entry->event_code), 
                     entry->message);
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  Ϊָ������׼��������
  * @param  address: ������ַ
  * @retval LogSystemStatus_t ����״̬
  */
static LogSystemStatus_t LogSystem_PrepareNewSector(uint32_t address)
{
    /* ����ַ��Ч�� */
    if (address < log_manager.start_address || address > log_manager.end_address) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* ����ַ�Ƿ��������߽� */
    if ((address % LOG_SECTOR_SIZE) != 0) {
        address = (address / LOG_SECTOR_SIZE) * LOG_SECTOR_SIZE;
    }
    
    DEBUG_Printf("׼����־ϵͳ������: 0x%08X\r\n", address);
    
    /* �������� */
    if (W25Q128_EraseSector(address) != W25Q128_OK) {
        DEBUG_Printf("��־ϵͳ��������ʧ��: 0x%08X\r\n", address);
        return LOG_SYSTEM_ERROR;
    }
    
    /* ���²������� */
    log_manager.total_erase_count++;
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ���������־
  * @param  format: �����ʽ
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_OutputAll(LogFormat_t format)
{
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    LogSystem_OutputHeader();
    
    DEBUG_Printf("========== ��־ϵͳ������� ==========\r\n");
    
    /* ��ʱ��˳�����������־ */
    uint32_t entries_to_read = log_manager.is_full ? log_manager.max_entries : log_manager.total_entries;
    
    for (uint32_t i = 0; i < entries_to_read; i++) {
        LogEntry_t entry;
        uint32_t actual_index = log_manager.is_full ? 
            ((log_manager.oldest_entry_index + i) % log_manager.max_entries) : i;
        uint32_t entry_addr = LogSystem_GetEntryAddress(actual_index);
        
        if (W25Q128_ReadData(entry_addr, (uint8_t*)&entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
            DEBUG_Printf("��ȡʧ�ܣ���ַ:0x%08X\r\n", entry_addr);
            break;
        }
        
        /* ��֤��Ŀ */
        if (LogSystem_VerifyEntry(&entry) && entry.log_category > 0 && entry.log_category <= 4) {
            /* ��ʽ����� */
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
        
        /* ����ι�� */
        if ((i % 100) == 0) {
            HAL_IWDG_Refresh(&hiwdg);
        }
    }
    
    LogSystem_OutputFooter();
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ���ָ���������־
  * @param  category: ��־����
  * @param  format: �����ʽ
  * @retval LogSystemStatus_t ����״̬
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
        DEBUG_Printf("���� %s ����־��¼\r\n", log_category_strings[category]);
        return LOG_SYSTEM_OK;
    }
    
    DEBUG_Printf("������� %s ��־...\r\n", log_category_strings[category]);
    
    for (uint32_t i = 0; i < entries_to_read; i++) {
        LogEntry_t entry;
        uint32_t actual_index = log_manager.is_full ? 
            ((log_manager.oldest_entry_index + i) % log_manager.max_entries) : i;
        uint32_t entry_addr = LogSystem_GetEntryAddress(actual_index);
        
        if (W25Q128_ReadData(entry_addr, (uint8_t*)&entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
            DEBUG_Printf("��ȡʧ�ܣ���ַ:0x%08X\r\n", entry_addr);
            break;
        }
        
        /* ��֤��Ŀ */
        if (LogSystem_VerifyEntry(&entry) && entry.log_category == category) {
            /* ��ʽ����� */
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
        
        /* ����ι�� */
        if ((i % 100) == 0) {
            HAL_IWDG_Refresh(&hiwdg);
        }
    }
    
    DEBUG_Printf("���� %s �����ɣ��� %d ����¼\r\n", 
                 log_category_strings[category], entries_to_read);
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��ȡ��־����
  * @param  None
  * @retval ��־����
  */
uint32_t LogSystem_GetEntryCount(void)
{
    return log_manager.total_entries;
}

/**
  * @brief  ��ȡ��ʹ������
  * @param  None
  * @retval ��ʹ���ֽ���
  */
uint32_t LogSystem_GetUsedSize(void)
{
    return log_manager.used_size;
}

/**
  * @brief  ��ȡ�����Ȱٷֱ�
  * @param  None
  * @retval �����Ȱٷֱ�
  */
uint8_t LogSystem_GetHealthPercentage(void)
{
    return log_manager.health_percentage;
}

/**
  * @brief  ����Ƿ�����
  * @param  None
  * @retval 1:���� 0:δ��
  */
uint8_t LogSystem_IsFull(void)
{
    return log_manager.is_full;
}

/**
  * @brief  ����Ƿ��ѳ�ʼ��
  * @param  None
  * @retval 1:�ѳ�ʼ�� 0:δ��ʼ��
  */
uint8_t LogSystem_IsInitialized(void)
{
    return log_manager.is_initialized;
}

/**
  * @brief  ���½����Ȱٷֱ�
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_UpdateHealthPercentage(void)
{
    /* �򻯵Ľ����ȼ��㣺���ڲ������� */
    if (log_manager.total_erase_count < LOG_MAX_ERASE_CYCLES / 10) {
        log_manager.health_percentage = 100;
    } else if (log_manager.total_erase_count < LOG_MAX_ERASE_CYCLES / 2) {
        log_manager.health_percentage = 80;
    } else if (log_manager.total_erase_count < LOG_MAX_ERASE_CYCLES) {
        log_manager.health_percentage = 50;
    } else {
        log_manager.health_percentage = 20;
    }
    
    /* �����Ⱦ��� */
    if (log_manager.health_percentage <= LOG_HEALTH_WARNING_THRESHOLD) {
        oled_show_flash_warning = 1;
        DEBUG_Printf("Flash�����Ⱦ���: %d%%\r\n", log_manager.health_percentage);
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ������ĿУ���
  * @param  entry: ��־��Ŀָ��
  * @retval У���
  */
uint32_t LogSystem_CalculateChecksum(const LogEntry_t* entry)
{
    if (!entry) return 0;
    
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)entry;
    
    /* ����checksum��magic_number�ֶ� */
    for (uint32_t i = 0; i < sizeof(LogEntry_t) - 8; i++) {
        checksum += data[i];
    }
    
    return checksum;
}

/**
  * @brief  ��֤��Ŀ��Ч��
  * @param  entry: ��־��Ŀָ��
  * @retval 1:��Ч 0:��Ч
  */
uint8_t LogSystem_VerifyEntry(const LogEntry_t* entry)
{
    if (!entry) return 0;
    
    /* ���ħ�� */
    if (entry->magic_number != LOG_MAGIC_NUMBER) {
        return 0;
    }
    
    /* ���У��� */
    uint32_t calculated_checksum = LogSystem_CalculateChecksum(entry);
    if (calculated_checksum != entry->checksum) {
        return 0;
    }
    
    /* ��������Ч�� */
    if (!LogSystem_ValidateCategory((LogCategory_t)entry->log_category)) {
        return 0;
    }
    
    return 1;
}

/**
  * @brief  ��ʽ��ʱ���
  * @param  timestamp: ʱ���
  * @param  buffer: ���������
  * @param  buffer_size: ��������С
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
  * @brief  �����־ͷ
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_OutputHeader(void)
{
    DEBUG_Printf("\r\n==================== ��־ϵͳ��� ====================\r\n");
    DEBUG_Printf("ʱ���        | ����      | CH | �¼� | ��Ϣ\r\n");
    DEBUG_Printf("--------------------------------------------------------\r\n");
    return LOG_SYSTEM_OK;
}

/**
  * @brief  �����־β
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_OutputFooter(void)
{
    DEBUG_Printf("--------------------------------------------------------\r\n");
    DEBUG_Printf("����Ŀ��: %d, ���ÿռ�: %d KB, ������: %d%%\r\n", 
                 LogSystem_GetEntryCount(),
                 LogSystem_GetUsedSize() / 1024,
                 LogSystem_GetHealthPercentage());
    DEBUG_Printf("=====================================================\r\n\r\n");
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ���ݺ������������д��λ��
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_FindLastWritePosition(void)
{
    uint32_t address = log_manager.start_address;
    uint32_t entry_count = 0;
    LogEntry_t temp_entry;
    LogResetMarker_t reset_marker;
    
    DEBUG_Printf("ɨ����־ϵͳ�洢����...\r\n");
    
    /* ���ȼ���Ƿ�������ñ�� */
    if (W25Q128_ReadData(log_manager.start_address, (uint8_t*)&reset_marker, LOG_RESET_MARKER_SIZE) == W25Q128_OK) {
        if (reset_marker.reset_magic == LOG_RESET_MARKER_MAGIC && reset_marker.final_magic == LOG_MAGIC_NUMBER) {
            DEBUG_Printf("�������ñ�ǣ�ʱ���:%u�����ô���:%u\r\n", 
                         reset_marker.reset_timestamp, reset_marker.reset_count);
            DEBUG_Printf("����ԭ��: %s\r\n", reset_marker.reset_reason);
            
            /* ���¹�����״̬Ϊ��״̬ */
            log_manager.write_address = log_manager.start_address;  // ����־���������ñ��
            log_manager.total_entries = 0;
            log_manager.used_size = 0;
            log_manager.total_reset_count = reset_marker.reset_count;
            
            DEBUG_Printf("�Ѵ�����״̬�ָ����´�д�뽫�������ñ��\r\n");
            return LOG_SYSTEM_OK;
        }
    }
    
    DEBUG_Printf("δ�������ñ�ǣ���ʼ����ɨ��...\r\n");
    
    /* ɨ����־ϵͳ�Ĵ洢���� */
    while (address + LOG_ENTRY_SIZE <= log_manager.end_address + 1) {
        /* ����ι�� */
        if ((entry_count % 1000) == 0) {
            HAL_IWDG_Refresh(&hiwdg);
        }
        
        /* ��ȡ��Ŀ */
        if (W25Q128_ReadData(address, (uint8_t*)&temp_entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
            DEBUG_Printf("��־ϵͳ��ȡʧ�ܣ���ַ:0x%08X\r\n", address);
            break;
        }
        
        /* �����Ŀ��Ч�� */
        if (!LogSystem_VerifyEntry(&temp_entry) || temp_entry.log_category < LOG_CATEGORY_SAFETY || temp_entry.log_category > LOG_CATEGORY_MONITOR) {
            /* �ҵ���һ����Ч��Ŀ���������д��λ�� */
            break;
        }
        
        /* ������һ��Ŀ */
        address += LOG_ENTRY_SIZE;
        entry_count++;
        
        /* ��ֹɨ�賬ʱ */
        if (entry_count > log_manager.max_entries) {
            DEBUG_Printf("��־ϵͳɨ����Ŀ�����ޣ����ܴ洢������\r\n");
            log_manager.is_full = 1;
            break;
        }
    }
    
    /* ���¹�����״̬ */
    log_manager.write_address = address;
    log_manager.total_entries = entry_count;
    log_manager.used_size = entry_count * LOG_ENTRY_SIZE;
    
    /* ����Ƿ����� */
    if (log_manager.write_address >= log_manager.end_address + 1) {
        log_manager.is_full = 1;
        log_manager.write_address = log_manager.start_address; // ѭ�����ǣ��ص���ʼλ��
        DEBUG_Printf("��־ϵͳ�洢������������ѭ������ģʽ\r\n");
    }
    
    DEBUG_Printf("��־ϵͳɨ�����: �ҵ� %d ����־��д��λ��:0x%08X\r\n",
                 entry_count, log_manager.write_address);
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��ȡ��־����
  * @param  None
  * @retval ��־����
  */
uint32_t LogSystem_GetLogCount(void)
{
    return LogSystem_GetEntryCount();
}

/**
  * @brief  ��ȡָ���������־����
  * @param  category: ��־����
  * @retval �÷������־����
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
            DEBUG_Printf("��ȡʧ�ܣ���ַ:0x%08X\r\n", entry_addr);
            break;
        }
        
        if (entry.log_category == category) {
            count++;
        }
        
        /* ����ι�� */
        if ((i % 100) == 0) {
            HAL_IWDG_Refresh(&hiwdg);
        }
    }
    
    return count;
}

/**
  * @brief  ��ȡָ����������־��Ŀ
  * @param  entry_index: ��־��Ŀ����
  * @param  entry: ��־��Ŀָ��
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_ReadEntry(uint32_t entry_index, LogEntry_t* entry)
{
    if (!entry) {
        return LOG_SYSTEM_ERROR;
    }
    
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    /* ���������Ч�� */
    if (entry_index >= log_manager.total_entries) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* ����ʵ�ʵ�ַ */
    uint32_t entry_addr = LogSystem_GetEntryAddress(entry_index);
    
    /* ��Flash��ȡ���� */
    if (W25Q128_ReadData(entry_addr, (uint8_t*)entry, LOG_ENTRY_SIZE) != W25Q128_OK) {
        return LOG_SYSTEM_ERROR;
    }
    
    /* ��֤��ȡ����Ŀ */
    if (!LogSystem_VerifyEntry(entry)) {
        return LOG_SYSTEM_ERROR;
    }
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��ʽ����־ϵͳ����ȫ�����������ݣ�
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_Format(void)
{
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    // ��ͣ��־д��
    log_manager.is_initialized = 0;
    
    // ������Ҫ��������������
    uint32_t sector_size = 4096;  // 4KB����
    uint32_t start_sector = log_manager.start_address / sector_size;
    uint32_t end_sector = log_manager.end_address / sector_size;
    
    // �����������
    for (uint32_t sector = start_sector; sector <= end_sector; sector++) {
        uint32_t sector_addr = sector * sector_size;
        
        // ʹ��W25Q128��������������
        if (W25Q128_EraseSector(sector_addr) != W25Q128_OK) {
            // �ָ���ʼ��״̬
            log_manager.is_initialized = 1;
            return LOG_SYSTEM_ERROR;
        }
        
        // ι����ֹ���������и�λ
        HAL_IWDG_Refresh(&hiwdg);
    }
    
    // ���³�ʼ��ϵͳ
    return LogSystem_Init();
}

/**
  * @brief  ���������־��¼
  * @param  index: ��־����
  * @param  format: �����ʽ
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_OutputSingle(uint32_t index, LogFormat_t format)
{
    if (!log_manager.is_initialized) {
        DEBUG_Printf("��־ϵͳδ��ʼ��\r\n");
        return LOG_SYSTEM_NOT_INIT;
    }
    
    // ���������Ч��
    uint32_t total_entries = LogSystem_GetEntryCount();
    if (index >= total_entries) {
        DEBUG_Printf("����������Χ: %d >= %d\r\n", index, total_entries);
        return LOG_SYSTEM_ERROR;
    }
    
    LogEntry_t entry;
    char time_str[32];
    
    // ��ȡ��־��Ŀ
    uint32_t entry_addr = log_manager.start_address + (index * LOG_ENTRY_SIZE);
    if (W25Q128_ReadData(entry_addr, (uint8_t*)&entry, sizeof(LogEntry_t)) != W25Q128_OK) {
        DEBUG_Printf("��ȡ��־ʧ��: ���� %d\r\n", index);
        return LOG_SYSTEM_ERROR;
    }
    
    // ��֤��־��Ŀ
    if (entry.magic_number != LOG_MAGIC_NUMBER || entry.log_category > 3) {
        DEBUG_Printf("��Ч��־��Ŀ: ���� %d\r\n", index);
        return LOG_SYSTEM_ERROR;
    }
    
    // ��ʽ��ʱ���
    LogSystem_FormatTimestamp(entry.timestamp, time_str, sizeof(time_str));
    
    // ���ݸ�ʽ���
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
  * @brief  ������־ϵͳ�����������־��
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_Reset(void)
{
    if (!log_manager.is_initialized) {
        return LOG_SYSTEM_NOT_INIT;
    }
    
    DEBUG_Printf("��ʼ���������־ϵͳ...\r\n");
    
    /* �������ñ�� */
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
    
    /* �������ñ�ǵ�У��� */
    uint32_t checksum = 0;
    uint8_t* data = (uint8_t*)&reset_marker;
    for (int i = 0; i < sizeof(LogResetMarker_t) - 8; i++) { // �ų�checksum��final_magic
        checksum += data[i];
    }
    reset_marker.checksum = checksum;
    
    /* ����Ƿ���Ҫ������һ������ */
    if ((log_manager.start_address % LOG_SECTOR_SIZE) == 0) {
        DEBUG_Printf("������һ����������д�����ñ��...\r\n");
        if (W25Q128_EraseSector(log_manager.start_address) != W25Q128_OK) {
            DEBUG_Printf("������һ������ʧ��\r\n");
            return LOG_SYSTEM_ERROR;
        }
        log_manager.total_erase_count++;
    }
    
    /* д�����ñ�ǵ�Flash��ͷ */
    if (W25Q128_WriteData(log_manager.start_address, (uint8_t*)&reset_marker, LOG_RESET_MARKER_SIZE) != W25Q128_OK) {
        DEBUG_Printf("д�����ñ��ʧ��\r\n");
        return LOG_SYSTEM_ERROR;
    }
    
    /* ���¹���ṹΪ��״̬ */
    log_manager.write_address = log_manager.start_address;
    log_manager.read_address = log_manager.start_address;
    log_manager.used_size = 0;
    log_manager.total_entries = 0;
    log_manager.oldest_entry_index = 0;
    log_manager.newest_entry_index = 0;
    log_manager.is_full = 0;
    log_manager.total_write_count = 0;
    log_manager.total_reset_count++;
    
    /* ���ֳ�ʼ��״̬�ͽ����Ȳ��� */
    
    DEBUG_Printf("��־ϵͳ��������գ�\r\n");
    DEBUG_Printf("- ���ñ����д��Flash��ʱ���: %u\r\n", reset_marker.reset_timestamp);
    DEBUG_Printf("- ���ô���: %u\r\n", log_manager.total_reset_count);
    DEBUG_Printf("- д��λ������Ϊ: 0x%08X\r\n", log_manager.write_address);
    DEBUG_Printf("- ϵͳ�����󽫴����ñ�ǻָ���״̬\r\n");
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��ȡָ����������־��Ŀ��ַ
  * @param  entry_index: ��־��Ŀ����
  * @retval ��־��Ŀ��ַ
  */
static uint32_t LogSystem_GetEntryAddress(uint32_t entry_index)
{
    return log_manager.start_address + (entry_index * LOG_ENTRY_SIZE);
}

/**
  * @brief  ����ѭ�������߼�
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
static LogSystemStatus_t LogSystem_HandleCircularOverwrite(void)
{
    if (!log_manager.is_full) {
        return LOG_SYSTEM_OK;  // ��δ��������ѭ������
    }
    
    /* ��ѭ������ģʽ�£�ȷ��oldest_entry_indexʼ��ָ����ɵ���Ч��¼ */
    /* ��newest_entry_index��ǰ�ƶ�ʱ��oldest_entry_indexҲҪ��Ӧ���� */
    
    /* ���㵱ǰд�����Ŀ���� */
    uint32_t current_entry_index = (log_manager.write_address - log_manager.start_address) / LOG_ENTRY_SIZE;
    
    /* ��ѭ��ģʽ�£�oldest_entry_indexӦ�ø���д��λ�� */
    log_manager.oldest_entry_index = current_entry_index;
    
    /* ȷ���������ᳬ����Χ */
    if (log_manager.oldest_entry_index >= log_manager.max_entries) {
        log_manager.oldest_entry_index = log_manager.oldest_entry_index % log_manager.max_entries;
    }
    
    /* ������״̬�£�����Ŀ������Ϊ���ֵ */
    log_manager.total_entries = log_manager.max_entries;
    log_manager.used_size = log_manager.max_entries * LOG_ENTRY_SIZE;
    
    DEBUG_Printf("ѭ������: oldest_index=%d, newest_index=%d, current_pos=0x%08X\r\n",
                 log_manager.oldest_entry_index, 
                 log_manager.newest_entry_index,
                 log_manager.write_address);
    
    return LOG_SYSTEM_OK;
}

/**
  * @brief  ��֤������Ч��
  * @param  category: ��־����
  * @retval 1:��Ч 0:��Ч
  */
uint8_t LogSystem_ValidateCategory(LogCategory_t category)
{
    return (category == LOG_CATEGORY_SAFETY || category == LOG_CATEGORY_SYSTEM || category == LOG_CATEGORY_MONITOR);
}

/**
  * @brief  ��ȡ�����ַ���
  * @param  category: ��־����
  * @retval �����ַ���
  */
const char* LogSystem_GetCategoryString(uint8_t category)
{
    if (category >= 1 && category <= 3) {
        return log_category_strings[category];
    }
    return "INVALID";
}

/**
  * @brief  ��ȡ�¼������ַ���
  * @param  event_code: �¼�����
  * @retval �¼������ַ���
  */
const char* LogSystem_GetEventCodeString(uint16_t event_code)
{
    /* �����¼����뷶Χ�������� */
    if (event_code >= 0x1000 && event_code <= 0x1FFF) {
        if (event_code <= 0x100F) return "��ȫ�쳣";
        if (event_code <= 0x102F) return "ϵͳ����";
        if (event_code <= 0x103F) return "�쳣���";
    } else if (event_code >= 0x3000 && event_code <= 0x3FFF) {
        if (event_code <= 0x300F) return "ϵͳ��������";
    } else if (event_code >= 0x4000 && event_code <= 0x4FFF) {
        if (event_code <= 0x403F) return "ϵͳ����";
    }
    
    return "δ֪�¼�";
}

/**
  * @brief  ����ʼ����־ϵͳ
  * @param  None
  * @retval LogSystemStatus_t ����״̬
  */
LogSystemStatus_t LogSystem_DeInit(void)
{
    /* ��¼ϵͳֹͣ��־ */
    LogSystem_Record(LOG_CATEGORY_SYSTEM, 0, LOG_EVENT_SYSTEM_STOP, "ͳһ�洢��־ϵͳֹͣ");
    
    /* �ȴ�����д����� */
    SmartDelay(100);
    
    /* ����ʼ��Flash */
    W25Q128_DeInit();
    
    /* �����־ */
    log_manager.is_initialized = 0;
    
    return LOG_SYSTEM_OK;
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */ 


