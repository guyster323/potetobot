#ifndef POTETO_SAY_H
#define POTETO_SAY_H

#include <stddef.h>

#define POTETO_SAY_PHASE_MAX 16
#define POTETO_SAY_TEXT_MAX 480

/* Parse "TXT v1 thinking", "TXT v1 <answer>", or "SAY v1 thinking|answer <text>".
   0 on a real text line, -1 otherwise. phase is "thinking" or "answer". */
int poteto_apply_say(const char *line, char *phase, size_t phase_len, char *text, size_t text_len);

#endif
