#ifndef POTETO_KEY3_I2S_H
#define POTETO_KEY3_I2S_H

#include <stddef.h>
#include <stdint.h>

/* RX only. Does not start the speaker and does not call bsp_i2c_init. */
int poteto_i2s_open(void);

/* Copies whatever the microphone DMA has ready. n is 0 when none is waiting. */
void poteto_i2s_read(uint8_t *dst, size_t cap, size_t *n);

#endif
