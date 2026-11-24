#ifndef POTETO_PBF1_H
#define POTETO_PBF1_H

#include <stddef.h>
#include <stdint.h>

/* PBF1 atlas. Offsets were read from firmware/font/bubble_font.bin:
   height at byte 4, index entry size at byte 6, glyph count at byte 8,
   bitmap base at byte 12. Each index entry is a codepoint plus
   (absolute offset | width << 24). */

typedef struct poteto_pbf1 {
    const uint8_t *data;
    size_t length;
    uint32_t count;
    uint32_t bitmap_base;
    uint16_t height;
} poteto_pbf1;

/* 0 when every header field and every bitmap offset fits this buffer. */
int poteto_pbf1_open(const uint8_t *data, size_t length, poteto_pbf1 *out);

/* Bitmap for cp, or NULL. *width is 8 or 16. The bitmap is height rows. */
const uint8_t *poteto_pbf1_glyph(const poteto_pbf1 *font, uint32_t cp, int *width);

#endif
