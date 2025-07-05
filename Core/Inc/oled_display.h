#ifndef __OLED_DISPLAY_H__
#define __OLED_DISPLAY_H__

#include "main.h"

// OLED显示屏分辨率
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// OLED字体大小常量定义
#define FONT_SIZE_6X8   6   // 6x8字体（默认推荐）
#define FONT_SIZE_8X16  8   // 8x16字体（备用）

// OLED显示区域分区
#define OLED_ALARM_AREA_Y     0   // 顶部报警信息区Y坐标
#define OLED_STATUS_AREA_Y   10   // 中部三通道状态区Y坐标
#define OLED_INFO_AREA_Y     42   // 底部温度与风扇转速区Y坐标

// 显示内容缓存大小定义
#define DISPLAY_CACHE_SIZE 256   // 显示内容缓存大小

// 脏区刷新相关定义
#define OLED_DIRTY_REGION_MAX 8   // 最大脏区数量
typedef struct {
    uint8_t x_start;      // 脏区起始X坐标
    uint8_t y_start;      // 脏区起始Y坐标（页）
    uint8_t width;        // 脏区宽度
    uint8_t height;       // 脏区高度（页数）
    uint8_t is_valid;     // 脏区是否有效
} DirtyRegion_t;

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
// 显示自检进度条和步骤描述
void OLED_ShowSelfTestBarWithStep(uint8_t percent, uint8_t step);
// 显示主界面三分区内容
void OLED_ShowMainScreen(const char* alarm, const char* status1, const char* status2, const char* status3, const char* tempInfo, const char* fanInfo);
// 滚动显示报警信息
void OLED_ScrollAlarm(const char* alarmList[], uint8_t count, uint8_t step);
// 刷新脏区（只刷新有变化的区域）
void OLED_RefreshDirtyArea(void);
// 显示英文字符串（默认6x8字体，8x16字体备用）
void OLED_DrawString(uint8_t x, uint8_t y, const char* str, uint8_t fontSize);
// 显示英文字符串（6x8字体，简化接口）
void OLED_DrawString6x8(uint8_t x, uint8_t y, const char* str);
// 显示英文字符串（8x16字体，备用接口）
void OLED_DrawString8x16(uint8_t x, uint8_t y, const char* str);

// 智能刷新功能函数声明
void OLED_ShowMainScreenSmart(const char* alarm, const char* status1, const char* status2, const char* status3, const char* tempInfo, const char* fanInfo);
void OLED_ForceRefresh(void);  // 强制刷新显示
void OLED_ResetDisplayCache(void);  // 重置显示缓存

// 脏区刷新功能函数声明
void OLED_ShowMainScreenDirty(const char* alarm, const char* status1, const char* status2, const char* status3, const char* tempInfo, const char* fanInfo);
void OLED_DrawStringDirty(uint8_t x, uint8_t y, const char* str, uint8_t fontSize);  // 脏区字符串显示
void OLED_RefreshDirtyRegions(void);  // 只刷新脏区
void OLED_ClearDirtyRegions(void);    // 清除脏区记录
void OLED_AddDirtyRegion(uint8_t x, uint8_t y, uint8_t w, uint8_t h);  // 添加脏区

#endif // __OLED_DISPLAY_H__ 

