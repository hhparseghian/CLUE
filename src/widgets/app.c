#define CLUE_IMPL
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "clue/app.h"
#include "clue/clue.h"
#include "clue/draw.h"
#include "clue/theme.h"
#include "clue/tooltip.h"
#include "clue/timer.h"
#include "clue/dropdown.h"
#include "clue/menu.h"
#include "clue/shortcut.h"
#include "clue/tabs.h"

/* Global app singleton */
static ClueApp *g_app = NULL;

/* Scan widget tree for open dropdowns/menus and draw their overlays.
 * Must traverse into tab pages which aren't normal children. */
static void draw_overlays(ClueWidget *w)
{
    if (!w || !w->base.visible) return;

    if (w->type_id == CLUE_WIDGET_DROPDOWN) {
        clue_dropdown_draw_overlay((ClueDropdown *)w);
    }

    if (w->type_id == CLUE_WIDGET_MENUBAR) {
        clue_menubar_draw_overlay((ClueMenuBar *)w);
    }

    /* Traverse into tab pages (stored separately from children) */
    if (w->type_id == CLUE_WIDGET_TABS) {
        ClueTabs *tabs = (ClueTabs *)w;
        int active = clue_tabs_get_active(tabs);
        if (active >= 0 && active < tabs->tab_count && tabs->tab_pages[active]) {
            draw_overlays(tabs->tab_pages[active]);
        }
    }

    for (int i = 0; i < w->base.child_count; i++) {
        draw_overlays((ClueWidget *)w->base.children[i]);
    }
}

/* Default font search paths */
static const char *font_paths[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    NULL,
};

static UIFont *load_default_font(int size)
{
    for (int i = 0; font_paths[i]; i++) {
        UIFont *f = clue_font_load(font_paths[i], size);
        if (f) return f;
    }
    fprintf(stderr, "clue-app: no default font found\n");
    return NULL;
}

ClueApp *clue_app_new(const char *title, int w, int h)
{
    if (g_app) {
        fprintf(stderr, "clue-app: app already exists\n");
        return g_app;
    }

    if (clue_init() != 0) {
        fprintf(stderr, "clue-app: clue_init failed\n");
        return NULL;
    }

    ClueApp *app = calloc(1, sizeof(ClueApp));
    if (!app) {
        clue_shutdown();
        return NULL;
    }

    app->window = clue_window_create(w, h, title);
    if (!app->window) {
        fprintf(stderr, "clue-app: failed to create window\n");
        free(app);
        clue_shutdown();
        return NULL;
    }

    app->renderer = clue_gl_renderer_create();
    if (!app->renderer) {
        fprintf(stderr, "clue-app: failed to create renderer\n");
        free(app);
        clue_shutdown();
        return NULL;
    }

    app->default_font = load_default_font(16);
    app->running = false;
    app->root = NULL;

    g_app = app;
    return app;
}

void clue_app_set_root(ClueApp *app, ClueWidget *root)
{
    if (!app) return;
    app->root = root;
    if (root) {
        app->window->widget_root = &root->base;
    }
}

void clue_app_run(ClueApp *app)
{
    if (!app) return;
    app->running = true;
    int mouse_x = 0, mouse_y = 0;

    while (app->running) {
        UIEvent events[32];
        int count = clue_poll_events(events, 32);

        for (int i = 0; i < count; i++) {
            if (events[i].type == UI_EVENT_CLOSE) {
                app->running = false;
                break;
            }
            if (events[i].type == UI_EVENT_MOUSE_MOVE) {
                mouse_x = events[i].mouse_move.x;
                mouse_y = events[i].mouse_move.y;
                /* Reset cursor before dispatch; widgets override if needed */
                clue_window_set_cursor(app->window, UI_CURSOR_DEFAULT);
            }
            /* Global keyboard shortcuts */
            if (events[i].type == UI_EVENT_KEY && events[i].key.pressed) {
                if (clue_shortcut_dispatch(events[i].key.keycode,
                                           events[i].key.modifiers))
                    continue;
            }

            /* If a widget has captured the mouse, send mouse events to it */
            if (app->captured_widget &&
                (events[i].type == UI_EVENT_MOUSE_MOVE ||
                 events[i].type == UI_EVENT_MOUSE_BUTTON ||
                 events[i].type == UI_EVENT_MOUSE_SCROLL)) {
                if (app->captured_widget->on_event)
                    app->captured_widget->on_event(app->captured_widget, &events[i]);
            } else if (app->modal_widget) {
                /* Modal mode: only the modal widget receives events */
                clue_widget_dispatch_event(&app->modal_widget->base, &events[i]);
            } else if (app->root) {
                int consumed = clue_widget_dispatch_event(&app->root->base, &events[i]);
                /* Click on empty space clears focus */
                if (!consumed &&
                    events[i].type == UI_EVENT_MOUSE_BUTTON &&
                    events[i].mouse_button.pressed) {
                    clue_focus_widget(NULL);
                }
            }
        }

        if (!app->running) break;

        /* Timers */
        clue_timer_tick();

        /* Layout */
        if (app->root) {
            app->root->base.x = 0;
            app->root->base.y = 0;
            app->root->base.w = app->window->w;
            app->root->base.h = app->window->h;
            clue_cwidget_layout_tree(app->root);
        }

        /* Draw */
        app->renderer->begin_frame(app->window);
        const ClueTheme *th = clue_theme_get();
        app->renderer->clear(th->bg.r, th->bg.g, th->bg.b, th->bg.a);

        if (app->root) {
            clue_cwidget_draw_tree(app->root);
        }

        /* Overlays (dropdowns, menus -- drawn on top of widgets) */
        if (app->root) {
            draw_overlays(app->root);
        }

        /* Tooltip (drawn last, on top of everything) */
        if (app->root) {
            clue_tooltip_update(app->root, mouse_x, mouse_y);
            clue_tooltip_draw(mouse_x, mouse_y);
        }

        app->renderer->end_frame(app->window);
        clue_window_swap_buffers(app->window);
    }
}

void clue_app_quit(ClueApp *app)
{
    if (app) app->running = false;
}

void clue_app_destroy(ClueApp *app)
{
    if (!app) return;

    clue_timer_cancel_all();

    if (app->root) {
        clue_cwidget_destroy(app->root);
        app->root = NULL;
    }

    if (app->default_font) {
        clue_font_destroy(app->default_font);
    }

    if (app->renderer) {
        clue_gl_renderer_destroy(app->renderer);
    }

    clue_shutdown();

    if (g_app == app) g_app = NULL;
    free(app);
}

ClueApp *clue_app_get(void)
{
    return g_app;
}

UIFont *clue_app_default_font(void)
{
    return g_app ? g_app->default_font : NULL;
}

void clue_capture_mouse(UIWidget *widget)
{
    if (g_app) g_app->captured_widget = widget;
}

void clue_release_mouse(void)
{
    if (g_app) g_app->captured_widget = NULL;
}

void clue_focus_widget(UIWidget *widget)
{
    if (!g_app) return;

    /* Clear old focus */
    if (g_app->focused_widget && g_app->focused_widget != widget) {
        g_app->focused_widget->focused = false;
    }

    /* Set new focus */
    if (widget) {
        widget->focused = true;
    }
    g_app->focused_widget = widget;
}
