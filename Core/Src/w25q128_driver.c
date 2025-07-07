/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : w25q128_driver.c
  * @brief          : W25Q128JVSIQ SPI Flash����ʵ���ļ�
  ******************************************************************************
  * @attention
  *
  * ����Ϊ��ͨ���л������ϵͳ��Ƶ�W25Q128 SPI Flash����ʵ��
  * ֧�ֻ����Ķ�д����������������־�洢����
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "w25q128_driver.h"
#include "usart.h"

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

/* �ⲿSPI��� */
extern SPI_HandleTypeDef hspi2;

/* Flash�豸��Ϣ */
static W25Q128_Info_t flash_info = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* �ڲ��������� */
static W25Q128_Status_t W25Q128_SPI_Transmit(uint8_t *data, uint16_t size);
static W25Q128_Status_t W25Q128_SPI_Receive(uint8_t *data, uint16_t size);
/* static W25Q128_Status_t W25Q128_SPI_TransmitReceive(uint8_t *tx_data, uint8_t *rx_data, uint16_t size); */
static void W25Q128_CS_Init(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  ��ʼ��W25Q128 Flash
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_Init(void)
{
    DEBUG_Printf("? ��ʼ��ʼ��W25Q128 Flash...\r\n");
    DEBUG_Printf("? SPI������Ϣ��飺\r\n");
    DEBUG_Printf("   - SPIʵ��: SPI2\r\n");
    DEBUG_Printf("   - SPIģʽ: Mode 0 (CPOL=0, CPHA=0) - ����W25Q128Ҫ��\r\n");
    DEBUG_Printf("   - NSS����: ������� (SPI_NSS_SOFT)\r\n");
    DEBUG_Printf("   - ʱ�ӷ�Ƶ: 16 (Լ2.25MHz) - �������133MHzҪ��\r\n");
    DEBUG_Printf("   - CS����: PB12 (GPIO�ֶ�����)\r\n");
    
    /* ��ʼ��CS���� */
    W25Q128_CS_Init();
    
    /* ����CSΪ�ߵ�ƽ���ͷ�FlashоƬ */
    W25Q128_CS_HIGH();
    DEBUG_Printf("CS���ų�ʼ����ɣ�Flash�����ͷ�״̬\r\n");
    
    /* �ȴ��豸�ȶ� - �����ĵ�tPUW��tVSLʱ��Ҫ�� */
    SmartDelayWithForceFeed(1000);  // 1��ȷ��VCC�ȶ���tPUW��ʱ���
    DEBUG_Printf("�豸�ϵ��ȶ���ʱ��� (tPUW + tVSL)\r\n");
    
    /* ������λоƬ */
    uint8_t     reset_cmd = 0x66;  // Enable Reset
    W25Q128_CS_LOW();
    W25Q128_SPI_Transmit(&reset_cmd, 1);
    W25Q128_CS_HIGH();
    SmartDelayWithForceFeed(50);  // ���ӵ�50ms
    
    reset_cmd = 0x99;  // Reset Device
    W25Q128_CS_LOW();
    W25Q128_SPI_Transmit(&reset_cmd, 1);
    W25Q128_CS_HIGH();
    SmartDelayWithForceFeed(300);  // ���ӵ�300ms�ȴ���λ���
    DEBUG_Printf("оƬ��λ���\r\n");
    
    /* �ͷŵ���ģʽ�����֮ǰ������� */
    if (W25Q128_PowerUp() != W25Q128_OK) {
        DEBUG_Printf("W25Q128��ʼ��ʧ�ܣ��޷��ͷŵ���ģʽ\r\n");
        return W25Q128_ERROR;
    }
    DEBUG_Printf("����ģʽ�ͷ����\r\n");
    
    /* ��ȡ�豸��Ϣ - ֧�����Ի��� */
    W25Q128_Status_t read_result = W25Q128_ERROR;
    for (int retry = 0; retry < 3; retry++) {
        SmartDelayWithForceFeed(50);  // ÿ������ǰ��ʱ
        read_result = W25Q128_ReadInfo(&flash_info);
        if (read_result == W25Q128_OK) {
            break;
        }
        DEBUG_Printf("�豸��Ϣ��ȡ���� %d/3\r\n", retry + 1);
    }
    
    if (read_result != W25Q128_OK) {
        DEBUG_Printf("W25Q128��ʼ��ʧ�ܣ��޷���ȡ�豸��Ϣ\r\n");
        DEBUG_Printf("����ԭ��\r\n");
        DEBUG_Printf("1. SPIʱ��Ƶ������\r\n");
        DEBUG_Printf("2. CS������������\r\n");
        DEBUG_Printf("3. оƬ��������\r\n");
        DEBUG_Printf("4. оƬ��������\r\n");
        return W25Q128_ERROR;
    }
    DEBUG_Printf("�豸��Ϣ��ȡ�ɹ�\r\n");
    
    /* ��֤�豸ID */
    DEBUG_Printf("��ȡ�����豸��Ϣ��\r\n");
    DEBUG_Printf("- ������ID: 0x%02X (����: 0xEF)\r\n", flash_info.manufacturer_id);
    DEBUG_Printf("- �豸ID: 0x%04X (����: 0x4018)\r\n", flash_info.device_id);
    DEBUG_Printf("- �ڴ�����: 0x%02X\r\n", flash_info.memory_type);
    DEBUG_Printf("- ����: 0x%02X\r\n", flash_info.capacity);
    
    if (flash_info.manufacturer_id != 0xEF || flash_info.device_id != 0x4018) {
        DEBUG_Printf("W25Q128��ʼ��ʧ�ܣ��豸ID��ƥ��\r\n");
        if (flash_info.manufacturer_id == 0xFF && flash_info.device_id == 0xFFFF) {
            DEBUG_Printf("�����������ȡ��ȫ0xFF����������������\r\n");
        } else if (flash_info.manufacturer_id == 0x00 && flash_info.device_id == 0x0000) {
            DEBUG_Printf("�����������ȡ��ȫ0x00�������ǹ�������\r\n");
        } else {
            DEBUG_Printf("�����������ȡ������оƬ������оƬ�ͺ�\r\n");
        }
        return W25Q128_ERROR;
    }
    
    /* �ȴ��豸׼����� */
    if (W25Q128_WaitForReady() != W25Q128_OK) {
        DEBUG_Printf("W25Q128��ʼ��ʧ�ܣ��豸δ׼������\r\n");
        return W25Q128_ERROR;
    }
    DEBUG_Printf("�豸׼���������ͨ��\r\n");
    
    DEBUG_Printf("W25Q128��ʼ���ɹ� (MID:0x%02X, DID:0x%04X, ����:%dMB)\r\n", 
                 flash_info.manufacturer_id, flash_info.device_id, 
                 flash_info.total_size / (1024 * 1024));
    
    return W25Q128_OK;
}

/**
  * @brief  ����ʼ��W25Q128 Flash
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_DeInit(void)
{
    /* �������ģʽ�Խ�ʡ���� */
    W25Q128_PowerDown();
    
    /* ����CSΪ�ߵ�ƽ */
    W25Q128_CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  ��ȡFlash�豸��Ϣ
  * @param  info: �洢�豸��Ϣ�Ľṹ��ָ��
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_ReadInfo(W25Q128_Info_t *info)
{
    uint8_t cmd_buf[4];
    uint8_t rx_buf[8];
    
    if (info == NULL) {
        return W25Q128_ERROR;
    }
    
    /* ��ȡ״̬�Ĵ���1������� */
    uint8_t status_reg1 = 0;
    cmd_buf[0] = 0x05;  // Read Status Register-1 ����
    W25Q128_CS_LOW();
    if (HAL_SPI_TransmitReceive(&hspi2, cmd_buf, rx_buf, 2, W25Q128_SPI_TIMEOUT) == HAL_OK) {
        status_reg1 = rx_buf[1];
        DEBUG_Printf("״̬�Ĵ���1: 0x%02X (WEL=%d, WIP=%d, BP=%d%d%d)\r\n", 
                    status_reg1, (status_reg1>>1)&1, status_reg1&1, 
                    (status_reg1>>4)&1, (status_reg1>>3)&1, (status_reg1>>2)&1);
    } else {
        DEBUG_Printf("״̬�Ĵ���1��ȡʧ��\r\n");
    }
    W25Q128_CS_HIGH();
    
    /* ��ȡJEDEC ID - ʹ����������ȷ��ʱ����ȷ */
    cmd_buf[0] = W25Q128_CMD_JEDEC_ID;
    cmd_buf[1] = 0x00;  // �����ֽ�
    cmd_buf[2] = 0x00;  // �����ֽ�  
    cmd_buf[3] = 0x00;  // �����ֽ�
    
    W25Q128_CS_LOW();
    if (HAL_SPI_TransmitReceive(&hspi2, cmd_buf, rx_buf, 4, W25Q128_SPI_TIMEOUT) != HAL_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    W25Q128_CS_HIGH();
    
    /* ������һ���ֽڣ������������ʹ�ú���3�ֽ���ΪJEDEC ID */
    rx_buf[0] = rx_buf[1];  // ������ID
    rx_buf[1] = rx_buf[2];  // �ڴ�����
    rx_buf[2] = rx_buf[3];  // ����
    
    DEBUG_Printf("JEDEC ID: 0x%02X 0x%02X 0x%02X (������: 0x%02X, ����: 0x%02X, ����: 0x%02X)\r\n", 
                rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[0], rx_buf[1], rx_buf[2]);
    
    /* ��ϣ���������ID��ȡ�������жԱ� */
    DEBUG_Printf("? ��������ID��ȡ��������ȫ�����...\r\n");
    
    /* ����2: ��ȡ�豸ID (90hָ��) */
    uint8_t alt_cmd[6] = {0x90, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t alt_rx[6];
    
    W25Q128_CS_LOW();
    if (HAL_SPI_TransmitReceive(&hspi2, alt_cmd, alt_rx, 6, W25Q128_SPI_TIMEOUT) == HAL_OK) {
        DEBUG_Printf("�豸ID(90h): ������=0x%02X, �豸=0x%02X\r\n", alt_rx[4], alt_rx[5]);
    } else {
        DEBUG_Printf("�豸ID(90h)��ȡʧ��\r\n");
    }
    W25Q128_CS_HIGH();
    
    /* ����3: ��ȡΨһID (4Bhָ��) */
    alt_cmd[0] = 0x4B;  // Read Unique ID 
    W25Q128_CS_LOW();
    if (HAL_SPI_TransmitReceive(&hspi2, alt_cmd, alt_rx, 6, W25Q128_SPI_TIMEOUT) == HAL_OK) {
        DEBUG_Printf("ΨһID(4Bh)ǰ2�ֽ�: 0x%02X 0x%02X\r\n", alt_rx[4], alt_rx[5]);
    } else {
        DEBUG_Printf("ΨһID(4Bh)��ȡʧ��\r\n");
    }
    W25Q128_CS_HIGH();
    
    /* ��ϸ��JEDEC ID���� */
    if (rx_buf[0] == 0x00 && rx_buf[1] == 0x00 && rx_buf[2] == 0x00) {
        DEBUG_Printf("? ��ȡ��ȫ0���ݣ�����ԭ��\r\n");
        DEBUG_Printf("   - SPIͨ�����⣨ʱ�ӡ������ߣ�\r\n");
        DEBUG_Printf("   - CS�źſ�������\r\n");
        DEBUG_Printf("   - оƬδ�ϵ��λ��\r\n");
        return W25Q128_ERROR;
    } else if (rx_buf[0] == 0xFF && rx_buf[1] == 0xFF && rx_buf[2] == 0xFF) {
        DEBUG_Printf("? ��ȡ��ȫFF���ݣ�����ԭ��\r\n");
        DEBUG_Printf("   - оƬδ���ӻ����Ӳ���\r\n");
        DEBUG_Printf("   - SPI�������ô���\r\n");
        DEBUG_Printf("   - оƬ��\r\n");
        return W25Q128_ERROR;
    } else if (rx_buf[0] == 0xEF && rx_buf[1] == 0x40 && rx_buf[2] == 0x18) {
        DEBUG_Printf("? JEDEC ID��֤ͨ����W25Q128JVоƬʶ��ɹ���\r\n");
    } else {
        DEBUG_Printf("??  JEDEC ID��ƥ�䣬Ԥ��: EF 40 18, ʵ��: %02X %02X %02X\r\n", 
                    rx_buf[0], rx_buf[1], rx_buf[2]);
        DEBUG_Printf("   �����������ͺŵ�FlashоƬ\r\n");
    }
    
    /* �����豸��Ϣ */
    info->manufacturer_id = rx_buf[0];
    info->memory_type = rx_buf[1];
    info->capacity = rx_buf[2];
    info->device_id = (rx_buf[1] << 8) | rx_buf[2];
    
    /* ����������Ϣ */
    info->total_size = W25Q128_FLASH_SIZE;
    info->page_size = W25Q128_PAGE_SIZE;
    info->sector_size = W25Q128_SECTOR_SIZE;
    info->block_size = W25Q128_BLOCK_SIZE;
    
    return W25Q128_OK;
}

/**
  * @brief  ��ȡFlash����
  * @param  address: ��ȡ��ַ
  * @param  data: ���ݻ�����ָ��
  * @param  size: ��ȡ��С
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_ReadData(uint32_t address, uint8_t *data, uint32_t size)
{
    uint8_t cmd_buf[4];
    
    if (data == NULL || size == 0) {
        return W25Q128_ERROR;
    }
    
    /* ��ַ��Ч�Լ�� */
    if (!W25Q128_IsAddressValid(address) || !W25Q128_IsAddressValid(address + size - 1)) {
        return W25Q128_ERROR;
    }
    
    /* �ȴ��豸׼����� */
    if (W25Q128_WaitForReady() != W25Q128_OK) {
        return W25Q128_TIMEOUT;
    }
    
    /* ׼����ȡ���� */
    cmd_buf[0] = W25Q128_CMD_READ_DATA;
    cmd_buf[1] = (address >> 16) & 0xFF;
    cmd_buf[2] = (address >> 8) & 0xFF;
    cmd_buf[3] = address & 0xFF;
    
    W25Q128_CS_LOW();
    if (W25Q128_SPI_Transmit(cmd_buf, 4) != W25Q128_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    
    if (W25Q128_SPI_Receive(data, size) != W25Q128_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    W25Q128_CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  д��Flash����
  * @param  address: д���ַ
  * @param  data: ���ݻ�����ָ��
  * @param  size: д���С
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_WriteData(uint32_t address, uint8_t *data, uint32_t size)
{
    uint8_t cmd_buf[4];
    uint32_t write_size;
    uint32_t page_offset;
    
    if (data == NULL || size == 0) {
        return W25Q128_ERROR;
    }
    
    /* ��ַ��Ч�Լ�� */
    if (!W25Q128_IsAddressValid(address) || !W25Q128_IsAddressValid(address + size - 1)) {
        return W25Q128_ERROR;
    }
    
    while (size > 0) {
        /* ����ҳ��ƫ�� */
        page_offset = address % W25Q128_PAGE_SIZE;
        
        /* ���㱾��д���С�����ܿ�ҳ�� */
        write_size = W25Q128_PAGE_SIZE - page_offset;
        if (write_size > size) {
            write_size = size;
        }
        
        /* �ȴ��豸׼����� */
        if (W25Q128_WaitForReady() != W25Q128_OK) {
            return W25Q128_TIMEOUT;
        }
        
        /* ʹ��д�� */
        if (W25Q128_WriteEnable() != W25Q128_OK) {
            return W25Q128_ERROR;
        }
        
        /* ׼��ҳ������� */
        cmd_buf[0] = W25Q128_CMD_PAGE_PROGRAM;
        cmd_buf[1] = (address >> 16) & 0xFF;
        cmd_buf[2] = (address >> 8) & 0xFF;
        cmd_buf[3] = address & 0xFF;
        
        W25Q128_CS_LOW();
        if (W25Q128_SPI_Transmit(cmd_buf, 4) != W25Q128_OK) {
            W25Q128_CS_HIGH();
            return W25Q128_ERROR;
        }
        
        if (W25Q128_SPI_Transmit(data, write_size) != W25Q128_OK) {
            W25Q128_CS_HIGH();
            return W25Q128_ERROR;
        }
        W25Q128_CS_HIGH();
        
        /* �ȴ�д����� */
        if (W25Q128_WaitForReady() != W25Q128_OK) {
            return W25Q128_TIMEOUT;
        }
        
        /* ���µ�ַ�ʹ�С */
        address += write_size;
        data += write_size;
        size -= write_size;
    }
    
    return W25Q128_OK;
}

/**
  * @brief  ����Flash����
  * @param  address: ������ַ
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_EraseSector(uint32_t address)
{
    uint8_t cmd_buf[4];
    
    /* ��ַ��Ч�Լ�� */
    if (!W25Q128_IsAddressValid(address)) {
        return W25Q128_ERROR;
    }
    
    /* �ȴ��豸׼����� */
    if (W25Q128_WaitForReady() != W25Q128_OK) {
        return W25Q128_TIMEOUT;
    }
    
    /* ʹ��д�� */
    if (W25Q128_WriteEnable() != W25Q128_OK) {
        return W25Q128_ERROR;
    }
    
    /* ׼�������������� */
    cmd_buf[0] = W25Q128_CMD_SECTOR_ERASE;
    cmd_buf[1] = (address >> 16) & 0xFF;
    cmd_buf[2] = (address >> 8) & 0xFF;
    cmd_buf[3] = address & 0xFF;
    
    W25Q128_CS_LOW();
    if (W25Q128_SPI_Transmit(cmd_buf, 4) != W25Q128_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    W25Q128_CS_HIGH();
    
    /* �ȴ�������� */
    if (W25Q128_WaitForReady() != W25Q128_OK) {
        return W25Q128_TIMEOUT;
    }
    
    return W25Q128_OK;
}

/**
  * @brief  ���Flash�Ƿ�æµ
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_IsBusy(void)
{
    uint8_t status = W25Q128_ReadStatusRegister(1);
    return (status & W25Q128_STATUS_BUSY) ? W25Q128_BUSY : W25Q128_OK;
}

/**
  * @brief  �ȴ�Flash׼�����
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_WaitForReady(void)
{
    uint32_t timeout = HAL_GetTick() + W25Q128_TIMEOUT_MS;
    
    while (HAL_GetTick() < timeout) {
        if (W25Q128_IsBusy() == W25Q128_OK) {
            return W25Q128_OK;
        }
        /* ʹ��������ʱ�����⿴�Ź���λ */
        SmartDelay(10);
    }
    
    return W25Q128_TIMEOUT;
}

/**
  * @brief  ��ȡ״̬�Ĵ���
  * @param  register_num: �Ĵ������ (1-3)
  * @retval uint8_t ״̬�Ĵ���ֵ
  */
uint8_t W25Q128_ReadStatusRegister(uint8_t register_num)
{
    uint8_t cmd, status = 0;
    
    switch (register_num) {
        case 1:
            cmd = W25Q128_CMD_READ_STATUS1;
            break;
        case 2:
            cmd = W25Q128_CMD_READ_STATUS2;
            break;
        case 3:
            cmd = W25Q128_CMD_READ_STATUS3;
            break;
        default:
            return 0;
    }
    
    W25Q128_CS_LOW();
    W25Q128_SPI_Transmit(&cmd, 1);
    W25Q128_SPI_Receive(&status, 1);
    W25Q128_CS_HIGH();
    
    return status;
}

/**
  * @brief  ʹ��д��
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_WriteEnable(void)
{
    uint8_t cmd = W25Q128_CMD_WRITE_ENABLE;
    
    W25Q128_CS_LOW();
    if (W25Q128_SPI_Transmit(&cmd, 1) != W25Q128_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    W25Q128_CS_HIGH();
    
    /* ��֤дʹ��λ */
    uint8_t status = W25Q128_ReadStatusRegister(1);
    if (!(status & W25Q128_STATUS_WEL)) {
        return W25Q128_ERROR;
    }
    
    return W25Q128_OK;
}

/**
  * @brief  ��ֹд��
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_WriteDisable(void)
{
    uint8_t cmd = W25Q128_CMD_WRITE_DISABLE;
    
    W25Q128_CS_LOW();
    if (W25Q128_SPI_Transmit(&cmd, 1) != W25Q128_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    W25Q128_CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  �������ģʽ
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_PowerDown(void)
{
    uint8_t cmd = W25Q128_CMD_POWER_DOWN;
    
    W25Q128_CS_LOW();
    if (W25Q128_SPI_Transmit(&cmd, 1) != W25Q128_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    W25Q128_CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  �ͷŵ���ģʽ
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_PowerUp(void)
{
    uint8_t cmd = W25Q128_CMD_RELEASE_POWER;
    
    W25Q128_CS_LOW();
    if (W25Q128_SPI_Transmit(&cmd, 1) != W25Q128_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    W25Q128_CS_HIGH();
    
    /* �ȴ��豸���� */
    SmartDelayWithForceFeed(100);
    
    return W25Q128_OK;
}

/**
  * @brief  ��ȡ���������ַ
  * @param  address: ԭʼ��ַ
  * @retval uint32_t ���������ַ
  */
uint32_t W25Q128_GetSectorAddress(uint32_t address)
{
    return address & ~(W25Q128_SECTOR_SIZE - 1);
}

/**
  * @brief  ��ȡ������ַ
  * @param  address: ԭʼ��ַ
  * @retval uint32_t ������ַ
  */
uint32_t W25Q128_GetBlockAddress(uint32_t address)
{
    return address & ~(W25Q128_BLOCK_SIZE - 1);
}

/**
  * @brief  ����ַ��Ч��
  * @param  address: ����ַ
  * @retval uint8_t 1-��Ч��0-��Ч
  */
uint8_t W25Q128_IsAddressValid(uint32_t address)
{
    return (address < W25Q128_FLASH_SIZE) ? 1 : 0;
}

/**
  * @brief  ����Flash����
  * @param  None
  * @retval W25Q128_Status_t ����״̬
  */
W25Q128_Status_t W25Q128_TestConnection(void)
{
    W25Q128_Info_t test_info;
    
    DEBUG_Printf("��ʼ����W25Q128����...\r\n");
    DEBUG_Printf("ʹ��PB12��ΪCS���ţ�ͨ��GPIO�ֶ�����\r\n");
    
    /* ��ʼ��CS���� */
    W25Q128_CS_Init();
    
    /* ���Զ�ȡ�豸��Ϣ */
    if (W25Q128_ReadInfo(&test_info) != W25Q128_OK) {
        DEBUG_Printf("Flash���Ӳ���ʧ�ܣ��޷���ȡ�豸��Ϣ\r\n");
        return W25Q128_ERROR;
    }
    
    DEBUG_Printf("Flash���Ӳ��Գɹ�\r\n");
    DEBUG_Printf("��ȡ�����豸��Ϣ��\r\n");
    DEBUG_Printf("- ������ID: 0x%02X\r\n", test_info.manufacturer_id);
    DEBUG_Printf("- �豸ID: 0x%04X\r\n", test_info.device_id);
    DEBUG_Printf("- �ڴ�����: 0x%02X\r\n", test_info.memory_type);
    DEBUG_Printf("- ����: 0x%02X\r\n", test_info.capacity);
    
    /* ����Ƿ�ΪW25Q128 */
    if (test_info.manufacturer_id == 0xEF && test_info.device_id == 0x4018) {
        DEBUG_Printf("ȷ��ΪW25Q128оƬ\r\n");
        return W25Q128_OK;
    } else {
        DEBUG_Printf("���棺��W25Q128оƬ\r\n");
        return W25Q128_ERROR;
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  SPI��������
  * @param  data: ��������ָ��
  * @param  size: ���ʹ�С
  * @retval W25Q128_Status_t ����״̬
  */
static W25Q128_Status_t W25Q128_SPI_Transmit(uint8_t *data, uint16_t size)
{
    if (HAL_SPI_Transmit(&hspi2, data, size, W25Q128_SPI_TIMEOUT) != HAL_OK) {
        return W25Q128_ERROR;
    }
    return W25Q128_OK;
}

/**
  * @brief  SPI��������
  * @param  data: ��������ָ��
  * @param  size: ���մ�С
  * @retval W25Q128_Status_t ����״̬
  */
static W25Q128_Status_t W25Q128_SPI_Receive(uint8_t *data, uint16_t size)
{
    if (HAL_SPI_Receive(&hspi2, data, size, W25Q128_SPI_TIMEOUT) != HAL_OK) {
        return W25Q128_ERROR;
    }
    return W25Q128_OK;
}

/**
  * @brief  ��ʼ��CS����
  * @param  None
  * @retval None
  */
static void W25Q128_CS_Init(void)
{
    /* ����PB12ΪGPIO���ģʽ�������ֶ�����CS�ź� */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* ʹ��GPIOBʱ�� */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* ����PB12Ϊ������� */
    GPIO_InitStruct.Pin = W25Q128_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(W25Q128_CS_GPIO_PORT, &GPIO_InitStruct);
    
    /* ��ʼ��CSΪ�ߵ�ƽ��оƬδѡ��״̬�� */
    W25Q128_CS_HIGH();
    
    DEBUG_Printf("CS��������ΪGPIO���ģʽ (PB12-�ֶ�����)\r\n");
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */ 


