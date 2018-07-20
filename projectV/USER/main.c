#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "usart.h"
#include "timer.h"
#include "flagConfig.h"

/************************************************
 ALIENTEKս��STM32������ʵ��8
 ��ʱ���ж�ʵ��
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

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

 int main(void)
 {		
	u32 distancevaluedisp=0;
	float motorvdisp;
	u8 trigvalue[30];
	u8 i=0;
	delay_init();	    	 //��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(9600);	 //���ڳ�ʼ��Ϊ115200
	uart2_init(115200);
	TIM3_Mode_Config();
	TIM4_Mode_Config();
	//Enocode_start();
	init__measureFlag();
	EXTIX_Init();
	LED_Init();			     //LED�˿ڳ�ʼ��
	//delay_ms(100);
	//USART_RX_STA=0;
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
		if(USART_RX_STA&0x8000)
		{					   
			//len=USART_RX_STA&0x3fff;
			for(i=0;i<36;i++){
				if(USART_RX_BUF[i]==0x9a)
					break;
			}
			switch(USART_RX_BUF[i])
			{
				case 0x9a:
					switch(USART_RX_BUF[i+1]){
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
							switch(USART_RX_BUF[i+2]){
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
							switch(USART_RX_BUF[i+2]){
								case 0xd1:
									_measureFlag.trigselect=1;
									break;
								case 0xd2:
									_measureFlag.trigselect=2;
									break;
								}	
							switch(USART_RX_BUF[i+3]){
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
									trigvalue[j]=USART_RX_BUF[i+4];
									if(trigvalue[j]==0x00)break;
									i++;
								}
								
							}
							_measureFlag.trigvalue=Txt2Float(trigvalue);
						break;
						
					}
				break;	
			}
			
			USART_RX_STA=0;
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
		
		delay_ms(200);
	}	 

 
}	 
 
