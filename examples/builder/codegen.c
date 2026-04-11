#include <stdio.h>
#include <string.h>
#include "builder.h"

#define CODE_BUF_SIZE 8192

static char g_code[CODE_BUF_SIZE];

static void emit_widget(char *buf, int *pos, int max, BuilderNode *node)
{
    int p = *pos;

#define EMIT(...) do { \
    p += snprintf(buf + p, max - p, __VA_ARGS__); \
    if (p >= max) { *pos = p; return; } \
} while(0)

    if (strcmp(node->type_name, "Button") == 0) {
        EMIT("    ClueButton *%s = clue_button_new(\"%s\");\n",
             node->var_name, node->label);
    } else if (strcmp(node->type_name, "Label") == 0) {
        EMIT("    ClueLabel *%s = clue_label_new(\"%s\");\n",
             node->var_name, node->label);
    } else if (strcmp(node->type_name, "Text Input") == 0) {
        EMIT("    ClueTextInput *%s = clue_text_input_new(\"%s\");\n",
             node->var_name, node->label);
    } else if (strcmp(node->type_name, "Checkbox") == 0) {
        EMIT("    ClueCheckbox *%s = clue_checkbox_new(\"%s\");\n",
             node->var_name, node->label);
    } else if (strcmp(node->type_name, "Toggle") == 0) {
        EMIT("    ClueToggle *%s = clue_toggle_new(\"%s\");\n",
             node->var_name, node->label);
    } else if (strcmp(node->type_name, "Slider") == 0) {
        EMIT("    ClueSlider *%s = clue_slider_new(0, 100, 50);\n",
             node->var_name);
    } else if (strcmp(node->type_name, "Dropdown") == 0) {
        EMIT("    ClueDropdown *%s = clue_dropdown_new(\"%s\");\n",
             node->var_name, node->label);
    } else if (strcmp(node->type_name, "Separator") == 0) {
        EMIT("    ClueSeparator *%s = clue_separator_new(CLUE_HORIZONTAL);\n",
             node->var_name);
    } else if (strcmp(node->type_name, "Progress") == 0) {
        EMIT("    ClueProgress *%s = clue_progress_new();\n",
             node->var_name);
    } else if (strcmp(node->type_name, "Spinbox") == 0) {
        EMIT("    ClueSpinbox *%s = clue_spinbox_new(0, 100, 1, 0);\n",
             node->var_name);
    } else if (strcmp(node->type_name, "Box (V)") == 0) {
        EMIT("    ClueBox *%s = clue_box_new(CLUE_VERTICAL, 4);\n",
             node->var_name);
    } else if (strcmp(node->type_name, "Box (H)") == 0) {
        EMIT("    ClueBox *%s = clue_box_new(CLUE_HORIZONTAL, 4);\n",
             node->var_name);
    } else if (strcmp(node->type_name, "Image") == 0) {
        EMIT("    ClueImage *%s = clue_image_new(\"path/to/image.png\");\n",
             node->var_name);
    }

    if (node->w > 0)
        EMIT("    %s->base.base.w = %d;\n", node->var_name, node->w);
    if (node->h > 0)
        EMIT("    %s->base.base.h = %d;\n", node->var_name, node->h);
    if (node->hexpand)
        EMIT("    %s->base.style.hexpand = true;\n", node->var_name);
    if (node->vexpand)
        EMIT("    %s->base.style.vexpand = true;\n", node->var_name);

    EMIT("    clue_container_add(root, %s);\n\n", node->var_name);

#undef EMIT

    *pos = p;
}

void builder_codegen_update(void)
{
    int pos = 0;
    int max = CODE_BUF_SIZE;

    pos += snprintf(g_code + pos, max - pos,
        "#include <clue/clue.h>\n\n"
        "int main(void)\n"
        "{\n"
        "    ClueApp *app = clue_app_new(\"My App\", 600, 400);\n"
        "    ClueBox *root = clue_box_new(CLUE_VERTICAL, 8);\n"
        "    clue_style_set_padding(&root->base.style, 12);\n\n");

    for (int i = 0; i < g_state.count && pos < max; i++) {
        emit_widget(g_code, &pos, max, &g_state.nodes[i]);
    }

    pos += snprintf(g_code + pos, max - pos,
        "    clue_app_set_root(app, root);\n"
        "    clue_app_run(app);\n"
        "    clue_app_destroy(app);\n"
        "    return 0;\n"
        "}\n");

    if (g_state.code_view)
        clue_text_editor_set_text(g_state.code_view, g_code);
}
