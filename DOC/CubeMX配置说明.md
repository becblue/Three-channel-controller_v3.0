# CubeMX�����ļ�IWDG���Ź�����˵��

## ����
���ĵ�˵������STM32CubeMX�����ļ������IWDG���Ź��ľ����޸����ݣ�ȷ�������ʵ�ֵĿ��Ź�������ȫƥ�䡣

## �����ļ�·��
- **�����ļ�**: `Three-channel controller_v3.0.ioc`
- **оƬ�ͺ�**: STM32F103RCT6
- **CubeMX�汾**: 6.12.1

## ��Ҫ�޸�����

### 1. ���IWDG����
```
# ԭ����
Mcu.IPNb=10

# ������
Mcu.IPNb=11
Mcu.IP3=IWDG  # ���IWDG��Ϊ��3��IP
```

### 2. IWDG��������
```
# IWDG��������
IWDG.IPParameters=Prescaler,Reload
IWDG.Prescaler=IWDG_PRESCALER_64
IWDG.Reload=1875
```

**�������**:
- **Ԥ��Ƶ��**: 64
- **��װ��ֵ**: 1875
- **��ʱʱ��**: 3�� (LSIʱ��37KHz������)
- **���㹫ʽ**: ��ʱʱ�� = (Ԥ��Ƶ�� �� ��װ��ֵ) / LSIʱ��Ƶ��

### 3. ������������
```
# ���IWDG��������
VP_IWDG_VS_IWDG.Mode=IWDG_Activate
VP_IWDG_VS_IWDG.Signal=IWDG_VS_IWDG
```

### 4. ������������
```
# ԭ����
Mcu.PinsNb=50

# ������
Mcu.PinsNb=51
Mcu.Pin48=VP_IWDG_VS_IWDG  # ���IWDG��������
```

### 5. ��ʼ������˳��
```
# ���³�ʼ�������б�
ProjectManager.functionlistsort=
1-SystemClock_Config-RCC-false-HAL-false,
2-MX_GPIO_Init-GPIO-false-HAL-true,
3-MX_DMA_Init-DMA-false-HAL-true,
4-MX_ADC1_Init-ADC1-false-HAL-true,
5-MX_I2C1_Init-I2C1-false-HAL-true,
6-MX_IWDG_Init-IWDG-false-HAL-true,  # ���IWDG��ʼ��
7-MX_SPI2_Init-SPI2-false-HAL-true,
8-MX_TIM3_Init-TIM3-false-HAL-true,
9-MX_USART1_UART_Init-USART1-false-HAL-true,
10-MX_USART3_UART_Init-USART3-false-HAL-true
```

## ���ɵĴ����ļ�

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

### 3. main.c�еĵ���
```c
int main(void)
{
    // ... ������ʼ������ ...
    
    MX_IWDG_Init();    // IWDG��ʼ��
    
    // ... �������� ...
}
```

## ʹ��CubeMX������

### 1. ����Ŀ
- ����STM32CubeMX
- ѡ�� "Open Project"
- ѡ�� `Three-channel controller_v3.0.ioc` �ļ�

### 2. �鿴IWDG����
- ����������б����ҵ� "IWDG"
- ���չ���鿴����ѡ��:
  - **Prescaler**: 64
  - **Counter Reload Value**: 1875
  - **Window Value**: 4095 (Ĭ��ֵ)

### 3. ��֤����
- ��� "Parameter Settings" ѡ�
- ȷ��Ԥ��Ƶ������װ��ֵ��ȷ
- ��� "NVIC Settings" ѡ���IWDG�����жϣ�

### 4. �������ɴ���
- ��� "Generate Code" ��ť
- ѡ����Ŀ·��
- ���ɳɹ�����������ļ�

## ע������

### 1. ���������
- ���ɵĴ��������еĿ��Ź�����ģ����ȫ����
- `MX_IWDG_Init()` �����������д�������ȷ����
- �����޸��û�����

### 2. ���ñ���
- �������ļ������� "Keep User Code" ѡ��
- ��������ʱ�ᱣ���û������
- ȷ���û��Զ���Ŀ��Ź������߼����ᶪʧ

### 3. ���Կ���
- IWDG�ڵ���ģʽ�²���ֹͣ����
- �������ʱ��ͣ���Ź�����Ҫ�ڴ�����ʵ��
- ʹ�� `IwdgControl_Suspend()` ��ͣ�Զ�ι��

### 4. ϵͳ����
- IWDG���������еĿ��Ź�����ģ����ȫ����
- ��ʱʱ�䡢ι������Ȳ����ڴ����п�����
- ֧�ֶ�̬�������Ź���Ϊ

## ������֤

### 1. ������
```bash
# ������Ŀ��֤������ȷ��
cd MDK-ARM
# ʹ��Keil MDK������Ŀ
```

### 2. ���ܲ���
```c
// ��main.c����Ӳ��Դ���
void Test_IWDG_Config(void)
{
    // ��ʼ�����Ź�����ģ��
    IwdgControl_Init();
    
    // �������Ź�
    IwdgControl_Start();
    
    // �������
    DEBUG_Printf("IWDG������֤:\r\n");
    DEBUG_Printf("- Ԥ��Ƶ��: %d\r\n", IWDG_PRESCALER_64);
    DEBUG_Printf("- ��װ��ֵ: %d\r\n", 1875);
    DEBUG_Printf("- ��ʱʱ��: %dms\r\n", 3000);
}
```

## �����ų�

### 1. �������
- ��� `iwdg.h` �Ƿ���ȷ������ `main.h` ��
- ȷ�� `MX_IWDG_Init()` ����������ȷ
- ��֤HAL���IWDG����������

### 2. ����ʱ����
- ���LSIʱ���Ƿ���������
- ȷ��Ԥ��Ƶ������װ��ֵ������ȷ
- ��֤ϵͳʱ������

### 3. ���ò�ƥ��
- ���´�CubeMX��Ŀ�������
- �Ա����ɵĴ���������ʵ��
- ȷ�����в���һ��

## ������ʷ

| �汾 | ���� | �������� |
|------|------|----------|
| 1.0 | 2024-01-XX | ��ʼ�汾�����IWDG�������� |
| 1.1 | 2024-01-XX | �������ò���������������� |
| 1.2 | 2024-01-XX | ���³�ʼ������˳�� |

## ����

ͨ���������ø��£�STM32CubeMX��Ŀ������ȫ֧��IWDG���Ź����ܣ����ò��������ʵ����ȫƥ�䡣�û�����ͨ��CubeMXͼ�ν��淽��ع����Ź����ã�ͬʱ���������и߼�����ģ�����ȫ�����ԡ�

**��Ҫ����**: ��ʹ��CubeMX�������ɴ���ʱ����ȷ��ѡ�� "Keep User Code" ѡ��Ա������е��û�����β������ǡ� 