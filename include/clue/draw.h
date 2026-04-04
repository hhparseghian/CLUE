#ifndef CLUE_DRAW_H
#define CLUE_DRAW_H

#include "renderer.h"
#include "font.h"

/* Global drawing API -- uses the active ClueApp's renderer.
 * These are convenience wrappers so you don't need a renderer pointer. */

void clue_clear(float r, float g, float b, float a);
void clue_fill_rect(int x, int y, int w, int h, UIColor color);
void clue_fill_rect_solid(int x, int y, int w, int h, UIColor color);
void clue_fill_rounded_rect(int x, int y, int w, int h,
                            float radius, UIColor color);
void clue_draw_rect(int x, int y, int w, int h,
                    float thickness, UIColor color);
void clue_draw_rounded_rect(int x, int y, int w, int h,
                            float radius, float thickness, UIColor color);
void clue_draw_line(int x0, int y0, int x1, int y1,
                    float thickness, UIColor color);
void clue_fill_circle(int cx, int cy, int radius, UIColor color);
void clue_draw_circle(int cx, int cy, int radius,
                      float thickness, UIColor color);

/* Draw text with explicit font */
void clue_draw_text(int x, int y, const char *text,
                    UIFont *font, UIColor color);

/* Draw text with the app's default font */
void clue_draw_text_default(int x, int y, const char *text, UIColor color);

/* Draw an image texture */
void clue_draw_image(UITexture tex, int x, int y, int w, int h);

/* Set a clip rectangle (scissor test) */
void clue_set_clip_rect(int x, int y, int w, int h);

/* Reset clipping to full viewport */
void clue_reset_clip_rect(void);

#endif /* CLUE_DRAW_H */
