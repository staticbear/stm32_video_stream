#ifndef _TIMERS_H_
#define _TIMERS_H_

#include "stm32f4xx.h"

#define TIMERS_CNT 1

void TIM7Common_Init(void);
void SetTimer(uint8_t timer_n, uint32_t mlsec);
uint32_t GetTimerVal(uint8_t timer_n);

// general purpose timers
enum { 
	T1 = 0,
};
	

#endif
