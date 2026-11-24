#ifndef POTETO_COMPANION_H
#define POTETO_COMPANION_H

#include <stddef.h>
#include <stdint.h>

/* One held utterance, 16 kHz mono s16. 192000 bytes is 6.0 s.
   The bytes live on the heap (PSRAM on the device). Zero the struct
   before the first init. A chunk that would pass this cap sets overflow
   and is not stored. */
#define POTETO_VOICE_MAX 192000
/* 20 ms at 16 kHz mono s16. 640 bytes is 32000 bytes/s. */
#define POTETO_AUD_CHUNK 640
#define POTETO_FACE_MAX 16
/* 32 Hangul syllables, the short bubble. The full answer stays on the PC. */
#define POTETO_BUBBLE_KEEP 96

typedef struct poteto_companion {
    int seen;
    int level;
    int overflow;
    char expression[POTETO_FACE_MAX];
    char bubble[POTETO_BUBBLE_KEEP];
    int bubble_set;
    uint8_t *voice;
    size_t voice_cap;
    size_t voice_len;
    /* voice_utt is the KEY3 press that owns the screen. presses stops at 2. */
    unsigned voice_utt;
    unsigned presses;
} poteto_companion;

/* The struct must already be zeroed on the first call. Later calls keep voice. */
void poteto_companion_init(poteto_companion *st);

/* level 0 is held. Samples append only while held.
   Press selects listening and clears the capture.
   Release selects thinking and copies the capture to uplink.
   uplink_len is 0 unless this call released a non-empty capture.
   Returns 1 when the expression changed. */
int poteto_companion_button(poteto_companion *st, int level, const uint8_t *samples, size_t sample_len,
                            uint8_t *uplink, size_t uplink_cap, size_t *uplink_len);

/* "CUE v1 <utt> prepare|idle|error" and "CUE v1 <utt> speak <text>"
   change the face only when <utt> is the current KEY3 press.
   Untagged "CUE v1 prepare|idle|error|speak <text>" still applies
   before a second press, including a sim that has not seen KEY3.
   A well-formed cue for another press returns 1 and changes nothing.
   Returns 0 when the face changes, -1 when the line is not a cue. */
int poteto_companion_cue(poteto_companion *st, const char *line);

/* "MIC v1 <hex>\n". Empty input is not a voice line.
   The device uplink is AUD v1, not this one-line form. */
int poteto_voice_format(const uint8_t *data, size_t len, char *line, size_t line_cap);
int poteto_voice_parse(const char *line, uint8_t *data, size_t cap, size_t *len);

/* AUD v1 <utt> <seq> <len> <hex>\n
   AUD v1 <utt> end <total> <count>\n
   AUD v1 <utt> drop <reason>\n
   len is even and at most POTETO_AUD_CHUNK. total is at most POTETO_VOICE_MAX. */
int poteto_aud_chunk(unsigned utt, unsigned seq, const uint8_t *data, size_t len, char *line, size_t line_cap);
int poteto_aud_end(unsigned utt, size_t total, unsigned count, char *line, size_t line_cap);
int poteto_aud_drop(unsigned utt, const char *reason, char *line, size_t line_cap);

poteto_companion *poteto_companion_board(void);

#endif
