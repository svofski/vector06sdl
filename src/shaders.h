#pragma once

#if HAVE_OPENGL

#ifdef __APPLE__
#include <OpenGL/GL.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

bool init_shaders(GLuint & program_id);

#endif
