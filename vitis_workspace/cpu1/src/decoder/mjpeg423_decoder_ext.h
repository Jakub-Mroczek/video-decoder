#ifndef _MJPEG423_DECODER_EXT_H
#define _MJPEG423_DECODER_EXT_H

#include "mjpeg423_decoder.h"

void hw_idct_init();
void hw_idct_start(void *rdata, uint32_t rsize, void *wdata, uint32_t wsize);
void hw_idct_poll();

#endif /* _MJPEG423_DECODER_EXT_H */
