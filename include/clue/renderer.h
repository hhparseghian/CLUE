#ifndef CLUE_RENDERER_H
#define CLUE_RENDERER_H

#include <stdbool.h>

/* Forward declarations */
struct UIWindow;
struct UIFont;

/* RGBA colour (0.0 - 1.0) */
typedef struct {
    float r, g, b, a;
} UIColor;

/* Convenience colour constructors */
#define UI_RGB(rv, gv, bv)       ((UIColor){(rv)/255.0f, (gv)/255.0f, (bv)/255.0f, 1.0f})
#define UI_RGBA(rv, gv, bv, av)  ((UIColor){(rv)/255.0f, (gv)/255.0f, (bv)/255.0f, (av)/255.0f})
#define UI_RGBF(rv, gv, bv)      ((UIColor){(rv), (gv), (bv), 1.0f})
#define UI_RGBAF(rv, gv, bv, av) ((UIColor){(rv), (gv), (bv), (av)})

/* Opaque texture handle for images */
typedef unsigned int UITexture;

/* Renderer interface for drawing operations */
typedef struct {
    /* Lifecycle */
    int   (*init)(void);
    void  (*shutdown)(void);

    /* Frame management */
    void  (*begin_frame)(struct UIWindow *win);
    void  (*end_frame)(struct UIWindow *win);
    void  (*clear)(float r, float g, float b, float a);

    /* Primitives */
    void  (*fill_rect)(int x, int y, int w, int h, UIColor color);
    void  (*fill_rect_solid)(int x, int y, int w, int h, UIColor color);
    void  (*fill_rounded_rect)(int x, int y, int w, int h,
                               float radius, UIColor color);
    void  (*draw_rect)(int x, int y, int w, int h,
                       float thickness, UIColor color);
    void  (*draw_rounded_rect)(int x, int y, int w, int h,
                               float radius, float thickness, UIColor color);
    void  (*draw_line)(int x0, int y0, int x1, int y1,
                       float thickness, UIColor color);
    void  (*fill_circle)(int cx, int cy, int radius, UIColor color);
    void  (*draw_circle)(int cx, int cy, int radius,
                         float thickness, UIColor color);

    /* Text */
    void  (*draw_text)(int x, int y, const char *text,
                       struct UIFont *font, UIColor color);

    /* Images */
    void  (*draw_image)(UITexture tex, int x, int y, int w, int h);

    /* Clipping */
    void  (*set_clip_rect)(int x, int y, int w, int h);
    void  (*reset_clip_rect)(void);
} UIRenderer;

/* Create the default OpenGL ES renderer */
UIRenderer *clue_gl_renderer_create(void);

/* Destroy the OpenGL ES renderer */
void clue_gl_renderer_destroy(UIRenderer *renderer);

/* Load a texture from an image file (PNG, JPG, BMP, etc).
 * Returns 0 on failure. */
UITexture clue_texture_load(const char *path);

/* Load a texture from raw RGBA pixel data */
UITexture clue_texture_load_rgba(const unsigned char *pixels,
                                 int w, int h);

/* Destroy a texture */
void clue_texture_destroy(UITexture tex);

#endif /* CLUE_RENDERER_H */
