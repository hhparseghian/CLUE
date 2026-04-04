/*
 * CLUE -- Font rendering via FreeType
 *
 * Loads TTF/OTF fonts, rasterizes glyphs on demand into a texture atlas,
 * and provides metrics for text measurement and layout.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <GLES2/gl2.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "clue/font.h"
#include "clue/renderer.h"

/* ------------------------------------------------------------------ */
/* Glyph atlas                                                         */
/* ------------------------------------------------------------------ */

#define ATLAS_WIDTH   1024
#define ATLAS_HEIGHT  1024
#define GLYPH_PAD     1      /* padding between glyphs in atlas */
#define MAX_GLYPHS    256    /* ASCII cache (expandable later for UTF-8) */

/* Cached glyph info */
typedef struct {
    bool     loaded;
    int      advance_x;     /* horizontal advance in pixels */
    int      bmp_w, bmp_h;  /* bitmap dimensions */
    int      bmp_left;      /* left bearing */
    int      bmp_top;       /* top bearing (from baseline) */
    /* UV coordinates in atlas */
    float    u0, v0;
    float    u1, v1;
} GlyphInfo;

struct UIFont {
    FT_Face     face;
    int         size_px;
    int         ascent;
    int         descent;
    int         line_height;
    GLuint      atlas_tex;
    int         atlas_x;     /* next free x position in atlas */
    int         atlas_y;     /* current row y */
    int         atlas_row_h; /* tallest glyph in current row */
    GlyphInfo   glyphs[MAX_GLYPHS];
};

/* Shared FreeType library instance */
static FT_Library ft_lib = NULL;
static int        ft_ref = 0;

static int ft_init(void)
{
    if (ft_ref == 0) {
        if (FT_Init_FreeType(&ft_lib)) {
            fprintf(stderr, "clue-font: FT_Init_FreeType failed\n");
            return -1;
        }
    }
    ft_ref++;
    return 0;
}

static void ft_shutdown(void)
{
    ft_ref--;
    if (ft_ref <= 0 && ft_lib) {
        FT_Done_FreeType(ft_lib);
        ft_lib = NULL;
        ft_ref = 0;
    }
}

/* ------------------------------------------------------------------ */
/* Glyph rasterization                                                 */
/* ------------------------------------------------------------------ */

static GlyphInfo *ensure_glyph(UIFont *font, unsigned int codepoint)
{
    if (codepoint >= MAX_GLYPHS) return NULL;

    GlyphInfo *g = &font->glyphs[codepoint];
    if (g->loaded) return g;

    if (FT_Load_Char(font->face, codepoint, FT_LOAD_RENDER)) {
        return NULL;
    }

    FT_GlyphSlot slot = font->face->glyph;
    FT_Bitmap *bmp = &slot->bitmap;

    g->advance_x = (int)(slot->advance.x >> 6);
    g->bmp_w     = (int)bmp->width;
    g->bmp_h     = (int)bmp->rows;
    g->bmp_left  = slot->bitmap_left;
    g->bmp_top   = slot->bitmap_top;

    /* If glyph has no bitmap (e.g. space), just record advance */
    if (bmp->width == 0 || bmp->rows == 0) {
        g->u0 = g->v0 = g->u1 = g->v1 = 0.0f;
        g->loaded = true;
        return g;
    }

    /* Check if glyph fits in current row */
    if (font->atlas_x + (int)bmp->width + GLYPH_PAD > ATLAS_WIDTH) {
        /* Move to next row */
        font->atlas_x  = 0;
        font->atlas_y += font->atlas_row_h + GLYPH_PAD;
        font->atlas_row_h = 0;
    }

    if (font->atlas_y + (int)bmp->rows > ATLAS_HEIGHT) {
        fprintf(stderr, "clue-font: atlas full\n");
        return NULL;
    }

    /* Upload glyph bitmap to atlas */
    glBindTexture(GL_TEXTURE_2D, font->atlas_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    font->atlas_x, font->atlas_y,
                    bmp->width, bmp->rows,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    bmp->buffer);

    /* Compute UV coordinates */
    g->u0 = (float)font->atlas_x / ATLAS_WIDTH;
    g->v0 = (float)font->atlas_y / ATLAS_HEIGHT;
    g->u1 = (float)(font->atlas_x + bmp->width) / ATLAS_WIDTH;
    g->v1 = (float)(font->atlas_y + bmp->rows) / ATLAS_HEIGHT;

    /* Advance atlas cursor */
    font->atlas_x += (int)bmp->width + GLYPH_PAD;
    if ((int)bmp->rows > font->atlas_row_h) {
        font->atlas_row_h = (int)bmp->rows;
    }

    g->loaded = true;
    return g;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

UIFont *clue_font_load(const char *path, int size_px)
{
    if (ft_init() != 0) return NULL;

    UIFont *font = calloc(1, sizeof(UIFont));
    if (!font) {
        ft_shutdown();
        return NULL;
    }

    if (FT_New_Face(ft_lib, path, 0, &font->face)) {
        fprintf(stderr, "clue-font: failed to load %s\n", path);
        free(font);
        ft_shutdown();
        return NULL;
    }

    FT_Set_Pixel_Sizes(font->face, 0, size_px);

    font->size_px     = size_px;
    font->ascent      = (int)(font->face->size->metrics.ascender >> 6);
    font->descent     = (int)(font->face->size->metrics.descender >> 6);
    font->line_height = (int)(font->face->size->metrics.height >> 6);

    /* Create atlas texture */
    glGenTextures(1, &font->atlas_tex);
    glBindTexture(GL_TEXTURE_2D, font->atlas_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* Allocate empty atlas */
    unsigned char *zeroes = calloc(ATLAS_WIDTH * ATLAS_HEIGHT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                 ATLAS_WIDTH, ATLAS_HEIGHT, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, zeroes);
    free(zeroes);

    /* Pre-cache printable ASCII */
    for (unsigned int c = 32; c < 127; c++) {
        ensure_glyph(font, c);
    }

    return font;
}

void clue_font_destroy(UIFont *font)
{
    if (!font) return;
    if (font->atlas_tex) glDeleteTextures(1, &font->atlas_tex);
    if (font->face) FT_Done_Face(font->face);
    free(font);
    ft_shutdown();
}

int clue_font_text_width(UIFont *font, const char *text)
{
    if (!font || !text) return 0;
    int width = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        GlyphInfo *g = ensure_glyph(font, *p);
        if (g) width += g->advance_x;
    }
    return width;
}

int clue_font_line_height(UIFont *font)
{
    return font ? font->line_height : 0;
}

int clue_font_ascent(UIFont *font)
{
    return font ? font->ascent : 0;
}

/* ------------------------------------------------------------------ */
/* Text drawing (called from gl_renderer)                              */
/* ------------------------------------------------------------------ */

/* These are set by the renderer during init */
static GLuint text_prog = 0;
static GLint  text_a_pos = -1;
static GLint  text_a_uv = -1;
static GLint  text_u_resolution = -1;
static GLint  text_u_color = -1;
static GLint  text_u_tex = -1;

static const char *text_vert_src =
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "varying   vec2 v_uv;\n"
    "uniform   vec2 u_resolution;\n"
    "void main() {\n"
    "    vec2 ndc = (a_pos / u_resolution) * 2.0 - 1.0;\n"
    "    ndc.y = -ndc.y;\n"
    "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
    "    v_uv = a_uv;\n"
    "}\n";

static const char *text_frag_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform vec4      u_color;\n"
    "uniform sampler2D u_tex;\n"
    "void main() {\n"
    "    float a = texture2D(u_tex, v_uv).r;\n"
    "    gl_FragColor = vec4(u_color.rgb, u_color.a * a);\n"
    "}\n";

static GLuint compile_text_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "clue-font: shader error:\n%s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

int clue_font_renderer_init(void)
{
    GLuint vs = compile_text_shader(GL_VERTEX_SHADER, text_vert_src);
    GLuint fs = compile_text_shader(GL_FRAGMENT_SHADER, text_frag_src);
    if (!vs || !fs) return -1;

    text_prog = glCreateProgram();
    glAttachShader(text_prog, vs);
    glAttachShader(text_prog, fs);
    glLinkProgram(text_prog);

    GLint ok;
    glGetProgramiv(text_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(text_prog, sizeof(log), NULL, log);
        fprintf(stderr, "clue-font: link error:\n%s\n", log);
        glDeleteProgram(text_prog);
        text_prog = 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!text_prog) return -1;

    text_a_pos        = glGetAttribLocation(text_prog,  "a_pos");
    text_a_uv         = glGetAttribLocation(text_prog,  "a_uv");
    text_u_resolution = glGetUniformLocation(text_prog, "u_resolution");
    text_u_color      = glGetUniformLocation(text_prog, "u_color");
    text_u_tex        = glGetUniformLocation(text_prog, "u_tex");

    return 0;
}

void clue_font_renderer_shutdown(void)
{
    if (text_prog) {
        glDeleteProgram(text_prog);
        text_prog = 0;
    }
}

void clue_font_draw_text(int x, int y, const char *text,
                         UIFont *font, UIColor color,
                         int vp_w, int vp_h)
{
    if (!font || !text || !text_prog) return;

    glUseProgram(text_prog);
    glUniform2f(text_u_resolution, (float)vp_w, (float)vp_h);
    glUniform4f(text_u_color, color.r, color.g, color.b, color.a);
    glUniform1i(text_u_tex, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->atlas_tex);

    glEnableVertexAttribArray(text_a_pos);
    glEnableVertexAttribArray(text_a_uv);

    /* y is the top of the text; baseline = y + ascent */
    float pen_x = (float)x;
    float baseline = (float)y + (float)font->ascent;

    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        GlyphInfo *g = ensure_glyph(font, *p);
        if (!g) continue;

        if (g->bmp_w > 0 && g->bmp_h > 0) {
            float gx = pen_x + (float)g->bmp_left;
            float gy = baseline - (float)g->bmp_top;
            float gw = (float)g->bmp_w;
            float gh = (float)g->bmp_h;

            GLfloat verts[] = {
                gx,      gy,      g->u0, g->v0,
                gx + gw, gy,      g->u1, g->v0,
                gx,      gy + gh, g->u0, g->v1,
                gx + gw, gy,      g->u1, g->v0,
                gx + gw, gy + gh, g->u1, g->v1,
                gx,      gy + gh, g->u0, g->v1,
            };

            glVertexAttribPointer(text_a_pos, 2, GL_FLOAT, GL_FALSE,
                                  4 * sizeof(GLfloat), verts);
            glVertexAttribPointer(text_a_uv, 2, GL_FLOAT, GL_FALSE,
                                  4 * sizeof(GLfloat), verts + 2);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        pen_x += (float)g->advance_x;
    }

    glDisableVertexAttribArray(text_a_pos);
    glDisableVertexAttribArray(text_a_uv);
}
