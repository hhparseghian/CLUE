#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "clue/shortcut.h"
#include "clue/event.h"
#include <xkbcommon/xkbcommon-keysyms.h>

#define MAX_SHORTCUTS 64

typedef struct {
    int                 mods;       /* required UI_MOD_* flags */
    int                 key;        /* xkb keysym (lowercase) */
    ClueSignalCallback  callback;
    void               *user_data;
    bool                active;
} Shortcut;

static Shortcut g_shortcuts[MAX_SHORTCUTS];

/* Parse "Ctrl+Shift+S" into mods + key */
static int parse_shortcut(const char *str, int *out_mods, int *out_key)
{
    if (!str || !str[0]) return 0;
    *out_mods = 0;
    *out_key = 0;

    const char *p = str;
    while (*p) {
        if (strncmp(p, "Ctrl+", 5) == 0)       { *out_mods |= UI_MOD_CTRL;  p += 5; }
        else if (strncmp(p, "Shift+", 6) == 0)  { *out_mods |= UI_MOD_SHIFT; p += 6; }
        else if (strncmp(p, "Alt+", 4) == 0)    { *out_mods |= UI_MOD_ALT;   p += 4; }
        else if (strncmp(p, "Super+", 6) == 0)  { *out_mods |= UI_MOD_SUPER; p += 6; }
        else break;
    }

    /* Single letter */
    if (strlen(p) == 1 && isalpha((unsigned char)*p)) {
        *out_key = XKB_KEY_a + (tolower((unsigned char)*p) - 'a');
        return 1;
    }

    /* Function keys */
    if (p[0] == 'F' && isdigit((unsigned char)p[1])) {
        int n = atoi(p + 1);
        if (n >= 1 && n <= 12) {
            *out_key = XKB_KEY_F1 + (n - 1);
            return 1;
        }
    }

    /* Named keys */
    if (strcmp(p, "Delete") == 0)       { *out_key = XKB_KEY_Delete; return 1; }
    if (strcmp(p, "Escape") == 0)       { *out_key = XKB_KEY_Escape; return 1; }
    if (strcmp(p, "Return") == 0)       { *out_key = XKB_KEY_Return; return 1; }
    if (strcmp(p, "Enter") == 0)        { *out_key = XKB_KEY_Return; return 1; }
    if (strcmp(p, "Tab") == 0)          { *out_key = XKB_KEY_Tab; return 1; }
    if (strcmp(p, "Space") == 0)        { *out_key = XKB_KEY_space; return 1; }
    if (strcmp(p, "Backspace") == 0)    { *out_key = XKB_KEY_BackSpace; return 1; }
    if (strcmp(p, "Home") == 0)         { *out_key = XKB_KEY_Home; return 1; }
    if (strcmp(p, "End") == 0)          { *out_key = XKB_KEY_End; return 1; }
    if (strcmp(p, "PageUp") == 0)       { *out_key = XKB_KEY_Page_Up; return 1; }
    if (strcmp(p, "PageDown") == 0)     { *out_key = XKB_KEY_Page_Down; return 1; }

    return 0;
}

void clue_shortcut_add(const char *shortcut, ClueSignalCallback callback,
                       void *user_data)
{
    if (!shortcut || !callback) return;
    int mods, key;
    if (!parse_shortcut(shortcut, &mods, &key)) return;

    for (int i = 0; i < MAX_SHORTCUTS; i++) {
        if (!g_shortcuts[i].active) {
            g_shortcuts[i].mods = mods;
            g_shortcuts[i].key = key;
            g_shortcuts[i].callback = callback;
            g_shortcuts[i].user_data = user_data;
            g_shortcuts[i].active = true;
            return;
        }
    }
}

void clue_shortcut_remove(ClueSignalCallback callback)
{
    for (int i = 0; i < MAX_SHORTCUTS; i++) {
        if (g_shortcuts[i].active && g_shortcuts[i].callback == callback)
            g_shortcuts[i].active = false;
    }
}

void clue_shortcut_clear(void)
{
    for (int i = 0; i < MAX_SHORTCUTS; i++)
        g_shortcuts[i].active = false;
}

int clue_shortcut_dispatch(int keycode, int modifiers)
{
    /* Normalize uppercase keysym to lowercase */
    int lk = keycode;
    if (lk >= XKB_KEY_A && lk <= XKB_KEY_Z)
        lk = lk - XKB_KEY_A + XKB_KEY_a;

    for (int i = 0; i < MAX_SHORTCUTS; i++) {
        Shortcut *s = &g_shortcuts[i];
        if (!s->active) continue;
        if (s->key != lk) continue;
        /* Check modifiers match exactly */
        if ((modifiers & (UI_MOD_CTRL | UI_MOD_SHIFT | UI_MOD_ALT | UI_MOD_SUPER)) != s->mods)
            continue;
        s->callback(s->user_data, s->user_data);
        return 1;
    }
    return 0;
}
