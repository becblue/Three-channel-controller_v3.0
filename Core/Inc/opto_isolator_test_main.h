#ifndef __OPTO_ISOLATOR_TEST_MAIN_H__
#define __OPTO_ISOLATOR_TEST_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/************************************************************
 * ��������·����������ͷ�ļ�
 * �ṩ�����Ĳ������̺��û���������
 * ����ͨ������������Ʋ��Թ���
 ************************************************************/

/**
  * @brief  ��������·�����������ʼ��
  * @retval ��
  */
void OptoTestMain_Init(void);

/**
  * @brief  �����ڽ��յ����ַ�
  * @param  ch: ���յ����ַ�
  * @retval ��
  */
void OptoTestMain_ProcessChar(char ch);

/**
  * @brief  �����û����������
  * @param  cmd: �����ַ���
  * @retval ��
  */
void OptoTestMain_ProcessCommand(char* cmd);

/**
  * @brief  ��ʾ��ϸ������Ϣ
  * @retval ��
  */
void OptoTestMain_ShowHelp(void);

/**
  * @brief  �Զ�����������
  * @retval ��
  */
void OptoTestMain_AutoTest(void);

/**
  * @brief  ��������·������ѭ��
  * @retval ��
  */
void OptoTestMain_Process(void);

/**
  * @brief  ����������·���������
  * @retval ��
  */
void OptoTestMain_CheckDesign(void);

#ifdef __cplusplus
}
#endif

#endif /* __OPTO_ISOLATOR_TEST_MAIN_H__ */ 