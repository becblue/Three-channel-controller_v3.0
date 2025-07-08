# CubeMX����ͬ��˵��

## �޸�Ŀ��
Ϊ�˱���CubeMX�����ļ���ʵ����Ŀ�����һ���ԣ�ͬ���޸���DC_CTRL���ŵ��жϼ��ģʽ���á�

## �޸�����

### ����
��֮ǰ���޸��У������ڴ����н�DC_CTRL���ŵ��ж�ģʽ��ֻ��������ظ�Ϊͬʱ��������غ��½��أ���û��ͬ������CubeMX�����ļ����������ò�һ�¡�

### �޸ĵ��ļ�
**�ļ�**: `Three-channel controller_v3.0.ioc`

### �����޸�
**DC_CTRL���ţ�PB5�����ñ��**��

```diff
PB5.GPIOParameters=GPIO_PuPd,GPIO_Label,GPIO_ModeDefaultEXTI
PB5.GPIO_Label=DC_CTRL
- PB5.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING
+ PB5.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
PB5.GPIO_PuPd=GPIO_PULLUP
PB5.Locked=true
PB5.Signal=GPXTI5
```

## �޸�ԭ��

### 1. ��������
DC_CTRL������Ҫ����Դ״̬�������仯��
- **������**��DC24V�ϵ�ʱ���������쳣��
- **�½���**��DC24V�ָ�ʱ���쳣��������

### 2. ����һ����
ȷ��CubeMX������ʵ�ʴ����е�GPIO���ñ���һ�£�

**�����е�����**��`Core/Src/gpio.c`����
```c
GPIO_InitStruct.Pin = DC_CTRL_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;  // ˫�����ж�
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(DC_CTRL_GPIO_Port, &GPIO_InitStruct);
```

**CubeMX����**��`Three-channel controller_v3.0.ioc`����
```
PB5.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING  // ˫�����ж�
```

### 3. ���������ű���һ��
����DC_CTRL���ŵ�����������ʹ���ź����ű���һ�£�

| ���� | �ź��� | �ж�ģʽ | ��; |
|------|--------|----------|------|
| PA15 | K3_EN | GPIO_MODE_IT_RISING_FALLING | ͨ��3ʹ���ź� |
| PB8 | K2_EN | GPIO_MODE_IT_RISING_FALLING | ͨ��2ʹ���ź� |
| PB9 | K1_EN | GPIO_MODE_IT_RISING_FALLING | ͨ��1ʹ���ź� |
| **PB5** | **DC_CTRL** | **GPIO_MODE_IT_RISING_FALLING** | **��Դ����ź�** |

## ��֤����

### 1. �����ļ����
ʹ��grep������֤�����Ƿ���ȷ��
```bash
grep "GPIO_MODE_IT_RISING_FALLING" "Three-channel controller_v3.0.ioc"
```

**Ԥ�ڽ��**��
```
PA15.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
PB5.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING    # ����
PB8.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
PB9.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
```

### 2. CubeMX����������֤
1. ��CubeMX�д�`Three-channel controller_v3.0.ioc`�ļ�
2. ���PB5��DC_CTRL����������
3. ȷ���ж�ģʽΪ"External Interrupt Mode with Rising/Falling edge trigger detection"
4. �������ɴ��룬��֤���ɵ�`gpio.c`�ļ���DC_CTRL������ȷ

## ע������

### 1. �汾����
- ���޸�ȷ����CubeMX�����ļ�������һ����
- �Ŷӳ�Աʹ��CubeMX�������ɴ���ʱ���Ḳ�����ǵ��޸�

### 2. ��������
- �����Ҫ��CubeMX���޸��������ã���ȷ����Ҫ�ı�DC_CTRL���ж�ģʽ
- �κ�GPIO���õ��޸Ķ�Ӧ��ͬʱ����CubeMX�����ļ�

### 3. ���ݽ���
- �������޸�ǰ����ԭʼ��`.ioc`�ļ�
- ���������ļ��İ汾���Ƽ�¼

## �ܽ�
ͨ��ͬ���޸�CubeMX�����ļ���ȷ���ˣ�
1. **����һ����**��CubeMX������ʵ�ʴ�����ȫһ��
2. **����������**��DC_CTRL��������ȷ����Դ״̬��˫��仯
3. **����������**���Ŷӳ�Ա���԰�ȫ��ʹ��CubeMX�������ɴ���
4. **ά����**�������������޸�����ȷ�Ĳο���׼

����������˴�Ӳ�����õ����ʵ�ֵ�����ͬ����ȷ����Դ��ع����ڸ������涼������ȷ�� 