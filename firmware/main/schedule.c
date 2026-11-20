#include "schedule.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    int fps;
    int loops;
} poteto_expr_info_t;

static const poteto_expr_info_t k_expressions[] = {
    {"idle", 6, 1},
    {"listening", 15, 1},
    {"thinking", 12, 1},
    {"speaking", 6, 1},
    {"tilt", 6, 1},
    {"breath", 6, 1},
    {"emphasis", 6, 1},
    {"smile", 6, 1},
    {"happy", 12, 1},
    {"surprised", 12, 0},
    {"sleepy", 10, 1},
    {"dizzy", 12, 1},
    {"wink", 12, 0},
};

static int known_character(const char *character) {
    return strcmp(character, "poteto") == 0 || strcmp(character, "sunny") == 0;
}

static const poteto_expr_info_t *find_expression(const char *expression) {
    size_t i;
    for (i = 0; i < sizeof(k_expressions) / sizeof(k_expressions[0]); i++) {
        if (strcmp(k_expressions[i].name, expression) == 0) {
            return &k_expressions[i];
        }
    }
    return NULL;
}

int poteto_frame_at(const char *character, const char *expression, int time_ms, poteto_frame_t *out) {
    const poteto_expr_info_t *info;
    int count;
    int step;
    int index;
    const char *path;

    if (character == NULL || expression == NULL || out == NULL) {
        return -1;
    }
    if (!known_character(character)) {
        return -1;
    }
    info = find_expression(expression);
    if (info == NULL) {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    snprintf(out->character, sizeof(out->character), "%s", character);
    snprintf(out->expression, sizeof(out->expression), "%s", expression);
    out->fps = info->fps;
    out->loops = info->loops;
    count = poteto_frame_count(character, expression);
    if (count < 0) {
        count = 0;
    }
    out->frame_count = count;
    if (count == 0) {
        return -1;
    }

    step = 1000 / info->fps;
    if (step < 1) {
        step = 1;
    }
    if (time_ms < 0) {
        time_ms = 0;
    }
    index = time_ms / step;
    if (info->loops) {
        index %= count;
    } else if (index >= count) {
        index = count - 1;
    }
    out->frame_index = index;
    path = poteto_frame_path(character, expression, index);
    if (path == NULL) {
        out->path[0] = '\0';
        return 0;
    }
    snprintf(out->path, sizeof(out->path), "%s", path);
    return 0;
}

int poteto_apply_line(const char *line, poteto_frame_t *out) {
    char cmd[8];
    char ver[8];
    char character[16];
    char expression[16];
    int time_ms = 0;
    int fields;

    if (line == NULL || out == NULL) {
        return -1;
    }
    fields = sscanf(line, "%7s %7s %15s %15s %d", cmd, ver, character, expression, &time_ms);
    if (fields < 4) {
        return -1;
    }
    if (strcmp(cmd, "SEL") != 0 || strcmp(ver, "v1") != 0) {
        return -1;
    }
    return poteto_frame_at(character, expression, time_ms, out);
}
