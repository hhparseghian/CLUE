#include <stdlib.h>
#include <string.h>
#include "builder.h"

#define HISTORY_MAX 32

/* Snapshot of builder node state (not widgets — we recreate those) */
typedef struct {
    int    count;
    int    next_id;
    BuilderNode nodes[MAX_NODES];
} Snapshot;

static Snapshot g_undo[HISTORY_MAX];
static Snapshot g_redo[HISTORY_MAX];
static int g_undo_count = 0;
static int g_redo_count = 0;

static void snapshot_capture(Snapshot *s)
{
    s->count = g_state.count;
    s->next_id = g_state.next_id;
    memcpy(s->nodes, g_state.nodes, sizeof(BuilderNode) * g_state.count);
    /* Don't capture widget pointers — they'll be invalid after restore */
    for (int i = 0; i < s->count; i++)
        s->nodes[i].widget = NULL;
}

static void snapshot_restore(Snapshot *s)
{
    /* Clear current widgets */
    while (g_state.count > 0) {
        g_state.selected = g_state.count - 1;
        builder_canvas_delete_selected();
    }

    /* Rebuild each node */
    for (int i = 0; i < s->count; i++) {
        BuilderNode *src = &s->nodes[i];
        builder_canvas_add_widget(src->type_name);
        int idx = g_state.count - 1;
        if (idx >= 0) {
            BuilderNode *dst = &g_state.nodes[idx];
            dst->id = src->id;
            dst->parent_id = src->parent_id;
            strncpy(dst->var_name, src->var_name, sizeof(dst->var_name) - 1);
            strncpy(dst->label, src->label, sizeof(dst->label) - 1);
            dst->w = src->w;
            dst->h = src->h;
            dst->hexpand = src->hexpand;
            dst->vexpand = src->vexpand;
            if (dst->widget) {
                dst->widget->base.w = src->w;
                dst->widget->base.h = src->h;
                dst->widget->style.hexpand = src->hexpand;
                dst->widget->style.vexpand = src->vexpand;
            }
        }
    }

    g_state.next_id = s->next_id;

    /* Reparent */
    for (int i = 0; i < g_state.count; i++) {
        BuilderNode *node = &g_state.nodes[i];
        if (node->parent_id < 0 || !node->widget) continue;
        ClueWidget *parent_widget = NULL;
        for (int j = 0; j < g_state.count; j++) {
            if (g_state.nodes[j].id == node->parent_id) {
                parent_widget = g_state.nodes[j].widget;
                break;
            }
        }
        if (!parent_widget) continue;
        if (node->widget->base.parent)
            clue_widget_remove_child(node->widget->base.parent, &node->widget->base);
        clue_widget_add_child(&parent_widget->base, &node->widget->base);
    }

    g_state.selected = -1;
    builder_properties_refresh();
    builder_tree_refresh();
    builder_codegen_update();
}

void builder_history_push(void)
{
    if (g_undo_count >= HISTORY_MAX) {
        /* Drop oldest */
        memmove(&g_undo[0], &g_undo[1], sizeof(Snapshot) * (HISTORY_MAX - 1));
        g_undo_count--;
    }
    snapshot_capture(&g_undo[g_undo_count++]);
    g_redo_count = 0; /* Clear redo stack on new action */
}

void builder_undo(void)
{
    if (g_undo_count == 0) return;

    /* Capture current state to redo stack */
    if (g_redo_count < HISTORY_MAX)
        snapshot_capture(&g_redo[g_redo_count++]);

    /* Pop from undo and restore */
    g_undo_count--;
    snapshot_restore(&g_undo[g_undo_count]);
}

void builder_redo(void)
{
    if (g_redo_count == 0) return;

    /* Capture current state to undo stack */
    if (g_undo_count < HISTORY_MAX)
        snapshot_capture(&g_undo[g_undo_count++]);

    /* Pop from redo and restore */
    g_redo_count--;
    snapshot_restore(&g_redo[g_redo_count]);
}