#include <stdlib.h>
#include "clue/clue.h"

/*
 * Backend instances are defined in their respective source files
 * and only available when compiled in via CMake options.
 */

#ifdef CLUE_HAS_WAYLAND
extern UIBackend wayland_backend;
#endif

#ifdef CLUE_HAS_X11
extern UIBackend x11_backend;
#endif

#ifdef CLUE_HAS_DRM
extern UIBackend drm_backend;
#endif

/* Select the best available backend at runtime.
 * Priority: Wayland > X11 > DRM (bare metal fallback). */
UIBackend *clue_select_backend(void)
{
#ifdef CLUE_HAS_WAYLAND
    if (getenv("WAYLAND_DISPLAY")) {
        return &wayland_backend;
    }
#endif

#ifdef CLUE_HAS_X11
    if (getenv("DISPLAY")) {
        return &x11_backend;
    }
#endif

#ifdef CLUE_HAS_DRM
    return &drm_backend;
#endif

    return NULL;
}
