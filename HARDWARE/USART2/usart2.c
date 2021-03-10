#include "sys.h"
#include "delay.h"
#include "usart2.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#if SYSTEM_SUPPORT_OS
#include "includes.h" //ucos 使用
#endif

//////////////////////////////////////////////////////////////////////////////////

u8 USART2_RX_BUF[USART2_MAX_RECV_LEN]; //接收缓冲,最大USART2_MAX_RECV_LEN个字节.
u8 USART2_TX_BUF[USART2_MAX_SEND_LEN]; //发送缓冲,最大USART2_MAX_SEND_LEN字节

vu16 USART2_RX_STA = 0;

void usart2_init(u32 bound)
{

    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); //使能USART2，GPIOA时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    //USART2_TX   GPIOA.2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);          //初始化GPIOA.2

    //USART2_RX	  GPIOA.3初始化
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;             //PA3
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);                //初始化GPIOA.3

    //Usart2 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);                           //根据指定的参数初始化VIC寄存器

    //USART 初始化设置

    USART_InitStructure.USART_BaudRate = bound;                                     //串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     //字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          //一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 //收发模式

    USART_Init(USART2, &USART_InitStructure);      //初始化串口2
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); //开启串口接受中断
    USART_Cmd(USART2, ENABLE);                     //使能串口2
}

//接收TDLAS模块浓度数据
void USART2_IRQHandler(void) //串口2中断服务程序
{
    u8 Res;
    OS_ERR err;

    int len = 0;
    int t = 0;

#ifdef SYSTEM_SUPPORT_OS
    //进入中断
    OSIntEnter();
#endif

    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //接收中断(接收到的数据必须是0x0d 0x0a结尾)
    {
        Res = USART_ReceiveData(USART2); //读取接收到的数据

        if ((USART2_RX_STA & 0x8000) == 0) //接收未完成
        {
            if (USART2_RX_STA & 0x4000) //接收到了0x0d
            {
                if (Res != 0x0a)
                    USART2_RX_STA = 0; //接收错误,重新开始
                else
                    USART2_RX_STA |= 0x8000; //接收完成了
            }
            else //还没收到0X0D
            {
                if (Res == 0x0d)
                    USART2_RX_STA |= 0x4000;
                else
                {
                    USART2_RX_BUF[USART2_RX_STA & 0X3FFF] = Res;
                    USART2_RX_STA++;
                    if (USART2_RX_STA > (USART2_MAX_RECV_LEN - 1))
                        USART2_RX_STA = 0; //接收数据错误,重新开始接收
                }
            }
        }
    }

#if 1 //发送消息队列
    if (USART2_RX_STA & 0x8000)
    {     
		//dgb_printf_safe("TDLAS:%s\r\n",USART2_RX_BUF);
		len = USART2_RX_STA & 0x3FFF; //得到此次接收数据的长度
        OSQPost((OS_Q *)&g_queue_usart2,
                (void *)USART2_RX_BUF,
                (OS_MSG_SIZE)len,
                (OS_OPT)OS_OPT_POST_FIFO,
                (OS_ERR *)&err);
        if (err != OS_ERR_NONE)
        {
            dgb_printf_safe("[USART2_IRQHandler]OSQPost error code %d\r\n", err);
        }	
    }
#endif
	
#ifdef SYSTEM_SUPPORT_OS
    OSIntExit();
#endif
}


void u2_printf(char *fmt, ...)
{
    u16 i, j;
    va_list ap;
    va_start(ap, fmt);
    vsprintf((char *)USART2_TX_BUF, fmt, ap);
    va_end(ap);
    i = strlen((const char *)USART2_TX_BUF); //此次发送数据的长度
    for (j = 0; j < i; j++)                  //循环发送数据
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
            ;                                              //等待上次传输完成
        USART_SendData(USART2, (uint8_t)USART2_TX_BUF[j]); //发送数据到串口2
    }
}
