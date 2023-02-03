#include "rcc_control.h"

/*	
		select HSE 8MHZ with main PLL, 
		SYSCLK = 168 MHz
		AHB bus  = 168 MHz
		APB1 = 42 MHz
		APB2 = 84 MHz

*/

void RCC_InitSysClk()
{
	RCC->CR |= RCC_CR_HSEON;
	while(!RCC->CR & RCC_CR_HSERDY);
	
	RCC->PLLCFGR = 8 << RCC_PLLCFGR_PLLM_Pos |
								 336 << RCC_PLLCFGR_PLLN_Pos |
								 0 << RCC_PLLCFGR_PLLP_Pos |
								 7 << RCC_PLLCFGR_PLLQ_Pos |
								 RCC_PLLCFGR_PLLSRC;
	
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | RCC_CFGR_HPRE_DIV1;
	
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE1_Msk) | RCC_CFGR_PPRE1_DIV4;
	
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE2_Msk) | RCC_CFGR_PPRE2_DIV2;
	
	RCC->CR |= RCC_CR_PLLON;
	while(!RCC->CR & RCC_CR_PLLRDY);
	
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
	
	RCC->CR = (RCC->CR & ~RCC_CR_HSION);
	
	//PLL2 output 24MHZ
	
	//input HSE 8MHZ * PLLI2SN / PLLM / PLLI2SR = 24MHZ
  //    							48        8         2
	RCC->PLLI2SCFGR = 48 << RCC_PLLI2SCFGR_PLLI2SN_Pos |
										2 << RCC_PLLI2SCFGR_PLLI2SR_Pos;
										
										
	RCC->CR |= RCC_CR_PLLI2SON;
	while(!RCC->CR & RCC_CR_PLLI2SRDY);
	
	return;
}

