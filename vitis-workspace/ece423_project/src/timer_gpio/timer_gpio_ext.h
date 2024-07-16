#ifndef _TIMER_GPIO_EXT_H
#define _TIMER_GPIO_EXT_H

#include "xparameters.h"
#include "xscugic.h"

extern XScuGic IInstance;

static inline void gpio_disable_irq(void)
{
    XScuGic_Disable(&IInstance, XPAR_XGPIOPS_0_INTR);
}

static inline void gpio_enable_irq(void)
{
    XScuGic_Enable(&IInstance, XPAR_XGPIOPS_0_INTR);
}

static inline void timer_disable_irq(void)
{
    XScuGic_Disable(&IInstance, XPAR_SCUTIMER_INTR);
}

static inline void timer_enable_irq(void)
{
    XScuGic_Enable(&IInstance, XPAR_SCUTIMER_INTR);
}

#endif /* _TIMER_GPIO_EXT_H */
