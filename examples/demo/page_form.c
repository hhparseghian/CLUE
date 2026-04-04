#include "demo.h"

ClueBox *build_grid_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueLabel *lbl = clue_label_new("Form grid layout:");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);

    ClueGrid *grid = clue_grid_new(2, 8, 12);
    grid->base.base.w = 360;
    clue_grid_set_col_width(grid, 0, 80);

    ClueLabel *l1 = clue_label_new("Name:");
    l1->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueTextInput *i1 = clue_text_input_new("Enter name");
    i1->base.base.w = 240;

    ClueLabel *l2 = clue_label_new("Email:");
    l2->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueTextInput *i2 = clue_text_input_new("Enter email");
    i2->base.base.w = 240;

    ClueLabel *l3 = clue_label_new("Role:");
    l3->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueDropdown *dd = clue_dropdown_new("Select...");
    dd->base.base.w = 240;
    clue_dropdown_add_item(dd, "Developer");
    clue_dropdown_add_item(dd, "Designer");
    clue_dropdown_add_item(dd, "Manager");

    clue_container_add(grid, l1);
    clue_container_add(grid, i1);
    clue_container_add(grid, l2);
    clue_container_add(grid, i2);
    clue_container_add(grid, l3);
    clue_container_add(grid, dd);

    clue_container_add(page, lbl);
    clue_container_add(page, grid);

    return page;
}
