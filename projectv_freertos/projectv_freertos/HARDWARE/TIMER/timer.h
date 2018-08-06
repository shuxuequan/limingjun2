#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//定时器 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/3
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////   


extern u16 tim4flag;
extern u32 distancevalue;
extern float motorv;
/* 提供给其他C文件调用的函数 */
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
