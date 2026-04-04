#ifndef CLUE_FONT_INTERNAL_H
#define CLUE_FONT_INTERNAL_H

#include "clue/font.h"
#include "clue/renderer.h"

/* Called by gl_renderer init/shutdown */
int  clue_font_renderer_init(void);
void clue_font_renderer_shutdown(void);

/* Called by gl_renderer draw_text */
void clue_font_draw_text(int x, int y, const char *text,
                         UIFont *font, UIColor color,
                         int vp_w, int vp_h);

#endif /* CLUE_FONT_INTERNAL_H */
