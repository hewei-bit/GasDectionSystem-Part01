#ifndef __MQ135_H
#define __MQ135_H

#include "stdio.h"	
#include "sys.h" 
#include "delay.h"

#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos สนำร	  
#endif

#define ADC1_DR_Address ((u32)0x4001244c);

void mq135_init(void);

#endif

