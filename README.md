# CLUE — Cross-platform Lightweight UI Engine

A lightweight GUI toolkit written in C99 with OpenGL ES 2 rendering and multiple display backends.

## Features

- **Widgets**: buttons, labels, text inputs, checkboxes, sliders, dropdowns, progress bars, tabs, grids, tables, tree views, list views, dialogs, tooltips, menus
- **Backends**: Wayland, X11, DRM/KMS
- **Theming**: built-in dark/light themes with full customization support
- **Signals**: GTK-style signal/callback system
- **Timers**: repeating and one-shot timers
- **Font rendering**: FreeType-based text rendering
- **Zero dependencies beyond system libs**: EGL, GLES2, FreeType, xkbcommon

## Building

```bash
mkdir build && cd build
cmake ..
make
```

### Backend options

```bash
cmake .. -DUI_BACKEND_WAYLAND=ON -DUI_BACKEND_X11=ON -DUI_BACKEND_DRM=ON
```

### Dependencies

- CMake 3.10+
- EGL and OpenGL ES 2
- FreeType2
- xkbcommon
- Wayland (optional): wayland-client, wayland-egl, wayland-protocols
- X11 (optional): libX11
- DRM (optional): libdrm, gbm, libinput, libudev

## Quick start

```c
#include <clue/clue.h>

static void on_clicked(void *widget, void *data)
{
    (void)widget;
    clue_label_set_text((ClueLabel *)data, "Button clicked!");
}

int main(void)
{
    ClueApp *app = clue_app_new("My App", 400, 300);
    if (!app) return 1;

    ClueBox *vbox = clue_box_new(CLUE_VERTICAL, 12);
    clue_style_set_padding(&vbox->base.style, 20);

    ClueLabel *label = clue_label_new("Hello, CLUE!");
    ClueButton *btn = clue_button_new("Click Me");
    clue_signal_connect(btn, "clicked", on_clicked, label);

    clue_container_add((ClueWidget *)vbox, (ClueWidget *)label);
    clue_container_add((ClueWidget *)vbox, (ClueWidget *)btn);

    clue_app_set_root(app, (ClueWidget *)vbox);
    clue_app_run(app);
    clue_app_destroy(app);
    return 0;
}
```

### Standalone project

Link against the installed library with pkg-config:

```bash
cc main.c $(pkg-config --cflags --libs clue) -o myapp
```

## License

[MIT](LICENSE)
