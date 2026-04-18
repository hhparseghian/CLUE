#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "clue/file_dialog.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/draw.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/box.h"
#include "clue/label.h"
#include "clue/button.h"
#include "clue/text_input.h"
#include "clue/listview.h"
#include "clue/separator.h"
#include "clue/dropdown.h"
#include "clue/signal.h"
#include "clue/overlay.h"
#include "clue/clue.h"

#define MAX_ENTRIES 512
#define DIALOG_W    500
#define DIALOG_H    450
#define DIALOG_H_FILTERS 600

/* ------------------------------------------------------------------ */
/* Internal state for the file dialog                                  */
/* ------------------------------------------------------------------ */

typedef struct {
    /* Widgets */
    ClueBox       *root;
    ClueLabel     *path_label;
    ClueListView  *list;
    ClueTextInput *filename_input;
    ClueButton    *btn_ok;
    ClueButton    *btn_cancel;
    ClueButton    *btn_up;
    ClueDropdown  *filter_dd;

    /* File entries */
    char          *entries[MAX_ENTRIES];
    bool           is_dir[MAX_ENTRIES];
    int            entry_count;

    /* Filters */
    const ClueFileFilter *filters;
    int            filter_count;
    int            active_filter;  /* -1 = all files */

    /* State */
    char           current_dir[1024];
    char           selected_path[1024];
    ClueFileDialogMode mode;
    bool           running;
    bool           ok;
    ClueFileDialogResult result; /* used by overlay variant for deferred cleanup */
} FileDialogState;

/* ------------------------------------------------------------------ */
/* Directory scanning                                                  */
/* ------------------------------------------------------------------ */

static int entry_cmp(const void *a, const void *b)
{
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    /* Directories first (they start with /) ... actually let's use is_dir */
    /* Simple alphabetical sort */
    return strcmp(sa, sb);
}

static void clear_entries(FileDialogState *s)
{
    for (int i = 0; i < s->entry_count; i++)
        free(s->entries[i]);
    s->entry_count = 0;
}

static const char *active_filter_pattern(FileDialogState *s)
{
    if (!s->filters || s->active_filter < 0 ||
        s->active_filter >= s->filter_count)
        return NULL; /* all files */
    return s->filters[s->active_filter].pattern;
}

static bool matches_filter(const char *name, const char *pattern)
{
    if (!pattern || !pattern[0]) return true;

    const char *dot = strrchr(name, '.');
    if (!dot) return false;

    /* Check each space-separated extension in pattern */
    const char *p = pattern;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        const char *end = p;
        while (*end && *end != ' ') end++;

        int len = (int)(end - p);
        if ((int)strlen(dot) == len && strncmp(dot, p, len) == 0)
            return true;

        p = end;
    }
    return false;
}

static void scan_directory(FileDialogState *s)
{
    clear_entries(s);

    DIR *dir = opendir(s->current_dir);
    if (!dir) return;

    /* Separate dirs and files for sorting */
    char *dirs[MAX_ENTRIES];
    char *files[MAX_ENTRIES];
    bool  dirs_isdir[MAX_ENTRIES];
    bool  files_isdir[MAX_ENTRIES];
    int   dir_count = 0, file_count = 0;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (dir_count + file_count >= MAX_ENTRIES - 1) break;

        /* Skip "." */
        if (strcmp(ent->d_name, ".") == 0) continue;

        /* Build full path for stat */
        char fullpath[2048];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", s->current_dir, ent->d_name);

        struct stat st;
        if (stat(fullpath, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            dirs[dir_count] = strdup(ent->d_name);
            dirs_isdir[dir_count] = true;
            dir_count++;
        } else {
            if (!matches_filter(ent->d_name, active_filter_pattern(s))) continue;
            files[file_count] = strdup(ent->d_name);
            files_isdir[file_count] = false;
            file_count++;
        }
    }
    closedir(dir);

    /* Sort each group */
    qsort(dirs, dir_count, sizeof(char *), entry_cmp);
    qsort(files, file_count, sizeof(char *), entry_cmp);

    /* Merge: dirs first, then files */
    for (int i = 0; i < dir_count && s->entry_count < MAX_ENTRIES; i++) {
        s->entries[s->entry_count] = dirs[i];
        s->is_dir[s->entry_count] = true;
        s->entry_count++;
    }
    for (int i = 0; i < file_count && s->entry_count < MAX_ENTRIES; i++) {
        s->entries[s->entry_count] = files[i];
        s->is_dir[s->entry_count] = false;
        s->entry_count++;
    }
}

/* ------------------------------------------------------------------ */
/* List view callback                                                  */
/* ------------------------------------------------------------------ */

static FileDialogState *g_fds = NULL;

static const char *list_item_cb(int index, void *user_data)
{
    FileDialogState *s = (FileDialogState *)user_data;
    if (index < 0 || index >= s->entry_count) return "";
    return s->entries[index];
}

/* ------------------------------------------------------------------ */
/* Icon draw callback                                                  */
/* ------------------------------------------------------------------ */

static int file_icon_cb(int index, int x, int y, int h, void *user_data)
{
    FileDialogState *s = (FileDialogState *)user_data;
    if (index < 0 || index >= s->entry_count) return 0;

    int icon_size = 14;
    int ix = x + 2;
    int iy = y + (h - icon_size) / 2;

    if (s->is_dir[index]) {
        /* Folder icon: yellow rectangle with a tab */
        ClueColor folder = CLUE_RGB(220, 180, 50);
        clue_fill_rounded_rect(ix, iy + 3, icon_size, icon_size - 3, 2.0f, folder);
        clue_fill_rounded_rect(ix, iy, icon_size / 2, 4, 1.0f, folder);
    } else {
        /* File icon: white/gray document outline */
        ClueColor file_bg = CLUE_RGB(180, 185, 195);
        ClueColor fold = CLUE_RGB(140, 145, 155);
        clue_fill_rounded_rect(ix, iy, icon_size - 2, icon_size, 1.5f, file_bg);
        /* Dog-ear fold in top-right */
        int fx = ix + icon_size - 6;
        clue_fill_rect(fx, iy, 4, 4, fold);
    }

    return icon_size + 6;
}

/* ------------------------------------------------------------------ */
/* Widget callbacks                                                    */
/* ------------------------------------------------------------------ */

/* Truncate path from the left if too long for the dialog width.
 * Shows ".../<tail>" when the full path won't fit. */
static void set_path_label(FileDialogState *s)
{
    ClueFont *font = clue_app_default_font();
    int max_w = DIALOG_W - 40;  /* padding + some margin */
    const char *path = s->current_dir;

    if (font && clue_font_text_width(font, path) > max_w) {
        /* Walk forward through path separators until it fits */
        const char *p = path;
        while (*p) {
            const char *slash = strchr(p + 1, '/');
            if (!slash) break;
            char buf[1024];
            snprintf(buf, sizeof(buf), "...%s", slash);
            if (clue_font_text_width(font, buf) <= max_w) {
                clue_label_set_text(s->path_label, buf);
                return;
            }
            p = slash;
        }
    }
    clue_label_set_text(s->path_label, path);
}

static void navigate_to(FileDialogState *s, const char *dir)
{
    if (realpath(dir, s->current_dir) == NULL) {
        strncpy(s->current_dir, dir, sizeof(s->current_dir) - 1);
    }
    set_path_label(s);
    scan_directory(s);
    clue_listview_set_data(s->list, s->entry_count, list_item_cb, s);
    clue_listview_set_selected(s->list, -1);
    s->list->scroll_y = 0;
}

static void on_list_selected(void *w, void *d)
{
    FileDialogState *s = (FileDialogState *)d;
    int idx = clue_listview_get_selected(s->list);
    if (idx < 0 || idx >= s->entry_count) return;

    if (s->is_dir[idx]) {
        /* Navigate into directory */
        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", s->current_dir, s->entries[idx]);
        navigate_to(s, path);
    } else {
        /* File selected: put name in input */
        clue_text_input_set_text(s->filename_input, s->entries[idx]);
    }
}

static void on_up(void *w, void *d)
{
    FileDialogState *s = (FileDialogState *)d;
    char path[2048];
    snprintf(path, sizeof(path), "%s/..", s->current_dir);
    navigate_to(s, path);
}

static void on_ok(void *w, void *d)
{
    FileDialogState *s = (FileDialogState *)d;
    const char *fname = clue_text_input_get_text(s->filename_input);
    if (fname && fname[0]) {
        int n = snprintf(s->selected_path, sizeof(s->selected_path),
                         "%s/%s", s->current_dir, fname);
        (void)n;
        s->ok = true;
    }
    s->running = false;
}

static void on_cancel(void *w, void *d)
{
    FileDialogState *s = (FileDialogState *)d;
    s->ok = false;
    s->running = false;
}

static void on_filter_changed(void *w, void *d);

/* ------------------------------------------------------------------ */
/* Build the dialog UI                                                 */
/* ------------------------------------------------------------------ */

static void build_ui(FileDialogState *s)
{
    const ClueTheme *th = clue_theme_get();

    s->root = clue_box_new(CLUE_VERTICAL, 8);
    clue_style_set_padding(&s->root->base.style, 12);
    s->root->base.style.hexpand = true;
    s->root->base.style.vexpand = true;

    /* Top bar: Up button + path label */
    /* Top: Up button + path label on separate rows so path can wrap */
    s->btn_up = clue_button_new("Up");
    clue_signal_connect(s->btn_up, "clicked", on_up, s);

    s->path_label = clue_label_new("");
    s->path_label->base.style.fg_color = th->fg;
    s->path_label->base.style.hexpand = true;
    set_path_label(s);

    /* File list */
    s->list = clue_listview_new();
    s->list->base.base.h = 280;
    s->list->base.style.hexpand = true;
    s->list->base.style.vexpand = true;
    s->list->icon_cb = file_icon_cb;
    clue_listview_set_data(s->list, s->entry_count, list_item_cb, s);
    clue_signal_connect(s->list, "selected", on_list_selected, s);

    /* Filename input */
    s->filename_input = clue_text_input_new(
        s->mode == CLUE_FILE_SAVE ? "Enter filename..." : "Select a file...");
    s->filename_input->base.style.hexpand = true;

    /* Separator */
    ClueSeparator *sep = clue_separator_new(CLUE_HORIZONTAL);

    /* Button row */
    ClueBox *btn_row = clue_box_new(CLUE_HORIZONTAL, 8);
    btn_row->base.style.h_align = CLUE_ALIGN_END;
    btn_row->base.style.hexpand = true;

    s->btn_cancel = clue_button_new("Cancel");
    clue_signal_connect(s->btn_cancel, "clicked", on_cancel, s);

    const char *ok_label = s->mode == CLUE_FILE_SAVE ? "Save" : "Open";
    s->btn_ok = clue_button_new(ok_label);
    clue_signal_connect(s->btn_ok, "clicked", on_ok, s);

    clue_container_add(btn_row, s->btn_cancel);
    clue_container_add(btn_row, s->btn_ok);

    /* Filter dropdown (if filters provided) */
    if (s->filters && s->filter_count > 0) {
        s->filter_dd = clue_dropdown_new("All Files");
        s->filter_dd->base.style.hexpand = true;
        s->filter_dd->max_visible = 3;
        for (int i = 0; i < s->filter_count; i++) {
            char label[256];
            snprintf(label, sizeof(label), "%s (%s)",
                     s->filters[i].name, s->filters[i].pattern);
            clue_dropdown_add_item(s->filter_dd, label);
        }
        clue_dropdown_add_item(s->filter_dd, "All Files");
        clue_dropdown_set_selected(s->filter_dd, 0);
        clue_signal_connect(s->filter_dd, "changed", on_filter_changed, s);
    }

    /* Assemble */
    clue_container_add(s->root, s->btn_up);
    clue_container_add(s->root, s->path_label);
    if (s->filter_dd)
        clue_container_add(s->root, s->filter_dd);
    clue_container_add(s->root, s->list);
    clue_container_add(s->root, s->filename_input);
    clue_container_add(s->root, sep);
    clue_container_add(s->root, btn_row);
}

/* ------------------------------------------------------------------ */
/* Run the file dialog                                                 */
/* ------------------------------------------------------------------ */

static void on_filter_changed(void *w, void *d)
{
    FileDialogState *s = (FileDialogState *)d;
    int idx = clue_dropdown_get_selected(s->filter_dd);
    /* Last item is always "All Files" */
    if (idx >= s->filter_count)
        s->active_filter = -1;
    else
        s->active_filter = idx;
    /* Rescan with new filter */
    scan_directory(s);
    clue_listview_set_data(s->list, s->entry_count, list_item_cb, s);
    clue_listview_set_selected(s->list, -1);
    s->list->scroll_y = 0;
}

static ClueFileDialogResult run_file_dialog(ClueFileDialogMode mode,
                                            const char *title,
                                            const char *start_dir,
                                            const char *default_filename,
                                            const ClueFileFilter *filters,
                                            int filter_count)
{
    ClueFileDialogResult result = {.ok = false};

    ClueApp *app = clue_app_get();
    if (!app) return result;

    FileDialogState state = {0};
    state.mode = mode;
    state.running = true;
    state.ok = false;
    state.filters = filters;
    state.filter_count = filter_count;
    state.active_filter = filter_count > 0 ? 0 : -1;
    g_fds = &state;

    /* Set starting directory */
    if (start_dir && start_dir[0]) {
        if (realpath(start_dir, state.current_dir) == NULL)
            strncpy(state.current_dir, start_dir, sizeof(state.current_dir) - 1);
    } else {
        if (getcwd(state.current_dir, sizeof(state.current_dir)) == NULL)
            snprintf(state.current_dir, sizeof(state.current_dir), "/");
    }

    scan_directory(&state);
    build_ui(&state);

    /* Pre-fill filename for save mode */
    if (mode == CLUE_FILE_SAVE && default_filename && default_filename[0]) {
        clue_text_input_set_text(state.filename_input, default_filename);
    }

    /* Create window */
    int dlg_h = (filter_count > 0) ? DIALOG_H_FILTERS : DIALOG_H;
    ClueWindow *win = clue_window_create(DIALOG_W, dlg_h,
                                        title ? title : "File");
    if (!win) {
        clear_entries(&state);
        clue_cwidget_destroy(state.root);
        return result;
    }

    clue_window_set_type(win, CLUE_WINDOW_DIALOG);
    clue_window_set_parent(win, app->window);

    /* Nested event loop */
    while (state.running) {
        ClueEvent events[32];
        int count = clue_poll_events(events, 32);

        for (int i = 0; i < count; i++) {
            if (events[i].type == CLUE_EVENT_CLOSE && events[i].window == win) {
                state.running = false;
                break;
            }
            if (events[i].type == CLUE_EVENT_CLOSE && events[i].window == app->window) {
                state.running = false;
                app->running = false;
                break;
            }

            if (events[i].window == win &&
                events[i].type == CLUE_EVENT_MOUSE_MOVE) {
                clue_window_set_cursor(win, CLUE_CURSOR_DEFAULT);
            }

            if (events[i].window == win) {
                if (app->captured_widget &&
                    (events[i].type == CLUE_EVENT_MOUSE_MOVE ||
                     events[i].type == CLUE_EVENT_MOUSE_BUTTON ||
                     events[i].type == CLUE_EVENT_MOUSE_SCROLL)) {
                    clue_widget_dispatch_event(app->captured_widget, &events[i]);
                } else {
                    clue_widget_dispatch_event(&state.root->base.base, &events[i]);
                }
            }
        }

        /* Render main window */
        if (app->root) {
            clue_window_make_current(app->window);
            app->renderer->begin_frame(app->window);
            const ClueTheme *th = clue_theme_get();
            app->renderer->clear(th->bg.r, th->bg.g, th->bg.b, th->bg.a);
            clue_cwidget_draw_tree(app->root);
            app->renderer->end_frame(app->window);
            clue_window_swap_buffers(app->window);
        }

        /* Render file dialog */
        clue_window_make_current(win);
        app->renderer->begin_frame(win);
        const ClueTheme *th = clue_theme_get();
        app->renderer->clear(th->surface.r, th->surface.g, th->surface.b, 1.0f);

        state.root->base.base.x = 0;
        state.root->base.base.y = 0;
        state.root->base.base.w = win->w;
        state.root->base.base.h = win->h;
        clue_cwidget_layout_tree(state.root);
        clue_cwidget_draw_tree(state.root);

        /* Draw dropdown overlay if open */
        if (state.filter_dd) {
            clue_reset_clip_rect();
            clue_dropdown_draw_overlay(state.filter_dd);
        }

        app->renderer->end_frame(win);
        clue_window_swap_buffers(win);
    }

    clue_window_destroy(win);

    if (state.ok && state.selected_path[0]) {
        strncpy(result.path, state.selected_path, sizeof(result.path) - 1);
        result.ok = true;
    }

    clear_entries(&state);
    clue_cwidget_destroy(state.root);
    g_fds = NULL;

    return result;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

ClueFileDialogResult clue_file_dialog_open(const char *title,
                                           const char *start_dir,
                                           const ClueFileFilter *filters,
                                           int filter_count)
{
    return run_file_dialog(CLUE_FILE_OPEN,
                           title ? title : "Open File",
                           start_dir, NULL,
                           filters, filter_count);
}

ClueFileDialogResult clue_file_dialog_save(const char *title,
                                           const char *start_dir,
                                           const char *filename,
                                           const ClueFileFilter *filters,
                                           int filter_count)
{
    return run_file_dialog(CLUE_FILE_SAVE,
                           title ? title : "Save File",
                           start_dir, filename,
                           filters, filter_count);
}

/* ================================================================== */
/* Overlay-based file dialog (non-blocking, for DRM / single-window)  */
/* ================================================================== */

typedef struct {
    FileDialogState       fds;
    ClueOverlay          *overlay;
    ClueFileDialogCallback callback;
    void                 *user_data;
} OverlayFileDialog;

static OverlayFileDialog *g_ofd = NULL;

static void ov_on_list_selected(void *w, void *d)
{
    OverlayFileDialog *ofd = (OverlayFileDialog *)d;
    FileDialogState *s = &ofd->fds;
    int idx = clue_listview_get_selected(s->list);
    if (idx < 0 || idx >= s->entry_count) return;

    if (s->is_dir[idx]) {
        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", s->current_dir, s->entries[idx]);
        navigate_to(s, path);
    } else {
        clue_text_input_set_text(s->filename_input, s->entries[idx]);
    }
}

static void ov_on_up(void *w, void *d)
{
    OverlayFileDialog *ofd = (OverlayFileDialog *)d;
    FileDialogState *s = &ofd->fds;
    char path[2048];
    snprintf(path, sizeof(path), "%s/..", s->current_dir);
    navigate_to(s, path);
}

static void ov_on_filter_changed(void *w, void *d)
{
    OverlayFileDialog *ofd = (OverlayFileDialog *)d;
    FileDialogState *s = &ofd->fds;
    int idx = clue_dropdown_get_selected(s->filter_dd);
    if (idx >= s->filter_count)
        s->active_filter = -1;
    else
        s->active_filter = idx;
    scan_directory(s);
    clue_listview_set_data(s->list, s->entry_count, list_item_cb, s);
    clue_listview_set_selected(s->list, -1);
    s->list->scroll_y = 0;
}

/* Deferred cleanup -- runs on next timer tick, after event dispatch is done */
static bool ov_deferred_cleanup(void *data)
{
    OverlayFileDialog *ofd = (OverlayFileDialog *)data;
    FileDialogState *s = &ofd->fds;

    clear_entries(s);
    clue_overlay_destroy(ofd->overlay);

    ClueFileDialogCallback cb = ofd->callback;
    void *ud = ofd->user_data;
    ClueFileDialogResult result = ofd->fds.result;
    free(ofd);

    if (cb) cb(&result, ud);
    return false;
}

static void ov_finish(OverlayFileDialog *ofd, bool ok)
{
    if (!ofd || ofd != g_ofd) return; /* guard re-entry */
    g_ofd = NULL;

    FileDialogState *s = &ofd->fds;
    s->result = (ClueFileDialogResult){.ok = false};

    if (ok) {
        const char *fname = clue_text_input_get_text(s->filename_input);
        if (fname && fname[0]) {
            char tmp[2048];
            snprintf(tmp, sizeof(tmp), "%s/%s", s->current_dir, fname);
            strncpy(s->result.path, tmp, sizeof(s->result.path) - 1);
            s->result.path[sizeof(s->result.path) - 1] = '\0';
            s->result.ok = true;
        }
    }

    /* Dismiss but don't destroy yet -- we're still inside the button handler */
    ofd->overlay->callback = NULL;
    clue_overlay_dismiss(ofd->overlay, CLUE_OVERLAY_CANCEL);

    /* Defer destruction to next tick */
    clue_timer_once(0, ov_deferred_cleanup, ofd);
}

static void ov_on_dismiss(ClueOverlayResult result, void *user_data)
{
    OverlayFileDialog *ofd = (OverlayFileDialog *)user_data;
    if (!ofd) return;
    ov_finish(ofd, result == CLUE_OVERLAY_OK);
}

static void run_file_dialog_overlay(ClueFileDialogMode mode,
                                    const char *title,
                                    const char *start_dir,
                                    const char *default_filename,
                                    const ClueFileFilter *filters,
                                    int filter_count,
                                    ClueFileDialogCallback callback,
                                    void *user_data)
{
    ClueApp *app = clue_app_get();
    if (!app) return;

    OverlayFileDialog *ofd = calloc(1, sizeof(OverlayFileDialog));
    if (!ofd) return;

    FileDialogState *s = &ofd->fds;
    s->mode = mode;
    s->running = true;
    s->filters = filters;
    s->filter_count = filter_count;
    s->active_filter = filter_count > 0 ? 0 : -1;
    ofd->callback = callback;
    ofd->user_data = user_data;
    g_ofd = ofd;

    /* Set starting directory */
    if (start_dir && start_dir[0]) {
        if (realpath(start_dir, s->current_dir) == NULL)
            strncpy(s->current_dir, start_dir, sizeof(s->current_dir) - 1);
    } else {
        if (getcwd(s->current_dir, sizeof(s->current_dir)) == NULL)
            snprintf(s->current_dir, sizeof(s->current_dir), "/");
    }

    scan_directory(s);

    /* Build UI into a box */
    s->root = clue_box_new(CLUE_VERTICAL, 8);
    clue_style_set_padding(&s->root->base.style, 12);
    s->root->base.style.hexpand = true;
    s->root->base.style.vexpand = true;

    s->btn_up = clue_button_new("Up");
    clue_signal_connect(s->btn_up, "clicked", ov_on_up, ofd);

    s->path_label = clue_label_new("");
    s->path_label->base.style.hexpand = true;
    set_path_label(s);

    s->list = clue_listview_new();
    s->list->base.style.hexpand = true;
    s->list->base.style.vexpand = true;
    s->list->icon_cb = file_icon_cb;
    clue_listview_set_data(s->list, s->entry_count, list_item_cb, s);
    clue_signal_connect(s->list, "selected", ov_on_list_selected, ofd);

    s->filename_input = clue_text_input_new(
        mode == CLUE_FILE_SAVE ? "Enter filename..." : "Select a file...");
    s->filename_input->base.style.hexpand = true;

    if (mode == CLUE_FILE_SAVE && default_filename && default_filename[0])
        clue_text_input_set_text(s->filename_input, default_filename);

    /* Filter dropdown */
    if (filters && filter_count > 0) {
        s->filter_dd = clue_dropdown_new("All Files");
        s->filter_dd->base.style.hexpand = true;
        s->filter_dd->max_visible = 3;
        for (int i = 0; i < filter_count; i++) {
            char label[256];
            snprintf(label, sizeof(label), "%s (%s)",
                     filters[i].name, filters[i].pattern);
            clue_dropdown_add_item(s->filter_dd, label);
        }
        clue_dropdown_add_item(s->filter_dd, "All Files");
        clue_dropdown_set_selected(s->filter_dd, 0);
        clue_signal_connect(s->filter_dd, "changed", ov_on_filter_changed, ofd);
    }

    /* Assemble content */
    clue_container_add(s->root, s->btn_up);
    clue_container_add(s->root, s->path_label);
    if (s->filter_dd)
        clue_container_add(s->root, s->filter_dd);
    clue_container_add(s->root, s->list);
    clue_container_add(s->root, s->filename_input);

    /* Create overlay */
    int dlg_h = (filter_count > 0) ? DIALOG_H_FILTERS : DIALOG_H;
    ofd->overlay = clue_overlay_new(title ? title : "File", DIALOG_W, dlg_h);

    clue_overlay_set_content(ofd->overlay, (ClueWidget *)s->root);

    const char *ok_label = mode == CLUE_FILE_SAVE ? "Save" : "Open";
    clue_overlay_add_button(ofd->overlay, ok_label, CLUE_OVERLAY_OK);
    clue_overlay_add_button(ofd->overlay, "Cancel", CLUE_OVERLAY_CANCEL);
    clue_overlay_set_callback(ofd->overlay, ov_on_dismiss, ofd);

    clue_overlay_show(ofd->overlay);
}

/* ------------------------------------------------------------------ */
/* Public API -- overlay variants                                      */
/* ------------------------------------------------------------------ */

void clue_file_dialog_open_overlay(const char *title,
                                   const char *start_dir,
                                   const ClueFileFilter *filters,
                                   int filter_count,
                                   ClueFileDialogCallback callback,
                                   void *user_data)
{
    run_file_dialog_overlay(CLUE_FILE_OPEN,
                            title ? title : "Open File",
                            start_dir, NULL,
                            filters, filter_count,
                            callback, user_data);
}

void clue_file_dialog_save_overlay(const char *title,
                                   const char *start_dir,
                                   const char *filename,
                                   const ClueFileFilter *filters,
                                   int filter_count,
                                   ClueFileDialogCallback callback,
                                   void *user_data)
{
    run_file_dialog_overlay(CLUE_FILE_SAVE,
                            title ? title : "Save File",
                            start_dir, filename,
                            filters, filter_count,
                            callback, user_data);
}
