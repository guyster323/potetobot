#include "rle.h"

size_t poteto_rle_decode(const uint8_t *src, size_t src_len, uint16_t *dst, size_t pixel_count) {
    size_t offset = 0;
    size_t written = 0;

    if (src == NULL || dst == NULL || pixel_count == 0) {
        return 0;
    }
    while (written < pixel_count) {
        unsigned int run;
        unsigned int kind;
        unsigned int count;
        if (offset + 2 > src_len) {
            return 0;
        }
        run = (unsigned int)src[offset] | ((unsigned int)src[offset + 1] << 8);
        offset += 2;
        kind = run & 0x8000u;
        count = run & 0x7fffu;
        if (count == 0 || written + count > pixel_count) {
            return 0;
        }
        if (kind) {
            size_t i;
            for (i = 0; i < count; i++) {
                dst[written + i] = 0;
            }
        } else {
            size_t i;
            if (offset + count * 2 > src_len) {
                return 0;
            }
            for (i = 0; i < count; i++) {
                dst[written + i] = (uint16_t)(src[offset] | (src[offset + 1] << 8));
                offset += 2;
            }
        }
        written += count;
    }
    return written;
}
