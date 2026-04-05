#ifndef CLUE_RENDERER_H
#define CLUE_RENDERER_H

#include <stdbool.h>

/* Forward declarations */
struct ClueWindow;
struct ClueFont;

/* RGBA colour (0.0 - 1.0) */
typedef struct {
    float r, g, b, a;
} ClueColor;

/* Convenience colour constructors */
#define CLUE_RGB(rv, gv, bv)       ((ClueColor){(rv)/255.0f, (gv)/255.0f, (bv)/255.0f, 1.0f})
#define CLUE_RGBA(rv, gv, bv, av)  ((ClueColor){(rv)/255.0f, (gv)/255.0f, (bv)/255.0f, (av)/255.0f})
#define CLUE_RGBF(rv, gv, bv)      ((ClueColor){(rv), (gv), (bv), 1.0f})
#define CLUE_RGBAF(rv, gv, bv, av) ((ClueColor){(rv), (gv), (bv), (av)})

/* Opaque texture handle for images */
typedef unsigned int ClueTexture;

/* Renderer interface for drawing operations */
typedef struct {
    /* Lifecycle */
    int   (*init)(void);
    void  (*shutdown)(void);

    /* Frame management */
    void  (*begin_frame)(struct ClueWindow *win);
    void  (*end_frame)(struct ClueWindow *win);
    void  (*clear)(float r, float g, float b, float a);

    /* Primitives */
    void  (*fill_rect)(int x, int y, int w, int h, ClueColor color);
    void  (*fill_rounded_rect)(int x, int y, int w, int h,
                               float radius, ClueColor color);
    void  (*draw_rect)(int x, int y, int w, int h,
                       float thickness, ClueColor color);
    void  (*draw_rounded_rect)(int x, int y, int w, int h,
                               float radius, float thickness, ClueColor color);
    void  (*draw_line)(int x0, int y0, int x1, int y1,
                       float thickness, ClueColor color);
    void  (*fill_circle)(int cx, int cy, int radius, ClueColor color);
    void  (*draw_circle)(int cx, int cy, int radius,
                         float thickness, ClueColor color);
    void  (*draw_arc)(int cx, int cy, int radius,
                      float start_rad, float end_rad,
                      float thickness, ClueColor color);

    /* Text */
    void  (*draw_text)(int x, int y, const char *text,
                       struct ClueFont *font, ClueColor color);

    /* Images */
    void  (*draw_image)(ClueTexture tex, int x, int y, int w, int h);

    /* Clipping */
    void  (*set_clip_rect)(int x, int y, int w, int h);
    void  (*reset_clip_rect)(void);
} ClueRenderer;

/* Create the default OpenGL ES renderer */
ClueRenderer *clue_gl_renderer_create(void);

/* Destroy the OpenGL ES renderer */
void clue_gl_renderer_destroy(ClueRenderer *renderer);

/* Load a texture from an image file (PNG, JPG, BMP, etc).
 * Returns 0 on failure. */
ClueTexture clue_texture_load(const char *path);

/* Load a texture from raw RGBA pixel data */
ClueTexture clue_texture_load_rgba(const unsigned char *pixels,
                                 int w, int h);

/* Destroy a texture */
void clue_texture_destroy(ClueTexture tex);

#endif /* CLUE_RENDERER_H */
