#include "demo.h"

static void on_list_selected(ClueListView *list, void *data)
{
    int idx = clue_listview_get_selected(list);
    char buf[64];
    snprintf(buf, sizeof(buf), "Fruit: %s", idx >= 0 ? fruits[idx] : "none");
    clue_label_set_text(g_status, buf);
}

ClueBox *build_list_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueLabel *lbl = clue_label_new("Fruit list (virtual scrolling):");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);

    ClueListView *lv = clue_listview_new();
    lv->base.base.w = 350;
    lv->base.base.h = 280;
    clue_listview_set_data(lv, 20, fruit_item, NULL);
    clue_signal_connect(lv, "selected", on_list_selected, NULL);

    clue_container_add(page, lbl);
    clue_container_add(page, lv);

    return page;
}
