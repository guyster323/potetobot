#include "channel.h"

#include "destination.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

int poteto_channel_take(const uint8_t *voice, size_t voice_len, char *answer, size_t answer_len) {
    (void)voice;
    (void)voice_len;
    if (answer != NULL && answer_len > 0) {
        answer[0] = '\0';
    }
    return -1;
}

static int json_string_at(const char *at, const char *key, char *out, size_t cap) {
    size_t key_len;
    const char *p;
    size_t n = 0;
    if (at == NULL || key == NULL || out == NULL || cap < 2) {
        return -1;
    }
    key_len = strlen(key);
    if (strncmp(at, "\"", 1) != 0 || strncmp(at + 1, key, key_len) != 0 || at[1 + key_len] != '"') {
        return -1;
    }
    p = at + key_len + 2;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    if (*p != ':') {
        return -1;
    }
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    if (*p != '"') {
        return -1;
    }
    p++;
    while (*p != '\0' && n + 1 < cap) {
        if (*p == '\\') {
            p++;
            if (*p == '\0') {
                return -1;
            }
            if (*p == 'n') {
                out[n++] = '\n';
            } else if (*p == 'r') {
                out[n++] = '\r';
            } else {
                out[n++] = *p;
            }
            p++;
            continue;
        }
        if (*p == '"') {
            out[n] = '\0';
            return 0;
        }
        out[n++] = *p;
        p++;
    }
    return -1;
}

static const char *find_key(const char *start, const char *end, const char *key) {
    size_t key_len = strlen(key);
    const char *p = start;
    if (start == NULL || end == NULL || start >= end) {
        return NULL;
    }
    while (p + key_len + 2 < end) {
        if (p[0] == '"' && strncmp(p + 1, key, key_len) == 0 && p[1 + key_len] == '"') {
            return p;
        }
        p++;
    }
    return NULL;
}

static const char *skip_ws(const char *p, const char *end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
        p++;
    }
    return p;
}

static int hex_nibble(char ch) {
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

static int emit_utf8(char *out, size_t cap, size_t *n, unsigned cp) {
    if (cp < 0x80) {
        if (*n + 1 >= cap) {
            return -1;
        }
        out[(*n)++] = (char)cp;
        return 0;
    }
    if (cp < 0x800) {
        if (*n + 2 >= cap) {
            return -1;
        }
        out[(*n)++] = (char)(0xC0 | (cp >> 6));
        out[(*n)++] = (char)(0x80 | (cp & 0x3F));
        return 0;
    }
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return -1;
    }
    if (cp < 0x10000) {
        if (*n + 3 >= cap) {
            return -1;
        }
        out[(*n)++] = (char)(0xE0 | (cp >> 12));
        out[(*n)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[(*n)++] = (char)(0x80 | (cp & 0x3F));
        return 0;
    }
    if (*n + 4 >= cap) {
        return -1;
    }
    out[(*n)++] = (char)(0xF0 | (cp >> 18));
    out[(*n)++] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[(*n)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[(*n)++] = (char)(0x80 | (cp & 0x3F));
    return 0;
}

static const char *skip_string(const char *p, const char *end) {
    int esc = 0;
    if (p >= end || *p != '"') {
        return NULL;
    }
    p++;
    while (p < end) {
        if (esc) {
            esc = 0;
            p++;
            continue;
        }
        if (*p == '\\') {
            esc = 1;
            p++;
            continue;
        }
        if (*p == '"') {
            return p + 1;
        }
        p++;
    }
    return NULL;
}

static const char *match_brace(const char *open, const char *end, char close_ch) {
    char open_ch = close_ch == '}' ? '{' : '[';
    int depth = 0;
    int str = 0;
    int esc = 0;
    const char *p = open;
    while (p < end) {
        if (str) {
            if (esc) {
                esc = 0;
            } else if (*p == '\\') {
                esc = 1;
            } else if (*p == '"') {
                str = 0;
            }
            p++;
            continue;
        }
        if (*p == '"') {
            str = 1;
        } else if (*p == open_ch) {
            depth++;
        } else if (*p == close_ch) {
            depth--;
            if (depth == 0) {
                return p;
            }
        }
        p++;
    }
    return NULL;
}

static int take_string(const char **pp, const char *end, char *out, size_t cap) {
    const char *p = *pp;
    size_t n = 0;
    if (p >= end || *p != '"') {
        return -1;
    }
    p++;
    while (p < end) {
        unsigned char ch = (unsigned char)*p;
        if (*p == '\\') {
            p++;
            if (p >= end) {
                return -1;
            }
            if (*p == 'u') {
                unsigned cp = 0;
                int k;
                p++;
                for (k = 0; k < 4; k++) {
                    int h;
                    if (p >= end) {
                        return -1;
                    }
                    h = hex_nibble(*p++);
                    if (h < 0) {
                        return -1;
                    }
                    cp = (cp << 4) | (unsigned)h;
                }
                if (emit_utf8(out, cap, &n, cp) != 0) {
                    return -1;
                }
                continue;
            }
            ch = (unsigned char)*p++;
            if (ch == 'n') {
                ch = '\n';
            } else if (ch == 'r') {
                ch = '\r';
            } else if (ch == 't') {
                ch = '\t';
            }
        } else if (*p == '"') {
            if (n >= cap) {
                return -1;
            }
            out[n] = '\0';
            *pp = p + 1;
            return 0;
        } else {
            p++;
        }
        if (n + 1 >= cap) {
            return -1;
        }
        out[n++] = (char)ch;
    }
    return -1;
}

static int payload_has_request(const char *p, const char *end, const char *request_id) {
    char needle[96];
    size_t needle_len;
    size_t matched = 0;
    int esc = 0;
    if (snprintf(needle, sizeof(needle), "\"requestId\":\"%s\"", request_id) >= (int)sizeof(needle)) {
        return 0;
    }
    needle_len = strlen(needle);
    if (p >= end || *p != '"') {
        return 0;
    }
    p++;
    while (p < end) {
        char ch;
        if (esc) {
            esc = 0;
            ch = *p++;
            if (ch == 'n') {
                ch = '\n';
            } else if (ch == 'r') {
                ch = '\r';
            } else if (ch == 't') {
                ch = '\t';
            }
        } else if (*p == '\\') {
            esc = 1;
            p++;
            continue;
        } else if (*p == '"') {
            return 0;
        } else {
            ch = *p++;
        }
        if (ch == needle[matched]) {
            matched++;
            if (matched == needle_len) {
                return 1;
            }
        } else {
            matched = (ch == needle[0]) ? 1 : 0;
        }
    }
    return 0;
}

static const char *skip_value(const char *p, const char *end) {
    p = skip_ws(p, end);
    if (p >= end) {
        return NULL;
    }
    if (*p == '"') {
        return skip_string(p, end);
    }
    if (*p == '{') {
        const char *close = match_brace(p, end, '}');
        return close == NULL ? NULL : close + 1;
    }
    if (*p == '[') {
        const char *close = match_brace(p, end, ']');
        return close == NULL ? NULL : close + 1;
    }
    while (p < end && *p != ',' && *p != '}' && *p != ']') {
        p++;
    }
    return p;
}

static int object_answer(const char *open, const char *close, const char *request_id, char *body, size_t body_cap,
                         char *from, size_t from_cap) {
    const char *p = open + 1;
    int saw_thread = 0;
    int saw_body = 0;
    int saw_from = 0;
    int saw_request = 0;
    char key[80];
    char thread[80];
    body[0] = '\0';
    from[0] = '\0';
    while (p < close) {
        p = skip_ws(p, close);
        if (p >= close) {
            break;
        }
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p != '"') {
            return -1;
        }
        if (take_string(&p, close, key, sizeof(key)) != 0) {
            return -1;
        }
        p = skip_ws(p, close);
        if (p >= close || *p != ':') {
            return -1;
        }
        p = skip_ws(p + 1, close);
        if (strcmp(key, "thread_id") == 0) {
            if (saw_thread || take_string(&p, close, thread, sizeof(thread)) != 0) {
                return -1;
            }
            saw_thread = 1;
        } else if (strcmp(key, "body") == 0) {
            if (saw_body || take_string(&p, close, body, body_cap) != 0) {
                return -1;
            }
            saw_body = 1;
        } else if (strcmp(key, "from_handle") == 0) {
            if (saw_from || take_string(&p, close, from, from_cap) != 0) {
                return -1;
            }
            saw_from = 1;
        } else if (strcmp(key, "payload") == 0) {
            if (*p == '"') {
                if (payload_has_request(p, close, request_id)) {
                    saw_request = 1;
                }
            } else if (*p == '{') {
                const char *inner = match_brace(p, close, '}');
                const char *q;
                if (inner == NULL) {
                    return -1;
                }
                q = p;
                while (q < inner) {
                    const char *key_at = find_key(q, inner, "requestId");
                    char got[80];
                    if (key_at == NULL) {
                        break;
                    }
                    if (json_string_at(key_at, "requestId", got, sizeof(got)) == 0 && strcmp(got, request_id) == 0) {
                        saw_request = 1;
                        break;
                    }
                    q = key_at + 11;
                }
            }
            p = skip_value(p, close);
            if (p == NULL) {
                return -1;
            }
        } else {
            p = skip_value(p, close);
            if (p == NULL) {
                return -1;
            }
        }
    }
    if (!saw_thread || !saw_body || !saw_from || !saw_request) {
        return 0;
    }
    if (strcmp(thread, request_id) != 0 || body[0] == '\0' || strncmp(from, "orca_session_id:", 16) != 0) {
        return 0;
    }
    return 1;
}

int poteto_channel_match(const char *inbox_json, const char *request_id, char *answer, size_t answer_len,
                         char *from_handle, size_t from_cap) {
    const char *p;
    const char *end;
    int found = 0;
    int str = 0;
    int esc = 0;
    char body[8192];
    char from[160];
    if (answer != NULL && answer_len > 0) {
        answer[0] = '\0';
    }
    if (from_handle != NULL && from_cap > 0) {
        from_handle[0] = '\0';
    }
    if (inbox_json == NULL || request_id == NULL || request_id[0] == '\0') {
        return -1;
    }
    end = inbox_json + strlen(inbox_json);
    p = inbox_json;
    while (p < end) {
        const char *close;
        int got;
        if (str) {
            if (esc) {
                esc = 0;
            } else if (*p == '\\') {
                esc = 1;
            } else if (*p == '"') {
                str = 0;
            }
            p++;
            continue;
        }
        if (*p == '"') {
            str = 1;
            p++;
            continue;
        }
        if (*p != '{') {
            p++;
            continue;
        }
        close = match_brace(p, end, '}');
        if (close == NULL) {
            return -1;
        }
        got = object_answer(p, close, request_id, body, sizeof(body), from, sizeof(from));
        if (got < 0) {
            return -1;
        }
        if (got == 1) {
            if (found) {
                return -1;
            }
            if (answer != NULL && strlen(body) + 1 > answer_len) {
                return -1;
            }
            if (from_handle != NULL && strlen(from) + 1 > from_cap) {
                return -1;
            }
            if (answer != NULL && answer_len > 0) {
                memcpy(answer, body, strlen(body) + 1);
            }
            if (from_handle != NULL && from_cap > 0) {
                memcpy(from_handle, from, strlen(from) + 1);
            }
            found = 1;
        }
        p++;
    }
    return found ? 0 : -1;
}

#ifndef _WIN32
int poteto_channel_ask(const char *line, char *answer, size_t answer_len) {
    (void)line;
    if (answer != NULL && answer_len > 0) {
        answer[0] = '\0';
    }
    return -1;
}
#else

static int safe_token(const char *text) {
    size_t i;
    if (text == NULL || text[0] == '\0' || strlen(text) > 120) {
        return -1;
    }
    for (i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' ||
                 c == ':' || c == '.';
        if (!ok) {
            return -1;
        }
    }
    return 0;
}

static char *filtered_env(void) {
    char *src = GetEnvironmentStringsA();
    char *p;
    size_t need = 1;
    char *out;
    char *w;
    if (src == NULL) {
        return NULL;
    }
    for (p = src; *p != '\0'; p += strlen(p) + 1) {
        if (strncmp(p, "ORCA_", 5) == 0) {
            continue;
        }
        need += strlen(p) + 1;
    }
    out = (char *)malloc(need + 1);
    if (out == NULL) {
        FreeEnvironmentStringsA(src);
        return NULL;
    }
    w = out;
    for (p = src; *p != '\0'; p += strlen(p) + 1) {
        size_t n;
        if (strncmp(p, "ORCA_", 5) == 0) {
            continue;
        }
        n = strlen(p) + 1;
        memcpy(w, p, n);
        w += n;
    }
    *w = '\0';
    FreeEnvironmentStringsA(src);
    return out;
}

static int run_captured(char *cmdline, char *output, size_t cap) {
    SECURITY_ATTRIBUTES sa;
    HANDLE read_pipe = NULL;
    HANDLE write_pipe = NULL;
    HANDLE nul = NULL;
    HANDLE in_read = NULL;
    HANDLE in_write = NULL;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char *env;
    DWORD got = 0;
    size_t used = 0;
    int truncated = 0;
    DWORD wait;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    nul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0) || !CreatePipe(&in_read, &in_write, &sa, 0) ||
        nul == INVALID_HANDLE_VALUE) {
        if (read_pipe != NULL) {
            CloseHandle(read_pipe);
        }
        if (write_pipe != NULL) {
            CloseHandle(write_pipe);
        }
        if (in_read != NULL) {
            CloseHandle(in_read);
        }
        if (in_write != NULL) {
            CloseHandle(in_write);
        }
        if (nul != NULL && nul != INVALID_HANDLE_VALUE) {
            CloseHandle(nul);
        }
        return -1;
    }
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);
    CloseHandle(in_write);
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = in_read;
    si.hStdOutput = write_pipe;
    si.hStdError = nul;
    memset(&pi, 0, sizeof(pi));
    env = filtered_env();
    if (env == NULL || !CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, env, NULL, &si, &pi)) {
        free(env);
        CloseHandle(in_read);
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        CloseHandle(nul);
        return -1;
    }
    CloseHandle(in_read);
    free(env);
    CloseHandle(write_pipe);
    CloseHandle(nul);
    while (used + 1 < cap) {
        DWORD chunk = 0;
        if (!ReadFile(read_pipe, output + used, (DWORD)(cap - 1 - used), &chunk, NULL) || chunk == 0) {
            break;
        }
        used += chunk;
        if (used + 1 >= cap) {
            DWORD extra = 0;
            char toss[1];
            if (ReadFile(read_pipe, toss, 1, &extra, NULL) && extra > 0) {
                truncated = 1;
            }
            break;
        }
    }
    output[used] = '\0';
    wait = WaitForSingleObject(pi.hProcess, 30000);
    if (wait != WAIT_OBJECT_0) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(read_pipe);
        return -1;
    }
    GetExitCodeProcess(pi.hProcess, &got);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(read_pipe);
    if (truncated) {
        output[0] = '\0';
        return -1;
    }
    return got == 0 ? 0 : -1;
}

static int orca_exe(char *out, size_t cap) {
    DWORD n = GetEnvironmentVariableA("ORCA_CLI_COMMAND", out, (DWORD)cap);
    if (n == 0 || n >= cap) {
        if (cap < 5) {
            return -1;
        }
        memcpy(out, "orca", 5);
    }
    if (strchr(out, '"') != NULL) {
        return -1;
    }
    return 0;
}

static int sender_path(char *out, size_t cap) {
    char appdata[MAX_PATH];
    DWORD n;
    if (GetEnvironmentVariableA("POTETOBOT_SENDER_FILE", out, (DWORD)cap) > 0) {
        return 0;
    }
    n = GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return -1;
    }
    if (snprintf(out, cap, "%s\\potetobot\\host-terminal.json", appdata) >= (int)cap) {
        return -1;
    }
    return 0;
}

static int load_handle(char *handle, size_t cap) {
    char path[MAX_PATH];
    FILE *file;
    char buf[512];
    size_t n;
    const char *at;
    if (sender_path(path, sizeof(path)) != 0) {
        return -1;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return -1;
    }
    n = fread(buf, 1, sizeof(buf) - 1, file);
    fclose(file);
    buf[n] = '\0';
    at = find_key(buf, buf + n, "handle");
    if (at == NULL || json_string_at(at, "handle", handle, cap) != 0 || strncmp(handle, "term_", 5) != 0 ||
        safe_token(handle) != 0) {
        return -1;
    }
    return 0;
}

static int save_handle(const char *handle) {
    char path[MAX_PATH];
    char dir[MAX_PATH];
    FILE *file;
    char *slash;
    if (sender_path(path, sizeof(path)) != 0) {
        return -1;
    }
    snprintf(dir, sizeof(dir), "%s", path);
    slash = strrchr(dir, '\\');
    if (slash != NULL) {
        *slash = '\0';
        CreateDirectoryA(dir, NULL);
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return -1;
    }
    fprintf(file, "{\"handle\":\"%s\"}\n", handle);
    fclose(file);
    return 0;
}

static int create_handle(char *handle, size_t cap) {
    char exe[MAX_PATH];
    char cmdline[1024];
    char output[4096];
    const char *at;
    if (orca_exe(exe, sizeof(exe)) != 0) {
        return -1;
    }
    snprintf(cmdline, sizeof(cmdline), "\"%s\" terminal create --title potetobot-host --json", exe);
    if (run_captured(cmdline, output, sizeof(output)) != 0) {
        return -1;
    }
    at = find_key(output, output + strlen(output), "handle");
    if (at == NULL || json_string_at(at, "handle", handle, cap) != 0 || strncmp(handle, "term_", 5) != 0 ||
        safe_token(handle) != 0) {
        return -1;
    }
    return save_handle(handle);
}

static int write_body(const char *line, char *path, size_t cap) {
    char temp[MAX_PATH];
    FILE *file;
    DWORD n = GetTempPathA(MAX_PATH, temp);
    if (n == 0 || n >= MAX_PATH || cap < MAX_PATH) {
        return -1;
    }
    if (GetTempFileNameA(temp, "ptb", 0, path) == 0) {
        return -1;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return -1;
    }
    if (fwrite(line, 1, strlen(line), file) != strlen(line)) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

static int count_chat_id(const char *json, const char *uuid) {
    const char *p;
    const char *end;
    int count = 0;
    int str = 0;
    int esc = 0;
    if (json == NULL || uuid == NULL || uuid[0] == '\0') {
        return -1;
    }
    end = json + strlen(json);
    p = json;
    while (p < end) {
        const char *close;
        const char *q;
        char key[80];
        char got[80];
        if (str) {
            if (esc) {
                esc = 0;
            } else if (*p == '\\') {
                esc = 1;
            } else if (*p == '"') {
                str = 0;
            }
            p++;
            continue;
        }
        if (*p == '"') {
            str = 1;
            p++;
            continue;
        }
        if (*p != '{') {
            p++;
            continue;
        }
        close = match_brace(p, end, '}');
        if (close == NULL) {
            return -1;
        }
        q = p + 1;
        while (q < close) {
            q = skip_ws(q, close);
            if (q >= close) {
                break;
            }
            if (*q == ',') {
                q++;
                continue;
            }
            if (*q != '"') {
                return -1;
            }
            if (take_string(&q, close, key, sizeof(key)) != 0) {
                return -1;
            }
            q = skip_ws(q, close);
            if (q >= close || *q != ':') {
                return -1;
            }
            q = skip_ws(q + 1, close);
            if (strcmp(key, "id") == 0) {
                if (take_string(&q, close, got, sizeof(got)) != 0) {
                    return -1;
                }
                if (strcmp(got, uuid) == 0) {
                    count++;
                }
            } else {
                q = skip_value(q, close);
                if (q == NULL) {
                    return -1;
                }
            }
        }
        p++;
    }
    return count;
}

static int chat_belongs(const char *exe, const char *bot_id, const char *from_handle) {
    char cmdline[1024];
    char *output;
    const char *uuid = from_handle + strlen("orca_session_id:");
    int count;
    output = (char *)malloc(1 << 20);
    if (output == NULL) {
        return -1;
    }
    snprintf(cmdline, sizeof(cmdline), "\"%s\" bot chats --bot \"%s\" --json", exe, bot_id);
    if (run_captured(cmdline, output, 1 << 20) != 0) {
        free(output);
        return -1;
    }
    count = count_chat_id(output, uuid);
    free(output);
    return count == 1 ? 0 : -1;
}

int poteto_channel_ask(const char *line, char *answer, size_t answer_len) {
    poteto_destination dest;
    char exe[MAX_PATH];
    char handle[80];
    char body_path[MAX_PATH];
    char subject[64];
    char cmdline[2048];
    char *output;
    char request[80];
    char from[160];
    const char *at;
    size_t i;
    ULONGLONG deadline;
    static int sent_once = 0;
    if (answer != NULL && answer_len > 0) {
        answer[0] = '\0';
    }
    if (sent_once) {
        fprintf(stderr, "channel already sent\n");
        return -1;
    }
    if (line == NULL || line[0] == '\0' || line[0] == '\n' || line[0] == '\r' || strlen(line) > 2000) {
        fprintf(stderr, "channel question rejected\n");
        return -1;
    }
    memset(&dest, 0, sizeof(dest));
    if (poteto_decide_submission(&dest) != 0) {
        return -1;
    }
    if (strcmp(dest.status, POTETO_DEST_STATUS_NOT_SENT) == 0 || dest.bot_id[0] == '\0') {
        fprintf(stderr, "channel status not_sent\n");
        return -1;
    }
    if (strcmp(dest.provider, "orca_bots") != 0) {
        fprintf(stderr, "channel status blocked\n");
        return -1;
    }
    if (safe_token(dest.bot_id) != 0 || orca_exe(exe, sizeof(exe)) != 0) {
        return -1;
    }
    if (load_handle(handle, sizeof(handle)) != 0) {
        if (create_handle(handle, sizeof(handle)) != 0) {
            fprintf(stderr, "channel sender missing\n");
            return -1;
        }
    }
    if (write_body(line, body_path, sizeof(body_path)) != 0) {
        return -1;
    }
    snprintf(subject, sizeof(subject), "potetobot-%08lx", (unsigned long)GetTickCount());
    (void)i;
    snprintf(cmdline, sizeof(cmdline),
             "\"%s\" orchestration send --to \"bot:%s\" --chat new --type question --from \"%s\" --subject \"%s\" "
             "--body-file \"%s\" --json",
             exe, dest.bot_id, handle, subject, body_path);
    output = (char *)malloc(1 << 20);
    if (output == NULL) {
        DeleteFileA(body_path);
        return -1;
    }
    if (run_captured(cmdline, output, 1 << 20) != 0) {
        DeleteFileA(body_path);
        free(output);
        return -1;
    }
    DeleteFileA(body_path);
    sent_once = 1;
    at = find_key(output, output + strlen(output), "messageId");
    if (at == NULL || json_string_at(at, "messageId", request, sizeof(request)) != 0 || strncmp(request, "msg_", 4) != 0) {
        free(output);
        fprintf(stderr, "channel send had no request id\n");
        return -1;
    }
    fprintf(stderr, "request %s\n", request);
    fflush(stderr);
    deadline = GetTickCount64() + 120000;
    for (;;) {
        snprintf(cmdline, sizeof(cmdline), "\"%s\" orchestration inbox --terminal \"%s\" --limit 20 --full --json", exe, handle);
        if (run_captured(cmdline, output, 1 << 20) == 0 &&
            poteto_channel_match(output, request, answer, answer_len, from, sizeof(from)) == 0) {
            if (chat_belongs(exe, dest.bot_id, from) != 0) {
                fprintf(stderr, "channel from_handle mismatch %s\n", request);
                free(output);
                return -1;
            }
            fprintf(stderr, "from %s\n", from);
            free(output);
            return 0;
        }
        if (GetTickCount64() >= deadline) {
            break;
        }
        Sleep(2000);
    }
    fprintf(stderr, "timeout %s\n", request);
    free(output);
    return -1;
}

#endif
