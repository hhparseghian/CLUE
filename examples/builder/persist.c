#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builder.h"

#define BUF_SIZE (64 * 1024)

/* ------------------------------------------------------------------ */
/* Save to file                                                        */
/* ------------------------------------------------------------------ */

bool builder_save(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return false;

    fprintf(f, "{\n");
    fprintf(f, "  \"version\": 1,\n");
    fprintf(f, "  \"nodes\": [\n");

    for (int i = 0; i < g_state.count; i++) {
        BuilderNode *n = &g_state.nodes[i];
        fprintf(f, "    {\n");
        fprintf(f, "      \"id\": %d,\n", n->id);
        fprintf(f, "      \"parent_id\": %d,\n", n->parent_id);
        fprintf(f, "      \"type\": \"%s\",\n", n->type_name);
        fprintf(f, "      \"var_name\": \"%s\",\n", n->var_name);
        fprintf(f, "      \"label\": \"%s\",\n", n->label);
        fprintf(f, "      \"w\": %d,\n", n->w);
        fprintf(f, "      \"h\": %d,\n", n->h);
        fprintf(f, "      \"font_size\": %d,\n", n->font_size);
        fprintf(f, "      \"h_align\": %d,\n", n->h_align);
        fprintf(f, "      \"v_align\": %d,\n", n->v_align);
        fprintf(f, "      \"hexpand\": %s,\n", n->hexpand ? "true" : "false");
        fprintf(f, "      \"vexpand\": %s\n", n->vexpand ? "true" : "false");
        fprintf(f, "    }%s\n", (i < g_state.count - 1) ? "," : "");
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);
    return true;
}

/* ------------------------------------------------------------------ */
/* Load from file                                                      */
/* ------------------------------------------------------------------ */

/* Match registered widget type names to static strings (for type_name pointer) */
static const char *canonical_type(const char *s)
{
    static const char *types[] = {
        "Button", "Label", "Text Input", "Checkbox", "Toggle",
        "Slider", "Dropdown", "Separator", "Progress", "Spinbox",
        "Box (V)", "Box (H)", "Image",
    };
    int n = sizeof(types) / sizeof(types[0]);
    for (int i = 0; i < n; i++) {
        if (strcmp(types[i], s) == 0) return types[i];
    }
    return types[0];
}

/* Very small JSON parser: finds "key": value pairs.
 * Returns pointer to value (in buf) or NULL. */
static const char *find_field(const char *obj_start, const char *obj_end, const char *key)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(obj_start, pattern);
    if (!p || p >= obj_end) return NULL;
    p += strlen(pattern);
    while (p < obj_end && (*p == ' ' || *p == ':' || *p == '\t')) p++;
    return p;
}

static void parse_string(const char *p, char *out, int out_sz)
{
    out[0] = '\0';
    if (*p != '"') return;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < out_sz - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
}

static int parse_int(const char *p)
{
    return atoi(p);
}

static bool parse_bool(const char *p)
{
    return strncmp(p, "true", 4) == 0;
}

bool builder_load(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return false;

    char *buf = malloc(BUF_SIZE);
    if (!buf) { fclose(f); return false; }
    size_t n = fread(buf, 1, BUF_SIZE - 1, f);
    buf[n] = '\0';
    fclose(f);

    /* Clear current state via public API */
    while (g_state.count > 0) {
        g_state.selected = g_state.count - 1;
        builder_canvas_delete_selected();
    }

    /* Walk through each { ... } object */
    const char *p = buf;
    int max_id = 0;

    while ((p = strchr(p, '{')) != NULL) {
        /* Skip the outer {version,nodes} object */
        const char *type_p = find_field(p, buf + n, "type");
        if (!type_p) { p++; continue; }

        const char *obj_end = strchr(p, '}');
        if (!obj_end) break;

        char type[64];
        parse_string(type_p, type, sizeof(type));

        char var_name[64] = "";
        char label[128] = "";
        int id = 0, parent_id = -1, w = 0, h = 0;
        bool hexpand = false, vexpand = false;

        const char *f;
        if ((f = find_field(p, obj_end, "id"))) id = parse_int(f);
        if ((f = find_field(p, obj_end, "parent_id"))) parent_id = parse_int(f);
        if ((f = find_field(p, obj_end, "var_name"))) parse_string(f, var_name, sizeof(var_name));
        if ((f = find_field(p, obj_end, "label"))) parse_string(f, label, sizeof(label));
        if ((f = find_field(p, obj_end, "w"))) w = parse_int(f);
        if ((f = find_field(p, obj_end, "h"))) h = parse_int(f);
        int font_size = 0, h_align = 0, v_align = 0;
        if ((f = find_field(p, obj_end, "font_size"))) font_size = parse_int(f);
        if ((f = find_field(p, obj_end, "h_align"))) h_align = parse_int(f);
        if ((f = find_field(p, obj_end, "v_align"))) v_align = parse_int(f);
        if ((f = find_field(p, obj_end, "hexpand"))) hexpand = parse_bool(f);
        if ((f = find_field(p, obj_end, "vexpand"))) vexpand = parse_bool(f);

        /* Add widget via canvas; it handles creation and selection */
        builder_canvas_add_widget(canonical_type(type));

        /* Patch the just-added node with loaded values */
        int idx = g_state.count - 1;
        if (idx >= 0) {
            BuilderNode *node = &g_state.nodes[idx];
            node->id = id;
            node->parent_id = parent_id;
            strncpy(node->var_name, var_name, sizeof(node->var_name) - 1);
            strncpy(node->label, label, sizeof(node->label) - 1);
            node->w = w;
            node->h = h;
            node->font_size = font_size;
            node->h_align = h_align;
            node->v_align = v_align;
            node->hexpand = hexpand;
            node->vexpand = vexpand;
            if (node->widget) {
                node->widget->base.w = w;
                node->widget->base.h = h;
                node->widget->style.hexpand = hexpand;
                node->widget->style.vexpand = vexpand;
                node->widget->style.h_align = (ClueAlign)h_align;
                node->widget->style.v_align = (ClueAlign)v_align;
            }
            if (id > max_id) max_id = id;
        }

        p = obj_end + 1;
    }

    g_state.next_id = max_id + 1;

    /* Reparent widgets based on parent_id */
    for (int i = 0; i < g_state.count; i++) {
        BuilderNode *node = &g_state.nodes[i];
        if (node->parent_id < 0 || !node->widget) continue;

        /* Find parent node */
        ClueWidget *parent_widget = NULL;
        for (int j = 0; j < g_state.count; j++) {
            if (g_state.nodes[j].id == node->parent_id) {
                parent_widget = g_state.nodes[j].widget;
                break;
            }
        }
        if (!parent_widget) continue;

        /* Move widget to new parent */
        if (node->widget->base.parent)
            clue_widget_remove_child(node->widget->base.parent, &node->widget->base);
        clue_widget_add_child(&parent_widget->base, &node->widget->base);
    }

    g_state.selected = -1;
    builder_properties_refresh();
    builder_tree_refresh();
    builder_codegen_update();

    free(buf);
    return true;
}