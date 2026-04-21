#include <stdio.h>
#include <string.h>
#include "builder.h"

static ClueBox *g_props_box = NULL;
static ClueLabel *g_title = NULL;
static ClueTextInput *g_inp_name = NULL;
static ClueTextInput *g_inp_label = NULL;
static ClueSpinbox *g_inp_w = NULL;
static ClueSpinbox *g_inp_h = NULL;
static ClueCheckbox *g_chk_hexpand = NULL;
static ClueCheckbox *g_chk_vexpand = NULL;
static ClueSpinbox  *g_inp_font_size = NULL;
static ClueDropdown *g_dd_halign = NULL;
static ClueDropdown *g_dd_valign = NULL;

/* Field labels (for show/hide) */
static ClueLabel    *g_lbl_w = NULL;
static ClueLabel    *g_lbl_h = NULL;
static ClueLabel    *g_lbl_font_size = NULL;
static ClueLabel    *g_lbl_text = NULL;
static ClueLabel    *g_lbl_name = NULL;
static ClueLabel    *g_lbl_halign = NULL;
static ClueLabel    *g_lbl_valign = NULL;

/* Empty-state placeholder */
static ClueLabel    *g_empty_hint = NULL;

/* Separators between sections */
static ClueSeparator *g_sep1 = NULL;
static ClueSeparator *g_sep2 = NULL;
static ClueSeparator *g_sep3 = NULL;

static void on_name_changed(void *w, void *data)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;
    BuilderNode *node = &g_state.nodes[sel];

    const char *text = clue_text_input_get_text(g_inp_name);
    strncpy(node->var_name, text, sizeof(node->var_name) - 1);
    node->var_name[sizeof(node->var_name) - 1] = '\0';

    builder_codegen_update();
    builder_tree_refresh();
}

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

static void on_font_size_changed(void *w, void *data)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;
    BuilderNode *node = &g_state.nodes[sel];

    int size = (int)clue_spinbox_get_value(g_inp_font_size);
    node->font_size = size;

    /* Apply to label widget */
    if (node->widget && strcmp(node->type_name, "Label") == 0 && size > 0) {
        const char *font_paths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            NULL
        };
        for (int i = 0; font_paths[i]; i++) {
            ClueFont *f = clue_font_load(font_paths[i], size);
            if (f) {
                node->widget->style.font = f;
                break;
            }
        }
    }

    builder_codegen_update();
}

static void on_align_changed(void *w, void *data)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;
    BuilderNode *node = &g_state.nodes[sel];

    node->h_align = clue_dropdown_get_selected(g_dd_halign);
    node->v_align = clue_dropdown_get_selected(g_dd_valign);

    if (node->widget) {
        node->widget->style.h_align = (ClueAlign)node->h_align;
        node->widget->style.v_align = (ClueAlign)node->v_align;
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

static ClueLabel *make_field_label(const char *text)
{
    ClueLabel *l = clue_label_new(text);
    l->base.style.fg_color = CLUE_RGB(160, 160, 170);
    l->base.style.margin_top = 4;
    return l;
}

ClueScroll *builder_properties_create(void)
{
    ClueScroll *scroll = clue_scroll_new();
    scroll->base.base.w = 220;
    scroll->base.style.vexpand = true;

    ClueBox *box = clue_box_new(CLUE_VERTICAL, 4);
    clue_style_set_padding(&box->base.style, 8);
    box->base.style.padding_right = 18;  /* room for scrollbar */
    box->base.style.hexpand = true;
    g_props_box = box;

    g_title = clue_label_new("Properties");
    g_title->base.style.fg_color = CLUE_RGB(220, 220, 230);
    g_title->base.style.margin_bottom = 6;
    clue_container_add(box, g_title);

    /* Empty-state hint (shown when nothing selected) */
    g_empty_hint = clue_label_new("No widget selected");
    g_empty_hint->base.style.fg_color = CLUE_RGB(140, 140, 150);
    g_empty_hint->base.style.margin_top = 8;
    clue_container_add(box, g_empty_hint);

    /* Variable name */
    g_lbl_name = make_field_label("Name:");
    clue_container_add(box, g_lbl_name);
    g_inp_name = clue_text_input_new("...");
    g_inp_name->base.style.hexpand = true;
    clue_signal_connect(g_inp_name, "changed", on_name_changed, NULL);
    clue_container_add(box, g_inp_name);

    /* Label/text field */
    g_lbl_text = make_field_label("Text:");
    clue_container_add(box, g_lbl_text);
    g_inp_label = clue_text_input_new("...");
    g_inp_label->base.style.hexpand = true;
    clue_signal_connect(g_inp_label, "changed", on_label_changed, NULL);
    clue_container_add(box, g_inp_label);

    /* Font size */
    g_lbl_font_size = make_field_label("Font size:");
    clue_container_add(box, g_lbl_font_size);
    g_inp_font_size = clue_spinbox_new(0, 120, 1, 0);
    g_inp_font_size->base.style.hexpand = true;
    clue_signal_connect(g_inp_font_size, "changed", on_font_size_changed, NULL);
    clue_container_add(box, g_inp_font_size);

    /* Size section */
    g_sep1 = clue_separator_new(CLUE_HORIZONTAL);
    clue_container_add(box, g_sep1);

    g_lbl_w = make_field_label("Width:");
    clue_container_add(box, g_lbl_w);
    g_inp_w = clue_spinbox_new(0, 2000, 10, 0);
    g_inp_w->base.style.hexpand = true;
    clue_signal_connect(g_inp_w, "changed", on_size_changed, NULL);
    clue_container_add(box, g_inp_w);

    g_lbl_h = make_field_label("Height:");
    clue_container_add(box, g_lbl_h);
    g_inp_h = clue_spinbox_new(0, 2000, 10, 0);
    g_inp_h->base.style.hexpand = true;
    clue_signal_connect(g_inp_h, "changed", on_size_changed, NULL);
    clue_container_add(box, g_inp_h);

    /* Alignment section */
    g_sep2 = clue_separator_new(CLUE_HORIZONTAL);
    clue_container_add(box, g_sep2);

    g_lbl_halign = make_field_label("H Align:");
    clue_container_add(box, g_lbl_halign);
    g_dd_halign = clue_dropdown_new("Start");
    g_dd_halign->base.style.hexpand = true;
    clue_dropdown_add_item(g_dd_halign, "Start");
    clue_dropdown_add_item(g_dd_halign, "Center");
    clue_dropdown_add_item(g_dd_halign, "End");
    clue_signal_connect(g_dd_halign, "changed", on_align_changed, NULL);
    clue_container_add(box, g_dd_halign);

    g_lbl_valign = make_field_label("V Align:");
    clue_container_add(box, g_lbl_valign);
    g_dd_valign = clue_dropdown_new("Start");
    g_dd_valign->base.style.hexpand = true;
    clue_dropdown_add_item(g_dd_valign, "Start");
    clue_dropdown_add_item(g_dd_valign, "Center");
    clue_dropdown_add_item(g_dd_valign, "End");
    clue_signal_connect(g_dd_valign, "changed", on_align_changed, NULL);
    clue_container_add(box, g_dd_valign);

    /* Expand section */
    g_sep3 = clue_separator_new(CLUE_HORIZONTAL);
    clue_container_add(box, g_sep3);
    g_chk_hexpand = clue_checkbox_new("H Expand");
    clue_signal_connect(g_chk_hexpand, "toggled", on_expand_changed, NULL);
    clue_container_add(box, g_chk_hexpand);

    g_chk_vexpand = clue_checkbox_new("V Expand");
    clue_signal_connect(g_chk_vexpand, "toggled", on_expand_changed, NULL);
    clue_container_add(box, g_chk_vexpand);

    clue_container_add(scroll, box);
    return scroll;
}

static void set_field_visible(ClueWidget *label, ClueWidget *field, bool visible)
{
    if (label) label->base.visible = visible;
    if (field) field->base.visible = visible;
}

static void set_all_fields_visible(bool v)
{
    ClueWidget *widgets[] = {
        (ClueWidget *)g_lbl_name,    (ClueWidget *)g_inp_name,
        (ClueWidget *)g_lbl_text,    (ClueWidget *)g_inp_label,
        (ClueWidget *)g_lbl_font_size,(ClueWidget *)g_inp_font_size,
        (ClueWidget *)g_sep1,
        (ClueWidget *)g_lbl_w,       (ClueWidget *)g_inp_w,
        (ClueWidget *)g_lbl_h,       (ClueWidget *)g_inp_h,
        (ClueWidget *)g_sep2,
        (ClueWidget *)g_lbl_halign,  (ClueWidget *)g_dd_halign,
        (ClueWidget *)g_lbl_valign,  (ClueWidget *)g_dd_valign,
        (ClueWidget *)g_sep3,
        (ClueWidget *)g_chk_hexpand, (ClueWidget *)g_chk_vexpand,
    };
    for (size_t i = 0; i < sizeof(widgets)/sizeof(widgets[0]); i++) {
        if (widgets[i]) widgets[i]->base.visible = v;
    }
}

void builder_properties_refresh(void)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) {
        clue_label_set_text(g_title, "Properties");
        set_all_fields_visible(false);
        if (g_empty_hint) g_empty_hint->base.base.visible = true;
        return;
    }

    if (g_empty_hint) g_empty_hint->base.base.visible = false;
    set_all_fields_visible(true);

    BuilderNode *node = &g_state.nodes[sel];

    char title[128];
    snprintf(title, sizeof(title), "%s", node->type_name);
    clue_label_set_text(g_title, title);

    clue_text_input_set_text(g_inp_name, node->var_name);
    clue_text_input_set_text(g_inp_label, node->label);
    clue_spinbox_set_value(g_inp_w, node->w);
    clue_spinbox_set_value(g_inp_h, node->h);
    clue_spinbox_set_value(g_inp_font_size, node->font_size);
    clue_dropdown_set_selected(g_dd_halign, node->h_align);
    clue_dropdown_set_selected(g_dd_valign, node->v_align);

    /* Show/hide fields based on widget type */
    const char *t = node->type_name;
    bool has_text = (strcmp(t, "Button") == 0 ||
                     strcmp(t, "Label") == 0 ||
                     strcmp(t, "Text Input") == 0 ||
                     strcmp(t, "Checkbox") == 0 ||
                     strcmp(t, "Toggle") == 0 ||
                     strcmp(t, "Dropdown") == 0);

    bool has_font = (strcmp(t, "Label") == 0 ||
                     strcmp(t, "Button") == 0);

    /* Width/height meaningless for auto-sized widgets */
    bool has_size = !(strcmp(t, "Label") == 0 ||
                      strcmp(t, "Checkbox") == 0 ||
                      strcmp(t, "Toggle") == 0 ||
                      strcmp(t, "Radio") == 0 ||
                      strcmp(t, "Separator") == 0);

    set_field_visible((ClueWidget *)g_lbl_text, (ClueWidget *)g_inp_label, has_text);
    set_field_visible((ClueWidget *)g_lbl_font_size, (ClueWidget *)g_inp_font_size, has_font);
    set_field_visible((ClueWidget *)g_lbl_w, (ClueWidget *)g_inp_w, has_size);
    set_field_visible((ClueWidget *)g_lbl_h, (ClueWidget *)g_inp_h, has_size);

    /* Sync expand checkboxes */
    if (clue_checkbox_is_checked(g_chk_hexpand) != node->hexpand)
        clue_checkbox_set_checked(g_chk_hexpand, node->hexpand);
    if (clue_checkbox_is_checked(g_chk_vexpand) != node->vexpand)
        clue_checkbox_set_checked(g_chk_vexpand, node->vexpand);
}
