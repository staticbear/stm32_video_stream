#ifndef _FLASH_CONTROL_H_
#define _FLASH_CONTROL_H_


#include "stm32f4xx.h"

void FLASH_EnableCache(void);
int FLASH_SaveSettings(uint8_t *ptrData, int data_size);
int FLASH_LoadSettings(uint8_t *ptrData, int data_size);

#endif
