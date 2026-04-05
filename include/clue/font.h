#ifndef CLUE_FONT_H
#define CLUE_FONT_H

/* Opaque font handle */
typedef struct ClueFont ClueFont;

/* Load a font from a TTF/OTF file at the given pixel size.
 * Returns NULL on failure. */
ClueFont *clue_font_load(const char *path, int size_px);

/* Destroy a font and free its resources */
void clue_font_destroy(ClueFont *font);

/* Measure the width in pixels of a string rendered with this font */
int clue_font_text_width(ClueFont *font, const char *text);

/* Get the line height (ascent - descent + line gap) in pixels */
int clue_font_line_height(ClueFont *font);

/* Get the font ascent in pixels (distance from baseline to top) */
int clue_font_ascent(ClueFont *font);

#endif /* CLUE_FONT_H */
