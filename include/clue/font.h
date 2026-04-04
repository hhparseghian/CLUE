#ifndef CLUE_FONT_H
#define CLUE_FONT_H

/* Opaque font handle */
typedef struct UIFont UIFont;

/* Load a font from a TTF/OTF file at the given pixel size.
 * Returns NULL on failure. */
UIFont *clue_font_load(const char *path, int size_px);

/* Destroy a font and free its resources */
void clue_font_destroy(UIFont *font);

/* Measure the width in pixels of a string rendered with this font */
int clue_font_text_width(UIFont *font, const char *text);

/* Get the line height (ascent - descent + line gap) in pixels */
int clue_font_line_height(UIFont *font);

/* Get the font ascent in pixels (distance from baseline to top) */
int clue_font_ascent(UIFont *font);

#endif /* CLUE_FONT_H */
