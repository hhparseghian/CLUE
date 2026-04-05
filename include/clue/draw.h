#ifndef CLUE_DRAW_H
#define CLUE_DRAW_H

#include "renderer.h"
#include "font.h"

/* Global drawing API -- uses the active ClueApp's renderer.
 * These are convenience wrappers so you don't need a renderer pointer. */

void clue_clear(float r, float g, float b, float a);
void clue_fill_rect(int x, int y, int w, int h, ClueColor color);
void clue_fill_rounded_rect(int x, int y, int w, int h,
                            float radius, ClueColor color);
void clue_draw_rect(int x, int y, int w, int h,
                    float thickness, ClueColor color);
void clue_draw_rounded_rect(int x, int y, int w, int h,
                            float radius, float thickness, ClueColor color);
void clue_draw_line(int x0, int y0, int x1, int y1,
                    float thickness, ClueColor color);
void clue_fill_circle(int cx, int cy, int radius, ClueColor color);
void clue_draw_circle(int cx, int cy, int radius,
                      float thickness, ClueColor color);
void clue_draw_arc(int cx, int cy, int radius,
                   float start_rad, float end_rad,
                   float thickness, ClueColor color);

/* Draw text with explicit font */
void clue_draw_text(int x, int y, const char *text,
                    ClueFont *font, ClueColor color);

/* Draw text with the app's default font */
void clue_draw_text_default(int x, int y, const char *text, ClueColor color);

/* Draw an image texture */
void clue_draw_image(ClueTexture tex, int x, int y, int w, int h);

/* Set a clip rectangle (scissor test) */
void clue_set_clip_rect(int x, int y, int w, int h);

/* Reset clipping to full viewport */
void clue_reset_clip_rect(void);

#endif /* CLUE_DRAW_H */
