#ifndef POTETO_CHANNEL_H
#define POTETO_CHANNEL_H

#include <stddef.h>
#include <stdint.h>

/* One text line in, that request's final answer out.
   Returns 0 only when the line was submitted to the one selected Orca bot
   and this call read that request's final answer.
   An empty destination returns -1 and does not send.
   grok_bot returns -1 and does not call Orca.
   A mismatch, a failure row, or a timeout returns -1 and does not send again.
   PCM bytes are not a question; poteto_channel_take does not send. */
int poteto_channel_ask(const char *line, char *answer, size_t answer_len);

int poteto_channel_take(const uint8_t *voice, size_t voice_len, char *answer, size_t answer_len);

/* 0 when exactly one inbox row has this request id as both thread_id and
   payload requestId, a non-empty body, and an orca_session_id from_handle.
   Any other row is ignored. Two matches fail closed. */
int poteto_channel_match(const char *inbox_json, const char *request_id, char *answer, size_t answer_len,
                         char *from_handle, size_t from_cap);

#endif
