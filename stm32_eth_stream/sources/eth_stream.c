#include "stm32f4xx.h"
#include "rcc_control.h"
#include "timers.h"
#include "flash_control.h"
#include "ethernet.h"
#include "ov9655.h"
#include "misc_func.h"

extern uint32_t flag_eth_restart;
extern uint32_t flag_stream_status;
extern CLIENT_T client_data;
extern FIFO_T FIFO_TX_POOL; 

int main()
{
	FLASH_EnableCache();
	RCC_InitSysClk();
	TIM7Common_Init();
	
	GPIO_InitCAMPins();
	I2C1_Init();
	SetTimer(T1, 5);
	while(GetTimerVal(T1));

	Eth_GetDeviceInfo();
	GPIO_InitEhtPins();
	ETH_Init();
	
	while(1)
	{
		if(flag_eth_restart)
		{
			DCMI_CAM_Disable();
			DCMI_DMA_Disable();
			ETH_Disable();
			SetTimer(T1, 10);
			while(GetTimerVal(T1));
			ETH_Init();
		}
		
		if(flag_stream_status == STREAM_ERR)
		{
			DCMI_DMA_Stop();
			SetTimer(T1, 10);
			while(GetTimerVal(T1));
			DCMI_DMA_Start(client_data.dma_transfer_size);
		}
		
		EthernetRxHandler();
		EthernetTxHandler();
		EthernetDataHandler();
	}
}
