#include "channel.h"
#include "companion.h"
#include "schedule.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int from_hex(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

static uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int load_wav_or_raw(const char *path, uint8_t *out, size_t cap, size_t *len) {
    FILE *file;
    uint8_t *buf;
    long sz;
    size_t n;
    size_t off;
    file = fopen(path, "rb");
    if (file == NULL) {
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }
    sz = ftell(file);
    if (sz < 0 || (size_t)sz > cap) {
        fclose(file);
        return -1;
    }
    rewind(file);
    buf = (uint8_t *)malloc((size_t)sz);
    if (buf == NULL) {
        fclose(file);
        return -1;
    }
    n = fread(buf, 1, (size_t)sz, file);
    fclose(file);
    if (n != (size_t)sz) {
        free(buf);
        return -1;
    }
    if (n >= 12 && memcmp(buf, "RIFF", 4) == 0 && memcmp(buf + 8, "WAVE", 4) == 0) {
        off = 12;
        while (off + 8 <= n) {
            uint32_t chunk = read_le32(buf + off + 4);
            if (memcmp(buf + off, "data", 4) == 0) {
                if (off + 8 + chunk > n || chunk > cap || chunk == 0) {
                    free(buf);
                    return -1;
                }
                memcpy(out, buf + off + 8, chunk);
                *len = chunk;
                free(buf);
                return 0;
            }
            off += 8 + chunk + (chunk & 1u);
        }
        free(buf);
        return -1;
    }
    memcpy(out, buf, n);
    *len = n;
    free(buf);
    return 0;
}

static int parse_hex(const char *text, uint8_t *out, size_t cap, size_t *len) {
    size_t n = 0;
    char path[512];
    if (text == NULL || text[0] == '-' || text[0] == '\n' || text[0] == '\0') {
        *len = 0;
        return 0;
    }
    if (text[0] == '@') {
        size_t i = 0;
        while (text[1 + i] != '\0' && text[1 + i] != '\n' && text[1 + i] != '\r' && i + 1 < sizeof(path)) {
            path[i] = text[1 + i];
            i++;
        }
        path[i] = '\0';
        return load_wav_or_raw(path, out, cap, len);
    }
    while (text[n] != '\0' && text[n] != '\n' && text[n] != '\r' && text[n] != ' ') {
        n++;
    }
    if (n == 0 || (n % 2) != 0 || (n / 2) > cap) {
        return -1;
    }
    {
        size_t i;
        for (i = 0; i < n; i += 2) {
            int hi = from_hex(text[i]);
            int lo = from_hex(text[i + 1]);
            if (hi < 0 || lo < 0) {
                return -1;
            }
            out[i / 2] = (uint8_t)((hi << 4) | lo);
        }
    }
    *len = n / 2;
    return 0;
}

static void show(const poteto_companion *st, int time_ms, const uint8_t *uplink, size_t uplink_len) {
    poteto_frame_t frame;
    char *mic = NULL;
    size_t i;
    printf("expression %s\n", st->expression);
    printf("bubble_set %d\n", st->bubble_set);
    printf("bubble %s\n", st->bubble_set ? st->bubble : "-");
    if (uplink_len == 0 || uplink == NULL) {
        printf("uplink -\n");
        printf("mic -\n");
    } else {
        size_t mic_cap = 7 + (uplink_len * 2) + 2;
        fputs("uplink ", stdout);
        for (i = 0; i < uplink_len; i++) {
            printf("%02x", uplink[i]);
        }
        fputc('\n', stdout);
        mic = (char *)malloc(mic_cap);
        if (mic == NULL || poteto_voice_format(uplink, uplink_len, mic, mic_cap) != 0) {
            printf("mic -\n");
        } else {
            fputs("mic ", stdout);
            fputs(mic, stdout);
        }
        free(mic);
    }
    if (poteto_frame_at("poteto", st->expression, time_ms, &frame) != 0) {
        printf("frame missing\n");
        return;
    }
    printf("frame %d %d %s\n", frame.frame_index, frame.frame_count, frame.path);
}

int main(void) {
    poteto_companion st;
    char line[1024];
    uint8_t *samples;
    uint8_t *uplink;
    memset(&st, 0, sizeof(st));
    poteto_companion_init(&st);
    samples = (uint8_t *)malloc(POTETO_VOICE_MAX);
    uplink = (uint8_t *)malloc(POTETO_VOICE_MAX);
    if (samples == NULL || uplink == NULL) {
        fprintf(stderr, "sim: no memory\n");
        return 1;
    }
    while (fgets(line, (int)sizeof(line), stdin) != NULL) {
        if (strncmp(line, "end", 3) == 0) {
            break;
        }
        if (strncmp(line, "reset", 5) == 0) {
            poteto_companion_init(&st);
            show(&st, 0, NULL, 0);
            continue;
        }
        if (strncmp(line, "at ", 3) == 0) {
            show(&st, atoi(line + 3), NULL, 0);
            continue;
        }
        if (strncmp(line, "btn ", 4) == 0) {
            size_t sample_len = 0;
            size_t uplink_len = 0;
            int level;
            if ((line[4] != '0' && line[4] != '1') || line[5] != ' ') {
                fprintf(stderr, "sim: bad button\n");
                return 1;
            }
            level = line[4] - '0';
            if (parse_hex(line + 6, samples, POTETO_VOICE_MAX, &sample_len) != 0) {
                fprintf(stderr, "sim: bad samples\n");
                return 1;
            }
            poteto_companion_button(&st, level, sample_len == 0 ? NULL : samples, sample_len, uplink, POTETO_VOICE_MAX,
                                    &uplink_len);
            show(&st, 0, uplink_len == 0 ? NULL : uplink, uplink_len);
            continue;
        }
        if (strncmp(line, "cue ", 4) == 0) {
            if (poteto_companion_cue(&st, line + 4) < 0) {
                fprintf(stderr, "sim: bad cue\n");
                return 1;
            }
            show(&st, 0, NULL, 0);
            continue;
        }
        fprintf(stderr, "sim: unknown\n");
        return 1;
    }
    free(samples);
    free(uplink);
    return 0;
}
