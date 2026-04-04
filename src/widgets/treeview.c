#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/treeview.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#define ARROW_SIZE 8
#define SCROLLBAR_W 6

static UIFont *tv_font(ClueTreeView *tv)
{
    return tv->base.style.font ? tv->base.style.font : clue_app_default_font();
}

/* Count visible (expanded) nodes for scrolling */
static int count_visible(ClueTreeNode *node)
{
    if (!node) return 0;
    int count = 0;
    for (int i = 0; i < node->child_count; i++) {
        count++; /* the child itself */
        if (node->children[i]->expanded)
            count += count_visible(node->children[i]);
    }
    return count;
}

/* Get the nth visible node (0-indexed, depth-first) */
typedef struct { int index; ClueTreeNode *result; } NodeSearch;

static void find_visible_at(ClueTreeNode *node, NodeSearch *s, int target)
{
    if (!node || s->result) return;
    for (int i = 0; i < node->child_count; i++) {
        if (s->index == target) { s->result = node->children[i]; return; }
        s->index++;
        if (node->children[i]->expanded)
            find_visible_at(node->children[i], s, target);
        if (s->result) return;
    }
}

static ClueTreeNode *get_visible_node(ClueTreeNode *root, int idx)
{
    NodeSearch s = {0, NULL};
    find_visible_at(root, &s, idx);
    return s.result;
}

/* Draw tree recursively, returns number of rows drawn */
static int draw_tree_nodes(ClueTreeView *tv, ClueTreeNode *node,
                           int x, int y, int bw, int *row,
                           int clip_top, int clip_bottom)
{
    UIFont *font = tv_font(tv);
    const ClueTheme *th = clue_theme_get();
    int ih = tv->item_height;
    int drawn = 0;

    for (int i = 0; i < node->child_count; i++) {
        ClueTreeNode *child = node->children[i];
        int iy = y + (*row) * ih - tv->scroll_y;

        if (iy + ih > clip_top && iy < clip_bottom) {
            /* Selection / hover bg */
            if (child == tv->selected) {
                clue_fill_rect(x, iy, bw, ih, th->list.selected_bg);
            } else if (child == tv->hovered) {
                clue_fill_rect(x, iy, bw, ih, th->list.hover_bg);
            }

            int indent = x + child->depth * tv->indent;

            /* Expand/collapse arrow for nodes with children */
            if (child->child_count > 0 && font) {
                int ay = iy + ih / 2;
                int ax = indent + 2;
                if (child->expanded) {
                    /* Down arrow */
                    clue_draw_line(ax, ay - 3, ax + ARROW_SIZE/2, ay + 3,
                                   1.5f, th->fg_dim);
                    clue_draw_line(ax + ARROW_SIZE/2, ay + 3, ax + ARROW_SIZE, ay - 3,
                                   1.5f, th->fg_dim);
                } else {
                    /* Right arrow */
                    clue_draw_line(ax, ay - ARROW_SIZE/2, ax + 5, ay,
                                   1.5f, th->fg_dim);
                    clue_draw_line(ax + 5, ay, ax, ay + ARROW_SIZE/2,
                                   1.5f, th->fg_dim);
                }
            }

            /* Label */
            if (font && child->label) {
                UIColor fg = (child == tv->selected) ? th->list.selected_fg : th->list.fg;
                int lx = indent + tv->indent;
                int ty = iy + (ih - clue_font_line_height(font)) / 2;
                clue_draw_text(lx, ty, child->label, font, fg);
            }
        }

        (*row)++;
        drawn++;

        if (child->expanded) {
            drawn += draw_tree_nodes(tv, child, x, y, bw, row,
                                     clip_top, clip_bottom);
        }
    }
    return drawn;
}

static void treeview_draw(ClueWidget *w)
{
    ClueTreeView *tv = (ClueTreeView *)w;
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    clue_fill_rounded_rect(x, y, bw, bh, th->corner_radius, th->list.bg);
    clue_draw_rounded_rect(x, y, bw, bh, th->corner_radius, 1.0f,
                           th->surface_border);

    if (!tv->root) return;

    clue_set_clip_rect(x, y, bw, bh);
    int row = 0;
    draw_tree_nodes(tv, tv->root, x, y, bw, &row, y, y + bh);
    clue_reset_clip_rect();

    /* Scrollbar */
    int total = count_visible(tv->root) * tv->item_height;
    if (total > bh) {
        float ratio = (float)bh / (float)total;
        int bar_h = (int)(ratio * bh);
        if (bar_h < 20) bar_h = 20;
        float pos = (float)tv->scroll_y / (float)(total - bh);
        int bar_y = y + (int)(pos * (bh - bar_h));
        clue_fill_rounded_rect(x + bw - SCROLLBAR_W - 2, bar_y,
                               SCROLLBAR_W, bar_h, SCROLLBAR_W / 2.0f,
                               UI_RGBA(150, 150, 160, 120));
    }
}

static void treeview_layout(ClueWidget *w)
{
    ClueTreeView *tv = (ClueTreeView *)w;
    UIFont *font = tv_font(tv);
    if (font && tv->item_height == 0)
        tv->item_height = clue_font_line_height(font) + 8;
    if (w->base.w == 0) w->base.w = 250;
    if (w->base.h == 0) w->base.h = 200;
}

static void clamp_scroll(ClueTreeView *tv)
{
    int total = count_visible(tv->root) * tv->item_height;
    int max_y = total - tv->base.base.h;
    if (max_y < 0) max_y = 0;
    if (tv->scroll_y > max_y) tv->scroll_y = max_y;
    if (tv->scroll_y < 0) tv->scroll_y = 0;
}

static int treeview_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueTreeView *tv = (ClueTreeView *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    switch (event->type) {
    case UI_EVENT_MOUSE_MOVE: {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        tv->hovered = NULL;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            int idx = (my - y + tv->scroll_y) / tv->item_height;
            tv->hovered = get_visible_node(tv->root, idx);
            return 1;
        }
        return 0;
    }

    case UI_EVENT_MOUSE_BUTTON: {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        if (!(mx >= x && mx < x + bw && my >= y && my < y + bh))
            return 0;
        if (!event->mouse_button.pressed || event->mouse_button.btn != 0)
            return 1;

        clue_focus_widget(&w->base);
        int idx = (my - y + tv->scroll_y) / tv->item_height;
        ClueTreeNode *node = get_visible_node(tv->root, idx);
        if (!node) return 1;

        /* Check if click is on the expand arrow area */
        int indent = x + node->depth * tv->indent;
        if (mx >= indent && mx < indent + tv->indent && node->child_count > 0) {
            node->expanded = !node->expanded;
            clamp_scroll(tv);
        } else {
            tv->selected = node;
            clue_signal_emit(tv, "selected");
        }
        return 1;
    }

    case UI_EVENT_MOUSE_SCROLL: {
        int mx = event->mouse_scroll.x;
        int my = event->mouse_scroll.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            tv->scroll_y -= (int)(event->mouse_scroll.dy * 30.0f);
            clamp_scroll(tv);
            return 1;
        }
        return 0;
    }

    default: return 0;
    }
}

static void free_tree_node(ClueTreeNode *node)
{
    if (!node) return;
    for (int i = 0; i < node->child_count; i++)
        free_tree_node(node->children[i]);
    free(node->label);
    free(node);
}

static void treeview_destroy(ClueWidget *w)
{
    ClueTreeView *tv = (ClueTreeView *)w;
    free_tree_node(tv->root);
}

static const ClueWidgetVTable treeview_vtable = {
    .draw = treeview_draw, .layout = treeview_layout,
    .handle_event = treeview_handle_event, .destroy = treeview_destroy,
};

ClueTreeView *clue_treeview_new(void)
{
    ClueTreeView *tv = calloc(1, sizeof(ClueTreeView));
    if (!tv) return NULL;
    clue_cwidget_init(&tv->base, &treeview_vtable);
    tv->base.base.focusable = true;
    tv->indent = 20;
    tv->root = clue_tree_node_new(NULL); /* invisible root */
    return tv;
}

ClueTreeNode *clue_tree_node_new(const char *label)
{
    ClueTreeNode *n = calloc(1, sizeof(ClueTreeNode));
    if (!n) return NULL;
    n->label = label ? strdup(label) : NULL;
    n->expanded = false;
    return n;
}

static void update_depth(ClueTreeNode *node, int depth)
{
    node->depth = depth;
    for (int i = 0; i < node->child_count; i++)
        update_depth(node->children[i], depth + 1);
}

ClueTreeNode *clue_tree_node_add(ClueTreeNode *parent, ClueTreeNode *child)
{
    if (!parent || !child || parent->child_count >= CLUE_TREE_MAX_CHILDREN)
        return NULL;
    child->parent = parent;
    parent->children[parent->child_count++] = child;
    update_depth(child, parent->depth + 1);
    return child;
}

ClueTreeNode *clue_treeview_get_selected(ClueTreeView *tv)
{
    return tv ? tv->selected : NULL;
}
