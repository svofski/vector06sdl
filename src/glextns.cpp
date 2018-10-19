#include "SDL.h"
#include "SDL_opengl.h"
#include "glextns.h"

PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDETACHSHADERPROC glDetachShader;

PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i;
PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f;
PFNGLPROGRAMUNIFORM2FPROC glProgramUniform2f;
PFNGLPROGRAMUNIFORM3FPROC glProgramUniform3f;

bool initGLExtensions();

//
// gracias a Augusto Ruiz https://github.com/AugustoRuiz/sdl2glsl
// -----------------------------------------
bool initGLExtensions() {
    glCreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
    glAttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
    glDetachShader = (PFNGLDETACHSHADERPROC)SDL_GL_GetProcAddress("glDetachShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
    glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)SDL_GL_GetProcAddress("glValidateProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");

    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
    glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)SDL_GL_GetProcAddress("glProgramUniform1i");
    glProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)SDL_GL_GetProcAddress("glProgramUniform1f");
    glProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)SDL_GL_GetProcAddress("glProgramUniform2f");
    glProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)SDL_GL_GetProcAddress("glProgramUniform3f");

    return glCreateShader && glShaderSource && glCompileShader && glGetShaderiv && 
        glGetShaderInfoLog && glDeleteShader && glAttachShader && glCreateProgram &&
        glLinkProgram && glValidateProgram && glGetProgramiv && glGetProgramInfoLog &&
        glUseProgram && glDeleteProgram && glDetachShader &&
        glProgramUniform1i && glProgramUniform1f && glProgramUniform2f && 
        glProgramUniform3f;
}
// ----------------------------------------



