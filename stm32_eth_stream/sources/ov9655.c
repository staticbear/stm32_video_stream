#include "ov9655.h"
#include "ethernet.h"
#include "misc_func.h"

extern FIFO_T FIFO_TX_POOL;
extern FIFO_T FIFO_DATA_RDY;

uint32_t flag_stream_status;
uint32_t flag_new_frame;
uint32_t swap_offset;
uint16_t swap_value;

void GPIO_InitCAMPins()
{
	//PB8 I2C1_SCL +
	//PB9 I2C1_SDA +
	
	//PC6  DCMI_D0 +
	//PC7  DCMI_D1 +
	//PC8  DCMI_D2 +
	//PE1  DCMI_D3 +
	//PC11 DCMI_D4 +
	//PB6  DCMI_D5 +
	//PE5  DCMI_D6 +
	//PE6  DCMI_D7 +
	
	//PA6  DCMI_PIXCLK +
	//PB7  DCMI_VSYNC  +
	//PA4  DCMI_HREF   +
	
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOAEN;
	
	
	//PC6  DCMI_D0
	//PC7  DCMI_D1
	//PC8  DCMI_D2
	//PC11 DCMI_D4
	// configure ports for alternative functions
	GPIOC->MODER = (GPIOC->MODER & ~((3UL << 6 * 2) | (3UL < 7 * 2) | (3UL < 8 * 2) | (3UL < 11 * 2))) |
								 (2UL << 6 * 2) |
								 (2UL << 7 * 2) |
								 (2UL << 8 * 2) |
								 (2UL << 11 * 2);
	
	// set very hight speed
	GPIOC->OSPEEDR |= (3UL << 6 * 2) |
										(3UL << 7 * 2) |
										(3UL << 8 * 2) |
										(3UL << 11 * 2);
	
	// alternative functions - DCMI
	GPIOC->AFR[0] = (GPIOC->AFR[0] & ~((15UL << 7 * 4) | (15UL < 6 * 4))) |
									(13UL << 6 * 4) |
									(13UL << 7 * 4);  
									
	GPIOC->AFR[1] = (GPIOC->AFR[1] & ~((15UL << 3 * 4) | (15UL < 0 * 4))) |
									(13UL << 0 * 4) |
									(13UL << 3 * 4);  
									
									
	//PC9 as MCO2 with 24MHz
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	GPIOC->MODER = (GPIOC->MODER & ~(3UL << 9 * 2)) |
								 (2UL << 9 * 2);
								 
	GPIOC->OSPEEDR |= (3UL << 9 * 2);
	
	GPIOC->OTYPER &= ~(1UL << 9);	
	
	// alternative functions - MCO2
	GPIOC->AFR[1] = (GPIOC->AFR[1] & ~(15UL << 1 * 4)) |
									(0UL << 1 * 4); 
									
									
	//PE1  DCMI_D3								
	//PE5  DCMI_D6
	//PE6  DCMI_D7
	// configure ports for alternative functions
	GPIOE->MODER = (GPIOE->MODER & ~((3UL << 6 * 2) | (3UL < 5 * 2) | (3UL < 1 * 2))) |
								 (2UL << 1 * 2) |
								 (2UL << 5 * 2) |
								 (2UL << 6 * 2);
	
	// set very hight speed
	GPIOE->OSPEEDR |= (3UL << 1 * 2) |
										(3UL << 5 * 2) |
										(3UL << 6 * 2);
	
	// alternative functions - DCMI
	GPIOE->AFR[0] = (GPIOE->AFR[0] & ~((15UL << 6 * 4) | (15UL < 5 * 4)| (15UL < 1 * 4))) |
									(13UL << 1 * 4) |
									(13UL << 5 * 4) |
									(13UL << 6 * 4);  
									
 
	
	//PB6  DCMI_D5
	//PB7  DCMI_VSYNC
	// configure ports for alternative functions
	GPIOB->MODER = (GPIOB->MODER & ~((3UL << 7 * 2) | (3UL < 6 * 2))) |
								 (2UL << 6 * 2) |
								 (2UL << 7 * 2);
	
	// set very hight speed
	GPIOB->OSPEEDR |= (3UL << 6 * 2) |
										(3UL << 7 * 2);
	
	// alternative functions - DCMI
	GPIOB->AFR[0] = (GPIOB->AFR[0] & ~((15UL << 7 * 4) | (15UL < 6 * 4))) |
									(13UL << 6 * 4) |
									(13UL << 7 * 4);  
	
	//PA6  DCMI_PIXCLK
	//PA4  DCMI_HREF
	// configure ports for alternative functions
	GPIOA->MODER = (GPIOA->MODER & ~((3UL << 6 * 2) | (3UL < 4 * 2))) |
								 (2UL << 4 * 2) |
								 (2UL << 6 * 2);
	
	// set very hight speed
	GPIOA->OSPEEDR |= (3UL << 4 * 2) |
										(3UL << 6 * 2);
	
	// alternative functions - DCMI
	GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((15UL << 6 * 4) | (15UL < 4 * 4))) |
									(13UL << 4 * 4) |
									(13UL << 6 * 4);  
	
	//PB8 I2C1_SCL
	//PB9 I2C1_SDA
	// configure ports for alternative functions
	GPIOB->MODER = (GPIOB->MODER & ~((3UL << 8 * 2) | (3UL < 9 * 2))) |
								 (2UL << 8 * 2) |
								 (2UL << 9 * 2);
	
	// ports as open drain
	GPIOB->OTYPER |= (1UL << 8) | (1UL << 9);
	
	// set very hight speed
	GPIOB->OSPEEDR |= (3UL << 8 * 2) |
										(3UL << 9 * 2);
	
	// alternative functions - I2C
	GPIOB->AFR[1] = (GPIOB->AFR[1] & ~((15UL << 0 * 4) | (15UL < 1 * 4))) |
									(4UL << 0 * 4) |
									(4UL << 1 * 4);  
									
									
	//PC9 as MCO2 with 24MHz
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	GPIOC->MODER = (GPIOC->MODER & ~(3UL << 9 * 2)) |
								 (2UL << 9 * 2);
								 
	GPIOC->OSPEEDR |= (3UL << 9 * 2);
	
	GPIOC->OTYPER &= ~(1UL << 9);	
	
	// alternative functions - MCO2
	GPIOC->AFR[1] = (GPIOC->AFR[1] & ~(15UL << 1 * 4)) |
									(0UL << 1 * 4); 

  RCC->CFGR = (RCC->CFGR & ~((7UL << RCC_CFGR_MCO2PRE_Pos) | (3UL << RCC_CFGR_MCO2_Pos))) |
							(1UL << RCC_CFGR_MCO2_Pos) |    // PLL2S 24MHz 
							(0UL << RCC_CFGR_MCO2PRE_Pos);  // no div
	
	return;
}

//---------------------------------------------------------------------

void DCMI_CAM_Init()
{
	RCC->AHB2ENR |= RCC_AHB2ENR_DCMIEN;
	
	DCMI->IER = DCMI_IER_VSYNC_IE;
	
	DCMI->CR = DCMI_CR_PCKPOL |
						 DCMI_CR_VSPOL  |
						 DCMI_CR_CAPTURE;
	
	NVIC_EnableIRQ(DCMI_IRQn);
	NVIC_SetPriority(DCMI_IRQn, 0);
	
	DCMI->CR |= DCMI_CR_ENABLE;
	
	flag_new_frame = 0;
	flag_stream_status = STREAM_ON;
	
	return;
}

//---------------------------------------------------------------------

void DCMI_CAM_Disable()
{
	DCMI->CR = 0;
	RCC->AHB2ENR &= ~RCC_AHB2ENR_DCMIEN;
	NVIC_DisableIRQ(DCMI_IRQn);
	flag_stream_status = STREAM_OFF;
	
	return;
}

//---------------------------------------------------------------------

void DCMI_IRQHandler()
{
	DCMI->ICR = DCMI_ICR_VSYNC_ISC;
	flag_new_frame = 1;
	
	return;
}

//---------------------------------------------------------------------

void DCMI_DMA_Init(uint16_t transfer_size, uint32_t addr_buf0, uint32_t addr_buf1)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
	
	// 1280 * 2 = 2560 bytes per line (max) , headers common size = 44
	DMA2_Stream1->NDTR = transfer_size; //320;
	DMA2_Stream1->M0AR = addr_buf0;
	DMA2_Stream1->M1AR = addr_buf1;
	DMA2_Stream1->PAR = (uint32_t)&DCMI->DR;
	DMA2_Stream1->FCR = (3UL << DMA_SxFCR_FTH_Pos) | 
											 DMA_SxFCR_DMDIS;
	
	NVIC_EnableIRQ(DMA2_Stream1_IRQn);
	NVIC_SetPriority(DMA2_Stream1_IRQn, 0);
	
	//Stream 1, channel 1
	DMA2_Stream1->CR = (1UL << DMA_SxCR_CHSEL_Pos)  | // channel 1 selected
										 (1UL << DMA_SxCR_MBURST_Pos) | // INCR4 (fifo size =4 word and our data size = word) so max burst is 4
										 (2UL << DMA_SxCR_MSIZE_Pos)  | // 10: word (32-bit)
										 (2UL << DMA_SxCR_PSIZE_Pos)  | // 10: word (32-bit)
											DMA_SxCR_MINC |							  // memory address incremented 
											DMA_SxCR_DBM 	| 						  // double  bufer enabled
										 (3UL << DMA_SxCR_PL_Pos)		  | // very hight priorety
											DMA_SxCR_TCIE |								// transfer complete interrupt
											DMA_SxCR_TEIE;								// transfer error interrupt
											
	DMA2_Stream1->CR |= DMA_SxCR_EN;										
	
	
	return;
}

//---------------------------------------------------------------------

void DMA2_Stream1_IRQHandler(void)
{
	if((DMA2->LISR & DMA_LISR_TCIF1) != 0)
	{
		uint32_t dma_buf_new;
		uint32_t dma_buf_cur;
		
		uint32_t pck_data_offset = sizeof(Eth_Header_T) + sizeof(IPv4_Header_T) + sizeof(UDP_Header_T); 
		pck_data_offset += 2; // need add 2 for alignment dma start addr
		dma_buf_new = PopFifo(&FIFO_TX_POOL);
		if(dma_buf_new != 0)
		{
			if((DMA2_Stream1->CR & DMA_SxCR_CT) == 0){ // current dma buf = 0, can acccess dma buf1
				dma_buf_cur = DMA2_Stream1->M1AR;
				DMA2_Stream1->M1AR = dma_buf_new + pck_data_offset;
			}
			else{																			 // current dma buf = 1, can acccess dma buf0
				dma_buf_cur = DMA2_Stream1->M0AR;
				DMA2_Stream1->M0AR = dma_buf_new + pck_data_offset;
			}
			
			
			//add begin frame marker
			if(flag_new_frame){
				*(uint16_t *)(dma_buf_cur - 2) = FRAME_BEGIN;
				swap_value = *(uint16_t *)(dma_buf_cur - 2 + swap_offset);
				flag_new_frame = 0;
			}
			else{
				*(uint16_t *)(dma_buf_cur - 2) = swap_value;
				swap_value = *(uint16_t *)(dma_buf_cur - 2 + swap_offset);
			}
			
			PushFifo(&FIFO_DATA_RDY, dma_buf_cur - pck_data_offset);
		}
		
		DMA2->LIFCR = DMA_LIFCR_CTCIF1;
	}
	else
	{
		flag_stream_status = STREAM_ERR;
	}
	
	if((DMA2->LISR & DMA_LISR_TEIF1) != 0)
	{
		DMA2->LIFCR = DMA_LIFCR_CTEIF1;
		flag_stream_status = STREAM_ERR;
	}
	return;
}

//---------------------------------------------------------------------

int DCMI_DMA_Start(uint16_t dma_transfer_size_in_bytes)
{
	uint32_t dma_buf0; 
	uint32_t dma_buf1;
	uint32_t pck_data_offset = sizeof(Eth_Header_T) + sizeof(IPv4_Header_T) + sizeof(UDP_Header_T); 
	pck_data_offset += 2; // need add 2 for alignment dma start addr
	
	__disable_irq();
	dma_buf0 = PopFifo(&FIFO_TX_POOL);
	if(dma_buf0 == 0){
		__enable_irq();
		return 1;
	}
	
	dma_buf1 = PopFifo(&FIFO_TX_POOL);
	if(dma_buf1 == 0){
		PushFifo(&FIFO_TX_POOL, dma_buf0);
		__enable_irq();
		return 2;
	}
	__enable_irq();
			
	swap_offset = dma_transfer_size_in_bytes;
	DCMI_DMA_Init(dma_transfer_size_in_bytes / 4, dma_buf0 + pck_data_offset, dma_buf1 + pck_data_offset);
	DCMI_CAM_Init();
	
	return 0;
}

//---------------------------------------------------------------------

void DCMI_DMA_Stop()
{
	DCMI_CAM_Disable();	
	uint32_t pck_data_offset = sizeof(Eth_Header_T) + sizeof(IPv4_Header_T) + sizeof(UDP_Header_T); 
	pck_data_offset += 2; // need add 2 for alignment dma start addr
	uint32_t dcmi_dma_buf0 = DMA2_Stream1->M0AR - pck_data_offset;
	uint32_t dcmi_dma_buf1 = DMA2_Stream1->M1AR - pck_data_offset;
	DCMI_DMA_Disable();
			
	__disable_irq();
	PushFifo(&FIFO_TX_POOL, dcmi_dma_buf0);
	PushFifo(&FIFO_TX_POOL, dcmi_dma_buf1);
	__enable_irq();
	
	return;
}

//---------------------------------------------------------------------

void DCMI_DMA_Disable()
{
	DMA2_Stream1->CR = 0;
	RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA2EN;
	NVIC_DisableIRQ(DMA2_Stream1_IRQn);
	return;
}
	
//---------------------------------------------------------------------

void I2C1_Init()
{
	
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
	I2C1->CCR = 240;
	I2C1->TRISE = 0x12;
	I2C1->CR2 = 42;
	I2C1->CR1 = I2C_CR1_PE;
	
	return;
}

//---------------------------------------------------------------------

int I2C1_Write(uint8_t addr_8bit, uint8_t data_8bit)
{
	int timeout;
	
	// Detect the bus is busy or not
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR2 & I2C_SR2_BUSY) != 0){
		if((timeout--) == 0) return 1;
	}
	
	// SCCB Generate START
	I2C1->CR1 |= I2C_CR1_START;
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR2 & I2C_SR2_MSL) == 0){
		if((timeout--) == 0) return 2;
	}
	
	// SCCB Send 7-bits addr
	I2C1->DR = ADDR_WRITE;
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_ADDR) == 0){
		if((timeout--) == 0) return 3;
	}
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR2 & I2C_SR2_TRA) == 0){
		if((timeout--) == 0) return 4;
	}
	
	// SCCB Send Reg Data.
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_TXE) == 0){
		if((timeout--) == 0) return 5;
	}
	I2C1->DR = addr_8bit;
	
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_TXE) == 0){
		if((timeout--) == 0) return 6;
	}
	I2C1->DR = data_8bit;
	
	// SCCB Generate STOP
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_TXE) == 0){
		if((timeout--) == 0) return 7;
	}
	I2C1->CR1 |= I2C_CR1_STOP;
	
	return 0;
}

//---------------------------------------------------------------------

int I2C1_Read(uint8_t addr_8bit, uint8_t *data_8bit)
{
	int timeout;
	// Detect the bus is busy or not
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR2 & I2C_SR2_BUSY) != 0){
		if((timeout--) == 0) return 1;
	}
	
	// SCCB Generate START
	I2C1->CR1 |= I2C_CR1_START;
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR2 & I2C_SR2_MSL) == 0){
		if((timeout--) == 0) return 2;
	}
	
	// SCCB Send 7-bits addr device + rw bit
	I2C1->DR = ADDR_WRITE;
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_ADDR) == 0){
		if((timeout--) == 0) return 3;
	}
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR2 & I2C_SR2_TRA) == 0){
		if((timeout--) == 0) return 4;
	}
	
	// SCCB Send 8-reg addr
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_TXE) == 0){
		if((timeout--) == 0) return 5;
	}
	I2C1->DR = addr_8bit;
	
	// SCCB Generate STOP
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_TXE) == 0){
		if((timeout--) == 0) return 6;
	}
	I2C1->CR1 |= I2C_CR1_STOP;
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR2 & I2C_SR2_BUSY) != 0){
		if((timeout--) == 0) return 7;
	}
	
	// SCCB Generate START
	I2C1->CR1 |= I2C_CR1_START;
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR2 & I2C_SR2_MSL) == 0){
		if((timeout--) == 0) return 8;
	}
	
	// SCCB Send 7-bits addr
	I2C1->DR = ADDR_READ;
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_ADDR) == 0){
		if((timeout--) == 0) return 9;
	}
	I2C1->SR2;
	timeout = I2C_TIMEOUT_MAX;
	while((I2C1->SR1 & I2C_SR1_RXNE) == 0){
		if((timeout--) == 0) return 10;
	}
	I2C1->CR1 |= I2C_CR1_STOP;
	
	*data_8bit = I2C1->DR;
	
	return 0;
}
