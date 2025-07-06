# CubeMX配置文件IWDG看门狗更新说明

## 概述
本文档说明了在STM32CubeMX配置文件中添加IWDG看门狗的具体修改内容，确保与代码实现的看门狗功能完全匹配。

## 配置文件路径
- **配置文件**: `Three-channel controller_v3.0.ioc`
- **芯片型号**: STM32F103RCT6
- **CubeMX版本**: 6.12.1

## 主要修改内容

### 1. 添加IWDG外设
```
# 原配置
Mcu.IPNb=10

# 新配置
Mcu.IPNb=11
Mcu.IP3=IWDG  # 添加IWDG作为第3个IP
```

### 2. IWDG参数配置
```
# IWDG基本参数
IWDG.IPParameters=Prescaler,Reload
IWDG.Prescaler=IWDG_PRESCALER_64
IWDG.Reload=1875
```

**参数详解**:
- **预分频器**: 64
- **重装载值**: 1875
- **超时时间**: 3秒 (LSI时钟37KHz条件下)
- **计算公式**: 超时时间 = (预分频器 × 重装载值) / LSI时钟频率

### 3. 虚拟引脚配置
```
# 添加IWDG虚拟引脚
VP_IWDG_VS_IWDG.Mode=IWDG_Activate
VP_IWDG_VS_IWDG.Signal=IWDG_VS_IWDG
```

### 4. 引脚数量更新
```
# 原配置
Mcu.PinsNb=50

# 新配置
Mcu.PinsNb=51
Mcu.Pin48=VP_IWDG_VS_IWDG  # 添加IWDG虚拟引脚
```

### 5. 初始化函数顺序
```
# 更新初始化函数列表
ProjectManager.functionlistsort=
1-SystemClock_Config-RCC-false-HAL-false,
2-MX_GPIO_Init-GPIO-false-HAL-true,
3-MX_DMA_Init-DMA-false-HAL-true,
4-MX_ADC1_Init-ADC1-false-HAL-true,
5-MX_I2C1_Init-I2C1-false-HAL-true,
6-MX_IWDG_Init-IWDG-false-HAL-true,  # 添加IWDG初始化
7-MX_SPI2_Init-SPI2-false-HAL-true,
8-MX_TIM3_Init-TIM3-false-HAL-true,
9-MX_USART1_UART_Init-USART1-false-HAL-true,
10-MX_USART3_UART_Init-USART3-false-HAL-true
```

## 生成的代码文件

### 1. iwdg.h
```c
#ifndef __IWDG_H__
#define __IWDG_H__

#include "main.h"

extern IWDG_HandleTypeDef hiwdg;

void MX_IWDG_Init(void);

#endif /* __IWDG_H__ */
```

### 2. iwdg.c
```c
#include "iwdg.h"

IWDG_HandleTypeDef hiwdg;

void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload = 1875;
    
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        Error_Handler();
    }
}
```

### 3. main.c中的调用
```c
int main(void)
{
    // ... 其他初始化代码 ...
    
    MX_IWDG_Init();    // IWDG初始化
    
    // ... 其他代码 ...
}
```

## 使用CubeMX打开配置

### 1. 打开项目
- 启动STM32CubeMX
- 选择 "Open Project"
- 选择 `Three-channel controller_v3.0.ioc` 文件

### 2. 查看IWDG配置
- 在左侧外设列表中找到 "IWDG"
- 点击展开查看配置选项:
  - **Prescaler**: 64
  - **Counter Reload Value**: 1875
  - **Window Value**: 4095 (默认值)

### 3. 验证配置
- 检查 "Parameter Settings" 选项卡
- 确认预分频器和重装载值正确
- 检查 "NVIC Settings" 选项卡（IWDG无需中断）

### 4. 重新生成代码
- 点击 "Generate Code" 按钮
- 选择项目路径
- 生成成功后会更新相关文件

## 注意事项

### 1. 代码兼容性
- 生成的代码与现有的看门狗控制模块完全兼容
- `MX_IWDG_Init()` 函数已在现有代码中正确调用
- 无需修改用户代码

### 2. 配置保护
- 本配置文件启用了 "Keep User Code" 选项
- 重新生成时会保留用户代码段
- 确保用户自定义的看门狗控制逻辑不会丢失

### 3. 调试考虑
- IWDG在调试模式下不会停止运行
- 如需调试时暂停看门狗，需要在代码中实现
- 使用 `IwdgControl_Suspend()` 暂停自动喂狗

### 4. 系统集成
- IWDG配置与现有的看门狗控制模块完全集成
- 超时时间、喂狗间隔等参数在代码中可配置
- 支持动态调整看门狗行为

## 配置验证

### 1. 编译检查
```bash
# 编译项目验证配置正确性
cd MDK-ARM
# 使用Keil MDK编译项目
```

### 2. 功能测试
```c
// 在main.c中添加测试代码
void Test_IWDG_Config(void)
{
    // 初始化看门狗控制模块
    IwdgControl_Init();
    
    // 启动看门狗
    IwdgControl_Start();
    
    // 检查配置
    DEBUG_Printf("IWDG配置验证:\r\n");
    DEBUG_Printf("- 预分频器: %d\r\n", IWDG_PRESCALER_64);
    DEBUG_Printf("- 重装载值: %d\r\n", 1875);
    DEBUG_Printf("- 超时时间: %dms\r\n", 3000);
}
```

## 故障排除

### 1. 编译错误
- 检查 `iwdg.h` 是否正确包含在 `main.h` 中
- 确认 `MX_IWDG_Init()` 函数声明正确
- 验证HAL库的IWDG驱动已启用

### 2. 运行时问题
- 检查LSI时钟是否正常启动
- 确认预分频器和重装载值计算正确
- 验证系统时钟配置

### 3. 配置不匹配
- 重新打开CubeMX项目检查配置
- 对比生成的代码与现有实现
- 确认所有参数一致

## 更新历史

| 版本 | 日期 | 更新内容 |
|------|------|----------|
| 1.0 | 2024-01-XX | 初始版本，添加IWDG基本配置 |
| 1.1 | 2024-01-XX | 完善配置参数，添加虚拟引脚 |
| 1.2 | 2024-01-XX | 更新初始化函数顺序 |

## 结论

通过本次配置更新，STM32CubeMX项目现在完全支持IWDG看门狗功能，配置参数与代码实现完全匹配。用户可以通过CubeMX图形界面方便地管理看门狗配置，同时保持与现有高级控制模块的完全兼容性。

**重要提醒**: 在使用CubeMX重新生成代码时，请确保选择 "Keep User Code" 选项，以保护现有的用户代码段不被覆盖。 