/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : w25q128_driver.h
  * @brief          : W25Q128JVSIQ SPI Flash驱动头文件
  ******************************************************************************
  * @attention
  *
  * 这是为三通道切换箱控制系统设计的W25Q128 SPI Flash驱动
  * 支持基础的读写擦除操作，用于日志存储功能
  * 
  * 硬件设计说明：
 * - CS引脚连接到PB12，使用普通GPIO手动控制CS信号
 * - 使用SPI_NSS_SOFT模式，通过软件精确控制CS时序
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

/* W25Q128 Flash信息结构体 */
typedef struct {
    uint8_t  manufacturer_id;       // 制造商ID (0xEF)
    uint8_t  memory_type;           // 存储器类型 (0x40)
    uint8_t  capacity;              // 容量ID (0x18 = 128Mbit)
    uint16_t device_id;             // 设备ID (0x4018)
    uint64_t unique_id;             // 唯一ID
    uint32_t total_size;            // 总容量（字节）
    uint32_t page_size;             // 页大小（字节）
    uint32_t sector_size;           // 扇区大小（字节）
    uint32_t block_size;            // 块大小（字节）
} W25Q128_Info_t;

/* Flash操作状态 */
typedef enum {
    W25Q128_OK = 0,                 // 操作成功
    W25Q128_ERROR,                  // 操作失败
    W25Q128_BUSY,                   // 设备忙碌
    W25Q128_TIMEOUT,                // 操作超时
    W25Q128_PROTECTED               // 写保护
} W25Q128_Status_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* W25Q128 Flash容量参数 */
#define W25Q128_FLASH_SIZE          0x1000000   // 16MB (128Mbit)
#define W25Q128_SECTOR_SIZE         0x1000      // 4KB
#define W25Q128_PAGE_SIZE           0x100       // 256 bytes
#define W25Q128_BLOCK_SIZE          0x10000     // 64KB

/* W25Q128 Flash命令定义 */
#define W25Q128_CMD_WRITE_ENABLE    0x06        // 写使能
#define W25Q128_CMD_WRITE_DISABLE   0x04        // 写禁止
#define W25Q128_CMD_READ_STATUS1    0x05        // 读状态寄存器1
#define W25Q128_CMD_READ_STATUS2    0x35        // 读状态寄存器2
#define W25Q128_CMD_READ_STATUS3    0x15        // 读状态寄存器3
#define W25Q128_CMD_WRITE_STATUS1   0x01        // 写状态寄存器1
#define W25Q128_CMD_READ_DATA       0x03        // 读数据
#define W25Q128_CMD_FAST_READ       0x0B        // 快速读
#define W25Q128_CMD_PAGE_PROGRAM    0x02        // 页编程
#define W25Q128_CMD_SECTOR_ERASE    0x20        // 扇区擦除（4KB）
#define W25Q128_CMD_BLOCK_ERASE     0xD8        // 块擦除（64KB）
#define W25Q128_CMD_CHIP_ERASE      0xC7        // 芯片擦除
#define W25Q128_CMD_POWER_DOWN      0xB9        // 掉电模式
#define W25Q128_CMD_RELEASE_POWER   0xAB        // 释放掉电模式
#define W25Q128_CMD_DEVICE_ID       0x90        // 读设备ID
#define W25Q128_CMD_JEDEC_ID        0x9F        // 读JEDEC ID
#define W25Q128_CMD_UNIQUE_ID       0x4B        // 读唯一ID

/* 状态寄存器位定义 */
#define W25Q128_STATUS_BUSY         0x01        // 忙碌位
#define W25Q128_STATUS_WEL          0x02        // 写使能锁存位
#define W25Q128_STATUS_BP0          0x04        // 块保护位0
#define W25Q128_STATUS_BP1          0x08        // 块保护位1
#define W25Q128_STATUS_BP2          0x10        // 块保护位2
#define W25Q128_STATUS_TB           0x20        // 顶部/底部保护位
#define W25Q128_STATUS_SEC          0x40        // 扇区保护位
#define W25Q128_STATUS_SRP0         0x80        // 状态寄存器保护位0

/* CS引脚定义 - 使用PB12作为普通GPIO，手动控制CS信号 */
#define W25Q128_CS_PIN              GPIO_PIN_12     // PB12 - 手动CS控制
#define W25Q128_CS_GPIO_PORT        GPIOB           // GPIOB  
#define W25Q128_CS_LOW()            HAL_GPIO_WritePin(W25Q128_CS_GPIO_PORT, W25Q128_CS_PIN, GPIO_PIN_RESET)
#define W25Q128_CS_HIGH()           HAL_GPIO_WritePin(W25Q128_CS_GPIO_PORT, W25Q128_CS_PIN, GPIO_PIN_SET)

/* 超时定义 */
#define W25Q128_TIMEOUT_MS          5000        // 操作超时时间（毫秒）
#define W25Q128_SPI_TIMEOUT         1000        // SPI通信超时时间

/* CS接地模式特殊命令 */
#define W25Q128_CMD_ENABLE_RESET    0x66        // 使能复位
#define W25Q128_CMD_RESET_DEVICE    0x99        // 复位设备

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* 初始化和配置函数 */
W25Q128_Status_t W25Q128_Init(void);
W25Q128_Status_t W25Q128_DeInit(void);
W25Q128_Status_t W25Q128_ReadInfo(W25Q128_Info_t *info);
W25Q128_Status_t W25Q128_Reset(void);

/* 基础操作函数 */
W25Q128_Status_t W25Q128_ReadData(uint32_t address, uint8_t *data, uint32_t size);
W25Q128_Status_t W25Q128_WriteData(uint32_t address, uint8_t *data, uint32_t size);
W25Q128_Status_t W25Q128_EraseSector(uint32_t address);
W25Q128_Status_t W25Q128_EraseBlock(uint32_t address);
W25Q128_Status_t W25Q128_EraseChip(void);

/* 状态检查函数 */
W25Q128_Status_t W25Q128_IsBusy(void);
W25Q128_Status_t W25Q128_WaitForReady(void);
uint8_t W25Q128_ReadStatusRegister(uint8_t register_num);
W25Q128_Status_t W25Q128_WriteStatusRegister(uint8_t register_num, uint8_t value);

/* 写保护函数 */
W25Q128_Status_t W25Q128_WriteEnable(void);
W25Q128_Status_t W25Q128_WriteDisable(void);
W25Q128_Status_t W25Q128_SetProtection(uint8_t protection_level);

/* 电源管理函数 */
W25Q128_Status_t W25Q128_PowerDown(void);
W25Q128_Status_t W25Q128_PowerUp(void);

/* 工具函数 */
uint32_t W25Q128_GetSectorAddress(uint32_t address);
uint32_t W25Q128_GetBlockAddress(uint32_t address);
uint8_t W25Q128_IsAddressValid(uint32_t address);

/* 调试和测试函数 */
W25Q128_Status_t W25Q128_TestConnection(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __W25Q128_DRIVER_H */ 

