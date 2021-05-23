#include "stm32f1xx_hal.h"
#include "ticker.h"


volatile uint32_t clocks_per_usec = 0;

uint32_t delay_usec_init(void)
{
	/* Enable TRC */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	/* Enable Data Watchpoint and Trace clock cycle counter */
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	/* Reset the clock cycle counter value */
	DWT->CYCCNT = 0;
	/* add small delay to check if clock cycle counter started */
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	clocks_per_usec = 0;
	if (DWT->CYCCNT)
		clocks_per_usec = SystemCoreClock / 1000000;

	return clocks_per_usec;
}