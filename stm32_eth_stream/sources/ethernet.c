#include "ethernet.h"
#include "misc_func.h"
#include "flash_control.h"
#include "ov9655.h"

RX_DESCRIPTOR_T RX_DESCRIPTOR_LIST[RX_DESC_CNT];
TX_DESCRIPTOR_T TX_DESCRIPTOR_LIST[TX_DESC_CNT];

uint8_t RX_DESCRIPTOR_BUF[RX_BUF_CNT][DESCR_BUF_SIZE];
uint8_t TX_DESCRIPTOR_BUF[TX_BUF_CNT][DESCR_BUF_SIZE];

FIFO_T FIFO_RX_POOL;
FIFO_T FIFO_RX_RDY;

FIFO_T FIFO_TX_POOL;
FIFO_T FIFO_TX_RDY;
FIFO_T FIFO_DATA_RDY;

static uint32_t tx_descr_idx;
CLIENT_T client_data;
DEVICE_T device_data;

static uint8_t hard_pass[8] = {'r', 'o', 'o', 't','r', 'o', 'o', 't'};

uint32_t flag_eth_restart;
extern uint32_t flag_stream_status;

void GPIO_InitEhtPins()
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
									RCC_AHB1ENR_GPIOBEN |
									RCC_AHB1ENR_GPIOCEN;
	
	// PA1 ETH_RMII_REF_CLK
	// PA2 ETH_MDIO
	// PA7 ETH_RMII_CRS_DV
	// configure ports for alternative functions
	GPIOA->MODER = (GPIOA->MODER & ~((3UL << 1 * 2) | (3UL < 2 * 2) | (3UL << 7 * 2))) |
								 (2UL << 1 * 2) |
								 (2UL << 2 * 2) |
								 (2UL << 7 * 2);
	
	// set very hight speed
	GPIOA->OSPEEDR |= (3UL << 1 * 2) |
										(3UL << 2 * 2) |
										(3UL << 7 * 2);
	
	// alternative functions - ethernet
	GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((15UL << 1 * 4) | (15UL < 2 * 4) | (15UL << 7 * 4))) |
									(11UL << 1 * 4) |
									(11UL << 2 * 4) |
									(11UL << 7 * 4);  
	// PB11 ETH_RMII_TX_EN
  // PB12 ETH_RMII_TXD0
	// PB13 ETH_RMII_TXD1
	// configure ports for alternative functions
	GPIOB->MODER = (GPIOB->MODER & ~((3UL << 11 * 2) | (3UL < 12 * 2) | (3UL << 13 * 2))) |
								 (2UL << 11 * 2) |
								 (2UL << 12 * 2) |
								 (2UL << 13 * 2);
	
	// set very hight speed
	GPIOB->OSPEEDR |= (3UL << 11 * 2) |
										(3UL << 12 * 2) |
										(3UL << 13 * 2);
										
	// alternative functions - ethernet
	GPIOB->AFR[1] = (GPIOB->AFR[1] & ~((15UL << 3 * 4) | (15UL < 4 * 4) | (15UL << 5 * 4))) |
									(11UL << 3 * 4) |
									(11UL << 4 * 4) |
									(11UL << 5 * 4);  
	
	// PC1 ETH_MDC
	// PC4 ETH_RMII_RXD0
	// PC5 ETH_RMII_RXD1
	// configure ports for alternative functions
	GPIOC->MODER = (GPIOC->MODER & ~((3UL << 1 * 2) | (3UL < 4 * 2) | (3UL << 5 * 2))) |
								 (2UL << 1 * 2) |
								 (2UL << 4 * 2) |
								 (2UL << 5 * 2);
								 
	// set very hight speed
	GPIOC->OSPEEDR |= (3UL << 1 * 2) |
										(3UL << 4 * 2) |
										(3UL << 5 * 2);
										
	// alternative functions - ethernet
	GPIOC->AFR[0] = (GPIOC->AFR[0] & ~((15UL << 1 * 4) | (15UL < 4 * 4) | (15UL << 5 * 4))) |
									(11UL << 1 * 4) |
									(11UL << 4 * 4) |
									(11UL << 5 * 4);  
	
	return;
}

//---------------------------------------------------------------------

void Eth_GetDeviceInfo()
{
	if(FLASH_LoadSettings((uint8_t *)&device_data, sizeof(device_data)) != 0)
	{
		// default settings
		device_data.device_ip[0] = MY_IP0;
		device_data.device_ip[1] = MY_IP1;
		device_data.device_mac[0] = MY_MAC0;
		device_data.device_mac[1] = MY_MAC1;
		device_data.device_mac[2] = MY_MAC2;
		device_data.device_id = MY_ID;
		mem_cpy8(device_data.device_pass, &hard_pass[0], sizeof(device_data.device_pass));
	}
	
	return;
}

//---------------------------------------------------------------------

void ETH_Init()
{
	int i;
	uint8_t *buf_addr;
	
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	// select rmii interface
	SYSCFG->PMC |= SYSCFG_PMC_MII_RMII;
	
	// enable all parts of ethernet
	RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN |
									RCC_AHB1ENR_ETHMACTXEN |
									RCC_AHB1ENR_ETHMACRXEN;
	
	InitFifo(&FIFO_RX_POOL, RX_BUF_CNT);
	InitFifo(&FIFO_RX_RDY, RX_BUF_CNT);
	
	//fill rx pool of avaliable address
	for(i = 0; i < RX_BUF_CNT; i++)
		PushFifo(&FIFO_RX_POOL, (uint32_t)&RX_DESCRIPTOR_BUF[i][0]);
		
	//init rx descriptors
	for(i = 0; i < RX_DESC_CNT; i++)
	{
		RX_DESCRIPTOR_LIST[i].RDES1 = DESCR_BUF_SIZE;
		buf_addr = (uint8_t *)PopFifo(&FIFO_RX_POOL);
		RX_DESCRIPTOR_LIST[i].RDES2 = buf_addr;
		RX_DESCRIPTOR_LIST[i].RDES3 = 0;
		RX_DESCRIPTOR_LIST[i].RDES0 = RDES0_OWN;
	}
	RX_DESCRIPTOR_LIST[RX_DESC_CNT - 1].RDES1 |= RDES1_RER;
	
	InitFifo(&FIFO_TX_POOL, TX_BUF_CNT);
	InitFifo(&FIFO_TX_RDY, TX_BUF_CNT * 2); // x2 because contain buffer addr and buffer size
	InitFifo(&FIFO_DATA_RDY, TX_BUF_CNT);
	
	//fill tx pool of avaliable address
	for(i = 0; i < TX_BUF_CNT; i++)
		PushFifo(&FIFO_TX_POOL, (uint32_t)&TX_DESCRIPTOR_BUF[i][0]);	
	
	
	//init tx descriptors
	for(i = 0; i < TX_DESC_CNT; i++)
	{
		TX_DESCRIPTOR_LIST[i].TDES1 = 0;
		buf_addr = (uint8_t *)PopFifo(&FIFO_TX_POOL);
		TX_DESCRIPTOR_LIST[i].TDES2 = buf_addr;
		TX_DESCRIPTOR_LIST[i].TDES3 = 0;
		TX_DESCRIPTOR_LIST[i].TDES0 = 0;
	}
	TX_DESCRIPTOR_LIST[TX_DESC_CNT - 1].TDES0 = TDES0_TER;
	
	tx_descr_idx = 0;
	client_data.client_ip[0] = 0;
	client_data.client_ip[1] = 0;
	client_data.client_mac[0] = 0;
	client_data.client_mac[1] = 0;
	client_data.client_mac[2] = 0;
	
	ETH->DMABMR |= ETH_DMABMR_SR;							// Software reset MAC, DMA
	while(ETH->DMABMR & ETH_DMABMR_SR);
	
	ETH->DMABMR = ETH_DMABMR_PBL_16Beat | 
								ETH_DMABMR_FB;
	
	ETH->DMARDLAR = (uint32_t)RX_DESCRIPTOR_LIST;
	ETH->DMATDLAR = (uint32_t)TX_DESCRIPTOR_LIST;
	
	ETH->MACA0HR = device_data.device_mac[2];
	ETH->MACA0LR = ((device_data.device_mac[1] << 16) | device_data.device_mac[0]);
	
	NVIC_EnableIRQ(ETH_IRQn);
	NVIC_SetPriority(ETH_IRQn, 0);
						
	ETH->DMAIER = ETH_DMAIER_NISE |						// Normal interrupt summary enable
								ETH_DMAIER_TBUIE |
								ETH_DMAIER_ERIE | 	
								ETH_DMAIER_RIE |
								ETH_DMAIER_AISE |						// Abnormal interrupt summary enable
								ETH_DMAIER_TPSIE |
								ETH_DMAIER_TJTIE |
								ETH_DMAIER_ROIE |
								ETH_DMAIER_TUIE |
								ETH_DMAIER_RBUIE |
								ETH_DMAIER_RPSIE |
								ETH_DMAIER_RWTIE |
								ETH_DMAIER_ETIE |
								ETH_DMAIER_FBEIE;								
								
	
	ETH->MACCR = ETH_MACCR_FES |						  // Fast Ethernet speed 100 Mbit/s
							 ETH_MACCR_DM |								// full duplex mode
							 ETH_MACCR_TE |								// Transmitter enable
							 ETH_MACCR_RE;								// Receiver enable
	
	ETH->DMAOMR = ETH_DMAOMR_RSF |						// Receive store and forward
								ETH_DMAOMR_TTC_64Bytes |		// Transmit threshold control
								ETH_DMAOMR_ST  |						// Start/stop transmission
								ETH_DMAOMR_SR;							// Start/stop receive
								
	flag_eth_restart = 0;
	
	return;
}

//---------------------------------------------------------------------

void ETH_Disable()
{
	// enable all parts of ethernet
	RCC->AHB1ENR &= ~(RCC_AHB1ENR_ETHMACEN |
									  RCC_AHB1ENR_ETHMACTXEN |
									  RCC_AHB1ENR_ETHMACRXEN);
	
	NVIC_DisableIRQ(ETH_IRQn);
	
	return;
}

//---------------------------------------------------------------------

uint16_t MakeArp(uint8_t *ptrRxBuf, uint8_t *ptrTxBuf)
{
	Eth_Header_T *Eth_Header_Tx, *Eth_Header_Rx;
	Arp_Header_T *Arp_Header_Tx, *Arp_Header_Rx;
	
	Arp_Header_Rx = (Arp_Header_T *)(ptrRxBuf + (uint32_t)sizeof(Eth_Header_T));
	Arp_Header_Tx = (Arp_Header_T *)(ptrTxBuf + sizeof(Eth_Header_T));
						
	if( (Arp_Header_Rx->op_reply == 0x0100) &&
			(Arp_Header_Rx->target_ip[0] == device_data.device_ip[0] && 
			 Arp_Header_Rx->target_ip[1] == device_data.device_ip[1])
		){
				//make eth header
				Eth_Header_Tx = (Eth_Header_T *)ptrTxBuf;
				Eth_Header_Rx = (Eth_Header_T *)ptrRxBuf;
			
				Eth_Header_Tx->target_mac[0] = Eth_Header_Rx->sender_mac[0];
				Eth_Header_Tx->target_mac[1] = Eth_Header_Rx->sender_mac[1];
				Eth_Header_Tx->target_mac[2] = Eth_Header_Rx->sender_mac[2];
							
				Eth_Header_Tx->sender_mac[0] = device_data.device_mac[0];
				Eth_Header_Tx->sender_mac[1] = device_data.device_mac[1];
				Eth_Header_Tx->sender_mac[2] = device_data.device_mac[2];
							
				Eth_Header_Tx->type = 0x0608;

				//make arp header
				Arp_Header_Tx->hw_type = 0x0100;
								
				Arp_Header_Tx->pt_type = 0x0008;
								
				Arp_Header_Tx->hw_size = 0x06;
				Arp_Header_Tx->pt_size = 0x04;
				Arp_Header_Tx->op_reply = 0x0200;
								
				Arp_Header_Tx->sender_mac[0] = device_data.device_mac[0];
				Arp_Header_Tx->sender_mac[1] = device_data.device_mac[1];
				Arp_Header_Tx->sender_mac[2] = device_data.device_mac[2];
								
				Arp_Header_Tx->sender_ip[0] = device_data.device_ip[0];
				Arp_Header_Tx->sender_ip[1] = device_data.device_ip[1];
			
				Arp_Header_Tx->target_mac[0] = Arp_Header_Rx->sender_mac[0];
				Arp_Header_Tx->target_mac[1] = Arp_Header_Rx->sender_mac[1];
				Arp_Header_Tx->target_mac[2] = Arp_Header_Rx->sender_mac[2];
								
				Arp_Header_Tx->target_ip[0] = Arp_Header_Tx->sender_ip[0];
				Arp_Header_Tx->target_ip[1] = Arp_Header_Tx->sender_ip[1];
				
				return 60;
			}
		
	return 0;
}

//---------------------------------------------------------------------

uint16_t MakeICMP(uint8_t *ptrRxBuf, uint8_t *ptrTxBuf)
{
	uint32_t crc_sum;
	uint16_t total_length = 0;
	uint32_t icmp_data_size;
	uint16_t *icmp_data_rx;
	uint16_t *icmp_data_tx;
	Eth_Header_T *Eth_Header_Tx, *Eth_Header_Rx;
	IPv4_Header_T *IPv4_Header_Tx, *IPv4_Header_Rx;
	ICMP_Header_T *ICMP_Header_Tx, *ICMP_Header_Rx;
	
	Eth_Header_Rx = (Eth_Header_T *)ptrRxBuf;
	IPv4_Header_Rx = (IPv4_Header_T *)(ptrRxBuf + sizeof(Eth_Header_T));
	ICMP_Header_Rx = (ICMP_Header_T *)(ptrRxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T));
	icmp_data_rx = (uint16_t*)(ptrRxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T) + sizeof(ICMP_Header_T));
	icmp_data_tx = (uint16_t*)(ptrTxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T) + sizeof(ICMP_Header_T));
	
	if(ICMP_Header_Rx->type == 0x08 )
	{			
		//make eth header
		Eth_Header_Tx = (Eth_Header_T *)ptrTxBuf;
		Eth_Header_Tx->target_mac[0] = Eth_Header_Rx->sender_mac[0];
		Eth_Header_Tx->target_mac[1] = Eth_Header_Rx->sender_mac[1];
		Eth_Header_Tx->target_mac[2] = Eth_Header_Rx->sender_mac[2];
			
		Eth_Header_Tx->sender_mac[0] = device_data.device_mac[0];
		Eth_Header_Tx->sender_mac[1] = device_data.device_mac[1];
		Eth_Header_Tx->sender_mac[2] = device_data.device_mac[2];
									
		Eth_Header_Tx->type = 0x0008;
		
		//make IPv4
		IPv4_Header_Tx = (IPv4_Header_T *)(ptrTxBuf + sizeof(Eth_Header_T));
		IPv4_Header_Tx->vlen = 0x45;
		IPv4_Header_Tx->srvc_field = 0;
		IPv4_Header_Tx->total_len = IPv4_Header_Rx->total_len;
		IPv4_Header_Tx->id = 0;
		IPv4_Header_Tx->f_offfset = 0;
		IPv4_Header_Tx->ttl = 0x80;
		IPv4_Header_Tx->protocol = 01;
										
		crc_sum = device_data.device_ip[1];
		crc_sum += device_data.device_ip[0];
		crc_sum += IPv4_Header_Rx->ip_source[1];				
		crc_sum += IPv4_Header_Rx->ip_source[0]; 
		crc_sum += IPv4_Header_Tx->total_len + 0x0045 + 0x0180;
															
		crc_sum = (uint32_t)(crc_sum>>16) + (uint32_t)(crc_sum & 0xFFFF);
		crc_sum = (uint32_t)(crc_sum>>16) + (uint32_t)(crc_sum & 0xFFFF);
										
		crc_sum = ~crc_sum;
										
		IPv4_Header_Tx->header_crc = crc_sum;
										
		IPv4_Header_Tx->ip_source[0] = device_data.device_ip[0];
		IPv4_Header_Tx->ip_source[1] = device_data.device_ip[1];
										
		IPv4_Header_Tx->ip_target[0] = IPv4_Header_Rx->ip_source[0];
		IPv4_Header_Tx->ip_target[1] = IPv4_Header_Rx->ip_source[1];
										
		//make ICMP 
		ICMP_Header_Tx = (ICMP_Header_T *)(ptrTxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T));
		ICMP_Header_Tx->type = 0;
		ICMP_Header_Tx->code = 0;
										
		ICMP_Header_Tx->id = ICMP_Header_Rx->id;
										
		ICMP_Header_Tx->seq_numb = ICMP_Header_Rx->seq_numb;
										
		crc_sum = ICMP_Header_Tx->seq_numb + ICMP_Header_Tx->id;	
		
		icmp_data_size = (IPv4_Header_Rx->total_len >> 8) | (IPv4_Header_Rx->total_len & 0xFF);					
		icmp_data_size -= (sizeof(IPv4_Header_T) + sizeof(ICMP_Header_T));
		icmp_data_size >>= 1;
		
		for(int i = 0; i < icmp_data_size; i++)
		{
			icmp_data_tx[i] = icmp_data_rx[i];
			crc_sum += icmp_data_tx[i];
		}
										
		crc_sum = (uint32_t)(crc_sum>>16) + (uint32_t)(crc_sum & 0xFFFF);
		crc_sum = ~crc_sum;
										
		ICMP_Header_Tx->crc = crc_sum;			
		total_length = sizeof(Eth_Header_T) + sizeof(IPv4_Header_T) + sizeof(ICMP_Header_T) + icmp_data_size * 2;
	}
	return total_length;
}

//---------------------------------------------------------------------

uint16_t MakeUDPCmd(uint8_t *ptrRxBuf, uint8_t *ptrTxBuf, uint16_t dlen)
{
	
	uint32_t crc_sum;

	IPv4_Header_T *IPv4_Header_Rx = (IPv4_Header_T *)(ptrRxBuf + sizeof(Eth_Header_T));
	UDP_Header_T *UDP_Header_Rx = (UDP_Header_T *)(ptrRxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T));
							
	//make eth header
	Eth_Header_T *Eth_Header_Tx = (Eth_Header_T *)ptrTxBuf;
	Eth_Header_T *Eth_Header_Rx = (Eth_Header_T *)ptrRxBuf;
	Eth_Header_Tx->target_mac[0] = Eth_Header_Rx->sender_mac[0];
	Eth_Header_Tx->target_mac[1] = Eth_Header_Rx->sender_mac[1];
	Eth_Header_Tx->target_mac[2] = Eth_Header_Rx->sender_mac[2];
								
	Eth_Header_Tx->sender_mac[0] = device_data.device_mac[0];
	Eth_Header_Tx->sender_mac[1] = device_data.device_mac[1];
	Eth_Header_Tx->sender_mac[2] = device_data.device_mac[2];
														
	Eth_Header_Tx->type = 0x0008;
	
	uint16_t total_len = sizeof(IPv4_Header_T) + sizeof(UDP_Header_T) + dlen;
	
	//make IPv4
	IPv4_Header_T *IPv4_Header_Tx = (IPv4_Header_T *)(ptrTxBuf + sizeof(Eth_Header_T));
	IPv4_Header_Tx->vlen = 0x45;
	IPv4_Header_Tx->srvc_field = 0;
	IPv4_Header_Tx->total_len = ((total_len << 8) & 0xFF00) | (total_len >> 8);
	IPv4_Header_Tx->id = 0;
	IPv4_Header_Tx->f_offfset = 0;
	IPv4_Header_Tx->ttl = 0x80;
	IPv4_Header_Tx->protocol = 0x11;
							
	crc_sum = device_data.device_ip[1];
	crc_sum += device_data.device_ip[0];
	crc_sum += IPv4_Header_Rx->ip_source[1];				
	crc_sum += IPv4_Header_Rx->ip_source[0];
	crc_sum += IPv4_Header_Tx->total_len + 0x0045 + 0x1180;
												
	crc_sum = (uint32_t)(crc_sum>>16) + (uint32_t)(crc_sum & 0xFFFF);
	crc_sum = (uint32_t)(crc_sum>>16) + (uint32_t)(crc_sum & 0xFFFF);
							
	crc_sum = ~crc_sum;
							
	IPv4_Header_Tx->header_crc = crc_sum;
							
	IPv4_Header_Tx->ip_source[0] = device_data.device_ip[0];
	IPv4_Header_Tx->ip_source[1] = device_data.device_ip[1];
							
	IPv4_Header_Tx->ip_target[0] = IPv4_Header_Rx->ip_source[0];	
	IPv4_Header_Tx->ip_target[1] = IPv4_Header_Rx->ip_source[1];	
		
	//make UDP
	UDP_Header_T *UDP_Header_Tx = (UDP_Header_T *)(ptrTxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T));
		
	UDP_Header_Tx->source_port = PORT_CMD;
	UDP_Header_Tx->target_port = UDP_Header_Rx->source_port;
	
	uint16_t udp_len = sizeof(UDP_Header_T) + dlen;
	udp_len = ((udp_len << 8) & 0xFF00) | (udp_len >> 8);	
	
	UDP_Header_Tx->length = udp_len;
	UDP_Header_Tx->crc = 00;
		
	total_len += sizeof(Eth_Header_T);
		
	return total_len;
}

//---------------------------------------------------------------------
uint16_t MakeUdpData(uint8_t *ptrTxBuf, uint16_t size)
{

	UDP_Header_T *UDP_Header_Tx = (UDP_Header_T *)(ptrTxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T));
	
	uint32_t crc_sum;
	Eth_Header_T *Eth_Header_Tx;
	IPv4_Header_T *IPv4_Header_Tx;
							
	//make eth header
	Eth_Header_Tx = (Eth_Header_T *)ptrTxBuf;
	Eth_Header_Tx->target_mac[0] = client_data.client_mac[0];
	Eth_Header_Tx->target_mac[1] = client_data.client_mac[1];
	Eth_Header_Tx->target_mac[2] = client_data.client_mac[2];
								
	Eth_Header_Tx->sender_mac[0] = device_data.device_mac[0];
	Eth_Header_Tx->sender_mac[1] = device_data.device_mac[2];
	Eth_Header_Tx->sender_mac[2] = device_data.device_mac[2];
														
	Eth_Header_Tx->type = 0x0008;
	
	uint16_t total_len = sizeof(IPv4_Header_T) + sizeof(UDP_Header_T) + size;
	
	//make IPv4
	IPv4_Header_Tx = (IPv4_Header_T *)(ptrTxBuf + sizeof(Eth_Header_T));
	IPv4_Header_Tx->vlen = 0x45;
	IPv4_Header_Tx->srvc_field = 0;
	IPv4_Header_Tx->total_len = ((total_len << 8) & 0xFF00) | (total_len >> 8);
	IPv4_Header_Tx->id = 0;
	IPv4_Header_Tx->f_offfset = 0;
	IPv4_Header_Tx->ttl = 0x80;
	IPv4_Header_Tx->protocol = 0x11;
							
	crc_sum = device_data.device_ip[1];
	crc_sum += device_data.device_ip[0];
	crc_sum += client_data.client_ip[1];				
	crc_sum += client_data.client_ip[0];
	crc_sum += IPv4_Header_Tx->total_len + 0x0045 + 0x1180;
												
	crc_sum = (uint32_t)(crc_sum>>16) + (uint32_t)(crc_sum & 0xFFFF);
	crc_sum = (uint32_t)(crc_sum>>16) + (uint32_t)(crc_sum & 0xFFFF);
							
	crc_sum = ~crc_sum;
							
	IPv4_Header_Tx->header_crc = crc_sum;
							
	IPv4_Header_Tx->ip_source[0] = device_data.device_ip[0];
	IPv4_Header_Tx->ip_source[1] = device_data.device_ip[1];
							
	IPv4_Header_Tx->ip_target[0] = client_data.client_ip[0];	
	IPv4_Header_Tx->ip_target[1] = client_data.client_ip[1];	
		
	//make UDP
	UDP_Header_Tx = (UDP_Header_T *)(ptrTxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T));
		
	UDP_Header_Tx->source_port = PORT_DATA;
	UDP_Header_Tx->target_port = client_data.data_port;
	
	uint16_t udp_len = sizeof(UDP_Header_T) + size;
	udp_len = ((udp_len << 8) & 0xFF00) | (udp_len >> 8);	
	
	UDP_Header_Tx->length = udp_len;
	UDP_Header_Tx->crc = 00;
	
	total_len += sizeof(Eth_Header_T);
		
	return total_len;
}

//---------------------------------------------------------------------

uint16_t HandleUdpCmd(uint8_t *ptrRxBuf, uint8_t *ptrTxBuf)
{
	//CMD_ARG_T
	Eth_Header_T *Eth_Header_Rx = (Eth_Header_T *)ptrRxBuf;
	IPv4_Header_T *IPv4_Header_Rx = (IPv4_Header_T *)(ptrRxBuf + sizeof(Eth_Header_T));
	CMD_ARG_T *ETH_RxCmd = (CMD_ARG_T *)(ptrRxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T) + sizeof(UDP_Header_T));
	CMD_ARG_T *ETH_TxCmd = (CMD_ARG_T *)(ptrTxBuf + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T) + sizeof(UDP_Header_T));
	
	ETH_TxCmd->cmd = ETH_RxCmd->cmd;
	
	uint16_t answ_len = sizeof(CMD_ARG_T);
	
	uint8_t is_client;
	
	//check if current ip & mac address connected
	if(client_data.client_ip[0] == IPv4_Header_Rx->ip_source[0]  &&
		 client_data.client_ip[1] == IPv4_Header_Rx->ip_source[1]  &&
	   client_data.client_mac[0] == Eth_Header_Rx->sender_mac[0] &&
		 client_data.client_mac[1] == Eth_Header_Rx->sender_mac[1] &&
		 client_data.client_mac[2] == Eth_Header_Rx->sender_mac[2]
	)
		is_client = 1;
	else
		is_client = 0;
	
	switch(ETH_RxCmd->cmd)
	{
		case CMD_STATUS:{
			
			ETH_TxCmd->data[0] = ANSW_OK;
			STATUS_PAR_T *status = (STATUS_PAR_T *)&ETH_TxCmd->data[1];
			// current device ip
			status->device_ip[0] = device_data.device_ip[0];
			status->device_ip[1] = device_data.device_ip[1];
			// device id
			status->device_id = device_data.device_id;
			// connected client ip
			status->client_ip[0] = client_data.client_ip[0];
			status->client_ip[1] = client_data.client_ip[1];
			
			break;
		}
		
		case CMD_CHANGE_IP:{
			
			if(client_data.client_ip[0] != 0 && client_data.client_ip[1] != 0){
				ETH_TxCmd->data[0] = ANSW_IN_USE;
				break;
			}
			
			CHANGE_IP_ARG_T *chg_ip_arg = (CHANGE_IP_ARG_T *)&ETH_RxCmd->data[0];
			if(chg_ip_arg->device_id != device_data.device_id){
				// ignore request with wrong device id
				return 0;
			}
			else
			{
				if(flag_stream_status != STREAM_OFF)
					DCMI_DMA_Stop();
				
				CHANGE_IP_ARG_T *chg_ip_arg = (CHANGE_IP_ARG_T *)&ETH_RxCmd->data[0];
				device_data.device_ip[0] = chg_ip_arg->new_ip[0];
				device_data.device_ip[1] = chg_ip_arg->new_ip[1];
				
				FLASH_SaveSettings((uint8_t *)&device_data, sizeof(device_data));
				
				flag_eth_restart = 1;
				
				return 0;
				
			}
		}
		
		case CMD_CONNECT:{
			//no active client
			if(client_data.client_ip[0] == 0 && client_data.client_ip[1] == 0)
			{
				CONNECT_ARG_T *connect_arg = (CONNECT_ARG_T *)&ETH_RxCmd->data[0];
				//check password
				if(cmp_data8(&device_data.device_pass[0], connect_arg->pass, sizeof(device_data.device_pass)) == 0)
				{
					if(flag_stream_status != STREAM_OFF)
						DCMI_DMA_Stop();
					
					client_data.client_ip[0] = IPv4_Header_Rx->ip_source[0];
					client_data.client_ip[1] = IPv4_Header_Rx->ip_source[1];
					
					client_data.client_mac[0] = Eth_Header_Rx->sender_mac[0];
					client_data.client_mac[1] = Eth_Header_Rx->sender_mac[1];
					client_data.client_mac[2] = Eth_Header_Rx->sender_mac[2];
					
					client_data.data_port = connect_arg->data_port;
					
					ETH_TxCmd->data[0] = ANSW_OK;
					
				}
				else
					ETH_TxCmd->data[0] = ANSW_WRONG_PASS;
			}
			else {
				ETH_TxCmd->data[0] = ANSW_IN_USE;
				ETH_TxCmd->data[1] = (uint8_t)(client_data.client_ip[0] >> 0) & 0xFF;
				ETH_TxCmd->data[2] = (uint8_t)(client_data.client_ip[0] >> 8) & 0xFF;
				ETH_TxCmd->data[3] = (uint8_t)(client_data.client_ip[1] >> 0) & 0xFF;
				ETH_TxCmd->data[4] = (uint8_t)(client_data.client_ip[1] >> 8) & 0xFF;
			}
			break;
		}
		case CMD_DISCONNECT:{
			
			DISCONNECT_ARG_T *disconnect_arg = (DISCONNECT_ARG_T *)&ETH_RxCmd->data[0];
			//check password
			if(cmp_data8(&device_data.device_pass[0], disconnect_arg->pass, sizeof(device_data.device_pass)) == 0)
			{
				if(flag_stream_status != STREAM_OFF)
					DCMI_DMA_Stop();
				
				client_data.client_ip[0] = 0;
				client_data.client_ip[1] = 0;
					
				client_data.client_mac[0] = 0;
				client_data.client_mac[1] = 0;
				client_data.client_mac[2] = 0;
				
				ETH_TxCmd->data[0] = ANSW_OK;
			}
			else
				ETH_TxCmd->data[0] = ANSW_WRONG_PASS;
			
			break;
		}
		case CMD_CAMERA_WR:{
			if(is_client == 0){
				ETH_TxCmd->data[0] = ANSW_NOT_CONNECTED;
				break;
			}
		
			ETH_TxCmd->data[0] = ANSW_OK;
			ETH_TxCmd->data[1] = I2C1_Write(ETH_RxCmd->data[0], ETH_RxCmd->data[1]);
			break;
		}
		case CMD_CAMERA_RD:{
			if(is_client == 0){
				ETH_TxCmd->data[0] = ANSW_NOT_CONNECTED;
				break;
			}
		
			ETH_TxCmd->data[0] = ANSW_OK;
			ETH_TxCmd->data[1] = I2C1_Read(ETH_RxCmd->data[0], &ETH_TxCmd->data[2]);
			break;
		}
		case CMD_STREAM_ON:{
			if(is_client == 0)
			{
				ETH_TxCmd->data[0] = ANSW_NOT_CONNECTED;
				break;
			}
			
			client_data.dma_transfer_size = ETH_RxCmd->data[0] | ETH_RxCmd->data[1] << 8; 
			if(client_data.dma_transfer_size < DMA_TRANSFER_MIN / 4 || (client_data.dma_transfer_size % 4))
			{
				ETH_TxCmd->data[0] = ANSW_DMA_BUF_WRONG_SIZE;
				break;
			}
			
			if(flag_stream_status == STREAM_ON){
				ETH_TxCmd->data[0] = ANSW_STREAM_ON;
				break;
			}
			
			if(DCMI_DMA_Start(client_data.dma_transfer_size) != 0)
			{
				ETH_TxCmd->data[0] = ANSW_NO_DMA_BUF;
				break;
			}
			
			ETH_TxCmd->data[0] = ANSW_OK;
		
			break;
		}
		case CMD_STREAM_OFF:{
			if(is_client == 0)
			{
				ETH_TxCmd->data[0] = ANSW_NOT_CONNECTED;
				break;
			}
			
			if(flag_stream_status == STREAM_OFF){
				ETH_TxCmd->data[0] = ANSW_STREAM_OFF;
				break;
			}
			
			DCMI_DMA_Stop();
			
			ETH_TxCmd->data[0] = ANSW_OK;
		
			break;
		}
		
		default:{
			ETH_TxCmd->data[0] = ANSW_WRONG_CMD;
			break;
		}
	}
	
	return answ_len;
}

//---------------------------------------------------------------------

void	ETH_IRQHandler()
{
	uint32_t ibt;
	uint8_t *new_rx_addr;
	int i;
	
	ibt = ETH->DMASR;
	
	if(ibt & ETH_DMASR_NIS)
	{
		if(ibt & ETH_DMASR_RS)				// completion of the frame reception
		{
			for(i = 0; i < RX_DESC_CNT; i++)
			{
				if((RX_DESCRIPTOR_LIST[i].RDES0 & RDES0_OWN) == 0)
				{
					if((RX_DESCRIPTOR_LIST[i].RDES0 & RDES0_FS) && (RX_DESCRIPTOR_LIST[i].RDES0 & RDES0_LS) && !(RX_DESCRIPTOR_LIST[i].RDES0 & RDES0_ES))
					{
						new_rx_addr = (uint8_t *)PopFifo(&FIFO_RX_POOL);
						if(new_rx_addr){
							PushFifo(&FIFO_RX_RDY, (uint32_t)RX_DESCRIPTOR_LIST[i].RDES2);
							RX_DESCRIPTOR_LIST[i].RDES2 = new_rx_addr;
						}
					}
					
					RX_DESCRIPTOR_LIST[i].RDES0 = RDES0_OWN;
				}
			}
		}

		ETH->DMASR &= ETH_DMASR_NIS | ETH_DMASR_TBUS | ETH_DMASR_ERS | ETH_DMASR_RS;
	}
	if(ibt & ETH_DMASR_AIS)
	{
		//fatal error, need restart
		if(ibt & ETH_DMASR_FBES)
			flag_eth_restart = 1;	
		
		/*need handler*/
		ETH->DMASR &= ETH_DMASR_AIS | ETH_DMASR_TPSS | ETH_DMASR_TJTS | ETH_DMASR_ROS | ETH_DMASR_TUS | 
									ETH_DMASR_RBUS | ETH_DMASR_RPSS | ETH_DMASR_RWTS | ETH_DMASR_ETS | ETH_DMASR_FBES;
	}
	
	return;
}

//---------------------------------------------------------------------

void EthernetDataHandler()
{
	if(GetFifoCnt(&FIFO_DATA_RDY) == 0)
		return;
	
	uint8_t *buf_tx = (uint8_t *)PopFifo(&FIFO_DATA_RDY);
	uint16_t tx_packet_size = MakeUdpData(buf_tx, client_data.dma_transfer_size);
	
	__disable_irq();
	PushFifo(&FIFO_TX_RDY, (uint32_t)buf_tx);
	PushFifo(&FIFO_TX_RDY, (uint32_t)tx_packet_size);
	__enable_irq();
	
	return;
}

//---------------------------------------------------------------------

int EthernetTxHandler()
{
	uint8_t *buf_tx;
	uint16_t tx_packet_size;
	int poll_idx;
	int poll_end; 
	
	if(GetFifoCnt(&FIFO_TX_RDY) == 0)
		return 0;
	
	// from current idx round up to position idx - 1
	poll_idx = tx_descr_idx;
	if(tx_descr_idx == 0)
		poll_end = TX_DESC_CNT - 1;
	else
		poll_end = tx_descr_idx - 1;
	
	while(1)
	{
		
		if((TX_DESCRIPTOR_LIST[poll_idx].TDES0 & TDES0_OWN) == 0)
		{
			__disable_irq();
			buf_tx = (uint8_t *)PopFifo(&FIFO_TX_RDY);
			if(buf_tx == 0){
				__enable_irq();
				return 0;
			}
			
			tx_packet_size = (uint16_t)PopFifo(&FIFO_TX_RDY);
			if(tx_packet_size == 0){
				PushFifo(&FIFO_TX_POOL, (uint32_t)buf_tx);
				__enable_irq();
				return 1;
			}
			
			PushFifo(&FIFO_TX_POOL, (uint32_t)TX_DESCRIPTOR_LIST[poll_idx].TDES2);
			__enable_irq();
			
			TX_DESCRIPTOR_LIST[poll_idx].TDES1 = tx_packet_size & 0xFFF;
			TX_DESCRIPTOR_LIST[poll_idx].TDES2 = buf_tx;
			TX_DESCRIPTOR_LIST[poll_idx].TDES3 = 0;
			TX_DESCRIPTOR_LIST[poll_idx].TDES0 = TDES0_OWN |
																					 TDES0_LS |
																					 TDES0_FS;
			
			TX_DESCRIPTOR_LIST[TX_DESC_CNT - 1].TDES0 |= TDES0_TER;
			
			if(poll_idx == TX_DESC_CNT - 1)
				tx_descr_idx = 0;
			else
				tx_descr_idx = poll_idx + 1;
			
			ETH->DMATPDR = 0; 						// tx poll demand
			
			return 0;
		}
		
		if(poll_idx == TX_DESC_CNT - 1)
			poll_idx = 0;
		else if(poll_idx == poll_end)
			break;
		else
			poll_idx++;
	}
	
	return 2;
}

//---------------------------------------------------------------------

void EthernetRxHandler()
{
	uint8_t *buf_rx;
	uint8_t *buf_tx;
	uint16_t tx_packet_size = 0;
	uint16_t cmd_answ_size;
	uint8_t ip_target, ip_broadcast;
	Eth_Header_T *Eth_Header_Rx;
	IPv4_Header_T *IPv4_Header_Rx;

	if(GetFifoCnt(&FIFO_RX_RDY) == 0)
		return ;
	
	__disable_irq();
	buf_rx = (uint8_t *)PopFifo(&FIFO_RX_RDY);
	if(buf_rx == 0){
		__enable_irq();
		return;
	}
	
	buf_tx = (uint8_t *)PopFifo(&FIFO_TX_POOL);
	if(buf_tx == 0){
		PushFifo(&FIFO_RX_POOL, (uint32_t)buf_rx);
		__enable_irq();
		return;
	}
	__enable_irq();
	
	Eth_Header_Rx = (Eth_Header_T *)buf_rx;
	IPv4_Header_Rx = (IPv4_Header_T *)(buf_rx + sizeof(Eth_Header_T));
	
	//is target ip
	if(IPv4_Header_Rx->ip_target[0] == device_data.device_ip[0] && IPv4_Header_Rx->ip_target[1] == device_data.device_ip[1])
		ip_target = 1;
	else
		ip_target = 0;
		
	//is broadcast
	if((IPv4_Header_Rx->ip_target[1] >> 8) == 0xFF)
		ip_broadcast = 1;
	else 
		ip_broadcast = 0;
	
	// ARP
	if(Eth_Header_Rx->type == 0x0608) 
		tx_packet_size = MakeArp(buf_rx, buf_tx);
	// IPv4
	else if(Eth_Header_Rx->type == 0x0008 && (ip_target || ip_broadcast))
	{	
		// ICMP
		if(IPv4_Header_Rx->protocol == 0x01 && ip_target){
			tx_packet_size = MakeICMP(buf_rx, buf_tx);
		}
		// UDP
		else if(IPv4_Header_Rx->protocol == 0x11){
				UDP_Header_T *UDP_Header_Rx = (UDP_Header_T *)(buf_rx + sizeof(Eth_Header_T) + sizeof(IPv4_Header_T));
				if(UDP_Header_Rx->target_port == PORT_CMD){		
					cmd_answ_size = HandleUdpCmd(buf_rx, buf_tx);
					if(cmd_answ_size != 0)
						tx_packet_size = MakeUDPCmd(buf_rx, buf_tx, cmd_answ_size);
				} 
		}
	}
	
	__disable_irq();
	if(tx_packet_size){
		PushFifo(&FIFO_TX_RDY, (uint32_t)buf_tx);
		PushFifo(&FIFO_TX_RDY, (uint32_t)tx_packet_size);
	}
	else{
		PushFifo(&FIFO_TX_POOL, (uint32_t)buf_tx);
	}
	
	PushFifo(&FIFO_RX_POOL, (uint32_t)buf_rx);
	__enable_irq();
	
	return;
}
