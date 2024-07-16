#include <stdint.h>

#include "xaxidma.h"
#include "xil_cache.h"
#include "xil_cache_l.h"

#include "../common/util.h"
#include "../config.h"
#include "../profile.h"

#if CONFIG_ENABLE_HW_IDCT

static struct profile l1_flush_time;
static struct profile l2_flush_time;
static struct profile hw_idct_time;

static XAxiDma AxiDma;

void hw_idct_init()
{
    if (XAxiDma_CfgInitialize(&AxiDma, XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID)))
        error_and_exit("Cannot initialize DMA");
    XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
}

/*
 * @param rdata is read from Main Memory to DMA to Device
 * @param wdata is written from Device to DMA to Main Memory
 */
void hw_idct_do(void *rdata, uint32_t rsize, void *wdata, uint32_t wsize)
{
    /* TODO: profile this; might be better to flush/invalidate range */
	PROFILE_US_START
    Xil_L1DCacheFlush();
    PROFILE_US_STOP(&l1_flush_time);
    PROFILE_US_START
	/*
	 * XXX: assumes wdata not read back into L2 cache by another CPU until DMA
	 *      transaction is finished
	 */
    Xil_L2CacheFlush();
    PROFILE_US_STOP(&l2_flush_time);

    PROFILE_US_START
    if (XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)rdata, (u32)rsize, XAXIDMA_DMA_TO_DEVICE))
        error_and_exit("Cannot start dma-to-device transfer");
    if (XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)wdata, (u32)wsize, XAXIDMA_DEVICE_TO_DMA))
        error_and_exit("Cannot start device-to-dma transfer");

    while (!(XAxiDma_ReadReg(AxiDma.RxBdRing[0].ChanBase, XAXIDMA_SR_OFFSET) & XAXIDMA_ERR_INTERNAL_MASK));
    // while (XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA) || XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE));
    XAxiDma_Reset(&AxiDma);
    while (!XAxiDma_ResetIsDone(&AxiDma));
    PROFILE_US_STOP(&hw_idct_time);
}

void hw_idct_print_profile()
{
	PROFILE_PRINT_RESULTS("L1 Flush Time", "us", &l1_flush_time);
	PROFILE_PRINT_RESULTS("L2 Flush Time", "us", &l2_flush_time);
	PROFILE_PRINT_RESULTS("HW IDCT Time", "us", &hw_idct_time);
}

#endif /* CONFIG_ENABLE_HW_IDCT */
