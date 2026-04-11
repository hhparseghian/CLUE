#include <stdio.h>
#include <string.h>
#include "builder.h"

static ClueBox *g_props_box = NULL;
static ClueLabel *g_title = NULL;
static ClueTextInput *g_inp_label = NULL;
static ClueSpinbox *g_inp_w = NULL;
static ClueSpinbox *g_inp_h = NULL;
static ClueCheckbox *g_chk_hexpand = NULL;
static ClueCheckbox *g_chk_vexpand = NULL;

static void on_label_changed(void *w, void *data)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;
    BuilderNode *node = &g_state.nodes[sel];

    const char *text = clue_text_input_get_text(g_inp_label);
    strncpy(node->label, text, sizeof(node->label) - 1);

    /* Apply to widget */
    ClueWidget *cw = node->widget;
    if (!cw) return;

    if (strcmp(node->type_name, "Button") == 0)
        clue_button_set_label((ClueButton *)cw, text);
    else if (strcmp(node->type_name, "Label") == 0)
        clue_label_set_text((ClueLabel *)cw, text);
    else if (strcmp(node->type_name, "Text Input") == 0)
        clue_text_input_set_text((ClueTextInput *)cw, text);
    else if (strcmp(node->type_name, "Checkbox") == 0)
        ; /* checkbox label is set at creation */
    else if (strcmp(node->type_name, "Toggle") == 0)
        ; /* toggle label is set at creation */

    builder_codegen_update();
}

static void on_size_changed(void *w, void *data)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;
    BuilderNode *node = &g_state.nodes[sel];

    node->w = (int)clue_spinbox_get_value(g_inp_w);
    node->h = (int)clue_spinbox_get_value(g_inp_h);

    if (node->widget) {
        node->widget->base.w = node->w;
        node->widget->base.h = node->h;
    }

    builder_codegen_update();
}

static void on_expand_changed(void *w, void *data)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;
    BuilderNode *node = &g_state.nodes[sel];

    node->hexpand = clue_checkbox_is_checked(g_chk_hexpand);
    node->vexpand = clue_checkbox_is_checked(g_chk_vexpand);

    if (node->widget) {
        node->widget->style.hexpand = node->hexpand;
        node->widget->style.vexpand = node->vexpand;
    }

    builder_codegen_update();
}

ClueScroll *builder_properties_create(void)
{
    ClueScroll *scroll = clue_scroll_new();
    scroll->base.base.w = 180;
    scroll->base.style.vexpand = true;

    ClueBox *box = clue_box_new(CLUE_VERTICAL, 4);
    clue_style_set_padding(&box->base.style, 6);
    box->base.style.hexpand = true;
    g_props_box = box;

    g_title = clue_label_new("Properties");
    g_title->base.style.fg_color = CLUE_RGB(200, 200, 210);
    g_title->base.style.margin_bottom = 4;
    clue_container_add(box, g_title);

    /* Label/text field */
    ClueLabel *lbl_text = clue_label_new("Text:");
    lbl_text->base.style.fg_color = CLUE_RGB(160, 160, 170);
    clue_container_add(box, lbl_text);

    g_inp_label = clue_text_input_new("...");
    g_inp_label->base.style.hexpand = true;
    clue_signal_connect(g_inp_label, "changed", on_label_changed, NULL);
    clue_container_add(box, g_inp_label);

    /* Width */
    ClueLabel *lbl_w = clue_label_new("Width:");
    lbl_w->base.style.fg_color = CLUE_RGB(160, 160, 170);
    clue_container_add(box, lbl_w);

    g_inp_w = clue_spinbox_new(0, 2000, 10, 0);
    g_inp_w->base.base.w = 120;
    clue_signal_connect(g_inp_w, "changed", on_size_changed, NULL);
    clue_container_add(box, g_inp_w);

    /* Height */
    ClueLabel *lbl_h = clue_label_new("Height:");
    lbl_h->base.style.fg_color = CLUE_RGB(160, 160, 170);
    clue_container_add(box, lbl_h);

    g_inp_h = clue_spinbox_new(0, 2000, 10, 0);
    g_inp_h->base.base.w = 120;
    clue_signal_connect(g_inp_h, "changed", on_size_changed, NULL);
    clue_container_add(box, g_inp_h);

    /* Expand */
    clue_container_add(box, clue_separator_new(CLUE_HORIZONTAL));
    g_chk_hexpand = clue_checkbox_new("H Expand");
    clue_signal_connect(g_chk_hexpand, "toggled", on_expand_changed, NULL);
    clue_container_add(box, g_chk_hexpand);

    g_chk_vexpand = clue_checkbox_new("V Expand");
    clue_signal_connect(g_chk_vexpand, "toggled", on_expand_changed, NULL);
    clue_container_add(box, g_chk_vexpand);

    clue_container_add(scroll, box);
    return scroll;
}

void builder_properties_refresh(void)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) {
        clue_label_set_text(g_title, "Properties");
        clue_text_input_set_text(g_inp_label, "");
        clue_spinbox_set_value(g_inp_w, 0);
        clue_spinbox_set_value(g_inp_h, 0);
        return;
    }

    BuilderNode *node = &g_state.nodes[sel];

    char title[128];
    snprintf(title, sizeof(title), "%s", node->type_name);
    clue_label_set_text(g_title, title);

    clue_text_input_set_text(g_inp_label, node->label);
    clue_spinbox_set_value(g_inp_w, node->w);
    clue_spinbox_set_value(g_inp_h, node->h);

    /* Sync expand checkboxes */
    if (clue_checkbox_is_checked(g_chk_hexpand) != node->hexpand)
        clue_checkbox_set_checked(g_chk_hexpand, node->hexpand);
    if (clue_checkbox_is_checked(g_chk_vexpand) != node->vexpand)
        clue_checkbox_set_checked(g_chk_vexpand, node->vexpand);
}
