#include "pbf1.h"

#include <string.h>

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static int bitmap_fits(size_t length, uint32_t offset, int width, uint16_t height) {
    size_t bytes;
    if (width != 8 && width != 16) {
        return 0;
    }
    bytes = (size_t)(width / 8) * (size_t)height;
    if (bytes == 0 || offset > length || bytes > length - offset) {
        return 0;
    }
    return 1;
}

int poteto_pbf1_open(const uint8_t *data, size_t length, poteto_pbf1 *out) {
    uint16_t height;
    uint16_t entry_size;
    uint32_t count;
    uint32_t bitmap_base;
    uint32_t previous;
    uint32_t i;

    if (out == NULL) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    if (data == NULL || length < 16) {
        return -1;
    }
    if (data[0] != 'P' || data[1] != 'B' || data[2] != 'F' || data[3] != '1') {
        return -1;
    }
    height = read_u16(data + 4);
    entry_size = read_u16(data + 6);
    count = read_u32(data + 8);
    bitmap_base = read_u32(data + 12);
    if (height != 16 || entry_size != 8 || count == 0) {
        return -1;
    }
    if ((uint64_t)count > (length - 16) / 8) {
        return -1;
    }
    if (bitmap_base != 16u + count * 8u || bitmap_base > length) {
        return -1;
    }
    previous = 0;
    for (i = 0; i < count; i++) {
        const uint8_t *entry = data + 16 + (size_t)i * 8;
        uint32_t codepoint = read_u32(entry);
        uint32_t info = read_u32(entry + 4);
        uint32_t offset = info & 0x00FFFFFFu;
        int width = (int)(info >> 24);
        if (i > 0 && codepoint <= previous) {
            return -1;
        }
        if (offset < bitmap_base || !bitmap_fits(length, offset, width, height)) {
            return -1;
        }
        previous = codepoint;
    }
    out->data = data;
    out->length = length;
    out->count = count;
    out->bitmap_base = bitmap_base;
    out->height = height;
    return 0;
}

const uint8_t *poteto_pbf1_glyph(const poteto_pbf1 *font, uint32_t cp, int *width) {
    uint32_t lo;
    uint32_t hi;
    if (font == NULL || font->data == NULL || font->count == 0) {
        return NULL;
    }
    lo = 0;
    hi = font->count;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        const uint8_t *entry = font->data + 16 + (size_t)mid * 8;
        uint32_t key = read_u32(entry);
        uint32_t info;
        uint32_t offset;
        int glyph_width;
        if (key == cp) {
            info = read_u32(entry + 4);
            offset = info & 0x00FFFFFFu;
            glyph_width = (int)(info >> 24);
            if (!bitmap_fits(font->length, offset, glyph_width, font->height)) {
                return NULL;
            }
            if (width != NULL) {
                *width = glyph_width;
            }
            return font->data + offset;
        }
        if (key < cp) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return NULL;
}
