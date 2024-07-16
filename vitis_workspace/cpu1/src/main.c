#include "decoder/mjpeg423_decoder_ext.h"
#include "ece423_vid_ctl/ece423_vid_ctl.h"
#include "timer_gpio/timer_gpio.h"

static timer_isr(void *data)
{

}

static gpio_isr(void *data)
{

}

int main(int argc, const char *argv[])
{
	hw_idct_init();

	vdma_init(CONFIG_VBUFF_WIDTH, CONFIG_VBUFF_HEIGHT, CONFIG_VBUFF_LENGTH);

	timer_gpio_init(timer_isr, gpio_isr);

	return 0;
}
