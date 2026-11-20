#include "say.h"

#include <string.h>

static const char *skip_space(const char *s) {
    while (s != NULL && (*s == ' ' || *s == '\t')) {
        s++;
    }
    return s;
}

static int token(const char **cursor, char *out, size_t out_len) {
    const char *s;
    size_t n = 0;
    if (cursor == NULL || *cursor == NULL || out == NULL || out_len < 2) {
        return -1;
    }
    s = skip_space(*cursor);
    if (*s == '\0' || *s == '\r' || *s == '\n') {
        return -1;
    }
    while (*s != '\0' && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') {
        if (n + 1 >= out_len) {
            return -1;
        }
        out[n++] = *s++;
    }
    out[n] = '\0';
    *cursor = s;
    return 0;
}

static void copy_body(const char *cursor, char *text, size_t text_len) {
    size_t n = 0;
    while (cursor[n] != '\0' && cursor[n] != '\r' && cursor[n] != '\n') {
        n++;
    }
    while (n > 0 && (cursor[n - 1] == ' ' || cursor[n - 1] == '\t')) {
        n--;
    }
    if (n >= text_len) {
        n = text_len - 1;
    }
    memcpy(text, cursor, n);
    text[n] = '\0';
}

int poteto_apply_say(const char *line, char *phase, size_t phase_len, char *text, size_t text_len) {
    const char *cursor;
    char cmd[8];
    char ver[8];
    char kind[POTETO_SAY_PHASE_MAX];

    if (line == NULL || phase == NULL || text == NULL || phase_len < 9 || text_len < 4) {
        return -1;
    }
    cursor = line;
    if (token(&cursor, cmd, sizeof(cmd)) != 0 || token(&cursor, ver, sizeof(ver)) != 0) {
        return -1;
    }
    if (strcmp(ver, "v1") != 0) {
        return -1;
    }
    if (strcmp(cmd, "TXT") == 0) {
        cursor = skip_space(cursor);
        copy_body(cursor, text, text_len);
        if (strcmp(text, "thinking") == 0) {
            memcpy(phase, "thinking", 9);
            memcpy(text, "...", 4);
            return 0;
        }
        memcpy(phase, "answer", 7);
        return 0;
    }
    if (strcmp(cmd, "SAY") != 0) {
        return -1;
    }
    if (token(&cursor, kind, sizeof(kind)) != 0) {
        return -1;
    }
    if (strcmp(kind, "thinking") != 0 && strcmp(kind, "answer") != 0) {
        return -1;
    }
    if (strlen(kind) >= phase_len) {
        return -1;
    }
    cursor = skip_space(cursor);
    copy_body(cursor, text, text_len);
    memcpy(phase, kind, strlen(kind) + 1);
    return 0;
}
