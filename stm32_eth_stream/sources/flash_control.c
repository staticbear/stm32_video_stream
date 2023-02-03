#include "flash_control.h"

#define FLASH_TIMEOUT_MAX	10000
#define ADDRSETTING				0x0800C000 // sector 3
#define SECTOR4SETTING 		3 

void FLASH_EnableCache()
{
	FLASH->ACR = FLASH_ACR_PRFTEN |
							 FLASH_ACR_ICEN |
							 FLASH_ACR_DCEN |
							 FLASH_ACR_LATENCY_5WS;
	return;
}

//---------------------------------------------------------------------

uint8_t FlashUnlock()
{
	if(FLASH->CR & FLASH_CR_LOCK)// if flash memory is locked
	{
		FLASH->KEYR = 0x45670123;  // write key 1
		FLASH->KEYR = 0xCDEF89AB;  // write key 2
		if(FLASH->CR & FLASH_CR_LOCK)return 0;
	}
	return 1;
}

//---------------------------------------------------------------------

void FlashLock()
{
	FLASH->CR=FLASH_CR_LOCK;
}

//---------------------------------------------------------------------

int SectorErase(uint8_t sector)
{
	int timeout;
	
	if(sector > 11)
		return 1;
	
	if(FLASH->CR & FLASH_CR_LOCK)
		return 2;
	
	timeout = FLASH_TIMEOUT_MAX;
	while(FLASH->SR & FLASH_SR_BSY)
	{
		if((timeout--) == 0) return 3;
	}
	
	FLASH->CR = FLASH_CR_SER | ((sector & 0x0F) << 3);
	FLASH->CR |= FLASH_CR_STRT;
	
	timeout = FLASH_TIMEOUT_MAX;
	while(FLASH->SR & FLASH_SR_BSY)
	{
		if((timeout--) == 0) return 4;
	}
	
	return 0;
}

//---------------------------------------------------------------------

uint8_t WriteToFlash(uint8_t *addr, uint8_t data)
{
	int timeout;
	
	if(FLASH->CR & FLASH_CR_LOCK)
		return 1;
	
	timeout = FLASH_TIMEOUT_MAX;
	while(FLASH->SR & FLASH_SR_BSY)
	{
		if((timeout--) == 0) 
			return 2;
	}
	
	//write page
  FLASH->CR = (0 << 8) |									// programm size x8
							FLASH_CR_PG;   
	
	*addr = data;
	
	timeout = FLASH_TIMEOUT_MAX;
	while(FLASH->SR & FLASH_SR_BSY)
	{
		if((timeout--) == 0) 
			return 3;
	}
	
	//check
	if(*addr != data)
		return 4;
	
	return 0;
}

//---------------------------------------------------------------------

int FLASH_LoadSettings(uint8_t *ptrData, int data_size)
{
	uint8_t *ptrSrc = (uint8_t *)ADDRSETTING;
	uint8_t *ptrDst = ptrData;
	uint16_t crc = 0;
	
	for(int i = 0; i < data_size; i++){
		ptrDst[i] = ptrSrc[i];
		crc += ptrSrc[i];
	}
	
	if(crc != *(uint16_t*)&ptrSrc[data_size])
		return 1;
	
	return 0;
}

//---------------------------------------------------------------------

int FLASH_SaveSettings(uint8_t *ptrData, int data_size)
{
	int ret;
	uint16_t crc;
	
	if(FlashUnlock() == 0)
		return 1;
	
	SectorErase(SECTOR4SETTING);
	
	uint8_t *ptrDst = (uint8_t *)ADDRSETTING;
	uint8_t *ptrSrc = ptrData;
	crc = 0;
	
	for(int i = 0; i < data_size; i++){
		ret = WriteToFlash(&ptrDst[i], ptrSrc[i]);
		if(ret != 0)break;
		crc += ptrSrc[i];
	}
	
	ret = WriteToFlash(&ptrDst[data_size], (uint8_t)(crc & 0xFF));
	if(ret != 0){
		FlashLock();
		return ret;
	}
	
	ret = WriteToFlash(&ptrDst[data_size + 1], (uint8_t)(crc >> 8));
	FlashLock();

	return ret;
}
