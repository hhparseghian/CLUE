#ifndef CLUE_THEME_H
#define CLUE_THEME_H

#include "renderer.h"

/* Theme: centralized colours, spacing, and per-widget defaults */
typedef struct {
    /* --- Base palette --- */
    UIColor  bg;                /* app/window background */
    UIColor  fg;                /* primary text */
    UIColor  fg_dim;            /* secondary/muted text */
    UIColor  fg_bright;         /* highlighted/title text */
    UIColor  accent;            /* primary accent */
    UIColor  accent_hover;
    UIColor  accent_pressed;
    UIColor  surface;           /* cards, panels */
    UIColor  surface_hover;
    UIColor  surface_border;
    UIColor  success;
    UIColor  warning;
    UIColor  error;

    /* --- Per-widget colours --- */
    struct {
        UIColor bg;
        UIColor fg;
        UIColor placeholder;
        UIColor border;
        UIColor focus_border;
        UIColor cursor;
    } input;

    struct {
        UIColor bg;
        UIColor bg_hover;
        UIColor bg_pressed;
        UIColor fg;
    } button;

    struct {
        UIColor bg;
        UIColor fg;
        UIColor selected_bg;
        UIColor selected_fg;
        UIColor hover_bg;
        UIColor stripe;         /* alternating row tint */
    } list;

    struct {
        UIColor bar_bg;
        UIColor active_bg;
        UIColor indicator;      /* active tab underline */
        UIColor fg;
        UIColor fg_active;
    } tabs;

    struct {
        UIColor track;
        UIColor fill;
        UIColor thumb;
        UIColor thumb_active;
    } slider;

    struct {
        UIColor box_border;
        UIColor box_checked;
        UIColor checkmark;
        UIColor fg;
    } checkbox;

    struct {
        UIColor bg;
        UIColor border;
        UIColor fg;
        UIColor list_bg;
        UIColor hover_bg;
        UIColor selected_bg;
        UIColor arrow;
    } dropdown;

    struct {
        UIColor overlay;        /* dim overlay behind modal */
        UIColor bg;
        UIColor border;
        UIColor title_fg;
        UIColor separator;
    } dialog;

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
