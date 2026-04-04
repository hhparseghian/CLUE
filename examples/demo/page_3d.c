#include "demo.h"
#include <math.h>
#include <GLES2/gl2.h>

static float g_cube_rx = 0.4f, g_cube_ry = 0.6f;
static float g_cube_scale = 1.0f;
static bool g_cube_dragging = false;
static GLuint g_cube_prog = 0;
static GLint g_cube_u_mvp, g_cube_a_pos, g_cube_a_col;

static const float cube_data[] = {
    /* Front (red) */
    -1,-1, 1,  0.9f,0.3f,0.3f,   1,-1, 1,  0.9f,0.3f,0.3f,   1, 1, 1,  0.9f,0.3f,0.3f,
    -1,-1, 1,  0.9f,0.3f,0.3f,   1, 1, 1,  0.9f,0.3f,0.3f,  -1, 1, 1,  0.9f,0.3f,0.3f,
    /* Back (green) */
    -1,-1,-1,  0.3f,0.7f,0.3f,  -1, 1,-1,  0.3f,0.7f,0.3f,   1, 1,-1,  0.3f,0.7f,0.3f,
    -1,-1,-1,  0.3f,0.7f,0.3f,   1, 1,-1,  0.3f,0.7f,0.3f,   1,-1,-1,  0.3f,0.7f,0.3f,
    /* Top (blue) */
    -1, 1,-1,  0.3f,0.4f,0.9f,  -1, 1, 1,  0.3f,0.4f,0.9f,   1, 1, 1,  0.3f,0.4f,0.9f,
    -1, 1,-1,  0.3f,0.4f,0.9f,   1, 1, 1,  0.3f,0.4f,0.9f,   1, 1,-1,  0.3f,0.4f,0.9f,
    /* Bottom (yellow) */
    -1,-1,-1,  0.9f,0.85f,0.2f,  1,-1,-1,  0.9f,0.85f,0.2f,  1,-1, 1,  0.9f,0.85f,0.2f,
    -1,-1,-1,  0.9f,0.85f,0.2f,  1,-1, 1,  0.9f,0.85f,0.2f, -1,-1, 1,  0.9f,0.85f,0.2f,
    /* Right (orange) */
     1,-1,-1,  0.9f,0.5f,0.1f,   1, 1,-1,  0.9f,0.5f,0.1f,   1, 1, 1,  0.9f,0.5f,0.1f,
     1,-1,-1,  0.9f,0.5f,0.1f,   1, 1, 1,  0.9f,0.5f,0.1f,   1,-1, 1,  0.9f,0.5f,0.1f,
    /* Left (purple) */
    -1,-1,-1,  0.6f,0.3f,0.8f,  -1,-1, 1,  0.6f,0.3f,0.8f,  -1, 1, 1,  0.6f,0.3f,0.8f,
    -1,-1,-1,  0.6f,0.3f,0.8f,  -1, 1, 1,  0.6f,0.3f,0.8f,  -1, 1,-1,  0.6f,0.3f,0.8f,
};

static const char *cube_vert_src =
    "attribute vec3 a_pos;\n"
    "attribute vec3 a_col;\n"
    "varying vec3 v_col;\n"
    "uniform mat4 u_mvp;\n"
    "void main() {\n"
    "    gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
    "    v_col = a_col;\n"
    "}\n";

static const char *cube_frag_src =
    "precision mediump float;\n"
    "varying vec3 v_col;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(v_col, 1.0);\n"
    "}\n";

static void cube_init_gl(void)
{
    if (g_cube_prog) return;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &cube_vert_src, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &cube_frag_src, NULL);
    glCompileShader(fs);

    g_cube_prog = glCreateProgram();
    glAttachShader(g_cube_prog, vs);
    glAttachShader(g_cube_prog, fs);
    glLinkProgram(g_cube_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    g_cube_a_pos = glGetAttribLocation(g_cube_prog, "a_pos");
    g_cube_a_col = glGetAttribLocation(g_cube_prog, "a_col");
    g_cube_u_mvp = glGetUniformLocation(g_cube_prog, "u_mvp");
}

static void mat4_mul(float *out, const float *a, const float *b)
{
    float tmp[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float s = 0;
            for (int k = 0; k < 4; k++)
                s += a[r + k * 4] * b[k + c * 4];
            tmp[r + c * 4] = s;
        }
    memcpy(out, tmp, sizeof(tmp));
}

static void cube_build_mvp(float *mvp, float rx, float ry, float sc, float aspect)
{
    float f = 1.0f / tanf(30.0f * 3.14159265f / 180.0f);
    float n = 0.1f, fa = 100.0f;
    float proj[16] = {0};
    proj[0]  = f / aspect;
    proj[5]  = f;
    proj[10] = (fa + n) / (n - fa);
    proj[11] = -1.0f;
    proj[14] = (2.0f * fa * n) / (n - fa);

    float cy = cosf(ry), sy = sinf(ry);
    float roty[16] = {
         cy, 0, -sy, 0,
          0, 1,   0, 0,
         sy, 0,  cy, 0,
          0, 0,   0, 1,
    };

    float cx = cosf(rx), sx = sinf(rx);
    float rotx[16] = {
        1,  0,   0, 0,
        0, cx,  sx, 0,
        0, -sx, cx, 0,
        0,  0,   0, 1,
    };

    float scale[16] = {
        sc, 0,  0,  0,
        0,  sc, 0,  0,
        0,  0,  sc, 0,
        0,  0,  0,  1,
    };

    float trans[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, -5, 1,
    };

    float tmp1[16], tmp2[16], model[16];
    mat4_mul(tmp1, roty, scale);
    mat4_mul(tmp2, rotx, tmp1);
    mat4_mul(model, trans, tmp2);
    mat4_mul(mvp, proj, model);
}

static void cube_draw_cb(int x, int y, int w, int h, void *data)
{
    (void)data;
    cube_init_gl();
    if (!g_cube_prog) return;

    ClueApp *app = clue_app_get();
    int win_h = app && app->window ? app->window->h : h;

    glViewport(x, win_h - y - h, w, h);
    glScissor(x, win_h - y - h, w, h);
    glEnable(GL_SCISSOR_TEST);

    glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glUseProgram(g_cube_prog);

    float aspect = (float)w / (float)h;
    float mvp[16];
    cube_build_mvp(mvp, g_cube_rx, g_cube_ry, g_cube_scale, aspect);
    glUniformMatrix4fv(g_cube_u_mvp, 1, GL_FALSE, mvp);

    int stride = 6 * sizeof(float);
    glVertexAttribPointer(g_cube_a_pos, 3, GL_FLOAT, GL_FALSE, stride, cube_data);
    glEnableVertexAttribArray(g_cube_a_pos);
    glVertexAttribPointer(g_cube_a_col, 3, GL_FLOAT, GL_FALSE, stride, cube_data + 3);
    glEnableVertexAttribArray(g_cube_a_col);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDisableVertexAttribArray(g_cube_a_pos);
    glDisableVertexAttribArray(g_cube_a_col);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);

    clue_draw_text_default(x + 8, y + h - 20, "Drag to rotate, scroll to zoom",
                           UI_RGB(100, 100, 120));
}

static void cube_event_cb(const ClueCanvasEvent *ev, void *data)
{
    (void)data;
    if (ev->type == CLUE_CANVAS_PRESS && ev->button == 0)
        g_cube_dragging = true;
    if (ev->type == CLUE_CANVAS_RELEASE && ev->button == 0)
        g_cube_dragging = false;
    if (ev->type == CLUE_CANVAS_MOTION && g_cube_dragging) {
        g_cube_ry += ev->dx * 0.01f;
        g_cube_rx += ev->dy * 0.01f;
    }
    if (ev->type == CLUE_CANVAS_SCROLL) {
        g_cube_scale += ev->scroll_y * 0.1f;
        if (g_cube_scale < 0.3f) g_cube_scale = 0.3f;
        if (g_cube_scale > 3.0f) g_cube_scale = 3.0f;
    }
}

ClueBox *build_3d_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 8);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;

    ClueLabel *lbl = clue_label_new("3D Canvas (drag to rotate, scroll to zoom):");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);

    ClueCanvas *canvas = clue_canvas_new(500, 400);
    canvas->base.style.hexpand = true;
    canvas->base.style.vexpand = true;
    clue_canvas_set_draw(canvas, cube_draw_cb, NULL);
    clue_canvas_set_event(canvas, cube_event_cb, NULL);

    clue_container_add(page, lbl);
    clue_container_add(page, canvas);

    return page;
}
