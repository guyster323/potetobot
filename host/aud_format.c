#include "companion.h"

#include <stdio.h>
#include <stdlib.h>

/* Reads raw PCM16 and writes AUD v1 frames. The firmware uses the same formatter. */

static int emit_drop(unsigned utt, const char *reason) {
    char line[64];
    if (poteto_aud_drop(utt, reason, line, sizeof(line)) != 0) {
        return 1;
    }
    fputs(line, stdout);
    return 0;
}

int main(int argc, char **argv) {
    FILE *file;
    long sz;
    unsigned char *buf;
    size_t n;
    size_t off = 0;
    unsigned seq = 0;
    char line[1600];
    if (argc != 2) {
        fprintf(stderr, "aud_format: pcm path\n");
        return 2;
    }
    file = fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "aud_format: open\n");
        return 2;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 2;
    }
    sz = ftell(file);
    if (sz < 0) {
        fclose(file);
        return 2;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 2;
    }
    if (sz == 0 || (sz % 2) != 0) {
        fclose(file);
        return emit_drop(1, "empty") == 0 ? 3 : 1;
    }
    if ((size_t)sz > POTETO_VOICE_MAX) {
        fclose(file);
        return emit_drop(1, "overflow") == 0 ? 3 : 1;
    }
    buf = (unsigned char *)malloc((size_t)sz);
    if (buf == NULL) {
        fclose(file);
        return 1;
    }
    n = fread(buf, 1, (size_t)sz, file);
    fclose(file);
    if (n != (size_t)sz) {
        free(buf);
        return 1;
    }
    while (off < n) {
        size_t chunk = n - off;
        if (chunk > POTETO_AUD_CHUNK) {
            chunk = POTETO_AUD_CHUNK;
        }
        if (poteto_aud_chunk(1, seq, buf + off, chunk, line, sizeof(line)) != 0) {
            free(buf);
            return 1;
        }
        fputs(line, stdout);
        off += chunk;
        seq++;
    }
    if (poteto_aud_end(1, n, seq, line, sizeof(line)) != 0) {
        free(buf);
        return 1;
    }
    fputs(line, stdout);
    free(buf);
    fprintf(stderr, "aud_bytes %u\n", (unsigned)n);
    fprintf(stderr, "aud_frames %u\n", seq);
    return 0;
}
