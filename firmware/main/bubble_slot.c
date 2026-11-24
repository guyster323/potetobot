#include "bubble_slot.h"

#include "say.h"

#include <stdio.h>
#include <string.h>

#define SAY_SLOTS 8

typedef struct {
    char phase[POTETO_SAY_PHASE_MAX];
    char text[POTETO_SAY_TEXT_MAX];
} say_slot_t;

static say_slot_t s_slots[SAY_SLOTS];
static int s_count;
static int s_origin;

void poteto_slots_reset(void) {
    memset(s_slots, 0, sizeof(s_slots));
    s_count = 0;
    s_origin = 0;
}

int poteto_slots_push(const char *phase, const char *text, int replace_thinking) {
    say_slot_t *slot;
    if (phase == NULL || text == NULL) {
        return -1;
    }
    if (replace_thinking && strcmp(phase, "answer") == 0 && s_count > 0) {
        say_slot_t *last = &s_slots[(s_origin + s_count - 1) % SAY_SLOTS];
        if (strcmp(last->phase, "thinking") == 0) {
            snprintf(last->phase, sizeof(last->phase), "%s", phase);
            snprintf(last->text, sizeof(last->text), "%s", text);
            return 0;
        }
    }
    if (s_count < SAY_SLOTS) {
        slot = &s_slots[s_count];
        s_count++;
    } else {
        slot = &s_slots[s_origin];
        s_origin = (s_origin + 1) % SAY_SLOTS;
    }
    snprintf(slot->phase, sizeof(slot->phase), "%s", phase);
    snprintf(slot->text, sizeof(slot->text), "%s", text);
    return 0;
}

int poteto_slots_push_txt(const char *phase, const char *text) {
    return poteto_slots_push(phase, text, 1);
}

int poteto_slots_push_mic(const char *text) {
    return poteto_slots_push("mic", text, 0);
}

int poteto_slots_count(void) {
    return s_count;
}

static const say_slot_t *slot_at(int index) {
    if (index < 0 || index >= s_count) {
        return NULL;
    }
    return &s_slots[(s_origin + index) % SAY_SLOTS];
}

const char *poteto_slots_phase(int index) {
    const say_slot_t *slot = slot_at(index);
    return slot == NULL ? NULL : slot->phase;
}

const char *poteto_slots_text(int index) {
    const say_slot_t *slot = slot_at(index);
    return slot == NULL ? NULL : slot->text;
}
