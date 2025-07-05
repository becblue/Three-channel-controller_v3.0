#ifndef __OPTO_ISOLATOR_TEST_H__
#define __OPTO_ISOLATOR_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * ��������·����ģ��ͷ�ļ�
 * ר�����ڲ��Ժ����K1_EN��K2_EN��K3_EN�źŵĹ�������·����״̬
 * �ṩ�����Ĳ��ԡ���ء���Ϻ��Ż����鹦��
 ************************************************************/

/**
  * @brief  ��ʼ����������·����ģ��
  * @retval ��
  */
void OptoTest_Init(void);

/**
  * @brief  ��ʼ��������·����
  * @param  test_duration_sec: ���Գ���ʱ��(��)
  * @retval ��
  */
void OptoTest_Start(uint16_t test_duration_sec);

/**
  * @brief  ֹͣ��������·����
  * @retval ��
  */
void OptoTest_Stop(void);

/**
  * @brief  ��������·������ѭ��������ѭ���е��ã�
  * @retval ��
  */
void OptoTest_Process(void);

/**
  * @brief  ���ʵʱ״̬��Ϣ
  * @retval ��
  */
void OptoTest_PrintRealTimeStatus(void);

/**
  * @brief  �����ϸ���Խ��
  * @retval ��
  */
void OptoTest_PrintResults(void);

/**
  * @brief  �ֶ���������״̬���
  * @retval ��
  */
void OptoTest_ManualCheck(void);

/**
  * @brief  ��ȡ��ǰ����״̬
  * @retval 1:���������� 0:����ֹͣ
  */
uint8_t OptoTest_IsRunning(void);

/**
  * @brief  ��ȡ���Ծ���ʱ��
  * @retval ���Ծ���ʱ��(����)
  */
uint32_t OptoTest_GetElapsedTime(void);

#ifdef __cplusplus
}
#endif

#endif /* __OPTO_ISOLATOR_TEST_H__ */ 