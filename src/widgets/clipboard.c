#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "clue/clipboard.h"

/* Internal clipboard buffer (always works within the app) */
static char *g_clipboard = NULL;

/* Try system clipboard via command-line tools */
static int try_system_set(const char *text)
{
    /* Try wl-copy first (Wayland), then xclip (X11) */
    const char *cmds[] = {
        "wl-copy 2>/dev/null",
        "xclip -selection clipboard 2>/dev/null",
        "xsel --clipboard --input 2>/dev/null",
        NULL
    };
    for (int i = 0; cmds[i]; i++) {
        FILE *p = popen(cmds[i], "w");
        if (p) {
            fputs(text, p);
            int r = pclose(p);
            if (r == 0) return 1;
        }
    }
    return 0;
}

static char *try_system_get(void)
{
    const char *cmds[] = {
        "wl-paste --no-newline 2>/dev/null",
        "xclip -selection clipboard -o 2>/dev/null",
        "xsel --clipboard --output 2>/dev/null",
        NULL
    };
    for (int i = 0; cmds[i]; i++) {
        FILE *p = popen(cmds[i], "r");
        if (p) {
            char buf[8192];
            size_t n = fread(buf, 1, sizeof(buf) - 1, p);
            int r = pclose(p);
            if (r == 0 && n > 0) {
                buf[n] = '\0';
                return strdup(buf);
            }
        }
    }
    return NULL;
}

void clue_clipboard_set(const char *text)
{
    free(g_clipboard);
    g_clipboard = text ? strdup(text) : NULL;
    if (text) try_system_set(text);
}

char *clue_clipboard_get(void)
{
    /* Try system clipboard first */
    char *sys = try_system_get();
    if (sys) return sys;
    /* Fall back to internal buffer */
    return g_clipboard ? strdup(g_clipboard) : NULL;
}
