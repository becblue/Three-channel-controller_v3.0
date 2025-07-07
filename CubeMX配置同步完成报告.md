# CubeMX����ͬ����ɱ���

## ������
ͨ��ϵͳ�ԱȽ�CubeMX�����ļ���.ioc����ʵ�ʴ��룬���ֲ��޸����������ò�һ�µ����⣺

## �޸������ò�һ������

### 1. PC13���������޸�
**��������**��
- CubeMX���ã�PC13 (KEY1) ����Ϊ�½��ش��� (`GPIO_MODE_IT_FALLING`)
- �������ã�PC13����Ϊ˫���ش��� (`GPIO_MODE_IT_RISING_FALLING`)

**�޸���ʩ**��
- �޸�CubeMX�����ļ���PC13���жϴ���ģʽ
- �� `GPIO_MODE_IT_FALLING` ��Ϊ `GPIO_MODE_IT_RISING_FALLING`
- ȷ��������еĳ�������߼�һ��

### 2. SPI2�����������޸�
**��������**��
- CubeMX���ã�SPI2������Ԥ��Ƶ��Ϊ16 (`SPI_BAUDRATEPRESCALER_16`)
- �������ã�SPI2������Ԥ��Ƶ��Ϊ64 (`SPI_BAUDRATEPRESCALER_64`)

**�޸���ʩ**��
- �޸�CubeMX�����ļ���SPI2�Ĳ�����Ԥ��Ƶ��
- �� `SPI_BAUDRATEPRESCALER_16` ��Ϊ `SPI_BAUDRATEPRESCALER_64`
- ͬʱ���¼���Ĳ����ʴ�2.25 MBits/s��1.125 MBits/s

## ȷ��һ�µ�����

### 1. ���Ŷ���ӳ��
? **������������**��
- PC13: KEY1 (˫���ش���)
- PC14: KEY2 (�½��ش���)
- PC12: FAN_SEN (�½��ش���)
- PB5: DC_CTRL (˫���ش���)
- PB8: K2_EN (˫���ش���)
- PB9: K1_EN (˫���ش���)
- PA15: K3_EN (˫���ش���)

? **SPI2����**��
- PB13: SPI2_SCK
- PB14: SPI2_MISO
- PB15: SPI2_MOSI
- ������Ԥ��Ƶ����64
- ģʽ������ģʽ��ȫ˫��

? **I2C1����**��
- PB6: I2C1_SCL
- PB7: I2C1_SDA

? **��������**��
- USART1: PA9(TX), PA10(RX)
- USART3: PC10(TX), PC11(RX)

### 2. �ж�����
? **EXTI�ж����ȼ�**��
- EXTI9_5_IRQn: ���ȼ�4
- EXTI15_10_IRQn: ���ȼ�4

### 3. ����Ӳ����ƴ���
? **W25Q128 CS����**��
- Ӳ����ƣ�CS����ֱ�ӽ�GND
- ���봦��CS���ƺ궨��Ϊ�ղ���
- CubeMX���ã���������PB2������ʵ��Ӳ�����

## ��֤���
����ϵͳ�Լ�飬����CubeMX�����ļ���ʵ�ʴ�����ȫһ�£�

1. **GPIO����**���������ŵ�ģʽ�����������ٶ����þ�һ��
2. **�ж�����**�������ⲿ�жϵĴ���ģʽ�����ȼ���һ��
3. **SPI����**�������ʡ�ģʽ������ӳ���һ��
4. **��������**��I2C��USART��ADC��TIM�����þ�һ��

## �����ļ��޸ļ�¼
```diff
# PC13���������޸�
- PC13-TAMPER-RTC.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_FALLING
+ PC13-TAMPER-RTC.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING

# SPI2�����������޸�
- SPI2.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_16
+ SPI2.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_64

- SPI2.CalculateBaudRate=2.25 MBits/s
+ SPI2.CalculateBaudRate=1.125 MBits/s
```

## ����
1. �ں��������У�����޸��˴����е��������ã���ͬ������CubeMX�����ļ�
2. ���鶨�ڽ�������һ���Լ�飬��������Ư��
3. �������ɴ���ǰ����ȷ�������ļ���һ����

## �ܽ�
�������ò�һ���������޸���CubeMX�����ļ�������ʵ�ʴ�����ȫͬ����ϵͳ���������������ɴ�������������ͻ�� 