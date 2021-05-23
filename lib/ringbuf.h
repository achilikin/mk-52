/**
 * ringbuf.h
 *
 *  Created on: 10 Jun 2018
 *      Author: maker
 *     License: MIT
 */

#ifndef GENERALIO_RINGBUF_H_
#define GENERALIO_RINGBUF_H_

#include <stdint.h>

/* simple ring buffer implementation for UART RX/TX buffers */
typedef struct ring_buf_s {
	uint32_t mask;
    volatile uint32_t head; /** where to write to */
    volatile uint32_t tail; /** where to read from */
    uint8_t  *data;
} ring_buf_t;

/* size MUST be power of 2 */
static inline void rbuf_init(ring_buf_t *rbuf, uint8_t *buf, uint32_t size) {
	rbuf->mask = size - 1;
	rbuf->head = rbuf->tail = 0;
	rbuf->data = buf;
}

static inline void rbuf_reset(ring_buf_t *rbuf) {
	rbuf->head = rbuf->tail = 0;
}

static inline void rbuf_write(ring_buf_t *rbuf, uint8_t data) {
	rbuf->data[rbuf->head] = data;
	rbuf->head = (rbuf->head + 1) & rbuf->mask;
}

static inline uint8_t rbuf_read(ring_buf_t *rbuf) {
	uint8_t data = rbuf->data[rbuf->tail];
	rbuf->tail = (rbuf->tail + 1) & rbuf->mask;
	return data;
}

static inline uint16_t rbuf_size(ring_buf_t *rbuf) {
	return (rbuf->head - rbuf->tail) & rbuf->mask;
}

static inline int rbuf_is_empty(ring_buf_t *rbuf) {
	return rbuf->head == rbuf->tail;
}

static inline int rbuf_is_full(ring_buf_t *rbuf) {
	return (rbuf->head == ((rbuf->tail + 1) & rbuf->mask));
}

#endif /* GENERALIO_RINGBUF_H_ */
