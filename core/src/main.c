/**
 * MIT License
 */
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

#include "lib/ringbuf.h"
#include "lib/serial.h"
#include "lib/serial_cli.h"
#include "lib/oled.h"

#define ENABLE_DEBUG_PRINT   1 /* by default print scan results to the serial port */
#define DEBUG_VIRTUAL_DIGITS 0 /* print digits 13 & 14 */
#define SCAN_START_DELAY     2 /* delay in mks for lines to stabilize */

#define OLED_OUTPUT_ENABLED   1 /* use OLED for output */
#define OLED_DEMO_DIGITS_FONT 1 /* OLED demo output */

#if OLED_OUTPUT_ENABLED
#undef OLED_DEMO_DIGITS_FONT
#define OLED_DEMO_DIGITS_FONT 0 /* disable demo if OLED is used for output */
#endif

#define OLED_DIGITS_PLACEHOLDERS_COLOR 0x00 /* draw OLED digits placeholders */

#define NUM_DIGITS OLED_DIGITS 	/* 12 real digit positions */
#define NUM_VIRT   2		  	/* 2 virtual digit positions */
#define NUM_SCAN_POS (NUM_DIGITS + NUM_VIRT) /* 12 real digit positions and 2 virtual */
#define PROGRAM_RUNNING 0x6F /* '9' at virtual position 0 as a running program flag */

typedef struct scan_s {
	union {
		uint8_t scan_buf[NUM_SCAN_POS]; /** one line of scanned segments codes */
		struct {
			uint8_t digits[NUM_DIGITS]; /** digit scans */
			uint8_t key[NUM_VIRT];      /** key scans */
		};
	};
	uint16_t scan_time; /** number of scan intervals before detecting this line */
} scan_t;

#define NUM_LINES 16
scan_t vfd[NUM_LINES]; /** ring buffer for scanned digits */

#define LINE_TYPE_NORMAL 0x80
#define LINE_TYPE_IDLE   0x40
#define LINE_TYPE_EXEC   0x20

/**
 * ring buffer for event generated by VFD scanner per scanned line
 * high nibble contains LINE_TYPE_*
 * low nibble contains line number for vfd[]
 */
ring_buf_t evbuf;
uint8_t vfd_events[32];

/**
 * map table from a scan code to the sequencial index of supported symbols
 * index is 1 based, 0 indicates an unknow symbol
 * high bit of a scan code used by SEG_DOT, and we clear before using this table,
 * so we need 0x7F entries max
 */
static const uint8_t seg_map[0x80] = {
	[0x00] = SYM_SPACE + 1,
	[0x40] = SYM_MINUS + 1,
	[0x3F] = SYM_0 + 1,
	[0x06] = SYM_1 + 1,
	[0x5B] = SYM_2 + 1,
	[0x4F] = SYM_3 + 1,
	[0x66] = SYM_4 + 1,
	[0x6D] = SYM_5 + 1,
	[0x7D] = SYM_6 + 1,
	[0x07] = SYM_7 + 1,
	[0x7F] = SYM_8 + 1,
	[0x6F] = SYM_9 + 1,
	[0x39] = SYM_C + 1,
	[0x79] = SYM_E + 1,
	[0x38] = SYM_L + 1,
	[0x31] = SYM_R + 1,
	[0x46] = SYM_M1 + 1,
	[0x47] = SYM_RF + 1,
	[0x67] = SYM_RP + 1,
};

/* mapping from an index to printable character */
static const uint8_t seg_sym[] = {
	'?', ' ', '-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'C', 'E', 'L', 'R', '{', 'F', 'P' };

/** application flags controlled by CLI */
#if ENABLE_DEBUG_PRINT
uint8_t app_flags = APP_PRINT_ENABLE;
#else
uint8_t app_flags = 0;
#endif

static ticker_t tick10ms;

int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	/* Configure the system clock */
	SystemClock_Config();
	/* Initialize all configured peripherals */
	/* initialize DWT for usec resolution delays, will set clocks_per_usec */
	delay_usec_init();
	MX_GPIO_Init();
	/* use HAL init procedure, but then use registers directly for better speed */
	MX_SPI2_Init();
	spi->CR1 |= SPI_CR1_SPE; /* enable SPI */

	/* use standard HAL init for TIM4, but replace HAL interrupt handler with simple one */
	MX_TIM4_Init();
	/* clear Update Interrupt Flag, to get the first interrupt at needed interval */
	__HAL_TIM_CLEAR_FLAG(&htim4, TIM_SR_UIF);

	rbuf_init(&evbuf, vfd_events, 32);
	/* do not use MX_USART3_UART_Init(); --> replaced with serial_init() */
	serial_init(UART_BR_38400);

	if (clocks_per_usec == 0) /* would never happen */
		serial_puts("DWT init failed!\n");

	cli("help", NULL);
	cli_init();

	/**
	 * STM32 + OLED all segments set to 0x00 = 67 mA
	 * STM32 + OLED all segments set to 0x0F = 207 mA
	 * STM32 + OLED set to '-8.8.8.8.8.8.8.8.8.8.8." = 85mA
	 */
	oled_init(OLED_DEFAULT_BKG_COLOR);
	sh1122_set_start_line(OLED_START_LINE);

	ticker_init(&tick10ms, 10);

#if OLED_DIGITS_PLACEHOLDERS_COLOR
	oled_draw_line(0, 0, OLED_WIDTH, OLED_DIGITS_PLACEHOLDERS_COLOR); /* top */
	oled_draw_line(0, OLED_FONT_HEIGHT - 1, OLED_WIDTH, OLED_DIGITS_PLACEHOLDERS_COLOR); /* bottom */
	oled_draw_row(0, 1, OLED_FONT_HEIGHT - 2, OLED_DIGITS_PLACEHOLDERS_COLOR); /* sign placeholder */
	oled_draw_row(9, 1, OLED_FONT_HEIGHT - 2, OLED_DIGITS_PLACEHOLDERS_COLOR);
	for (uint16_t row = OLED_SYM_OFFSET - 1; row < OLED_WIDTH; row += OLED_FONT_WIDTH) /* symbols placeholders */
		oled_draw_row(row, 1, OLED_FONT_HEIGHT - 2, OLED_DIGITS_PLACEHOLDERS_COLOR);
	oled_flush_frame();
#endif

#if OLED_DEMO_DIGITS_FONT
	static ticker_t tick1s;
	static uint8_t demo = 0;
	static const uint8_t disp[] = {
		SYM_1, SYM_2, SYM_3, SYM_4, SYM_5, SYM_6, SYM_7, SYM_8, SYM_MINUS, SYM_9, SYM_0,
		SYM_C, SYM_E, SYM_L, SYM_R, SYM_M1, SYM_RF, SYM_RP, SYM_MINUS, SYM_SPACE, SYM_E, SYM_E};
	ticker_init(&tick1s, 1000);
#endif

	bool blank = false; /* true if previous line was blank */
	/* the main  loop */
	while (true) {
		cli_interact(cli, NULL);

#if OLED_DEMO_DIGITS_FONT
		if (ticker_tick(&tick1s)) {
			uint8_t dot = SEG_DOT * !!(demo & 0x01);
			uint8_t	start = (sizeof(disp) / 2) * !!(demo & 0x02);
			/**
			 * total ~8.8 ms for printing all digits and flusing the frame,
			 * or ~26.7 using HAL SPI
			 */
			oled_print(0, dot ? SYM_MINUS : SYM_SPACE); /* special case, print the sign simbol, ~3.3 usec*/
			dbg_low();
			for (uint8_t pos = 1; pos < OLED_DIGITS; pos++) {/* ~435 usec */
				oled_print(pos, dot + disp[pos - 1 + start]); /* ~38 usec*/
			}
			dbg_high();
			oled_flush_frame(); /* ~8.5 ms with SPI_BAUDRATEPRESCALER_8, ~4.2 ms with SPI_BAUDRATEPRESCALER_4 */
			demo++;
		}
#endif
#if OLED_OUTPUT_ENABLED
		if (ticker_tick(&tick10ms))
			vfd_wd++;

		if (vfd_wd == (VFD_WD_TIMEOUT / 10)) {
			oled_clear_frame(0);
			oled_flush_frame();
			sh1122_set_oled_on(false);
		}
#endif
		/**
		 * display scanner will send an event
		 * bits 7..6 - scan line type: normal or detected program execution
		 * bits 5..0 - scan buffer line index to read
		 * if all bits are 0 then all digits are off
		 */
		if (rbuf_size(&evbuf)) {
			uint8_t i, line;
			uint8_t line_type = rbuf_read(&evbuf);
			if (line_type & LINE_TYPE_NORMAL) {
				line = line_type & ~(LINE_TYPE_NORMAL | LINE_TYPE_IDLE | LINE_TYPE_EXEC);
#if OLED_OUTPUT_ENABLED
				/* if a program is running then set color to dimmest one */
				oled_set_font_color((line_type & LINE_TYPE_EXEC) ? OLED_COLOR_DIM : OLED_DEFAULT_FONT_COLOR);
				/* print to oled frame buffer */
				for (i = 0; i < NUM_DIGITS; i++) {
					uint8_t scan = vfd[line].scan_buf[i];
					if (i == 0) { /* only G segment is valid for the sign */
						oled_print(0, (scan & SEG_G) ? SYM_MINUS : SYM_SPACE);
						continue;
					} else {
						uint8_t sym = seg_map[scan & 0x7F]; /* 0: invalid, else symbol index + 1 */
						if (sym <= 1) {
							oled_print(i, (scan & SEG_DOT) | SYM_SPACE);
							continue;
						}
						oled_print(i, (scan & SEG_DOT) | (sym - 1));
					}
				}
				oled_flush_frame();
#endif
				if (blank) {
#if OLED_OUTPUT_ENABLED
					sh1122_set_oled_on(true);
#endif
					if (app_flags & APP_PRINT_ENABLE) {
						uint32_t cycle_time = (vfd_curr_arr + 1) * NUM_SCAN_POS;
						serial_print(" %u cycles (%u,%u ms)\n", vfd[line].scan_time,
									 (vfd[line].scan_time * cycle_time) / 1000,
									 (vfd[line].scan_time * cycle_time) % 1000);
					}
				}
				if (app_flags & APP_PRINT_ENABLE) {
					if (app_flags & APP_PRINT_HEX_SCAN) {
						for (i = 0; i < NUM_SCAN_POS; i++)
							serial_print("%02X ", vfd[line].scan_buf[i]);
					}
					serial_putc('\'');
					for (i = 0; i < NUM_SCAN_POS; i++) {
						uint8_t scan = vfd[line].scan_buf[i];
						uint8_t sym = seg_map[scan & 0x7F];
						if (sym) {
							sym = seg_sym[sym];
							serial_putc(sym);
							if (scan & SEG_DOT)
								serial_putc('.');
							if (i == (NUM_DIGITS - 1))
								serial_puts("' ["); /* print virtual digits in brackets */
						} else
							serial_print("(%02X)", scan);
					}
					serial_print("]");
					if (line_type & LINE_TYPE_EXEC)
						serial_puts(" RUNNIG");
					serial_putc('\n');
				}
				blank = false;
			} else if (line_type & LINE_TYPE_IDLE) {
#if OLED_OUTPUT_ENABLED
				/**
				 * if a program is running then do not turn oled off,
				 * instead, clear it and flush to animate the execution
				 */
				if (line_type & LINE_TYPE_EXEC) {
					oled_clear_frame(OLED_COLOR_BLACK);
					oled_flush_frame();
				} else
					sh1122_set_oled_on(false);
#endif
				if (app_flags & APP_PRINT_ENABLE)
					serial_puts("'             '");
				blank = true;
			}
		}
	}
}

/**
 * Display scanner is implemented two interrupts:
 * 1. Interrupt on the rising edge of the pin wired to digit 8 grid control signal
 * 2. Timer 4 interupt
 *
 * First interrupt reads segments of the digit 8 and then start TIM4 counter.
 * TIM4 interrupts 13 times and scans corresponding digit segments.
 * The last interrupt of this cycle will check if any changes detected and
 * will send notification event to the main loop
 */
uint32_t scan_ts; 				/* scan start timestamp */
volatile uint32_t vfd_scan_period; 	/* interval between scan pin interrupts, in usec */
volatile uint32_t vfd_curr_arr; /* timer auto reload register for current scan period */
volatile uint32_t vfd_tim_arr;	/* timer auto reload register modified by timer */
volatile uint32_t vfd_wd;		/* watchdog timer, scan interrupt resets to 0 */
static scan_t   raw;	   		/* buffer to store single scan data */
static uint8_t  digit_idx; 		/* index of a digit being scanned, 0 - scan pin interrupt */
static uint8_t  scan_line; 		/* index of the scan buffer entry */
static uint16_t raw_new;   		/* mask of values changed from the last scan */
static uint16_t raw_valid; 		/* mask of digits with at least one segment on */

/* mapping to convert our scanning indexes to digits' indexes */
static const uint8_t digits_map[NUM_SCAN_POS] = {8, 7, 6, 5, 4, 3, 2, 1, 0, 11, 10, 9, 12, 13};

static inline void read_segments(uint8_t idx) {
	uint16_t reg = SEG_GPIO_Port->IDR;
	uint16_t seg = reg & SEG_PINS;
	if (seg)
		raw_valid |= 1 << idx;
	if (raw.scan_buf[idx] != seg)
		raw_new |= 1 << idx;
	raw.scan_buf[idx] = seg;
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	led_on(); /* pulse for oscilloscope for execution teracking */

	vfd_wd = 0; /* reset watchdog timer */
	/* the first call of the interrupt, store the current system clock counter */
	if (scan_ts == 0) {
		scan_ts = DWT->CYCCNT;
		goto exit;
	}

	vfd_scan_period = DWT->CYCCNT - scan_ts;
	scan_ts = DWT->CYCCNT;
	vfd_scan_period /= clocks_per_usec;
	if (vfd_scan_period < 1000) /* ignore any very short intervals: MK52 is starting up */
		goto exit;
	/* re-calculate and update scanning timer period */
	vfd_curr_arr = vfd_tim_arr = vfd_scan_period / NUM_SCAN_POS;
	tim_set_arr(TIM4, vfd_tim_arr + 2); /* 2 usec extra delay for the first tim interrupt */

	delay_usec(SCAN_START_DELAY); /* small delay for segments' signals to stabilize */
	/* reset counter for a new scan cycle */
	digit_idx = 0;
	raw_new = raw_valid = 0;
	read_segments(digits_map[digit_idx++]);
	tim_enable(TIM4); /* start our scanning timer */
exit:
	led_off();
	/* ~5.4 us if SCAN_START_DELAY is 2us */
}

/**
 * use our own timer IRQ handler as we do not need all extra HAL stuff
 * for our simple timer configuration
 */
void TIM4_IRQHandler(void)
{
	static uint8_t is_running; /** true if program execution in progress */
	dbg_low();
	read_segments(digits_map[digit_idx++]);

	if (digit_idx == NUM_SCAN_POS) { /* last scan interrupt */
		tim_disable(TIM4);

		if (!(app_flags & APP_PRINT_KEY_SCAN)) {
			/* ignore virtual digits to avoid false positive events */
			raw_new &= 0x0FFF;
			raw_valid &= 0x0FFF;
		}

		/* at list one real digit had changed */
		if (raw_new && raw_valid) {
			is_running = (raw.key[0] == PROGRAM_RUNNING) ? LINE_TYPE_EXEC : 0;
			raw.scan_buf[0] &= SEG_G; /* only '-' is valid for the first position */
			memcpy(&vfd[scan_line], &raw, sizeof(scan_t));
			scan_line |= LINE_TYPE_NORMAL | is_running;
			rbuf_write(&evbuf, scan_line);
			scan_line = (scan_line + 1) & (NUM_LINES - 1);
			raw.scan_time = 0;
		} else if (!raw_valid) { /* all digits are blank */
			if (!raw.scan_time)	 /* first invalid scan */
				rbuf_write(&evbuf, LINE_TYPE_IDLE | is_running);
			raw.scan_time += 1;
		}
	}
	/**
	 * a small compensation for IRQ handler code execution
	 * '0x03' value had been handpicked using an oscilloscope
	 * */
	if ((digit_idx & 0x03) == 0x03)
		vfd_tim_arr -= 2;
	tim_set_arr(TIM4, vfd_tim_arr);

	/** tim_set_arr() resets interrupt flag for us,
	 * no need to reset it here
	TIM4->SR &= ~TIM_SR_UIF;
	*/
	dbg_high();
	/* ~1.0us for normal scan */
	/* ~2.5us for the last scan */
}
