/**
 * Simple command line parser
 */
#include <stdlib.h>
#include <stdio.h>

#include "serial.h"
#include "serial_cli.h"

const char *is_on(uint8_t val)
{
	if (val) return "ON ";
	return "OFF";
}

char *get_arg(char *str)
{
	char *arg;

	for(arg = str; *arg && *arg != ' '; arg++);

	if (*arg == ' ') {
		*arg = '\0';
		arg++;
	}

	return arg;
}

int8_t str_is(const char *str, const char *cmd)
{
	while (*str > ' ') {
		if (*str != *cmd)
			return 0;
		str++;
		cmd++;
	}
	if (*cmd == 0)
		return 1;
	return 0;
}

uint16_t argtou(char *arg, char **end)
{
	uint8_t digit;
	uint16_t val = 0;

	if ((arg[0] == '0') && (arg[1] == 'x'))
		arg++;
	if (*arg == 'x') {
		arg++;
		while (*arg > ' ') {
			digit = *arg;
			if ((digit >= 'A') && (digit <= 'F'))
				digit = digit - 'A' + 10;
			else if ((digit >= 'a') && (digit <= 'f'))
				digit = digit - 'a' + 10;
			else if ((digit >= '0') && (digit <= '9'))
				digit = digit - '0';
			else
				goto retval;
			val = (val << 4) | digit;
			arg++;
		}
	} else {
		while (*arg > ' ') {
			digit = *arg;
			if ((digit >= '0') && (digit <= '9'))
				digit = digit - '0';
			else
				break;
			val = val * 10 + digit;
			arg++;
		}
	}

retval:
	while (*arg && *arg <= ' ')
		arg++;
	*end = arg;
	return val;
}


static uint8_t  cursor;
static char cmd[CMD_LEN + 1];
static char hist[CMD_LEN + 1];

void cli_init(void)
{
	cursor = 0;

	for(uint8_t i = 0; i <= CMD_LEN; i++) {
		cmd[i] = '\0';
		hist[i] = '\0';
	}
	serial_puts("> ");
}

int8_t cli_interact(cli_processor *process, void *ptr)
{
	uint16_t ch;

	while((ch = serial_getc()) != 0) {
		if (ch & EXTRA_KEY) {
			if (ch == ARROW_UP && (cursor == 0)) {
				/* execute the last successful command */
				for(cursor = 0; ; cursor++) {
					cmd[cursor] = hist[cursor];
					if (cmd[cursor] == '\0')
						break;
				}
				serial_puts(cmd);
			}
			return 1;
		}

		if (ch == '\n') {
			serial_putc(ch);
			if (*cmd) {
				int8_t ret = process(cmd, ptr);
				memcpy(hist, cmd, sizeof(cmd));
				if (ret == CLI_EARG)
					serial_puts("Invalid argument\n");
				else if (ret == CLI_ENOTSUP)
					serial_puts("Unknown command\n");
				else if (ret == CLI_ENODEV)
					serial_puts("Device error\n");
			}
			for(uint8_t i = 0; i < cursor; i++)
				cmd[i] = '\0';
			cursor = 0;
			serial_putc('>');
			serial_putc(' ');
			return 1;
		}

		/* backspace processing */
		if (ch == '\b') {
			if (cursor) {
				cursor--;
				cmd[cursor] = '\0';
				serial_putc('\b');
				serial_putc(' ');
				serial_putc('\b');
			}
		}

		/* skip control or damaged bytes */
		if (ch < ' ')
			return 0;

		/* echo */
		serial_putc((uint8_t)ch);

		cmd[cursor++] = (uint8_t)ch;
		cursor &= CMD_LEN;
		/* clean up in case of overflow (command too long) */
		if (!cursor) {
			for(uint8_t i = 0; i <= CMD_LEN; i++)
				cmd[i] = '\0';
		}

		return 1;
	}

	return 0;
}
