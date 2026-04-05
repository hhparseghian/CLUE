#ifndef CLUE_IMAGE_H
#define CLUE_IMAGE_H

#include "clue_widget.h"
#include "renderer.h"

/* Image widget -- displays a texture */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    ClueTexture   texture;
    int         img_w;      /* original image width */
    int         img_h;      /* original image height */
    bool        owns_texture; /* true if we should free on destroy */
} ClueImage;

/* Create an image widget from a file path */
ClueImage *clue_image_new(const char *path);

/* Create an image widget from an existing texture */
ClueImage *clue_image_new_from_texture(ClueTexture tex, int w, int h);

/* Set the display size (0 = use original image size) */
void clue_image_set_size(ClueImage *image, int w, int h);

#endif /* CLUE_IMAGE_H */
