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

## Getting started

### Install dependencies

```bash
./build.sh --install-deps
```

### Build

```bash
./build.sh
```

Options:
- `--wayland-only` — build only the Wayland backend
- `--x11-only` — build only the X11 backend
- `--drm-only` — build only the DRM/KMS backend
- `--no-wayland`, `--no-x11`, `--no-drm` — disable individual backends
- `--clean` — clean rebuild
- `-j N` — parallel jobs

### Install

```bash
cd build
sudo make install
```

This installs:
- `libclue.a` to `/usr/local/lib/`
- Headers to `/usr/local/include/clue/`
- `clue.pc` to `/usr/local/lib/pkgconfig/`

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

    clue_container_add(vbox, label);
    clue_container_add(vbox, btn);

    clue_app_set_root(app, vbox);
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
