#ifndef CLUE_STYLE_H
#define CLUE_STYLE_H

#include "renderer.h"

/* Forward declaration */
struct UIFont;

/* Alignment for positioning children within their parent */
typedef enum {
    CLUE_ALIGN_START  = 0,  /* left / top (default) */
    CLUE_ALIGN_CENTER = 1,
    CLUE_ALIGN_END    = 2,  /* right / bottom */
} ClueAlign;

/* Style properties for widgets */
typedef struct {
    int      padding_top, padding_right, padding_bottom, padding_left;
    int      margin_top, margin_right, margin_bottom, margin_left;
    UIColor  bg_color;
    UIColor  fg_color;
    float    corner_radius;
    struct UIFont *font;   /* NULL = use app default */
    ClueAlign     h_align; /* horizontal alignment of children */
    ClueAlign     v_align; /* vertical alignment of children */
    bool          hexpand; /* expand to fill parent width */
    bool          vexpand; /* expand to fill parent height */
} ClueStyle;

/* Default style: transparent bg, white fg, no padding/margin */
#define CLUE_STYLE_DEFAULT ((ClueStyle){ \
    0, 0, 0, 0,  \
    0, 0, 0, 0,  \
    {0.0f, 0.0f, 0.0f, 0.0f}, \
    {1.0f, 1.0f, 1.0f, 1.0f}, \
    0.0f, \
    NULL, \
    CLUE_ALIGN_START, CLUE_ALIGN_START, \
    false, false \
})

/* Set uniform padding on all sides */
static inline void clue_style_set_padding(ClueStyle *s, int p)
{
    s->padding_top = s->padding_right = s->padding_bottom = s->padding_left = p;
}

/* Set uniform margin on all sides */
static inline void clue_style_set_margin(ClueStyle *s, int m)
{
    s->margin_top = s->margin_right = s->margin_bottom = s->margin_left = m;
}

#endif /* CLUE_STYLE_H */
