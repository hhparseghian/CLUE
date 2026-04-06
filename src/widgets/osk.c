#define CLUE_IMPL
#include <stdlib.h>
#include <string.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include "clue/osk.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/draw.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/button.h"
#include "clue/box.h"
#include "clue/grid.h"
#include "clue/clue_widget.h"
#include "clue/text_input.h"
#include "clue/event.h"

/* ------------------------------------------------------------------ */
/* Key definition                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *label;       /* display label (lowercase) */
    const char *shift_label; /* display label when shifted */
    int         keycode;     /* XKB keysym (0 = use text) */
    const char *text;        /* text to inject (lowercase) */
    const char *shift_text;  /* text to inject (shifted) */
    int         width;       /* key width multiplier (1 = normal) */
} OskKeyDef;

/* ------------------------------------------------------------------ */
/* QWERTY layout                                                       */
/* ------------------------------------------------------------------ */

static const OskKeyDef qwerty_row0[] = {
    {"q","Q", 0,"q","Q",1}, {"w","W", 0,"w","W",1}, {"e","E", 0,"e","E",1},
    {"r","R", 0,"r","R",1}, {"t","T", 0,"t","T",1}, {"y","Y", 0,"y","Y",1},
    {"u","U", 0,"u","U",1}, {"i","I", 0,"i","I",1}, {"o","O", 0,"o","O",1},
    {"p","P", 0,"p","P",1},
};

static const OskKeyDef qwerty_row1[] = {
    {"a","A", 0,"a","A",1}, {"s","S", 0,"s","S",1}, {"d","D", 0,"d","D",1},
    {"f","F", 0,"f","F",1}, {"g","G", 0,"g","G",1}, {"h","H", 0,"h","H",1},
    {"j","J", 0,"j","J",1}, {"k","K", 0,"k","K",1}, {"l","L", 0,"l","L",1},
};

static const OskKeyDef qwerty_row2[] = {
    {"Shift",  "Shift",   XKB_KEY_Shift_L, NULL, NULL, 2},
    {"z","Z", 0,"z","Z",1}, {"x","X", 0,"x","X",1}, {"c","C", 0,"c","C",1},
    {"v","V", 0,"v","V",1}, {"b","B", 0,"b","B",1}, {"n","N", 0,"n","N",1},
    {"m","M", 0,"m","M",1},
    {"Bksp",   "Bksp",    XKB_KEY_BackSpace, NULL, NULL, 2},
};

static const OskKeyDef qwerty_row3[] = {
    {"123",   "123",   0, NULL, NULL, 2},
    {"Space", "Space", XKB_KEY_space, " ", " ", 5},
    {"Enter", "Enter", XKB_KEY_Return, NULL, NULL, 2},
    {"Done",  "Done",  0, NULL, NULL, 2},
};

/* Row descriptors */
typedef struct {
    const OskKeyDef *keys;
    int count;
} OskRow;

static const OskRow qwerty_rows[] = {
    { qwerty_row0, sizeof(qwerty_row0) / sizeof(qwerty_row0[0]) },
    { qwerty_row1, sizeof(qwerty_row1) / sizeof(qwerty_row1[0]) },
    { qwerty_row2, sizeof(qwerty_row2) / sizeof(qwerty_row2[0]) },
    { qwerty_row3, sizeof(qwerty_row3) / sizeof(qwerty_row3[0]) },
};
#define QWERTY_ROW_COUNT 4

/* Number/symbol layout (toggled from QWERTY "123" key) */
static const OskKeyDef num_sym_row0[] = {
    {"1","!", 0,"1","!",1}, {"2","@", 0,"2","@",1}, {"3","#", 0,"3","#",1},
    {"4","$", 0,"4","$",1}, {"5","%", 0,"5","%",1}, {"6","^", 0,"6","^",1},
    {"7","&", 0,"7","&",1}, {"8","*", 0,"8","*",1}, {"9","(", 0,"9","(",1},
    {"0",")", 0,"0",")",1},
};

static const OskKeyDef num_sym_row1[] = {
    {"-","_", 0,"-","_",1}, {"=","+", 0,"=","+",1}, {"[","{", 0,"[","{",1},
    {"]","}", 0,"]","}",1}, {"\\","|", 0,"\\","|",1}, {";",":", 0,";",":",1},
    {"'","\"", 0,"'","\"",1}, {",","<", 0,",","<",1}, {".",">", 0,".",">",1},
};

static const OskKeyDef num_sym_row2[] = {
    {"Shift",  "Shift",   XKB_KEY_Shift_L, NULL, NULL, 2},
    {"/","?", 0,"/","?",1}, {"`","~", 0,"`","~",1},
    {"@","@", 0,"@","@",1}, {"!","!", 0,"!","!",1},
    {"Bksp",   "Bksp",    XKB_KEY_BackSpace, NULL, NULL, 2},
};

static const OskKeyDef num_sym_row3[] = {
    {"abc",   "abc",   0, NULL, NULL, 2},
    {"Space", "Space", XKB_KEY_space, " ", " ", 5},
    {"Enter", "Enter", XKB_KEY_Return, NULL, NULL, 2},
    {"Done",  "Done",  0, NULL, NULL, 2},
};

static const OskRow num_sym_rows[] = {
    { num_sym_row0, sizeof(num_sym_row0) / sizeof(num_sym_row0[0]) },
    { num_sym_row1, sizeof(num_sym_row1) / sizeof(num_sym_row1[0]) },
    { num_sym_row2, sizeof(num_sym_row2) / sizeof(num_sym_row2[0]) },
    { num_sym_row3, sizeof(num_sym_row3) / sizeof(num_sym_row3[0]) },
};

/* ------------------------------------------------------------------ */
/* Numpad layout                                                       */
/* ------------------------------------------------------------------ */

static const OskKeyDef numpad_row0[] = {
    {"7","7", 0,"7","7",1}, {"8","8", 0,"8","8",1}, {"9","9", 0,"9","9",1},
};
static const OskKeyDef numpad_row1[] = {
    {"4","4", 0,"4","4",1}, {"5","5", 0,"5","5",1}, {"6","6", 0,"6","6",1},
};
static const OskKeyDef numpad_row2[] = {
    {"1","1", 0,"1","1",1}, {"2","2", 0,"2","2",1}, {"3","3", 0,"3","3",1},
};
static const OskKeyDef numpad_row3[] = {
    {"-","-", 0,"-","-",1}, {"0","0", 0,"0","0",1}, {".",".", 0,".",".",1},
};
static const OskKeyDef numpad_row4[] = {
    {"Bksp", "Bksp", XKB_KEY_BackSpace, NULL, NULL, 1},
    {"Enter","Enter", XKB_KEY_Return, NULL, NULL, 1},
    {"Done", "Done",  0, NULL, NULL, 1},
};

static const OskRow numpad_rows[] = {
    { numpad_row0, 3 },
    { numpad_row1, 3 },
    { numpad_row2, 3 },
    { numpad_row3, 3 },
    { numpad_row4, 3 },
};
#define NUMPAD_ROW_COUNT 5

/* ------------------------------------------------------------------ */
/* OSK state                                                           */
/* ------------------------------------------------------------------ */

#define OSK_MAX_BUTTONS 64

typedef struct {
    ClueWidget        base;          /* MUST be first -- modal widget */
    ClueOskType       type;
    bool              visible;
    bool              shifted;
    bool              num_sym_mode;  /* QWERTY toggled to 123/symbol layout */

    /* Key buttons */
    ClueButton       *buttons[OSK_MAX_BUTTONS];
    const OskKeyDef  *key_defs[OSK_MAX_BUTTONS];
    int               button_count;

    /* Layout */
    int               panel_w;
    int               panel_h;
    int               panel_x;
    int               panel_y;

    /* Saved focus -- restored after dismiss */
    UIWidget         *saved_focus;

    /* Auto-show */
    bool              auto_enabled;
    ClueOskType       auto_type;
    bool              handling_event; /* true while dispatching to buttons */
} ClueOsk;

static ClueOsk *g_osk = NULL;

/* ------------------------------------------------------------------ */
/* Forward declarations                                                */
/* ------------------------------------------------------------------ */

static void osk_rebuild_keys(ClueOsk *osk);
static void osk_update_labels(ClueOsk *osk);

/* ------------------------------------------------------------------ */
/* Inject input into focused widget                                    */
/* ------------------------------------------------------------------ */

static void inject_key(int keycode, const char *text, int modifiers)
{
    ClueApp *app = clue_app_get();
    if (!app || !app->focused_widget) return;

    ClueEvent ev = {0};
    ev.type = CLUE_EVENT_KEY;
    ev.window = app->window;
    ev.key.keycode = keycode;
    ev.key.pressed = 1;
    ev.key.modifiers = modifiers;
    if (text) {
        strncpy(ev.key.text, text, sizeof(ev.key.text) - 1);
    }

    /* Dispatch to focused widget */
    if (app->focused_widget->on_event)
        app->focused_widget->on_event(app->focused_widget, &ev);
}

/* ------------------------------------------------------------------ */
/* Button click handler                                                */
/* ------------------------------------------------------------------ */

static void on_key_clicked(void *widget, void *user_data)
{
    ClueOsk *osk = g_osk;
    if (!osk) return;

    /* Find which key was pressed */
    const OskKeyDef *def = NULL;
    for (int i = 0; i < osk->button_count; i++) {
        if ((void *)osk->buttons[i] == widget) {
            def = osk->key_defs[i];
            break;
        }
    }
    if (!def) return;

    /* Special keys */
    if (def->keycode == XKB_KEY_Shift_L) {
        osk->shifted = !osk->shifted;
        osk_update_labels(osk);
        return;
    }

    /* "123" / "abc" toggle (QWERTY only) */
    if (def->text == NULL && def->keycode == 0) {
        if (strcmp(def->label, "123") == 0 || strcmp(def->label, "abc") == 0) {
            osk->num_sym_mode = !osk->num_sym_mode;
            osk->shifted = false;
            osk_rebuild_keys(osk);
            return;
        }
        if (strcmp(def->label, "Done") == 0) {
            clue_osk_hide();
            return;
        }
    }

    /* Actionable keys */
    if (def->keycode == XKB_KEY_BackSpace) {
        inject_key(XKB_KEY_BackSpace, NULL, 0);
    } else if (def->keycode == XKB_KEY_Return) {
        inject_key(XKB_KEY_Return, NULL, 0);
    } else if (def->keycode == XKB_KEY_space) {
        inject_key(XKB_KEY_space, " ", 0);
    } else if (def->text) {
        const char *txt = osk->shifted ? def->shift_text : def->text;
        inject_key(0, txt, osk->shifted ? CLUE_MOD_SHIFT : 0);
        /* Auto-unshift after typing a character */
        if (osk->shifted) {
            osk->shifted = false;
            osk_update_labels(osk);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Build / rebuild key buttons                                         */
/* ------------------------------------------------------------------ */

static void osk_clear_keys(ClueOsk *osk)
{
    for (int i = 0; i < osk->button_count; i++) {
        clue_cwidget_destroy((ClueWidget *)osk->buttons[i]);
        osk->buttons[i] = NULL;
        osk->key_defs[i] = NULL;
    }
    osk->button_count = 0;
}

static void osk_add_key(ClueOsk *osk, const OskKeyDef *def)
{
    if (osk->button_count >= OSK_MAX_BUTTONS) return;

    const char *label = osk->shifted ? def->shift_label : def->label;
    ClueButton *btn = clue_button_new(label);
    if (!btn) return;

    btn->base.base.focusable = false;
    clue_signal_connect(btn, "clicked", on_key_clicked, osk);

    int idx = osk->button_count;
    osk->buttons[idx] = btn;
    osk->key_defs[idx] = def;
    osk->button_count++;
}

static void osk_rebuild_keys(ClueOsk *osk)
{
    osk_clear_keys(osk);

    if (osk->type == CLUE_OSK_QWERTY) {
        const OskRow *rows = osk->num_sym_mode ? num_sym_rows : qwerty_rows;
        for (int r = 0; r < QWERTY_ROW_COUNT; r++) {
            for (int k = 0; k < rows[r].count; k++) {
                osk_add_key(osk, &rows[r].keys[k]);
            }
        }
    } else {
        for (int r = 0; r < NUMPAD_ROW_COUNT; r++) {
            for (int k = 0; k < numpad_rows[r].count; k++) {
                osk_add_key(osk, &numpad_rows[r].keys[k]);
            }
        }
    }
}

static void osk_update_labels(ClueOsk *osk)
{
    for (int i = 0; i < osk->button_count; i++) {
        const OskKeyDef *def = osk->key_defs[i];
        const char *label = osk->shifted ? def->shift_label : def->label;
        clue_button_set_label(osk->buttons[i], label);
    }
}

/* ------------------------------------------------------------------ */
/* Draw                                                                */
/* ------------------------------------------------------------------ */

#define OSK_KEY_PAD   4
#define OSK_PAD       8
#define OSK_PREVIEW_H 32

static void osk_draw(ClueWidget *w)
{
    ClueOsk *osk = (ClueOsk *)w;
    if (!osk->visible) return;

    ClueApp *app = clue_app_get();
    if (!app) return;

    const ClueTheme *th = clue_theme_get();
    ClueFont *font = clue_app_default_font();

    int win_w = app->window->w;
    int win_h = app->window->h;

    /* Check if focused widget is a text input (for preview) */
    const char *preview_text = NULL;
    if (app->focused_widget) {
        ClueWidget *fw = (ClueWidget *)app->focused_widget;
        if (fw->type_id == CLUE_WIDGET_TEXT_INPUT) {
            preview_text = ((ClueTextInput *)fw)->text;
        }
    }
    bool has_preview = (preview_text != NULL && osk->type == CLUE_OSK_QWERTY);
    int preview_extra = has_preview ? OSK_PREVIEW_H + OSK_KEY_PAD : 0;

    /* Calculate panel geometry */
    if (osk->type == CLUE_OSK_QWERTY) {
        osk->panel_w = win_w;
        osk->panel_h = win_h * 2 / 5;
        if (osk->panel_h < 180) osk->panel_h = 180;
        osk->panel_h += preview_extra;
        osk->panel_x = 0;
        osk->panel_y = win_h - osk->panel_h;
    } else {
        osk->panel_w = 240;
        osk->panel_h = 280;
        osk->panel_x = (win_w - osk->panel_w) / 2;
        osk->panel_y = win_h - osk->panel_h - 20;
    }

    /* Dim area above keyboard */
    clue_fill_rect(0, 0, win_w, osk->panel_y, CLUE_RGBA(0, 0, 0, 80));

    /* Panel background */
    clue_fill_rounded_rect(osk->panel_x, osk->panel_y,
                           osk->panel_w, osk->panel_h,
                           6.0f, th->surface);
    clue_draw_rounded_rect(osk->panel_x, osk->panel_y,
                           osk->panel_w, osk->panel_h,
                           6.0f, 1.0f, th->surface_border);

    /* Text preview bar */
    int px = osk->panel_x + OSK_PAD;
    int py = osk->panel_y + OSK_PAD;

    if (has_preview && font) {
        int prev_x = px;
        int prev_y = py;
        int prev_w = osk->panel_w - OSK_PAD * 2;

        /* Preview background */
        clue_fill_rounded_rect(prev_x, prev_y, prev_w, OSK_PREVIEW_H,
                               4.0f, th->input.bg);
        clue_draw_rounded_rect(prev_x, prev_y, prev_w, OSK_PREVIEW_H,
                               4.0f, 1.0f, th->surface_border);

        /* Preview text */
        int text_y = prev_y + (OSK_PREVIEW_H - clue_font_line_height(font)) / 2;
        clue_set_clip_rect(prev_x + 6, prev_y, prev_w - 12, OSK_PREVIEW_H);
        const char *display = (preview_text[0]) ? preview_text : "...";
        ClueColor fg = preview_text[0] ? th->fg : th->fg_dim;
        clue_draw_text(prev_x + 8, text_y, display, font, fg);
        clue_reset_clip_rect();

        py += OSK_PREVIEW_H + OSK_KEY_PAD;
    }

    /* Layout and draw keys */
    int avail_w = osk->panel_w - OSK_PAD * 2;
    int btn_idx = 0;

    const OskRow *rows;
    int row_count;

    if (osk->type == CLUE_OSK_QWERTY) {
        rows = osk->num_sym_mode ? num_sym_rows : qwerty_rows;
        row_count = QWERTY_ROW_COUNT;
    } else {
        rows = numpad_rows;
        row_count = NUMPAD_ROW_COUNT;
    }

    int avail_h = osk->panel_h - OSK_PAD * 2 - preview_extra;
    int row_h = (avail_h - OSK_KEY_PAD * (row_count - 1)) / row_count;
    if (row_h < 20) row_h = 20;

    for (int r = 0; r < row_count; r++) {
        /* Calculate total width units for this row */
        int total_units = 0;
        for (int k = 0; k < rows[r].count; k++)
            total_units += rows[r].keys[k].width;

        int unit_w = (avail_w - OSK_KEY_PAD * (rows[r].count - 1)) / total_units;
        int kx = px;

        for (int k = 0; k < rows[r].count; k++) {
            if (btn_idx >= osk->button_count) break;

            int key_w = unit_w * rows[r].keys[k].width;
            /* Give leftover pixels to the last key in the row */
            if (k == rows[r].count - 1)
                key_w = px + avail_w - kx;

            ClueButton *btn = osk->buttons[btn_idx];
            btn->base.base.x = kx;
            btn->base.base.y = py;
            btn->base.base.w = key_w;
            btn->base.base.h = row_h;
            btn->base.base.visible = true;

            clue_cwidget_draw_tree((ClueWidget *)btn);

            kx += key_w + OSK_KEY_PAD;
            btn_idx++;
        }

        py += row_h + OSK_KEY_PAD;
    }
}

/* ------------------------------------------------------------------ */
/* Event handling                                                      */
/* ------------------------------------------------------------------ */

static int osk_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueOsk *osk = (ClueOsk *)w;
    if (!osk->visible) return 0;

    /* Route mouse events to key buttons.
     * Set handling_event so focus changes from button clicks are ignored,
     * and restore focus afterwards since buttons clear it. */
    ClueApp *eapp = clue_app_get();
    UIWidget *prev_focus = eapp ? eapp->focused_widget : NULL;
    osk->handling_event = true;
    for (int i = 0; i < osk->button_count; i++) {
        if (clue_widget_dispatch_event(&osk->buttons[i]->base.base, event)) {
            osk->handling_event = false;
            /* Restore focus that the button cleared, but only if OSK is still visible
             * (Hide/Enter may have dismissed it) */
            if (osk->visible && eapp && prev_focus)
                clue_focus_widget(prev_focus);
            return 1;
        }
    }
    osk->handling_event = false;

    /* Check if event is in the panel area -- consume it */
    int mx = 0, my = 0;
    if (event->type == CLUE_EVENT_MOUSE_BUTTON) {
        mx = event->mouse_button.x;
        my = event->mouse_button.y;
    } else if (event->type == CLUE_EVENT_MOUSE_MOVE) {
        mx = event->mouse_move.x;
        my = event->mouse_move.y;
    }

    if (my >= osk->panel_y) return 1;

    /* Pass events above the keyboard through to the app widget tree */
    ClueApp *app = clue_app_get();
    if (app && app->root)
        return clue_widget_dispatch_event(&app->root->base, event);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Layout / destroy                                                    */
/* ------------------------------------------------------------------ */

static void osk_layout(ClueWidget *w)
{
    /* Panel geometry is calculated in draw */
    (void)w;
}

static void osk_destroy(ClueWidget *w)
{
    ClueOsk *osk = (ClueOsk *)w;
    osk_clear_keys(osk);
}

static const ClueWidgetVTable osk_vtable = {
    .draw         = osk_draw,
    .layout       = osk_layout,
    .handle_event = osk_handle_event,
    .destroy      = osk_destroy,
};

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void clue_osk_show(ClueOskType type)
{
    ClueApp *app = clue_app_get();
    if (!app) return;

    /* If already showing, just switch type */
    if (g_osk && g_osk->visible) {
        if (g_osk->type == type) return;
        g_osk->type = type;
        g_osk->shifted = false;
        g_osk->num_sym_mode = false;
        osk_rebuild_keys(g_osk);
        return;
    }

    if (!g_osk) {
        g_osk = calloc(1, sizeof(ClueOsk));
        if (!g_osk) return;
        clue_cwidget_init(&g_osk->base, &osk_vtable);
    }

    g_osk->type = type;
    g_osk->visible = true;
    g_osk->shifted = false;
    g_osk->num_sym_mode = false;
    g_osk->saved_focus = app->focused_widget;

    osk_rebuild_keys(g_osk);

    app->modal_widget = (ClueWidget *)g_osk;
}

static bool g_osk_hiding = false;  /* re-entry guard */

void clue_osk_hide(void)
{
    if (!g_osk || !g_osk->visible || g_osk_hiding) return;

    g_osk_hiding = true;

    ClueApp *app = clue_app_get();
    g_osk->visible = false;

    if (app && app->modal_widget == (ClueWidget *)g_osk)
        app->modal_widget = NULL;

    osk_clear_keys(g_osk);

    g_osk_hiding = false;
}

bool clue_osk_visible(void)
{
    return g_osk && g_osk->visible;
}

void clue_osk_set_auto(bool enabled, ClueOskType type)
{
    /* Ensure g_osk struct exists for storing the setting */
    if (!g_osk && enabled) {
        g_osk = calloc(1, sizeof(ClueOsk));
        if (!g_osk) return;
        clue_cwidget_init(&g_osk->base, &osk_vtable);
    }
    if (g_osk) {
        g_osk->auto_enabled = enabled;
        g_osk->auto_type = type;
    }
}

void clue_osk_on_focus_changed(UIWidget *widget)
{
    if (!g_osk || !g_osk->auto_enabled || g_osk_hiding || g_osk->handling_event) return;

    if (widget) {
        ClueWidget *cw = (ClueWidget *)widget;
        if (cw->type_id == CLUE_WIDGET_TEXT_INPUT ||
            cw->type_id == CLUE_WIDGET_SPINBOX) {
            ClueOskType type = g_osk->auto_type;
            /* Use numpad for spinbox */
            if (cw->type_id == CLUE_WIDGET_SPINBOX)
                type = CLUE_OSK_NUMPAD;
            if (!g_osk->visible)
                clue_osk_show(type);
            return;
        }
    }

    /* Focus moved away from text widget -- hide */
    if (g_osk->visible)
        clue_osk_hide();
}