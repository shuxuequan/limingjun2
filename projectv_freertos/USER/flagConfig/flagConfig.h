#ifndef __FLAGCONFIG_H
#define __FLAGCONFIG_H	
#include "sys.h"
typedef struct
{
u8 startVmeasure;//��ʼ�����ٶ� 1��ʼ 0�����ҿ��Դ�ӡ
u8 startprint;//1��ʼ 0�����ҿ��Դ�ӡ
u8 resulttrigvalueflag;//�ж��д����ٶ�flagһ��
u8 trigreportflag;//��ʾ�ϱ������ٶ�flagһ��
u8 ExitFlag;//�ⲿ����
u8 trigflag;//������ʼ��ʾ����
u8 trigselect;//�ⲿ���� �ڲ�����
u8 page;
u8 rate;
float trigvalue;//�趨�ڲ������ٶ�
float resulttrigvalue;//�����ٶ�
float movevalue;//�ƶ�����
}measureFlag;


extern measureFlag _measureFlag;
void init__measureFlag(void);

#endif




