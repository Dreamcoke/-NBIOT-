#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "stm32f10x_it.h"
#include "stm32f10x_it.c"
#include "misc.h" //包含NVIC的固件
#include "misc.c" //包含NVIC的固件
#include "dht11.h"
#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "nb.h"
#include "BH1750.h"
#include "sds011.h"
#include "gps.h"
#include "string.h"
extern u8 res;
extern u8 pm25_low;
extern u8 pm25_high; 
extern u8 pm10_low;
extern u8 pm10_high;
extern u16 pm25;
extern u16 pm10;
extern _SaveData Save_Data;
int flag_n=1;
u8 test=31;

u8 Tx_Flag=0;
u8 Rx_Flag=0;
u8 flag_a=13;
u8 flag_w=1;
u8 flag_r=0;
int aro = 0;//甲醛取消，为了Python脚本接收数据不做改动，赋0
char weather1[10]="rain";
char weather2[10]="sun";
float flag_g;
char transfer_stringWifi[0x78];
u8 temperature=0;  	    
u8 humidity=0;
u16 point_1 = 0;
int len_nb;
int delay_i=0;
int id = 10000001;
void RCC_Configuration(void); //时钟配置
void GPIO_Configuration(void); //GPIO配置
char* char2ascii(char* s,char* out);
char buf[256];


int main(void)
{ 
    RCC_Configuration();//配置时钟
    USART2_Configuration();//配置nb_iot
    GPIO_Configuration(); //配置GPIO
    sds011_USART_Init();//配置温湿度
    delay_init();	
    gps_init();//配置gps
    GPIOConfig();	//配置光照模块
    Init_BH1750();       //初始化BH1750
    
    while(1){ 
	    while(flag_n==1)
		  {
			  delay_ms(1000);
				delay_ms(1000);
			  printf("AT+CSQ\r\n");
		  }
		  printf("AT+NSOCR=DGRAM,17,4567,1\r\n");
			while(flag_n==2)
		  {  
			  
				delay_ms(1000);
		    GPIO_SetBits(GPIOE,GPIO_Pin_0);
	      if(temperature == 0)
	    	  GPIO_ResetBits(GPIOE,GPIO_Pin_5);
	      else
	    	  GPIO_SetBits(GPIOE,GPIO_Pin_5);
	      flag_a = DHT11_Read_Data(&temperature,&humidity);		//读取温湿度值	flag_a = DHT11_Read_Data(&temperature,&humidity);		//读取温湿度值	
	      flag_g=Get_gy30();//读取光照强度
	      flag_w=GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7);
	      if(flag_w==0)
	        flag_r=1;//下雨
	      else 
		      flag_r=0;//晴天
	      parseGpsBuffer();
       
	      sprintf(transfer_stringWifi,"%d,%d,%.2f,%x,%f,%d,%.2f,%d,%s,%s,%d\0",temperature,humidity,pm25/10.0,flag_a,flag_g,flag_r,pm10/10.0,aro,Save_Data.latitude,Save_Data.longitude,id);
	      len_nb=strlen(transfer_stringWifi);
	      for(delay_i=0;delay_i<5;delay_i++){
					delay_ms(1000);
				}
				
		    printf("AT+NSOST=0,118.89.244.53,8848,%d,%s\r\n",len_nb,char2ascii(transfer_stringWifi,buf));
	      
	      if(humidity>=80)
	      {
		      GPIO_SetBits(GPIOE,GPIO_Pin_1);//报警灯
		      GPIO_ResetBits(GPIOE,GPIO_Pin_0);//报警灯
		      GPIO_ResetBits(GPIOE,GPIO_Pin_2);//风扇低电平
	        GPIO_ResetBits(GPIOE,GPIO_Pin_4);//蜂鸣器低电平
		      delay_ms(1000);
					delay_ms(1000);
					delay_ms(1000);
					delay_ms(1000);
					delay_ms(1000);
	      }
	      if(humidity<80)
	      {
		      GPIO_ResetBits(GPIOE,GPIO_Pin_1);//报警灯
		      GPIO_ResetBits(GPIOE,GPIO_Pin_0);//报警灯
		      GPIO_SetBits(GPIOE,GPIO_Pin_2);//风扇低电平
	        GPIO_SetBits(GPIOE,GPIO_Pin_4);//蜂鸣器高电平
		      for(delay_i=0;delay_i<60;delay_i++){
					 delay_ms(1000);
				}
	      }	   
		  				
		}
}
}

void RCC_Configuration(void)//时钟配置子程序
{ 
	SystemInit(); //72MHz
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//使能AFIO的时钟，串口属于复用功能
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能GPIOA的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE,ENABLE);//使能GPIOE的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1的时钟， USART1挂接到APB2上。其他USART2-5挂接到APB1上
	
}



void GPIO_Configuration(void) //GPIO配置
{ /*引脚设置*/
	GPIO_InitTypeDef GPIO_InitStructure;//声明GPIO初始化结构变量。

	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_0|GPIO_Pin_6|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_8|GPIO_Pin_3|GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOE,&GPIO_InitStructure);
	GPIO_Init(GPIOC,&GPIO_InitStructure);
	
	GPIO_SetBits(GPIOE,GPIO_Pin_4);	
	
	GPIO_SetBits(GPIOE,GPIO_Pin_5);
	GPIO_SetBits(GPIOE,GPIO_Pin_6);
	GPIO_ResetBits(GPIOC,GPIO_Pin_5);	
	GPIO_SetBits(GPIOE,GPIO_Pin_3);
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
}

//char* char2ascii(char* s){
//		char ascii_s[10240];
//	  int i=0;
//	  for(i=0;i<strlen(s);i++){
		
//		}
//	 return ascii_s;
//}
char* char2ascii(char* s,char* out){
	int i = 0, n;
    char *p;
    p = out;
    for (i = 0; i < strlen(s); i++)
    {
        n = sprintf(p, "%x", s[i]);
        p += n;
    }
    return out;
}


void USART2_IRQHandler(void)                	//串口1中断服务程序
{
	u8 Res;
  
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) 
	{
		Res =USART_ReceiveData(USART2);//(USART1->DR);	//读取接收到的数据
	  USART_RX_BUF[point_1++] = Res;
    
	if(flag_n==1)
	{
		
		if(0 <=USART_RX_BUF[7]*10 + USART_RX_BUF[8] <=31)			//确定是否收到"GPRMC/GNRMC"这一帧数据
	  {
			
			flag_n=2;

	
	  }
	}
	
}
}
