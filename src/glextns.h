#pragma once

#if HAVE_OPENGL

#ifdef __APPLE__
#include <OpenGL/GL.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif


extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i;
extern PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f;
extern PFNGLPROGRAMUNIFORM2FPROC glProgramUniform2f;
extern PFNGLPROGRAMUNIFORM3FPROC glProgramUniform3f;

bool initGLExtensions();

#endif

