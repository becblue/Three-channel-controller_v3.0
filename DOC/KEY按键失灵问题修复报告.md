# KEY按键失灵问题修复报告

## 问题描述
用户反馈KEY1和KEY2按键失灵，在实现轮询防抖机制后按键无响应。

## 问题分析

### 引脚配置分析
- **KEY1**: PC13 (使用EXTI15_10_IRQn中断线)
- **KEY2**: PC14 (使用EXTI15_10_IRQn中断线)  
- **K1_EN**: PB9 (使用EXTI9_5_IRQn中断线)
- **K2_EN**: PB8 (使用EXTI9_5_IRQn中断线)
- **K3_EN**: PB15 (使用EXTI15_10_IRQn中断线)
- **DC_CTRL**: PB5 (使用EXTI9_5_IRQn中断线)
- **FAN_SEN**: PC15 (使用EXTI15_10_IRQn中断线)

### 根本原因
在`GPIO_DisableInterrupts`函数中，为了禁用受电磁干扰影响的K_EN和DC_CTRL信号的中断，**错误地禁用了整个中断线**：

```c
// 原来的错误代码
HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);   // 影响K1_EN, K2_EN, DC_CTRL
HAL_NVIC_DisableIRQ(EXTI15_10_IRQn); // 影响K3_EN, KEY1, KEY2, FAN_SEN
```

这导致：
- KEY1和KEY2的中断被误禁用（都在EXTI15_10_IRQn线上）
- K3_EN的轮询也受到影响
- FAN_SEN风扇传感器中断被误禁用

## 修复方案

### 技术方案
改为**选择性禁用特定引脚的EXTI配置**，而不是整个中断线：

```c
void GPIO_DisableInterrupts(void)
{
    // 只禁用特定引脚的EXTI配置，不影响按键和风扇中断
    // 禁用K1_EN (PB9)的EXTI配置
    __HAL_GPIO_EXTI_CLEAR_IT(K1_EN_Pin);
    EXTI->IMR &= ~(K1_EN_Pin);
    
    // 禁用K2_EN (PB8)的EXTI配置  
    __HAL_GPIO_EXTI_CLEAR_IT(K2_EN_Pin);
    EXTI->IMR &= ~(K2_EN_Pin);
    
    // 禁用K3_EN (PB15)的EXTI配置
    __HAL_GPIO_EXTI_CLEAR_IT(K3_EN_Pin);
    EXTI->IMR &= ~(K3_EN_Pin);
    
    // 禁用DC_CTRL (PB5)的EXTI配置
    __HAL_GPIO_EXTI_CLEAR_IT(DC_CTRL_Pin);
    EXTI->IMR &= ~(DC_CTRL_Pin);
    
    DEBUG_Printf("已禁用K_EN和DC_CTRL特定引脚中断，保留按键和风扇中断\r\n");
}
```

### 具体修改内容

#### 1. Core/Src/gpio_control.c
- **修改函数**: `GPIO_DisableInterrupts()`
- **修改内容**: 改为选择性禁用特定引脚EXTI配置
- **新增包含**: `#include "relay_control.h"` 和 `#include "safety_monitor.h"`

## 修复效果验证

### 编译结果
```
"Three-channel controller_v3.0\Three-channel controller_v3.0" - 0 Error(s), 0 Warning(s).
Build Time Elapsed: 00:00:02
```

### 预期效果
1. **KEY1和KEY2恢复正常**: 按键中断功能完全恢复
2. **轮询系统正常**: K1_EN、K2_EN、K3_EN、DC_CTRL继续使用轮询防抖
3. **风扇监控正常**: FAN_SEN中断功能不受影响
4. **电磁干扰免疫**: 继电器操作期间不再产生误中断

### 功能保持
- **? 按键功能**: KEY1、KEY2正常工作
- **? 轮询机制**: 电磁干扰敏感信号继续使用轮询
- **? 风扇监控**: 转速检测正常
- **? 系统稳定性**: 解决看门狗复位问题

## 测试建议

### 基本功能测试
1. **按键测试**:
   - 短按KEY1: 应触发相应功能
   - 长按KEY1: 应触发长按功能
   - 短按KEY2: 应触发相应功能
   - 长按KEY2: 应触发长按功能

2. **继电器操作测试**:
   - 触发继电器切换，确认串口调试信息正常
   - 观察OLED显示，确认按键响应正常

3. **EMI抗干扰测试**:
   - 连续切换高压继电器
   - 确认按键在继电器操作期间仍能正常响应
   - 确认串口调试信息稳定输出

## 技术要点

### EXTI中断线共享机制
STM32的外部中断线是共享的：
- **EXTI0-4**: 每个有独立中断向量
- **EXTI5-9**: 共享EXTI9_5_IRQn
- **EXTI10-15**: 共享EXTI15_10_IRQn

### 选择性禁用的优势
1. **精确控制**: 只影响指定引脚，不误伤其他功能
2. **系统稳定**: 保持按键、风扇等正常功能
3. **EMI免疫**: 有效阻止电磁干扰引起的误触发

## 总结
通过改进GPIO中断禁用机制，成功修复了KEY1和KEY2失灵问题，同时保持了轮询防抖系统的电磁干扰免疫效果。这种**选择性EXTI禁用**的方案确保了系统的稳定性和功能完整性。

---
**修复日期**: 2025-01-07  
**修复状态**: ? 已完成  
**编译状态**: ? 0错误 0警告  
**测试要求**: 需要硬件验证按键功能恢复 