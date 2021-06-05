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

void ticker_init(ticker_t *tick, uint32_t interval)
{
	tick->interval = interval;
	tick->last_time = 0;
}

uint32_t ticker_tick(ticker_t *tick) {
	uint32_t msec = millis();
	if ((msec - tick->last_time) >= tick->interval) {
		tick->last_time = msec;
		return msec;
	}
	return 0;
}
