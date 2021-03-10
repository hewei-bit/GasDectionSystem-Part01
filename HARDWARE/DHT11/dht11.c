#include "dht11.h"
#include "delay.h"
  
//复位DHT11
void DHT11_Rst(void)	   
{                 
	DHT11_IO_OUT(); 	//SET OUTPUT
    DHT11_DQ_OUT=0; 	//拉低DQ
    delay_ms(20);    	//拉低至少18ms
    DHT11_DQ_OUT=1; 	//DQ=1 
	delay_us(30);     	//主机拉高20~40us
}
//等待DHT11的回应
//返回1:未检测到DHT11的存在
//返回0:存在
u8 DHT11_Check(void) 	   
{   
	u8 retry=0;
	DHT11_IO_IN();//SET INPUT	 
    while (DHT11_DQ_IN&&retry<100)//DHT11会拉低40~80us
	{
		retry++;
		delay_us(1);
	};	 
	if(retry>=100)return 1;
	else retry=0;
    while (!DHT11_DQ_IN&&retry<100)//DHT11拉低后会再次拉高40~80us
	{
		retry++;
		delay_us(1);
	};
	if(retry>=100)return 1;	    
	return 0;
}
//从DHT11读取一个位
//返回值：1/0
u8 DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
	while(DHT11_DQ_IN&&retry<100)//等待变为低电平
	{
		retry++;
		delay_us(1);
	}
	retry=0;
	while(!DHT11_DQ_IN&&retry<100)//等待变高电平
	{
		retry++;
		delay_us(1);
	}
	delay_us(40);//等待40us
	if(DHT11_DQ_IN)return 1;
	else return 0;		   
}
//从DHT11读取一个字节
//返回值：读到的数据
u8 DHT11_Read_Byte(void)    
{        
    u8 i,dat;
    dat=0;
	for (i=0;i<8;i++) 
	{
   		dat<<=1; 
	    dat|=DHT11_Read_Bit();
    }						    
    return dat;
}
//从DHT11读取一次数据
//temp:温度值(范围:0~50°)
//humi:湿度值(范围:20%~90%)
//返回值：0,正常;1,读取失败
int32_t DHT11_Read_Data(uint8_t *pdht_data)    
{        
 	u8 buf[5];
	
	u8 i;
	DHT11_Rst();
	if(DHT11_Check()==0)
	{
		for(i=0;i<5;i++)//读取40位数据
		{
			buf[i]=DHT11_Read_Byte();
		}
		for(i=0;i<4;i++)//读取40位数据
		{
			pdht_data[i]=buf[i];
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
		{
			return 0;
		}
	}
	else return 1;
		    
}

//初始化DHT11的IO口 DQ 同时检测DHT11的存在
//返回1:不存在
//返回0:存在    	 
u8 DHT11_Init(void)
{	 
 	GPIO_InitTypeDef  GPIO_InitStructure;
 	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);	//使能PG端口时钟
	
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;				//PG11端口配置
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		//推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOG, &GPIO_InitStructure);				 	//初始化IO口
 	GPIO_SetBits(GPIOG,GPIO_Pin_11);						//PG11 输出高
			    
	DHT11_Rst();  //复位DHT11
	return DHT11_Check();//等待DHT11的回应
} 

//static GPIO_InitTypeDef		GPIO_InitStructure;

int32_t dht11_read(uint8_t *pdht_data)
{
	uint32_t t=0;
	
	uint8_t d;
	
	int32_t i=0;
	int32_t j=0;
	uint32_t check_sum=0;
	
	//保证引脚为输出模式
	DHT11_IO_OUT(); 	
	
	PGout(11)=0;
	delay_ms(20);
	
	PGout(11)=1;	
	delay_us(30);
	
	//保证引脚为输入模式
	DHT11_IO_IN();

	//等待DHT11响应，等待低电平出现
	t=0;
	while(PGin(11))
	{
		t++;
		delay_us(1);
		if(t >= 4000)
			return -1;
	}
	
	//若低电平超出100us
	t=0;
	while(PGin(11)==0)
	{
		t++;
		delay_us(1);
		if(t >= 100)
			return -2;
	}
	
	
	//若高电平超出100us
	t=0;
	while(PGin(11))
	{
		t++;
		delay_us(1);
		if(t >= 100)
			return -3;
	}
	
	//连续接收5个字节
	for(j=0; j<5; j++)
	{
		d = 0;
		//完成8个bit数据的接收，高位优先
		for(i=7; i>=0; i--)
		{
			//等待低电平持续完毕
			t=0;
			while(PGin(11)==0)
			{	
				t++;
				delay_us(1);
				if(t >= 100)
					return -4;
			}	
			
			delay_us(40);
			
			if(PGin(11))
			{
				d|=1<<i;
				//等待数据1的高电平时间持续完毕
				t=0;
				while(PGin(11))
				{
					t++;
					delay_us(1);
					if(t >= 100)
						return -5;
				}			
			
			}
		}	
		pdht_data[j] = d;
	}
	
	//通信的结束
	delay_us(100);
	
	//计算校验和
	check_sum=pdht_data[0]+pdht_data[1]+pdht_data[2]+pdht_data[3];
	
	check_sum = check_sum & 0xFF;
	if(check_sum != pdht_data[4])
		return -6;
	
	return 0;
}





