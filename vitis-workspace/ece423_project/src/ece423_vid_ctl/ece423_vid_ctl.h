#ifndef __VDMA_H__
#define __VDMA_H__

#include "../common/mjpeg423_types.h"






uint32_t vdma_init(uint32_t width, uint32_t height, uint32_t frame_buff_limit);


uint32_t vdma_out();

rgb_pixel_t* buff_next();

uint32_t buff_reg();

void vdma_close();



#endif
