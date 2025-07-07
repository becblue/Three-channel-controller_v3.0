/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : w25q128_driver.h
  * @brief          : W25Q128JVSIQ SPI Flash����ͷ�ļ�
  ******************************************************************************
  * @attention
  *
  * ����Ϊ��ͨ���л������ϵͳ��Ƶ�W25Q128 SPI Flash����
  * ֧�ֻ����Ķ�д����������������־�洢����
  * 
  * Ӳ�����˵����
 * - CS�������ӵ�PB12��ʹ����ͨGPIO�ֶ�����CS�ź�
 * - ʹ��SPI_NSS_SOFTģʽ��ͨ�������ȷ����CSʱ��
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __W25Q128_DRIVER_H
#define __W25Q128_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "smart_delay.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* W25Q128 Flash��Ϣ�ṹ�� */
typedef struct {
    uint8_t  manufacturer_id;       // ������ID (0xEF)
    uint8_t  memory_type;           // �洢������ (0x40)
    uint8_t  capacity;              // ����ID (0x18 = 128Mbit)
    uint16_t device_id;             // �豸ID (0x4018)
    uint64_t unique_id;             // ΨһID
    uint32_t total_size;            // ���������ֽڣ�
    uint32_t page_size;             // ҳ��С���ֽڣ�
    uint32_t sector_size;           // ������С���ֽڣ�
    uint32_t block_size;            // ���С���ֽڣ�
} W25Q128_Info_t;

/* Flash����״̬ */
typedef enum {
    W25Q128_OK = 0,                 // �����ɹ�
    W25Q128_ERROR,                  // ����ʧ��
    W25Q128_BUSY,                   // �豸æµ
    W25Q128_TIMEOUT,                // ������ʱ
    W25Q128_PROTECTED               // д����
} W25Q128_Status_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* W25Q128 Flash�������� */
#define W25Q128_FLASH_SIZE          0x1000000   // 16MB (128Mbit)
#define W25Q128_SECTOR_SIZE         0x1000      // 4KB
#define W25Q128_PAGE_SIZE           0x100       // 256 bytes
#define W25Q128_BLOCK_SIZE          0x10000     // 64KB

/* W25Q128 Flash����� */
#define W25Q128_CMD_WRITE_ENABLE    0x06        // дʹ��
#define W25Q128_CMD_WRITE_DISABLE   0x04        // д��ֹ
#define W25Q128_CMD_READ_STATUS1    0x05        // ��״̬�Ĵ���1
#define W25Q128_CMD_READ_STATUS2    0x35        // ��״̬�Ĵ���2
#define W25Q128_CMD_READ_STATUS3    0x15        // ��״̬�Ĵ���3
#define W25Q128_CMD_WRITE_STATUS1   0x01        // д״̬�Ĵ���1
#define W25Q128_CMD_READ_DATA       0x03        // ������
#define W25Q128_CMD_FAST_READ       0x0B        // ���ٶ�
#define W25Q128_CMD_PAGE_PROGRAM    0x02        // ҳ���
#define W25Q128_CMD_SECTOR_ERASE    0x20        // ����������4KB��
#define W25Q128_CMD_BLOCK_ERASE     0xD8        // �������64KB��
#define W25Q128_CMD_CHIP_ERASE      0xC7        // оƬ����
#define W25Q128_CMD_POWER_DOWN      0xB9        // ����ģʽ
#define W25Q128_CMD_RELEASE_POWER   0xAB        // �ͷŵ���ģʽ
#define W25Q128_CMD_DEVICE_ID       0x90        // ���豸ID
#define W25Q128_CMD_JEDEC_ID        0x9F        // ��JEDEC ID
#define W25Q128_CMD_UNIQUE_ID       0x4B        // ��ΨһID

/* ״̬�Ĵ���λ���� */
#define W25Q128_STATUS_BUSY         0x01        // æµλ
#define W25Q128_STATUS_WEL          0x02        // дʹ������λ
#define W25Q128_STATUS_BP0          0x04        // �鱣��λ0
#define W25Q128_STATUS_BP1          0x08        // �鱣��λ1
#define W25Q128_STATUS_BP2          0x10        // �鱣��λ2
#define W25Q128_STATUS_TB           0x20        // ����/�ײ�����λ
#define W25Q128_STATUS_SEC          0x40        // ��������λ
#define W25Q128_STATUS_SRP0         0x80        // ״̬�Ĵ�������λ0

/* CS���Ŷ��� - ʹ��PB12��Ϊ��ͨGPIO���ֶ�����CS�ź� */
#define W25Q128_CS_PIN              GPIO_PIN_12     // PB12 - �ֶ�CS����
#define W25Q128_CS_GPIO_PORT        GPIOB           // GPIOB  
#define W25Q128_CS_LOW()            HAL_GPIO_WritePin(W25Q128_CS_GPIO_PORT, W25Q128_CS_PIN, GPIO_PIN_RESET)
#define W25Q128_CS_HIGH()           HAL_GPIO_WritePin(W25Q128_CS_GPIO_PORT, W25Q128_CS_PIN, GPIO_PIN_SET)

/* ��ʱ���� */
#define W25Q128_TIMEOUT_MS          5000        // ������ʱʱ�䣨���룩
#define W25Q128_SPI_TIMEOUT         1000        // SPIͨ�ų�ʱʱ��

/* CS�ӵ�ģʽ�������� */
#define W25Q128_CMD_ENABLE_RESET    0x66        // ʹ�ܸ�λ
#define W25Q128_CMD_RESET_DEVICE    0x99        // ��λ�豸

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* ��ʼ�������ú��� */
W25Q128_Status_t W25Q128_Init(void);
W25Q128_Status_t W25Q128_DeInit(void);
W25Q128_Status_t W25Q128_ReadInfo(W25Q128_Info_t *info);
W25Q128_Status_t W25Q128_Reset(void);

/* ������������ */
W25Q128_Status_t W25Q128_ReadData(uint32_t address, uint8_t *data, uint32_t size);
W25Q128_Status_t W25Q128_WriteData(uint32_t address, uint8_t *data, uint32_t size);
W25Q128_Status_t W25Q128_EraseSector(uint32_t address);
W25Q128_Status_t W25Q128_EraseBlock(uint32_t address);
W25Q128_Status_t W25Q128_EraseChip(void);

/* ״̬��麯�� */
W25Q128_Status_t W25Q128_IsBusy(void);
W25Q128_Status_t W25Q128_WaitForReady(void);
uint8_t W25Q128_ReadStatusRegister(uint8_t register_num);
W25Q128_Status_t W25Q128_WriteStatusRegister(uint8_t register_num, uint8_t value);

/* д�������� */
W25Q128_Status_t W25Q128_WriteEnable(void);
W25Q128_Status_t W25Q128_WriteDisable(void);
W25Q128_Status_t W25Q128_SetProtection(uint8_t protection_level);

/* ��Դ������ */
W25Q128_Status_t W25Q128_PowerDown(void);
W25Q128_Status_t W25Q128_PowerUp(void);

/* ���ߺ��� */
uint32_t W25Q128_GetSectorAddress(uint32_t address);
uint32_t W25Q128_GetBlockAddress(uint32_t address);
uint8_t W25Q128_IsAddressValid(uint32_t address);

/* ���ԺͲ��Ժ��� */
W25Q128_Status_t W25Q128_TestConnection(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __W25Q128_DRIVER_H */ 

