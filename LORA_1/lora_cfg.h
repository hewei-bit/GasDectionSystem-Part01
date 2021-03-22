#ifndef _LORA_CFG_H
#define _LORA_CFG_H


//设备参数定义
typedef struct
{
   u16 addr;//设备地址
   u8 chn;//信道
   u8 power;//发射功率
   u8 wlrate;//空中速率
   u8 wltime;//休眠时间
   u8 mode;//工作模式
   u8 mode_sta;//发送状态
   u8 bps;//串口波特率
   u8 parity;//校验位
}LoRa_CFG;

//空中速率(单位:Kbps)
#define  LORA_RATE_0K3  0 //0.3
#define  LORA_RATE_1K2  1 //1.2
#define  LORA_RATE_2K4  2 //2.4
#define  LORA_RATE_4K8  3 //4.8
#define  LORA_RATE_9K6  4 //9.6
#define  LORA_RATE_19K2 5 //19.2

//休眠时间(单位:秒)
#define LORA_WLTIME_1S  0  //1秒
#define LORA_WLTIME_2S  1  //2秒

//工作模式
#define LORA_MODE_GEN   0   //一般模式
#define LORA_MODE_WK    1   //唤醒模式
#define LORA_MODE_SLEEP 2   //省电模式

//发射功率  
#define LORA_PW_11dBm  0   //11dBm
#define LORA_PW_14Bbm  1   //14dBm
#define LORA_PW_17Bbm  2   //17dBm
#define LORA_PW_20Bbm  3   //20dBm

//发送状态
#define LORA_STA_Tran 0 //透明传输
#define LORA_STA_Dire 1 //定向传输

//串口波特率(单位:bps)
#define LORA_TTLBPS_1200    0  //1200
#define LORA_TTLBPS_2400    1  //2400
#define LORA_TTLBPS_4800    2  //4800
#define LORA_TTLBPS_9600    3  //9600
#define LORA_TTLBPS_19200   4  //19200
#define LORA_TTLBPS_38400   5  //38400
#define LORA_TTLBPS_57600   6  //57600
#define LORA_TTLBPS_115200  7  //115200

//串口数据校验
#define LORA_TTLPAR_8N1  0 //8位数据
#define LORA_TTLPAR_8E1  1 //8位数据+1位偶校验
#define LORA_TTLPAR_8O1  2 //8位数据+1位奇校验


//设备出厂默认参数
#define LORA_ADDR    14               //设备地址
#define LORA_CHN     23               //通信信道
#define LORA_POWER   LORA_PW_20Bbm    //发射功率
#define LORA_RATE    LORA_RATE_19K2   //空中速率
#define LORA_WLTIME  LORA_WLTIME_1S   //休眠时间
#define LORA_MODE    LORA_MODE_GEN    //工作模式
#define LORA_STA     LORA_STA_Tran    //发送状态
#define LORA_TTLBPS  LORA_TTLBPS_115200 //波特率
#define LORA_TTLPAR  LORA_TTLPAR_8N1  //校验位   


#endif
