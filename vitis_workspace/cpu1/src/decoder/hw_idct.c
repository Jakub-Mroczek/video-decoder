#include "xaxidma.h"
#include "xil_cache.h"
#include "xil_cache_l.h"

#include "../common/util.h"

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
void hw_idct_start(void *rdata, uint32_t rsize, void *wdata, uint32_t wsize)
{
	if (XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)rdata, (u32)rsize, XAXIDMA_DMA_TO_DEVICE))
		error_and_exit("Cannot start dma-to-device transfer");
	if (XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)wdata, (u32)wsize, XAXIDMA_DEVICE_TO_DMA))
		error_and_exit("Cannot start device-to-dma transfer");
}

void hw_idct_poll()
{
	while (!(XAxiDma_ReadReg(AxiDma.RxBdRing[0].ChanBase, XAXIDMA_SR_OFFSET) & XAXIDMA_ERR_INTERNAL_MASK));
	XAxiDma_Reset(&AxiDma);
	while (!XAxiDma_ResetIsDone(&AxiDma));
}
