#ifndef __OLED_DISPLAY_H__
#define __OLED_DISPLAY_H__

#include "main.h"

// OLED��ʾ���ֱ���
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// OLED��ʾ�������
#define OLED_ALARM_AREA_Y     0   // ����������Ϣ��Y����
#define OLED_STATUS_AREA_Y   10   // �в���ͨ��״̬��Y����
#define OLED_INFO_AREA_Y     42   // �ײ��¶������ת����Y����

// ����OLED I2C����
uint8_t OLED_TestConnection(void);
// ��ʼ��OLED��ʾ��
void OLED_Init(void);
// ����
void OLED_Clear(void);
// ˢ���Դ浽OLED
void OLED_Refresh(void);
// ��ʾLOGO��������ʾ��
void OLED_ShowLogo(void);
// ��ʾ�Լ������
void OLED_ShowSelfTestBar(uint8_t percent);
// ��ʾ����������������
void OLED_ShowMainScreen(const char* alarm, const char* status1, const char* status2, const char* status3, const char* tempInfo, const char* fanInfo);
// ������ʾ������Ϣ
void OLED_ScrollAlarm(const char* alarmList[], uint8_t count, uint8_t step);
// ˢ��������ֻˢ���б仯������
void OLED_RefreshDirtyArea(void);
// ��ʾӢ���ַ�����6x8/8x16��
void OLED_DrawString(uint8_t x, uint8_t y, const char* str, uint8_t fontSize);

#endif // __OLED_DISPLAY_H__ 

