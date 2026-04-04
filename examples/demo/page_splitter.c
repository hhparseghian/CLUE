#include "demo.h"

ClueBox *build_splitter_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 0);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;

    ClueSplitter *split = clue_splitter_new(CLUE_HORIZONTAL);
    split->base.base.w = 600;
    split->base.base.h = 400;
    split->base.style.hexpand = true;
    split->base.style.vexpand = true;
    clue_splitter_set_ratio(split, 0.35f);

    /* Left pane: a list */
    ClueBox *left = clue_box_new(CLUE_VERTICAL, 6);
    clue_style_set_padding(&left->base.style, 8);
    ClueLabel *left_title = clue_label_new("Left Pane");
    left_title->base.style.fg_color = UI_RGB(255, 255, 255);
    ClueListView *lv = clue_listview_new();
    lv->base.base.w = 200;
    lv->base.base.h = 300;
    lv->base.style.hexpand = true;
    lv->base.style.vexpand = true;
    clue_listview_set_data(lv, 20, fruit_item, NULL);
    clue_container_add(left, left_title);
    clue_container_add(left, lv);

    /* Right pane: an editor */
    ClueBox *right = clue_box_new(CLUE_VERTICAL, 6);
    clue_style_set_padding(&right->base.style, 8);
    ClueLabel *right_title = clue_label_new("Right Pane");
    right_title->base.style.fg_color = UI_RGB(255, 255, 255);
    ClueTextEditor *ed = clue_text_editor_new();
    ed->base.base.w = 300;
    ed->base.base.h = 300;
    ed->base.style.hexpand = true;
    ed->base.style.vexpand = true;
    clue_text_editor_set_line_numbers(ed, true);
    clue_text_editor_set_text(ed, "// Drag the divider to resize panes\n");
    clue_container_add(right, right_title);
    clue_container_add(right, ed);

    clue_container_add(split, left);
    clue_container_add(split, right);
    clue_container_add(page, split);

    return page;
}
