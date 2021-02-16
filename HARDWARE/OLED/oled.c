#include "stm32f10x.h"
#include "sys.h"
#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"  	 
#include "bmp.h"

extern void delay_us(uint32_t nus);
extern void delay_ms(uint32_t nms);

static GPIO_InitTypeDef   			GPIO_InitStructure;


//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127	
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127 			   
void i2c_sda_mode(GPIOMode_TypeDef mode)
{
	/* 配置PB7引脚为输出模式 */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;					//第7根引脚
	GPIO_InitStructure.GPIO_Mode = mode;						//设置输出/输入模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;			//设置IO的速度为100MHz，频率越高性能越好，频率越低，功耗越低
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void i2c_start(void)
{
	SDA_OUT();		//保证SDA引脚为输出模式
	SCL=1;
	SDA_W=1;
	delay_us(4);
	SDA_W=0;
	delay_us(4);		
	SCL=0; 
	delay_us(4);	//钳住I2C总线，准备发送或接受数据
}

/**********************************************
//IIC Stop
**********************************************/
void i2c_stop(void)
{
	SDA_OUT();		//保证SDA引脚为输出模式
	SCL=0;
	SDA_W=0;
	delay_us(4);
	SCL=1;	
	delay_us(4);	
	SDA_W=1;
	delay_us(4);	//发送I2C结束信号
}

uint32_t i2c_wait_ack(void)
{
	uint32_t ack=0;

	SDA_IN();//保证SDA引脚为输入模式
	SDA_R = 1;	delay_us(1);
	SCL = 1;	delay_us(1);
	
	//判断SDA引脚的电平
	if(SDA_R){
		ack=1;
		//中止整个i2c通信
		i2c_stop();
	}
	
	SCL = 0;	delay_us(1);
	return ack;
}

void Write_IIC_Byte(unsigned char IIC_Byte)
{
	uint32_t i=0;
	//保证SDA引脚为输出模式
	SDA_OUT();		//保证SDA引脚为输出模式
	SCL=0;
	SDA_W=0;
	//延时1us
	delay_us(1);
	
	for(i=0; i<8; i++)
	{
		//以MSB最高有效位进行数据的发送
		if(IIC_Byte & (1<<(7-i)))
			SDA_W=1;
		else
			SDA_W=0;
	
		//延时1us
		delay_us(1);
		
		//设置时钟线为高电平，告诉从机现在可以读取SDA引脚的电平
		SCL=1;	
		
		//延时1us
		delay_us(1);	

		//设置时钟线为低电平，告诉从机现在不可以读取SDA引脚的电平，因为主机现在要更改SDA引脚的电平
		SCL=0;	
		
		//延时1us
		delay_us(1);			
	}
}

void Write_IIC_Command(unsigned char IIC_Command)
{
	i2c_start();
	Write_IIC_Byte(0x78);         
	i2c_wait_ack();	
	Write_IIC_Byte(0x00);		
	i2c_wait_ack();	
	Write_IIC_Byte(IIC_Command); 
	i2c_wait_ack();	
	i2c_stop();
}

void Write_IIC_Data(unsigned char IIC_Data)
{
	i2c_start();
	Write_IIC_Byte(0x78);			//D/C#=0; R/W#=0
	i2c_wait_ack();	
	Write_IIC_Byte(0x40);			//write data
	i2c_wait_ack();	
	Write_IIC_Byte(IIC_Data);
	i2c_wait_ack();	
	i2c_stop();
}
void OLED_WR_Byte(unsigned dat,unsigned cmd)
{
	if(cmd)
	{
		Write_IIC_Data(dat);
	}
	else {
		Write_IIC_Command(dat);
	}
}


/********************************************
// fill_Picture
********************************************/
void fill_picture(unsigned char fill_Data)
{
	unsigned char m,n;
	for(m=0;m<8;m++)
	{
		OLED_WR_Byte(0xb0+m,0);		//page0-page1
		OLED_WR_Byte(0x00,0);		//low column start address
		OLED_WR_Byte(0x10,0);		//high column start address
		for(n=0;n<128;n++)
		{
			OLED_WR_Byte(fill_Data,1);
		}
	}
}

//坐标设置
void OLED_Set_Pos(unsigned char x, unsigned char y) 
{ 	OLED_WR_Byte(0xb0+y,OLED_CMD);
	OLED_WR_Byte(((x&0xf0)>>4)|0x10,OLED_CMD);
	OLED_WR_Byte((x&0x0f),OLED_CMD); 
}   

//开启OLED显示    
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14,OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF,OLED_CMD);  //DISPLAY ON
}

//关闭OLED显示     
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10,OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE,OLED_CMD);  //DISPLAY OFF
}		 

//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!	  
void OLED_Clear(void)  
{  
	uint8_t i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(0,OLED_DATA); 
	} //更新显示
}

void OLED_Clear_Line(uint8_t line)  
{  
	uint8_t n;		    

	OLED_WR_Byte (0xb0+line,OLED_CMD);    //设置页地址（0~7）
	OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
	OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址  
	
	for(n=0;n<128;n++)
		OLED_WR_Byte(0,OLED_DATA); 

}


void OLED_On(void)  
{  
	uint8_t i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(1,OLED_DATA); 
	} //更新显示
}

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示				 
//size:选择字体 16/12 
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Char_Size)
{      	
	unsigned char c=0,i=0;	
	c=chr-' ';//得到偏移后的值			
	if(x>Max_Column-1){x=0;y=y+2;}
	if(Char_Size ==16)
	{
		OLED_Set_Pos(x,y);	
		for(i=0;i<8;i++)
		OLED_WR_Byte(F8X16[c*16+i],OLED_DATA);
		OLED_Set_Pos(x,y+1);
		for(i=0;i<8;i++)
		OLED_WR_Byte(F8X16[c*16+i+8],OLED_DATA);
	}
	else {	
		OLED_Set_Pos(x,y);
		for(i=0;i<6;i++)
		OLED_WR_Byte(F6x8[c][i],OLED_DATA);
		
	}
}
//m^n函数
uint32_t oled_pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}	

//显示2个数字
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);	 		  
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size2)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size2/2)*t,y,' ',size2);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2); 
	}
} 

//显示一个字符号串
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t Char_Size)
{
	unsigned char j=0;
	while (chr[j]!='\0')
	{		
		OLED_ShowChar(x,y,chr[j],Char_Size);
		x+=8;
		if(x>120){x=0;y+=2;}
			j++;
	}
}

//显示汉字
void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t no)
{      			    
	uint8_t t,adder=0;
	OLED_Set_Pos(x,y);	
    for(t=0;t<16;t++)
	{
		OLED_WR_Byte(Hzk[2*no][t],OLED_DATA);
		adder+=1;
	}	
	OLED_Set_Pos(x,y+1);	
    for(t=0;t<16;t++)
	{	
		OLED_WR_Byte(Hzk[2*no+1][t],OLED_DATA);
		adder+=1;
	}					
}

/***********功能描述：显示显示BMP图片128×64起始点坐标(x,y),x的范围0～127，y为页的范围0～7*****************/
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[])
{ 	
	unsigned int j=0;
	unsigned char x,y;

	if(y1%8==0) 
		y=y1/8;      
	else 
		y=y1/8+1;
	for(y=y0;y<y1;y++)
	{
		OLED_Set_Pos(x0,y);
		for(x=x0;x<x1;x++)
		{      
			OLED_WR_Byte(BMP[j++],OLED_DATA);	    	
		}
	}
} 

//初始化SSD1306					    
void OLED_Init(void)
{ 	
 	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	//使能端口B的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	
	
	/* 配置PB6 PB7引脚为输出模式 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;		//第6根和7根引脚
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz频率越高性能越好，频率越低，功耗越低
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	delay_ms(200);

	OLED_WR_Byte(0xAE,OLED_CMD);//--display off
	OLED_WR_Byte(0x00,OLED_CMD);//---set low column address
	OLED_WR_Byte(0x10,OLED_CMD);//---set high column address
	OLED_WR_Byte(0x40,OLED_CMD);//--set start line address  
	OLED_WR_Byte(0xB0,OLED_CMD);//--set page address
	OLED_WR_Byte(0x81,OLED_CMD); // contract control
	OLED_WR_Byte(0xFF,OLED_CMD);//--128   
	OLED_WR_Byte(0xA1,OLED_CMD);//set segment remap 
	OLED_WR_Byte(0xA6,OLED_CMD);//--normal / reverse
	OLED_WR_Byte(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3F,OLED_CMD);//--1/32 duty
	OLED_WR_Byte(0xC8,OLED_CMD);//Com scan direction
	OLED_WR_Byte(0xD3,OLED_CMD);//-set display offset
	OLED_WR_Byte(0x00,OLED_CMD);//
	
	OLED_WR_Byte(0xD5,OLED_CMD);//set osc division
	OLED_WR_Byte(0x80,OLED_CMD);//
	
	OLED_WR_Byte(0xD8,OLED_CMD);//set area color mode off
	OLED_WR_Byte(0x05,OLED_CMD);//
	
	OLED_WR_Byte(0xD9,OLED_CMD);//Set Pre-Charge Period
	OLED_WR_Byte(0xF1,OLED_CMD);//
	
	OLED_WR_Byte(0xDA,OLED_CMD);//set com pin configuartion
	OLED_WR_Byte(0x12,OLED_CMD);//
	
	OLED_WR_Byte(0xDB,OLED_CMD);//set Vcomh
	OLED_WR_Byte(0x30,OLED_CMD);//
	
	OLED_WR_Byte(0x8D,OLED_CMD);//set charge pump enable
	OLED_WR_Byte(0x14,OLED_CMD);//
	
	OLED_WR_Byte(0xAF,OLED_CMD);//--turn on oled panel
}  

void start_oled_ui(void)
{
	OLED_Clear();

	//显示logo
//	OLED_DrawBMP(0,0,128,8,(uint8_t *)pic_logo);	

	//持续2秒
//	delay_ms(2000);
//	OLED_Clear();
	
	//显示“输气站气体泄漏”
	OLED_ShowCHinese(8,0,0);
	OLED_ShowCHinese(24,0,1);
	OLED_ShowCHinese(40,0,2);	
	OLED_ShowCHinese(56,0,3);	
	OLED_ShowCHinese(72,0,4);		
	OLED_ShowCHinese(88,0,5);		
	OLED_ShowCHinese(104,0,6);
	
	//显示“监测系统”
	OLED_ShowCHinese(32,3,7);
	OLED_ShowCHinese(48,3,8);
	OLED_ShowCHinese(64,3,9);	
	OLED_ShowCHinese(80,3,10);	

	//显示 "北京理工 何蔚"
	OLED_ShowCHinese(0,6,11);
	OLED_ShowCHinese(16,6,12);
	OLED_ShowCHinese(32,6,13);
	OLED_ShowCHinese(48,6,14);
	OLED_ShowCHinese(62,6,15);
	OLED_ShowCHinese(78,6,16);
	OLED_ShowCHinese(98,6,17);	
	OLED_ShowCHinese(112,6,18);
	
	//持续2秒
	delay_ms(2000);
	
	OLED_Clear();
}



























