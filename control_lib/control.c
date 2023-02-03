#include "control.h"

#define MY_PORT_CMD 		0x2771
#define MY_PORT_DATA		0x2772

#define FRAME_WIDTH     	1280
#define FRAME_HEIGHT   		1024

#define MAX_FRAME_BUF_CNT	2

#define MAX_FRAME_SIZE		(FRAME_WIDTH * FRAME_HEIGHT * 2)

#define MAX_DMA_TRANSFER    1280
#define MIN_DMA_TRANSFER    320

#define FRAME_BEGIN			0x55AA

struct sockaddr_in cmd_to;
struct sockaddr_in cmd_from;
struct sockaddr_in data_from;

int socket_cmd;
int socket_data;

uint8_t save_pass[8] = {0};

int connected_status = STATUS_DISCONNECTED;
int thread_status = THREAD_STOP;
int stream_status = STREAM_STOP;
int set_dma_size = 0;

pthread_t thread_id;
FRAME_DATA_T frame_data[MAX_FRAME_BUF_CNT];

void *thread_recv_data(void *ptr);

//---------------------------------------------------------------------

int Eth_Init()
{
	int ret = 0;
	
	if(connected_status != STATUS_DISCONNECTED){
		Eth_Disconnect();
		Eth_Free();
	}
		
	thread_status = THREAD_STOP;
	stream_status = STREAM_STOP;
	
	// alloc memory for frames
	for(int i = 0; i < MAX_FRAME_BUF_CNT; i++){
		frame_data[i].buf = malloc(MAX_FRAME_SIZE);
		frame_data[i].status = BUF_FREE;
		if(frame_data[i].buf == NULL)
			ret = 1;
	}
	
	if(ret != 0)
	{
		for(int i = 0; i < MAX_FRAME_BUF_CNT; i++){
			if(frame_data[i].buf != NULL)
				free(frame_data[i].buf);
		}
	}
	
	return ret;
}

//---------------------------------------------------------------------

int Eth_Free()
{
	if(connected_status != STATUS_DISCONNECTED)
		return 1;
	
	for(int i = 0; i < MAX_FRAME_BUF_CNT; i++){
		if(frame_data[i].buf != NULL)
			free(frame_data[i].buf);
	}
	
	return 0;
}

//---------------------------------------------------------------------

void Eth_Disconnect()
{
	if(connected_status == STATUS_DISCONNECTED){
		printf("Eth_Disconnect, not connected\n");
		return;
	}
	
	if(thread_status != STREAM_STOP)
	{
		thread_status = THREAD_STOPPING;
		stream_status = STREAM_STOP;
	
		sleep(1);
		if (thread_status != THREAD_STOP){
			printf("Eth_Disconnect, thread stop error\n");
			exit(0);
		}
	}
	
	Eth_CmdDisconnect(&save_pass);

	close(socket_cmd);
	close(socket_data);
	
	connected_status = STATUS_DISCONNECTED;
	
	return;
}


//---------------------------------------------------------------------

int Eth_CmdDisconnect(uint8_t (*password)[8])
{
	
	CMD_ARG_T cmd, cmd_answ;
	memset(&cmd, 0, sizeof(CMD_ARG_T));
	memset(&cmd_answ, 0, sizeof(CMD_ARG_T));
	
	cmd.cmd = CMD_DISCONNECT;
	DISCONNECT_ARG_T *disconnect_arg = (DISCONNECT_ARG_T *)&cmd.data[0];
	memcpy(&disconnect_arg->pass[0], password, 8);
	
	int ret = Eth_SendCmd((uint8_t*)&cmd, (uint8_t*)&cmd_answ, sizeof(CMD_ARG_T), sizeof(CMD_ARG_T));
	if(ret != 0){
		printf("Eth_CmdDisconnect, Eth_SendCmd ret = %d\n", ret);
		return 1;
	}
	
	if(cmd_answ.cmd != CMD_DISCONNECT || cmd_answ.data[0] != ANSW_OK){
		printf("Eth_CmdDisconnect, Eth_SendCmd cmd_answ.cmd = %d cmd_answ.data[0] = %d\n", cmd_answ.cmd, cmd_answ.data[0]);
		return 2;
	}
	
	return 0;
}


//---------------------------------------------------------------------

int Eth_Connect(const char *device_ip, int device_cmd_port, uint8_t (*password)[8])
{
	int ret;
	
	if(connected_status == STATUS_CONNECTED){
		printf("Eth_Connect, already connected\n");
		return 1;
	}
	
	printf("Eth_Connect, start init, device_ip: %.15s\n", device_ip);
	
	// create thread to recv camera image data
	thread_status = THREAD_STARTING;
	ret = pthread_create(&thread_id, NULL, thread_recv_data, NULL);
	if(ret != 0){
		printf("Eth_Connect, pthread_create ret = %d\n", ret);
		return 4;
	}
	
	sleep(1);
	if(thread_status != THREAD_RUN){
		printf("Eth_Connect, thread start error\n");
		exit(0);
	}
	
	// create sockets and bind
	socket_cmd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(socket_cmd < 0){
		printf("Eth_Connect, socket_cmd =  %d\n", socket_cmd);
		thread_status = THREAD_STOPPING;
		sleep(1);
        if (thread_status != THREAD_STOP){
              printf("Eth_Connect, thread stop error\n");
			  exit(0);
		}
		return 5;
	}
	
	socket_data = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(socket_data < 0){
		printf("Eth_Connect, socket_data =  %d\n", socket_data);
		thread_status = THREAD_STOPPING;
		sleep(1);
        if (thread_status != THREAD_STOP){
              printf("Eth_Connect, thread stop error\n");
			  exit(0);
		}
		close(socket_cmd);
		return 6;
	}
	
	memset(&cmd_to, 0, sizeof(struct sockaddr_in)); 
	memset(&cmd_from, 0, sizeof(struct sockaddr_in)); 
    memset(&data_from, 0, sizeof(struct sockaddr_in)); 
	
	cmd_to.sin_family      = AF_INET;
    cmd_to.sin_addr.s_addr = inet_addr(device_ip);
    cmd_to.sin_port        = htons(device_cmd_port);
	
	cmd_from.sin_family      = AF_INET;
    cmd_from.sin_addr.s_addr = INADDR_ANY;
    cmd_from.sin_port        = htons(MY_PORT_CMD);
	
	data_from.sin_family      = AF_INET;
    data_from.sin_addr.s_addr = INADDR_ANY;
    data_from.sin_port        = htons(MY_PORT_DATA);
	
	struct timeval read_timeout;
	read_timeout.tv_sec = 1;
	read_timeout.tv_usec = 0;
	if(setsockopt(socket_cmd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)) < 0){
		printf("Eth_Connect, setsockopt(socket_cmd) error\n");
		return 7;
	}
	
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 500000;
	if(setsockopt(socket_data, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)) < 0){
		printf("Eth_Connect, setsockopt(socket_data) error\n");
		return 8;
	}
	
	ret = bind(socket_cmd, (struct sockaddr*)&cmd_from, sizeof(struct sockaddr_in));
	if(ret < 0){
		printf("Eth_Connect, bind socket_cmd ret = %d\n", ret);
		thread_status = THREAD_STOPPING;
		sleep(1);
        if (thread_status != THREAD_STOP){
              printf("Eth_Connect, thread stop error\n");
			  exit(0);
		}
		close(socket_cmd);
		close(socket_data);
		return 9;
	}
	
	ret = bind(socket_data, (struct sockaddr*)&data_from, sizeof(struct sockaddr_in));
	if(ret < 0){
		printf("Eth_Connect, bind data_from ret = %d\n", ret);
		thread_status = THREAD_STOPPING;
		sleep(1);
        if (thread_status != THREAD_STOP){
              printf("Eth_Connect, thread stop error\n");
			  exit(0);
		}
		close(socket_cmd);
		close(socket_data);
		return 10;
	}
	
	// to be sure no one has connected to the device now
	Eth_CmdDisconnect(password);
	
	// try to connect to the device
	CMD_ARG_T cmd, cmd_answ;
	memset(&cmd, 0, sizeof(CMD_ARG_T));
	memset(&cmd_answ, 0, sizeof(CMD_ARG_T));
	
	cmd.cmd = CMD_CONNECT;
	CONNECT_ARG_T *connect_arg = (CONNECT_ARG_T *)&cmd.data[0];
	memcpy(&connect_arg->pass[0], password, 8);
	connect_arg->data_port = (uint16_t)((MY_PORT_DATA >> 8) |
							 (MY_PORT_DATA << 8));
	
	
	printf("Eth_Connect, pass: %.8s\n", connect_arg->pass);
	
	ret = Eth_SendCmd((uint8_t*)&cmd, (uint8_t*)&cmd_answ, sizeof(CMD_ARG_T), sizeof(CMD_ARG_T));
	printf("Eth_Connect, Eth_SendCmd ret = %d\n", ret);
	if(ret != 0){
		thread_status = THREAD_STOPPING;
		sleep(1);
        if (thread_status != THREAD_STOP){
              printf("Eth_Connect, thread error\n");
			  exit(0);
		}
		close(socket_cmd);
		close(socket_data);
		return 11;
	}
	
	printf("Eth_Connect, Eth_SendCmd ret = %d, answ_cmd = %d, answ_code = %d\n", ret, cmd_answ.cmd, cmd_answ.data[0]);
	
	if(cmd_answ.cmd != CMD_CONNECT || cmd_answ.data[0] != ANSW_OK){
		thread_status = THREAD_STOPPING;
		sleep(1);
        if (thread_status != THREAD_STOP){
              printf("Eth_Connect, thread stop error\n");
			  exit(0);
		}
		close(socket_cmd);
		close(socket_data);
		return 12;
	}
	
	memcpy(save_pass, password, sizeof(save_pass));
	
	connected_status = STATUS_CONNECTED;
	
	return 0;
}

//---------------------------------------------------------------------

int Eth_DeviceList(int device_cmd_port, uint8_t *ptrBuf, int buf_size, int *rcv_size)
{
	int socket_cmd_brd;
	struct sockaddr_in cmd_to_brd;
	struct sockaddr_in cmd_from_brd;
	int ret;
	
	if(buf_size < (int)sizeof(CMD_ARG_T) || (buf_size % sizeof(CMD_ARG_T)) != 0){
		printf("Eth_DeviceList, wrong buf_size\n");
		return 1;
	}
	
	if(connected_status == STATUS_CONNECTED){
		printf("Eth_DeviceList, can't send broadcast while device connected\n");
		return 2;
	}
	
	socket_cmd_brd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(socket_cmd_brd < 0){
		printf("Eth_DeviceList, socket_cmd_brd error\n");
		return 3;
	}
	
	memset(&cmd_to_brd, 0, sizeof(struct sockaddr_in)); 
	memset(&cmd_from_brd, 0, sizeof(struct sockaddr_in));
	
	cmd_to_brd.sin_family      = AF_INET;
    cmd_to_brd.sin_addr.s_addr = inet_addr("192.168.1.255");
    cmd_to_brd.sin_port        = htons(device_cmd_port);
	
	cmd_from_brd.sin_family      = AF_INET;
    cmd_from_brd.sin_addr.s_addr = INADDR_ANY;
    cmd_from_brd.sin_port        = htons(MY_PORT_CMD);
	
	struct timeval read_timeout;
	read_timeout.tv_sec = 1;
	read_timeout.tv_usec = 0;
	if(setsockopt(socket_cmd_brd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)) < 0)
	{
		printf("Eth_DeviceList, setsockopt error\n");
		close(socket_cmd_brd);
		return 4;
	}
	
	int yes=1;
	if(setsockopt(socket_cmd_brd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0)
	{
		printf("Eth_DeviceList, setsockopt error\n");
		close(socket_cmd_brd);
		return 5;
	}
	
	ret = bind(socket_cmd_brd, (struct sockaddr*)&cmd_from_brd, sizeof(struct sockaddr_in));
	if(ret < 0){
		printf("Eth_DeviceList, bind error\n");
		close(socket_cmd_brd);
		return 6;
	}
	
	CMD_ARG_T cmd_arg;
	cmd_arg.cmd = CMD_STATUS;
	ret = sendto(socket_cmd_brd, (char *)&cmd_arg, sizeof(CMD_ARG_T), 0, (struct sockaddr*)&cmd_to_brd, sizeof(cmd_to_brd));
	if(ret != sizeof(CMD_ARG_T)){
		printf("Eth_DeviceList, ret != txlen\n");
		close(socket_cmd_brd);
		return 7;
	}
	
	int recv_ptr = 0;
	do{
		ret = recvfrom(socket_cmd_brd, (char *)&ptrBuf[recv_ptr], sizeof(CMD_ARG_T), 0, NULL, NULL);
		if(ret == SO_ERROR){
			printf("Eth_DeviceList, recvfrom SO_ERROR \n");
			close(socket_cmd_brd);
			return 8;
		}
		
		if(ret == 0 || ret == -1)
			break;
		
		recv_ptr += ret;
	}
	while(recv_ptr < buf_size);
	
	close(socket_cmd_brd);
	
	*rcv_size = recv_ptr;
	
	return 0;
}

//---------------------------------------------------------------------

int Eth_ChangeIp(int device_cmd_port, uint32_t device_id, uint32_t new_ip)
{
	int socket_cmd_brd;
	struct sockaddr_in cmd_to_brd;
	struct sockaddr_in cmd_from_brd;
	int ret;
	
	if(new_ip == 0){
		printf("Eth_ChangeIp, new_ip = null\n");
		return 1;
	}
	
	if(connected_status == STATUS_CONNECTED){
		printf("Eth_ChangeIp, can't send broadcast while device connected\n");
		return 2;
	}
	
	socket_cmd_brd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(socket_cmd_brd < 0){
		printf("Eth_ChangeIp, socket_cmd_brd error\n");
		return 3;
	}
	
	memset(&cmd_to_brd, 0, sizeof(struct sockaddr_in)); 
	memset(&cmd_from_brd, 0, sizeof(struct sockaddr_in));
	
	cmd_to_brd.sin_family      = AF_INET;
    cmd_to_brd.sin_addr.s_addr = inet_addr("192.168.1.255");
    cmd_to_brd.sin_port        = htons(device_cmd_port);
	
	cmd_from_brd.sin_family      = AF_INET;
    cmd_from_brd.sin_addr.s_addr = INADDR_ANY;
    cmd_from_brd.sin_port        = htons(MY_PORT_CMD);
	
	struct timeval read_timeout;
	read_timeout.tv_sec = 1;
	read_timeout.tv_usec = 0;
	if(setsockopt(socket_cmd_brd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)) < 0)
	{
		printf("Eth_ChangeIp, setsockopt error\n");
		close(socket_cmd_brd);
		return 4;
	}
	
	int yes=1;
	if(setsockopt(socket_cmd_brd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0)
	{
		printf("Eth_ChangeIp, setsockopt error\n");
		close(socket_cmd_brd);
		return 5;
	}
	
	ret = bind(socket_cmd_brd, (struct sockaddr*)&cmd_from_brd, sizeof(struct sockaddr_in));
	if(ret < 0){
		printf("Eth_ChangeIp, bind error\n");
		close(socket_cmd_brd);
		return 6;
	}
	
	CMD_ARG_T cmd_arg;
	cmd_arg.cmd = CMD_CHANGE_IP;
	CHANGE_IP_ARG_T * chg_ip_arg = (CHANGE_IP_ARG_T *)&cmd_arg.data[0];
	chg_ip_arg->device_id = device_id;
	chg_ip_arg->new_ip[0] = (uint16_t)(new_ip & 0xFFFF);
	chg_ip_arg->new_ip[1] = (uint16_t)(new_ip >> 16);
	
	ret = sendto(socket_cmd_brd, (char *)&cmd_arg, sizeof(CMD_ARG_T), 0, (struct sockaddr*)&cmd_to_brd, sizeof(cmd_to_brd));
	if(ret != sizeof(CMD_ARG_T)){
		printf("Eth_ChangeIp, ret != txlen\n");
		close(socket_cmd_brd);
		return 7;
	}
	

	ret = recvfrom(socket_cmd_brd, (char *)&cmd_arg, sizeof(CMD_ARG_T), 0, NULL, NULL);
	close(socket_cmd_brd);
	
	if(ret == SO_ERROR || ret == 0 || ret == -1)
		return 0;

	if(ret != sizeof(CMD_ARG_T)){
		printf("Eth_ChangeIp, recvfrom lost data\n");
		return 8;
	}
	else if(cmd_arg.cmd == ANSW_IN_USE){
		printf("Eth_ChangeIp, can't change ip while device in use\n");
		return 9;
	}
	else{
		printf("Eth_ChangeIp, answer %x\n", cmd_arg.cmd);
		return 10;
	}
}

//---------------------------------------------------------------------

int Eth_SendCmd(uint8_t *ptrTxData, uint8_t *ptrRxData, int txlen, int rxmax)
{
	int ret;
	
	if(ptrTxData == 0){
		printf("Eth_SendCmd, ptrTxData = null\n");
		return 1;
	}
	
	if(ptrRxData == 0){
		printf("Eth_SendCmd, ptrRxData = null\n");
		return 2;
	}
	
	if(txlen < (int)sizeof(CMD_ARG_T) || (txlen % sizeof(CMD_ARG_T)) != 0){
		printf("Eth_SendCmd, wrong txlen\n");
		return 3;
	}
	
	if(rxmax < (int)sizeof(CMD_ARG_T) || (rxmax % sizeof(CMD_ARG_T)) != 0){
		printf("Eth_SendCmd, wrong rxmax\n");
		return 4;
	}
		
	ret = sendto(socket_cmd, (char *)ptrTxData, txlen, 0, (struct sockaddr*)&cmd_to, sizeof(cmd_to));
	if(ret != txlen){
		printf("Eth_SendCmd, ret != txlen\n");
		return 5;
	}

	ret = recvfrom(socket_cmd, (char *)ptrRxData, rxmax, 0, NULL, NULL);
	if(ret == SO_ERROR || ret == 0 || ret == -1){
		printf("Eth_SendCmd, recvfrom SO_ERROR \n");
		return 6;
	}
	else
		return 0;
}

//---------------------------------------------------------------------

void *thread_recv_data(void *ptr)
{
	thread_status = THREAD_RUN;
	int act_buf = 0;
	int nxt_buf = 0;
	int rcv_buf = 0;
	int wr_ptr = 0;
	int recv_len;
	int sm = SM_WAIT_SIGN;
	
	while(1)
	{
		if(thread_status == THREAD_STOPPING)
			break;
		
		if(stream_status != STREAM_RUN){
			if(sm != SM_WAIT_SIGN)
			{
				act_buf = 0;
				wr_ptr = 0;
				sm = SM_WAIT_SIGN;
			}
			usleep(5000);
			continue;
		}
		
		recv_len = recvfrom(socket_data, (char *)&frame_data[act_buf].buf[wr_ptr], set_dma_size, 0, NULL, NULL);
		if(recv_len == SO_ERROR || recv_len == 0 || recv_len == -1)
		{
			printf("thread_recv_data, recv_len == SO_ERROR || recv_len == 0 %d\n", recv_len);
			//printf("something went wrong with data %s \n", strerror(errno));
			sm = SM_WAIT_SIGN;
			wr_ptr = 0;
			usleep(5000);
			continue;
		}
		if(recv_len != set_dma_size){
			//printf("thread_recv_data, recv_len != set_dma_size %d\n", recv_len);
			stream_status = STREAM_STOP;
			sm = SM_WAIT_SIGN;
			wr_ptr = 0;
			usleep(5000);
			continue;
		}
		
		if(sm == SM_WAIT_SIGN){
			if(*(uint16_t*)&frame_data[act_buf].buf[0] == FRAME_BEGIN){
				//printf("thread_recv_data, sm == SM_WAIT_SIGN, FRAME_BEGIN\n");
				wr_ptr += recv_len;
				sm = SM_READ_DATA;
			}
			continue;
		}
		else if(sm == SM_READ_DATA){
			
			wr_ptr += recv_len;
			if(wr_ptr >= MAX_FRAME_SIZE){
				
				//printf("thread_recv_data, sm == SM_READ_DATA, wr_ptr >= MAX_FRAME_SIZE\n");
				wr_ptr = 0;
				
				if(act_buf < MAX_FRAME_BUF_CNT - 1)
					nxt_buf = act_buf + 1;
				else
					nxt_buf = 0;
				
				if(frame_data[nxt_buf].status == BUF_FREE){
					rcv_buf = act_buf;
					act_buf = nxt_buf;
					sm = SM_CHECK_SIGN;
				}
				else
					sm = SM_WAIT_SIGN;
			}

			continue;
		}
		else if(sm == SM_CHECK_SIGN){
			if(*(uint16_t*)&frame_data[act_buf].buf[0] == FRAME_BEGIN){
				//printf("thread_recv_data, sm == SM_CHECK_SIGN, FRAME_BEGIN\n");
				frame_data[rcv_buf].status = BUF_VALID;
				wr_ptr += recv_len;
				sm = SM_READ_DATA;
			}
			else{
				//printf("thread_recv_data, sm == SM_CHECK_SIGN, FRAME_BEGIN wrong\n");
				wr_ptr = 0;
				sm = SM_WAIT_SIGN;
			}
			continue;
		}
	}
	
	stream_status = STREAM_STOP;
	thread_status = THREAD_STOP;
	
	return NULL;
}

//---------------------------------------------------------------------

int Eth_StartStream(uint16_t dma_size)
{
	int ret;

	if(connected_status != STATUS_CONNECTED){
		printf("Eth_StartStream, not connected\n");
		return 1;
	}
	
	if(thread_status != THREAD_RUN){
		printf("Eth_StartStream, recv thread stopped\n");
		return 2;
	}
	
	if(stream_status == STREAM_RUN){
		printf("Eth_StartStream, stread already run\n");
		return 3;
	}
	
	if(dma_size > MAX_DMA_TRANSFER || dma_size < MIN_DMA_TRANSFER || (dma_size % 4)){
		printf("Eth_StartStream, wring dma size\n");
		return 4;
	}
	
	// start stream
	CMD_ARG_T cmd, cmd_answ;
	memset(&cmd, 0, sizeof(CMD_ARG_T));
	memset(&cmd_answ, 0, sizeof(CMD_ARG_T));
	
	cmd.cmd = CMD_STREAM_ON;
	STREAM_ON_ARG_T *stream_on_arg = (STREAM_ON_ARG_T *)&cmd.data[0];
	stream_on_arg->dma_size = dma_size;
	
	printf("Eth_StartStream, dma_size: %d\n", dma_size);
	
	ret = Eth_SendCmd((uint8_t*)&cmd, (uint8_t*)&cmd_answ, sizeof(CMD_ARG_T), sizeof(CMD_ARG_T));
	if(ret != 0){
		printf("Eth_StartStream, Eth_SendCmd ret = %d\n", ret);
		return 5;
	}
	
	if(cmd_answ.cmd != CMD_STREAM_ON || cmd_answ.data[0] != ANSW_OK){
		printf("Eth_StartStream, CMD_STREAM_ON ret = %d\n", cmd_answ.data[0]);
		return 6;
	}
	
	set_dma_size = dma_size;
	
	stream_status = STREAM_RUN;
	
	return 0;
}	

//---------------------------------------------------------------------

int Eth_StopStream()
{
	int ret;

	if(connected_status != STATUS_CONNECTED){
		printf("Eth_StopStream, not connected\n");
		return 1;
	}
	
	if(stream_status != STREAM_RUN){
		printf("Eth_StopStream, stread already stopped\n");
		return 3;
	}
	
	// stop stream
	CMD_ARG_T cmd, cmd_answ;
	memset(&cmd, 0, sizeof(CMD_ARG_T));
	memset(&cmd_answ, 0, sizeof(CMD_ARG_T));
	
	cmd.cmd = CMD_STREAM_OFF;
	
	ret = Eth_SendCmd((uint8_t*)&cmd, (uint8_t*)&cmd_answ, sizeof(CMD_ARG_T), sizeof(CMD_ARG_T));
	if(ret != 0){
		printf("Eth_StopStream, Eth_SendCmd ret = %d\n", ret);
		return 4;
	}
	
	if(cmd_answ.cmd != CMD_STREAM_OFF || cmd_answ.data[0] != ANSW_OK){
		printf("Eth_StopStream, CMD_STREAM_OFF ret = %d\n", cmd_answ.data[0]);
		return 5;
	}
	
	stream_status = STREAM_STOP;
	
	return 0;
}

//---------------------------------------------------------------------

int Eth_CameraI2C_Read(uint8_t reg_addr, uint8_t *ptrRdBuf)
{
	int ret;

	if(connected_status != STATUS_CONNECTED){
		printf("Eth_CameraI2C_Read, not connected\n");
		return 1;
	}
	
	// stop stream
	CMD_ARG_T cmd;
	CMD_ARG_T cmd_answ;
	memset(&cmd, 0, sizeof(CMD_ARG_T));
	memset(&cmd_answ, 0, sizeof(CMD_ARG_T));
	
	cmd.cmd = CMD_CAMERA_RD;
	CAMERA_RD_ARG_T *cam_rd_arg = (CAMERA_RD_ARG_T *)&cmd.data[0];
	cam_rd_arg->reg_addr = reg_addr;
	
	ret = Eth_SendCmd((uint8_t*)&cmd, (uint8_t*)&cmd_answ, sizeof(CMD_ARG_T), sizeof(CMD_ARG_T));
	if(ret != 0){
		printf("Eth_CameraI2C_Read, Eth_SendCmd ret = %d\n", ret);
		return 2;
	}
	
	if(cmd_answ.cmd != CMD_CAMERA_RD || cmd_answ.data[0] != ANSW_OK){
		printf("Eth_CameraI2C_Read, CMD_CAMERA_RD ret = %d\n", cmd_answ.data[1]);
		return 3;
	}
	
	printf("Eth_CameraI2C_Read, reg_addr = %d rd_data = %d\n", reg_addr, cmd_answ.data[2]);
	
	*ptrRdBuf = cmd_answ.data[2];
	
	return 0;
}

//---------------------------------------------------------------------

int Eth_CameraI2C_Write(uint8_t reg_addr, uint8_t reg_data)
{
	int ret;

	if(connected_status != STATUS_CONNECTED){
		printf("Eth_CameraI2C_Write, not connected\n");
		return 1;
	}
	
	// stop stream
	CMD_ARG_T cmd, cmd_answ;
	memset(&cmd, 0, sizeof(CMD_ARG_T));
	memset(&cmd_answ, 0, sizeof(CMD_ARG_T));
	
	cmd.cmd = CMD_CAMERA_WR;
	CAMERA_WR_ARG_T *cam_rd_arg = (CAMERA_WR_ARG_T *)&cmd.data[0];
	cam_rd_arg->reg_addr = reg_addr;
	cam_rd_arg->reg_data = reg_data;
	
	ret = Eth_SendCmd((uint8_t*)&cmd, (uint8_t*)&cmd_answ, sizeof(CMD_ARG_T), sizeof(CMD_ARG_T));
	if(ret != 0){
		printf("Eth_CameraI2C_Write, Eth_SendCmd ret = %d\n", ret);
		return 2;
	}
	
	if(cmd_answ.cmd != CMD_CAMERA_WR || cmd_answ.data[0] != ANSW_OK){
		printf("Eth_CameraI2C_Write, CMD_CAMERA_WR ret = %d\n", cmd_answ.data[0]);
		return 3;
	}
	
	return 0;
}

//---------------------------------------------------------------------

int Eth_GetReadyFrameNumber()
{
	for(int i = 0; i < MAX_FRAME_BUF_CNT; i++){
		if(frame_data[i].status == BUF_VALID)
			return i;
	}
	return -1;
}

//---------------------------------------------------------------------

int Eth_FrameFree(int number)
{
	if(number > MAX_FRAME_BUF_CNT - 1 || number < 0)
		return -1;
	
	frame_data[number].status = BUF_FREE;
	
	return 0;
}

//---------------------------------------------------------------------

uint8_t *GetFrame(int number)
{
	if(number > MAX_FRAME_BUF_CNT - 1 || number < 0)
		return 0;
	
	if(frame_data[number].status != BUF_VALID)
		return 0;
	
	return frame_data[number].buf;
}
