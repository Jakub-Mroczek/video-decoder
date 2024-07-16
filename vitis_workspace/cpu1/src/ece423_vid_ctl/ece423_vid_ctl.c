#include "ece423_vid_ctl.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_types.h"
#include "stdio.h"
#include "../common/util.h"
#define CHANNELS 	3


#define write_reg(BaseAddress, RegOffset, Data) Xil_Out32((BaseAddress) + (RegOffset), (uint32_t)(Data))
#define read_reg(BaseAddress, RegOffset)        Xil_In32((BaseAddress) + (RegOffset))

rgb_pixel_t** frame_buff;


static volatile int32_t front;
static volatile int32_t rear;
static volatile int32_t mid;

static uint32_t h_size, w_size, frame_buff_limit;

rgb_pixel_t* buff_disp();



uint32_t vdma_init(uint32_t width, uint32_t height, uint32_t frame_buff_size){
	front = 0;
	rear = 0;
	h_size = height;
	w_size = width;
	frame_buff_limit = frame_buff_size;
	frame_buff = malloc(frame_buff_limit * sizeof(frame_buff));
    write_reg(XPAR_AXI_VDMA_0_BASEADDR, 0x00, 0x04);	// read reset
    while(read_reg(XPAR_AXI_VDMA_0_BASEADDR, 0x00) & 0x4){}
    for (int count = 0; count < frame_buff_limit; count++){
		if((frame_buff[count] = malloc(width*height*sizeof(rgb_pixel_t)))==NULL) return FALSE;
	}
    return TRUE;
}


uint32_t vdma_out(){
	rgb_pixel_t* frame_out = buff_disp();
	if (frame_out != NULL){
		write_reg(XPAR_AXI_VDMA_0_BASEADDR, 0x00, 0x83);


		write_reg(XPAR_AXI_VDMA_0_BASEADDR, 0x5C, frame_out);

		write_reg(XPAR_AXI_VDMA_0_BASEADDR, 0x58, w_size*CHANNELS);
		write_reg(XPAR_AXI_VDMA_0_BASEADDR, 0x54, w_size*CHANNELS);

		write_reg(XPAR_AXI_VDMA_0_BASEADDR, 0x50, h_size);
		return TRUE;
	} else {
		return FALSE;
	}
}


rgb_pixel_t* buff_next(){
	if(((front +1) % frame_buff_limit) != (rear % frame_buff_limit)){
		front++;
		return frame_buff[front % frame_buff_limit];
	}	else {
		return NULL;
	}
}

uint32_t buff_reg(){
	if(mid < front){
		mid++;
		Xil_DCacheFlushRange((UINTPTR)frame_buff[mid  % frame_buff_limit], w_size*h_size*sizeof(rgb_pixel_t));
		return TRUE;
	} else return FALSE;
}


rgb_pixel_t* buff_disp(){
	if ((mid - rear) >= 1){
		return frame_buff[++rear % frame_buff_limit];
	} else {
		return NULL;
	}
}


void vdma_close(){
	for (int count = 0; count < frame_buff_limit; count++){
		free(frame_buff[count]);
	}
}



