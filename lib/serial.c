/**
 * Simple handler for serial terminal (similar to getch) on STM32F10x
 */
#include <stdio.h>
#include <stdarg.h>
#include <stm32f1xx_hal.h>

#include <main.h>
#include <target.h>
#include "serial.h"
#include "ringbuf.h"

/* Escape sequence states */
#define ESC_CHAR    0
#define ESC_BRACKET 1
#define ESC_BRCHAR  2
#define ESC_TILDA   3
#define ESC_CRLF    5

static UART_HandleTypeDef huart;
static USART_TypeDef *uart;

#define UART_RX_BUF_SIZE 32
#define UART_TX_BUF_SIZE 128

static uint8_t rx_buffer[UART_RX_BUF_SIZE];
static uint8_t tx_buffer[UART_TX_BUF_SIZE];

static ring_buf_t rx_rbuf;
static ring_buf_t tx_rbuf;

/**
 * Some of HAL functions defined here for USART3, change for different USART
 */

/* called from HAL_UART_Init */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
	IRQn_Type uart_irq;
	GPIO_InitTypeDef uart_tx;
	GPIO_InitTypeDef uart_rx;
	GPIO_TypeDef *uart_uart;
	uart_tx.Mode = GPIO_MODE_AF_PP;
	uart_tx.Speed = GPIO_SPEED_FREQ_HIGH;

	uart_rx.Mode = GPIO_MODE_INPUT;
	uart_rx.Pull = GPIO_NOPULL;

#if (USART_TO_USE == 1)
		uart_irq = USART1_IRQn;
		uart_uart = GPIOA;
		/** USART1 GPIO Configuration
			PA9  ------> USART1_TX
			PA10 ------> USART1_RX
			Alternative:
			PB6 ------> USART1_TX
			PB7 ------> USART1_RX
		*/
		uart_tx.Pin = GPIO_PIN_9;
		uart_rx.Pin = GPIO_PIN_10;

		/* Peripheral clock enable */
		__HAL_RCC_USART1_CLK_ENABLE();
#elif (USART_TO_USE == 2)
		uart_irq = USART2_IRQn;
		uart_uart = GPIOA;
		/** USART2 GPIO Configuration
			PA2 ------> USART1_TX
			PA3 ------> USART1_RX
		*/
		uart_tx.Pin = GPIO_PIN_2;
		uart_rx.Pin = GPIO_PIN_3;

		/* Peripheral clock enable */
		__HAL_RCC_USART2_CLK_ENABLE();
#elif (USART_TO_USE == 3)
		uart_irq = USART3_IRQn;
		uart_uart = GPIOB;
		/** USART3 GPIO Configuration
			PB10 ------> USART3_TX
			PB11 ------> USART3_RX
		*/
		uart_tx.Pin = GPIO_PIN_10;
		uart_rx.Pin = GPIO_PIN_11;

		/* Peripheral clock enable */
		__HAL_RCC_USART3_CLK_ENABLE();
#else
		#error USART_TO_USE must be in 1 to 3 range!
#endif
	HAL_GPIO_Init(uart_uart, &uart_tx);
	HAL_GPIO_Init(uart_uart, &uart_rx);

	/* USART interrupt Init */
	HAL_NVIC_SetPriority(uart_irq, 0, 0);
	HAL_NVIC_EnableIRQ(uart_irq);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
	IRQn_Type uart_irq;
	uint32_t uart_pins;
	GPIO_TypeDef *uart_uart;
#if (USART_TO_USE == 1)
	/* Peripheral clock disable */
	__HAL_RCC_USART1_CLK_DISABLE();
	uart_uart = GPIOA;
	/** USART1 GPIO Configuration
	 PA9  ------> USART1_TX
	 PA10 ------> USART1_RX
	*/
	uart_pins = GPIO_PIN_9 | GPIO_PIN_10;
	/* USART1 interrupt DeInit */
	uart_irq = USART1_IRQn;
#elif (USART_TO_USE == 2)
	/* Peripheral clock disable */
	__HAL_RCC_USART2_CLK_DISABLE();
	uart_uart = GPIOA;
	/** USART2 GPIO Configuration
	 PA2 ------> USART1_TX
	 PA3 ------> USART1_RX
	 */
	uart_pins = GPIO_PIN_2 | GPIO_PIN_3;

	/* USART2 interrupt DeInit */
	uart_irq = USART2_IRQn;
#else /* (USART_TO_USE == 3) */
	/* Peripheral clock disable */
	__HAL_RCC_USART3_CLK_DISABLE();
	uart_uart = GPIOB;
	/** USART3 GPIO Configuration
	 PB10 ------> USART3_TX
	 PB11 ------> USART3_RX
	*/
	uart_pins = GPIO_PIN_10 | GPIO_PIN_11;

	/* USART3 interrupt DeInit */
	uart_irq = USART3_IRQn;
#endif

	HAL_GPIO_DeInit(uart_uart, uart_pins);
	HAL_NVIC_DisableIRQ(uart_irq);
}

/* Very basic interrupt driven RX/TX for an UART */
#if (USART_TO_USE == 1)
void USART1_IRQHandler(void)
#elif (USART_TO_USE == 2)
void USART2_IRQHandler(void)
#else /* (USART_TO_USE == 3) */
void USART3_IRQHandler(void)
#endif
{
	uint32_t sr = uart->SR;

	// ToDo: use DMA for RX to avoid loosing bytes at high speed
	if (sr & USART_SR_RXNE) {
		uint8_t ch = uart->DR;
		/**
		 * at 115200 'arrow up' generates codes too fast for the interrupt
		 * remove is_full check to cope with the speed and hope that we will not overflow :)
		 */
		/* if (!rbuf_is_full(&rx_rbuf)) */
			rbuf_write(&rx_rbuf, ch);
		return;
	}

	if (sr & USART_SR_TXE) {
		if (rbuf_is_empty(&tx_rbuf))
			uart->CR1 &= ~USART_CR1_TXEIE; /* disable TX interrupt */
		else
			uart->DR = rbuf_read(&tx_rbuf); /* will clear USART_SR_TXE & USART_SR_TC */
	}
}

int serial_init(uint32_t baud)
{
	rbuf_init(&rx_rbuf, rx_buffer, UART_RX_BUF_SIZE);
	rbuf_init(&tx_rbuf, tx_buffer, UART_TX_BUF_SIZE);
#if (USART_TO_USE == 1)
	huart.Instance = USART1;
	uart = USART1;
#elif (USART_TO_USE == 2)
	huart.Instance = USART2;
	uart = USART2;
#else /* (USART_TO_USE == 3) */
	huart.Instance = USART3;
	uart = USART3;
#endif
	huart.Init.BaudRate = baud;
	huart.Init.WordLength = UART_WORDLENGTH_8B;
	huart.Init.StopBits = UART_STOPBITS_1;
	huart.Init.Parity = UART_PARITY_NONE;
	huart.Init.Mode = UART_MODE_TX_RX;
	huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart.Init.OverSampling = UART_OVERSAMPLING_16;

	HAL_UART_Init(&huart);
	/* enable RX interrupt */
	uart->CR1 |= USART_CR1_RXNEIE;
	return 0;
}

static inline uint16_t _serial_getc(void)
{
	uint16_t ch = 0xFF00;
	if (!rbuf_is_empty(&rx_rbuf))
		ch = rbuf_read(&rx_rbuf);
	return ch;
}

int serial_is_sending(void)
{
	return !rbuf_is_empty(&tx_rbuf);
}

uint16_t serial_getc(void)
{
	static uint8_t esc = ESC_CHAR;
	static uint8_t idx = 0;
	uint16_t ch;

	ch = _serial_getc();
	if (ch & 0xFF00)
		return 0;

	/* ESC sequence state machine */
	if (ch == 27) {
		esc = ESC_BRACKET;
		return 0;
	}
	if (esc == ESC_BRACKET) {
		if (ch == '[') {
			esc = ESC_BRCHAR;
			return 0;
		}
	}
	if (esc == ESC_BRCHAR) {
		esc = ESC_CHAR;
		if (ch >= 'A' && ch <= 'D') {
			ch |= EXTRA_KEY;
			return ch;
		}
		if ((ch >= '1') && (ch <= '6')) {
			esc = ESC_TILDA;
			idx = ch - '0';
			return 0;
		}
		return ch;
	}
	if (esc == ESC_TILDA) {
		esc = ESC_CHAR;
		if (ch == '~') {
			ch = EXTRA_KEY | idx;
			return ch;
		}
		return 0;
	}

	/* convert CR to LF */
	if (ch == '\r') {
		esc = ESC_CRLF;
		return '\n';
	}
	/* do not return LF if it is part of CR+LF combination */
	if (ch == '\n') {
		if (esc == ESC_CRLF) {
			esc = ESC_CHAR;
			return 0;
		}
	}
	esc = ESC_CHAR;
	return ch;
}

/* will block if TX buffer is full */
void serial_putc(uint8_t ch)
{
	while (rbuf_is_full(&tx_rbuf));
	rbuf_write(&tx_rbuf, ch);
	/* enable TX interrupt to start transmit */
	uart->CR1 |= USART_CR1_TXEIE;
}

void serial_puts(const char *str)
{
	for(unsigned i = 0; str[i]; i++) {
		if (str[i] == '\n')
			serial_putc('\r');
		serial_putc(str[i]);
	}
}

void serial_print(const char *format, ...)
{
	char buffer[UART_TX_BUF_SIZE];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);

	serial_puts(buffer);

	va_end(args);
}

void serial_putb(uint32_t val, uint8_t len)
{
	if (len > 32)
		len = 32;
	uint32_t mask = 0x00000001 << (len - 1);
	while(mask) {
		serial_putc(!!(val & mask) + '0');
		mask >>= 1;
	}
}

/** print uint8_t in hex format */
void serial_puth(uint8_t val)
{
	uint8_t hex = val >> 4;
	hex += '0';
	if (hex > '9')
		hex += 7;
	serial_putc(hex);
	hex = val & 0x0F;
	hex += '0';
	if (hex > '9')
		hex += 7;
	serial_putc(hex);
}