
#include "misc_func.h"

//---------------------------------------------------------------------

int InitFifo(FIFO_T *fifo_ptr, uint32_t max)
{
	int i;
	if(fifo_ptr == 0 || max >= FIFO_CNT_LIMIT || max == 0)
		return 1;
	
	fifo_ptr->fifo_cnt_max = max;
	fifo_ptr->fifo_cnt = 0;
	fifo_ptr->fifo_wr_ptr = 0;
	fifo_ptr->fifo_rd_ptr = 0;
	
	for(i = 0; i < max; i++)
		fifo_ptr->fifo[i] = 0;
	
	return 0;
}

//---------------------------------------------------------------------

int PushFifo(FIFO_T *fifo_ptr, uint32_t val)
{
	if(fifo_ptr == 0 || fifo_ptr->fifo_cnt >= fifo_ptr->fifo_cnt_max)
		return 1;
	
	fifo_ptr->fifo[fifo_ptr->fifo_wr_ptr] = val;
	fifo_ptr->fifo_cnt++;
	
	if(fifo_ptr->fifo_wr_ptr >= fifo_ptr->fifo_cnt_max)
		fifo_ptr->fifo_wr_ptr = 0;
	else
		fifo_ptr->fifo_wr_ptr++;
	
	return 0;
}

//---------------------------------------------------------------------

uint32_t PopFifo(FIFO_T *fifo_ptr)
{
	uint32_t addr;
	
	if(fifo_ptr == 0 || fifo_ptr->fifo_cnt == 0)
		return 0;
	
	addr = fifo_ptr->fifo[fifo_ptr->fifo_rd_ptr];
	fifo_ptr->fifo_cnt--;
	
	if(fifo_ptr->fifo_rd_ptr >= fifo_ptr->fifo_cnt_max)
		fifo_ptr->fifo_rd_ptr = 0;
	else
		fifo_ptr->fifo_rd_ptr++;
	
	return addr;
}

//---------------------------------------------------------------------

uint32_t GetFifoCnt(FIFO_T *fifo_ptr)
{
	if(fifo_ptr == 0)
		return 0;
	
	return fifo_ptr->fifo_cnt;
}

//---------------------------------------------------------------------

uint8_t cmp_data8(const uint8_t *src0, const uint8_t *src1, uint32_t size)
{
	uint32_t i;
	
	if(size == 0)
		return 0;
	
	if(src0 == 0 || src1 == 0)
		return 0xFF;
	
	for(i = 0; i < size; i++)
	{
		if(src0[i] != src1[i])
			return 1;
	}
	
	return 0;
}

//---------------------------------------------------------------------

void mem_cpy8(uint8_t *dst, uint8_t *src, uint32_t size)
{
	uint32_t i;
	
	if(size == 0)
		return;
	
	if(dst == 0 || src == 0)
		return;
	
	for(i = 0; i < size; i++)
		dst[i] = src[i];
	
	return;
}
