#ifndef CLUE_THEME_H
#define CLUE_THEME_H

#include "renderer.h"

/* Theme: centralized colours, spacing, and per-widget defaults */
typedef struct {
    /* --- Base palette --- */
    ClueColor  bg;                /* app/window background */
    ClueColor  fg;                /* primary text */
    ClueColor  fg_dim;            /* secondary/muted text */
    ClueColor  fg_bright;         /* highlighted/title text */
    ClueColor  accent;            /* primary accent */
    ClueColor  accent_hover;
    ClueColor  accent_pressed;
    ClueColor  surface;           /* cards, panels */
    ClueColor  surface_hover;
    ClueColor  surface_border;
    ClueColor  success;
    ClueColor  warning;
    ClueColor  error;

    /* --- Per-widget colours --- */
    struct {
        ClueColor bg;
        ClueColor fg;
        ClueColor placeholder;
        ClueColor border;
        ClueColor focus_border;
        ClueColor cursor;
    } input;

    struct {
        ClueColor bg;
        ClueColor bg_hover;
        ClueColor bg_pressed;
        ClueColor fg;
    } button;

    struct {
        ClueColor bg;
        ClueColor fg;
        ClueColor selected_bg;
        ClueColor selected_fg;
        ClueColor hover_bg;
        ClueColor stripe;         /* alternating row tint */
    } list;

    struct {
        ClueColor bar_bg;
        ClueColor active_bg;
        ClueColor indicator;      /* active tab underline */
        ClueColor fg;
        ClueColor fg_active;
    } tabs;

    struct {
        ClueColor track;
        ClueColor fill;
        ClueColor thumb;
        ClueColor thumb_active;
    } slider;

    struct {
        ClueColor box_border;
        ClueColor box_checked;
        ClueColor checkmark;
        ClueColor fg;
    } checkbox;

    struct {
        ClueColor bg;
        ClueColor border;
        ClueColor fg;
        ClueColor list_bg;
        ClueColor hover_bg;
        ClueColor selected_bg;
        ClueColor arrow;
    } dropdown;

    struct {
        ClueColor overlay;        /* dim overlay behind modal */
        ClueColor bg;
        ClueColor border;
        ClueColor title_fg;
        ClueColor separator;
    } dialog;

    struct {
        ClueColor key_bg;           /* letter/number keys */
        ClueColor key_fg;           /* key label text */
        ClueColor modifier_bg;      /* Shift, 123, abc */
        ClueColor action_bg;        /* Bksp, Enter, Space */
        ClueColor done_bg;          /* Done key */
        ClueColor preview_bg;       /* text preview bar */
        ClueColor panel_bg;         /* keyboard panel background */
    } osk;

    /* --- Spacing & sizing --- */
    int      padding_sm;        /* 4 */
    int      padding_md;        /* 8 */
    int      padding_lg;        /* 16 */
    int      spacing_sm;        /* 6 */
    int      spacing_md;        /* 12 */
    int      spacing_lg;        /* 20 */
    float    corner_radius;     /* 6 */

    /* --- Font sizes (pixels) --- */
    int      font_size_sm;      /* 12 */
    int      font_size_md;      /* 16 */
    int      font_size_lg;      /* 24 */
    int      font_size_xl;      /* 32 */
} ClueTheme;

/* Get the active theme */
const ClueTheme *clue_theme_get(void);

/* Set a custom theme (copies the struct) */
void clue_theme_set(const ClueTheme *theme);

/* Built-in themes */
const ClueTheme *clue_theme_dark(void);
const ClueTheme *clue_theme_light(void);

#endif /* CLUE_THEME_H */
