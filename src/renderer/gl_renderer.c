/*
 * CLUE -- OpenGL ES 2.0 renderer
 *
 * Implements 2D drawing primitives using a single SDF (signed distance
 * field) fragment shader. All shapes are drawn as screen-aligned quads;
 * the fragment shader evaluates the SDF to produce filled or stroked
 * geometry with per-pixel anti-aliasing.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <GLES2/gl2.h>
#include "gl_renderer.h"
#include "font_internal.h"
#include "clue/window.h"
#include "third_party/stb_image.h"

/* ------------------------------------------------------------------ */
/* Shader sources                                                      */
/* ------------------------------------------------------------------ */

static const char *vert_src =
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

/*
 * Fragment shader: evaluates an SDF for rect, rounded rect, circle, line.
 *
 * Uniforms:
 *   u_color      -- RGBA colour
 *   u_quad       -- (w, h) of the rendered quad in pixels (includes padding)
 *   u_size       -- (w, h) of the actual shape in pixels (no padding)
 *   u_radius     -- corner radius (0 = sharp)
 *   u_thickness  -- stroke width (0 = filled)
 *   u_shape      -- 0 = rect/rounded rect, 1 = circle, 2 = line
 *   u_line_a/b   -- line endpoints in pixel space relative to quad centre
 */
static const char *frag_src =
    "#extension GL_OES_standard_derivatives : enable\n"
    "precision highp float;\n"
    "varying vec2 v_uv;\n"
    "uniform vec4  u_color;\n"
    "uniform vec2  u_quad;\n"
    "uniform vec2  u_size;\n"
    "uniform float u_radius;\n"
    "uniform float u_thickness;\n"
    "uniform float u_shape;\n"
    "uniform vec2  u_line_a;\n"
    "uniform vec2  u_line_b;\n"
    "\n"
    "float sdf_rounded_rect(vec2 p, vec2 half_size, float r) {\n"
    "    vec2 d = abs(p) - half_size + vec2(r);\n"
    "    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;\n"
    "}\n"
    "\n"
    "float sdf_circle(vec2 p, float r) {\n"
    "    return length(p) - r;\n"
    "}\n"
    "\n"
    "float sdf_line(vec2 p, vec2 a, vec2 b, float half_w) {\n"
    "    vec2 ba = b - a;\n"
    "    vec2 pa = p - a;\n"
    "    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);\n"
    "    return length(pa - ba * h) - half_w;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec2 p = (v_uv - 0.5) * u_quad;\n"
    "    float d;\n"
    "    if (u_shape > 1.5) {\n"
    "        d = sdf_line(p, u_line_a, u_line_b, u_thickness * 0.5);\n"
    "    } else if (u_shape > 0.5) {\n"
    "        d = sdf_circle(p, u_size.x * 0.5);\n"
    "    } else {\n"
    "        d = sdf_rounded_rect(p, u_size * 0.5, u_radius);\n"
    "    }\n"
    "    float aa = (u_shape > 0.5) ? max(fwidth(d), 0.5) : fwidth(d);\n"
    "    if (u_thickness > 0.0 && u_shape < 1.5) {\n"
    "        d = abs(d) - u_thickness * 0.5;\n"
    "    }\n"
    "    float alpha = 1.0 - smoothstep(-aa, aa, d);\n"
    "    gl_FragColor = u_color * alpha;\n"
    "}\n";

/* Line fragment shader -- uses v_uv.y as distance from centre for AA */
static const char *line_frag_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform vec4 u_color;\n"
    "void main() {\n"
    "    float d = abs(v_uv.y);\n"
    "    float alpha = 1.0 - smoothstep(0.5, 1.0, d);\n"
    "    gl_FragColor = u_color * alpha;\n"
    "}\n";

/* Image fragment shader -- samples texture, outputs RGBA directly */
static const char *img_frag_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(u_tex, v_uv);\n"
    "}\n";

/* Flat color vertex shader -- no UV needed */
static const char *flat_vert_src =
    "attribute vec2 a_pos;\n"
    "uniform   vec2 u_resolution;\n"
    "void main() {\n"
    "    vec2 ndc = (a_pos / u_resolution) * 2.0 - 1.0;\n"
    "    ndc.y = -ndc.y;\n"
    "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
    "}\n";

/* Flat color fragment shader -- solid color, no SDF, no AA */
static const char *flat_frag_src =
    "precision mediump float;\n"
    "uniform vec4 u_color;\n"
    "void main() {\n"
    "    gl_FragColor = u_color;\n"
    "}\n";

/* ------------------------------------------------------------------ */
/* GL state                                                            */
/* ------------------------------------------------------------------ */

static struct {
    GLuint prog;
    GLint  a_pos;
    GLint  a_uv;
    GLint  u_resolution;
    GLint  u_color;
    GLint  u_quad;
    GLint  u_size;
    GLint  u_radius;
    GLint  u_thickness;
    GLint  u_shape;
    GLint  u_line_a;
    GLint  u_line_b;
    /* Line shader */
    GLuint line_prog;
    GLint  line_a_pos;
    GLint  line_a_uv;
    GLint  line_u_resolution;
    GLint  line_u_color;
    /* Flat color shader (no SDF, no AA) */
    GLuint flat_prog;
    GLint  flat_a_pos;
    GLint  flat_u_resolution;
    GLint  flat_u_color;
    /* Image shader (textured quad, reuses line_frag but samples texture) */
    GLuint img_prog;
    GLint  img_a_pos;
    GLint  img_a_uv;
    GLint  img_u_resolution;
    GLint  img_u_tex;
    int    vp_w, vp_h;
} gl = {0};

/* ------------------------------------------------------------------ */
/* Shader compilation                                                  */
/* ------------------------------------------------------------------ */

static GLuint compile_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "clue-gl: shader compile error:\n%s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static int build_program(void)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (!vs || !fs) return -1;

    gl.prog = glCreateProgram();
    glAttachShader(gl.prog, vs);
    glAttachShader(gl.prog, fs);
    glLinkProgram(gl.prog);

    GLint ok;
    glGetProgramiv(gl.prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(gl.prog, sizeof(log), NULL, log);
        fprintf(stderr, "clue-gl: shader link error:\n%s\n", log);
        glDeleteProgram(gl.prog);
        gl.prog = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!gl.prog) return -1;

    gl.a_pos        = glGetAttribLocation(gl.prog,  "a_pos");
    gl.a_uv         = glGetAttribLocation(gl.prog,  "a_uv");
    gl.u_resolution = glGetUniformLocation(gl.prog, "u_resolution");
    gl.u_color      = glGetUniformLocation(gl.prog, "u_color");
    gl.u_quad       = glGetUniformLocation(gl.prog, "u_quad");
    gl.u_size       = glGetUniformLocation(gl.prog, "u_size");
    gl.u_radius     = glGetUniformLocation(gl.prog, "u_radius");
    gl.u_thickness  = glGetUniformLocation(gl.prog, "u_thickness");
    gl.u_shape      = glGetUniformLocation(gl.prog, "u_shape");
    gl.u_line_a     = glGetUniformLocation(gl.prog, "u_line_a");
    gl.u_line_b     = glGetUniformLocation(gl.prog, "u_line_b");

    /* --- Line shader --- */
    GLuint line_vs = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint line_fs = compile_shader(GL_FRAGMENT_SHADER, line_frag_src);
    if (!line_vs || !line_fs) return -1;

    gl.line_prog = glCreateProgram();
    glAttachShader(gl.line_prog, line_vs);
    glAttachShader(gl.line_prog, line_fs);
    glLinkProgram(gl.line_prog);

    glGetProgramiv(gl.line_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(gl.line_prog, sizeof(log), NULL, log);
        fprintf(stderr, "clue-gl: line shader link error:\n%s\n", log);
        glDeleteProgram(gl.line_prog);
        gl.line_prog = 0;
    }
    glDeleteShader(line_vs);
    glDeleteShader(line_fs);
    if (!gl.line_prog) return -1;

    gl.line_a_pos        = glGetAttribLocation(gl.line_prog,  "a_pos");
    gl.line_a_uv         = glGetAttribLocation(gl.line_prog,  "a_uv");
    gl.line_u_resolution = glGetUniformLocation(gl.line_prog, "u_resolution");
    gl.line_u_color      = glGetUniformLocation(gl.line_prog, "u_color");

    /* --- Image shader --- */
    GLuint img_vs = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint img_fs = compile_shader(GL_FRAGMENT_SHADER, img_frag_src);
    if (!img_vs || !img_fs) return -1;

    gl.img_prog = glCreateProgram();
    glAttachShader(gl.img_prog, img_vs);
    glAttachShader(gl.img_prog, img_fs);
    glLinkProgram(gl.img_prog);

    glGetProgramiv(gl.img_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(gl.img_prog, sizeof(log), NULL, log);
        fprintf(stderr, "clue-gl: image shader link error:\n%s\n", log);
        glDeleteProgram(gl.img_prog);
        gl.img_prog = 0;
    }
    glDeleteShader(img_vs);
    glDeleteShader(img_fs);
    if (!gl.img_prog) return -1;

    gl.img_a_pos        = glGetAttribLocation(gl.img_prog,  "a_pos");
    gl.img_a_uv         = glGetAttribLocation(gl.img_prog,  "a_uv");
    gl.img_u_resolution = glGetUniformLocation(gl.img_prog, "u_resolution");
    gl.img_u_tex        = glGetUniformLocation(gl.img_prog, "u_tex");

    /* --- Flat color shader --- */
    GLuint flat_vs2 = compile_shader(GL_VERTEX_SHADER, flat_vert_src);
    GLuint flat_fs2 = compile_shader(GL_FRAGMENT_SHADER, flat_frag_src);
    if (!flat_vs2 || !flat_fs2) return -1;

    gl.flat_prog = glCreateProgram();
    glAttachShader(gl.flat_prog, flat_vs2);
    glAttachShader(gl.flat_prog, flat_fs2);
    glLinkProgram(gl.flat_prog);

    glGetProgramiv(gl.flat_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(gl.flat_prog, sizeof(log), NULL, log);
        fprintf(stderr, "clue-gl: flat shader link error:\n%s\n", log);
        glDeleteProgram(gl.flat_prog);
        gl.flat_prog = 0;
    }
    glDeleteShader(flat_vs2);
    glDeleteShader(flat_fs2);
    if (!gl.flat_prog) return -1;

    gl.flat_a_pos        = glGetAttribLocation(gl.flat_prog,  "a_pos");
    gl.flat_u_resolution = glGetUniformLocation(gl.flat_prog, "u_resolution");
    gl.flat_u_color      = glGetUniformLocation(gl.flat_prog, "u_color");

    return 0;
}

/* ------------------------------------------------------------------ */
/* Internal: emit a quad                                               */
/* ------------------------------------------------------------------ */

static void emit_quad(float x, float y, float w, float h)
{
    float x1 = x + w;
    float y1 = y + h;

    GLfloat verts[] = {
        x,  y,   0.0f, 0.0f,
        x1, y,   1.0f, 0.0f,
        x,  y1,  0.0f, 1.0f,
        x1, y,   1.0f, 0.0f,
        x1, y1,  1.0f, 1.0f,
        x,  y1,  0.0f, 1.0f,
    };

    glVertexAttribPointer(gl.a_pos, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), verts);
    glEnableVertexAttribArray(gl.a_pos);

    glVertexAttribPointer(gl.a_uv, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), verts + 2);
    glEnableVertexAttribArray(gl.a_uv);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

/* Set up common uniforms and emit a quad for rect/circle shapes.
 * The quad is padded for AA; u_size holds the true shape dimensions. */
static void draw_shape(int x, int y, int w, int h,
                       float radius, float thickness,
                       int shape, UIColor color)
{
    if (!gl.prog) return;

    float pad = (thickness > 0.0f ? thickness * 0.5f : 0.0f) + 2.5f;
    float fx = (float)x - pad;
    float fy = (float)y - pad;
    float fw = (float)w + pad * 2.0f;
    float fh = (float)h + pad * 2.0f;

    glUseProgram(gl.prog);
    glUniform2f(gl.u_resolution, (float)gl.vp_w, (float)gl.vp_h);
    glUniform4f(gl.u_color, color.r, color.g, color.b, color.a);
    glUniform2f(gl.u_quad, fw, fh);
    glUniform2f(gl.u_size, (float)w, (float)h);
    glUniform1f(gl.u_radius, radius);
    glUniform1f(gl.u_thickness, thickness);
    glUniform1f(gl.u_shape, (float)shape);
    glUniform2f(gl.u_line_a, 0.0f, 0.0f);
    glUniform2f(gl.u_line_b, 0.0f, 0.0f);

    emit_quad(fx, fy, fw, fh);
}

/* ------------------------------------------------------------------ */
/* Drawing primitives                                                  */
/* ------------------------------------------------------------------ */

static void gl_fill_rect(int x, int y, int w, int h, UIColor color)
{
    if (!gl.flat_prog) return;
    glUseProgram(gl.flat_prog);
    glUniform2f(gl.flat_u_resolution, (float)gl.vp_w, (float)gl.vp_h);
    glUniform4f(gl.flat_u_color, color.r, color.g, color.b, color.a);

    float x0 = (float)x, y0 = (float)y;
    float x1 = x0 + (float)w, y1 = y0 + (float)h;
    GLfloat verts[] = {
        x0, y0,  x1, y0,  x0, y1,
        x1, y0,  x1, y1,  x0, y1,
    };
    glVertexAttribPointer(gl.flat_a_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(gl.flat_a_pos);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void gl_fill_rounded_rect(int x, int y, int w, int h,
                                 float radius, UIColor color)
{
    draw_shape(x, y, w, h, radius, 0.0f, 0, color);
}

static void gl_draw_rect(int x, int y, int w, int h,
                         float thickness, UIColor color)
{
    draw_shape(x, y, w, h, 0.0f, thickness, 0, color);
}

static void gl_draw_rounded_rect(int x, int y, int w, int h,
                                 float radius, float thickness, UIColor color)
{
    draw_shape(x, y, w, h, radius, thickness, 0, color);
}

static void gl_fill_circle(int cx, int cy, int radius, UIColor color)
{
    if (!gl.flat_prog || radius <= 0) return;

    /* Draw as a triangle fan using the flat shader — no SDF AA artifacts */
    #define CIRCLE_SEGS 32
    GLfloat verts[(CIRCLE_SEGS + 2) * 2];
    float fcx = (float)cx, fcy = (float)cy, fr = (float)radius;

    /* Center vertex */
    verts[0] = fcx;
    verts[1] = fcy;

    for (int i = 0; i <= CIRCLE_SEGS; i++) {
        float angle = (float)i / (float)CIRCLE_SEGS * 6.2831853f;
        verts[(i + 1) * 2 + 0] = fcx + fr * cosf(angle);
        verts[(i + 1) * 2 + 1] = fcy + fr * sinf(angle);
    }

    glUseProgram(gl.flat_prog);
    glUniform2f(gl.flat_u_resolution, (float)gl.vp_w, (float)gl.vp_h);
    glUniform4f(gl.flat_u_color, color.r, color.g, color.b, color.a);
    glVertexAttribPointer(gl.flat_a_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(gl.flat_a_pos);
    glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGS + 2);
    #undef CIRCLE_SEGS
}

static void gl_draw_circle(int cx, int cy, int radius,
                           float thickness, UIColor color)
{
    int d = radius * 2;
    draw_shape(cx - radius, cy - radius, d, d, 0.0f, thickness, 1, color);
}

static void gl_draw_line(int x0, int y0, int x1, int y1,
                         float thickness, UIColor color)
{
    if (!gl.line_prog) return;

    float dx = (float)(x1 - x0);
    float dy = (float)(y1 - y0);
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;

    /* Perpendicular direction scaled to half-thickness + 1px AA margin */
    float aa = 1.0f;
    float half = thickness * 0.5f + aa;
    float px = (-dy / len) * half;
    float py = ( dx / len) * half;

    float ax = (float)x0, ay = (float)y0;
    float bx = (float)x1, by = (float)y1;

    /* UV.y: -1 at outer edge, 0 at centre, +1 at outer edge
     * The shader fades alpha near |y| = 1 for smooth edges */
    GLfloat verts[] = {
    /*  pos.x       pos.y       uv.x  uv.y */
        ax + px,    ay + py,    0.0f, -1.0f,
        ax - px,    ay - py,    0.0f,  1.0f,
        bx + px,    by + py,    1.0f, -1.0f,
        ax - px,    ay - py,    0.0f,  1.0f,
        bx - px,    by - py,    1.0f,  1.0f,
        bx + px,    by + py,    1.0f, -1.0f,
    };

    glUseProgram(gl.line_prog);
    glUniform2f(gl.line_u_resolution, (float)gl.vp_w, (float)gl.vp_h);
    glUniform4f(gl.line_u_color, color.r, color.g, color.b, color.a);

    glVertexAttribPointer(gl.line_a_pos, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), verts);
    glEnableVertexAttribArray(gl.line_a_pos);

    glVertexAttribPointer(gl.line_a_uv, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), verts + 2);
    glEnableVertexAttribArray(gl.line_a_uv);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(gl.line_a_pos);
    glDisableVertexAttribArray(gl.line_a_uv);
}

/* ------------------------------------------------------------------ */
/* Images                                                              */
/* ------------------------------------------------------------------ */

static void gl_draw_image(UITexture tex, int x, int y, int w, int h)
{
    if (!gl.img_prog || !tex) return;

    float fx = (float)x, fy = (float)y;
    float fw = (float)w, fh = (float)h;
    float x1 = fx + fw, y1 = fy + fh;

    GLfloat verts[] = {
        fx, fy, 0.0f, 0.0f,
        x1, fy, 1.0f, 0.0f,
        fx, y1, 0.0f, 1.0f,
        x1, fy, 1.0f, 0.0f,
        x1, y1, 1.0f, 1.0f,
        fx, y1, 0.0f, 1.0f,
    };

    glUseProgram(gl.img_prog);
    glUniform2f(gl.img_u_resolution, (float)gl.vp_w, (float)gl.vp_h);
    glUniform1i(gl.img_u_tex, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex);

    glVertexAttribPointer(gl.img_a_pos, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), verts);
    glEnableVertexAttribArray(gl.img_a_pos);
    glVertexAttribPointer(gl.img_a_uv, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), verts + 2);
    glEnableVertexAttribArray(gl.img_a_uv);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(gl.img_a_pos);
    glDisableVertexAttribArray(gl.img_a_uv);
}

/* ------------------------------------------------------------------ */
/* Clipping                                                            */
/* ------------------------------------------------------------------ */

static void gl_set_clip_rect(int x, int y, int w, int h)
{
    glEnable(GL_SCISSOR_TEST);
    /* GL scissor origin is bottom-left, convert from top-left */
    glScissor(x, gl.vp_h - y - h, w, h);
}

static void gl_reset_clip_rect(void)
{
    glDisable(GL_SCISSOR_TEST);
}

/* ------------------------------------------------------------------ */
/* Texture loading                                                     */
/* ------------------------------------------------------------------ */

UITexture clue_texture_load(const char *path)
{
    int w, h, channels;
    unsigned char *data = stbi_load(path, &w, &h, &channels, 4);
    if (!data) {
        fprintf(stderr, "clue-gl: failed to load image: %s\n", path);
        return 0;
    }

    UITexture tex = clue_texture_load_rgba(data, w, h);
    stbi_image_free(data);
    return tex;
}

UITexture clue_texture_load_rgba(const unsigned char *pixels, int w, int h)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    return (UITexture)tex;
}

void clue_texture_destroy(UITexture tex)
{
    GLuint t = (GLuint)tex;
    if (t) glDeleteTextures(1, &t);
}

/* ------------------------------------------------------------------ */
/* Text                                                                */
/* ------------------------------------------------------------------ */

static void gl_draw_text(int x, int y, const char *text,
                         struct UIFont *font, UIColor color)
{
    clue_font_draw_text(x, y, text, font, color, gl.vp_w, gl.vp_h);
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

static int gl_init(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (build_program() != 0) return -1;
    return clue_font_renderer_init();
}

static void gl_shutdown(void)
{
    clue_font_renderer_shutdown();
    if (gl.prog)      { glDeleteProgram(gl.prog);      gl.prog = 0; }
    if (gl.line_prog) { glDeleteProgram(gl.line_prog); gl.line_prog = 0; }
    if (gl.img_prog)  { glDeleteProgram(gl.img_prog);  gl.img_prog = 0; }
}

static void gl_begin_frame(struct UIWindow *win)
{
    if (!win) return;
    gl.vp_w = win->w;
    gl.vp_h = win->h;
    glViewport(0, 0, win->w, win->h);
}

static void gl_end_frame(struct UIWindow *win)
{
    (void)win;
    glFlush();
}

static void gl_clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

static UIRenderer g_gl_renderer = {
    .init              = gl_init,
    .shutdown          = gl_shutdown,
    .begin_frame       = gl_begin_frame,
    .end_frame         = gl_end_frame,
    .clear             = gl_clear,
    .fill_rect         = gl_fill_rect,
    .fill_rounded_rect = gl_fill_rounded_rect,
    .draw_rect         = gl_draw_rect,
    .draw_rounded_rect = gl_draw_rounded_rect,
    .draw_line         = gl_draw_line,
    .fill_circle       = gl_fill_circle,
    .draw_circle       = gl_draw_circle,
    .draw_text         = gl_draw_text,
    .draw_image        = gl_draw_image,
    .set_clip_rect     = gl_set_clip_rect,
    .reset_clip_rect   = gl_reset_clip_rect,
};

UIRenderer *clue_gl_renderer_create(void)
{
    if (g_gl_renderer.init) {
        if (g_gl_renderer.init() != 0) {
            fprintf(stderr, "clue-gl: renderer init failed\n");
            return NULL;
        }
    }
    return &g_gl_renderer;
}

void clue_gl_renderer_destroy(UIRenderer *renderer)
{
    if (renderer && renderer->shutdown) {
        renderer->shutdown();
    }
}
