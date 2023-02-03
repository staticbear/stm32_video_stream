#ifndef _OV9655_H_
#define _OV9655_H_

#include "stm32f4xx.h"

#define I2C_TIMEOUT_MAX  10000

#define ADDR_WRITE	0x60
#define ADDR_READ	  0x61

#define FRAME_BEGIN	0x55AA

enum {
	STREAM_OFF = 0x00,
	STREAM_ON = 0x01,
	STREAM_ERR = 0x02,
};

void I2C1_Init(void);
void GPIO_InitCAMPins(void);
void DCMI_CAM_Init(void);	
void DCMI_CAM_Disable(void);
void DCMI_DMA_Init(uint16_t transfer_size, uint32_t addr_buf0, uint32_t addr_buf1);
void DCMI_DMA_Disable(void);
int I2C1_Write(uint8_t addr_8bit, uint8_t data_8bit);
int I2C1_Read(uint8_t addr_8bit, uint8_t *data_8bit);

int DCMI_DMA_Start(uint16_t dma_transfer_size_in_bytes);
void DCMI_DMA_Stop(void);

#endif
