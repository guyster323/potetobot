#ifndef POTETO_DESTINATION_H
#define POTETO_DESTINATION_H

/* Host-only decision. Does not submit a line and does not touch the board. */

#define POTETO_DEST_STATUS_NOT_SENT "not_sent"
#define POTETO_DEST_STATUS_BLOCKED "blocked"

typedef struct poteto_destination {
    char provider[32];
    char bot_id[128];
    char source_path[512];
    const char *status;
} poteto_destination;

/* Read POTETOBOT_DESTINATION_FILE, or %APPDATA%\potetobot\destination.json.
   Missing file, empty provider, empty bot id, and an unknown provider are empty.
   Empty yields not_sent. grok_bot or orca_bots with a bot id yields blocked.
   Returns 0. The decision never submits. */
int poteto_decide_submission(poteto_destination *out);

#endif
