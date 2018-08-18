#ifndef __FLAGCONFIG_H
#define __FLAGCONFIG_H	
#include "sys.h"
typedef struct
{
u8 startVmeasure;//开始测量速度 1开始 0结束且可以打印
u8 startprint;//1开始 0结束且可以打印
u8 resulttrigvalueflag;//中断中触发速度flag一次
u8 trigreportflag;//显示上报触发速度flag一次
u8 ExitFlag;//外部触发
u8 trigflag;//触发开始显示距离
u8 trigselect;//外部触发 内部触发
u8 page;
u8 rate;
float trigvalue;//设定内部触发速度
float resulttrigvalue;//触发速度
float movevalue;//移动距离
}measureFlag;


extern measureFlag _measureFlag;
void init__measureFlag(void);

#endif




