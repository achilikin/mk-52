/**
 * Command line parser for MK-52 display scanner
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "lib/oled.h"
#include "lib/serial.h"
#include "lib/serial_cli.h"

static const char version[] = "2021-06-26\n";

// list of supported commands
const char cmd_list[] =
	"\n"
	"reset\n"
	"info\n"
	"print scan on|off\n" /* enable scan output to serial port */
	"print hex on|off\n"  /* enable raw scan in hex */
	"print key on|off\n"  /* enable keyboard scan codes */
	"oled on|off\n"
	"oled reset\n"
	"oled clear [$color]\n" 	/* color 0x00 to 0x0F */
	"oled font $color_value\n" 	/* 0: off, 15: max */
	"oled print $str\n" 		/* print a string of valid symbols */
	"oled line $start_line\n"  	/* 0 to 63 */
	"oled rotate on|off\n"
;

/* mapping from a letter to a MK52 symbol */
static const uint8_t let_sym[] = {
	' ', '-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'C', 'E', 'L', 'R', '{', 'F', 'P'};

static uint8_t get_oled_sym(uint8_t letter)
{
	for (uint8_t i = 0; i < sizeof(let_sym); i++) {
		if (letter == let_sym[i])
			return i;
	}
	return SYM_SPACE;
}

int8_t cli(char *buf, void *ptr)
{
	char cmd[CMD_LEN + 1];
	memcpy(cmd, buf, sizeof(cmd));
	char *arg = get_arg(cmd);

	if (str_is(cmd, "help")) {
		uint32_t *uid = ((uint32_t *)UID_BASE);
		serial_puts("UID: ");
		for(unsigned i = 0; i < 3; i++) {
			serial_print("%08lX", uid[i]);
			if (i < 2)
				serial_puts("-");
		}
		serial_print("\nRunning at: %u", SystemCoreClock);
		serial_print("\nVersion: %s", version);
		serial_print("Commands:");
		for (const char *ptr = cmd_list; *ptr; ptr++) {
			if (*ptr == '\n' && ptr[1]) {
				serial_putc('\n');
				serial_puts("    ");
				continue;
			}
			serial_putc(*ptr);
		}
		return 0;
	}

	/* SW reset */
	if (str_is(cmd, "reset")) {
		if (*arg != '\0')
			serial_puts("ignoring arguments\n");
		serial_puts("resetting...\n");
		while (serial_is_sending());
		HAL_Delay(10);
		NVIC_SystemReset();
	}

	if (str_is(cmd, "info")) {
		serial_print("DWT counter is running at %u clocks per usec\n", clocks_per_usec);
		serial_print("Scan cycle %u.%u msec\n", vfd_scan_period / 1000, vfd_scan_period % 1000);
		serial_print("Timer period %u usec\n", vfd_curr_arr);
		serial_print("Printing of hex scan codes is %s\n", is_on(app_flags & APP_PRINT_HEX_SCAN));
		serial_print("Printing of key scan codes is %s\n", is_on(app_flags & APP_PRINT_KEY_SCAN));
		return CLI_EOK;
	}

	if (str_is(cmd, "print")) {
		uint8_t flag = 0;
		if (str_is(arg, "scan"))
			flag = APP_PRINT_ENABLE;
		else if (str_is(arg, "hex"))
			flag = APP_PRINT_HEX_SCAN;
		else if (str_is(arg, "key"))
			flag = APP_PRINT_KEY_SCAN;
		else
			return CLI_EARG;
		arg = get_arg(arg);
		if (str_is(arg, "on"))
			app_flags |= flag;
		else if (str_is(arg, "off"))
			app_flags &= ~flag;
		else
			return CLI_EARG;
		return CLI_EOK;
	}

	if (str_is(cmd, "oled")) {
		if (str_is(arg, "font")) {
			arg = get_arg(arg);
			uint16_t color = argtou(arg, &arg);
			if (color > 0x0F)
				return CLI_EARG;
			oled_set_font_color(color); /* will be used at the next frame flush */
			return CLI_EOK;
		}

		if (str_is(arg, "line")) {
			arg = get_arg(arg);
			uint16_t line = argtou(arg, &arg);
			if (line > 0x3F)
				return CLI_EARG;
			sh1122_set_start_line(line);
			return CLI_EOK;
		}

		if (str_is(arg, "rotate")) {
			arg = get_arg(arg);
			if (str_is(arg, "on"))
				oled_rotate(true);
			else if (str_is(arg, "off"))
				oled_rotate(false);
			else
				return CLI_EARG;
			oled_flush_frame();
			return CLI_EOK;
		}

		if (str_is(arg, "reset")) {
			oled_init(OLED_DEFAULT_BKG_COLOR);
			return CLI_EOK;
		}

		if (str_is(arg, "clear")) {
			uint8_t fill = OLED_DEFAULT_BKG_COLOR;
			arg = get_arg(arg);
			if (*arg)
				fill = argtou(arg, &arg);
			if (fill > 0x0F)
				return CLI_EARG;
			oled_clear_ram(fill);
			return CLI_EOK;
		}

		if (str_is(arg, "print")) {
			uint8_t i = 0, pos  = 0;
			arg = get_arg(arg);
			if (arg[0] == '-') {
				i++;
				oled_print(pos++, SYM_MINUS);
			} else
				oled_print(pos++, SYM_SPACE);
			for (; arg[i] > ' '; i++) {
				uint8_t sym = get_oled_sym(arg[i]);
				if (arg[i+1] == '.') {
					i++;
					sym |= SEG_DOT;
				}
				oled_print(pos++, sym);
			}
			for (; pos < OLED_DIGITS; pos++)
				oled_print(pos, SYM_SPACE);
			oled_flush_frame();
			return CLI_EOK;
		}

		if (str_is(arg, "on"))
			sh1122_set_oled_on(true);
		else if (str_is(arg, "off"))
			sh1122_set_oled_on(false);
		else
			return CLI_EARG;
		return CLI_EOK;
	}
	return CLI_ENOTSUP;
}
