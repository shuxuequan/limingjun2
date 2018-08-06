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
 ALIENTEK Mini STM32F103������ FreeRTOSʵ��15-1
 FreeRTOS�����ʱ��ʵ��-�⺯���汾
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

//�������ȼ�
#define START_TASK_PRIO			1
//�����ջ��С	
#define START_STK_SIZE 			36
TaskHandle_t StartTask_Handler;
//������
void start_task(void *pvParameters);

//�������ȼ�
#define TIMERCONTROL_TASK_PRIO	2
//�����ջ��С	
#define TIMERCONTROL_STK_SIZE 	128  
//������
TaskHandle_t TimerControlTask_Handler;
//������
void timercontrol_task(void *pvParameters);

//�������ȼ�
#define PROCESS_TASK_PRIO	3
//�����ջ��С	
#define PROCESS_STK_SIZE 	512  
//������
TaskHandle_t ProcessTask_Handler;
//������
void process_task(void *pvParameters);
const u8 TEXT_Buffer[]={"WarShipSTM32 IIC TEST"};
#define SIZE sizeof(TEXT_Buffer)
#define FLASH_SAVE_ADDR  0X0801FC00		//����FLASH �����ַ(����Ϊż��������ֵҪ���ڱ�������ռ��FLASH�Ĵ�С+0X08000000)
////////////////////////////////////////////////////////
TimerHandle_t 	AutoReloadTimer_Handle;			//���ڶ�ʱ�����
TimerHandle_t	OneShotTimer_Handle;			//���ζ�ʱ�����

void AutoReloadCallback(TimerHandle_t xTimer); 	//���ڶ�ʱ���ص�����
void OneShotCallback(TimerHandle_t xTimer);		//���ζ�ʱ���ص�����


static u8 blConnectflag; // 1���� 0δ����

//��ֵ�ź������
SemaphoreHandle_t BinarySemaphore;	//��ֵ�ź������

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
	 if(flag) {// ����С���� 
		 while(s[i] != '.') {// С������ 
			 numf = numf/10.0f + s[i] - '0';
			 --i;
		 }
		 numf = numf/10.0f;
		 --i;
		 while(s[i]) {// �������� 
			 numi = 10 * numi + s[i] - '0';
			 --i;
		 }
	 }
	 else {
		 i = 0; 
		 while(s[i]) {//������С���� 
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
	delay_init();	    	 //��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(9600);	 //���ڳ�ʼ��Ϊ115200
	uart2_init(9600);
	TIM3_Mode_Config();
	TIM4_Mode_Config();
	//Enocode_start();
	bluetooth_print_init();
	init__measureFlag();
	EXTIX_Init();
	//AT24CXX_Init();			//IIC��ʼ��
	LED_Init();			     //LED�˿ڳ�ʼ��
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
	//������ʼ����
    xTaskCreate((TaskFunction_t )start_task,            //������
                (const char*    )"start_task",          //��������
                (uint16_t       )START_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&StartTask_Handler);   //������              
    vTaskStartScheduler();          //�����������
		 			
}

//��ʼ����������
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //�����ٽ���
    //����������ڶ�ʱ��
    AutoReloadTimer_Handle=xTimerCreate((const char*		)"AutoReloadTimer",
									    (TickType_t			)1000,
							            (UBaseType_t		)pdTRUE,
							            (void*				)1,
							            (TimerCallbackFunction_t)AutoReloadCallback); //���ڶ�ʱ��������1s(1000��ʱ�ӽ���)������ģʽ
    //�������ζ�ʱ��
	OneShotTimer_Handle=xTimerCreate((const char*			)"OneShotTimer",
							         (TickType_t			)2000,
							         (UBaseType_t			)pdFALSE,
							         (void*					)2,
							         (TimerCallbackFunction_t)OneShotCallback); //���ζ�ʱ��������2s(2000��ʱ�ӽ���)������ģʽ
	//������ֵ�ź���
	BinarySemaphore=xSemaphoreCreateBinary();									 
    //������ʱ����������
    xTaskCreate((TaskFunction_t )timercontrol_task,             
                (const char*    )"timercontrol_task",           
                (uint16_t       )TIMERCONTROL_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )TIMERCONTROL_TASK_PRIO,        
                (TaskHandle_t*  )&TimerControlTask_Handler); 

    xTaskCreate((TaskFunction_t )process_task,            //������
                (const char*    )"process_task",          //��������
                (uint16_t       )PROCESS_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )PROCESS_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&ProcessTask_Handler);   //������

	xTimerStart(AutoReloadTimer_Handle,0);
    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
    
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
							case 0xc1://ҷ����
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
				//printf("�ٶ� : %.3f\r\n",motorv);
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
					printf("�ٶ� : %.3f\r\n",motorv);
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


//TimerControl��������
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
				//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//�����ж�
				vTaskDelay(100);
				//while(count--){
					uart2_dev.USART_RX_STA=0;
					memset(uart2_dev.USART_RX_BUF,0x00,USART_REC_LEN);
	 				bluetoothConnect();
	 				//if(BinarySemaphore!=NULL)
	 				//{
	 					//��ȡ�ź����ɹ�
						//err=xSemaphoreTake(BinarySemaphore,0xffff);	//��ȡ�ź���
	 					//if(err==pdTRUE)	{
							//if(strcmp(uart2_dev.USART_RX_BUF,"OK")==0){
								//test1=1;
								//break;
							//}
	 					//}
	 				//}
				//}
				//USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);//�ر��ж�
			}
			
			vTaskDelay(2000); //��ʱ10ms��Ҳ����10��ʱ�ӽ���
		}
	#if 0	
		//ֻ��������ʱ���������ɹ��˲��ܶ�����в���
		if((AutoReloadTimer_Handle!=NULL)&&(OneShotTimer_Handle!=NULL))
		{
			key = KEY_Scan(0);
			switch(key)
			{
				case WKUP_PRES:     //��key_up���µĻ������ڶ�ʱ��
					xTimerStart(AutoReloadTimer_Handle,0);	//�������ڶ�ʱ��
					//printf("������ʱ��1\r\n");
					break;
				case KEY0_PRES:		//��key0���µĻ��򿪵��ζ�ʱ��
					xTimerStart(OneShotTimer_Handle,0);		//�������ζ�ʱ��
					//printf("������ʱ��2\r\n");
					break;
				case KEY1_PRES:		//��key1���»��͹رն�ʱ��
					xTimerStop(AutoReloadTimer_Handle,0); 	//�ر����ڶ�ʱ��
					xTimerStop(OneShotTimer_Handle,0); 		//�رյ��ζ�ʱ��
					//printf("�رն�ʱ��1��2\r\n");
					break;	
			}
		}
		num++;
		if(num==50) 	//ÿ500msLED0��˸һ��
		{
			num=0;
			LED0=!LED0;	
		}
        vTaskDelay(10); //��ʱ10ms��Ҳ����10��ʱ�ӽ���
	}
	#endif	
	



}

//���ڶ�ʱ���Ļص�����
void AutoReloadCallback(TimerHandle_t xTimer)
{
	static u8 test;
	//if(BinarySemaphore!=NULL){
		//xSemaphoreGive(BinarySemaphore);	//�ͷŶ�ֵ�ź���
	//}
	
	
	
}

//���ζ�ʱ���Ļص�����
void OneShotCallback(TimerHandle_t xTimer)
{
	static u8 tmr2_num = 0;
	tmr2_num++;		//���ڶ�ʱ��ִ�д�����1

	LED1=!LED1;
    //printf("��ʱ��2���н���\r\n");
}





