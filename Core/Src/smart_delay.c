/**
  ******************************************************************************
  * @file    smart_delay.c
  * @brief   ������ʱ����ʵ��
  *          ����ʱ�������Զ�ι�������׽�����Ź���λ����
  ******************************************************************************
  * @attention
  *
  * ������ʱϵͳʵ�֣�
  * 1. ������ʱ�ָ�Ϊ���С��ʱ��
  * 2. ��ÿ����ʱ��֮������ι��
  * 3. ����ϵͳ״̬�����Ƿ�ι��
  * 4. ������ʱ���Ⱥ�ϵͳ�ȶ���
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "smart_delay.h"
#include "iwdg.h"
#include "usart.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint32_t last_feed_time = 0;  // �ϴ�ι��ʱ��

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  ����Ƿ���Ҫι��
  * @retval 1:��Ҫι�� 0:����Ҫι��
  * @note   ���ݵ�ǰϵͳ״̬�ж��Ƿ���Ҫι��
  */
uint8_t SmartDelay_ShouldFeedWatchdog(void)
{
    SystemState_t current_state = SystemControl_GetState();
    
    // ������״̬������״̬���Լ�׶ζ���Ҫι��
    if(current_state == SYSTEM_STATE_NORMAL || 
       current_state == SYSTEM_STATE_ALARM ||
       current_state == SYSTEM_STATE_SELF_TEST ||
       current_state == SYSTEM_STATE_SELF_TEST_CHECK) {
        return 1;
    }
    
    // ����״̬�²�ι������ERROR״̬���ÿ��Ź���λϵͳ��
    return 0;
}

/**
  * @brief  ִ������ι������
  * @retval ��
  * @note   ����ϵͳ״̬���ܾ����Ƿ�ι��
  */
void SmartDelay_FeedWatchdog(void)
{
    uint32_t current_time = HAL_GetTick();
    
    // ����Ƿ���Ҫι��
    if(SmartDelay_ShouldFeedWatchdog()) {
        // ��ֹι������Ƶ��
        if(current_time - last_feed_time >= SMART_DELAY_MIN_FEED_TIME) {
            IWDG_Refresh();
            last_feed_time = current_time;
        }
    }
}

/**
  * @brief  ������ʱ�������Ƽ�ʹ�ã�
  * @param  delay_ms: ��ʱʱ�䣨���룩
  * @retval ��
  * @note   ����ʱ�����л����ϵͳ״̬�Զ�ι��
  */
void SmartDelay(uint32_t delay_ms)
{
    uint32_t remaining_time = delay_ms;
    uint32_t feed_interval = SMART_DELAY_FEED_INTERVAL_MS;
    
    // �����ʱʱ��̣ܶ�ֱ��ʹ��HAL_Delay
    if(delay_ms <= SMART_DELAY_MIN_FEED_TIME) {
        HAL_Delay(delay_ms);
        return;
    }
    
    // ����ʱ�����ж���ι��
    while(remaining_time > 0) {
        uint32_t current_delay = (remaining_time > feed_interval) ? feed_interval : remaining_time;
        
        // ִ����ʱ
        HAL_Delay(current_delay);
        remaining_time -= current_delay;
        
        // �������ʣ��ʱ�䣬����ι��
        if(remaining_time > 0) {
            SmartDelay_FeedWatchdog();
        }
    }
    
    // ��ʱ��������ιһ�ι�
    SmartDelay_FeedWatchdog();
}

/**
  * @brief  ��������Ϣ��������ʱ����
  * @param  delay_ms: ��ʱʱ�䣨���룩
  * @param  context: ��ʱ���������������ڵ��ԣ�
  * @retval ��
  * @note   ����ʱ�����л����������Ϣ���Զ�ι��
  */
void SmartDelayWithDebug(uint32_t delay_ms, const char* context)
{
    uint32_t remaining_time = delay_ms;
    uint32_t feed_interval = SMART_DELAY_FEED_INTERVAL_MS;
    uint32_t elapsed_time = 0;
    
    // ע��������ʱ������Ϣ�����ִ���������
    // DEBUG_Printf("[������ʱ] ��ʼ: %s, ��ʱ��: %lums\r\n", context, delay_ms);
    
    // �����ʱʱ��̣ܶ�ֱ��ʹ��HAL_Delay
    if(delay_ms <= SMART_DELAY_MIN_FEED_TIME) {
        HAL_Delay(delay_ms);
        // DEBUG_Printf("[������ʱ] ���: %s\r\n", context);
        return;
    }
    
    // ����ʱ�����ж���ι����ע�ͽ��������
    while(remaining_time > 0) {
        uint32_t current_delay = (remaining_time > feed_interval) ? feed_interval : remaining_time;
        
        // ִ����ʱ
        HAL_Delay(current_delay);
        remaining_time -= current_delay;
        elapsed_time += current_delay;
        
        // �������ʣ��ʱ�䣬����ι����ע�ͽ��������
        if(remaining_time > 0) {
            SmartDelay_FeedWatchdog();
            // uint32_t progress = (elapsed_time * 100) / delay_ms;
            // DEBUG_Printf("[������ʱ] %s: ���� %lu%% (%lums/%lums)\r\n", 
            //             context, progress, elapsed_time, delay_ms);
        }
    }
    
    // ��ʱ��������ιһ�ι�
    SmartDelay_FeedWatchdog();
    // DEBUG_Printf("[������ʱ] ���: %s, ����ʱ: %lums\r\n", context, delay_ms);
}

/**
  * @brief  ǿ��ι����ʱ���������ⳡ��ʹ�ã�
  * @param  delay_ms: ��ʱʱ�䣨���룩
  * @retval ��
  * @note   ����ϵͳ״̬��ζ���ι�������ڹؼ���ʼ���׶�
  */
void SmartDelayWithForceFeed(uint32_t delay_ms)
{
    uint32_t remaining_time = delay_ms;
    uint32_t feed_interval = SMART_DELAY_FEED_INTERVAL_MS;
    
    // �����ʱʱ��̣ܶ�ֱ��ʹ��HAL_Delay
    if(delay_ms <= SMART_DELAY_MIN_FEED_TIME) {
        HAL_Delay(delay_ms);
        return;
    }
    
    // ����ʱ������ǿ��ι��
    while(remaining_time > 0) {
        uint32_t current_delay = (remaining_time > feed_interval) ? feed_interval : remaining_time;
        
        // ִ����ʱ
        HAL_Delay(current_delay);
        remaining_time -= current_delay;
        
        // �������ʣ��ʱ�䣬ǿ��ι��
        if(remaining_time > 0) {
            uint32_t current_time = HAL_GetTick();
            if(current_time - last_feed_time >= SMART_DELAY_MIN_FEED_TIME) {
                IWDG_Refresh();  // ǿ��ι���������ϵͳ״̬
                last_feed_time = current_time;
            }
        }
    }
    
    // ��ʱ��������ǿ��ιһ�ι�
    IWDG_Refresh();
    last_feed_time = HAL_GetTick();
}

/**
  * @brief  ����ʱ��������ι����
  * @param  delay_ms: ��ʱʱ�䣨���룩
  * @retval ��
  * @note   ��ȫ��ͬ��HAL_Delay�����ڲ���Ҫι���ĳ���
  */
void SimpleDelay(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
} 


