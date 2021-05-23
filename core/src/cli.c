/**
 * Command line parser for MK-52 display scanner
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "lib/serial.h"
#include "lib/serial_cli.h"

static const char version[] = "2021-05-23\n";

// list of supported commands
const char cmd_list[] =
	"\n"
	"reset\n"
	"info\n"
	"print hex|key on|off\n"
;

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
		serial_print("Scan cycle %u.%u msec\n", scan_period / 1000, scan_period % 1000);
		serial_print("Timer period %u usec\n", tim_arr);
		serial_print("Printing of hex scan codes is %s\n", is_on(app_flags & APP_PRINT_HEX_SCAN));
		serial_print("Printing of key scan codes is %s\n", is_on(app_flags & APP_PRINT_KEY_SCAN));
		return CLI_EOK;
	}

	if (str_is(cmd, "print")) {
		uint8_t flag = 0;
		if (str_is(arg, "hex"))
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
	return CLI_ENOTSUP;
}
