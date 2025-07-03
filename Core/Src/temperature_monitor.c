#include "temperature_monitor.h"
#include "adc.h"           // 用于ADC采集接口
#include "tim.h"           // 用于风扇PWM控制
#include "gpio_control.h"  // 用于蜂鸣器/报警等控制
#include "usart.h"         // 用于DEBUG_Printf调试输出
#include <stdio.h>

// ================== NTC查表法参数 ===================
// 采用NTC热敏电阻RT值.csv数据，温度(℃)与阻值(kΩ)对照表
// 假设上拉电阻10kΩ，3.3V供电
static const struct {
    float temp;      // 温度(℃)
    float r_ntc;     // NTC阻值(kΩ)
} ntc_table[] = {
    {-40,197.39},{-39,186.54},{-38,176.35},{-37,166.8},{-36,157.82},{-35,149.39},{-34,141.51},{-33,134.09},{-32,127.11},{-31,120.53},
    {-30,114.34},{-29,108.53},{-28,103.04},{-27,97.87},{-26,92.989},{-25,88.381},{-24,84.036},{-23,79.931},{-22,76.052},{-21,72.384},
    {-20,68.915},{-19,65.634},{-18,62.529},{-17,59.589},{-16,56.804},{-15,54.166},{-14,51.665},{-13,49.294},{-12,47.046},{-11,44.913},
    {-10,42.889},{-9,40.967},{-8,39.142},{-7,37.408},{-6,35.761},{-5,34.196},{-4,32.707},{-3,31.291},{-2,29.945},{-1,28.664},
    {0,27.445},{1,26.283},{2,25.177},{3,24.124},{4,23.121},{5,22.165},{6,21.253},{7,20.384},{8,19.555},{9,18.764},
    {10,18.01},{11,17.29},{12,16.602},{13,15.946},{14,15.319},{15,14.72},{16,14.148},{17,13.601},{18,13.078},{19,12.578},
    {20,12.099},{21,11.642},{22,11.204},{23,10.785},{24,10.384},{25,10},{26,9.632},{27,9.28},{28,8.943},{29,8.619},
    {30,8.309},{31,8.012},{32,7.727},{33,7.453},{34,7.191},{35,6.939},{36,6.698},{37,6.466},{38,6.243},{39,6.029},
    {40,5.824},{41,5.627},{42,5.437},{43,5.255},{44,5.08},{45,4.911},{46,4.749},{47,4.593},{48,4.443},{49,4.299},
    {50,4.16},{51,4.027},{52,3.898},{53,3.774},{54,3.654},{55,3.539},{56,3.429},{57,3.322},{58,3.219},{59,3.119},
    {60,3.024},{61,2.931},{62,2.842},{63,2.756},{64,2.673},{65,2.593},{66,2.516},{67,2.441},{68,2.369},{69,2.3},
    {70,2.233},{71,2.168},{72,2.105},{73,2.044},{74,1.986},{75,1.929},{76,1.874},{77,1.821},{78,1.77},{79,1.72},
    {80,1.673},{81,1.626},{82,1.581},{83,1.538},{84,1.496},{85,1.455},{86,1.416},{87,1.377},{88,1.34},{89,1.304},
    {90,1.27},{91,1.236},{92,1.204},{93,1.172},{94,1.141},{95,1.112},{96,1.083},{97,1.055},{98,1.028},{99,1.002},
    {100,0.976},{101,0.951},{102,0.927},{103,0.904},{104,0.882},{105,0.86},{106,0.838},{107,0.818},{108,0.798},{109,0.778},
    {110,0.759},{111,0.741},{112,0.723},{113,0.706},{114,0.689},{115,0.673},{116,0.657},{117,0.641},{118,0.626},{119,0.612},
    {120,0.598},{121,0.584},{122,0.57},{123,0.557},{124,0.545},{125,0.532}
};
#define NTC_TABLE_SIZE (sizeof(ntc_table)/sizeof(ntc_table[0]))

#define NTC_PULLUP_RES 10.0f // 上拉电阻10kΩ
#define NTC_VREF 3.3f        // 参考电压3.3V

#define TEMP_HIGH_TH   35.0f   // 高温阈值(℃)
#define TEMP_OVER_TH   60.0f   // 超温阈值(℃)
#define TEMP_HYST      2.0f    // 回差(℃)
#define FAN_PWM_NORMAL 50      // 正常温度风扇占空比(%)
#define FAN_PWM_HIGH   95      // 高温风扇占空比(%)

static TempInfo_t tempInfos[TEMP_CH_MAX];
static TempState_t lastStates[TEMP_CH_MAX];
static uint8_t fan_pwm = FAN_PWM_NORMAL;

// ================== 风扇转速测量相关 ===================
#define FAN_SEN_PULSE_PER_REV 2 // 每转脉冲数（根据风扇规格调整）
volatile uint32_t fan_pulse_count = 0; // 1秒内脉冲计数
uint32_t fan_rpm = 0; // 当前风扇转速(RPM)

// ================== ADC DMA采样相关 ===================
uint16_t adc_dma_buf[TEMP_CH_MAX] = {0}; // DMA采样缓冲区

// ================== 内部工具函数 ===================

// 由ADC采集电压计算NTC阻值（单位：kΩ）
static float CalcNTCRes(float voltage)
{
    if(voltage <= 0.0f) return 1e6f; // 防止分母为0
    return NTC_PULLUP_RES * voltage / (NTC_VREF - voltage);
}

// 查表法：由NTC阻值反查温度（支持插值）
static float NTC_ResToTemp(float r_ntc)
{
    if(r_ntc >= ntc_table[0].r_ntc) return ntc_table[0].temp;
    for(int i=1; i<NTC_TABLE_SIZE; ++i) {
        if(r_ntc >= ntc_table[i].r_ntc) {
            float t1 = ntc_table[i-1].temp, r1 = ntc_table[i-1].r_ntc;
            float t2 = ntc_table[i].temp, r2 = ntc_table[i].r_ntc;
            // 线性插值
            return t1 + (r_ntc - r1) * (t2 - t1) / (r2 - r1);
        }
    }
    return ntc_table[NTC_TABLE_SIZE-1].temp;
}

// ================== 接口实现 ===================

// 初始化温度监控模块
void TemperatureMonitor_Init(void)
{
    for (int i = 0; i < TEMP_CH_MAX; ++i) {
        tempInfos[i].value_celsius = 25.0f;
        tempInfos[i].state = TEMP_STATE_NORMAL;
        lastStates[i] = TEMP_STATE_NORMAL;
    }
    fan_pwm = FAN_PWM_NORMAL;
    // 启动ADC DMA采集
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buf, TEMP_CH_MAX);
    DEBUG_Printf("[温度监控] 初始化完成，已启动ADC DMA采集\r\n");
}

// ADC DMA采集完成回调
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1) {
        // 采集完成回调，但不需要在这里处理，因为UpdateAll()会直接处理DMA缓冲区
    }
}

// 采集所有NTC温度并更新状态（建议定时调用）
void TemperatureMonitor_UpdateAll(void)
{
    // 直接处理DMA缓冲区，无需等待标志
    for (int i = 0; i < TEMP_CH_MAX; ++i) {
        float voltage = adc_dma_buf[i] * NTC_VREF / 4095.0f;
        float r_ntc = CalcNTCRes(voltage); // 先算阻值
        float temp = NTC_ResToTemp(r_ntc); // 再查表反查温度
        tempInfos[i].value_celsius = temp;
        // 状态判定（带回差）
        TempState_t prev = tempInfos[i].state;
        if (prev == TEMP_STATE_NORMAL) {
            if (temp >= TEMP_OVER_TH) tempInfos[i].state = TEMP_STATE_OVERHEAT;
            else if (temp >= TEMP_HIGH_TH) tempInfos[i].state = TEMP_STATE_HIGH;
        } else if (prev == TEMP_STATE_HIGH) {
            if (temp >= TEMP_OVER_TH) tempInfos[i].state = TEMP_STATE_OVERHEAT;
            else if (temp < (TEMP_HIGH_TH - TEMP_HYST)) tempInfos[i].state = TEMP_STATE_NORMAL;
        } else if (prev == TEMP_STATE_OVERHEAT) {
            if (temp < (TEMP_OVER_TH - TEMP_HYST)) {
                if (temp >= TEMP_HIGH_TH) tempInfos[i].state = TEMP_STATE_HIGH;
                else tempInfos[i].state = TEMP_STATE_NORMAL;
            }
        }
        // 状态变化调试输出
        if (tempInfos[i].state != lastStates[i]) {
            DEBUG_Printf("[温度监控] 通道%d 状态变化: %.1f℃ %d->%d\r\n", i+1, temp, lastStates[i], tempInfos[i].state);
            lastStates[i] = tempInfos[i].state;
        }
    }
    // 联动风扇PWM
    uint8_t max_state = TEMP_STATE_NORMAL;
    for (int i = 0; i < TEMP_CH_MAX; ++i) {
        if (tempInfos[i].state > max_state) max_state = tempInfos[i].state;
    }
    if (max_state == TEMP_STATE_OVERHEAT || max_state == TEMP_STATE_HIGH) {
        fan_pwm = FAN_PWM_HIGH;
    } else {
        fan_pwm = FAN_PWM_NORMAL;
    }
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim3);
    uint32_t ccr = (fan_pwm * (arr + 1)) / 100;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr);
}

// 获取指定通道的温度信息
TempInfo_t TemperatureMonitor_GetInfo(TempChannel_t channel)
{
    if (channel >= TEMP_CH_MAX) {
        TempInfo_t err = {0, TEMP_STATE_OVERHEAT};
        return err;
    }
    return tempInfos[channel];
}

// 获取所有通道的温度状态
void TemperatureMonitor_GetAllStates(TempState_t states[TEMP_CH_MAX])
{
    for (int i = 0; i < TEMP_CH_MAX; ++i) {
        states[i] = tempInfos[i].state;
    }
}

// 设置风扇PWM占空比（0~100）
void TemperatureMonitor_SetFanPWM(uint8_t duty)
{
    if (duty > 100) duty = 100;
    fan_pwm = duty;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim3);
    uint32_t ccr = (fan_pwm * (arr + 1)) / 100;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr);
}

// 查询风扇当前PWM占空比
uint8_t TemperatureMonitor_GetFanPWM(void)
{
    return fan_pwm;
}

// 检查并处理温度异常（如KLM标志、回差等）
void TemperatureMonitor_CheckAndHandleAlarm(void)
{
    for (int i = 0; i < TEMP_CH_MAX; ++i) {
        if (tempInfos[i].state == TEMP_STATE_OVERHEAT) {
            // 触发K/L/M异常（可调用异常管理接口）
            DEBUG_Printf("[温度异常] 通道%d 超温报警!\r\n", i+1);
            // 这里可联动报警输出、蜂鸣器等
        }
    }
}

// 1秒定时器回调（建议在定时中断或主循环中每秒调用一次）
void TemperatureMonitor_FanSpeed1sHandler(void)
{
    // 计算风扇转速
    fan_rpm = (fan_pulse_count * 60) / FAN_SEN_PULSE_PER_REV;
    fan_pulse_count = 0;
}

// 获取当前风扇转速信息
FanSpeedInfo_t TemperatureMonitor_GetFanSpeed(void)
{
    FanSpeedInfo_t info;
    info.rpm = fan_rpm;
    info.pulse_count = fan_pulse_count;
    return info;
}

// 串口调试输出当前温度和风扇状态
void TemperatureMonitor_DebugPrint(void)
{
    for (int i = 0; i < TEMP_CH_MAX; ++i) {
        float voltage = adc_dma_buf[i] * NTC_VREF / 4095.0f;
        float r_ntc = CalcNTCRes(voltage);
        float temp = NTC_ResToTemp(r_ntc);
        DEBUG_Printf("[温度监控] 通道%d: ADC原始值=%d, 电压=%.3fV, 温度=%.1f℃ 状态=%d\r\n",
            i+1, adc_dma_buf[i], voltage, temp, tempInfos[i].state);
    }
    DEBUG_Printf("[温度监控] 风扇PWM: %d%%\r\n", fan_pwm);
    DEBUG_Printf("[温度监控] 风扇转速: %d RPM\r\n", fan_rpm);
} 

