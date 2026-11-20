#ifndef POTETO_RLE_H
#define POTETO_RLE_H

#include <stddef.h>
#include <stdint.h>

/* Decode a packed frame into native-endian RGB565 pixels.
   Returns the number of pixels written, or 0 on a bad payload. */
size_t poteto_rle_decode(const uint8_t *src, size_t src_len, uint16_t *dst, size_t pixel_count);

#endif
