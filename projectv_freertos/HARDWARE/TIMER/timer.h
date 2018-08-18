#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������
//��ʱ�� ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/3
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////   


extern u16 tim4flag;
extern u32 distancevalue;
extern float motorv;
/* �ṩ������C�ļ����õĺ��� */
void TIM3_Mode_Config(void);
void TIM4_Mode_Config(void);
void Enocode_start(void);
void Enocode_stop(void);
extern u32 Enocode_recieve(void);
u16 Read_ENC_Count(void);
void Reset_ENC_Count(void);



extern volatile unsigned long long FreeRTOSRunTimeTicks;
void ConfigureTimeForRunTimeStats(void);


#endif
