#ifndef __OLED_DISPLAY_H__
#define __OLED_DISPLAY_H__

#include "main.h"

// OLED��ʾ���ֱ���
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// OLED�����С��������
#define FONT_SIZE_6X8   6   // 6x8���壨Ĭ���Ƽ���
#define FONT_SIZE_8X16  8   // 8x16���壨���ã�

// OLED��ʾ�������
#define OLED_ALARM_AREA_Y     0   // ����������Ϣ��Y����
#define OLED_STATUS_AREA_Y   10   // �в���ͨ��״̬��Y����
#define OLED_INFO_AREA_Y     42   // �ײ��¶������ת����Y����

// ��ʾ���ݻ����С����
#define DISPLAY_CACHE_SIZE 256   // ��ʾ���ݻ����С

// ����ˢ����ض���
#define OLED_DIRTY_REGION_MAX 8   // �����������
typedef struct {
    uint8_t x_start;      // ������ʼX����
    uint8_t y_start;      // ������ʼY���꣨ҳ��
    uint8_t width;        // �������
    uint8_t height;       // �����߶ȣ�ҳ����
    uint8_t is_valid;     // �����Ƿ���Ч
} DirtyRegion_t;

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
// ��ʾ�Լ�������Ͳ�������
void OLED_ShowSelfTestBarWithStep(uint8_t percent, uint8_t step);
// ��ʾ����������������
void OLED_ShowMainScreen(const char* alarm, const char* status1, const char* status2, const char* status3, const char* tempInfo, const char* fanInfo);
// ������ʾ������Ϣ
void OLED_ScrollAlarm(const char* alarmList[], uint8_t count, uint8_t step);
// ˢ��������ֻˢ���б仯������
void OLED_RefreshDirtyArea(void);
// ��ʾӢ���ַ�����Ĭ��6x8���壬8x16���屸�ã�
void OLED_DrawString(uint8_t x, uint8_t y, const char* str, uint8_t fontSize);
// ��ʾӢ���ַ�����6x8���壬�򻯽ӿڣ�
void OLED_DrawString6x8(uint8_t x, uint8_t y, const char* str);
// ��ʾӢ���ַ�����8x16���壬���ýӿڣ�
void OLED_DrawString8x16(uint8_t x, uint8_t y, const char* str);

// ����ˢ�¹��ܺ�������
void OLED_ShowMainScreenSmart(const char* alarm, const char* status1, const char* status2, const char* status3, const char* tempInfo, const char* fanInfo);
void OLED_ForceRefresh(void);  // ǿ��ˢ����ʾ
void OLED_ResetDisplayCache(void);  // ������ʾ����

// ����ˢ�¹��ܺ�������
void OLED_ShowMainScreenDirty(const char* alarm, const char* status1, const char* status2, const char* status3, const char* tempInfo, const char* fanInfo);
void OLED_DrawStringDirty(uint8_t x, uint8_t y, const char* str, uint8_t fontSize);  // �����ַ�����ʾ
void OLED_RefreshDirtyRegions(void);  // ֻˢ������
void OLED_ClearDirtyRegions(void);    // ���������¼
void OLED_AddDirtyRegion(uint8_t x, uint8_t y, uint8_t w, uint8_t h);  // �������

#endif // __OLED_DISPLAY_H__ 

