/**
 * Time, clocks and ticks related stuff.
 *
 * MIT License.
 *
 */

#ifndef TICKER_HEADER_FILE
#define TICKER_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

extern __IO uint32_t uwTick;
static inline uint32_t millis(void) {
	return uwTick;
}

/**
 * init Data Watchpoint & Trace (DWT) for microsecond delays
 * @return system clocks per microsecond or 0 if failed
 */
uint32_t delay_usec_init(void);

extern volatile uint32_t clocks_per_usec;

static inline void delay_usec(uint32_t microseconds) {
	uint32_t ticks_start = DWT->CYCCNT;
	microseconds = microseconds * clocks_per_usec - (clocks_per_usec /2); /* '/2' to compensate for aux code */
	while ((DWT->CYCCNT - ticks_start) < microseconds);
}

static inline void tim_enable(TIM_TypeDef *tim) {
	tim->DIER |= TIM_IT_UPDATE;
	tim->CR1 |= TIM_CR1_CEN;
}

static inline void tim_disable(TIM_TypeDef *tim) {
	tim->DIER &= ~TIM_IT_UPDATE;
	tim->CR1 &= ~TIM_CR1_CEN;
}

/**
 * initialize ARR with the new value and update it by first
 * setting EGR:TIM_EGR_UG flag and
 * resetting SR::TIM_SR_UIF (interrupt pending flag)
 */
static inline void tim_set_arr(TIM_TypeDef *tim, uint32_t arr) {
	tim->ARR = arr;
	tim->EGR |= TIM_EGR_UG; /* latch new ARR value*/
	tim->SR &= ~TIM_SR_UIF; /* reset the interrupt flag set by the previous line */
}

#ifdef __cplusplus
}
#endif
#endif