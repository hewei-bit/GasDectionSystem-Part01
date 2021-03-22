//核心头文件
#include "includes.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
//串口
#include "usart2.h"
#include "usart3.h"
#include "uart4.h"
//外设
#include "lora_app.h"
#include "led.h"
#include "beep.h"
#include "dht11.h"
#include "mq135.h"
#include "oled.h"
#include "key.h"
#include "rtc.h"

/****************************************************************
*名    称:输气站场气体泄漏检测系统
*作    者:何蔚
*创建日期:2020/10/14 建立系统框架
*当前日期:2020/11/16 完成V1.0
*当前日期:2020/12/24 完成V2.0
*当前日期:2021/01/08 完成V3.0
*当前日期:2021/03/09 完成V4.0

*当前日期:2021/03/10 完成定点检测节点代码
*当前日期:2021/03/11 完成遥测检测节点代码
*当前日期:2021/03/12 完成中继节点代码
*当前日期:2021/03/14 完成V5.0

*任务：
	1.开始任务 创建信号量 消息队列 互斥锁 事件标志组 任务
	2.DHT11温湿度采集,单总线采集，  	互斥锁 oled显示		数据通过消息队列发送数据到线程存储、转发任务
	3.TDLAS气体浓度采集，串口接收浓度数据，互斥锁 oled显示	数据通过消息队列发送数据到线程存储、转发任务        
	(该节点不使用)4.MQ135 浓度采集
	5.MQ4 浓度采集
	(该节点不使用)6.看门狗
	(该节点不使用)7.本地存储任务 等待多个内核对象 消息队列接收数据 以txt格式存入SD卡
	8.LORA转发 等待多个内核对象 消息队列接收数据 usart3 发送至上位机
	9.RTC时间显示 互斥锁 oled显示	
	10.LED0 信号量接受数据 报警
	11.LED1 系统运行提示
	12.BEEP 信号量接受数据 报警
	13.KEY 
	14.任务状态
	15.float
	(该节点不使用)16.mpu6050 六轴传感器 显示当前云台角度数据 
*说  明:		
	当前代码尽可能实现了模块化编程，一个任务管理一个硬件。最简单的
	led、蜂鸣器都由单独任务管理。
*****************************************************************/

//V1.0 完成任务和内核创建 传感器数据采集 json数据封装
//V2.0 完成OLED显示 串口响应  LORA任务中dht11和TDLAS的消息队列传输
//V3.0 完成上位机对下位机的数据问询
//V4.0 完成lora信息传输 
//V5.0 系统组网

/*****************************定义任务堆栈*************************************/
//UCOSIII中以下优先级用户程序不能使用，ALIENTEK
//将这些优先级分配给了UCOSIII的5个系统内部任务
//优先级0：中断服务服务管理任务 OS_IntQTask()
//优先级1：时钟节拍任务 OS_TickTask()
//优先级2：定时任务 OS_TmrTask()
//优先级OS_CFG_PRIO_MAX-2：统计任务 OS_StatTask()
//优先级OS_CFG_PRIO_MAX-1：空闲任务 OS_IdleTask()


//任务1 开始任务
#define START_TASK_PRIO		3	
#define START_STK_SIZE 		512
OS_TCB StartTaskTCB;
CPU_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *p_arg);

//任务2 DHT11温湿度采集 
#define DHT11_TASK_PRIO 	4
#define DHT11_STK_SIZE		128
OS_TCB DHT11_Task_TCB;
CPU_STK DHT11_TASK_STK[DHT11_STK_SIZE];
void DHT11_task(void *parg);

//任务3 TDLAS 气体浓度采集	usart2
#define TDLAS_TASK_PRIO 	5
#define TDLAS_STK_SIZE		512
OS_TCB TDLAS_Task_TCB;
CPU_STK TDLAS_TASK_STK[TDLAS_STK_SIZE];
void TDLAS_task(void *parg);

//任务4  mq135 气体浓度采集	
#define MQ135_TASK_PRIO 	5
#define MQ135_STK_SIZE		128
OS_TCB MQ135_Task_TCB;
CPU_STK MQ135_TASK_STK[MQ135_STK_SIZE];
void MQ135_task(void *parg);

//任务5 mq4 气体浓度采集	
#define MQ4_TASK_PRIO 	5
#define MQ4_STK_SIZE		128
OS_TCB MQ4_Task_TCB;
CPU_STK MQ4_TASK_STK[MQ4_STK_SIZE];
void MQ4_task(void *parg);

//任务6 看门狗 
#define IWG_TASK_PRIO 		6
#define IWG_STK_SIZE		128
OS_TCB IWG_Task_TCB;
CPU_STK IWG_TASK_STK[IWG_STK_SIZE];
void IWG_task(void *parg);

//任务7 等待多个内核对象 消息队列接收数据 以txt格式存入SD卡
#define SAVE_TASK_PRIO 		9
#define SAVE_STK_SIZE		512
OS_TCB SAVE_Task_TCB;
CPU_STK SAVE_TASK_STK[SAVE_STK_SIZE];
void SAVE_task(void *parg);

//任务8 LORA转发 等待多个内核对象 消息队列接收数据 usart3 发送至上位机
#define LORA_TASK_PRIO 		8
#define LORA_STK_SIZE		512
OS_TCB LORA_Task_TCB;
CPU_STK LORA_TASK_STK[LORA_STK_SIZE];
void LORA_task(void *parg);

//任务9 rtc时间显示	互斥锁 oled显示	
#define RTC_TASK_PRIO 		9
#define RTC_STK_SIZE		128
OS_TCB RTC_Task_TCB;
CPU_STK RTC_TASK_STK[RTC_STK_SIZE];
void RTC_task(void *parg);

//任务10 LED0任务 
#define LED0_TASK_PRIO		10
#define LED0_STK_SIZE 		128
OS_TCB Led0TaskTCB;
CPU_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *p_arg);

//任务11 LED1任务
#define LED1_TASK_PRIO		11	
#define LED1_STK_SIZE 		128
OS_TCB Led1TaskTCB;
CPU_STK LED1_TASK_STK[LED1_STK_SIZE];
void led1_task(void *p_arg);

//任务12.BEEP
#define BEEP_TASK_PRIO		12
#define BEEP_STK_SIZE 		128
OS_TCB BEEP_Task_TCB;
CPU_STK BEEP_TASK_STK[BEEP_STK_SIZE];
void BEEP_task(void *p_arg);

//任务13.key
#define KEY_TASK_PRIO		13
#define KEY_STK_SIZE 		128
OS_TCB KEY_Task_TCB;
CPU_STK KEY_TASK_STK[KEY_STK_SIZE];
void KEY_task(void *p_arg);

//任务14.任务状态
#define TASK_STA_TASK_PRIO		20
#define TASK_STA_STK_SIZE		128
OS_TCB	TASK_STA_Task_TCB;
CPU_STK	TASK_STA_TASK_STK[TASK_STA_STK_SIZE];
void TASK_STA_task(void *p_arg);


//任务15 任务运行提示
#define FLOAT_TASK_PRIO		21
#define FLOAT_STK_SIZE		128
OS_TCB	FloatTaskTCB;
__align(8) CPU_STK	FLOAT_TASK_STK[FLOAT_STK_SIZE];
void float_task(void *p_arg);


/*****************************事件标志组的对象******************************/
OS_FLAG_GRP				g_flag_grp;			

/*******************************信号量的对象******************************/
OS_SEM					g_sem_led;		
OS_SEM					g_sem_beep;			

/*******************************互斥锁的对象******************************/
OS_MUTEX				g_mutex_printf;	
OS_MUTEX				g_mutex_oled;		
OS_MUTEX				g_mutex_lcd;

OS_MUTEX				g_mutex_NODE;

/*****************************消息队列的对象*******************************/
OS_Q	 				g_queue_usart1;	
OS_Q	 				g_queue_usart2;				
OS_Q	 				g_queue_usart3;	
OS_Q	 				g_queue_usart4;	
OS_Q	 				g_queue_usart5;	

OS_Q					g_queue_dht11_to_lora;		//消息队列的对象
OS_Q					g_queue_dht11_to_txt;		//消息队列的对象

OS_Q					g_queue_TDLAS_to_lora;		//消息队列的对象
OS_Q					g_queue_TDLAS_to_txt;		//消息队列的对象

OS_Q					g_queue_MQ135_to_lora;		//消息队列的对象
OS_Q					g_queue_MQ135_to_txt;		//消息队列的对象

OS_Q					g_queue_MQ4_to_lora;		//消息队列的对象
OS_Q					g_queue_MQ4_to_txt;			//消息队列的对象

#define CORE_OBJ_NUM	2	//内核对象个数，一共3个：2个信号量和一个消息队列						

uint32_t 				g_oled_display_flag=1;
uint32_t 				g_oled_display_time_count=0;

extern __IO int MQ135_ADC_ConvertedValue;

//修改节点结构体
NODE node_1;
NODE *node1 = &node_1;

//节点初始化
void node_init(void)
{
	sprintf((char *)node_1.device_id,"%d",4);
	sprintf((char *)node_1.lora_address,"%d", LORA_ADDR);
	sprintf((char *)node_1.lora_channel,"%d", LORA_CHN);
	strcpy(node_1.temperature,"25.0");
	strcpy(node_1.humidity,"50.0");
	strcpy(node_1.CH4concentration,"000.00");
	strcpy((char *)node_1.over,"\r\n");	
}

//互斥访问usart1
#define DEBUG_PRINTF_EN	1
void dgb_printf_safe(const char *format, ...)
{
#if DEBUG_PRINTF_EN	
	OS_ERR err;
	
	va_list args;
	va_start(args, format);
	
	OSMutexPend(&g_mutex_printf,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
	vprintf(format, args);
	OSMutexPost(&g_mutex_printf,OS_OPT_POST_NONE,&err);
	
	va_end(args);
#else
	(void)0;
#endif
}

void oled_showString_safe(int x,int y,char *string,int size,OS_ERR err)
{
	OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		sprintf(string,"TDLAS: %d",x);
		OLED_ShowString(x,y,(uint8_t *)string,size);
	OSMutexPost(&g_mutex_oled,OS_OPT_NONE,&err);
}


/*******************************************
		1.硬件初始化
		2.串口初始化
		3.start_task创建
*********************************************/
int main(void)
{
	OS_ERR err;
	char node_message[16] = {0};
	CPU_SR_ALLOC();
	
	node_init();		//node结构体初始化
	
	delay_init();       //延时初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //中断分组配置
	
	uart_init(115200);    	//串口波特率设置
	usart2_init(9600);    //串口波特率设置
	uart4_init(115200);    	//串口波特率设置

	while(LoRa_Init())		//初始化ATK-LORA-01模块
	{
		printf("未检测到    LORA   模块!!! \r\n");		
		delay_ms(300);
	}
	LoRa_Set();			//初始化ATK-LORA-01模块
	Lora_mode = 0;      //标记"接收模式"
	set_Already = 1;
		
	LED_Init();         //LED初始化
	KEY_Init();			//KEY初始化 
	BEEP_Init(); 		//BEEP初始化
	
	while(DHT11_Init())//温湿度传感器的初始化
	{
		printf("未检测到   DHT11   模块!!! \r\n");		
		delay_ms(300);
	}
	mq135_init();		//mq135初始化
	
	
	OLED_Init();		//初始化OLED
	start_oled_ui();	//oled初始化配置
	sprintf(node_message,"CHN:%d ADDR:%d",My_LoRa_CFG.chn,My_LoRa_CFG.addr);
	OLED_ShowString(0,0,(uint8_t *)node_message,16);	
	
	RTC_Init();
	
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);  //开启UCOSIII
	while(1);
}

//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif
	
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	
	OS_CRITICAL_ENTER();	//进入临界区
	
	//创建事件标志组，所有标志位初值为0
	OSFlagCreate(&g_flag_grp,		"g_flag_grp",0,&err);
	
	//创建信号量，初值为0，有一个资源
	OSSemCreate(&g_sem_led,"g_sem_led",0,&err);
	OSSemCreate(&g_sem_beep,"g_sem_beep",0,&err);	
	
	//创建互斥量
	OSMutexCreate(&g_mutex_printf,	"g_mutex_printf",&err);	
	OSMutexCreate(&g_mutex_oled,	"g_mutex_oled",&err);
	OSMutexCreate(&g_mutex_lcd,		"g_mutex_olcd",&err);

	OSMutexCreate(&g_mutex_NODE,		"g_mutex_NODE",&err);
	
	//创建消息队列，用于usart2发送至TDLAS
	OSQCreate(&g_queue_usart1,"g_queue_usart1",16,&err);
	OSQCreate(&g_queue_usart2,"g_queue_usart2",16,&err);
	OSQCreate(&g_queue_usart3,"g_queue_usart3",16,&err);
	OSQCreate(&g_queue_usart4,"g_queue_usart4",16,&err);
	OSQCreate(&g_queue_usart5,"g_queue_usart5",16,&err);
	
	//创建消息队列，用于dht11发送至lora
	OSQCreate(&g_queue_dht11_to_lora,"g_queue_dht11_to_lora",16,&err);
	OSQCreate(&g_queue_dht11_to_txt,"g_queue_dht11_to_txt",16,&err);
	
	OSQCreate(&g_queue_TDLAS_to_lora,"g_queue_TDLAS_to_lora",16,&err);
	OSQCreate(&g_queue_TDLAS_to_txt,"g_queue_TDLAS_to_txt",16,&err);
	
	OSQCreate(&g_queue_MQ135_to_lora,"g_queue_MQ135_to_lora",16,&err);
	OSQCreate(&g_queue_MQ135_to_txt,"g_queue_MQ135_to_txt",16,&err);
	
	OSQCreate(&g_queue_MQ4_to_lora,"g_queue_MQ4_to_lora",16,&err);
	OSQCreate(&g_queue_MQ4_to_txt,"g_queue_MQ4_to_txt",16,&err);
	
	dgb_printf_safe("start_task task running\r\n");
	
	
	
	//2.创建DHT11任务
	OSTaskCreate((OS_TCB 	* )&DHT11_Task_TCB,		
				 (CPU_CHAR	* )"DHT11 task", 		
                 (OS_TASK_PTR )DHT11_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )DHT11_TASK_PRIO,     	
                 (CPU_STK   * )&DHT11_TASK_STK[0],	
                 (CPU_STK_SIZE)DHT11_STK_SIZE/10,	
                 (CPU_STK_SIZE)DHT11_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);		
	
	//3.创建TDLAS任务
	OSTaskCreate((OS_TCB 	* )&TDLAS_Task_TCB,		
				 (CPU_CHAR	* )"TDLAS task", 		
                 (OS_TASK_PTR )TDLAS_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )TDLAS_TASK_PRIO,     	
                 (CPU_STK   * )&TDLAS_TASK_STK[0],	
                 (CPU_STK_SIZE)TDLAS_STK_SIZE/10,	
                 (CPU_STK_SIZE)TDLAS_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
	
	//4.创建mq135任务
	OSTaskCreate((OS_TCB 	* )&MQ135_Task_TCB,		
				 (CPU_CHAR	* )"MQ135 task", 		
                 (OS_TASK_PTR )MQ135_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )MQ135_TASK_PRIO,     	
                 (CPU_STK   * )&MQ135_TASK_STK[0],	
                 (CPU_STK_SIZE)MQ135_STK_SIZE/10,	
                 (CPU_STK_SIZE)MQ135_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
				 
	//5.创建mq4任务
	OSTaskCreate((OS_TCB 	* )&MQ4_Task_TCB,		
				 (CPU_CHAR	* )"MQ4 task", 		
                 (OS_TASK_PTR )MQ4_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )MQ4_TASK_PRIO,     	
                 (CPU_STK   * )&MQ4_TASK_STK[0],	
                 (CPU_STK_SIZE)MQ4_STK_SIZE/10,	
                 (CPU_STK_SIZE)MQ4_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
		
	//7.创建SAVE任务
	OSTaskCreate((OS_TCB 	* )&SAVE_Task_TCB,		
				 (CPU_CHAR	* )"SAVE task", 		
                 (OS_TASK_PTR )SAVE_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )SAVE_TASK_PRIO,     	
                 (CPU_STK   * )&SAVE_TASK_STK[0],	
                 (CPU_STK_SIZE)SAVE_STK_SIZE/10,	
                 (CPU_STK_SIZE)SAVE_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
				 
	//8.创建LORA任务
	OSTaskCreate((OS_TCB 	* )&LORA_Task_TCB,		
				 (CPU_CHAR	* )"LORA task", 		
                 (OS_TASK_PTR )LORA_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LORA_TASK_PRIO,     	
                 (CPU_STK   * )&LORA_TASK_STK[0],	
                 (CPU_STK_SIZE)LORA_STK_SIZE/10,	
                 (CPU_STK_SIZE)LORA_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
	
	//9.创建RTC任务
	OSTaskCreate((OS_TCB 	* )&RTC_Task_TCB,		
				 (CPU_CHAR	* )"RTC task", 		
                 (OS_TASK_PTR )RTC_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )RTC_TASK_PRIO,     	
                 (CPU_STK   * )&RTC_TASK_STK[0],	
                 (CPU_STK_SIZE)RTC_STK_SIZE/10,	
                 (CPU_STK_SIZE)RTC_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
	
	//10.创建LED0任务
	OSTaskCreate((OS_TCB 	* )&Led0TaskTCB,		
				 (CPU_CHAR	* )"led0 task", 		
                 (OS_TASK_PTR )led0_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED0_TASK_PRIO,     
                 (CPU_STK   * )&LED0_TASK_STK[0],	
                 (CPU_STK_SIZE)LED0_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED0_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);				
				 
	//11.创建LED1任务
	OSTaskCreate((OS_TCB 	* )&Led1TaskTCB,		
				 (CPU_CHAR	* )"led1 task", 		
                 (OS_TASK_PTR )led1_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED1_TASK_PRIO,     	
                 (CPU_STK   * )&LED1_TASK_STK[0],	
                 (CPU_STK_SIZE)LED1_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED1_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);	
	
	//12.创建BEEP任务
	OSTaskCreate((OS_TCB 	* )&BEEP_Task_TCB,		
				 (CPU_CHAR	* )"BEEP task", 		
                 (OS_TASK_PTR )BEEP_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )BEEP_TASK_PRIO,     	
                 (CPU_STK   * )&BEEP_TASK_STK[0],	
                 (CPU_STK_SIZE)BEEP_STK_SIZE/10,	
                 (CPU_STK_SIZE)BEEP_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);			 
	
	//13.创建KEY任务
	OSTaskCreate((OS_TCB 	* )&KEY_Task_TCB,		
				 (CPU_CHAR	* )"KEY task", 		
                 (OS_TASK_PTR )KEY_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )KEY_TASK_PRIO,     	
                 (CPU_STK   * )&KEY_TASK_STK[0],	
                 (CPU_STK_SIZE)KEY_STK_SIZE/10,	
                 (CPU_STK_SIZE)KEY_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);	
				 
	//14.创建TASK_STA任务
	OSTaskCreate((OS_TCB 	* )&TASK_STA_Task_TCB,		
				 (CPU_CHAR	* )"TASK_STA task", 		
                 (OS_TASK_PTR )TASK_STA_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )TASK_STA_TASK_PRIO,     	
                 (CPU_STK   * )&TASK_STA_TASK_STK[0],	
                 (CPU_STK_SIZE)TASK_STA_STK_SIZE/10,	
                 (CPU_STK_SIZE)TASK_STA_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);
				 
	//15.创建浮点测试任务
	OSTaskCreate((OS_TCB 	* )&FloatTaskTCB,		
				 (CPU_CHAR	* )"float test task", 		
                 (OS_TASK_PTR )float_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )FLOAT_TASK_PRIO,     	
                 (CPU_STK   * )&FLOAT_TASK_STK[0],	
                 (CPU_STK_SIZE)FLOAT_STK_SIZE/10,	
                 (CPU_STK_SIZE)FLOAT_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, 
                 (OS_ERR 	* )&err);	
				 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
	OS_CRITICAL_EXIT();	//进入临界区
}


//任务2 DHT11温湿度采集 
void DHT11_task(void *p_arg)
{
	OS_ERR err;
	
	//DHT11 温湿度
	char buf[16] = {0};
	uint8_t dht11_data[5] = {0};
	//调试
	dgb_printf_safe("DHT11 task running\r\n");
	
	while(1)
	{
		//读取温湿度值							  
		DHT11_Read_Data(dht11_data);	
		
		//组合词条
		sprintf((char *)buf,"T:%02d.%dC H:%02d.%d%%",dht11_data[2],dht11_data[3],dht11_data[0],dht11_data[1]);
		
		//赋值结构体
		OSMutexPend(&g_mutex_NODE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		sprintf((char *)node_1.temperature,"%02d.%d",dht11_data[2],dht11_data[3]);
		sprintf((char *)node_1.humidity,"%02d.%d",dht11_data[0],dht11_data[1]);
		OSMutexPost(&g_mutex_NODE,OS_OPT_POST_NONE,&err);
		
		//OLED显示
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		OLED_ShowString(0,2,(uint8_t *)buf,16);		
		OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);

#if 1
		//调试专用
		//发送至usart1,进行调试
		//dgb_printf_safe("%s\r\n",buf);

#endif 
		
#if 0	
		//消息队列
		//发送消息给LORA任务
		OSQPost((OS_Q*		)&g_queue_dht11_to_lora,		
				(void*		)dht11_data,
				(OS_MSG_SIZE)sizeof(dht11_data),
				(OS_OPT		)OS_OPT_POST_FIFO,
				(OS_ERR*	)&err);
				
#endif

		//延时发生任务调度
		delay_ms(1000);
	}
}


//任务3TDLAS气体浓度采集，串口接收浓度数据，互斥锁 oled显示	数据通过消息队列发送数据到线程存储、转发任务
void TDLAS_task(void *p_arg)
{
	OS_ERR err;	
	
	int i = 0;
	int flag = 0;
	
	//消息队列接收结果
	uint8_t *TDLAS_res=NULL;
	OS_MSG_SIZE TDLAS_size;
	char TDLAS[20] = {0};
	
	//定点模块浓度
	int concen = 0;
	
	//遥测模块浓度
	int gq_val = 0;
	char gq_str[2] = {0};
	int nd_val = 0;
	char nd_str[5] = {0};
	//最终输出
	char Fix_result[20] = {0};
	char telemetry_result_light[20] = {0};
	char telemetry_result_CH4[20] = {0};
	
	
	dgb_printf_safe("TDLAS task running\r\n");
	
	while(1)
	{		
	
#if 1	
		//等待消息队列
		TDLAS_res = OSQPend((OS_Q*			)&g_queue_usart2,
							(OS_TICK		)0,
							(OS_OPT			)OS_OPT_PEND_BLOCKING,
							(OS_MSG_SIZE*	)&TDLAS_size,
							(CPU_TS*		)NULL,
							(OS_ERR*		)&err);
		
		if(err != OS_ERR_NONE)
		{
			dgb_printf_safe("[TDLAS_task_usart2][OSQPend]Error Code = %d\r\n",err);		
		}
		
		//转换TDLAS数值，并合成输出字符串

		if(USART2_RX_STA&0x8000)
		{                                           
			int len=USART2_RX_STA&0x3FFF;//得到此次接收数据的长度
			for(i = 0;i < len;i++)
			{
				TDLAS[i] = USART2_RX_BUF[i];
			}

			//dgb_printf_safe("TDLAS:%s\r\n",USART2_RX_BUF);
			//dgb_printf_safe("TDLAS:%s\r\n",TDLAS);
			USART2_RX_STA = 0;
		}	

#if 0
	
		//遥测模块需要截取字符串
		//光照强度部分转换
		strncpy(gq_str, TDLAS+8, 2);
		//dgb_printf_safe(" %s \r\n",gq_str);
		gq_val = atoi(gq_str);
		//dgb_printf_safe("Light intensity : %d \r\n",gq_val);
		sprintf(telemetry_result_light,"Light: %d",gq_val);
		
		//浓度部分转换
		strncpy(nd_str, TDLAS+21, 5);
		//dgb_printf_safe(" %s \r\n",nd_str);
		nd_val = atoi(nd_str);
		//dgb_printf_safe("CH4 concentration : %d \r\n",nd_val);
		sprintf(telemetry_result_CH4,"CH4: %d",nd_val);
		
		
		//在OLED上显示
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		OLED_ShowString(0,4,(uint8_t *)telemetry_result_CH4,16);
		OLED_ShowString(56,4,(uint8_t *)telemetry_result_light,16);
		OSMutexPost(&g_mutex_oled,OS_OPT_NONE,&err);
#endif

#if 1		
		OSMutexPend(&g_mutex_NODE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		strcpy(node_1.CH4concentration,TDLAS);
		OSMutexPost(&g_mutex_NODE,OS_OPT_POST_NONE,&err);
		
		//定点模块直接转换即可
		concen = atoi(TDLAS);
		sprintf(Fix_result,"TDLAS: %d ppm",concen);	

		//在OLED上显示
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		OLED_ShowString(0,4,(uint8_t *)Fix_result,20);
		OSMutexPost(&g_mutex_oled,OS_OPT_NONE,&err);

#endif

#endif 


	}
}
	
//任务4  mq135 氨气、硫化物、笨系蒸汽 气体浓度采集	
void MQ135_task(void *parg)
{
	OS_ERR err;

	//mq135转换值
	float MQ135_ADC_ConvertedValue_Local;
	float AIR_Quality;
	float AIR_Quality2;
	uint8_t air_1_buf[16] = {0};
	uint8_t air_2_buf[16] = {0};
	
	dgb_printf_safe("MQ135 task running\r\n");
	
	while(1)
	{
		
#if 0 
		//把电平的模拟信号转换成数值  公式：转换后 = 浮点数 ADC值 /4096 * 3.3
		MQ135_ADC_ConvertedValue_Local = (float)MQ135_ADC_ConvertedValue/4096*3.3;
		
		//空气质量检测值的转换公式 公式：ADC数值 * 3300 /4095
		AIR_Quality = ((float)MQ135_ADC_ConvertedValue_Local * VREF)/MAX_CONVERTED_VALUE;
		
		//再修正一次
		//AIR_Quality2 = AIR_Quality/1000;
		
		//组合词条
		sprintf((char *)air_1_buf,"AIR_1:%2.3f",AIR_Quality);
		sprintf((char *)air_2_buf,"CH4:%2.3f ppm ",AIR_Quality);


	 	//OLED显示浓度
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		OLED_ShowString(0,5,air_2_buf,16);
		OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
#endif
		
		//延时发生任务调度
		delay_ms(1000);
	}
}
	
//任务5 mq4 甲烷 气体浓度采集	
void MQ4_task(void *parg)
{
	OS_ERR err;
	
	/*************************
	MQ4 与 MQ135 区别不大因此使用相同的转换口，
	无天然气环境下，实测AOUT端的电压为0.0.725V，
	当检测到天然气时，电压升高0.1V，实际检测到的气体浓度增加200ppm
	*******************************************/

	//mq135转换值
	int MQ135_ADC_ConvertedValue_Local;
	float CH4_ppm;
	float Voltage;
	char MQ4[20] = {0};

	dgb_printf_safe("MQ4 task running\r\n");
	
	while(1)
	{
		//把电平的模拟信号转换成数值  公式：转换后 = 浮点数 ADC值 /4096 * 3.3
		Voltage = MQ135_ADC_ConvertedValue/4096*3.3;
		
//无天然气环境下,实测AOUT端电压为1.29V,当检测到天然气时,每升高0.1V,实际被测气体升高200ppm
		CH4_ppm = (Voltage - 1.29) / 0.1 * 200;
	
		//组合词条
		sprintf((char *)MQ4,"CH4:%2.3f ppm ",CH4_ppm);

		sprintf((char *)node_1.CH4concentration,"%2.3f",CH4_ppm);
		
		//OLED显示浓度
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		OLED_ShowString(0,4,(uint8_t *)MQ4,16);
		OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
	
		//延时发生任务调度
		delay_ms(2000);
	}
}

//任务6 看门狗 
void IWG_task(void *parg)
{
	
}
	
//任务7 等待多个内核对象 消息队列接收数据 以txt格式存入SD卡
void SAVE_task(void *parg)
{
	
}


//任务8 LORA转发 等待多个内核对象 消息队列接收数据 usart3 发送至上位机
void Send_Node(NODE *p,u8 len)     
{
    static u8 date,i;	   
	for(i=0;i<len;i++)
	{
		date= *(((u8*) &p->device_id)+i);    
		USART_SendData(USART3,date);   
		while( USART_GetFlagStatus(USART3,USART_FLAG_TC)!= SET); 
	}
}


void LORA_task(void *p_arg)
{
	OS_ERR err; 
	//消息队列接收结果
	int i=0;
	uint8_t *LORA_res=NULL;
	OS_MSG_SIZE LORA_size;
	char LORA[100] = {0};
	
	dgb_printf_safe("LORA task running\r\n");

	
	while(1)
	{
		
#if 0
		//等待消息队列
		LORA_res = OSQPend((OS_Q*			)&g_queue_usart3,
							(OS_TICK		)0,
							(OS_OPT			)OS_OPT_PEND_BLOCKING,
							(OS_MSG_SIZE*	)&LORA_size,
							(CPU_TS*		)NULL,
							(OS_ERR*		)&err);
		
		if(err != OS_ERR_NONE)
		{
			dgb_printf_safe("[LORA_task][OSQPend]Error Code = %d\r\n",err);		
		}
		
		//数据接收
		if(USART3_RX_STA&0x8000)
		{                                           
			int len=USART3_RX_STA&0x3FFF;//得到此次接收数据的长度
			for(i = 0;i < len;i++)
			{
				LORA[i] = USART3_RX_BUF[i];
			}
			dgb_printf_safe("LORA:%s\r\n",LORA);
			
			//OLED显示
			OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
			OLED_ShowString(0,4,(uint8_t *)LORA,16);		
			OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
		
			USART3_RX_STA = 0;
		}	
		
#endif 	
		
		
		OSMutexPend(&g_mutex_NODE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
			
		//发送结构体	
		Send_Node(&node_1,sizeof(NODE));
		//LoRa_SendData();
		printf("%s\r\n",node_1.humidity);
		printf("%s\r\n",node_1.temperature);
		
		OSMutexPost(&g_mutex_NODE,OS_OPT_POST_NONE,&err);
		
		//两秒发送一次
		delay_ms(2000);	
	}
	
}
	
//任务9 rtc时间显示	互斥锁 oled显示	
void RTC_task(void *parg)
{
	OS_ERR err;
	u8 t = 0;	
	char date_time[20] = {0};
	dgb_printf_safe("RTC task running\r\n");
	
	
//OLED显示时间		
#if 1		
	
	while(1)
	{
		if(t!=calendar.sec)
		{
			t=calendar.sec;
			OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
			
			
			sprintf(date_time,"%d:%d:%d %d:%d",
			calendar.w_year,calendar.w_month,calendar.w_date,calendar.hour,calendar.min);
			OLED_ShowString(0,7,(uint8_t *)date_time,20);
//			switch(calendar.week)
//			{
//				case 0:
//					LCD_ShowString(60,232,200,16,16,(u8 *)"Sunday   ");
//					break;
//				case 1:
//					LCD_ShowString(60,232,200,16,16,(u8 *)"Monday   ");
//					break;
//				case 2:
//					LCD_ShowString(60,232,200,16,16,(u8 *)"Tuesday  ");
//					break;
//				case 3:
//					LCD_ShowString(60,232,200,16,16,(u8 *)"Wednesday");
//					break;
//				case 4:
//					LCD_ShowString(60,232,200,16,16,(u8 *)"Thursday ");
//					break;
//				case 5:
//					LCD_ShowString(60,232,200,16,16,(u8 *)"Friday   ");
//					break;
//				case 6:
//					LCD_ShowString(60,232,200,16,16,(u8 *)"Saturday ");
//					break;  
//			}

			OSMutexPost(&g_mutex_oled,OS_OPT_NONE,&err);	
		}	
		delay_ms(1000);
	}
	
#endif	
	
	
//LCD显示时间		
#if 0	
	//显示时间框框
	POINT_COLOR=BLUE;
	OSMutexPend(&g_mutex_lcd,0,OS_OPT_PEND_BLOCKING,NULL,&err);
	LCD_ShowString(60,200,200,16,16,"    -  -  ");	   
	LCD_ShowString(60,216,200,16,16,"  :  :  ");	
	OSMutexPost(&g_mutex_lcd,OS_OPT_NONE,&err);		
	
	while(1)
	{
		if(t!=calendar.sec)
		{
			t=calendar.sec;
			OSMutexPend(&g_mutex_lcd,0,OS_OPT_PEND_BLOCKING,NULL,&err);
			LCD_ShowNum(60,200,calendar.w_year,4,16);									  
			LCD_ShowNum(100,200,calendar.w_month,2,16);									  
			LCD_ShowNum(124,200,calendar.w_date,2,16);	 
			switch(calendar.week)
			{
				case 0:
					LCD_ShowString(60,232,200,16,16,(u8 *)"Sunday   ");
					break;
				case 1:
					LCD_ShowString(60,232,200,16,16,(u8 *)"Monday   ");
					break;
				case 2:
					LCD_ShowString(60,232,200,16,16,(u8 *)"Tuesday  ");
					break;
				case 3:
					LCD_ShowString(60,232,200,16,16,(u8 *)"Wednesday");
					break;
				case 4:
					LCD_ShowString(60,232,200,16,16,(u8 *)"Thursday ");
					break;
				case 5:
					LCD_ShowString(60,232,200,16,16,(u8 *)"Friday   ");
					break;
				case 6:
					LCD_ShowString(60,232,200,16,16,(u8 *)"Saturday ");
					break;  
			}
			LCD_ShowNum(60,216,calendar.hour,2,16);									  
			LCD_ShowNum(84,216,calendar.min,2,16);									  
			LCD_ShowNum(108,216,calendar.sec,2,16);
			OSMutexPost(&g_mutex_lcd,OS_OPT_NONE,&err);	
		}	
		delay_ms(1000);
	}
#endif	
	

}
	
//任务10.系统运行提示 led0任务函数
void led0_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	while(1)
	{
		LED0=0;
		OSTimeDlyHMSM(0,0,0,200,OS_OPT_TIME_HMSM_STRICT,&err); //延时200ms
		LED0=1;
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err); //延时500ms
	}
}

//任务11.光报警任务 led1任务函数
// LED1 = 1 时LED启动
// LED1 = 0 时LED关闭
void led1_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	int concen = 0;
	while(1)
	{
		OSMutexPend(&g_mutex_NODE,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		concen = atoi(node_1.CH4concentration);
		OSMutexPost(&g_mutex_NODE,OS_OPT_POST_NONE,&err);
		
		//dgb_printf_safe("concen: %d \r\n",concen);
		if(concen > 20)
		{	
			LED1 = 1;
			delay_ms(1000);
			LED1 = 0;	
			delay_ms(2000);
		}
		else{
			LED1 = 0;	
		}
	}
}


//任务12.声音报警任务
// BEEP = 1 时蜂鸣器启动
// BEEP = 0 时蜂鸣器关闭
void BEEP_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	int concen = 0;
			
	dgb_printf_safe("BEEP task running\r\n");

	while(1)
	{	
		OSMutexPend(&g_mutex_NODE,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		concen = atoi(node_1.CH4concentration);
		OSMutexPost(&g_mutex_NODE,OS_OPT_POST_NONE,&err);
		
		//dgb_printf_safe("concen: %d \r\n",concen);
		if(concen > 20)
		{	
			BEEP = 0;
			delay_ms(1000);	
			BEEP = 1;
			delay_ms(1000);
		}
		else{
			BEEP = 0;	
		}
	}
}
	

//任务13.按键任务
void KEY_task(void *p_arg)
{
	OS_ERR err;

	OS_FLAGS flags=0;
	
	while(1)
	{
		//一直阻塞等待事件标志置1，等待成功后，将对应清0
		flags = OSFlagPend(&g_flag_grp,FLAG_GRP_KEY0_DOWN
									|FLAG_GRP_KEY1_DOWN
									|FLAG_GRP_KEY2_DOWN
									|FLAG_GRP_WK_UP_DOWN,
									0,OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME+OS_OPT_PEND_BLOCKING,NULL,&err);
		
		if(err != OS_ERR_NONE)
		{
			//dgb_printf_safe("[task1][OSFlagPend]Error Code = %d\r\n",err);
			continue;
		}
		
		//WK_UP摁下
		if(flags & FLAG_GRP_WK_UP_DOWN){	
			//禁止EXTI0触发中断
			NVIC_DisableIRQ(EXTI0_IRQn);
			if(WK_UP == 1){
				delay_ms(20);//消抖
				if(WK_UP == 1){
					//dgb_printf_safe("WK_UP happend\r\n");
					//等待按键WK_UP释放
					while(WK_UP == 1){
						delay_ms(1);
					}						
					
					BEEP = !BEEP;
				}
			}	
			//允许EXTI0触发中断
			NVIC_EnableIRQ(EXTI0_IRQn);	
			//清空EXTI0中断标志位
			EXTI_ClearITPendingBit(EXTI_Line0);			
		}
		
		//KEY2摁下
		if(flags & FLAG_GRP_KEY2_DOWN){	
			if(KEY2 == 0){
				delay_ms(20);//消抖
				if(KEY2 == 0){
					//dgb_printf_safe("key2 happend\r\n");
					//等待按键2释放
					while(KEY2==0){
						delay_ms(1);
					}	
					
					LED0 = !LED0;
				}
			}	
			//清空EXTI2中断标志位
			EXTI_ClearITPendingBit(EXTI_Line2);			
		}
		
		//KEY1摁下
		if(flags & FLAG_GRP_KEY1_DOWN){	
			if(KEY1 == 0){
				delay_ms(20);//消抖
				if(KEY1 == 0){
					//dgb_printf_safe("key1 happend\r\n");
					//等待按键1释放
					while(KEY1==0){
						delay_ms(1);
					}	
					//发送消息
//					OSSemPost(&g_json,OS_OPT_POST_1,&err);//发送信号量
				}
			}	
			//清空EXTI3中断标志位
			EXTI_ClearITPendingBit(EXTI_Line3);			
		}
		
//		//KEY0摁下
//		if(flags & FLAG_GRP_KEY0_DOWN){	
//			if(KEY0 == 0){
//				delay_ms(20);//消抖
//				if(KEY0 == 0){
//					dgb_printf_safe("key0 happend\r\n");
//					//等待按键0释放
//					while(KEY0==0){
//						delay_ms(1);
//					}		
//					
//					LED1 = !LED1;
//					LED0 = !LED0;
//				}
//			}	
//			//清空EXTI4中断标志位
//			EXTI_ClearITPendingBit(EXTI_Line4);			
//		}
		
		delay_ms(1000);
	}
}
	
//任务14.系统内存占用监视
void TASK_STA_task(void *p_arg)
{
	OS_ERR err;  
	
	CPU_STK_SIZE free,used; 
	
	//dgb_printf_safe("TASK_STA task running\r\n");
	
	delay_ms(3000);
	
	while(1)
	{

#if 0
		OSTaskStkChk (&DHT11_Task_TCB,&free,&used,&err); 
		dgb_printf_safe("app_task_ir    stk[used/free:%d/%d usage:%d%%]\r\n",used,free,(used*100)/(used+free)); 
	
		OSTaskStkChk (&LORA_Task_TCB,&free,&used,&err); 
		dgb_printf_safe("app_task_key   stk[used/free:%d/%d usage:%d%%]\r\n",used,free,(used*100)/(used+free));
	
		OSTaskStkChk (&RTC_Task_TCB,&free,&used,&err); 
		dgb_printf_safe("app_task_usart1   stk[used/free:%d/%d usage:%d%%]\r\n",used,free,(used*100)/(used+free));	
		
		OSTaskStkChk (&SAVE_Task_TCB,&free,&used,&err); 
		dgb_printf_safe("app_task_mpu6050  stk[used/free:%d/%d usage:%d%%]\r\n",used,free,(used*100)/(used+free));			

		OSTaskStkChk (&LED0_Task_TCB,&free,&used,&err); 
		dgb_printf_safe("app_task_rtc   stk[used/free:%d/%d usage:%d%%]\r\n",used,free,(used*100)/(used+free));  	  			  

		OSTaskStkChk (&LED1_Task_TCB,&free,&used,&err); 
		dgb_printf_safe("app_task_dht11 stk[used/free:%d/%d usage:%d%%]\r\n",used,free,(used*100)/(used+free));  

		OSTaskStkChk (&BEEP_Task_TCB,&free,&used,&err); 
		dgb_printf_safe("BEEP_task   stk[used/free:%d/%d usage:%d%%]\r\n",used,free,(used*100)/(used+free));

		OSTaskStkChk (&TASK_STA_Task_TCB,&free,&used,&err); 
		dgb_printf_safe("app_task_sta   stk[used/free:%d/%d usage:%d%%]\r\n",used,free,(used*100)/(used+free));
#endif		


			
		delay_ms(6000);
	}
}

//任务15.浮点测试任务
//系统运行监测
void float_task(void *p_arg)
{
	OS_ERR err; 
	CPU_SR_ALLOC();
	static float float_num=0.01;
	char node_message[16] = {0};
	
	while(1)
	{
		float_num+=0.01f;
		dgb_printf_safe("float_num的值为: %.4f\r\n",float_num);

		sprintf(node_message,"CHN:%d ADDR:%d",My_LoRa_CFG.chn,My_LoRa_CFG.addr);
		
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		OLED_Clear();
		OLED_ShowString(0,0,(uint8_t *)node_message,16);
		OSMutexPost(&g_mutex_oled,OS_OPT_NONE,&err);

		delay_ms(2000);			//延时500ms
	}
}
