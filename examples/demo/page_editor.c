#include "demo.h"

ClueBox *build_editor_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;

    ClueLabel *lbl = clue_label_new("Multi-line text editor:");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);

    ClueTextEditor *ed = clue_text_editor_new();
    ed->base.base.w = 500;
    ed->base.base.h = 300;
    ed->base.style.hexpand = true;
    ed->base.style.vexpand = true;
    clue_text_editor_set_line_numbers(ed, true);
    clue_text_editor_set_text(ed,
        "// Welcome to the CLUE text editor\n"
        "#include <stdio.h>\n"
        "\n"
        "int main(void) {\n"
        "    printf(\"Hello, world!\\n\");\n"
        "    return 0;\n"
        "}\n");

    clue_container_add(page, lbl);
    clue_container_add(page, ed);

    return page;
}
