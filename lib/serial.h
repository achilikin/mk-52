/**
 * Simple handler for serial terminal (similar to getch) on STM32F10x
 */

#ifndef SERIAL_CHAR_H
#define SERIAL_CHAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** non-character flag */
#define EXTRA_KEY   0x0100

/** support for arrow keys for very simple one command deep history */
#define ARROW_UP    0x0141
#define ARROW_DOWN  0x0142
#define ARROW_RIGHT 0x0143
#define ARROW_LEFT  0x0144

#define KEY_HOME    0x0101
#define KEY_INS	    0x0102
#define KEY_DEL	    0x0103
#define KEY_END	    0x0104
#define KEY_PGUP    0x0105
#define KEY_PGDN    0x0106

#define	UART_BR_2400	2400
#define UART_BR_4800	4800
#define UART_BR_9600 	9600
#define UART_BR_19200 	19200
#define UART_BR_38400 	38400
#define UART_BR_57600 	57600
#define UART_BR_115200 	115200


int serial_init(uint32_t baud);
uint16_t serial_getc(void);
void serial_putc(uint8_t ch);
void serial_puts(const char *str);
void serial_print(const char *format, ...);
void serial_putb(uint32_t val, uint8_t len); /** print val in binary format */
void serial_puth(uint8_t val);				 /** print uint8_t in hex format */

int serial_is_sending(void);

#ifdef __cplusplus
}
#endif

#endif
