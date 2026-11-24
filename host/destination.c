#include "destination.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <wchar.h>
#include <windows.h>
#endif

#define DEST_BODY_CAP 8192
#define DEST_DEPTH_MAX 32

static void clear_decision(poteto_destination *out) {
    memset(out, 0, sizeof(*out));
    out->status = POTETO_DEST_STATUS_NOT_SENT;
}

#ifdef _WIN32

static int utf8_from_wide(const wchar_t *wide, char *out, size_t out_len) {
    int wrote = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1, out, (int)out_len, NULL, NULL);
    if (wrote <= 0) {
        if (out_len > 0) {
            out[0] = '\0';
        }
        return -1;
    }
    return 0;
}

/* Empty override means the default file. A set path is used even when it is missing. */
static int resolve_path(wchar_t *path, size_t path_cap) {
    DWORD got;

    path[0] = L'\0';
    got = GetEnvironmentVariableW(L"POTETOBOT_DESTINATION_FILE", path, (DWORD)path_cap);
    if (got > 0 && got < path_cap) {
        return 0;
    }
    if (got >= path_cap) {
        path[0] = L'\0';
        return -1;
    }
    got = GetEnvironmentVariableW(L"APPDATA", path, (DWORD)path_cap);
    if (got == 0 || got >= path_cap) {
        path[0] = L'\0';
        return -1;
    }
    {
        static const wchar_t suffix[] = L"\\potetobot\\destination.json";
        size_t have = wcslen(path);
        size_t extra = wcslen(suffix);
        if (have + extra + 1 > path_cap) {
            path[0] = L'\0';
            return -1;
        }
        memcpy(path + have, suffix, (extra + 1) * sizeof(wchar_t));
    }
    return 0;
}

/* 0 read, -1 missing, -2 truncated or unreadable. A full buffer plus another byte is truncated. */
static int read_body(const wchar_t *path, char *body, size_t body_cap, size_t *out_len) {
    FILE *file;
    size_t nread;
    int extra;

    file = _wfopen(path, L"rb");
    if (file == NULL) {
        return -1;
    }
    nread = fread(body, 1, body_cap - 1, file);
    if (nread == body_cap - 1) {
        extra = fgetc(file);
        if (extra != EOF) {
            fclose(file);
            return -2;
        }
    }
    if (ferror(file)) {
        fclose(file);
        return -2;
    }
    fclose(file);
    body[nread] = '\0';
    *out_len = nread;
    return 0;
}

#else

static int resolve_path(wchar_t *path, size_t path_cap) {
    (void)path;
    (void)path_cap;
    return -1;
}

static int read_body(const wchar_t *path, char *body, size_t body_cap, size_t *out_len) {
    (void)path;
    (void)body;
    (void)body_cap;
    (void)out_len;
    return -1;
}

#endif

typedef struct json_cursor {
    const unsigned char *p;
    const unsigned char *end;
    int failed;
} json_cursor;

static void skip_ws(json_cursor *cur) {
    while (cur->p < cur->end) {
        unsigned char c = *cur->p;
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
        cur->p++;
    }
}

static int hex_value(unsigned char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static int read_hex4(json_cursor *cur, unsigned int *out) {
    unsigned int value = 0;
    int i;
    if ((size_t)(cur->end - cur->p) < 4) {
        return -1;
    }
    for (i = 0; i < 4; i++) {
        int digit = hex_value(cur->p[i]);
        if (digit < 0) {
            return -1;
        }
        value = (value << 4) | (unsigned int)digit;
    }
    cur->p += 4;
    *out = value;
    return 0;
}

static int append_cp(char *out, size_t cap, size_t *used, unsigned int cp, int store) {
    unsigned char tmp[4];
    int len;
    if (cp == 0 || cp > 0x10FFFF) {
        return -1;
    }
    if (cp < 0x80) {
        tmp[0] = (unsigned char)cp;
        len = 1;
    } else if (cp < 0x800) {
        tmp[0] = (unsigned char)(0xC0 | (cp >> 6));
        tmp[1] = (unsigned char)(0x80 | (cp & 0x3F));
        len = 2;
    } else if (cp < 0x10000) {
        tmp[0] = (unsigned char)(0xE0 | (cp >> 12));
        tmp[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[2] = (unsigned char)(0x80 | (cp & 0x3F));
        len = 3;
    } else {
        tmp[0] = (unsigned char)(0xF0 | (cp >> 18));
        tmp[1] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
        tmp[2] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[3] = (unsigned char)(0x80 | (cp & 0x3F));
        len = 4;
    }
    if (!store) {
        return 0;
    }
    if (*used + (size_t)len >= cap) {
        return -1;
    }
    memcpy(out + *used, tmp, (size_t)len);
    *used += (size_t)len;
    return 0;
}

/* 0 stored, 1 valid but longer than cap so the buffer is not a match key, -1 broken. */
static int parse_string(json_cursor *cur, char *out, size_t cap, int fail_if_long) {
    size_t used = 0;
    int too_long = 0;

    if (cur->p >= cur->end || *cur->p != '"') {
        cur->failed = 1;
        return -1;
    }
    cur->p++;
    while (cur->p < cur->end) {
        unsigned char c = *cur->p++;
        unsigned int cp;
        if (c == '"') {
            if (out != NULL && cap > 0) {
                out[too_long ? 0 : used] = '\0';
            }
            return too_long ? 1 : 0;
        }
        if (c == '\\') {
            if (cur->p >= cur->end) {
                cur->failed = 1;
                return -1;
            }
            c = *cur->p++;
            if (c == 'u') {
                unsigned int low;
                if (read_hex4(cur, &cp) != 0) {
                    cur->failed = 1;
                    return -1;
                }
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    if ((size_t)(cur->end - cur->p) < 2 || cur->p[0] != '\\' || cur->p[1] != 'u') {
                        cur->failed = 1;
                        return -1;
                    }
                    cur->p += 2;
                    if (read_hex4(cur, &low) != 0 || low < 0xDC00 || low > 0xDFFF) {
                        cur->failed = 1;
                        return -1;
                    }
                    cp = 0x10000u + (((cp - 0xD800u) << 10) | (low - 0xDC00u));
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    cur->failed = 1;
                    return -1;
                }
                if (append_cp(out, cap, &used, cp, !too_long) != 0) {
                    if (fail_if_long) {
                        cur->failed = 1;
                        return -1;
                    }
                    too_long = 1;
                }
                continue;
            }
            if (c == '"' || c == '\\' || c == '/') {
                cp = c;
            } else if (c == 'b') {
                cp = '\b';
            } else if (c == 'f') {
                cp = '\f';
            } else if (c == 'n') {
                cp = '\n';
            } else if (c == 'r') {
                cp = '\r';
            } else if (c == 't') {
                cp = '\t';
            } else {
                cur->failed = 1;
                return -1;
            }
            if (append_cp(out, cap, &used, cp, !too_long) != 0) {
                if (fail_if_long) {
                    cur->failed = 1;
                    return -1;
                }
                too_long = 1;
            }
            continue;
        }
        if (c < 0x20) {
            cur->failed = 1;
            return -1;
        }
        if (!too_long) {
            if (out == NULL || used + 1 >= cap) {
                if (fail_if_long) {
                    cur->failed = 1;
                    return -1;
                }
                too_long = 1;
            } else {
                out[used++] = (char)c;
            }
        }
    }
    cur->failed = 1;
    return -1;
}

static void parse_value(json_cursor *cur, int depth, char *provider, size_t provider_cap, char *bot_id, size_t bot_cap,
                        int *saw_provider, int *saw_bot);

static void parse_literal(json_cursor *cur, const char *literal) {
    size_t n = strlen(literal);
    if ((size_t)(cur->end - cur->p) < n || memcmp(cur->p, literal, n) != 0) {
        cur->failed = 1;
        return;
    }
    cur->p += n;
}

static void parse_number(json_cursor *cur) {
    if (cur->p < cur->end && *cur->p == '-') {
        cur->p++;
    }
    if (cur->p >= cur->end || *cur->p < '0' || *cur->p > '9') {
        cur->failed = 1;
        return;
    }
    if (*cur->p == '0') {
        cur->p++;
    } else {
        while (cur->p < cur->end && *cur->p >= '0' && *cur->p <= '9') {
            cur->p++;
        }
    }
    if (cur->p < cur->end && *cur->p == '.') {
        cur->p++;
        if (cur->p >= cur->end || *cur->p < '0' || *cur->p > '9') {
            cur->failed = 1;
            return;
        }
        while (cur->p < cur->end && *cur->p >= '0' && *cur->p <= '9') {
            cur->p++;
        }
    }
    if (cur->p < cur->end && (*cur->p == 'e' || *cur->p == 'E')) {
        cur->p++;
        if (cur->p < cur->end && (*cur->p == '+' || *cur->p == '-')) {
            cur->p++;
        }
        if (cur->p >= cur->end || *cur->p < '0' || *cur->p > '9') {
            cur->failed = 1;
            return;
        }
        while (cur->p < cur->end && *cur->p >= '0' && *cur->p <= '9') {
            cur->p++;
        }
    }
}

static void parse_array(json_cursor *cur, int depth, char *provider, size_t provider_cap, char *bot_id, size_t bot_cap,
                        int *saw_provider, int *saw_bot) {
    if (depth > DEST_DEPTH_MAX || cur->p >= cur->end || *cur->p != '[') {
        cur->failed = 1;
        return;
    }
    cur->p++;
    skip_ws(cur);
    if (cur->p < cur->end && *cur->p == ']') {
        cur->p++;
        return;
    }
    for (;;) {
        parse_value(cur, depth, provider, provider_cap, bot_id, bot_cap, saw_provider, saw_bot);
        if (cur->failed) {
            return;
        }
        skip_ws(cur);
        if (cur->p < cur->end && *cur->p == ',') {
            cur->p++;
            skip_ws(cur);
            continue;
        }
        if (cur->p < cur->end && *cur->p == ']') {
            cur->p++;
            return;
        }
        cur->failed = 1;
        return;
    }
}

static void parse_object(json_cursor *cur, int depth, char *provider, size_t provider_cap, char *bot_id, size_t bot_cap,
                         int *saw_provider, int *saw_bot) {
    if (depth > DEST_DEPTH_MAX || cur->p >= cur->end || *cur->p != '{') {
        cur->failed = 1;
        return;
    }
    cur->p++;
    skip_ws(cur);
    if (cur->p < cur->end && *cur->p == '}') {
        cur->p++;
        return;
    }
    for (;;) {
        char key[32];
        int key_state;
        int take_provider;
        int take_bot;
        key[0] = '\0';
        key_state = parse_string(cur, key, sizeof(key), 0);
        if (key_state < 0) {
            return;
        }
        skip_ws(cur);
        if (cur->p >= cur->end || *cur->p != ':') {
            cur->failed = 1;
            return;
        }
        cur->p++;
        skip_ws(cur);
        take_provider = depth == 0 && key_state == 0 && strcmp(key, "provider") == 0 && !*saw_provider;
        take_bot = depth == 0 && key_state == 0 && strcmp(key, "botId") == 0 && !*saw_bot;
        if (take_provider || take_bot) {
            char *slot = take_provider ? provider : bot_id;
            size_t slot_cap = take_provider ? provider_cap : bot_cap;
            if (take_provider) {
                *saw_provider = 1;
            } else {
                *saw_bot = 1;
            }
            slot[0] = '\0';
            if (cur->p < cur->end && *cur->p == '"') {
                if (parse_string(cur, slot, slot_cap, 1) != 0) {
                    slot[0] = '\0';
                    return;
                }
            } else {
                parse_value(cur, depth + 1, provider, provider_cap, bot_id, bot_cap, saw_provider, saw_bot);
            }
        } else {
            parse_value(cur, depth + 1, provider, provider_cap, bot_id, bot_cap, saw_provider, saw_bot);
        }
        if (cur->failed) {
            return;
        }
        skip_ws(cur);
        if (cur->p < cur->end && *cur->p == ',') {
            cur->p++;
            skip_ws(cur);
            continue;
        }
        if (cur->p < cur->end && *cur->p == '}') {
            cur->p++;
            return;
        }
        cur->failed = 1;
        return;
    }
}

static void parse_value(json_cursor *cur, int depth, char *provider, size_t provider_cap, char *bot_id, size_t bot_cap,
                        int *saw_provider, int *saw_bot) {
    if (cur->failed || cur->p >= cur->end) {
        cur->failed = 1;
        return;
    }
    if (*cur->p == '"') {
        if (parse_string(cur, NULL, 0, 0) < 0) {
            return;
        }
        return;
    }
    if (*cur->p == '{') {
        parse_object(cur, depth, provider, provider_cap, bot_id, bot_cap, saw_provider, saw_bot);
        return;
    }
    if (*cur->p == '[') {
        parse_array(cur, depth, provider, provider_cap, bot_id, bot_cap, saw_provider, saw_bot);
        return;
    }
    if (*cur->p == 't') {
        parse_literal(cur, "true");
        return;
    }
    if (*cur->p == 'f') {
        parse_literal(cur, "false");
        return;
    }
    if (*cur->p == 'n') {
        parse_literal(cur, "null");
        return;
    }
    parse_number(cur);
}

/* 0 only after one complete top-level object and nothing but whitespace remains. */
static int parse_document(const char *body, size_t length, char *provider, size_t provider_cap, char *bot_id,
                          size_t bot_cap) {
    json_cursor cur;
    int saw_provider = 0;
    int saw_bot = 0;
    const unsigned char *start = (const unsigned char *)body;

    provider[0] = '\0';
    bot_id[0] = '\0';
    memset(&cur, 0, sizeof(cur));
    cur.p = start;
    cur.end = start + length;
    if (length >= 3 && start[0] == 0xEF && start[1] == 0xBB && start[2] == 0xBF) {
        cur.p += 3;
    }
    skip_ws(&cur);
    parse_object(&cur, 0, provider, provider_cap, bot_id, bot_cap, &saw_provider, &saw_bot);
    if (cur.failed) {
        provider[0] = '\0';
        bot_id[0] = '\0';
        return -1;
    }
    skip_ws(&cur);
    if (cur.p != cur.end) {
        provider[0] = '\0';
        bot_id[0] = '\0';
        return -1;
    }
    return 0;
}

static void apply_decision(poteto_destination *out, const char *provider, const char *bot_id) {
    if (strcmp(provider, "grok_bot") != 0 && strcmp(provider, "orca_bots") != 0) {
        provider = "";
        bot_id = "";
    }
    snprintf(out->provider, sizeof(out->provider), "%s", provider);
    snprintf(out->bot_id, sizeof(out->bot_id), "%s", bot_id);
    if (out->provider[0] == '\0' || out->bot_id[0] == '\0') {
        out->status = POTETO_DEST_STATUS_NOT_SENT;
        return;
    }
    out->status = POTETO_DEST_STATUS_BLOCKED;
}

int poteto_decide_submission(poteto_destination *out) {
#ifdef _WIN32
    wchar_t path[1024];
#else
    wchar_t path[1];
#endif
    char body[DEST_BODY_CAP];
    char provider[32];
    char bot_id[128];
    size_t body_len = 0;
    int read_state;

    if (out == NULL) {
        return -1;
    }
    clear_decision(out);
    if (resolve_path(path, sizeof(path) / sizeof(path[0])) != 0) {
        return 0;
    }
#ifdef _WIN32
    if (utf8_from_wide(path, out->source_path, sizeof(out->source_path)) != 0) {
        out->source_path[0] = '\0';
        return 0;
    }
#endif
    read_state = read_body(path, body, sizeof(body), &body_len);
    if (read_state != 0) {
        return 0;
    }
    if (parse_document(body, body_len, provider, sizeof(provider), bot_id, sizeof(bot_id)) != 0) {
        return 0;
    }
    apply_decision(out, provider, bot_id);
    return 0;
}
