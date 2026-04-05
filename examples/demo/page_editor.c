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
        "// Try typing, selecting, copy/paste, undo/redo\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "\n"
        "typedef struct {\n"
        "    char *name;\n"
        "    int   age;\n"
        "} Person;\n"
        "\n"
        "Person *person_new(const char *name, int age) {\n"
        "    Person *p = malloc(sizeof(Person));\n"
        "    p->name = strdup(name);\n"
        "    p->age = age;\n"
        "    return p;\n"
        "}\n"
        "\n"
        "void person_free(Person *p) {\n"
        "    free(p->name);\n"
        "    free(p);\n"
        "}\n"
        "\n"
        "int main(void) {\n"
        "    Person *alice = person_new(\"Alice\", 30);\n"
        "    printf(\"%s is %d years old\\n\", alice->name, alice->age);\n"
        "    person_free(alice);\n"
        "    return 0;\n"
        "}\n");

    clue_container_add(page, lbl);
    clue_container_add(page, ed);

    return page;
}
