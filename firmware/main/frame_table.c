#include <string.h>

typedef struct {
    const char *character;
    const char *expression;
    int count;
    const char *const *paths;
} poteto_packed_loop_t;

static const char *const k_poteto_idle[] = {"poteto/idle/f00.png", "poteto/idle/f01.png", "poteto/idle/f02.png"};
static const char *const k_poteto_listening[] = {0};
static const char *const k_poteto_thinking[] = {0};
static const char *const k_poteto_speaking[] = {"poteto/speaking/f00.png", "poteto/speaking/f01.png", "poteto/speaking/f02.png", "poteto/speaking/f03.png"};
static const char *const k_poteto_tilt[] = {"poteto/tilt/f00.png", "poteto/tilt/f01.png", "poteto/tilt/f02.png", "poteto/tilt/f03.png"};
static const char *const k_poteto_breath[] = {"poteto/breath/f00.png"};
static const char *const k_poteto_emphasis[] = {"poteto/emphasis/f00.png", "poteto/emphasis/f01.png", "poteto/emphasis/f02.png"};
static const char *const k_poteto_smile[] = {"poteto/smile/f00.png", "poteto/smile/f01.png", "poteto/smile/f02.png", "poteto/smile/f03.png", "poteto/smile/f04.png"};
static const char *const k_poteto_happy[] = {0};
static const char *const k_poteto_surprised[] = {0};
static const char *const k_poteto_sleepy[] = {0};
static const char *const k_poteto_dizzy[] = {0};
static const char *const k_poteto_wink[] = {0};
static const char *const k_sunny_idle[] = {"sunny/idle/f00.png", "sunny/idle/f01.png"};
static const char *const k_sunny_listening[] = {"sunny/listening/f00.png"};
static const char *const k_sunny_thinking[] = {"sunny/thinking/f00.png"};
static const char *const k_sunny_speaking[] = {"sunny/speaking/f00.png", "sunny/speaking/f01.png", "sunny/speaking/f02.png", "sunny/speaking/f03.png"};
static const char *const k_sunny_tilt[] = {0};
static const char *const k_sunny_breath[] = {0};
static const char *const k_sunny_emphasis[] = {0};
static const char *const k_sunny_smile[] = {"sunny/smile/f00.png"};
static const char *const k_sunny_happy[] = {0};
static const char *const k_sunny_surprised[] = {0};
static const char *const k_sunny_sleepy[] = {0};
static const char *const k_sunny_dizzy[] = {0};
static const char *const k_sunny_wink[] = {0};

static const poteto_packed_loop_t k_loops[] = {
    {"poteto", "idle", 3, k_poteto_idle},
    {"poteto", "listening", 0, k_poteto_listening},
    {"poteto", "thinking", 0, k_poteto_thinking},
    {"poteto", "speaking", 4, k_poteto_speaking},
    {"poteto", "tilt", 4, k_poteto_tilt},
    {"poteto", "breath", 1, k_poteto_breath},
    {"poteto", "emphasis", 3, k_poteto_emphasis},
    {"poteto", "smile", 5, k_poteto_smile},
    {"poteto", "happy", 0, k_poteto_happy},
    {"poteto", "surprised", 0, k_poteto_surprised},
    {"poteto", "sleepy", 0, k_poteto_sleepy},
    {"poteto", "dizzy", 0, k_poteto_dizzy},
    {"poteto", "wink", 0, k_poteto_wink},
    {"sunny", "idle", 2, k_sunny_idle},
    {"sunny", "listening", 1, k_sunny_listening},
    {"sunny", "thinking", 1, k_sunny_thinking},
    {"sunny", "speaking", 4, k_sunny_speaking},
    {"sunny", "tilt", 0, k_sunny_tilt},
    {"sunny", "breath", 0, k_sunny_breath},
    {"sunny", "emphasis", 0, k_sunny_emphasis},
    {"sunny", "smile", 1, k_sunny_smile},
    {"sunny", "happy", 0, k_sunny_happy},
    {"sunny", "surprised", 0, k_sunny_surprised},
    {"sunny", "sleepy", 0, k_sunny_sleepy},
    {"sunny", "dizzy", 0, k_sunny_dizzy},
    {"sunny", "wink", 0, k_sunny_wink},
};

int poteto_frame_count(const char *character, const char *expression) {
    size_t i;
    for (i = 0; i < sizeof(k_loops) / sizeof(k_loops[0]); i++) {
        if (strcmp(k_loops[i].character, character) == 0 &&
            strcmp(k_loops[i].expression, expression) == 0) {
            return k_loops[i].count;
        }
    }
    return -1;
}

const char *poteto_frame_path(const char *character, const char *expression, int index) {
    size_t i;
    for (i = 0; i < sizeof(k_loops) / sizeof(k_loops[0]); i++) {
        if (strcmp(k_loops[i].character, character) == 0 &&
            strcmp(k_loops[i].expression, expression) == 0) {
            if (index < 0 || index >= k_loops[i].count) {
                return 0;
            }
            return k_loops[i].paths[index];
        }
    }
    return 0;
}
