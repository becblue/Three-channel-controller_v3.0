/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : w25q128_driver.c
  * @brief          : W25Q128JVSIQ SPI Flash驱动实现文件
  ******************************************************************************
  * @attention
  *
  * 这是为三通道切换箱控制系统设计的W25Q128 SPI Flash驱动实现
  * 支持基础的读写擦除操作，用于日志存储功能
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

/* 外部SPI句柄 */
extern SPI_HandleTypeDef hspi2;

/* Flash设备信息 */
static W25Q128_Info_t flash_info = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* 内部辅助函数 */
static W25Q128_Status_t W25Q128_SPI_Transmit(uint8_t *data, uint16_t size);
static W25Q128_Status_t W25Q128_SPI_Receive(uint8_t *data, uint16_t size);
/* static W25Q128_Status_t W25Q128_SPI_TransmitReceive(uint8_t *tx_data, uint8_t *rx_data, uint16_t size); */
static void W25Q128_CS_Init(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  初始化W25Q128 Flash
  * @param  None
  * @retval W25Q128_Status_t 操作状态
  */
W25Q128_Status_t W25Q128_Init(void)
{
    DEBUG_Printf("? 开始初始化W25Q128 Flash...\r\n");
    DEBUG_Printf("? SPI配置信息检查：\r\n");
    DEBUG_Printf("   - SPI实例: SPI2\r\n");
    DEBUG_Printf("   - SPI模式: Mode 0 (CPOL=0, CPHA=0) - 符合W25Q128要求\r\n");
    DEBUG_Printf("   - NSS控制: 软件控制 (SPI_NSS_SOFT)\r\n");
    DEBUG_Printf("   - 时钟分频: 16 (约2.25MHz) - 符合最大133MHz要求\r\n");
    DEBUG_Printf("   - CS引脚: PB12 (GPIO手动控制)\r\n");
    
    /* 初始化CS引脚 */
    W25Q128_CS_Init();
    
    /* 设置CS为高电平，释放Flash芯片 */
    W25Q128_CS_HIGH();
    DEBUG_Printf("CS引脚初始化完成，Flash处于释放状态\r\n");
    
    /* 等待设备稳定 - 符合文档tPUW和tVSL时序要求 */
    SmartDelayWithForceFeed(1000);  // 1秒确保VCC稳定和tPUW延时完成
    DEBUG_Printf("设备上电稳定延时完成 (tPUW + tVSL)\r\n");
    
    /* 尝试软复位芯片 */
    uint8_t     reset_cmd = 0x66;  // Enable Reset
    W25Q128_CS_LOW();
    W25Q128_SPI_Transmit(&reset_cmd, 1);
    W25Q128_CS_HIGH();
    SmartDelayWithForceFeed(50);  // 增加到50ms
    
    reset_cmd = 0x99;  // Reset Device
    W25Q128_CS_LOW();
    W25Q128_SPI_Transmit(&reset_cmd, 1);
    W25Q128_CS_HIGH();
    SmartDelayWithForceFeed(300);  // 增加到300ms等待复位完成
    DEBUG_Printf("芯片软复位完成\r\n");
    
    /* 释放掉电模式（如果之前进入过） */
    if (W25Q128_PowerUp() != W25Q128_OK) {
        DEBUG_Printf("W25Q128初始化失败：无法释放掉电模式\r\n");
        return W25Q128_ERROR;
    }
    DEBUG_Printf("掉电模式释放完成\r\n");
    
    /* 读取设备信息 - 支持重试机制 */
    W25Q128_Status_t read_result = W25Q128_ERROR;
    for (int retry = 0; retry < 3; retry++) {
        SmartDelayWithForceFeed(50);  // 每次重试前延时
        read_result = W25Q128_ReadInfo(&flash_info);
        if (read_result == W25Q128_OK) {
            break;
        }
        DEBUG_Printf("设备信息读取重试 %d/3\r\n", retry + 1);
    }
    
    if (read_result != W25Q128_OK) {
        DEBUG_Printf("W25Q128初始化失败：无法读取设备信息\r\n");
        DEBUG_Printf("可能原因：\r\n");
        DEBUG_Printf("1. SPI时钟频率问题\r\n");
        DEBUG_Printf("2. CS引脚连接问题\r\n");
        DEBUG_Printf("3. 芯片供电问题\r\n");
        DEBUG_Printf("4. 芯片焊接问题\r\n");
        return W25Q128_ERROR;
    }
    DEBUG_Printf("设备信息读取成功\r\n");
    
    /* 验证设备ID */
    DEBUG_Printf("读取到的设备信息：\r\n");
    DEBUG_Printf("- 制造商ID: 0x%02X (期望: 0xEF)\r\n", flash_info.manufacturer_id);
    DEBUG_Printf("- 设备ID: 0x%04X (期望: 0x4018)\r\n", flash_info.device_id);
    DEBUG_Printf("- 内存类型: 0x%02X\r\n", flash_info.memory_type);
    DEBUG_Printf("- 容量: 0x%02X\r\n", flash_info.capacity);
    
    if (flash_info.manufacturer_id != 0xEF || flash_info.device_id != 0x4018) {
        DEBUG_Printf("W25Q128初始化失败：设备ID不匹配\r\n");
        if (flash_info.manufacturer_id == 0xFF && flash_info.device_id == 0xFFFF) {
            DEBUG_Printf("错误分析：读取到全0xFF，可能是连接问题\r\n");
        } else if (flash_info.manufacturer_id == 0x00 && flash_info.device_id == 0x0000) {
            DEBUG_Printf("错误分析：读取到全0x00，可能是供电问题\r\n");
        } else {
            DEBUG_Printf("错误分析：读取到其他芯片，请检查芯片型号\r\n");
        }
        return W25Q128_ERROR;
    }
    
    /* 等待设备准备完成 */
    if (W25Q128_WaitForReady() != W25Q128_OK) {
        DEBUG_Printf("W25Q128初始化失败：设备未准备就绪\r\n");
        return W25Q128_ERROR;
    }
    DEBUG_Printf("设备准备就绪检查通过\r\n");
    
    DEBUG_Printf("W25Q128初始化成功 (MID:0x%02X, DID:0x%04X, 容量:%dMB)\r\n", 
                 flash_info.manufacturer_id, flash_info.device_id, 
                 flash_info.total_size / (1024 * 1024));
    
    return W25Q128_OK;
}

/**
  * @brief  反初始化W25Q128 Flash
  * @param  None
  * @retval W25Q128_Status_t 操作状态
  */
W25Q128_Status_t W25Q128_DeInit(void)
{
    /* 进入掉电模式以节省功耗 */
    W25Q128_PowerDown();
    
    /* 设置CS为高电平 */
    W25Q128_CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  读取Flash设备信息
  * @param  info: 存储设备信息的结构体指针
  * @retval W25Q128_Status_t 操作状态
  */
W25Q128_Status_t W25Q128_ReadInfo(W25Q128_Info_t *info)
{
    uint8_t cmd_buf[4];
    uint8_t rx_buf[8];
    
    if (info == NULL) {
        return W25Q128_ERROR;
    }
    
    /* 读取状态寄存器1进行诊断 */
    uint8_t status_reg1 = 0;
    cmd_buf[0] = 0x05;  // Read Status Register-1 命令
    W25Q128_CS_LOW();
    if (HAL_SPI_TransmitReceive(&hspi2, cmd_buf, rx_buf, 2, W25Q128_SPI_TIMEOUT) == HAL_OK) {
        status_reg1 = rx_buf[1];
        DEBUG_Printf("状态寄存器1: 0x%02X (WEL=%d, WIP=%d, BP=%d%d%d)\r\n", 
                    status_reg1, (status_reg1>>1)&1, status_reg1&1, 
                    (status_reg1>>4)&1, (status_reg1>>3)&1, (status_reg1>>2)&1);
    } else {
        DEBUG_Printf("状态寄存器1读取失败\r\n");
    }
    W25Q128_CS_HIGH();
    
    /* 读取JEDEC ID - 使用连续传输确保时序正确 */
    cmd_buf[0] = W25Q128_CMD_JEDEC_ID;
    cmd_buf[1] = 0x00;  // 虚拟字节
    cmd_buf[2] = 0x00;  // 虚拟字节  
    cmd_buf[3] = 0x00;  // 虚拟字节
    
    W25Q128_CS_LOW();
    if (HAL_SPI_TransmitReceive(&hspi2, cmd_buf, rx_buf, 4, W25Q128_SPI_TIMEOUT) != HAL_OK) {
        W25Q128_CS_HIGH();
        return W25Q128_ERROR;
    }
    W25Q128_CS_HIGH();
    
    /* 跳过第一个字节（命令回声），使用后续3字节作为JEDEC ID */
    rx_buf[0] = rx_buf[1];  // 制造商ID
    rx_buf[1] = rx_buf[2];  // 内存类型
    rx_buf[2] = rx_buf[3];  // 容量
    
    DEBUG_Printf("JEDEC ID: 0x%02X 0x%02X 0x%02X (制造商: 0x%02X, 类型: 0x%02X, 容量: 0x%02X)\r\n", 
                rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[0], rx_buf[1], rx_buf[2]);
    
    /* 诊断：尝试其他ID读取方法进行对比 */
    DEBUG_Printf("? 尝试其他ID读取方法进行全面诊断...\r\n");
    
    /* 方法2: 读取设备ID (90h指令) */
    uint8_t alt_cmd[6] = {0x90, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t alt_rx[6];
    
    W25Q128_CS_LOW();
    if (HAL_SPI_TransmitReceive(&hspi2, alt_cmd, alt_rx, 6, W25Q128_SPI_TIMEOUT) == HAL_OK) {
        DEBUG_Printf("设备ID(90h): 制造商=0x%02X, 设备=0x%02X\r\n", alt_rx[4], alt_rx[5]);
    } else {
        DEBUG_Printf("设备ID(90h)读取失败\r\n");
    }
    W25Q128_CS_HIGH();
    
    /* 方法3: 读取唯一ID (4Bh指令) */
    alt_cmd[0] = 0x4B;  // Read Unique ID 
    W25Q128_CS_LOW();
    if (HAL_SPI_TransmitReceive(&hspi2, alt_cmd, alt_rx, 6, W25Q128_SPI_TIMEOUT) == HAL_OK) {
        DEBUG_Printf("唯一ID(4Bh)前2字节: 0x%02X 0x%02X\r\n", alt_rx[4], alt_rx[5]);
    } else {
        DEBUG_Printf("唯一ID(4Bh)读取失败\r\n");
    }
    W25Q128_CS_HIGH();
    
    /* 详细的JEDEC ID分析 */
    if (rx_buf[0] == 0x00 && rx_buf[1] == 0x00 && rx_buf[2] == 0x00) {
        DEBUG_Printf("? 读取到全0数据，可能原因：\r\n");
        DEBUG_Printf("   - SPI通信问题（时钟、数据线）\r\n");
        DEBUG_Printf("   - CS信号控制问题\r\n");
        DEBUG_Printf("   - 芯片未上电或复位中\r\n");
        return W25Q128_ERROR;
    } else if (rx_buf[0] == 0xFF && rx_buf[1] == 0xFF && rx_buf[2] == 0xFF) {
        DEBUG_Printf("? 读取到全FF数据，可能原因：\r\n");
        DEBUG_Printf("   - 芯片未连接或连接不良\r\n");
        DEBUG_Printf("   - SPI引脚配置错误\r\n");
        DEBUG_Printf("   - 芯片损坏\r\n");
        return W25Q128_ERROR;
    } else if (rx_buf[0] == 0xEF && rx_buf[1] == 0x40 && rx_buf[2] == 0x18) {
        DEBUG_Printf("? JEDEC ID验证通过：W25Q128JV芯片识别成功！\r\n");
    } else {
        DEBUG_Printf("??  JEDEC ID不匹配，预期: EF 40 18, 实际: %02X %02X %02X\r\n", 
                    rx_buf[0], rx_buf[1], rx_buf[2]);
        DEBUG_Printf("   可能是其他型号的Flash芯片\r\n");
    }
    
    /* 解析设备信息 */
    info->manufacturer_id = rx_buf[0];
    info->memory_type = rx_buf[1];
    info->capacity = rx_buf[2];
    info->device_id = (rx_buf[1] << 8) | rx_buf[2];
    
    /* 设置容量信息 */
    info->total_size = W25Q128_FLASH_SIZE;
    info->page_size = W25Q128_PAGE_SIZE;
    info->sector_size = W25Q128_SECTOR_SIZE;
    info->block_size = W25Q128_BLOCK_SIZE;
    
    return W25Q128_OK;
}

/**
  * @brief  读取Flash数据
  * @param  address: 读取地址
  * @param  data: 数据缓冲区指针
  * @param  size: 读取大小
  * @retval W25Q128_Status_t 操作状态
  */
W25Q128_Status_t W25Q128_ReadData(uint32_t address, uint8_t *data, uint32_t size)
{
    uint8_t cmd_buf[4];
    
    if (data == NULL || size == 0) {
        return W25Q128_ERROR;
    }
    
    /* 地址有效性检查 */
    if (!W25Q128_IsAddressValid(address) || !W25Q128_IsAddressValid(address + size - 1)) {
        return W25Q128_ERROR;
    }
    
    /* 等待设备准备完成 */
    if (W25Q128_WaitForReady() != W25Q128_OK) {
        return W25Q128_TIMEOUT;
    }
    
    /* 准备读取命令 */
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
  * @brief  写入Flash数据
  * @param  address: 写入地址
  * @param  data: 数据缓冲区指针
  * @param  size: 写入大小
  * @retval W25Q128_Status_t 操作状态
  */
W25Q128_Status_t W25Q128_WriteData(uint32_t address, uint8_t *data, uint32_t size)
{
    uint8_t cmd_buf[4];
    uint32_t write_size;
    uint32_t page_offset;
    
    if (data == NULL || size == 0) {
        return W25Q128_ERROR;
    }
    
    /* 地址有效性检查 */
    if (!W25Q128_IsAddressValid(address) || !W25Q128_IsAddressValid(address + size - 1)) {
        return W25Q128_ERROR;
    }
    
    while (size > 0) {
        /* 计算页内偏移 */
        page_offset = address % W25Q128_PAGE_SIZE;
        
        /* 计算本次写入大小（不能跨页） */
        write_size = W25Q128_PAGE_SIZE - page_offset;
        if (write_size > size) {
            write_size = size;
        }
        
        /* 等待设备准备完成 */
        if (W25Q128_WaitForReady() != W25Q128_OK) {
            return W25Q128_TIMEOUT;
        }
        
        /* 使能写入 */
        if (W25Q128_WriteEnable() != W25Q128_OK) {
            return W25Q128_ERROR;
        }
        
        /* 准备页编程命令 */
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
        
        /* 等待写入完成 */
        if (W25Q128_WaitForReady() != W25Q128_OK) {
            return W25Q128_TIMEOUT;
        }
        
        /* 更新地址和大小 */
        address += write_size;
        data += write_size;
        size -= write_size;
    }
    
    return W25Q128_OK;
}

/**
  * @brief  擦除Flash扇区
  * @param  address: 扇区地址
  * @retval W25Q128_Status_t 操作状态
  */
W25Q128_Status_t W25Q128_EraseSector(uint32_t address)
{
    uint8_t cmd_buf[4];
    
    /* 地址有效性检查 */
    if (!W25Q128_IsAddressValid(address)) {
        return W25Q128_ERROR;
    }
    
    /* 等待设备准备完成 */
    if (W25Q128_WaitForReady() != W25Q128_OK) {
        return W25Q128_TIMEOUT;
    }
    
    /* 使能写入 */
    if (W25Q128_WriteEnable() != W25Q128_OK) {
        return W25Q128_ERROR;
    }
    
    /* 准备扇区擦除命令 */
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
    
    /* 等待擦除完成 */
    if (W25Q128_WaitForReady() != W25Q128_OK) {
        return W25Q128_TIMEOUT;
    }
    
    return W25Q128_OK;
}

/**
  * @brief  检查Flash是否忙碌
  * @param  None
  * @retval W25Q128_Status_t 操作状态
  */
W25Q128_Status_t W25Q128_IsBusy(void)
{
    uint8_t status = W25Q128_ReadStatusRegister(1);
    return (status & W25Q128_STATUS_BUSY) ? W25Q128_BUSY : W25Q128_OK;
}

/**
  * @brief  等待Flash准备完成
  * @param  None
  * @retval W25Q128_Status_t 操作状态
  */
W25Q128_Status_t W25Q128_WaitForReady(void)
{
    uint32_t timeout = HAL_GetTick() + W25Q128_TIMEOUT_MS;
    
    while (HAL_GetTick() < timeout) {
        if (W25Q128_IsBusy() == W25Q128_OK) {
            return W25Q128_OK;
        }
        /* 使用智能延时，避免看门狗复位 */
        SmartDelay(10);
    }
    
    return W25Q128_TIMEOUT;
}

/**
  * @brief  读取状态寄存器
  * @param  register_num: 寄存器编号 (1-3)
  * @retval uint8_t 状态寄存器值
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
  * @brief  使能写入
  * @param  None
  * @retval W25Q128_Status_t 操作状态
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
    
    /* 验证写使能位 */
    uint8_t status = W25Q128_ReadStatusRegister(1);
    if (!(status & W25Q128_STATUS_WEL)) {
        return W25Q128_ERROR;
    }
    
    return W25Q128_OK;
}

/**
  * @brief  禁止写入
  * @param  None
  * @retval W25Q128_Status_t 操作状态
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
  * @brief  进入掉电模式
  * @param  None
  * @retval W25Q128_Status_t 操作状态
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
  * @brief  释放掉电模式
  * @param  None
  * @retval W25Q128_Status_t 操作状态
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
    
    /* 等待设备唤醒 */
    SmartDelayWithForceFeed(100);
    
    return W25Q128_OK;
}

/**
  * @brief  获取扇区对齐地址
  * @param  address: 原始地址
  * @retval uint32_t 扇区对齐地址
  */
uint32_t W25Q128_GetSectorAddress(uint32_t address)
{
    return address & ~(W25Q128_SECTOR_SIZE - 1);
}

/**
  * @brief  获取块对齐地址
  * @param  address: 原始地址
  * @retval uint32_t 块对齐地址
  */
uint32_t W25Q128_GetBlockAddress(uint32_t address)
{
    return address & ~(W25Q128_BLOCK_SIZE - 1);
}

/**
  * @brief  检查地址有效性
  * @param  address: 检查地址
  * @retval uint8_t 1-有效，0-无效
  */
uint8_t W25Q128_IsAddressValid(uint32_t address)
{
    return (address < W25Q128_FLASH_SIZE) ? 1 : 0;
}

/**
  * @brief  测试Flash连接
  * @param  None
  * @retval W25Q128_Status_t 连接状态
  */
W25Q128_Status_t W25Q128_TestConnection(void)
{
    W25Q128_Info_t test_info;
    
    DEBUG_Printf("开始测试W25Q128连接...\r\n");
    DEBUG_Printf("使用PB12作为CS引脚，通过GPIO手动控制\r\n");
    
    /* 初始化CS引脚 */
    W25Q128_CS_Init();
    
    /* 尝试读取设备信息 */
    if (W25Q128_ReadInfo(&test_info) != W25Q128_OK) {
        DEBUG_Printf("Flash连接测试失败：无法读取设备信息\r\n");
        return W25Q128_ERROR;
    }
    
    DEBUG_Printf("Flash连接测试成功\r\n");
    DEBUG_Printf("读取到的设备信息：\r\n");
    DEBUG_Printf("- 制造商ID: 0x%02X\r\n", test_info.manufacturer_id);
    DEBUG_Printf("- 设备ID: 0x%04X\r\n", test_info.device_id);
    DEBUG_Printf("- 内存类型: 0x%02X\r\n", test_info.memory_type);
    DEBUG_Printf("- 容量: 0x%02X\r\n", test_info.capacity);
    
    /* 检查是否为W25Q128 */
    if (test_info.manufacturer_id == 0xEF && test_info.device_id == 0x4018) {
        DEBUG_Printf("确认为W25Q128芯片\r\n");
        return W25Q128_OK;
    } else {
        DEBUG_Printf("警告：非W25Q128芯片\r\n");
        return W25Q128_ERROR;
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  SPI发送数据
  * @param  data: 发送数据指针
  * @param  size: 发送大小
  * @retval W25Q128_Status_t 操作状态
  */
static W25Q128_Status_t W25Q128_SPI_Transmit(uint8_t *data, uint16_t size)
{
    if (HAL_SPI_Transmit(&hspi2, data, size, W25Q128_SPI_TIMEOUT) != HAL_OK) {
        return W25Q128_ERROR;
    }
    return W25Q128_OK;
}

/**
  * @brief  SPI接收数据
  * @param  data: 接收数据指针
  * @param  size: 接收大小
  * @retval W25Q128_Status_t 操作状态
  */
static W25Q128_Status_t W25Q128_SPI_Receive(uint8_t *data, uint16_t size)
{
    if (HAL_SPI_Receive(&hspi2, data, size, W25Q128_SPI_TIMEOUT) != HAL_OK) {
        return W25Q128_ERROR;
    }
    return W25Q128_OK;
}

/**
  * @brief  初始化CS引脚
  * @param  None
  * @retval None
  */
static void W25Q128_CS_Init(void)
{
    /* 配置PB12为GPIO输出模式，用于手动控制CS信号 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 使能GPIOB时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* 配置PB12为推挽输出 */
    GPIO_InitStruct.Pin = W25Q128_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(W25Q128_CS_GPIO_PORT, &GPIO_InitStruct);
    
    /* 初始化CS为高电平（芯片未选中状态） */
    W25Q128_CS_HIGH();
    
    DEBUG_Printf("CS引脚配置为GPIO输出模式 (PB12-手动控制)\r\n");
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */ 


