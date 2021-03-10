#ifndef __BEEP_H__
#define __BEEP_H__


#include "sys.h"

//蜂鸣器端口定义
#define BEEP PBout(8)	// BEEP,蜂鸣器接口		   

void BEEP_Init(void);	//初始化
		 				    
#endif

