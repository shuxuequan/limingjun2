#include "sys.h"
#include "delay.h"
#include "flagConfig.h"
#include "led.h"
#include "timer.h"
#include "lcd.h"
#include "key.h"
#include "malloc.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "exti.h"
#include "timers.h"
#include "semphr.h"
#include "24cxx.h"	 
#include "stmflash.h"
/************************************************
 ALIENTEK Mini STM32F103开发板 FreeRTOS实验15-1
 FreeRTOS软件定时器实验-库函数版本
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

//任务优先级
#define START_TASK_PRIO			1
//任务堆栈大小	
#define START_STK_SIZE 			36
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

//任务优先级
#define TIMERCONTROL_TASK_PRIO	2
//任务堆栈大小	
#define TIMERCONTROL_STK_SIZE 	128  
//任务句柄
TaskHandle_t TimerControlTask_Handler;
//任务函数
void timercontrol_task(void *pvParameters);

//任务优先级
#define PROCESS_TASK_PRIO	3
//任务堆栈大小	
#define PROCESS_STK_SIZE 	512  
//任务句柄
TaskHandle_t ProcessTask_Handler;
//任务函数
void process_task(void *pvParameters);
const u8 TEXT_Buffer[]={"WarShipSTM32 IIC TEST"};
#define SIZE sizeof(TEXT_Buffer)
#define FLASH_SAVE_ADDR  0X0801FC00		//设置FLASH 保存地址(必须为偶数，且其值要大于本代码所占用FLASH的大小+0X08000000)
////////////////////////////////////////////////////////
TimerHandle_t 	AutoReloadTimer_Handle;			//周期定时器句柄
TimerHandle_t	OneShotTimer_Handle;			//单次定时器句柄

void AutoReloadCallback(TimerHandle_t xTimer); 	//周期定时器回调函数
void OneShotCallback(TimerHandle_t xTimer);		//单次定时器回调函数


static u8 blConnectflag; // 1连上 0未连上

//二值信号量句柄
SemaphoreHandle_t BinarySemaphore;	//二值信号量句柄

int Slen(char s[]) {
	 int len = 0;
	 while(s[len]) ++len;
	 return len;
 }
  
 int Include(char s[],char c) {
	 int i = 0;
	 while(s[i]) {
		 if(s[i] == c) return 1;
		 ++i;
	 }
	 return 0;
 }
  
 char *Float2Txt(float a,char s[])	{
	 sprintf(s,"%.2f",a);
	 return s;
 }
  
 float Txt2Float(char s[]) {
	 float a = 0,numi = 0,numf = 0,mult = 1.0f;
	 int i = 0,flag = 0;
	 int len = Slen(s);
	 i = len - 1;
	 flag = Include(s,'.');
	 if(flag) {// 包含小数点 
		 while(s[i] != '.') {// 小数部分 
			 numf = numf/10.0f + s[i] - '0';
			 --i;
		 }
		 numf = numf/10.0f;
		 --i;
		 while(s[i]) {// 整数部分 
			 numi = 10 * numi + s[i] - '0';
			 --i;
		 }
	 }
	 else {
		 i = 0; 
		 while(s[i]) {//不包含小数点 
			 numi = 10 * numi + s[i] - '0';
			 ++i;
		 }
	 }
  
	 return numi + numf;
 }

static void bluetooth_AT(uint8_t * string){
	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	delay_ms(10);
	
	usart2_send_string(USART2,(uint8_t *)string, strlen(string));
	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	
}

static void bluetooth_print_init(){
		bluetooth_AT("AT+PSWD=0000\r\n");
		delay_ms(100);
}

static u8 isbluetoothConnected(){
		bluetooth_AT("AT+STATE ?\r\n");
		delay_ms(100);
		
}


static void bluetoothConnect(){
		bluetooth_AT("AT+LINK=020E,3A,D0801E\r\n");
}

int main(void)
{
	static u8 datatemp[SIZE];
	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(9600);	 //串口初始化为115200
	uart2_init(9600);
	TIM3_Mode_Config();
	TIM4_Mode_Config();
	//Enocode_start();
	bluetooth_print_init();
	init__measureFlag();
	EXTIX_Init();
	//AT24CXX_Init();			//IIC初始化
	LED_Init();			     //LED端口初始化
#if 0	
	do{
		uart2_dev.USART_RX_STA=0;
		GPIO_SetBits(GPIOA,GPIO_Pin_4);
		delay_ms(10);	
		printf("AT+PSWD?\r\n");
		GPIO_ResetBits(GPIOA,GPIO_Pin_4);
		delay_ms(100);

		uart2_dev.USART_RX_STA=0;
		GPIO_SetBits(GPIOA,GPIO_Pin_4);
		delay_ms(10);	
		printf("AT+LINK=020E,3A,D0801E\r\n");
		GPIO_ResetBits(GPIOA,GPIO_Pin_4);
		delay_ms(1000);
		printf("1234567\r\r\n");
		delay_ms(100);
	
	}while(0);
	
#endif	
	//AT24CXX_Write(0,(u8*)TEXT_Buffer,SIZE);
	//AT24CXX_Read(0,datatemp,SIZE);
	//STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)TEXT_Buffer,SIZE);
	STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)datatemp,SIZE);
	//创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
		 			
}

//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
    //创建软件周期定时器
    AutoReloadTimer_Handle=xTimerCreate((const char*		)"AutoReloadTimer",
									    (TickType_t			)1000,
							            (UBaseType_t		)pdTRUE,
							            (void*				)1,
							            (TimerCallbackFunction_t)AutoReloadCallback); //周期定时器，周期1s(1000个时钟节拍)，周期模式
    //创建单次定时器
	OneShotTimer_Handle=xTimerCreate((const char*			)"OneShotTimer",
							         (TickType_t			)2000,
							         (UBaseType_t			)pdFALSE,
							         (void*					)2,
							         (TimerCallbackFunction_t)OneShotCallback); //单次定时器，周期2s(2000个时钟节拍)，单次模式
	//创建二值信号量
	BinarySemaphore=xSemaphoreCreateBinary();									 
    //创建定时器控制任务
    xTaskCreate((TaskFunction_t )timercontrol_task,             
                (const char*    )"timercontrol_task",           
                (uint16_t       )TIMERCONTROL_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )TIMERCONTROL_TASK_PRIO,        
                (TaskHandle_t*  )&TimerControlTask_Handler); 

    xTaskCreate((TaskFunction_t )process_task,            //任务函数
                (const char*    )"process_task",          //任务名称
                (uint16_t       )PROCESS_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )PROCESS_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&ProcessTask_Handler);   //任务句柄

	xTimerStart(AutoReloadTimer_Handle,0);
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
    
}


void process_task(void *pvParameters)
{	
	u32 distancevaluedisp=0;
	float motorvdisp;
	u8 trigvalue[30];
	u8 i=0;
	while(1)
		{
			static u8 boot = 0;
			if(boot==0){
				GPIO_SetBits(GPIOA,GPIO_Pin_8);
				boot=1;
			}
			else{
				GPIO_ResetBits(GPIOA,GPIO_Pin_8);
				boot=0;
			}
			if((uart1_dev.USART_RX_STA)&0x8000)
			{					   
				//len=USART_RX_STA&0x3fff;
				for(i=0;i<36;i++){
					if(uart1_dev.USART_RX_BUF[i]==0x9a)
						break;
				}
				switch(uart1_dev.USART_RX_BUF[i])
				{
					case 0x9a:
						switch(uart1_dev.USART_RX_BUF[i+1]){
							case 0xaa://start
							Enocode_start();
							{
								char temp[36] = "t3.txt=\"";
								char buf[10];
								char end[3]={0xff,0xff,0xff};
								u8 i=0;
	
								sprintf(buf, "%d", distancevaluedisp/1000);
								strcat(temp, buf);
								strcat(temp, "\"");
								strcat(temp, end);
								usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
	
							}
							{
								char temp[36] = "t2.txt=\"";
								char buf[10];
								char end[3]={0xff,0xff,0xff};
								u8 i=0;
	
								sprintf(buf, "%.3f", motorvdisp);
								strcat(temp, buf);
								strcat(temp, "\"");
								strcat(temp, end);
								usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
	
							}
							_measureFlag.startVmeasure=1;
							break;
							case 0xaf://stop
							_measureFlag.startVmeasure=0;
							_measureFlag.trigflag=0;
							distancevaluedisp=0;
							motorvdisp=0;
							Enocode_stop();
							break;
							case 0xc1://曳引比
								switch(uart1_dev.USART_RX_BUF[i+2]){
									case 0xa1:
										_measureFlag.rate=1;
										break;
									case 0xa2:
										_measureFlag.rate=2;
										break;
									case 0xa3:
										_measureFlag.rate=4;
										break;
									case 0xa4:
										_measureFlag.rate=8;
										break;
								}
								_measureFlag.page=1;
							
							break;
							case 0xc2:
								switch(uart1_dev.USART_RX_BUF[i+2]){
									case 0xd1:
										_measureFlag.trigselect=1;
										break;
									case 0xd2:
										_measureFlag.trigselect=2;
										break;
									}	
								switch(uart1_dev.USART_RX_BUF[i+3]){
									case 0xa1:
										_measureFlag.rate=1;
										break;
									case 0xa2:
										_measureFlag.rate=2;
										break;
									case 0xa3:
										_measureFlag.rate=4;
										break;
									case 0xa4:
										_measureFlag.rate=8;
										break;
								}
								_measureFlag.page=2;
								{
									u8 j=0;
									for(j=0;j<30;j++){
										trigvalue[j]=uart1_dev.USART_RX_BUF[i+4];
										if(trigvalue[j]==0x00)break;
										i++;
									}
									
								}
								_measureFlag.trigvalue=Txt2Float(trigvalue);
							break;
							
						}
					break;	
				}
				
				(uart1_dev.USART_RX_STA)=0;
			}
		#if 1	
			if(_measureFlag.startVmeasure==1){
				//if(tim4flag==1){
					//tim4flag=0;
				
				motorvdisp=motorv;
				//printf("速度 : %.3f\r\n",motorv);
				{
					char temp[36] = "t2.txt=\"";
					char buf[10];
					char end[3]={0xff,0xff,0xff};
					u8 i=0;
					
					sprintf(buf, "%.3f", motorvdisp);
					strcat(temp, buf);
					strcat(temp, "\"");
					strcat(temp, end);
					usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					
				}
				if(_measureFlag.trigflag==1){
					distancevaluedisp=distancevalue;
					{
					char temp[36] = "t3.txt=\"";
					char buf[10];
					char end[3]={0xff,0xff,0xff};
					u8 i=0;
					
					sprintf(buf, "%.4f", (float)(distancevaluedisp)/1000000.0);
					strcat(temp, buf);
					strcat(temp, "\"");
					strcat(temp, end);
					usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					if(motorvdisp==0){
						_measureFlag.startVmeasure=0;
						_measureFlag.trigflag=0;
						distancevaluedisp=0;
						motorvdisp=0;
						Enocode_stop();
					}
					
				}
				}	
			#if 0
				if(_measureFlag.ExitFlag==1){
					printf("速度 : %.3f\r\n",motorv);
					//_measureFlag.ExitFlag=0;
					distancevalue=distancevalue+distance;
					{
					char temp[36] = "t3.txt=\"";
					char buf[10];
					char end[3]={0xff,0xff,0xff};
					u8 i=0;
					
					sprintf(buf, "%d", distancevalue/1000);
					strcat(temp, buf);
					strcat(temp, "\"");
					strcat(temp, end);
					usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					if(motorv==0){
					_measureFlag.startVmeasure=0;
					_measureFlag.ExitFlag=0;
					distancevalue=0;
					motorv=0;
					Enocode_stop();
				}
					
				}
				}
		#endif
				//}
			}
			
		#endif
			
			vTaskDelay(20);
			//printf("123243424!!!!!\r\n");
		}


}


//TimerControl的任务函数
void timercontrol_task(void *pvParameters)
{
	static u8 test,test1;
	u8 count=2;
	u8 key,num;
	BaseType_t err=pdFALSE;
	while(1)
	{
			
			if(1==Bluetooth_Detect()){
				test=0;
			}else{
				test=1;
			}
			vTaskDelay(10);
			if((test==0)&&(1==Bluetooth_Detect())){
				blConnectflag=1;
 			}else{
 				count=2;
 				blConnectflag=0;
				//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启中断
				vTaskDelay(100);
				//while(count--){
					uart2_dev.USART_RX_STA=0;
					memset(uart2_dev.USART_RX_BUF,0x00,USART_REC_LEN);
	 				bluetoothConnect();
	 				//if(BinarySemaphore!=NULL)
	 				//{
	 					//获取信号量成功
						//err=xSemaphoreTake(BinarySemaphore,0xffff);	//获取信号量
	 					//if(err==pdTRUE)	{
							//if(strcmp(uart2_dev.USART_RX_BUF,"OK")==0){
								//test1=1;
								//break;
							//}
	 					//}
	 				//}
				//}
				//USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);//关闭中断
			}
			
			vTaskDelay(2000); //延时10ms，也就是10个时钟节拍
		}
	#if 0	
		//只有两个定时器都创建成功了才能对其进行操作
		if((AutoReloadTimer_Handle!=NULL)&&(OneShotTimer_Handle!=NULL))
		{
			key = KEY_Scan(0);
			switch(key)
			{
				case WKUP_PRES:     //当key_up按下的话打开周期定时器
					xTimerStart(AutoReloadTimer_Handle,0);	//开启周期定时器
					//printf("开启定时器1\r\n");
					break;
				case KEY0_PRES:		//当key0按下的话打开单次定时器
					xTimerStart(OneShotTimer_Handle,0);		//开启单次定时器
					//printf("开启定时器2\r\n");
					break;
				case KEY1_PRES:		//当key1按下话就关闭定时器
					xTimerStop(AutoReloadTimer_Handle,0); 	//关闭周期定时器
					xTimerStop(OneShotTimer_Handle,0); 		//关闭单次定时器
					//printf("关闭定时器1和2\r\n");
					break;	
			}
		}
		num++;
		if(num==50) 	//每500msLED0闪烁一次
		{
			num=0;
			LED0=!LED0;	
		}
        vTaskDelay(10); //延时10ms，也就是10个时钟节拍
	}
	#endif	
	



}

//周期定时器的回调函数
void AutoReloadCallback(TimerHandle_t xTimer)
{
	static u8 test;
	//if(BinarySemaphore!=NULL){
		//xSemaphoreGive(BinarySemaphore);	//释放二值信号量
	//}
	
	
	
}

//单次定时器的回调函数
void OneShotCallback(TimerHandle_t xTimer)
{
	static u8 tmr2_num = 0;
	tmr2_num++;		//周期定时器执行次数加1

	LED1=!LED1;
    //printf("定时器2运行结束\r\n");
}





