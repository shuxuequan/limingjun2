#include "timer.h"
#include "led.h"
#include "flagConfig.h"
/*
*********************************************************************************************************
*	函 数 名: TIM3_Mode_Config
*	功能说明: 初始化正交编码器捕获定时器3
*	形    参: 无
*	返 回 值: 
*********************************************************************************************************
*/
#define DIATOR 34 //测速仪半径

void TIM3_Mode_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;      
		NVIC_InitTypeDef NVIC_InitStructure;
	
    //PB4 ch1  A,PB5 ch2 
   
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//使能GPIOB时钟
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO , ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);//将GPIO_InitStruct中的参数按缺省值输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5;         
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//P	B4 PB5浮空输入  
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);  
	
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM3, ENABLE );
	
	GPIO_PinRemapConfig( GPIO_PartialRemap_TIM3, ENABLE );
	
    TIM_DeInit(TIM3);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;  //设定计数器重装值   TIMx_ARR = 359*4
    TIM_TimeBaseStructure.TIM_Prescaler = 0; //TIM3时钟预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision =TIM_CKD_DIV1 ;//设置时钟分割 T_dts = T_ck_int    
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM向上计数 
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);              

    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Falling,TIM_ICPolarity_Falling);//使用编码器模式3，上升下降都计数
    TIM_ICStructInit(&TIM_ICInitStructure);//将结构体中的内容缺省输入
    TIM_ICInitStructure.TIM_ICFilter = 6;  //选择输入比较滤波器 
    TIM_ICInit(TIM3, &TIM_ICInitStructure);//将TIM_ICInitStructure中的指定参数初始化TIM3

//  TIM_ARRPreloadConfig(TIM4, ENABLE);//使能预装载
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);//清除TIM3的更新标志位
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);//运行更新中断
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 

	TIM_SetCounter( TIM3, 0 );
    TIM_Cmd(TIM3, DISABLE);   //启动TIM3定时器

}

/*
*********************************************************************************************************
*	函 数 名: TIM4_Int_Init
*	功能说明: 初始化正交编码器捕获定时器4
*	形    参: u16 arr,u16 psc
*	返 回 值: 
*********************************************************************************************************
*/
static void TIM4_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); 

	TIM_TimeBaseStructure.TIM_Period = arr; 
	TIM_TimeBaseStructure.TIM_Prescaler =psc; 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); 
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);//清除TIM4的更新标志位 	
	TIM_ITConfig( 
		TIM4, 
		TIM_IT_Update , 
		ENABLE  
		);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 

	//TIM_Cmd(TIM4, ENABLE); 
							 
}


volatile unsigned long long FreeRTOSRunTimeTicks;
static void TIM2_Int_Init(u16 arr,u16 psc);

void ConfigureTimeForRunTimeStats(void)
{
	
	FreeRTOSRunTimeTicks=0;
	TIM2_Int_Init(50-1,8-1);	//3?ê??ˉTIM3
}

/*
*********************************************************************************************************
*	函 数 名: TIM2_Int_Init
*	功能说明: 初始化freertostimer2
*	形    参: u16 arr,u16 psc
*	返 回 值: 
*********************************************************************************************************
*/
static void TIM2_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); 

	TIM_TimeBaseStructure.TIM_Period = arr; 
	TIM_TimeBaseStructure.TIM_Prescaler =psc; 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); 
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);//清除TIM4的更新标志位 	
	TIM_ITConfig( 
		TIM2, 
		TIM_IT_Update , 
		ENABLE  
		);
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 

	TIM_Cmd(TIM2, ENABLE); 
							 
}


void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET) 
	{
		FreeRTOSRunTimeTicks++;
	}
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);  
}


static u16 countnew,countold,overflow;

u16 tim4flag;
u32 distancevalue=0;
static u32 trigtimer=0;

float motorv;
void TIM4_IRQHandler(void)   
{
	static u8 boot = 0;
	u32 finalcount,distance;
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update); 
		tim4flag=1;
		#if 0
		if(boot==0){
			GPIO_SetBits(GPIOA,GPIO_Pin_11);
			boot=1;
		}
		else{
			GPIO_ResetBits(GPIOA,GPIO_Pin_11);
			boot=0;
		}
		#endif
		//GPIO_SetBits(GPIOA,GPIO_Pin_11);
		finalcount = Enocode_recieve();
			
		//printf("脉冲  : %d\r\n",finalcount);
		distance=(((finalcount*10)*DIATOR*314)/800)/_measureFlag.rate;
		//printf("距离 : %d\r\n",finalcount);
			
		motorv= ((float)distance)/10000.0;
		//printf("速度 : %.3f\r\n",motorv);
		if((_measureFlag.page==1)||(_measureFlag.page==0)){
		if(_measureFlag.ExitFlag==1){
			_measureFlag.trigflag=1;
			if(_measureFlag.resulttrigvalueflag==1){
				_measureFlag.resulttrigvalueflag=0;
				_measureFlag.resulttrigvalue=motorv;
			}
			printf("时间:%d ms速度:%.3f\r\n",trigtimer,motorv);
				trigtimer=trigtimer+10;
				//_measureFlag.ExitFlag=0;
				distancevalue=distancevalue+distance;
			}
		}
		else if(_measureFlag.page==2){
			if(_measureFlag.trigselect==1){
				if(motorv>_measureFlag.trigvalue)
				{
					if(_measureFlag.resulttrigvalueflag==1){
						_measureFlag.resulttrigvalueflag=0;
						_measureFlag.resulttrigvalue=motorv;
						GPIO_ResetBits(GPIOA,GPIO_Pin_7);
						_measureFlag.trigflag=1;
					}
					if(_measureFlag.trigflag==1){
						printf("时间:%d ms速度:%.3f\r\n",trigtimer,motorv);
						trigtimer=trigtimer+10;
						//_measureFlag.ExitFlag=0;
						distancevalue=distancevalue+distance;
					}
				}
			}else if(_measureFlag.trigselect==2){
				if(_measureFlag.ExitFlag==1){
				if(_measureFlag.resulttrigvalueflag==1){
					_measureFlag.resulttrigvalueflag=0;
					_measureFlag.resulttrigvalue=motorv;
				}
				_measureFlag.trigflag=1;
				printf("时间:%d ms速度:%.3f\r\n",trigtimer,motorv);
				trigtimer=trigtimer+10;
				//_measureFlag.ExitFlag=0;
				distancevalue=distancevalue+distance;
			}
			}
			
		}

		//GPIO_ResetBits(GPIOA,GPIO_Pin_11);
		
	}
}


//编码器接口模式     优先级--2   1  1
void TIM3_IRQHandler(void)
{   
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  ); 
		
		overflow++;
		}   
}

/*
*********************************************************************************************************
*	函 数 名: TIM4_Mode_Config
*	功能说明: 初始化正交编码器通用定时器3
*	形    参: 无
*	返 回 值: 
*********************************************************************************************************
*/
void TIM4_Mode_Config(void)
{
	TIM4_Int_Init(999,79);
}

/*
*********************************************************************************************************
*	函 数 名: Read_ENC_Count
*	功能说明: 正交编码器值
*	形    参: 无
*	返 回 值: 
*********************************************************************************************************
*/
u16 Read_ENC_Count(void)
{
	return TIM_GetCounter( TIM3 );
}

/*
*********************************************************************************************************
*	函 数 名: Reset_ENC_Count
*	功能说明: 初始化正交编码器定时器
*	形    参: 无
*	返 回 值: 
*********************************************************************************************************
*/
void Reset_ENC_Count(void)
{
	TIM_SetCounter( TIM3, 0x0000 );
}

/*
*********************************************************************************************************
*	函 数 名: TIM4_Mode_Config
*	功能说明: 初始化正交编码器通用定时器3
*	形    参: 无
*	返 回 值: 
*********************************************************************************************************
*/
u32 Enocode_recieve(void)
{	
	u32 returnConunt;
	static u8 olddir,newdir;
	  //Reset counter
	  #if 1
   		countold=countnew;
		countnew=Read_ENC_Count();
		if(overflow>0){
			if(((TIM3->CR1) &TIM_CounterMode_Down)==TIM_CounterMode_Down){
			returnConunt=65536-countnew;
			}
				//printf("11111\r\n");
			}else{
			returnConunt=countnew;
			//printf("22222\r\n");
			}
		//if(overflow>1){
			//printf("333333\r\n");
				//returnConunt=returnConunt+(overflow-1)*65536;
			//}
		//printf("overflow  : %d\r\n",overflow);
	#elif 0
		countold=countnew;
		countnew=Read_ENC_Count();
		if(((TIM3->CR1) &TIM_CounterMode_Down)==TIM_CounterMode_Down){
			if(overflow>0){
				returnConunt=65536-countold+65536-countnew;
				printf("11111\r\n");
			}else{
				if(countold>countnew){
					printf("22222\r\n");
					returnConunt=countold-countnew;
				}else{
					printf("33333\r\n");
					returnConunt=countnew-countold;
				}
			}
		}else{
			if(overflow>0){
				printf("44444\r\n");
				returnConunt=countold+countnew;
			}else{
				if(countold>countnew){
					printf("55555\r\n");
					returnConunt=countold-countnew;
				}else{
				printf("66666\r\n");
					returnConunt=countnew-countold;
				}
			 }
			}
		//returnConunt=count1*0.3+count2*0.7;
		if(overflow>1){
			returnConunt=returnConunt+(overflow-1)*65536;
		}
		printf("overflow  : %d\r\n",overflow);
	#endif
		Reset_ENC_Count();
		overflow=0;
	
	return returnConunt;
}

/*
*********************************************************************************************************
*	函 数 名: TIM4_Mode_Config
*	功能说明: 初始化正交编码器通用定时器3
*	形    参: 无
*	返 回 值: 
*********************************************************************************************************
*/
void Enocode_start(void)
{
	  //Reset counter
   	Reset_ENC_Count();
    overflow =0 ;
	tim4flag=0;
	distancevalue=0;
	trigtimer=0;
	motorv=0;
	_measureFlag.ExitFlag=0;
	TIM_Cmd(TIM4, ENABLE); 
	TIM_Cmd(TIM3, ENABLE); 
}
/*
*********************************************************************************************************
*	函 数 名: TIM4_Mode_Config
*	功能说明: 初始化正交编码器通用定时器3
*	形    参: 无
*	返 回 值: 
*********************************************************************************************************
*/
void Enocode_stop(void)
{
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);//清除TIM3的更新标志位 
	TIM_ClearFlag(TIM4, TIM_FLAG_Update);//清除TIM4的更新标志位 
	TIM_Cmd(TIM4, DISABLE); 
	TIM_Cmd(TIM3, DISABLE); 
	GPIO_SetBits(GPIOA,GPIO_Pin_7);
}


