#ifndef __OLED_DISPLAY_H__
#define __OLED_DISPLAY_H__

#include "main.h"

// OLED显示屏分辨率
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// OLED显示区域分区
#define OLED_ALARM_AREA_Y     0   // 顶部报警信息区Y坐标
#define OLED_STATUS_AREA_Y   10   // 中部三通道状态区Y坐标
#define OLED_INFO_AREA_Y     42   // 底部温度与风扇转速区Y坐标

// 测试OLED I2C连接
uint8_t OLED_TestConnection(void);
// 初始化OLED显示屏
void OLED_Init(void);
// 清屏
void OLED_Clear(void);
// 刷新显存到OLED
void OLED_Refresh(void);
// 显示LOGO（居中显示）
void OLED_ShowLogo(void);
// 显示自检进度条
void OLED_ShowSelfTestBar(uint8_t percent);
// 显示主界面三分区内容
void OLED_ShowMainScreen(const char* alarm, const char* status1, const char* status2, const char* status3, const char* tempInfo, const char* fanInfo);
// 滚动显示报警信息
void OLED_ScrollAlarm(const char* alarmList[], uint8_t count, uint8_t step);
// 刷新脏区（只刷新有变化的区域）
void OLED_RefreshDirtyArea(void);
// 显示英文字符串（6x8/8x16）
void OLED_DrawString(uint8_t x, uint8_t y, const char* str, uint8_t fontSize);

#endif // __OLED_DISPLAY_H__ 

