#include "timer_gpio.h"

#include "xparameters.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xgpiops.h"

#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_IRPT_INTR		XPAR_SCUTIMER_INTR

static int TimerSetupIntrSystem(XScuGic *IntcInstancePtr,
			      XScuTimer *TimerInstancePtr, u16 TimerIntrId, void (*fun_timer)(void* data),void (*fun_gpio)(void* data));

//static void TimerDisableIntrSystem(XScuGic *IntcInstancePtr, u16 TimerIntrId);

XScuTimer TInstance;	/* Cortex A9 Scu Private Timer Instance */
XScuGic IInstance;		/* Interrupt Controller Instance */


XGpioPs Gpio;
XGpioPs_Config *ConfigPtr;



void setup_gpiops(void (*fun_gpio)(void* data));


int timer_gpio_init(void (*fun_timer)(void* data), void (*fun_gpio)(void* data))
{
	int Status;
	XScuTimer_Config *ConfigPtr;

	/*
	 * Initialize the Scu Private Timer driver.
	 */
	ConfigPtr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);

	/*
	 * This is where the virtual address would be used, this example
	 * uses physical address.
	 */
	Status = XScuTimer_CfgInitialize(&TInstance, ConfigPtr,
					ConfigPtr->BaseAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Perform a self-test to ensure that the hardware was built correctly.
	 */
	Status = XScuTimer_SelfTest(&TInstance);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the device to interrupt subsystem so that interrupts
	 * can occur.
	 */
	Status = TimerSetupIntrSystem(&IInstance,
					&TInstance, TIMER_IRPT_INTR, fun_timer, fun_gpio);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable Auto reload mode.
	 */
	XScuTimer_EnableAutoReload(&TInstance);

	/*
	 * Disable and disconnect the interrupt system.
	 */
	//TimerDisableIntrSystem(&IntcInstance, TimerIntrId);

	return XST_SUCCESS;

}

void timer_start(unsigned int count)
{
	/*
	 * Load the timer counter register.
	 */
	XScuTimer_LoadTimer(&TInstance, count);

	/*
	 * Start the timer counter
	 */
	XScuTimer_Start(&TInstance);
}


void timer_stop()
{
	XScuTimer_Stop(&TInstance);
}

/*****************************************************************************/
/**
*
* This function sets up the interrupt system such that interrupts can occur
* for the device.
*
* @param	IntcInstancePtr is a pointer to the instance of XScuGic driver.
* @param	TimerInstancePtr is a pointer to the instance of XScuTimer
*		driver.
* @param	TimerIntrId is the Interrupt Id of the XScuTimer device.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None.
*
******************************************************************************/
static int TimerSetupIntrSystem(XScuGic *IntcInstancePtr,
			      XScuTimer *TimerInstancePtr, u16 TimerIntrId, void (*fun_timer)(void* data),void (*fun_gpio)(void* data))
{
	int Status;

	XScuGic_Config *IntcConfig;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	Xil_ExceptionInit();



	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				(Xil_ExceptionHandler)XScuGic_InterruptHandler,
				IntcInstancePtr);

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, TimerIntrId,
				(Xil_ExceptionHandler)fun_timer,
				(void *)TimerInstancePtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	//setup for gpiops interrupts
	setup_gpiops(fun_gpio);
	Status = XScuGic_Connect(&IInstance, XPAR_XGPIOPS_0_INTR,
					(Xil_ExceptionHandler)XGpioPs_IntrHandler,
					(void *)&Gpio);
	XScuGic_Enable(&IInstance, XPAR_XGPIOPS_0_INTR);


	/*
	 * Enable the interrupt for the device.
	 */
	XScuGic_Enable(IntcInstancePtr, TimerIntrId);

	/*
	 * Enable the timer interrupts for timer mode.
	 */
	XScuTimer_EnableInterrupt(TimerInstancePtr);

	/*
	 * Enable interrupts in the Processor.
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

void setup_gpiops(void (*fun_gpio)(void* data)){
	int Status;
	/* Initialize the GPIO driver. */
	ConfigPtr = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
	Status = XGpioPs_CfgInitialize(&Gpio, ConfigPtr,
					ConfigPtr->BaseAddr);


	XGpioPs_SetDirectionPin(&Gpio, PIN_OFFSET, 0);
	XGpioPs_SetOutputEnablePin(&Gpio, PIN_OFFSET, 0);

	XGpioPs_SetDirectionPin(&Gpio, PIN_OFFSET+1, 0);
	XGpioPs_SetOutputEnablePin(&Gpio, PIN_OFFSET+1, 0);

	XGpioPs_SetDirectionPin(&Gpio, PIN_OFFSET+2, 0);
	XGpioPs_SetOutputEnablePin(&Gpio, PIN_OFFSET+2, 0);

	XGpioPs_SetDirectionPin(&Gpio, PIN_OFFSET+3, 0);
	XGpioPs_SetOutputEnablePin(&Gpio, PIN_OFFSET+3, 0);

	XGpioPs_SetIntrTypePin(&Gpio, PIN_OFFSET, XGPIOPS_IRQ_TYPE_EDGE_RISING);
	XGpioPs_SetIntrTypePin(&Gpio, PIN_OFFSET+1, XGPIOPS_IRQ_TYPE_EDGE_RISING);
	XGpioPs_SetIntrTypePin(&Gpio, PIN_OFFSET+2, XGPIOPS_IRQ_TYPE_EDGE_RISING);
	XGpioPs_SetIntrTypePin(&Gpio, PIN_OFFSET+3, XGPIOPS_IRQ_TYPE_EDGE_RISING);


	XGpioPs_SetCallbackHandler(&Gpio, (void *)&Gpio, (XGpioPs_Handler)fun_gpio);


	/* Enable the GPIO interrupts of GPIO Bank. */
	XGpioPs_IntrEnablePin(&Gpio,PIN_OFFSET);
	XGpioPs_IntrEnablePin(&Gpio,PIN_OFFSET+1);
	XGpioPs_IntrEnablePin(&Gpio,PIN_OFFSET+2);
	XGpioPs_IntrEnablePin(&Gpio,PIN_OFFSET+3);
}

int read_pin(){
	if(XGpioPs_ReadPin(&Gpio, PIN_OFFSET)) return 0;
	else if(XGpioPs_ReadPin(&Gpio, PIN_OFFSET+1)) return 1;
	else if(XGpioPs_ReadPin(&Gpio, PIN_OFFSET+2)) return 2;
	else if(XGpioPs_ReadPin(&Gpio, PIN_OFFSET+3)) return 3;
	return -1;
}



