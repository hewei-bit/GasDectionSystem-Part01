/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2013; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           MASTER INCLUDES
*
*                                       IAR Development Kits
*                                              on the
*
*                                    STM32F429II-SK KICKSTART KIT
*
* Filename      : includes.h
* Version       : V1.00
* Programmer(s) : FT
*********************************************************************************************************
*/

#ifndef  INCLUDES_MODULES_PRESENT
#define  INCLUDES_MODULES_PRESENT


/*
*********************************************************************************************************
*                                         STANDARD LIBRARIES
*********************************************************************************************************
*/


#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <math.h>


/*
*********************************************************************************************************
*                                                 OS
*********************************************************************************************************
*/

#include  <os.h>


/*
*********************************************************************************************************
*                                              LIBRARIES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_def.h>
#include  <lib_ascii.h>
#include  <lib_math.h>
#include  <lib_mem.h>
#include  <lib_str.h>

/*
*********************************************************************************************************
*                                              APP / BSP
*********************************************************************************************************
*/

#include  <app_cfg.h>
#include  <bsp.h>
//#include  <bsp_int.h>

//事件标志组：硬件使用到的标志位
#define FLAG_GRP_RTC_WAKEUP					0x01
#define FLAG_GRP_RTC_ALARM_A				0x02
#define FLAG_GRP_RTC_ALARM_B				0x04

#define FLAG_GRP_KEY0_DOWN					0x10
#define FLAG_GRP_KEY1_DOWN					0x20
#define FLAG_GRP_KEY2_DOWN					0x40
#define FLAG_GRP_WK_UP_DOWN					0x80


#define MAX_CONVERTED_VALUE 	4095  	//最大转换值
#define VREF					3300	//最大电压值
#define QUEUE_NUM		10				//消息队列长度
#define CORE_OBJ_NUM	2				//内核对象个数，一共3个：2个信号量和一个消息队列						


//节点结构体
typedef struct 
{
	int device_id;
	int lora_address;
	int lora_channel;
	char temperature[10];
	char humidity[10];
	char CH4concentration[10];
} NODE;


extern OS_FLAG_GRP			g_flag_grp;		

extern OS_MUTEX				g_mutex_printf;		//互斥锁的对象

extern OS_Q	 				g_queue_usart1;				//消息队列的对象
extern OS_Q	 				g_queue_usart2;				//消息队列的对象
extern OS_Q	 				g_queue_usart3;				//消息队列的对象
extern OS_Q	 				g_queue_usart4;				//消息队列的对象
extern OS_Q	 				g_queue_usart5;				//消息队列的对象


extern void dgb_printf_safe(const char *format, ...);





#endif



