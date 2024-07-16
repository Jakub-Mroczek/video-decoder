//Set up the SCU timer subsystem
//Note: this also initializes the interrupt controller
//	handler: pointer to the interrupt handler
int timer_gpio_init(void (*fun_timer)(void* data), void (*fun_gpio)(void* data));
//Start the timer in period mode
//	period: timer period. The timer runs at a frequency equal to half that of the core
void timer_start(unsigned int count);
//Stop the timer
void timer_stop();

int read_pin();

#define PIN_OFFSET 54

