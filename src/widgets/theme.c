#include <string.h>
#include "clue/theme.h"

/* ------------------------------------------------------------------ */
/* Dark theme                                                          */
/* ------------------------------------------------------------------ */

static ClueTheme g_dark = {
    /* Base palette */
    .bg              = {0.12f, 0.12f, 0.14f, 1.0f},
    .fg              = {0.86f, 0.86f, 0.86f, 1.0f},
    .fg_dim          = {0.55f, 0.55f, 0.60f, 1.0f},
    .fg_bright       = {1.0f,  1.0f,  1.0f,  1.0f},
    .accent          = {0.22f, 0.47f, 0.78f, 1.0f},
    .accent_hover    = {0.29f, 0.55f, 0.86f, 1.0f},
    .accent_pressed  = {0.14f, 0.35f, 0.67f, 1.0f},
    .surface         = {0.16f, 0.16f, 0.19f, 1.0f},
    .surface_hover   = {0.20f, 0.20f, 0.24f, 1.0f},
    .surface_border  = {0.31f, 0.31f, 0.35f, 1.0f},
    .success         = {0.20f, 0.78f, 0.40f, 1.0f},
    .warning         = {0.90f, 0.70f, 0.15f, 1.0f},
    .error           = {0.85f, 0.25f, 0.25f, 1.0f},

    /* Input */
    .input = {
        .bg           = {0.14f, 0.14f, 0.16f, 1.0f},
        .fg           = {0.86f, 0.86f, 0.86f, 1.0f},
        .placeholder  = {0.40f, 0.40f, 0.44f, 1.0f},
        .border       = {0.31f, 0.31f, 0.35f, 1.0f},
        .focus_border = {0.22f, 0.47f, 0.78f, 1.0f},
        .cursor       = {0.78f, 0.78f, 0.86f, 1.0f},
    },

    /* Button */
    .button = {
        .bg         = {0.22f, 0.47f, 0.78f, 1.0f},
        .bg_hover   = {0.29f, 0.55f, 0.86f, 1.0f},
        .bg_pressed = {0.14f, 0.35f, 0.67f, 1.0f},
        .fg         = {1.0f,  1.0f,  1.0f,  1.0f},
    },

    /* List */
    .list = {
        .bg          = {0.16f, 0.16f, 0.19f, 1.0f},
        .fg          = {0.86f, 0.86f, 0.86f, 1.0f},
        .selected_bg = {0.22f, 0.47f, 0.78f, 1.0f},
        .selected_fg = {1.0f,  1.0f,  1.0f,  1.0f},
        .hover_bg    = {0.20f, 0.20f, 0.24f, 1.0f},
        .stripe      = {1.0f,  1.0f,  1.0f,  0.02f},
    },

    /* Tabs */
    .tabs = {
        .bar_bg    = {0.16f, 0.16f, 0.19f, 1.0f},
        .active_bg = {0.12f, 0.12f, 0.14f, 1.0f},
        .indicator = {0.22f, 0.47f, 0.78f, 1.0f},
        .fg        = {0.55f, 0.55f, 0.60f, 1.0f},
        .fg_active = {1.0f,  1.0f,  1.0f,  1.0f},
    },

    /* Slider */
    .slider = {
        .track        = {0.24f, 0.24f, 0.27f, 1.0f},
        .fill         = {0.22f, 0.47f, 0.78f, 1.0f},
        .thumb        = {0.94f, 0.94f, 0.96f, 1.0f},
        .thumb_active = {0.78f, 0.78f, 0.86f, 1.0f},
    },

    /* Checkbox */
    .checkbox = {
        .box_border  = {0.47f, 0.47f, 0.51f, 1.0f},
        .box_checked = {0.22f, 0.47f, 0.78f, 1.0f},
        .checkmark   = {1.0f,  1.0f,  1.0f,  1.0f},
        .fg          = {0.86f, 0.86f, 0.86f, 1.0f},
    },

    /* Dropdown */
    .dropdown = {
        .bg          = {0.14f, 0.14f, 0.16f, 1.0f},
        .border      = {0.31f, 0.31f, 0.35f, 1.0f},
        .fg          = {0.86f, 0.86f, 0.86f, 1.0f},
        .list_bg     = {0.16f, 0.16f, 0.19f, 1.0f},
        .hover_bg    = {0.24f, 0.24f, 0.30f, 1.0f},
        .selected_bg = {0.22f, 0.47f, 0.78f, 0.31f},
        .arrow       = {0.59f, 0.59f, 0.63f, 1.0f},
    },

    /* Dialog */
    .dialog = {
        .overlay   = {0.0f,  0.0f,  0.0f,  0.5f},
        .bg        = {0.16f, 0.16f, 0.19f, 1.0f},
        .border    = {0.31f, 0.31f, 0.35f, 1.0f},
        .title_fg  = {1.0f,  1.0f,  1.0f,  1.0f},
        .separator = {0.31f, 0.31f, 0.35f, 1.0f},
    },

    .osk = {
        .key_bg      = {0.25f, 0.27f, 0.30f, 1.0f},
        .key_fg      = {1.0f,  1.0f,  1.0f,  1.0f},
        .modifier_bg = {0.18f, 0.20f, 0.23f, 1.0f},
        .action_bg   = {0.22f, 0.24f, 0.27f, 1.0f},
        .done_bg     = {0.20f, 0.55f, 0.35f, 1.0f},
        .preview_bg  = {0.12f, 0.13f, 0.15f, 1.0f},
        .panel_bg    = {0.10f, 0.11f, 0.13f, 1.0f},
    },

    /* Spacing */
    .padding_sm = 4, .padding_md = 8, .padding_lg = 16,
    .spacing_sm = 6, .spacing_md = 12, .spacing_lg = 20,
    .corner_radius = 6.0f,
    .font_size_sm = 12, .font_size_md = 16, .font_size_lg = 24, .font_size_xl = 32,
};

/* ------------------------------------------------------------------ */
/* Light theme                                                         */
/* ------------------------------------------------------------------ */

static ClueTheme g_light = {
    .bg              = {0.95f, 0.95f, 0.96f, 1.0f},
    .fg              = {0.13f, 0.13f, 0.15f, 1.0f},
    .fg_dim          = {0.45f, 0.45f, 0.50f, 1.0f},
    .fg_bright       = {0.0f,  0.0f,  0.0f,  1.0f},
    .accent          = {0.15f, 0.45f, 0.82f, 1.0f},
    .accent_hover    = {0.20f, 0.52f, 0.88f, 1.0f},
    .accent_pressed  = {0.10f, 0.35f, 0.70f, 1.0f},
    .surface         = {1.0f,  1.0f,  1.0f,  1.0f},
    .surface_hover   = {0.92f, 0.92f, 0.94f, 1.0f},
    .surface_border  = {0.80f, 0.80f, 0.83f, 1.0f},
    .success         = {0.15f, 0.65f, 0.35f, 1.0f},
    .warning         = {0.80f, 0.60f, 0.10f, 1.0f},
    .error           = {0.80f, 0.20f, 0.20f, 1.0f},

    .input = {
        .bg           = {1.0f,  1.0f,  1.0f,  1.0f},
        .fg           = {0.13f, 0.13f, 0.15f, 1.0f},
        .placeholder  = {0.60f, 0.60f, 0.65f, 1.0f},
        .border       = {0.78f, 0.78f, 0.82f, 1.0f},
        .focus_border = {0.15f, 0.45f, 0.82f, 1.0f},
        .cursor       = {0.13f, 0.13f, 0.15f, 1.0f},
    },

    .button = {
        .bg         = {0.15f, 0.45f, 0.82f, 1.0f},
        .bg_hover   = {0.20f, 0.52f, 0.88f, 1.0f},
        .bg_pressed = {0.10f, 0.35f, 0.70f, 1.0f},
        .fg         = {1.0f,  1.0f,  1.0f,  1.0f},
    },

    .list = {
        .bg          = {1.0f,  1.0f,  1.0f,  1.0f},
        .fg          = {0.13f, 0.13f, 0.15f, 1.0f},
        .selected_bg = {0.15f, 0.45f, 0.82f, 1.0f},
        .selected_fg = {1.0f,  1.0f,  1.0f,  1.0f},
        .hover_bg    = {0.92f, 0.92f, 0.96f, 1.0f},
        .stripe      = {0.0f,  0.0f,  0.0f,  0.02f},
    },

    .tabs = {
        .bar_bg    = {0.92f, 0.92f, 0.94f, 1.0f},
        .active_bg = {0.95f, 0.95f, 0.96f, 1.0f},
        .indicator = {0.15f, 0.45f, 0.82f, 1.0f},
        .fg        = {0.45f, 0.45f, 0.50f, 1.0f},
        .fg_active = {0.0f,  0.0f,  0.0f,  1.0f},
    },

    .slider = {
        .track        = {0.82f, 0.82f, 0.85f, 1.0f},
        .fill         = {0.15f, 0.45f, 0.82f, 1.0f},
        .thumb        = {1.0f,  1.0f,  1.0f,  1.0f},
        .thumb_active = {0.90f, 0.90f, 0.92f, 1.0f},
    },

    .checkbox = {
        .box_border  = {0.65f, 0.65f, 0.70f, 1.0f},
        .box_checked = {0.15f, 0.45f, 0.82f, 1.0f},
        .checkmark   = {1.0f,  1.0f,  1.0f,  1.0f},
        .fg          = {0.13f, 0.13f, 0.15f, 1.0f},
    },

    .dropdown = {
        .bg          = {1.0f,  1.0f,  1.0f,  1.0f},
        .border      = {0.78f, 0.78f, 0.82f, 1.0f},
        .fg          = {0.13f, 0.13f, 0.15f, 1.0f},
        .list_bg     = {1.0f,  1.0f,  1.0f,  1.0f},
        .hover_bg    = {0.90f, 0.90f, 0.95f, 1.0f},
        .selected_bg = {0.15f, 0.45f, 0.82f, 0.20f},
        .arrow       = {0.45f, 0.45f, 0.50f, 1.0f},
    },

    .dialog = {
        .overlay   = {0.0f,  0.0f,  0.0f,  0.3f},
        .bg        = {1.0f,  1.0f,  1.0f,  1.0f},
        .border    = {0.80f, 0.80f, 0.83f, 1.0f},
        .title_fg  = {0.0f,  0.0f,  0.0f,  1.0f},
        .separator = {0.80f, 0.80f, 0.83f, 1.0f},
    },

    .osk = {
        .key_bg      = {0.92f, 0.93f, 0.95f, 1.0f},
        .key_fg      = {0.10f, 0.10f, 0.12f, 1.0f},
        .modifier_bg = {0.78f, 0.80f, 0.83f, 1.0f},
        .action_bg   = {0.82f, 0.84f, 0.87f, 1.0f},
        .done_bg     = {0.25f, 0.65f, 0.42f, 1.0f},
        .preview_bg  = {1.0f,  1.0f,  1.0f,  1.0f},
        .panel_bg    = {0.88f, 0.89f, 0.91f, 1.0f},
    },

    .padding_sm = 4, .padding_md = 8, .padding_lg = 16,
    .spacing_sm = 6, .spacing_md = 12, .spacing_lg = 20,
    .corner_radius = 6.0f,
    .font_size_sm = 12, .font_size_md = 16, .font_size_lg = 24, .font_size_xl = 32,
};

/* ------------------------------------------------------------------ */
/* Active theme                                                        */
/* ------------------------------------------------------------------ */

static ClueTheme g_active;
static int g_initialized = 0;

const ClueTheme *clue_theme_get(void)
{
    if (!g_initialized) {
        g_active = g_dark;
        g_initialized = 1;
    }
    return &g_active;
}

void clue_theme_set(const ClueTheme *theme)
{
    if (theme) {
        g_active = *theme;
        g_initialized = 1;
    }
}

const ClueTheme *clue_theme_dark(void)  { return &g_dark; }
const ClueTheme *clue_theme_light(void) { return &g_light; }
