#include "key3_edge.h"

#include <stddef.h>

int poteto_key3_edge_step(poteto_key3_edge *st, int level, int fell) {
    int open;
    if (st == NULL) {
        return 0;
    }
    if (!st->seen) {
        st->seen = 1;
        st->level = level;
        st->armed = 0;
        return 0;
    }
    if (level != 0) {
        st->armed = 1;
    }
    open = 0;
    if (st->armed && (fell || (st->level != 0 && level == 0))) {
        st->armed = 0;
        open = 1;
    }
    st->level = level;
    return open;
}
