#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "usart2.h"
#include "led.h"
#include "dht11.h"
#include "oled.h"
//#include "rc522.h"
#include "cjson.h"
#include "cJson_test.h"

#include "includes.h"

/****************************************************************
*名    称:输气站场气体泄漏检测系统
*作    者:何蔚
*创建日期:2020/10/14 建立系统框架
*当前日期:2020/11/16 完成V1.0
*当前日期:2020/12/24 完成V2.0
*当前日期:2021/01/08 完成V3.0
*当前日期:2021/01/18 完成V4.0
*当前日期:2021/02/16 未完成V4.0
*任务：
	1.开始任务 创建信号量 消息队列 互斥锁 事件标志组 任务
	2.DHT11温湿度采集,单总线采集，  	互斥锁 oled显示		数据通过消息队列发送数据到线程存储、转发任务
	3.TDLAS气体浓度采集，串口接收浓度数据，互斥锁 oled显示	数据通过消息队列发送数据到线程存储、转发任务        
	4.MQ135 浓度采集
	5.MQ4 浓度采集
	6.看门狗
	7.本地存储任务 等待多个内核对象 消息队列接收数据 以txt格式存入SD卡
	8.LORA转发 等待多个内核对象 消息队列接收数据 usart3 发送至上位机
	9.RTC时间显示 互斥锁 oled显示	
	10.LED0 信号量接受数据 报警
	11.LED1 系统运行提示
	12.BEEP 信号量接受数据 报警
	13.KEY 
	14.任务状态
	
	
*说  明:		
	当前代码尽可能实现了模块化编程，一个任务管理一个硬件。最简单的
	led、蜂鸣器都由单独任务管理。
*****************************************************************/

//V1.0 完成任务和内核创建 传感器数据采集 json数据封装
//V2.0 完成OLED显示 串口响应  LORA任务中dht11和TDLAS的消息队列传输
//V3.0 完成上位机对下位机的数据问询
//V4.0 完成lora信息传输 


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
#define FLOAT_TASK_PRIO		6
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
OS_MUTEX				g_mutex_TDLAS;
OS_MUTEX				g_mutex_DHT11;
OS_MUTEX				g_mutex_NODE;

/*****************************消息队列的对象*******************************/
OS_Q	 				g_queue_usart1;	
OS_Q	 				g_queue_usart2;				
OS_Q	 				g_queue_usart3;				

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

//节点结构体
struct NODE
{
	int device_id;
	int lora_address;
	int lora_channel;
	char* temperature;
	char* humidity;
	char* CH4concentration;
} *node1,node_1;


//存放DHT11和tdlas传感器数据
char temp_buf[16] = {0};
char humi_buf[16] = {0};
char TDLAS[20] = {0};


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


int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	
	/**************************硬件初始化*************************/
	delay_init();       //延时初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //中断分组配置
	uart_init(115200);    //串口波特率设置
	usart2_init(115200);    //串口波特率设置
	LED_Init();         //LED初始化
	OLED_Init();		//初始化OLED
	//RC522_Init();	    //初始化射频卡芯片
	
	while(DHT11_Init());//温湿度传感器的初始化
	
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区
	
	//1.创建开始任务
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

//任务1.开始任务函数
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
	
	//创建消息队列，用于usart2发送至TDLAS
	OSQCreate(&g_queue_usart1,"g_queue_usart1",16,&err);
	OSQCreate(&g_queue_usart2,"g_queue_usart2",16,&err);
	OSQCreate(&g_queue_usart2,"g_queue_usart3",16,&err);
	
	//创建消息队列，用于dht11发送至lora
	OSQCreate(&g_queue_dht11_to_lora,"g_queue_dht11_to_lora",16,&err);
	OSQCreate(&g_queue_dht11_to_txt,"g_queue_dht11_to_txt",16,&err);
	
	OSQCreate(&g_queue_TDLAS_to_lora,"g_queue_TDLAS_to_lora",16,&err);
	OSQCreate(&g_queue_TDLAS_to_txt,"g_queue_TDLAS_to_txt",16,&err);
	
	OSQCreate(&g_queue_MQ135_to_lora,"g_queue_MQ135_to_lora",16,&err);
	OSQCreate(&g_queue_MQ135_to_txt,"g_queue_MQ135_to_txt",16,&err);
	
	OSQCreate(&g_queue_MQ4_to_lora,"g_queue_MQ4_to_lora",16,&err);
	OSQCreate(&g_queue_MQ4_to_txt,"g_queue_MQ4_to_txt",16,&err);

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
void json_test(struct NODE* node)
{
	/* Build the JSON array [[1, 2], {"cool": true}] */
	/* print the version */
	cJSON *root = NULL;
	cJSON *Node = NULL;
	char *out = NULL;
	char *end = "#"; //数据尾部标识符
	//printf("Version: %s\n", cJSON_Version());

	/* Now some samplecode for building objects concisely: */	
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root,"reason",(const char *)"success");
	cJSON_AddItemToObject(root,"node_1",Node = cJSON_CreateObject());
	cJSON_AddNumberToObject(Node,"DeviceID",node->device_id);
	cJSON_AddNumberToObject(Node,"LORA_ADD",node->lora_address);
	cJSON_AddNumberToObject(Node,"LORA_CHN",node->lora_channel);
	cJSON_AddStringToObject(Node, "Temp", node->temperature);
	cJSON_AddStringToObject(Node, "humi", node->humidity);
	cJSON_AddStringToObject(Node, "CH4_concentration", (const char *)node->CH4concentration);

	/* Print to text */
//	if (print_preallocated(root) != 0) {
//		cJSON_Delete(root);
//		return ;
//	}

	out = cJSON_Print(root); 
	strcat(out,end);//添加数据尾部标识符
	dgb_printf_safe("%s",out);
	
	cJSON_Delete(root);	
	free(out);
		
}

//任务2 DHT11温湿度采集 
void DHT11_task(void *p_arg)
{
	OS_ERR err;
	
	//DHT11 温湿度
	uint8_t dht11_data[5] = {0};
	char buf[16] = {0};
	//调试
	dgb_printf_safe("DHT11 task running\r\n");
	
	//LCD_ShowString(30,130,200,16,16,"DHT11 OK");
	//POINT_COLOR=BLUE;//设置字体为蓝色 
	
	while(1)
	{
		//读取温湿度值							  
		DHT11_Read_Data(dht11_data);	
		//发送消息给LORA任务
		OSQPost((OS_Q*		)&g_queue_dht11_to_lora,		
				(void*		)dht11_data,
				(OS_MSG_SIZE)sizeof(dht11_data),
				(OS_OPT		)OS_OPT_POST_FIFO,
				(OS_ERR*	)&err);
				
		//发送给txt任务
		//OSQPost(&g_queue_dht11_to_txt,
				//(void *)dht11_data,
				//sizeof(dht11_data),
				//OS_OPT_POST_FIFO,
				//&err);
		
		//组合词条
		sprintf((char *)buf,"T:%02d.%dC H:%02d.%d%%",dht11_data[2],dht11_data[3],dht11_data[0],dht11_data[1]);
		//sprintf((char *)temp_buf,"%02d.%d",dht11_data[2],dht11_data[3]);
		//sprintf((char *)humi_buf,"%02d.%d",dht11_data[0],dht11_data[1]);
		
		//发送至usart1,进行调试
		//dgb_printf_safe("%s\r\n",buf);

#if 0					
		//lcd显示
		OSMutexPend(&g_mutex_lcd,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		LCD_ShowString(30,150,100,16,16,temp_buf);
		LCD_ShowString(30,170,100,16,16,humi_buf);
		OSMutexPost(&g_mutex_lcd,OS_OPT_NONE,&err);		
#endif	 

		//OLED显示
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		OLED_ShowString(0,2,buf,16);		
		OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);

		//延时发生任务调度
		delay_ms(4000);

	}
}

//任务3TDLAS气体浓度采集，串口接收浓度数据，互斥锁 oled显示	数据通过消息队列发送数据到线程存储、转发任务
void TDLAS_task(void *p_arg)
{
	OS_ERR err;	
	
	int i = 0;
	uint8_t TDLAS_buf[30];
	uint8_t *TDLAS_res=NULL;
	OS_MSG_SIZE TDLAS_size;
	dgb_printf_safe("TDLAS task running\r\n");
	
	while(1)
	{		
		
#if 1	
		//等待消息队列发生类型错误
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
		
		if(USART2_RX_STA&0x8000)
		{                                           
			int len=USART2_RX_STA&0x3FFF;//得到此次接收数据的长度
			//dgb_printf_safe("len: %d \r\n",len);
			for(i = 0;i < len;i++)
			{
				TDLAS[i] = USART2_RX_BUF[i];
			}
			//dgb_printf_safe("TDLAS:%s\r\n",USART2_RX_BUF);
			//dgb_printf_safe("TDLAS:%s\r\n",TDLAS);
			USART2_RX_STA = 0;
		}	
#endif 
		//延时发生任务调度
		delay_ms(1000);
	}
}


//任务4  mq135 气体浓度采集	
void MQ135_task(void *parg)
{
	OS_ERR err;
	//dgb_printf_safe("MQ135 task running\r\n");
	
	while(1)
	{
		delay_ms(100);
	}
}

//任务5 mq4 气体浓度采集	
void MQ4_task(void *parg)
{
	OS_ERR err;
	//dgb_printf_safe("MQ4 task running\r\n");
	
	while(1)
	{
		delay_ms(100);
	}
}

//任务6 看门狗 
void IWG_task(void *parg)
{
	OS_ERR err;
	//dgb_printf_safe("IWG task running\r\n");
	
	while(1)
	{
		delay_ms(100);
	}
}

//任务7 等待多个内核对象 消息队列接收数据 以txt格式存入SD卡
void SAVE_task(void *parg)
{
	OS_ERR err;
	//dgb_printf_safe("SAVE task running\r\n");
	
	while(1)
	{
		delay_ms(100);
	}
}

//任务8 LORA转发 等待多个内核对象 消息队列接收数据 usart3 发送至上位机
void LORA_task(void *p_arg)
{
	OS_ERR err;  
	int i = 0;
	char *dht11_data_recv = NULL;
	OS_MSG_SIZE dht11_data_size;
	char buf[16] = {0};
	char USART_buf[30];
	OS_MSG_SIZE USART_size;
	char *USART_res = NULL;
	
	//等待多个内核对象
	OS_OBJ_QTY index;
	OS_PEND_DATA pend_multi_tbl[CORE_OBJ_NUM];	
	pend_multi_tbl[0].PendObjPtr=(OS_PEND_OBJ*)&g_queue_usart1;
	pend_multi_tbl[1].PendObjPtr=(OS_PEND_OBJ*)&g_queue_dht11_to_lora;
	
	node1 = (struct NODE*)malloc(sizeof(struct NODE));
	
	dgb_printf_safe("LORA task running\r\n");
	
	while(1)
	{
#if 1	
		//等待多个内核对象
		index = OSPendMulti((OS_PEND_DATA*	)pend_multi_tbl,		//内核对象数组	
							(OS_OBJ_QTY		)CORE_OBJ_NUM,			//内核数量
							(OS_TICK	 	)0,						//永远等待下去
							(OS_OPT        	)OS_OPT_PEND_BLOCKING,
							(OS_ERR*		)&err);
		if(err != OS_ERR_NONE)
		{
			dgb_printf_safe("[app_task_lora_dht11][OSQPend]Error Code = %d\r\n",err);
			continue;
		}
		//如果就绪的是信号量
		if(pend_multi_tbl[0].RdyObjPtr == (OS_PEND_OBJ *)(&g_queue_usart1)){
			//dgb_printf_safe("\r\ncheck\r\n");
			if(USART_RX_STA&0x8000)
			{                                           
				int len=USART_RX_STA&0x3FFF;//得到此次接收数据的长度
				//dgb_printf_safe("usart1_len: %d \r\n",len);
				//dgb_printf_safe("TDLAS:%s\r\n",USART_RX_BUF);
					
				if(strstr((const char *)USART_RX_BUF,"check"))
				{
					OSMutexPend(&g_mutex_NODE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
					json_test(node1);
					OSMutexPost(&g_mutex_NODE,OS_OPT_POST_NONE,&err);
				}
				
				USART_RX_STA = 0;
			}	
		}
		else if(pend_multi_tbl[1].RdyObjPtr == (OS_PEND_OBJ *)(&g_queue_dht11_to_lora))
		{
			dht11_data_recv = pend_multi_tbl[1].RdyMsgPtr;
			sprintf((char *)buf,"T:%02d.%02dC H:%02d.%02d%%",dht11_data_recv[2],dht11_data_recv[3],dht11_data_recv[0],dht11_data_recv[1]);
			sprintf((char *)temp_buf,"%02d.%d",dht11_data_recv[2],dht11_data_recv[3]);
			sprintf((char *)humi_buf,"%02d.%d",dht11_data_recv[0],dht11_data_recv[1]);
			//dgb_printf_safe("%s\r\n",buf);
			//dgb_printf_safe("\r\n收到消息队列\r\n");

			OSMutexPend(&g_mutex_NODE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
			node1->device_id = 1;
			node1->lora_address = 10;
			node1->lora_channel = 23;
			node1->humidity = humi_buf;
			node1->temperature = temp_buf;
			node1->CH4concentration = TDLAS;
			//json_test(node1);
			OSMutexPost(&g_mutex_NODE,OS_OPT_POST_NONE,&err);
			
			memset((void *)buf,0,sizeof(buf));
			//memset((void *)temp_buf,0,sizeof(temp_buf));
			//memset((void *)humi_buf,0,sizeof(humi_buf));
		}
#endif	
	
#if 0
		//等待dht11消息队列
		dht11_data_recv = OSQPend((OS_Q*			)&g_queue_dht11_to_lora,   
								(OS_TICK		)0,
								(OS_OPT			)OS_OPT_PEND_BLOCKING,
								(OS_MSG_SIZE*	)&dht11_data_size,		
								(CPU_TS*		)0,
								(OS_ERR*		)&err);
		//正确接收到消息
		if(dht11_data_recv && dht11_data_size)
		{
			//dgb_printf_safe("recv from dht11 %d\r\n",err);
			//组合词条
			//sprintf((char *)buf,"T:%02d.%dC H:%02d.%d%%",dht11_data_recv[2],dht11_data_recv[3],dht11_data_recv[0],dht11_data_recv[1]);
			//sprintf((char *)temp_buf,"%02d.%d",dht11_data_recv[2],dht11_data_recv[3]);
			//sprintf((char *)humi_buf,"%02d.%d",dht11_data_recv[0],dht11_data_recv[1]);
			//sprintf((char *)CH4_buf,"5000");	
			
			node1->device_id = 10;
			node1->lora_address = 10;
			node1->lora_channel = 23;
			node1->humidity = humi_buf;
			node1->temperature = temp_buf;
			node1->CH4concentration = TDLAS;
			
			//发送至usart1,进行调试
			//dgb_printf_safe("%s\r\n",buf);
			json_test(node1);
		
			memset((void *)buf,0,sizeof(buf));
			memset((void *)temp_buf,0,sizeof(temp_buf));
			memset((void *)humi_buf,0,sizeof(humi_buf));
			memset((void *)CH4_buf,0,sizeof(CH4_buf));
		}		
#endif

#if 0	
		//等待消息队列
		USART_res = OSQPend((OS_Q*			)&g_queue_usart1,
							(OS_TICK		)0,
							(OS_OPT			)OS_OPT_PEND_BLOCKING,
							(OS_MSG_SIZE*	)&USART_size,
							(CPU_TS*		)NULL,
							(OS_ERR*		)&err);
		
		if(err != OS_ERR_NONE)
		{
			dgb_printf_safe("[_task_usart][OSQPend]Error Code = %d\r\n",err);		
		}
		
		if(USART_RX_STA&0x8000)
		{                                           
			int len=USART_RX_STA&0x3FFF;//得到此次接收数据的长度	
			//dgb_printf_safe("usart1_len: %d \r\n",len);
			//dgb_printf_safe("TDLAS:%s\r\n",USART_RX_BUF);
		
			if(strstr((const char *)USART_RX_BUF,"check"))
			{
				json_test(node1);
			}
			
			USART_RX_STA = 0;
		}	
#endif 
			
		delay_ms(1000);
		
	}
}
//任务9 rtc时间显示	互斥锁 oled显示	
void RTC_task(void *parg)
{
	OS_ERR err;
	//dgb_printf_safe("BEEP task running\r\n");
	
	while(1)
	{
		delay_ms(100);
	}
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
void led1_task(void *p_arg)
{
	OS_ERR err;
	p_arg = p_arg;
	while(1)
	{
		LED1=~LED1;
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err); //延时500ms
	}
}

//任务12.声音报警任务
void BEEP_task(void *p_arg)
{
	OS_ERR err;
	//dgb_printf_safe("BEEP task running\r\n");
	
	while(1)
	{
		delay_ms(100);
	}
}

//任务13.按键任务
void KEY_task(void *p_arg)
{
	OS_ERR err;

	OS_FLAGS flags=0;
	
	//dgb_printf_safe("KEY task running\r\n");
	
	while(1)
	{
#if 0
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
#endif		
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
void float_task(void *p_arg)
{
	CPU_SR_ALLOC();
	static float float_num=0.01;
	while(1)
	{
		
		//RC522_Handel();
		
		
		
		
#if 0
		float_num+=0.01f;
		OS_CRITICAL_ENTER();	//进入临界区
		printf("float_num的值为: %.4f\r\n",float_num);
		OS_CRITICAL_EXIT();		//退出临界区
#endif	
		delay_ms(500);			//延时500ms
	}
}
