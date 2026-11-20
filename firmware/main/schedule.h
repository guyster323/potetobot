#ifndef POTETO_SCHEDULE_H
#define POTETO_SCHEDULE_H

#define POTETO_CANVAS 480
#define POTETO_PATH_MAX 96

typedef struct {
    char character[16];
    char expression[16];
    int frame_index;
    int frame_count;
    int fps;
    int loops;
    char path[POTETO_PATH_MAX];
} poteto_frame_t;

int poteto_frame_count(const char *character, const char *expression);
const char *poteto_frame_path(const char *character, const char *expression, int index);

/* 0 when that loop has at least one frame. -1 when it does not. */
int poteto_frame_at(const char *character, const char *expression, int time_ms, poteto_frame_t *out);

/* Parse "SEL v1 <character> <expression> [time_ms]". */
int poteto_apply_line(const char *line, poteto_frame_t *out);

#endif
