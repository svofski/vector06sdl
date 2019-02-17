#if HAVE_OPENGL

#include <vector>
#include <iostream>
#include <string>
#include <streambuf>
#include <fstream>

#include "SDL.h"
#include "SDL_opengl.h"
#include "glextns.h"

#include "options.h"
#include "shaders.h"
#include "glextns.h"
#include "util.h"

#if USED_XXD 

#define IMPORT(ID) \
    extern "C" unsigned char ID[]; \
    extern "C" unsigned int ID##_len; 

#else 

#if MSYS_MINGW32
#define IMPORT(ID) \
    extern "C" uint8_t * binary_##ID##_start; \
    extern "C" uint8_t * binary_##ID##_end; \
    extern "C" size_t    binary_##ID##_size; \
    static const uint8_t * ID = (uint8_t *) &binary_##ID##_start; \
    static const size_t ID##_len = &binary_##ID##_size;
#else
#define IMPORT(ID) \
    extern "C" uint8_t * _binary_##ID##_start; \
    extern "C" uint8_t * _binary_##ID##_end; \
    extern "C" size_t    _binary_##ID##_size; \
    static const uint8_t * ID = (uint8_t *) &_binary_##ID##_start; \
    static const size_t ID##_len = &_binary_##ID##_size;
#endif
#endif
IMPORT(singlepass_vsh)
IMPORT(singlepass_fsh)

std::string get_vertex_src()
{
    if (Options.gl.default_shader) {
        return std::string((char *)singlepass_vsh, (size_t)singlepass_vsh_len);
    }
    else {
        return util::read_file(Options.gl.shader_basename + std::string(".vsh"));
    }
}

std::string get_frag_src()
{
    if (Options.gl.default_shader) {
        return std::string((char *)singlepass_fsh, (size_t)singlepass_fsh_len);
    }
    else {
        return util::read_file(Options.gl.shader_basename + std::string(".fsh"));
    }
}

bool init_shaders(GLuint & program_id)
{
    std::string vertexSource = get_vertex_src();
    std::string fragmentSource = get_frag_src();

    // Create an empty vertex shader handle
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // Send the vertex shader source code to GL
    // Note that std::string's .c_str is NULL character terminated.
    const GLchar *source = (const GLchar *)vertexSource.c_str();
    glShaderSource(vertexShader, 1, &source, 0);

    // Compile the vertex shader
    glCompileShader(vertexShader);

    GLint isCompiled = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

	// The maxLength includes the NULL character
	std::vector<GLchar> infoLog(maxLength);
	glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);

	// We don't need the shader anymore.
        glDeleteShader(vertexShader);

	// Use the infoLog as you see fit.
        fprintf(stderr, "Vertex shader compile error: %s\n", &infoLog[0]);

	// In this simple program, we'll just leave
        return false;
    }

    // Create an empty fragment shader handle
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    // Send the fragment shader source code to GL
    // Note that std::string's .c_str is NULL character terminated.
    source = (const GLchar *)fragmentSource.c_str();
    glShaderSource(fragmentShader, 1, &source, 0);

    // Compile the fragment shader
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
	GLint maxLength = 0;
	glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

	// The maxLength includes the NULL character
	std::vector<GLchar> infoLog(maxLength);
	glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);

	// We don't need the shader anymore.
	glDeleteShader(fragmentShader);
	// Either of them. Don't leak shaders.
	glDeleteShader(vertexShader);

	// Use the infoLog as you see fit.
        fprintf(stderr, "Fragment shader compile error: %s\n", &infoLog[0]);

	// In this simple program, we'll just leave
	return false;
    }

    // Vertex and fragment shaders are successfully compiled.
    // Now time to link them together into a program.
    // Get a program object.
    GLuint program = glCreateProgram();


    // Attach our shaders to our program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    // Link our program
    glLinkProgram(program);

    // Note the different functions here: glGetProgram* instead of glGetShader*.
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
    if (isLinked == GL_FALSE)
    {
	GLint maxLength = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

	// The maxLength includes the NULL character
	std::vector<GLchar> infoLog(maxLength);
	glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

	// We don't need the program anymore.
	glDeleteProgram(program);
	// Don't leak shaders either.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Use the infoLog as you see fit.
        fprintf(stderr, "Vertex shader compile error: %s\n", &infoLog[0]);

	// In this simple program, we'll just leave
	return false;
    }

    // Always detach shaders after a successful link.
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    program_id = program;
    return true;
}

#endif
