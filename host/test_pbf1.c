#include "pbf1.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *message) {
    fprintf(stderr, "pbf1: %s\n", message);
    return 1;
}

static int load(const char *path, uint8_t **out, size_t *length) {
    FILE *file;
    long size;
    uint8_t *data;
    file = fopen(path, "rb");
    if (file == NULL) {
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }
    size = ftell(file);
    if (size < 16) {
        fclose(file);
        return -1;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }
    data = (uint8_t *)malloc((size_t)size);
    if (data == NULL) {
        fclose(file);
        return -1;
    }
    if (fread(data, 1, (size_t)size, file) != (size_t)size) {
        free(data);
        fclose(file);
        return -1;
    }
    fclose(file);
    *out = data;
    *length = (size_t)size;
    return 0;
}

int main(int argc, char **argv) {
    uint8_t *data = NULL;
    uint8_t *copy = NULL;
    size_t length = 0;
    poteto_pbf1 font;
    int width = 0;
    const uint8_t *bits;
    static const uint8_t a_prefix[8] = {0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x24, 0x42};
    static const uint8_t go_prefix[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfc};
    uint8_t *entry = NULL;
    uint32_t info;
    uint32_t i;

    if (argc != 2) {
        return fail("need the real bubble_font.bin path");
    }
    if (load(argv[1], &data, &length) != 0) {
        return fail("could not read the font file");
    }
    if (poteto_pbf1_open(data, length, &font) != 0) {
        free(data);
        return fail("real atlas was rejected");
    }
    bits = poteto_pbf1_glyph(&font, 0x41, &width);
    if (bits == NULL || width != 8 || (size_t)(bits - data) + 16 > length || memcmp(bits, a_prefix, 8) != 0) {
        free(data);
        return fail("ASCII glyph did not match the file");
    }
    bits = poteto_pbf1_glyph(&font, 0xACE0, &width);
    if (bits == NULL || width != 16 || (size_t)(bits - data) + 32 > length || memcmp(bits, go_prefix, 8) != 0) {
        free(data);
        return fail("Hangul glyph did not match the file");
    }
    if (poteto_pbf1_glyph(&font, 0x10FFFF, &width) != NULL) {
        free(data);
        return fail("missing codepoint returned a bitmap");
    }
    copy = (uint8_t *)malloc(length);
    if (copy == NULL) {
        free(data);
        return fail("out of memory");
    }
    memcpy(copy, data, length);
    for (i = 0; i < font.count; i++) {
        uint8_t *slot = copy + 16 + (size_t)i * 8;
        uint32_t codepoint = (uint32_t)slot[0] | ((uint32_t)slot[1] << 8) | ((uint32_t)slot[2] << 16) |
                             ((uint32_t)slot[3] << 24);
        if (codepoint == 0x41) {
            entry = slot;
            break;
        }
    }
    if (entry == NULL) {
        free(copy);
        free(data);
        return fail("ASCII entry was not in the real index");
    }
    info = (uint32_t)entry[4] | ((uint32_t)entry[5] << 8) | ((uint32_t)entry[6] << 16) | ((uint32_t)entry[7] << 24);
    info = (info & 0xFF000000u) | 0x00FFFFFFu;
    entry[4] = (uint8_t)info;
    entry[5] = (uint8_t)(info >> 8);
    entry[6] = (uint8_t)(info >> 16);
    entry[7] = (uint8_t)(info >> 24);
    if (poteto_pbf1_open(copy, length, &font) == 0) {
        free(copy);
        free(data);
        return fail("out-of-range bitmap offset was accepted");
    }
    free(copy);
    free(data);
    printf("pbf1 ok ascii=U+0041 width=8 hangul=U+ACE0 width=16 oor=rejected bytes=%zu\n", length);
    return 0;
}
