#ifndef POTETO_KEY3_H
#define POTETO_KEY3_H

#include "companion.h"

#include <stddef.h>
#include <stdint.h>

typedef struct poteto_key3_face {
    int changed;
    char expression[POTETO_FACE_MAX];
    size_t uplink_len;
    const uint8_t *uplink;
} poteto_key3_face;

/* 0 when the line was queued. -1 when the line could not be queued. */
typedef int (*poteto_key3_line_fn)(const char *line, void *ctx);

/* GPIO18, active low. Arming does not open the microphone. */
void poteto_key3_init(void);

/* One capture step, independent of the 66 ms draw.
   While held, reads I2S in chunks of up to 2048 bytes and emits
   POTETO_AUD_CHUNK frames. Does not draw and does not write MIC v1. */
void poteto_key3_service(poteto_key3_line_fn fn, void *ctx);

/* Copies a pending face change. uplink_len stays 0. */
void poteto_key3_take_face(poteto_key3_face *face);

#endif
