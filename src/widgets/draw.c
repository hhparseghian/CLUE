#include "clue/draw.h"
#include "clue/app.h"

#define R() do { ClueApp *a = clue_app_get(); if (!a || !a->renderer) return; } while(0)
#define APP clue_app_get()

void clue_clear(float r, float g, float b, float a)
{ R(); APP->renderer->clear(r, g, b, a); }

void clue_fill_rect(int x, int y, int w, int h, UIColor color)
{ R(); APP->renderer->fill_rect(x, y, w, h, color); }

void clue_fill_rounded_rect(int x, int y, int w, int h,
                            float radius, UIColor color)
{ R(); APP->renderer->fill_rounded_rect(x, y, w, h, radius, color); }

void clue_draw_rect(int x, int y, int w, int h,
                    float thickness, UIColor color)
{ R(); APP->renderer->draw_rect(x, y, w, h, thickness, color); }

void clue_draw_rounded_rect(int x, int y, int w, int h,
                            float radius, float thickness, UIColor color)
{ R(); APP->renderer->draw_rounded_rect(x, y, w, h, radius, thickness, color); }

void clue_draw_line(int x0, int y0, int x1, int y1,
                    float thickness, UIColor color)
{ R(); APP->renderer->draw_line(x0, y0, x1, y1, thickness, color); }

void clue_fill_circle(int cx, int cy, int radius, UIColor color)
{ R(); APP->renderer->fill_circle(cx, cy, radius, color); }

void clue_draw_circle(int cx, int cy, int radius,
                      float thickness, UIColor color)
{ R(); APP->renderer->draw_circle(cx, cy, radius, thickness, color); }

void clue_draw_arc(int cx, int cy, int radius,
                   float start_rad, float end_rad,
                   float thickness, UIColor color)
{ R(); if (APP->renderer->draw_arc) APP->renderer->draw_arc(cx, cy, radius, start_rad, end_rad, thickness, color); }

void clue_draw_text(int x, int y, const char *text,
                    UIFont *font, UIColor color)
{ R(); APP->renderer->draw_text(x, y, text, font, color); }

void clue_draw_text_default(int x, int y, const char *text, UIColor color)
{
    ClueApp *a = clue_app_get();
    if (!a || !a->renderer || !a->default_font) return;
    a->renderer->draw_text(x, y, text, a->default_font, color);
}

void clue_draw_image(UITexture tex, int x, int y, int w, int h)
{ R(); APP->renderer->draw_image(tex, x, y, w, h); }

void clue_set_clip_rect(int x, int y, int w, int h)
{ R(); APP->renderer->set_clip_rect(x, y, w, h); }

void clue_reset_clip_rect(void)
{ R(); APP->renderer->reset_clip_rect(); }
