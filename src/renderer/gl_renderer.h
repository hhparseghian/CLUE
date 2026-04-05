#ifndef CLUE_GL_RENDERER_H
#define CLUE_GL_RENDERER_H

#include "clue/renderer.h"

/* Create the default OpenGL ES renderer */
ClueRenderer *clue_gl_renderer_create(void);

/* Destroy the OpenGL ES renderer */
void clue_gl_renderer_destroy(ClueRenderer *renderer);

#endif /* CLUE_GL_RENDERER_H */
