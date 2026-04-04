#include <stdlib.h>
#include "clue/box.h"
#include "clue/draw.h"

static void box_draw(ClueWidget *w)
{
    /* Draw background if set */
    if (w->style.bg_color.a > 0.001f) {
        clue_fill_rounded_rect(w->base.x, w->base.y, w->base.w, w->base.h,
                               w->style.corner_radius, w->style.bg_color);
    }
    /* Children are drawn by clue_cwidget_draw_tree after this returns */
}

static void box_layout(ClueWidget *w)
{
    ClueBox *box = (ClueBox *)w;
    ClueStyle *s = &w->style;

    /* Save parent-set size before layout modifies it */
    int preset_w = w->base.w;
    int preset_h = w->base.h;

    int pad_h = s->padding_left + s->padding_right;
    int pad_v = s->padding_top + s->padding_bottom;
    int avail_w = w->base.w - pad_h;
    int avail_h = w->base.h - pad_v;

    /* Reset expanded dimensions so children shrink when parent shrinks */
    for (int i = 0; i < w->base.child_count; i++) {
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        if (!child->base.visible) continue;
        if (child->style.hexpand) child->base.w = 0;
        if (child->style.vexpand) child->base.h = 0;
    }

    /* First pass: position children, measure content, count expanders */
    int cx = w->base.x + s->padding_left;
    int cy = w->base.y + s->padding_top;
    int max_cross = 0;
    int total_main = 0;
    int visible_count = 0;
    int expand_count = 0;

    for (int i = 0; i < w->base.child_count; i++) {
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        if (!child->base.visible) continue;

        if (visible_count > 0) {
            if (box->orientation == CLUE_VERTICAL)
                cy += box->spacing;
            else
                cx += box->spacing;
            total_main += box->spacing;
        }
        visible_count++;

        child->base.x = cx + child->style.margin_left;
        child->base.y = cy + child->style.margin_top;

        int child_w = child->base.w + child->style.margin_left + child->style.margin_right;
        int child_h = child->base.h + child->style.margin_top + child->style.margin_bottom;

        if (box->orientation == CLUE_VERTICAL) {
            if (child->style.vexpand) expand_count++;
            cy += child_h;
            total_main += child_h;
            if (child_w > max_cross) max_cross = child_w;
        } else {
            if (child->style.hexpand) expand_count++;
            cx += child_w;
            total_main += child_w;
            if (child_h > max_cross) max_cross = child_h;
        }
    }

    /* Size the box itself: keep parent-set size if hexpand/vexpand */
    int content_w = (box->orientation == CLUE_VERTICAL)
        ? max_cross + pad_h : total_main + pad_h;
    int content_h = (box->orientation == CLUE_VERTICAL)
        ? total_main + pad_v : max_cross + pad_v;

    /* Never shrink below content, but keep pre-set size if larger */
    if (content_w > w->base.w) w->base.w = content_w;
    if (content_h > w->base.h) w->base.h = content_h;
    /* Respect hexpand/vexpand: keep parent-set size */
    if (s->hexpand && preset_w > w->base.w) w->base.w = preset_w;
    if (s->vexpand && preset_h > w->base.h) w->base.h = preset_h;

    avail_w = w->base.w - pad_h;
    avail_h = w->base.h - pad_v;

    /* Second pass: distribute extra space to expanding children,
     * apply cross-axis expand, and handle alignment */
    int extra = 0;
    int per_expand = 0;
    if (expand_count > 0) {
        if (box->orientation == CLUE_VERTICAL)
            extra = avail_h - total_main;
        else
            extra = avail_w - total_main;
        if (extra > 0)
            per_expand = extra / expand_count;
        else
            extra = 0;
    }

    int offset_main = 0;
    for (int i = 0; i < w->base.child_count; i++) {
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        if (!child->base.visible) continue;

        if (box->orientation == CLUE_VERTICAL) {
            /* Shift by accumulated expand offset */
            child->base.y += offset_main;

            /* Expand in main axis (vertical) */
            if (child->style.vexpand && per_expand > 0) {
                child->base.h += per_expand;
                offset_main += per_expand;
            }

            /* Expand in cross axis (horizontal) */
            if (child->style.hexpand && avail_w > child->base.w)
                child->base.w = avail_w;

            /* Alignment in cross axis */
            int cw = child->base.w + child->style.margin_left + child->style.margin_right;
            int align_off = 0;
            if (s->h_align == CLUE_ALIGN_CENTER)
                align_off = (avail_w - cw) / 2;
            else if (s->h_align == CLUE_ALIGN_END)
                align_off = avail_w - cw;
            if (align_off > 0)
                child->base.x += align_off;
        } else {
            child->base.x += offset_main;

            if (child->style.hexpand && per_expand > 0) {
                child->base.w += per_expand;
                offset_main += per_expand;
            }

            if (child->style.vexpand && avail_h > child->base.h)
                child->base.h = avail_h;

            int ch = child->base.h + child->style.margin_top + child->style.margin_bottom;
            int align_off = 0;
            if (s->v_align == CLUE_ALIGN_CENTER)
                align_off = (avail_h - ch) / 2;
            else if (s->v_align == CLUE_ALIGN_END)
                align_off = avail_h - ch;
            if (align_off > 0)
                child->base.y += align_off;
        }
    }
}

static const ClueWidgetVTable box_vtable = {
    .draw         = box_draw,
    .layout       = box_layout,
    .handle_event = NULL,
    .destroy      = NULL,
};

ClueBox *clue_box_new(ClueBoxOrientation orientation, int spacing)
{
    ClueBox *b = calloc(1, sizeof(ClueBox));
    if (!b) return NULL;

    clue_cwidget_init(&b->base, &box_vtable);
    b->orientation = orientation;
    b->spacing     = spacing;

    return b;
}
