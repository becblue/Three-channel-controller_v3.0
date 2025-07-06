/**
  ******************************************************************************
  * @file    smart_delay.h
  * @brief   ������ʱ����ͷ�ļ�
  *          ����ʱ�������Զ�ι�������׽�����Ź���λ����
  ******************************************************************************
  * @attention
  *
  * ������ʱϵͳ�ص㣺
  * 1. ��ʱ�����ж���ι����Ĭ��ÿ100msһ�Σ�
  * 2. ����ԭ����ʱ����
  * 3. ֧�ֲ�ͬϵͳ״̬�µ�������Ϊ
  * 4. ��ȫ���HAL_Delay()���޷����
  *
  ******************************************************************************
  */

#ifndef __SMART_DELAY_H__
#define __SMART_DELAY_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "system_control.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define SMART_DELAY_FEED_INTERVAL_MS    100     // ι�������100ms
#define SMART_DELAY_MIN_FEED_TIME       50      // ��Сι��ʱ����

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes --------------------------------------------*/

/**
  * @brief  ������ʱ�������Ƽ�ʹ�ã�
  * @param  delay_ms: ��ʱʱ�䣨���룩
  * @retval ��
  * @note   ����ʱ�����л����ϵͳ״̬�Զ�ι��
  */
void SmartDelay(uint32_t delay_ms);

/**
  * @brief  ��������Ϣ��������ʱ����
  * @param  delay_ms: ��ʱʱ�䣨���룩
  * @param  context: ��ʱ���������������ڵ��ԣ�
  * @retval ��
  * @note   ����ʱ�����л����������Ϣ���Զ�ι��
  */
void SmartDelayWithDebug(uint32_t delay_ms, const char* context);

/**
  * @brief  ǿ��ι����ʱ���������ⳡ��ʹ�ã�
  * @param  delay_ms: ��ʱʱ�䣨���룩
  * @retval ��
  * @note   ����ϵͳ״̬��ζ���ι�������ڹؼ���ʼ���׶�
  */
void SmartDelayWithForceFeed(uint32_t delay_ms);

/**
  * @brief  ����ʱ��������ι����
  * @param  delay_ms: ��ʱʱ�䣨���룩
  * @retval ��
  * @note   ��ȫ��ͬ��HAL_Delay�����ڲ���Ҫι���ĳ���
  */
void SimpleDelay(uint32_t delay_ms);

/**
  * @brief  ����Ƿ���Ҫι��
  * @retval 1:��Ҫι�� 0:����Ҫι��
  * @note   ���ݵ�ǰϵͳ״̬�ж��Ƿ���Ҫι��
  */
uint8_t SmartDelay_ShouldFeedWatchdog(void);

/**
  * @brief  ִ������ι������
  * @retval ��
  * @note   ����ϵͳ״̬���ܾ����Ƿ�ι��
  */
void SmartDelay_FeedWatchdog(void);

#ifdef __cplusplus
}
#endif

#endif /* __SMART_DELAY_H__ */ 

