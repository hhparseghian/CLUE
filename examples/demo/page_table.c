#include "demo.h"

static const char *people[][3] = {
    {"Alice",   "alice@example.com",   "Developer"},
    {"Bob",     "bob@example.com",     "Designer"},
    {"Charlie", "charlie@example.com", "Manager"},
    {"Diana",   "diana@example.com",   "Developer"},
    {"Eve",     "eve@example.com",     "Analyst"},
    {"Frank",   "frank@example.com",   "Developer"},
    {"Grace",   "grace@example.com",   "Designer"},
    {"Hank",    "hank@example.com",    "Manager"},
};

static const char *table_cell(int row, int col, void *user_data)
{
    (void)user_data;
    if (row < 0 || row >= 8 || col < 0 || col >= 3) return "";
    return people[row][col];
}

static void on_table_selected(void *w, void *d)
{
    (void)d;
    int row = clue_table_get_selected(w);
    if (row >= 0 && row < 8) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Table: %s (%s)", people[row][0], people[row][2]);
        clue_label_set_text(g_status, buf);
    }
}

ClueBox *build_table_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueTable *tbl = clue_table_new();
    clue_table_add_column(tbl, "Name", 120);
    clue_table_add_column(tbl, "Email", 200);
    clue_table_add_column(tbl, "Role", 100);
    tbl->base.base.h = 280;
    clue_table_set_data(tbl, 8, table_cell, NULL);
    clue_signal_connect(tbl, "selected", on_table_selected, NULL);

    clue_container_add(page, tbl);
    return page;
}
