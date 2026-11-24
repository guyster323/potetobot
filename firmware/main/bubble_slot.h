#ifndef POTETO_BUBBLE_SLOT_H
#define POTETO_BUBBLE_SLOT_H

/* replace_thinking is the TXT answer path. A mic note passes 0 and keeps
   the previous thinking slot. */
int poteto_slots_push(const char *phase, const char *text, int replace_thinking);
int poteto_slots_push_txt(const char *phase, const char *text);
int poteto_slots_push_mic(const char *text);

void poteto_slots_reset(void);
int poteto_slots_count(void);
const char *poteto_slots_phase(int index);
const char *poteto_slots_text(int index);

#endif
