#ifndef _HW_IDCT_H
#define _HW_IDCT_H

void hw_idct_init();
void hw_idct_do(void *rdata, uint32_t rsize, void *wdata, uint32_t wsize);
void hw_idct_print_profile();

#endif /* _HW_IDCT */
