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


//�������ȼ�
#define BLPRINT_TASK_PRIO	4
//�����ջ��С	
#define BLPRINT_STK_SIZE 	128  
//������
TaskHandle_t blprint_Task_Handler;
//������
#define VERSION "Ӳ��V2.5 ��V1.3"
void process_task(void *pvParameters);
#define SIZE sizeof(TEXT_Buffer)
#define FLASH_SAVE_ADDR  0X0801FC00		//����FLASH �����ַ(����Ϊż��������ֵҪ���ڱ�������ռ��FLASH�Ĵ�С+0X08000000)
#define FLASH_SAVE_SNADDR  0X0801F800		//����FLASH �����ַ(����Ϊż��������ֵҪ���ڱ�������ռ��FLASH�Ĵ�С+0X08000000)

////////////////////////////////////////////////////////
#define BLUEADDRESS_SIZE  12
#define SNADDRESS_SIZE  8


TimerHandle_t 	AutoReloadTimer_Handle;			//���ڶ�ʱ�����
TimerHandle_t	OneShotTimer_Handle;			//���ζ�ʱ�����

void AutoReloadCallback(TimerHandle_t xTimer); 	//���ڶ�ʱ���ص�����
void OneShotCallback(TimerHandle_t xTimer);		//���ζ�ʱ���ص�����
void blprint_task(void *pvParameters);


static u8 blConnectflag; // 1���� 0δ����
static u16 bladressmark;//0x55 ˵����д��
static u8 blueAddress[36];
static u16 SNadressmark;//0x55 ˵����д��
static u8 SNno[9];


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
  static float a = 0,numi = 0,numf = 0,mult = 1.0f;
  unsigned int inumb = 0,flag = 0;
 float Txt2Float(char s[]) {
	 int len;
	 a = 0;
	 numi = 0;
	 numf = 0;
	 mult = 1.0f;
	 inumb = 0;
	 flag = 0;
	 len = Slen(s);
	 inumb = len - 1;
	 flag = Include(s,'.');
	 if(flag) {// ����С���� 
		 while(s[inumb] != '.') {// С������ 
			 numf = numf/10.0f + s[inumb] - '0';
			 --inumb;
		 }
		 numf = numf/10.0f;
		 //--inumb;
		 while(inumb) {// �������� 
			 numi = numi + (s[inumb-1] - '0')*10;
			 --inumb;
		 }
		 numi=numi/10;
	 }
	 else {
		 inumb = 0; 
		 while(s[inumb]) {//������С���� 
			 numi = 10 * numi + s[inumb] - '0';
			 ++inumb;
		 }
	 }
  
	 return numi + numf;
 }

static void bluetooth_AT(uint8_t * string){
	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	delay_ms(10);
	
	usart2_send_string(USART2,(uint8_t *)string, strlen(string));
	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	delay_ms(10);
	
}

static void snno_init(){
		memset(SNno,0x00,sizeof(SNno));
		STMFLASH_Read(FLASH_SAVE_SNADDR+SNADDRESS_SIZE+2,&SNadressmark,1);
		if(0x55AA==SNadressmark){
			STMFLASH_Read(FLASH_SAVE_SNADDR,(u16*)SNno,SNADDRESS_SIZE);
		}else{
		memset(SNno,0x00,sizeof(SNno));
		}
}


static void bluetooth_print_init(){
		memset(blueAddress,0x00,sizeof(blueAddress));
		STMFLASH_Read(FLASH_SAVE_ADDR+BLUEADDRESS_SIZE+2,&bladressmark,1);
		if(0x55AA==bladressmark){
			STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)blueAddress,BLUEADDRESS_SIZE);
		}else{
		memset(blueAddress,0x00,36);
		}
}

static u8 isbluetoothConnected(){
		bluetooth_AT("AT+STATE ?\r\n");
		delay_ms(100);
		
}


static void bluetoothConnect(){
		u8 bladdresstemp[36];
		u8 print[36];
		bladdresstemp[0]=blueAddress[0];
		bladdresstemp[1]=blueAddress[1];
		bladdresstemp[2]=blueAddress[2];
		bladdresstemp[3]=blueAddress[3];
		bladdresstemp[4]=',';
		bladdresstemp[5]=blueAddress[4];
		bladdresstemp[6]=blueAddress[5];
		bladdresstemp[7]=',';
		bladdresstemp[8]=blueAddress[6];
		bladdresstemp[9]=blueAddress[7];
		bladdresstemp[10]=blueAddress[8];
		bladdresstemp[11]=blueAddress[9];
		bladdresstemp[12]=blueAddress[10];
		bladdresstemp[13]=blueAddress[11];
		sprintf(print,"%s","AT+LINK=");
		strcat(print,bladdresstemp);
		strcat(print,"\r\n");
		//bluetooth_AT("AT+LINK=020E,3A,D0801E\r\n");
		bluetooth_AT(print);
}

int main(void)
{
	//static u8 datatemp[36];
	delay_init();	    	 //��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(9600);	 //���ڳ�ʼ��Ϊ115200
	uart2_init(9600);
	TIM3_Mode_Config();
	TIM4_Mode_Config();
	//Enocode_start();
	bluetooth_print_init();
	snno_init();
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
	//STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)datatemp,SIZE);
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
	xTaskCreate((TaskFunction_t )blprint_task,            //������
                (const char*    )"blprint_task",          //��������
                (uint16_t       )BLPRINT_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )BLPRINT_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&blprint_Task_Handler);   //������
	xTimerStart(AutoReloadTimer_Handle,0);
    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
    
}

void blprint_task(void *pvParameters){
	BaseType_t err;
	u8 charBUF[36];
	while(1){
	if(BinarySemaphore!=NULL)
		{
			//��ȡ�ź����ɹ�
			err=xSemaphoreTake(BinarySemaphore,0xffff);	//��ȡ�ź���
			if(err==pdTRUE)	{
				usart2_send_str(USART2,"\r\n");
				usart2_send_str(USART2,"\r\n");
				if(_measureFlag.page==2){
					usart2_send_str(USART2,"-�����ݽ��������ƶ����Ա��桪��\r\n");
				}else{
					usart2_send_str(USART2,"�����������ƶ�������Ա��桪��\r\n");
				}
				#if 0
				sprintf(charBUF, "%d", calendar.w_year);
				strcat(charBUF, "-");
				usart_send_string(USART1,charBUF);

				sprintf(charBUF, "%d", calendar.w_month);
				strcat(charBUF, "-");
				usart_send_string(USART1,charBUF);

				sprintf(charBUF, "%d", calendar.w_date);
				strcat(charBUF, "-");
				usart_send_string(USART1,charBUF);

				sprintf(charBUF, "%d", calendar.hour);
				strcat(charBUF, "-");
				usart_send_string(USART1,charBUF);

				sprintf(charBUF, "%d", calendar.min);
				strcat(charBUF, "-");
				usart_send_string(USART1,charBUF);

				sprintf(charBUF, "%d", calendar.sec);
				strcat(charBUF, "\r\n");
				usart_send_string(USART1,charBUF);
				#endif
				usart2_send_str(USART2,"�������:");
				sprintf(charBUF, "%s", SNno);
				usart2_send_str(USART2,charBUF);
				usart2_send_str(USART2,"\r\n");
				if(_measureFlag.page==0){
					usart2_send_str(USART2,"��������:��ֱ����\r\n");
				}else if(_measureFlag.page==1){
					usart2_send_str(USART2,"��������:�Զ�����\r\n");
				}
				
				if((_measureFlag.page==0)||(_measureFlag.page==2)){
				usart2_send_str(USART2,"����ҷ����:");
				switch(_measureFlag.rate){
					case 1:
						usart2_send_str(USART2,"1:1");
						break;
					case 2:
						usart2_send_str(USART2,"2:1");
						break;
					case 4:
						usart2_send_str(USART2,"4:1");	
						break;
					case 6:
						usart2_send_str(USART2,"6:1");
						break;
					case 8:
						usart2_send_str(USART2,"8:1");	
						break;
					default:
						break;
				}
				usart2_send_str(USART2,"\r\n");
				}
				if(_measureFlag.page==2){
					if(_measureFlag.trigselect==1){
						usart2_send_str(USART2,"������ʽ:�Զ�����\r\n");	
						}else{
						usart2_send_str(USART2,"������ʽ:�˹�����\r\n");
					}
				}	
				
				usart2_send_str(USART2,"�����ٶ�:");
				sprintf(charBUF, "%.3f", _measureFlag.resulttrigvalue);
				usart2_send_str(USART2,charBUF);
				usart2_send_str(USART2,"m/s\r\n");
				if(_measureFlag.page==2){
					usart2_send_str(USART2,"�ƶ�����:");
				}else{
					usart2_send_str(USART2,"�ƶ�����:");
				}
				sprintf(charBUF, "%.3f", _measureFlag.movevalue);
				usart2_send_str(USART2,charBUF);
				usart2_send_str(USART2,"m\r\n");

				usart2_send_str(USART2,"\r\n");
				usart2_send_str(USART2,"\r\n");
				usart2_send_str(USART2,"\r\n");
				usart2_send_str(USART2,"�Ϻ�ʢ��˹�Զ����豸�ɷ����޹�˾");
				usart2_send_str(USART2,"-----------4000666980-----------");
				usart2_send_str(USART2,"\r\n");
				usart2_send_str(USART2,"\r\n");
				usart2_send_str(USART2,"\r\n");
				usart2_send_str(USART2,"\r\n");
				usart2_send_str(USART2,"\r\n");
				
				
			}
			
		}
    }


}

void process_task(void *pvParameters)
{	
	u32 distancevaluedisp=0;
	float motorvdisp;
	u8 trigvalue[30];
	u8 bufstring[30];
	static u8 datatemp[30];
	u8* bufpoint;
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
				if(NULL!=strstr((const char *)uart1_dev.USART_RX_BUF,"AT+WBL")){
					bufpoint = uart1_dev.USART_RX_BUF+strlen("AT+WBL");
					sprintf((char *)bufstring, "%s", bufpoint);
					//STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)TEXT_Buffer,SIZE);
					//strcat(bufstring,"/5/5/A/A");
					STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)bufstring,BLUEADDRESS_SIZE);
					{
					u16 temp=0x55AA;
					STMFLASH_Write(FLASH_SAVE_ADDR+BLUEADDRESS_SIZE+2,&temp,1);
					}
					memset(datatemp,0x00,36);
					STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)datatemp,BLUEADDRESS_SIZE);
					if(strcmp(datatemp,bufstring)==0){
						{
									char temp[36] = "������ַ��¼�ɹ�\r\n";
									usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
		
						}
					}else{
									char temp[36] = "������ַ��¼ʧ��\r\n";
									usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					}
					
				}
				else if(NULL!=strstr((const char *)uart1_dev.USART_RX_BUF,"AT+RBL")){
					memset(datatemp,0x00,36);
					STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)datatemp,BLUEADDRESS_SIZE);
					{
									char temp[36];
									sprintf((char *)temp, "%s", "������ַΪ��");
									strcat(temp,datatemp);
									strcat(temp,"\r\n");
									usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					}
				}if(NULL!=strstr((const char *)uart1_dev.USART_RX_BUF,"AT+WSN")){
					bufpoint = uart1_dev.USART_RX_BUF+strlen("AT+WBL");
					sprintf((char *)bufstring, "%s", bufpoint);
					//STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)TEXT_Buffer,SIZE);
					//strcat(bufstring,"/5/5/A/A");
					STMFLASH_Write(FLASH_SAVE_SNADDR,(u16*)bufstring,SNADDRESS_SIZE);
					{
					u16 temp=0x55AA;
					STMFLASH_Write(FLASH_SAVE_SNADDR+SNADDRESS_SIZE+2,&temp,1);
					}
					memset(datatemp,0x00,36);
					STMFLASH_Read(FLASH_SAVE_SNADDR,(u16*)datatemp,SNADDRESS_SIZE);
					if(strcmp(datatemp,bufstring)==0){
						{
									char temp[36] = "SN����¼�ɹ�\r\n";
									usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
		
						}
					}else{
									char temp[36] = "SN����¼ʧ��\r\n";
									usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					}
					
				}
				else if(NULL!=strstr((const char *)uart1_dev.USART_RX_BUF,"AT+RSN")){
					memset(datatemp,0x00,36);
					STMFLASH_Read(FLASH_SAVE_SNADDR,(u16*)datatemp,SNADDRESS_SIZE);
					{
									char temp[36];
									sprintf((char *)temp, "%s", "SN��Ϊ��");
									strcat(temp,datatemp);
									strcat(temp,"\r\n");
									usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					}
				}else if(NULL!=strstr((const char *)uart1_dev.USART_RX_BUF,"AT+PSWD=")){
					strcat(uart1_dev.USART_RX_BUF,"\r\n");
					bluetooth_AT(uart1_dev.USART_RX_BUF);
					
				}else if(NULL!=strstr((const char *)uart1_dev.USART_RX_BUF,"AT+VERSION?")){
					{
									char temp[36];
									sprintf((char *)temp, "%s", "�汾��Ϊ��");
									strcat(temp,VERSION);
									strcat(temp,"\r\n");
									usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					}
					
				}else{
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
								_measureFlag.trigreportflag=1;
								_measureFlag.resulttrigvalueflag=1;
								_measureFlag.startprint=0;//���ܴ�ӡ
								break;
								case 0xaf://stop
								_measureFlag.startVmeasure=0;
								_measureFlag.trigflag=0;
								distancevaluedisp=0;
								motorvdisp=0;
								Enocode_stop();
								break;
								case 0xad://��ӡ
								if((_measureFlag.startprint==1)&&(blConnectflag==1)){
									{
										if(BinarySemaphore!=NULL){
											xSemaphoreGive(BinarySemaphore);	//�ͷŶ�ֵ�ź���
										}
									}
								}
								break;
								case 0xc0://ҷ����
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
											_measureFlag.rate=6;
											break;
										case 0xa5:
											_measureFlag.rate=8;
											break;	
									}
									_measureFlag.page=0;
								
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
											_measureFlag.rate=6;
											break;
										case 0xa5:
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
											_measureFlag.rate=6;
											break;
										case 0xa5:
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
								case 0xc3://ҷ����
									_measureFlag.rate=1;
									_measureFlag.page=3;
								
								break;
							}
						break;	
					}
					}
				memset(&uart1_dev,0x00,sizeof(uart1_dev));
				
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
					if(_measureFlag.trigreportflag==1){
						_measureFlag.trigreportflag=0;
						{
							char temp[36] = "t10.txt=\"";
							char buf[10];
							char end[3]={0xff,0xff,0xff};
							u8 i=0;

							sprintf(buf, "%.3f", (float)_measureFlag.resulttrigvalue);
							strcat(temp, buf);
							strcat(temp, "\"");
							strcat(temp, end);
							usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
						}	
						
					}
					distancevaluedisp=distancevalue;
					{
					char temp[36] = "t3.txt=\"";
					char buf[10];
					char end[3]={0xff,0xff,0xff};
					u8 i=0;
					
					sprintf(buf, "%.3f", (float)(distancevaluedisp)/1000000.0);
					strcat(temp, buf);
					strcat(temp, "\"");
					strcat(temp, end);
					usart_send_string(USART1,(uint8_t *)temp, strlen(temp));
					if(motorvdisp==0){
						_measureFlag.movevalue=(float)(distancevaluedisp)/1000000.0;
						_measureFlag.startVmeasure=0;
						_measureFlag.trigflag=0;
						_measureFlag.trigreportflag=1;
						_measureFlag.resulttrigvalueflag=1;
						distancevaluedisp=0;
						motorvdisp=0;
						Enocode_stop();
						_measureFlag.startprint=1;
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
			
			vTaskDelay(3000); //��ʱ10ms��Ҳ����10��ʱ�ӽ���
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
	//static u8 test;
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





