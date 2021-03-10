#include "delay.h"
#include "usart3.h"
#include "lora_app.h"
#include "lora_ui.h"
#include "led.h"

#include "string.h"
#include "stdio.h"


//////////////////////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F103开发板
//ATK-LORA-01模块功能驱动
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/4/1
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//********************************************************************************
//无

//设备参数初始化(具体设备参数见lora_cfg.h定义)
LoRa_CFG My_LoRa_CFG;

//全局参数
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;

//设备工作模式(用于记录设备状态)
u8 Lora_mode = 0; //0:配置模式 1:接收模式 2:发送模式
//记录中断状态
static u8 Int_mode = 0; //0:关闭 1:上升沿 2:下降沿

//AUX中断设置
//mode:配置的模式 0:关闭 1:上升沿 2:下降沿
void Aux_Int(u8 mode)
{
    if (!mode)
    {
        EXTI_InitStructure.EXTI_LineCmd = DISABLE; //关闭中断
        NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    }
    else
    {
        if (mode == 1)
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //上升沿
        else if (mode == 2)
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿

        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    }
    Int_mode = mode; //记录中断模式
    EXTI_Init(&EXTI_InitStructure);
    NVIC_Init(&NVIC_InitStructure);
}

//LORA_AUX中断服务函数
void EXTI4_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line4))
    {
        if (Int_mode == 1) //上升沿(发送:开始发送数据 接收:数据开始输出)
        {
            if (Lora_mode == 1) //接收模式
            {
                USART3_RX_STA = 0; //数据计数清0
            }
            Int_mode = 2; //设置下降沿触发
            LED0 = 0;     //DS0亮
        }
        else if (Int_mode == 2) //下降沿(发送:数据已发送完 接收:数据输出结束)
        {
            if (Lora_mode == 1) //接收模式
            {
                USART3_RX_STA |= 1 << 15; //数据计数标记完成
            }
            else if (Lora_mode == 2) //发送模式(串口数据发送完毕)
            {
                Lora_mode = 1; //进入接收模式
            }
            Int_mode = 1; //设置上升沿触发
            LED0 = 1;     //DS0灭
        }
        Aux_Int(Int_mode);                  //重新设置中断边沿
        EXTI_ClearITPendingBit(EXTI_Line4); //清除LINE4上的中断标志位
    }
}

//LoRa模块初始化
//返回值: 0,检测成功
//        1,检测失败
u8 LoRa_Init(void)
{
	 u8 retry=0;
	 u8 temp=1;
	
	 GPIO_InitTypeDef  GPIO_InitStructure;
		
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能PA端口时钟
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);	//使能复用功能时钟

     GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//禁止JTAG,从而PA15可以做普通IO使用,否则PA15不能做普通IO!!!	
	
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;	    		 //LORA_MD0
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	 GPIO_Init(GPIOA, &GPIO_InitStructure);	  				 //推挽输出 ，IO口速度为50MHz
	
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;				 //LORA_AUX
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 		     //下拉输入
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	 GPIO_Init(GPIOA, &GPIO_InitStructure);					 //根据设定参数初始化GPIOA.4
	
	 GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource4);
	
	 EXTI_InitStructure.EXTI_Line=EXTI_Line4;
  	 EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	 EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  //上升沿触发
  	 EXTI_InitStructure.EXTI_LineCmd = DISABLE;              //中断线关闭(先关闭后面再打开)
  	 EXTI_Init(&EXTI_InitStructure);//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器
	
	 NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;			//LORA_AUX
  	 NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;//抢占优先级2， 
  	 NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;		//子优先级3
  	 NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE; //关闭外部中断通道（后面再打开）
   	 NVIC_Init(&NVIC_InitStructure); 
	 
	 LORA_MD0=0;
	 LORA_AUX=0;
	
	 while(LORA_AUX)//确保LORA模块在空闲状态下(LORA_AUX=0)
	 {
		//Show_Str(40+30,50+20,200,16,"模块正忙,请稍等!!",16,0); 	
		 delay_ms(500);
		 //Show_Str(40+30,50+20,200,16,"                    ",16,0);
         delay_ms(100);		 
	 }
	 usart3_init(115200);//初始化串口3
	 
	 LORA_MD0=1;//进入AT模式
	 delay_ms(40);
	 retry=3;
	 while(retry--)
	 {
		 if(!lora_send_cmd("AT","OK",70))
		 {
			 temp=0;//检测成功
			 break;
		 }	
	 }
	 if(retry==0) temp=1;//检测失败
	 return temp;
}


//Lora模块参数配置
void LoRa_Set(void)
{
    u8 sendbuf[20];
    u8 lora_addrh, lora_addrl = 0;

	My_LoRa_CFG.addr =  LORA_ADDR;     	//设备地址
	My_LoRa_CFG.power = LORA_POWER;   	//发射功率
	My_LoRa_CFG.chn = LORA_CHN;       	//信道
	My_LoRa_CFG.wlrate = LORA_RATE;   	//空中速率
	My_LoRa_CFG.wltime = LORA_WLTIME;	//睡眠时间
	My_LoRa_CFG.mode = LORA_MODE;     	//工作模式
	My_LoRa_CFG.mode_sta = LORA_STA;  	//发送状态
	My_LoRa_CFG.bps = LORA_TTLBPS;    	//波特率设置
	My_LoRa_CFG.parity = LORA_TTLPAR; 	//校验位设置
	
	
    usart3_set(LORA_TTLBPS_115200, LORA_TTLPAR_8N1); //进入配置模式前设置通信波特率和校验位(115200 8位数据 1位停止 无数据校验）
    usart3_rx(1);                                    //开启串口3接收

    while (LORA_AUX);         	//等待模块空闲
    LORA_MD0 = 1; 				//进入配置模式
    delay_ms(40);
    Lora_mode = 0; 				//标记"配置模式"

    lora_addrh = (My_LoRa_CFG.addr >> 8) & 0xff;
    lora_addrl = My_LoRa_CFG.addr & 0xff;
    sprintf((char *)sendbuf, "AT+ADDR=%02x,%02x", lora_addrh, lora_addrl); //设置设备地址
    if(!lora_send_cmd(sendbuf, (void *)"OK", 50))
	{
		printf(" 设置设备地址 success \r\n");
	}
    sprintf((char *)sendbuf, "AT+WLRATE=%d,%d", My_LoRa_CFG.chn, My_LoRa_CFG.wlrate); //设置信道和空中速率
    if(!lora_send_cmd(sendbuf, (void *)"OK", 50))
	{
		printf(" 设置信道和空中速率 success \r\n");
	}
    sprintf((char *)sendbuf, "AT+TPOWER=%d", My_LoRa_CFG.power); //设置发射功率
    if(!lora_send_cmd(sendbuf, (void *)"OK", 50))
	{
		printf(" 设置发射功率  success \r\n");
	}
    sprintf((char *)sendbuf, "AT+CWMODE=%d", My_LoRa_CFG.mode); //设置工作模式
    if(!lora_send_cmd(sendbuf, (void *)"OK", 50))
	{
		printf(" 设置工作模式 success \r\n");
	}
    sprintf((char *)sendbuf, "AT+TMODE=%d", My_LoRa_CFG.mode_sta); //设置发送状态
    if(!lora_send_cmd(sendbuf, (void *)"OK", 50))
	{
		printf(" 设置发送状态 success \r\n");
	}
    sprintf((char *)sendbuf, "AT+WLTIME=%d", My_LoRa_CFG.wltime); //设置睡眠时间
    if(!lora_send_cmd(sendbuf, (void *)"OK", 50))
	{
		printf(" 设置睡眠时间 success \r\n");
	}
    sprintf((char *)sendbuf, "AT+UART=%d,%d", My_LoRa_CFG.bps, My_LoRa_CFG.parity); //设置串口波特率、数据校验位
    if(!lora_send_cmd(sendbuf, (void *)"OK", 50))
	{
		printf(" 设置串口波特率、数据校验位 success \r\n");
	}

    LORA_MD0 = 0; //退出配置,进入通信
    delay_ms(40);
    while (LORA_AUX)
        ; //判断是否空闲(模块会重新配置参数)
    USART3_RX_STA = 0;
    Lora_mode = 1;                             //标记"接收模式"
    usart3_set(My_LoRa_CFG.bps, My_LoRa_CFG.parity); //返回通信,更新通信串口配置(波特率、数据校验位)
    Aux_Int(1);                                //设置LORA_AUX上升沿中断
}

u8 Dire_Date[] = {0x11, 0x22, 0x33, 0x44, 0x55}; //定向传输数据
u8 date[30] = {0};                               //定向数组
u8 Tran_Data[30] = {0};                          //透传数组

#define Dire_DateLen sizeof(Dire_Date) / sizeof(Dire_Date[0])
u32 obj_addr; //记录用户输入目标地址
u8 obj_chn;   //记录用户输入目标信道

u8 wlcd_buff[10] = {0}; //LCD显示字符串缓冲区
//Lora模块发送数据
void LoRa_SendData(void)
{
    static u8 num = 0;
    u16 addr;
    u8 chn;
    u16 i = 0;

    if (My_LoRa_CFG.mode_sta == LORA_STA_Tran) //透明传输
    {
        sprintf((char *)Tran_Data, "ATK-LORA-01 TEST %d", num);
        u3_printf("%s\r\n", Tran_Data);
        //LCD_Fill(0, 195, 240, 220, WHITE);         //清除显示
        //Show_Str_Mid(10, 195, Tran_Data, 16, 240); //显示发送的数据

        num++;
        if (num == 255)
            num = 0;
    }
    else if (My_LoRa_CFG.mode_sta == LORA_STA_Dire) //定向传输
    {

        addr = (u16)obj_addr; //目标地址
        chn = obj_chn;        //目标信道

        date[i++] = (addr >> 8) & 0xff; //高位地址
        date[i++] = addr & 0xff;        //低位地址
        date[i] = chn;                  //无线信道

        for (i = 0; i < Dire_DateLen; i++) //数据写到发送BUFF
        {
            date[3 + i] = Dire_Date[i];
        }
        for (i = 0; i < (Dire_DateLen + 3); i++)
        {
            while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
                ; //循环发送,直到发送完毕
            USART_SendData(USART3, date[i]);
        }

        //将十六进制的数据转化为字符串打印在lcd_buff数组
        sprintf((char *)wlcd_buff, "%x %x %x %x %x %x %x %x",
                date[0], date[1], date[2], date[3], date[4], date[5], date[6], date[7]);

        //LCD_Fill(0, 200, 240, 230, WHITE);         //清除显示
        //Show_Str_Mid(10, 200, wlcd_buff, 16, 240); //显示发送的数据

        Dire_Date[4]++; //Dire_Date[4]数据更新
    }
}

u8 rlcd_buff[10] = {0}; //LCD显示字符串缓冲区

//Lora模块接收数据
void LoRa_ReceData(void)
{
    u16 i = 0;
    u16 len = 0;

    //有数据来了
    if (USART3_RX_STA & 0x8000)
    {
        len = USART3_RX_STA & 0X7FFF;
        USART3_RX_BUF[len] = 0; //添加结束符
        USART3_RX_STA = 0;

        for (i = 0; i < len; i++)
        {
            while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
                ; //循环发送,直到发送完毕
            USART_SendData(USART1, USART3_RX_BUF[i]);
        }
        //LCD_Fill(10, 260, 240, 320, WHITE);
        if (My_LoRa_CFG.mode_sta == LORA_STA_Tran) //透明传输
        {
            //Show_Str_Mid(10, 270, USART3_RX_BUF, 16, 240); //显示接收到的数据
			u3_printf("LORA_RX: %s \r\n",USART3_RX_BUF);
        }
        else if (My_LoRa_CFG.mode_sta == LORA_STA_Dire) //定向传输
        {
            //将十六进制的数据转化为字符串打印在lcd_buff数组
            sprintf((char *)rlcd_buff, "%x %x %x %x %x",
                    USART3_RX_BUF[0], USART3_RX_BUF[1], USART3_RX_BUF[2], USART3_RX_BUF[3], USART3_RX_BUF[4]);

            //Show_Str_Mid(10, 270, rlcd_buff, 16, 240); //显示接收到的数据
        }
        memset((char *)USART3_RX_BUF, 0x00, len); //串口接收缓冲区清0
    }
}

////发送和接收处理
//void LoRa_Process(void)
//{
//    u8 key = 0;
//    u8 t = 0;

//DATA:
//    Process_ui(); //界面显示
//    LoRa_Set();   //LoRa配置(进入配置需设置串口波特率为115200)
//    while (1)
//    {

//        key = KEY_Scan(0);

//        if (key == KEY0_PRES)
//        {
//            if (LoRa_CFG.mode_sta == LORA_STA_Dire) //若是定向传输,则进入配置目标地址和信道界面
//            {
//                usart3_rx(0); //关闭串口接收
//                Aux_Int(0);   //关闭中断
//                Dire_Set();   //进入设置目标地址和信道
//                goto DATA;
//            }
//        }
//        else if (key == WKUP_PRES) //返回主菜单页面
//        {
//            LORA_MD0 = 1; //进入配置模式
//            delay_ms(40);
//            usart3_rx(0); //关闭串口接收
//            Aux_Int(0);   //关闭中断
//            break;
//        }
//        else if (key == KEY1_PRES) //发送数据
//        {
//            if (!LORA_AUX && (LoRa_CFG.mode != LORA_MODE_SLEEP)) //空闲且非省电模式
//            {
//                Lora_mode = 2;   //标记"发送状态"
//                LoRa_SendData(); //发送数据
//            }
//        }
//        //数据接收
//        LoRa_ReceData();

//        t++;
//        if (t == 20)
//        {
//            t = 0;
//            LED1 = ~LED1;
//        }
//        delay_ms(10);
//    }
//}

////主测试函数
//void Lora_Test(void)
//{
//    u8 t = 0;
//    u8 key = 0;
//    u8 netpro = 0;

//    LCD_Clear(WHITE);
//    POINT_COLOR = RED;
//    Show_Str_Mid(0, 30, "ATK-LORA-01 测试程序", 16, 240);

//    while (LoRa_Init()) //初始化ATK-LORA-01模块
//    {
//        Show_Str(40 + 30, 50 + 20, 200, 16, "未检测到模块!!!", 16, 0);
//        delay_ms(300);
//        Show_Str(40 + 30, 50 + 20, 200, 16, "                ", 16, 0);
//    }
//    Show_Str(40 + 30, 50 + 20, 200, 16, "检测到模块!!!", 16, 0);
//    delay_ms(500);
//    Menu_ui(); //主界面显示

//    while (1)
//    {

//        key = KEY_Scan(0);
//        if (key)
//        {
//            Show_Str(30 + 10, 95 + 45 + netpro * 25, 200, 16, "  ", 16, 0); //清空之前的显示

//            if (key == KEY0_PRES) //KEY0按下
//            {
//                if (netpro < 6)
//                    netpro++;
//                else
//                    netpro = 0;
//            }
//            else if (key == KEY1_PRES) //KEY1按下
//            {
//                if (netpro > 0)
//                    netpro--;
//                else
//                    netpro = 6;
//            }
//            else if (key == WKUP_PRES) //KEY_UP按下
//            {
//                if (netpro == 0) //进入通信选项
//                {
//                    LoRa_Process(); //开始数据测试
//                    netpro = 0;     //索引返回第0
//                    Menu_ui();
//                }
//                else
//                {
//                    Show_Str(30 + 40, 95 + 45 + netpro * 25 + 2, 200, 16, "________", 16, 1);                                //显示下划线,表示选中
//                    Show_Str(30 + 10, 95 + 45 + netpro * 25, 200, 16, "→", 16, 0);                                           //指向新条目
//                    Menu_cfg(netpro);                                                                                        //参数配置
//                    LCD_Fill(30 + 40, 95 + 45 + netpro * 25 + 2 + 15, 30 + 40 + 100, 95 + 45 + netpro * 25 + 2 + 18, WHITE); //清除下划线显示
//                }
//            }
//            Show_Str(30 + 10, 95 + 45 + netpro * 25, 200, 16, "→", 16, 0); //指向新条目
//        }
//        t++;
//        if (t == 30)
//        {
//            t = 0;
//            LED1 = ~LED1;
//        }
//        delay_ms(10);
//    }
//}



void lora_at_response(u8 mode)
{
	if(USART3_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//添加结束符
		printf("%s",USART3_RX_BUF);	//发送到串口
		if(mode)USART3_RX_STA=0;
	} 
}
//lora发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//其他,期待应答结果的位置(str的位置)
u8* lora_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART3_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART3_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//lora发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 lora_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART3_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		while((USART3->SR&0X40)==0);//等待上一次数据发送完成  
		USART3->DR=(u32)cmd;
	}else u3_printf("%s\r\n",cmd);//发送命令
	
	if(ack&&waittime)		//需要等待应答
	{
	   while(--waittime)	//等待倒计时
	   { 
		  delay_ms(10);
		  if(USART3_RX_STA&0X8000)//接收到期待的应答结果
		  {
			  if(lora_check_cmd(ack))break;//得到有效数据 
			  USART3_RX_STA=0;
		  } 
	   }
	   if(waittime==0)res=1; 
	}
	return res;
} 



