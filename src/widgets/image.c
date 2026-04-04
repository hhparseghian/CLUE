#include <stdlib.h>
#include "clue/image.h"
#include "clue/draw.h"

static void image_draw(ClueWidget *w)
{
    ClueImage *img = (ClueImage *)w;
    if (!img->texture) return;

    clue_draw_image(img->texture, w->base.x, w->base.y, w->base.w, w->base.h);
}

static void image_layout(ClueWidget *w)
{
    ClueImage *img = (ClueImage *)w;
    if (w->base.w == 0) w->base.w = img->img_w;
    if (w->base.h == 0) w->base.h = img->img_h;
}

static void image_destroy(ClueWidget *w)
{
    ClueImage *img = (ClueImage *)w;
    if (img->owns_texture && img->texture) {
        clue_texture_destroy(img->texture);
        img->texture = 0;
    }
}

static const ClueWidgetVTable image_vtable = {
    .draw         = image_draw,
    .layout       = image_layout,
    .handle_event = NULL,
    .destroy      = image_destroy,
};

ClueImage *clue_image_new(const char *path)
{
    UITexture tex = clue_texture_load(path);
    if (!tex) return NULL;

    ClueImage *img = calloc(1, sizeof(ClueImage));
    if (!img) {
        clue_texture_destroy(tex);
        return NULL;
    }

    clue_cwidget_init(&img->base, &image_vtable);
    img->texture = tex;
    img->owns_texture = true;

    /* TODO: get actual image dimensions from stb_image.
     * For now, set a default and let the user resize. */
    img->img_w = 64;
    img->img_h = 64;
    img->base.base.w = img->img_w;
    img->base.base.h = img->img_h;

    return img;
}

ClueImage *clue_image_new_from_texture(UITexture tex, int w, int h)
{
    ClueImage *img = calloc(1, sizeof(ClueImage));
    if (!img) return NULL;

    clue_cwidget_init(&img->base, &image_vtable);
    img->texture = tex;
    img->owns_texture = false;
    img->img_w = w;
    img->img_h = h;
    img->base.base.w = w;
    img->base.base.h = h;

    return img;
}

void clue_image_set_size(ClueImage *image, int w, int h)
{
    if (!image) return;
    image->base.base.w = w;
    image->base.base.h = h;
}
