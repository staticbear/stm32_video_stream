#ifndef _CONTROL_H_
#define _CONTROL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <libintl.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/poll.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <pthread.h>

/*
CMD CONNECT
arg CONNECT_ARG_T

CMD DISCONNECT
arg DISCONNECT_ARG
*/

enum {
	STATUS_CONNECTED = 0x00,
	STATUS_DISCONNECTED = 0x01,
};

enum {
	THREAD_STOP = 0x00,
	THREAD_STARTING = 0x01,
	THREAD_RUN = 0x02,
	THREAD_STOPPING = 0x03,
};

enum {
	STREAM_RUN = 0x00,
	STREAM_STOP = 0x01,
};

enum {
	BUF_VALID = 0x01,
	BUF_FREE = 0x00,
};

enum {
	SM_WAIT_SIGN = 0x00,
	SM_READ_DATA = 0x01,
	SM_CHECK_SIGN = 0x02,
};

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
	uint32_t device_id;
	uint16_t client_ip[2];
}STATUS_PAR_T;

typedef struct {
	uint8_t cmd;
	uint8_t data[32];
}CMD_ARG_T;

typedef struct {
	uint8_t pass[8];
	uint16_t data_port;
}CONNECT_ARG_T;

typedef struct {
	uint16_t dma_size;
}STREAM_ON_ARG_T;

typedef struct {
	uint8_t pass[8];
}DISCONNECT_ARG_T;

typedef struct {
	uint8_t reg_addr;
}CAMERA_RD_ARG_T;

typedef struct {
	uint8_t reg_addr;
	uint8_t reg_data;
}CAMERA_WR_ARG_T;

typedef struct {
	uint32_t device_id;
	uint16_t new_ip[2];
}CHANGE_IP_ARG_T;

typedef struct {
	uint8_t *buf;
	int status;
}FRAME_DATA_T;

int Eth_Init(void);
int Eth_Free(void);
void Eth_Disconnect(void);
int Eth_DeviceList(int device_cmd_port, uint8_t *ptrBuf, int buf_size, int *rcv_size);
int Eth_ChangeIp(int device_cmd_port, uint32_t device_id, uint32_t new_ip);
int Eth_Connect(const char *device_ip, int device_cmd_port, uint8_t (*password)[8]);
int Eth_CmdDisconnect(uint8_t (*password)[8]);
int Eth_SendCmd(uint8_t *ptrTxData, uint8_t *ptrRxData, int txlen, int rxmax);
int Eth_CameraI2C_Read(uint8_t reg_addr, uint8_t *ptrRdBuf);
int Eth_CameraI2C_Write(uint8_t reg_addr, uint8_t reg_data);
int Eth_StartStream(uint16_t dma_size);
int Eth_StopStream(void);
int Eth_GetReadyFrameNumber(void);
int Eth_FrameFree(int number);
uint8_t *GetFrame(int number);


#ifdef __cplusplus
}
#endif

#endif
