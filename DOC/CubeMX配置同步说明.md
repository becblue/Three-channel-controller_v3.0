# CubeMX配置同步说明

## 修改目的
为了保持CubeMX配置文件与实际项目代码的一致性，同步修改了DC_CTRL引脚的中断检测模式配置。

## 修改内容

### 问题
在之前的修复中，我们在代码中将DC_CTRL引脚的中断模式从只检测上升沿改为同时检测上升沿和下降沿，但没有同步更新CubeMX配置文件，导致配置不一致。

### 修改的文件
**文件**: `Three-channel controller_v3.0.ioc`

### 具体修改
**DC_CTRL引脚（PB5）配置变更**：

```diff
PB5.GPIOParameters=GPIO_PuPd,GPIO_Label,GPIO_ModeDefaultEXTI
PB5.GPIO_Label=DC_CTRL
- PB5.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING
+ PB5.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
PB5.GPIO_PuPd=GPIO_PULLUP
PB5.Locked=true
PB5.Signal=GPXTI5
```

## 修改原因

### 1. 功能需求
DC_CTRL引脚需要检测电源状态的完整变化：
- **上升沿**：DC24V断电时（正常→异常）
- **下降沿**：DC24V恢复时（异常→正常）

### 2. 代码一致性
确保CubeMX配置与实际代码中的GPIO配置保持一致：

**代码中的配置**（`Core/Src/gpio.c`）：
```c
GPIO_InitStruct.Pin = DC_CTRL_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;  // 双边沿中断
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(DC_CTRL_GPIO_Port, &GPIO_InitStruct);
```

**CubeMX配置**（`Three-channel controller_v3.0.ioc`）：
```
PB5.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING  // 双边沿中断
```

### 3. 与其他引脚保持一致
现在DC_CTRL引脚的配置与其他使能信号引脚保持一致：

| 引脚 | 信号名 | 中断模式 | 用途 |
|------|--------|----------|------|
| PA15 | K3_EN | GPIO_MODE_IT_RISING_FALLING | 通道3使能信号 |
| PB8 | K2_EN | GPIO_MODE_IT_RISING_FALLING | 通道2使能信号 |
| PB9 | K1_EN | GPIO_MODE_IT_RISING_FALLING | 通道1使能信号 |
| **PB5** | **DC_CTRL** | **GPIO_MODE_IT_RISING_FALLING** | **电源监控信号** |

## 验证方法

### 1. 配置文件检查
使用grep命令验证配置是否正确：
```bash
grep "GPIO_MODE_IT_RISING_FALLING" "Three-channel controller_v3.0.ioc"
```

**预期结果**：
```
PA15.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
PB5.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING    # 新增
PB8.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
PB9.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
```

### 2. CubeMX重新生成验证
1. 在CubeMX中打开`Three-channel controller_v3.0.ioc`文件
2. 检查PB5（DC_CTRL）引脚配置
3. 确认中断模式为"External Interrupt Mode with Rising/Falling edge trigger detection"
4. 重新生成代码，验证生成的`gpio.c`文件中DC_CTRL配置正确

## 注意事项

### 1. 版本控制
- 此修改确保了CubeMX配置文件与代码的一致性
- 团队成员使用CubeMX重新生成代码时不会覆盖我们的修改

### 2. 后续开发
- 如果需要在CubeMX中修改其他配置，请确保不要改变DC_CTRL的中断模式
- 任何GPIO配置的修改都应该同时更新CubeMX配置文件

### 3. 备份建议
- 建议在修改前备份原始的`.ioc`文件
- 保持配置文件的版本控制记录

## 总结
通过同步修改CubeMX配置文件，确保了：
1. **配置一致性**：CubeMX配置与实际代码完全一致
2. **功能完整性**：DC_CTRL引脚能正确检测电源状态的双向变化
3. **开发便利性**：团队成员可以安全地使用CubeMX重新生成代码
4. **维护性**：后续的配置修改有明确的参考标准

这样就完成了从硬件配置到软件实现的完整同步，确保电源监控功能在各个层面都配置正确。 