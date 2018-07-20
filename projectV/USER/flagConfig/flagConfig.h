#ifndef __FLAGCONFIG_H
#define __FLAGCONFIG_H	
#include "sys.h"
typedef struct
{
u8 startVmeasure;//开始测量速度
u8 stopVmeasure;//开始测量速度
u8 VmeasureFlag;//开始测量速度
u8 ExitFlag;//开始测量速度
u8 trigflag;
u8 trigselect;//外部触发 内部触发
u8 page;
u8 rate;
float trigvalue;
}measureFlag;
extern measureFlag _measureFlag;
void init__measureFlag(void);
#endif