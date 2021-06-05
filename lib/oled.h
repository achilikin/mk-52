/*******************************************************************************
 * SH1122 based 256x64 16 Grayscale OLED for Elektronika MK-52
 * Frame buffer, font and oled_print() is highly optimized for MK-52 use
 * but, if needed, can be easily modified to be more generic
 *
 * Licence: MIT
 ******************************************************************************/
#ifndef MK52_SH1122_OLED_H
#define MK52_SH1122_OLED_H

#include "spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * OLED common APIs
*******************************************************************************/

#define OLED_WIDTH  256	/** display width in pixels */
#define OLED_HEIGHT 64 	/** display height */
#define OLED_PPB    2	/** pixel per byte */
#define OLED_LINE_SIZE (OLED_WIDTH / OLED_PPB) /* OLED line size in bytes */

#define OLED_FONT_WIDTH       22
#define OLED_FONT_CHAR_HEIGHT 35 /* extra for dot */
#define OLED_FONT_HEIGHT      37 /* 35 + 2 extra for dot */
#define OLED_FONT_DOT_HEIGHT  3 /* dot height */
#define OLED_FONT_SIGN_HEIGHT 3 /* sign height */

#define OLED_START_LINE (OLED_HEIGHT - (OLED_HEIGHT - OLED_FONT_HEIGHT) / 2)

#define OLED_SIGN_LINE  15 /* at which line the sign is located */
#define OLED_SYM_OFFSET 14 /* the first column of the first digit */
#define OLED_DOT_OFFSET 18 /* dot position offset within a symbol */

#define OLED_COLOR_BLACK 0x00
#define OLED_COLOR_DIM   0x01
#define OLED_COLOR_GRAY  0x07
#define OLED_COLOR_WHITE 0x0F

#define OLED_DEFAULT_FONT_COLOR OLED_COLOR_GRAY
#define OLED_DEFAULT_BKG_COLOR  OLED_COLOR_BLACK

enum MK52_SYM {
	SYM_SPACE,
	SYM_MINUS,
	SYM_0,
	SYM_1,
	SYM_2,
	SYM_3,
	SYM_4,
	SYM_5,
	SYM_6,
	SYM_7,
	SYM_8,
	SYM_9,
	SYM_C,
	SYM_E,
	SYM_L,
	SYM_R,	/* actually, symbol looks like Russian letter 'Г' */
	SYM_M1, /* actually, this is '-1' in one position, similar to: '˧' */
	SYM_RF, /* actually, symbol looks like '-7' in one position or reversed 'F': 'ꟻ' */
	SYM_RP,	/* actually, symbol looks like reversed 'P': 'ꟼ' */
	SYM_MAX
};

#define OLED_DIGITS 12 /** number of digit positions supported by MK-21 */

static inline void oled_cs_select(void) {
	OLED_CS_GPIO_Port->BSRR = (uint32_t)OLED_CS_Pin << 16u;
}

static inline void oled_cs_unselect(void) {
	OLED_CS_GPIO_Port->BSRR = OLED_CS_Pin;
}

static inline void oled_dc_cmd(void) {
	OLED_DC_GPIO_Port->BSRR = (uint32_t)OLED_DC_Pin << 16u;
}

static inline void oled_dc_data(void) {
	OLED_DC_GPIO_Port->BSRR = OLED_DC_Pin;
}

static inline void oled_rst_start(void) {
	OLED_RST_GPIO_Port->BSRR = (uint32_t)OLED_RST_Pin << 16u;
}

static inline void oled_rst_stop(void) {
	OLED_RST_GPIO_Port->BSRR = OLED_RST_Pin;
}

extern uint8_t oled_rotated; /* 0 for the default, anything else for 180 rotation */

/* frame buffer, holds only portion of RAM used by digits */
extern uint8_t oled_frame[OLED_LINE_SIZE * OLED_FONT_HEIGHT];

/* used by oled_init(), does not make sense to use separately */
void oled_reset(void);

/* initialize display and fill with the provided color */
void oled_init(uint8_t fill);

/** @param rotated: true to rotate by 180 */
void oled_rotate(bool rotated);

/* send one or two byte commands to SH1122 */
void oled_send_cmd(uint8_t cmd);
void oled_send_cmd_arg(uint8_t cmd, uint8_t arg);

/* send data to SH1122 */
void oled_send_data(uint8_t *data, uint16_t len);

/* frame buffer drawing primitives */
void oled_set_pixel(uint16_t x, uint16_t y, uint8_t color);
void oled_draw_line(uint8_t x, uint8_t y, uint16_t len, uint8_t color);
void oled_draw_row(uint8_t x, uint8_t y, uint8_t hight, uint8_t color);

/**
 * set color to eb used by oled_print
 * @param color: grayscale index to use, 0 (black) to 15 (brightest white)
 */
void oled_set_font_color(uint8_t color);

/**
 * copy a symbols to the frame buffer
 * @param pos: symbol position 0 to OLED_DIGITS, 0: sign, 1: first digit, ..
 * @param sym: high bit: dot is present, lower 7 bits: supported symbol index 0 to 18, or > 18 for number's sign
 *
 * @return 1 if printed, 0 otherwise
 */
uint8_t oled_print(uint8_t x, uint8_t sym);

/* flush frame buffer to OLED RAM */
static inline void oled_flush_frame(void) {
	oled_send_data(oled_frame, sizeof(oled_frame));
	oled_send_cmd_arg(0xB0, 32 * oled_rotated); /* reset row */
}

/* clear oled frame using provided color 0-15 */
void oled_clear_frame(uint8_t fill);

/* clear oled RAM using provided color 0-15 */
void oled_clear_ram(uint8_t fill);

/*******************************************************************************
 * SH1122 commands, mostly not used as all default parameters after reset
 * are good enough, added just in case
*******************************************************************************/

#define SH1122_CMD_NOP 0xE3 /* useful to align commands to 16bit for 16b SPI writes */

#define SH1122_CMD_SET_COL_LOW  0x00
#define SH1122_CMD_SET_COL_HIGH 0x10
#define SH1122_CMD_SET_ROW 		0xB0

#define SH1122_CMD_SET_LINE 	0x40 /* set RAM line as the start of scan */

#define SH1122_CMD_SET_CONTRAST 0x81

#define SH1122_CMD_SET_DIR_NORMAL  0xA0
#define SH1122_CMD_SET_DIR_REVERSE 0xA1

#define SH1122_CMD_SET_DISPLAY_ON 	  0xA5 /** set entire display ON regardles of data in the RAM */
#define SH1122_CMD_SET_DISPLAY_NORMAL 0xA4 /** display RAM content */

#define SH1122_CMD_SET_DISPLAY_REVERSE    0xA7
#define SH1122_CMD_SET_DISPLAY_NOTREVERSE 0xA6

#define SH1122_CMD_SET_MULT_RATION 0xA8

#define SH1122_CMD_SET_DC_DC 0xAD
	#define SH1122_DC_DC_EXTERNAL  0x00 /* sleep mode if combined with with OLED OFF */
	#define SH1122_DC_DC_INTERNAL  0x01
	#define SH1122_DC_DC_FREQ_06SF 0x00 /* POR */
	#define SH1122_DC_DC_FREQ_07SF 0x02
	#define SH1122_DC_DC_FREQ_08SF 0x04
	#define SH1122_DC_DC_FREQ_09SF 0x06
	#define SH1122_DC_DC_FREQ_10SF 0x08
	#define SH1122_DC_DC_FREQ_11SF 0x0A
	#define SH1122_DC_DC_FREQ_12SF 0x0C
	#define SH1122_DC_DC_FREQ_13SF 0x0E

#define SH1122_CMD_SET_OLED_ON  0xAF
#define SH1122_CMD_SET_OLED_OFF 0xAE /* power saving mode */

#define SH1122_CMD_SET_ROTATION_ON  0xC8
#define SH1122_CMD_SET_ROTATION_OFF 0xC0

#define SH1122_CMD_SET_OFFSET 0xD3

#define SH1122_CMD_SET_OSC_MODE 0xD5
	#define SH1122_OSC_MODE_RATION_MASK 0x0F /** 0 invalid, 2 DCLK POR */
	#define SH1122_OSC_MODE_FREQ_MASK 0x0F	 /** 0 invalid, 2 DCLK POR */

#define SH1122_CMD_SET_CHARGE_PERIOD 0xD9
	#define SH1122_PRECHARGE_MASK 0x0F
	#define SH1122_DISCHARGE_MASK 0x0F

#define SH1122_CMD_SET_VCOM_LEVEL 0xDB

#define SH1122_CMD_SET_VSEGM_LEVEL 0xDC

#define SH1122_CMD_SET_DISCHARGE_LEVEL 0x30
	#define SH1122_DISCHARGE_LEVEL_MASK 0x0F

/** send column lower and higher addresses */
static inline void sh1122_set_column(uint8_t col) {
	oled_send_cmd_arg(SH1122_CMD_SET_COL_LOW | (col & 0x0F), SH1122_CMD_SET_COL_HIGH | ((col >> 4) & 0x07));
}

static inline void sh1122_set_row(uint8_t row) {
	oled_send_cmd_arg(SH1122_CMD_SET_ROW, row & 0x3F);
}

/** send display start line */
static inline void sh1122_set_start_line(uint8_t row) {
	oled_send_cmd(SH1122_CMD_SET_LINE | (row & 0x3F));
}

static inline void sh1122_set_contrast(uint8_t contrast) {
	oled_send_cmd_arg(SH1122_CMD_SET_CONTRAST, contrast);
}

static inline void sh1122_set_remap(uint8_t adc) {
	if ((adc == SH1122_CMD_SET_DIR_NORMAL) || (adc == SH1122_CMD_SET_DIR_REVERSE))
		oled_send_cmd(adc);
}

static inline void sh1122_set_display_on(bool on) {
	oled_send_cmd(on ? SH1122_CMD_SET_DISPLAY_ON : SH1122_CMD_SET_DISPLAY_NORMAL);
}

static inline void sh1122_set_display_reverse(bool on) {
	oled_send_cmd(on ? SH1122_CMD_SET_DISPLAY_REVERSE : SH1122_CMD_SET_DISPLAY_NOTREVERSE);
}

/** ration 0x00 to 0x3F (0x3F POR) */
static inline void sh1122_set_multiplex_ration(uint8_t ration) {
	oled_send_cmd_arg(SH1122_CMD_SET_MULT_RATION, ration & 0x3F);
}

static inline void sh1122_set_dc_dc(uint8_t dc_dc, uint8_t freq) {
	oled_send_cmd_arg(SH1122_CMD_SET_DC_DC, (dc_dc & SH1122_DC_DC_INTERNAL) | (freq & SH1122_DC_DC_FREQ_13SF));
}

static inline void sh1122_set_oled_on(bool on) {
	oled_send_cmd(on ? SH1122_CMD_SET_OLED_ON : SH1122_CMD_SET_OLED_OFF);
}

/** flip display vertically */
static inline void sh1122_set_flip(bool flip) {
	oled_send_cmd(flip ? SH1122_CMD_SET_ROTATION_ON : SH1122_CMD_SET_ROTATION_OFF);
}

/** offset 0 (POR) to 0x3F */
static inline void sh1122_set_offset(uint8_t offset) {
	oled_send_cmd_arg(SH1122_CMD_SET_OFFSET, offset & 0x3F);
}

/** DCLK ration 0 (POR) to 0x0F, oscillator freq adjustments 0 to 0x0F (5 == 100% is POR, -25% to +50%) */
static inline void sh1122_set_osc_mode(uint8_t ration, uint8_t freq) {
	oled_send_cmd_arg(SH1122_CMD_SET_OSC_MODE, (ration & SH1122_OSC_MODE_RATION_MASK) | ((freq & SH1122_OSC_MODE_FREQ_MASK) << 4));
}

static inline void sh1122_set_charge_period(uint8_t precharge, uint8_t discharge) {
	precharge &= SH1122_PRECHARGE_MASK;
	discharge &= SH1122_DISCHARGE_MASK;
	if (!precharge || !discharge)
		return;
	oled_send_cmd_arg(SH1122_CMD_SET_CHARGE_PERIOD, precharge | (discharge << 4));
}

/** level = 0x35 POR */
static inline void sh1122_set_vcom_level(uint8_t level) {
	oled_send_cmd_arg(SH1122_CMD_SET_VCOM_LEVEL, level);
}

/** level = 0x35 POR */
static inline void sh1122_set_vsegm_level(uint8_t level) {
	oled_send_cmd_arg(SH1122_CMD_SET_VSEGM_LEVEL, level);
}

/** 0 POR */
static inline void sh1122_set_discharge_level(uint8_t level) {
	if (level <= SH1122_DISCHARGE_LEVEL_MASK)
		oled_send_cmd(SH1122_CMD_SET_DISCHARGE_LEVEL | level);
}

#ifdef __cplusplus
}
#endif
#endif