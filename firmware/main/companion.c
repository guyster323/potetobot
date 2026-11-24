#include "companion.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

static void set_expression(poteto_companion *st, const char *name) {
    snprintf(st->expression, sizeof(st->expression), "%s", name);
}

static size_t utf8_span(const char *text) {
    unsigned char lead = (unsigned char)text[0];
    size_t span = 1;
    size_t k;
    if ((lead & 0x80) == 0) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        span = 2;
    } else if ((lead & 0xF0) == 0xE0) {
        span = 3;
    } else if ((lead & 0xF8) == 0xF0) {
        span = 4;
    } else {
        return 0;
    }
    for (k = 1; k < span; k++) {
        if (((unsigned char)text[k] & 0xC0) != 0x80) {
            return 0;
        }
    }
    return span;
}

static void keep_bubble(poteto_companion *st, const char *text) {
    size_t n = 0;
    if (text == NULL) {
        st->bubble[0] = '\0';
        st->bubble_set = 0;
        return;
    }
    while (text[n] != '\0' && text[n] != '\r' && text[n] != '\n') {
        size_t span = utf8_span(text + n);
        if (span == 0 || n + span >= POTETO_BUBBLE_KEEP) {
            break;
        }
        n += span;
    }
    memcpy(st->bubble, text, n);
    st->bubble[n] = '\0';
    st->bubble_set = n > 0;
}

static int ensure_voice(poteto_companion *st) {
    if (st->voice != NULL) {
        return 0;
    }
#ifdef ESP_PLATFORM
    st->voice = heap_caps_malloc(POTETO_VOICE_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    st->voice = malloc(POTETO_VOICE_MAX);
#endif
    if (st->voice == NULL) {
        return -1;
    }
    st->voice_cap = POTETO_VOICE_MAX;
    return 0;
}

static void begin_voice(poteto_companion *st) {
    st->voice_utt++;
    if (st->voice_utt == 0) {
        st->voice_utt = 1;
    }
    if (st->presses < 2) {
        st->presses++;
    }
}

static void append_voice(poteto_companion *st, const uint8_t *samples, size_t sample_len) {
    size_t room;
    if (samples == NULL || sample_len == 0 || ensure_voice(st) != 0) {
        return;
    }
    if (st->overflow || st->voice_len >= st->voice_cap) {
        st->overflow = 1;
        return;
    }
    room = st->voice_cap - st->voice_len;
    if (sample_len > room) {
        st->overflow = 1;
        return;
    }
    memcpy(st->voice + st->voice_len, samples, sample_len);
    st->voice_len += sample_len;
}

void poteto_companion_init(poteto_companion *st) {
    uint8_t *voice;
    size_t cap;
    if (st == NULL) {
        return;
    }
    voice = st->voice;
    cap = st->voice_cap;
    memset(st, 0, sizeof(*st));
    st->voice = voice;
    st->voice_cap = cap;
    st->level = 1;
    set_expression(st, "idle");
}

int poteto_companion_button(poteto_companion *st, int level, const uint8_t *samples, size_t sample_len,
                            uint8_t *uplink, size_t uplink_cap, size_t *uplink_len) {
    int changed = 0;
    if (uplink_len != NULL) {
        *uplink_len = 0;
    }
    if (st == NULL) {
        return 0;
    }
    if (!st->seen) {
        st->seen = 1;
        st->level = level;
        if (level == 0) {
            begin_voice(st);
            st->voice_len = 0;
            st->overflow = 0;
            st->bubble_set = 0;
            set_expression(st, "listening");
            append_voice(st, samples, sample_len);
            changed = 1;
        }
        return changed;
    }
    if (st->level != 0 && level == 0) {
        begin_voice(st);
        st->voice_len = 0;
        st->overflow = 0;
        st->bubble_set = 0;
        set_expression(st, "listening");
        changed = 1;
    }
    if (level == 0) {
        append_voice(st, samples, sample_len);
    }
    if (st->level == 0 && level != 0) {
        set_expression(st, "thinking");
        changed = 1;
        if (st->voice_len > 0 && st->voice != NULL && uplink != NULL && uplink_len != NULL &&
            uplink_cap >= st->voice_len) {
            memcpy(uplink, st->voice, st->voice_len);
            *uplink_len = st->voice_len;
        }
    }
    st->level = level;
    return changed;
}

static int take_utt(const char **body, unsigned *utt) {
    const char *s = *body;
    unsigned value = 0;
    int digits = 0;
    if (*s < '0' || *s > '9') {
        return 0;
    }
    while (*s >= '0' && *s <= '9') {
        int digit = *s - '0';
        if (digits == 10 || value > 429496729u || (value == 429496729u && digit > 5)) {
            return -1;
        }
        value = value * 10u + (unsigned)digit;
        s++;
        digits++;
    }
    if (*s != ' ') {
        return 0;
    }
    *utt = value;
    *body = s + 1;
    return 1;
}

static int cue_applies(const poteto_companion *st, int tagged, unsigned utt) {
    if (st->presses == 0) {
        return 1;
    }
    if (!tagged) {
        return st->presses < 2;
    }
    return utt == st->voice_utt && utt != 0;
}

int poteto_companion_cue(poteto_companion *st, const char *line) {
    const char *body;
    unsigned utt = 0;
    int tagged;
    if (st == NULL || line == NULL || strncmp(line, "CUE v1 ", 7) != 0) {
        return -1;
    }
    body = line + 7;
    tagged = take_utt(&body, &utt);
    if (tagged < 0) {
        return -1;
    }
    if (!cue_applies(st, tagged, utt)) {
        return 1;
    }
    if (strncmp(body, "prepare", 7) == 0) {
        char tail = body[7];
        if (tail != '\0' && tail != '\n' && tail != '\r' && tail != ' ' && tail != '\t') {
            return -1;
        }
        set_expression(st, "tilt");
        st->bubble_set = 0;
        return 0;
    }
    if (strncmp(body, "idle", 4) == 0) {
        char tail = body[4];
        if (tail != '\0' && tail != '\n' && tail != '\r' && tail != ' ' && tail != '\t') {
            return -1;
        }
        set_expression(st, "idle");
        st->bubble[0] = '\0';
        st->bubble_set = 0;
        return 0;
    }
    if (strncmp(body, "error", 5) == 0) {
        char tail = body[5];
        if (tail != '\0' && tail != '\n' && tail != '\r' && tail != ' ' && tail != '\t') {
            return -1;
        }
        set_expression(st, "idle");
        keep_bubble(st, "error");
        return 0;
    }
    if (strncmp(body, "speak ", 6) != 0) {
        return -1;
    }
    keep_bubble(st, body + 6);
    if (!st->bubble_set) {
        return -1;
    }
    set_expression(st, "speaking");
    return 0;
}

static int nibble(int value) {
    return value < 10 ? '0' + value : 'a' + (value - 10);
}

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

int poteto_voice_format(const uint8_t *data, size_t len, char *line, size_t line_cap) {
    size_t i;
    if (data == NULL || line == NULL || len == 0 || len > POTETO_VOICE_MAX) {
        return -1;
    }
    if (line_cap < 7 + (len * 2) + 2) {
        return -1;
    }
    memcpy(line, "MIC v1 ", 7);
    for (i = 0; i < len; i++) {
        line[7 + (i * 2)] = (char)nibble(data[i] >> 4);
        line[7 + (i * 2) + 1] = (char)nibble(data[i] & 0x0F);
    }
    line[7 + (len * 2)] = '\n';
    line[7 + (len * 2) + 1] = '\0';
    return 0;
}

int poteto_aud_chunk(unsigned utt, unsigned seq, const uint8_t *data, size_t len, char *line, size_t line_cap) {
    int head;
    size_t i;
    if (data == NULL || line == NULL || len == 0 || len > POTETO_AUD_CHUNK || (len % 2) != 0) {
        return -1;
    }
    head = snprintf(line, line_cap, "AUD v1 %u %u %u ", utt, seq, (unsigned)len);
    if (head < 0 || (size_t)head + (len * 2) + 2 > line_cap) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        line[head + (int)(i * 2)] = (char)nibble(data[i] >> 4);
        line[head + (int)(i * 2) + 1] = (char)nibble(data[i] & 0x0F);
    }
    line[head + (int)(len * 2)] = '\n';
    line[head + (int)(len * 2) + 1] = '\0';
    return 0;
}

int poteto_aud_end(unsigned utt, size_t total, unsigned count, char *line, size_t line_cap) {
    int n;
    if (line == NULL || total == 0 || total > POTETO_VOICE_MAX || count == 0) {
        return -1;
    }
    n = snprintf(line, line_cap, "AUD v1 %u end %u %u\n", utt, (unsigned)total, count);
    if (n < 0 || (size_t)n >= line_cap) {
        return -1;
    }
    return 0;
}

int poteto_aud_drop(unsigned utt, const char *reason, char *line, size_t line_cap) {
    size_t i;
    int n;
    if (line == NULL || reason == NULL || reason[0] == '\0') {
        return -1;
    }
    for (i = 0; reason[i] != '\0'; i++) {
        if (reason[i] < 'a' || reason[i] > 'z') {
            return -1;
        }
    }
    n = snprintf(line, line_cap, "AUD v1 %u drop %s\n", utt, reason);
    if (n < 0 || (size_t)n >= line_cap) {
        return -1;
    }
    return 0;
}

int poteto_voice_parse(const char *line, uint8_t *data, size_t cap, size_t *len) {
    const char *hex;
    size_t n = 0;
    if (line == NULL || data == NULL || len == NULL || strncmp(line, "MIC v1 ", 7) != 0) {
        return -1;
    }
    hex = line + 7;
    if (from_hex(hex[0]) < 0) {
        return -1;
    }
    while (from_hex(hex[n]) >= 0) {
        n++;
    }
    if (n == 0 || (n % 2) != 0 || hex[n] != '\n') {
        return -1;
    }
    n /= 2;
    if (n > cap || n > POTETO_VOICE_MAX) {
        return -1;
    }
    {
        size_t i;
        for (i = 0; i < n; i++) {
            int hi = from_hex(hex[i * 2]);
            int lo = from_hex(hex[(i * 2) + 1]);
            data[i] = (uint8_t)((hi << 4) | lo);
        }
    }
    *len = n;
    return 0;
}

poteto_companion *poteto_companion_board(void) {
    static poteto_companion board;
    static int ready;
    if (!ready) {
        poteto_companion_init(&board);
        ready = 1;
    }
    return &board;
}
