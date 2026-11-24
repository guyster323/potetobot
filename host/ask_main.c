#include "channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Korean goes through --body-file. A C argv byte is not the question. */

static int read_file(const char *path, char **out) {
    FILE *file;
    long sz;
    char *buf;
    size_t n;
    file = fopen(path, "rb");
    if (file == NULL) {
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }
    sz = ftell(file);
    if (sz <= 0 || sz > 2000) {
        fclose(file);
        return -1;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }
    buf = (char *)malloc((size_t)sz + 1);
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
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
        n--;
    }
    buf[n] = '\0';
    if (n == 0 || memchr(buf, '\0', n) != NULL) {
        free(buf);
        return -1;
    }
    *out = buf;
    return 0;
}

static int write_answer(const char *path, const char *answer) {
    FILE *file = fopen(path, "wb");
    size_t n;
    if (file == NULL) {
        return -1;
    }
    n = strlen(answer);
    if (fwrite(answer, 1, n, file) != n) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

static int has_non_ascii(const char *text) {
    const unsigned char *p = (const unsigned char *)text;
    while (*p != '\0') {
        if (*p >= 0x80) {
            return 1;
        }
        p++;
    }
    return 0;
}

int main(int argc, char **argv) {
    char answer[8192];
    char *question = NULL;
    const char *answer_path = NULL;
    int i;
    if (argc == 2) {
        if (has_non_ascii(argv[1])) {
            fprintf(stderr, "ask: use --body-file\n");
            return 2;
        }
        question = argv[1];
    } else {
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--body-file") == 0 && i + 1 < argc) {
                if (read_file(argv[++i], &question) != 0) {
                    fprintf(stderr, "ask: body file\n");
                    return 2;
                }
                continue;
            }
            if (strcmp(argv[i], "--answer-file") == 0 && i + 1 < argc) {
                answer_path = argv[++i];
                continue;
            }
            fprintf(stderr, "ask: use --body-file\n");
            return 2;
        }
    }
    if (question == NULL || (argc != 2 && answer_path == NULL)) {
        fprintf(stderr, "ask: use --body-file and --answer-file\n");
        return 2;
    }
    if (poteto_channel_ask(question, answer, sizeof(answer)) != 0) {
        printf("answer -\n");
        if (question != argv[1]) {
            free(question);
        }
        return 1;
    }
    if (answer_path != NULL && write_answer(answer_path, answer) != 0) {
        fprintf(stderr, "ask: answer file\n");
        if (question != argv[1]) {
            free(question);
        }
        return 1;
    }
    if (argc == 2) {
        printf("answer %s\n", answer);
    } else {
        printf("answer_bytes %u\n", (unsigned)strlen(answer));
    }
    if (question != argv[1]) {
        free(question);
    }
    return 0;
}
