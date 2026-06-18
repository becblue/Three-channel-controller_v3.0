#ifndef __BOARD_IO_H__
#define __BOARD_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 原始输入读取：只返回 MCU 引脚当前电平，不做消抖和业务判断。 */
uint8_t BoardIO_ReadK1EnRaw(void);
uint8_t BoardIO_ReadK2EnRaw(void);
uint8_t BoardIO_ReadK3EnRaw(void);

/* 继电器状态反馈读取：高电平表示对应反馈有效。 */
uint8_t BoardIO_ReadK1_1StaRaw(void);
uint8_t BoardIO_ReadK1_2StaRaw(void);
uint8_t BoardIO_ReadK2_1StaRaw(void);
uint8_t BoardIO_ReadK2_2StaRaw(void);
uint8_t BoardIO_ReadK3_1StaRaw(void);
uint8_t BoardIO_ReadK3_2StaRaw(void);

/* 接触器状态反馈读取：高电平表示对应接触器反馈有效。 */
uint8_t BoardIO_ReadSw1StaRaw(void);
uint8_t BoardIO_ReadSw2StaRaw(void);
uint8_t BoardIO_ReadSw3StaRaw(void);

/* 外部电源和风扇测速输入读取。 */
uint8_t BoardIO_ReadDcCtrlRaw(void);
uint8_t BoardIO_ReadFanSenseRaw(void);

/* 温度 ADC 原始值读取：返回 ADC DMA 缓冲区中的原始采样值。 */
uint16_t BoardIO_ReadNtc1AdcRaw(void);
uint16_t BoardIO_ReadNtc2AdcRaw(void);
uint16_t BoardIO_ReadNtc3AdcRaw(void);

/* 本地维护按键原始输入读取：按下为低电平。 */
uint8_t BoardIO_ReadKey1Raw(void);
uint8_t BoardIO_ReadKey2Raw(void);

/* 继电器驱动输出：原始硬件为低电平有效。 */
void BoardIO_SetK1_1On(uint8_t state);
void BoardIO_SetK1_1Off(uint8_t state);
void BoardIO_SetK1_2On(uint8_t state);
void BoardIO_SetK1_2Off(uint8_t state);
void BoardIO_SetK2_1On(uint8_t state);
void BoardIO_SetK2_1Off(uint8_t state);
void BoardIO_SetK2_2On(uint8_t state);
void BoardIO_SetK2_2Off(uint8_t state);
void BoardIO_SetK3_1On(uint8_t state);
void BoardIO_SetK3_1Off(uint8_t state);
void BoardIO_SetK3_2On(uint8_t state);
void BoardIO_SetK3_2Off(uint8_t state);

/* 报警、蜂鸣器、风扇和 RS485 方向输出。 */
void BoardIO_SetAlarmOutput(uint8_t active);
void BoardIO_SetBuzzerOutput(uint8_t active);
void BoardIO_SetFanPwmPercent(uint8_t percent);
void BoardIO_SetRs485Enable(uint8_t state);

/* PC15 当前保持空闲预留，后续可作为 1kHz 运行心跳输出。 */
void BoardIO_SetRunHeartbeatOutput(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_IO_H__ */
