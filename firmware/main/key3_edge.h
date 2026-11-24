#ifndef POTETO_KEY3_EDGE_H
#define POTETO_KEY3_EDGE_H

typedef struct {
    int seen;
    int level;
    int armed;
} poteto_key3_edge;

/* level is the pin now (1 released). fell is a latch of a falling edge
   since the previous call, including one that already returned high.
   The first call never opens. A later open requires a release first,
   so a held pin does not repeat. Returns 1 when this edge should open. */
int poteto_key3_edge_step(poteto_key3_edge *st, int level, int fell);

#endif
