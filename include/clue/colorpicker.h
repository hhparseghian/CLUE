#ifndef CLUE_COLORPICKER_H
#define CLUE_COLORPICKER_H

#include "clue_widget.h"
#include "renderer.h"

/* Color picker -- gradient SV square + hue bar + palette swatches */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    ClueColor     color;      /* currently selected color */
    ClueColor    *palette;    /* array of colors */
    int         palette_count;
    int         cols;       /* columns in the grid */
    int         swatch_size;/* pixel size of each swatch */
    int         hovered;    /* hovered palette index (-1 = none) */
    /* HSV gradient picker */
    float       hue;        /* 0.0 - 360.0 */
    float       sat;        /* 0.0 - 1.0 */
    float       val;        /* 0.0 - 1.0 */
    int         grad_size;  /* size of the SV square */
    int         hue_bar_w;  /* width of the hue bar */
    bool        dragging_sv;
    bool        dragging_hue;
} ClueColorPicker;

/* Create a color picker with a default palette */
ClueColorPicker *clue_colorpicker_new(void);

/* Get the selected color */
ClueColor clue_colorpicker_get_color(ClueColorPicker *cp);

/* Set the selected color */
void clue_colorpicker_set_color(ClueColorPicker *cp, ClueColor color);

/* Add a color to the palette */
void clue_colorpicker_add_color(ClueColorPicker *cp, ClueColor color);

/* Signals: "changed" -- emitted when color selection changes */

#endif /* CLUE_COLORPICKER_H */
