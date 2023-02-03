#ifndef _ETHERNET_CONTROL_H_
#define _ETHERNET_CONTROL_H_

#include "stm32f4xx.h"

#define DMA_TRANSFER_MIN 	320

#define RX_DESC_CNT				2
#define RX_BUF_CNT				4

#define TX_DESC_CNT				4
#define TX_BUF_CNT				8

#define DESCR_BUF_SIZE		2048

//192.168.1.78
#define MY_IP0 					0xA8C0
#define MY_IP1 					0x4E01

// cmd port:0x1771, data port:0x1772
#define PORT_CMD 				0x7117
#define PORT_DATA 			0x7217

#define MY_MAC0 				0x4100
#define MY_MAC1 				0x2717
#define MY_MAC2 				0x1325

#define MY_ID   				0x11775533

#define MY_MAC_H MY_MAC2
#define MY_MAC_L ((MY_MAC1 << 16) | MY_MAC0)

#define RDES0_OWN 			(1UL << 31)
#define RDES0_AFM 			(1UL << 30)
#define RDES0_ES				(1UL << 15)
#define RDES0_DE				(1UL << 14)
#define RDES0_SAF				(1UL << 13)
#define RDES0_LE				(1UL << 12)
#define RDES0_OE				(1UL << 11)
#define RDES0_VLAN			(1UL << 10)
#define RDES0_FS				(1UL << 9)
#define RDES0_LS				(1UL << 8)
#define RDES0_IPHC			(1UL << 7)
#define RDES0_LCO				(1UL << 6)
#define RDES0_FT				(1UL << 5)
#define RDES0_RWT				(1UL << 4)
#define RDES0_RE				(1UL << 3)
#define RDES0_DE_2			(1UL << 2)
#define RDES0_CE				(1UL << 1)
#define RDES0_PCE				(1UL << 0)

#define RDES1_RER				(1UL << 15)

#define TDES0_OWN 			(1UL << 31)
#define TDES0_IC				(1UL << 30)
#define TDES0_LS				(1UL << 29)
#define TDES0_FS				(1UL << 28)
#define TDES0_TER				(1UL << 21)
#define TDES0_TCH				(1UL << 20)

enum {
	CMD_STATUS = 0x30,
	CMD_CHANGE_IP = 0x31,
	CMD_CONNECT = 0x32,
	CMD_DISCONNECT = 0x33,
	CMD_CAMERA_WR = 0x34,
	CMD_CAMERA_RD = 0x35,
	CMD_STREAM_ON = 0x36,
	CMD_STREAM_OFF = 0x37,
};

enum {
	ANSW_OK = 0x00,
	ANSW_IN_USE = 0x01,
	ANSW_WRONG_PASS = 0x02,
	ANSW_NOT_CONNECTED = 0x03,
	ANSW_NO_DMA_BUF = 0x04,
	ANSW_DMA_BUF_WRONG_SIZE = 0x05,
	ANSW_STREAM_ON = 0x06,
	ANSW_STREAM_OFF = 0x07,
	ANSW_WRONG_CMD = 0x08,
};

typedef struct {
	uint16_t device_ip[2];
	uint16_t device_mac[3];
	uint32_t device_id;
	uint8_t device_pass[8];
}DEVICE_T;

typedef struct {
	uint16_t client_ip[2];
	uint16_t client_mac[3];
	uint16_t data_port;
	uint16_t dma_transfer_size;
}CLIENT_T;

typedef struct {
	uint8_t cmd;
	uint8_t data[32];
}CMD_ARG_T;

typedef struct {
	uint8_t pass[8];
	uint16_t data_port;
}CONNECT_ARG_T;

typedef struct {
	uint16_t device_ip[2];
	uint32_t device_id;
	uint16_t client_ip[2];
}STATUS_PAR_T;

typedef struct {
	uint8_t pass[8];
}DISCONNECT_ARG_T;

typedef struct {
	uint32_t device_id;
	uint16_t new_ip[2];
}CHANGE_IP_ARG_T;


typedef struct {
		uint32_t RDES0;
		uint32_t RDES1;
		uint8_t *RDES2;
		uint8_t *RDES3;
}RX_DESCRIPTOR_T;

typedef struct {
		uint32_t TDES0;
		uint32_t TDES1;
		uint8_t *TDES2;
		uint8_t *TDES3;
}TX_DESCRIPTOR_T;

typedef struct 
{
		uint16_t target_mac[3];
		uint16_t sender_mac[3];
		uint16_t type;
} Eth_Header_T;

typedef struct 
{
		uint16_t hw_type;
		uint16_t pt_type;
		uint8_t hw_size;
		uint8_t pt_size;
		uint16_t op_reply;
		uint16_t sender_mac[3];
		uint16_t sender_ip[2];
		uint16_t target_mac[3];
		uint16_t target_ip[2];
} Arp_Header_T;

typedef struct 
{
		uint8_t vlen;
		uint8_t srvc_field;
		uint16_t total_len;
		uint16_t id;
		uint16_t f_offfset;
		uint8_t ttl;
		uint8_t protocol;
		uint16_t header_crc;
		uint16_t ip_source[2];
		uint16_t ip_target[2];
} IPv4_Header_T;

typedef struct 
{
		uint8_t type;
		uint8_t code;
		uint16_t crc;
		uint16_t id;
		uint16_t seq_numb;
} ICMP_Header_T;

typedef struct 
{
		uint16_t source_port;
		uint16_t target_port;
		uint16_t length;
		uint16_t crc;
} UDP_Header_T;

void Eth_GetDeviceInfo(void);
void GPIO_InitEhtPins(void);
void ETH_Init(void);
void ETH_Disable(void);
void EthernetRxHandler(void);
int EthernetTxHandler(void);
void EthernetDataHandler(void);

#endif
