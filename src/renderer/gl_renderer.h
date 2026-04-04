#ifndef CLUE_GL_RENDERER_H
#define CLUE_GL_RENDERER_H

#include "clue/renderer.h"

/* Create the default OpenGL ES renderer */
UIRenderer *clue_gl_renderer_create(void);

/* Destroy the OpenGL ES renderer */
void clue_gl_renderer_destroy(UIRenderer *renderer);

#endif /* CLUE_GL_RENDERER_H */
