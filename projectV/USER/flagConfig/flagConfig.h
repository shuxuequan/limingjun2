#ifndef __FLAGCONFIG_H
#define __FLAGCONFIG_H	
#include "sys.h"
typedef struct
{
u8 startVmeasure;//��ʼ�����ٶ�
u8 stopVmeasure;//��ʼ�����ٶ�
u8 VmeasureFlag;//��ʼ�����ٶ�
u8 ExitFlag;//��ʼ�����ٶ�
u8 trigflag;
u8 trigselect;//�ⲿ���� �ڲ�����
u8 page;
u8 rate;
float trigvalue;
}measureFlag;
extern measureFlag _measureFlag;
void init__measureFlag(void);
#endif