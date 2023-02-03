#include "timers.h"

static uint32_t timer_val[TIMERS_CNT];

//---------------------------------------------------------------------
// T = 1 mlsec
// APB1 42 MHz but timer multiple it x2!!!
void TIM7Common_Init()
{
	for(int i = 0; i < TIMERS_CNT; i++)
		timer_val[i] = 0;
	
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
	
	TIM7->PSC = 840;		 // 84MHz / 840 
	TIM7->ARR = 100;     // 1MHz / 100 = 1 mlsec
	
	TIM7->DIER = TIM_DIER_UIE;
	
	TIM7->CR1 = TIM_CR1_ARPE | 
							TIM_CR1_CEN;
	
	
	NVIC_EnableIRQ(TIM7_IRQn);
	NVIC_SetPriority(TIM7_IRQn, 0);
	
	return;
}

//---------------------------------------------------------------------
void TIM7_IRQHandler()
{
	TIM7->SR &= ~TIM_SR_UIF; 
	
	for(int i = 0; i < TIMERS_CNT; i++){
		if(timer_val[i] != 0)timer_val[i]--;
	}
	
	return;
}

//---------------------------------------------------------------------
void SetTimer(uint8_t timer_n, uint32_t mlsec)
{
	timer_val[timer_n] = mlsec;
	return;
}

//---------------------------------------------------------------------
uint32_t GetTimerVal(uint8_t timer_n)
{
	return timer_val[timer_n];
}
