#ifndef POTETO_BUBBLE_INCLUDED
#define POTETO_BUBBLE_INCLUDED

#include <stdint.h>

/* Speech bubble on the 480x480 panel, below the eyes and sunglasses.
   The rectangle is x=12, y=356, width=456, height=112. The sweet-potato
   top and the newspaper sun stay above it. Pixels under the bubble are
   dimmed, not erased. The glyph atlas is the font data partition, not
   the factory app image. */
#define POTETO_BUBBLE_X 12
#define POTETO_BUBBLE_Y 356
#define POTETO_BUBBLE_W 456
#define POTETO_BUBBLE_H 112

void poteto_bubble_init(void);
void poteto_bubble_push(const char *phase, const char *text);
void poteto_bubble_poll(void);
void poteto_bubble_paint(uint16_t *frame);

#endif
