#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 调试串口总开关，默认关闭，避免影响生产运行时序。 */
#define APP_DEBUG_UART_ENABLE          0
#define APP_DEBUG_UART_BUFFER_SIZE     256

/* 分模块诊断开关，后续联调时按需打开。 */
#define APP_DIAG_INPUT_ENABLE          0
#define APP_DIAG_RELAY_ENABLE          0
#define APP_DIAG_SAFETY_ENABLE         0
#define APP_DIAG_WATCHDOG_ENABLE       0

/* 继电器动作时序，依据原始需求固定为 500ms 脉冲和 500ms 反馈等待。 */
#define APP_RELAY_PULSE_MS             500U
#define APP_RELAY_FEEDBACK_WAIT_MS     500U

/* 输入采样确认参数：DC_CTRL 必须先于 K_EN 判定。 */
#define APP_DC_CONFIRM_PERIOD_MS       50U
#define APP_DC_CONFIRM_COUNT           3U
#define APP_KEN_CONFIRM_PERIOD_MS      150U
#define APP_KEN_CONFIRM_COUNT          3U

/* 蜂鸣器报警时序：紧急故障 100ms 响/100ms 停，一般故障 500ms 响/500ms 停。 */
#define APP_BEEP_URGENT_ON_MS          100U
#define APP_BEEP_URGENT_OFF_MS         100U
#define APP_BEEP_GENERAL_ON_MS         500U
#define APP_BEEP_GENERAL_OFF_MS        500U

/* 温度控制和温度异常确认参数。 */
#define APP_TEMP_FAN_HIGH_ON_C         35
#define APP_TEMP_FAN_LOW_ON_C          33
#define APP_TEMP_ALARM_SET_C           60
#define APP_TEMP_ALARM_CLEAR_C         58
#define APP_TEMP_CONFIRM_PERIOD_MS     500U
#define APP_TEMP_CONFIRM_COUNT         3U
#define APP_FAN_PWM_LOW_PERCENT        50U
#define APP_FAN_PWM_HIGH_PERCENT       95U

/* 看门狗策略参数：调试阶段默认关闭，生产阶段改为 1 启用。 */
#define APP_WATCHDOG_ENABLE            0
#define APP_WATCHDOG_TIMEOUT_MS        3000U
#define APP_WATCHDOG_FEED_PERIOD_MS    500U
#define APP_MAIN_LOOP_WARN_MS          50U
#define APP_MAIN_LOOP_MAX_MS           200U

/* 看门狗复位原因只作为诊断提示，不作为 A~O 业务异常。 */
#define APP_WATCHDOG_RESET_DIAG_ENABLE 1

/* 维护按键和日志参数：两颗按键仅用于日志串口操作，不参与蜂鸣器静音。 */
#define APP_MAINT_KEY_CONFIRM_MS       50U
#define APP_MAINT_KEY_LONG_MS          3000U
#define APP_LOG_ENABLE                 1
#define APP_LOG_MAX_RECORDS            2000U
#define APP_LOG_PAGE_SIZE              50U
#define APP_LOG_TIME_UNIT_MS           1000U

#ifdef __cplusplus
}
#endif

#endif /* __APP_CONFIG_H__ */
