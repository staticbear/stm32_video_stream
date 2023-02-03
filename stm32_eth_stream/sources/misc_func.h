#ifndef _MISC_FUNC_H_
#define _MISC_FUNC_H_

#include "stm32f4xx.h"
#define FIFO_CNT_LIMIT 32

typedef struct {
	uint32_t fifo[FIFO_CNT_LIMIT];
	uint32_t fifo_cnt;
	uint32_t fifo_wr_ptr;
	uint32_t fifo_rd_ptr;
	uint32_t fifo_cnt_max;
}FIFO_T;

int InitFifo(FIFO_T *fifo_ptr, uint32_t max);
int PushFifo(FIFO_T *fifo_ptr, uint32_t addr);
uint32_t PopFifo(FIFO_T *fifo_ptr);
uint32_t GetFifoCnt(FIFO_T *fifo_ptr);
uint8_t cmp_data8(const uint8_t *src0, const uint8_t *src1, uint32_t size);
void mem_cpy8(uint8_t *dst, uint8_t *src, uint32_t size);

#endif
