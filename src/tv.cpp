#include <string>
#include "globaldefs.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "options.h"
#include "tv.h"


#if HAVE_OPENGL

#ifdef __APPLE__
#include <OpenGL/GL.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <iostream>
#include <string>
#include <streambuf>
#include <fstream>

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
bool init_shaders(GLuint & program_id);

#endif

#if HAS_IMAGE
extern "C" DECLSPEC int SDLCALL IMG_SavePNG(SDL_Surface *surface, const char *file);
#endif

TV::TV() : pixelformat(SDL_PIXELFORMAT_ARGB8888)
{
}

TV::~TV()
{
    if (Options.novideo) {
        delete[] bmp;
    }
}

int TV::probe()
{
    static int display_in_use = 0; /* Only using first display */

    int i, display_mode_count;
    SDL_DisplayMode mode;
    Uint32 f;

    SDL_Log("SDL_GetNumVideoDisplays(): %i", SDL_GetNumVideoDisplays());

    display_mode_count = SDL_GetNumDisplayModes(display_in_use);
    if (display_mode_count < 1) {
        SDL_Log("SDL_GetNumDisplayModes failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Log("SDL_GetNumDisplayModes: %i", display_mode_count);

    for (i = 0; i < display_mode_count; ++i) {
        if (SDL_GetDisplayMode(display_in_use, i, &mode) != 0) {
            SDL_Log("SDL_GetDisplayMode failed: %s", SDL_GetError());
            return 1;
        }
        f = mode.format;

        SDL_Log("Mode %i\tbpp %i\t%s\t%i x %i %dHz", i,
                SDL_BITSPERPIXEL(f), SDL_GetPixelFormatName(f), mode.w, mode.h,
                mode.refresh_rate);
    }

    return 0;
}

void TV::init()
{
    if (Options.novideo || Options.opengl) {
        /* if novideo, we need a texture to draw to */
        /* if OpenGl, allocate pixels */
        this->bmp = new uint32_t[Options.screen_width * Options.screen_height];
    }
    if (Options.novideo) {
        return;
    }

    SDL_Init(SDL_INIT_VIDEO);
#if 0
    probe();
#endif
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);

    printf("Current display mode: %dx%d %dHz\n", display_mode.w, display_mode.h,
            display_mode.refresh_rate);
    this->refresh_rate = display_mode.refresh_rate;

    if (Options.opengl) {
        this->init_opengl();
    }
    else {
        this->init_regular();
    }
}

void TV::init_regular()
{
    int window_options = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    int renderer_options = SDL_RENDERER_ACCELERATED;
    if (Options.vsync) {
        renderer_options |= SDL_RENDERER_PRESENTVSYNC;
    }

    int window_height = Options.screen_height * 2;
    int window_width = window_height * 5 / 4;

    window_width += 2 * (Options.border_width - 32);

    this->window = SDL_CreateWindow("Вектор-06ц", 0, 0, 
            window_width, window_height, window_options);
    this->renderer = SDL_CreateRenderer(this->window, -1, 
            renderer_options);

    SDL_RendererInfo rinfo;
    SDL_GetRendererInfo(this->renderer, &rinfo);
    printf("SDL_RendererInfo: %s SDL_RENDERER_ACCELERATED\n", 
            (rinfo.flags & SDL_RENDERER_ACCELERATED) ? "yes" : "no");
    printf("SDL_RendererInfo: %s SDL_RENDERER_TARGETTEXTURE\n", 
            (rinfo.flags & SDL_RENDERER_TARGETTEXTURE) ? "yes" : "no");

    uint32_t window_pixelformat = SDL_GetWindowPixelFormat(this->window);
    switch (window_pixelformat) {
        /* in theory we should follow native format, but on linux vm
         * I'm getting native format RGB888, which gets internally
         * converted into ARGB8888 anyway */
        case SDL_PIXELFORMAT_ARGB8888:
            printf("Native pixelformat: ARGB8888\n");
            this->pixelformat = SDL_PIXELFORMAT_ARGB8888;
            break;
        case SDL_PIXELFORMAT_ABGR8888:
            printf("Native pixelformat: ABGR8888\n");
            this->pixelformat = SDL_PIXELFORMAT_ABGR8888;
            break;
        case SDL_PIXELFORMAT_RGB888:
            printf("Native pixelformat: RGB888\n");
            this->pixelformat = SDL_PIXELFORMAT_RGB888;
            break;
        case SDL_PIXELFORMAT_BGR888:
            printf("Native pixelformat: BGR888\n");
            this->pixelformat = SDL_PIXELFORMAT_BGR888;
            break;
        default:
            printf("Unknown native pixelformat: %08x\n", window_pixelformat);
            break;
    }

    if (!Options.window) {
        SDL_SetWindowFullscreen(this->window, 
#if __MACOSX__
                SDL_WINDOW_FULLSCREEN
#else
                SDL_WINDOW_FULLSCREEN_DESKTOP
#endif
                );
    }
    else {
        SDL_SetWindowPosition(this->window, 100, 100);
    }

    this->tex_width = Options.screen_width;
    this->tex_height = Options.screen_height;
    for (int i = 0; i < TV::NTEXTURES; ++i) {
        this->texture[i] = SDL_CreateTexture(this->renderer, this->pixelformat,
                SDL_TEXTUREACCESS_STREAMING, this->tex_width, this->tex_height);
    }
    this->texture_n = 0;

    SDL_RenderSetLogicalSize(this->renderer, window_width, window_height);
}

void print_tex_format_info(GLenum internalformat)
{
#if 0
    extern "C" {
    extern void APIENTRY glGetInternalformativ (GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);
    }
    GLint pformat, format, type;

    pformat = format = type = 0;

    glGetInternalformativ(GL_TEXTURE_2D, internalformat, GL_INTERNALFORMAT_PREFERRED, 1, &pformat);
    glGetInternalformativ(GL_TEXTURE_2D, internalformat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
    glGetInternalformativ(GL_TEXTURE_2D, internalformat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

    printf("IF [%x]\nPF [%x]\nXF [%x]\nTP [%x]\n\n",
            internalformat,
            pformat,
            format,
            type
          );
#endif
}


//void ogl_debug_print_callback(GLenum source, GLenum type, GLuint id, 
//        GLenum severity, GLsizei length, const GLchar* message, 
//        const void* userParam) {
//    if(severity!=0x826b) {
//        fprintf(stderr, 
//            "GL CALLBACK: type = %s, severity = 0x%x, message = %s\n", 
//            type_to_string(type).c_str(), severity, message);
//    }
//}

void TV::init_opengl()
{
#if HAVE_OPENGL
    int window_height = Options.screen_height * 2;
    int window_width = window_height * 5 / 4;

    window_width += 2 * (Options.border_width - 32);

    int window_options = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | 
        SDL_WINDOW_OPENGL;
    this->window = SDL_CreateWindow("Вектор-06ц (gl)", 0, 0, 
            window_width, window_height, window_options);
    gl_context = SDL_GL_CreateContext(window);


    if (initGLExtensions()) {
        init_shaders(this->gl_program_id);
    } else {
        fprintf(stderr, "Could not init gl extensions require for loading shaders\n");
    }

    const GLubyte* openGLVersion = glGetString(GL_VERSION);
    printf("GL_VERSION: %s\n", openGLVersion);
    //glEnable(GL_DEBUG_OUTPUT);
    //glDebugMessageCallback(ogl_debug_print_callback, nullptr);

    SDL_GL_SetSwapInterval(Options.vsync ? 1 : 0);

    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    print_tex_format_info(GL_RGB);
    print_tex_format_info(GL_RGB8);
    print_tex_format_info(GL_RGBA8);
    init_gl_textures();

    if (!Options.window) {
        SDL_SetWindowFullscreen(this->window, 
#if __MACOSX__
                SDL_WINDOW_FULLSCREEN
#else
                SDL_WINDOW_FULLSCREEN_DESKTOP
#endif
                );
    }
    else {
        SDL_SetWindowPosition(this->window, 100, 100);
    }
#endif
}


void TV::init_gl_textures()
{
#if HAVE_OPENGL
    printf("Texture size: %dx%d\n", Options.screen_width, Options.screen_height);
    this->pixelformat = SDL_PIXELFORMAT_ARGB8888;
    glGenTextures(1, &this->gl_textures[0]);
    glBindTexture(GL_TEXTURE_2D, gl_textures[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, 
            /* internalformat */ GL_RGBA8,
            Options.screen_width, Options.screen_height, /* border */ 0, 
            /* format */ GL_RGBA, 
            /* type */   GL_UNSIGNED_BYTE, 
            (uint8_t*)this->bmp);
#endif
}

void TV::window_resized(SDL_Event & event)
{
#if HAVE_OPENGL
    if (Options.opengl) {
        int window_width, window_height;
        SDL_GetWindowSize(this->window, &window_width, &window_height);

    	int ww = window_height * 5 / 4;
        int wh = window_height;
        if (ww > window_width) {
            ww = window_width;
            wh = window_width * 4 / 5;
        }
        this->gl_window_width = ww;
        this->gl_window_height = wh;

        int ox = window_width/2 - ww/2;
        int oy = window_height/2 - wh/2;
        glViewport(ox, oy, ww, wh);
    }
#endif
}

void TV::handle_window_event(SDL_Event & event)
{
    switch (event.window.event) {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_EXPOSED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            window_resized(event);
            break;
        }    
}

void TV::toggle_fullscreen()
{
    int windowflags = SDL_GetWindowFlags(this->window);
#if __MACOSX__
    /* on mac only this works */
    int fs = SDL_WINDOW_FULLSCREEN;
#else
    /* on windows regular FULLSCREEN distorts aspect ratio disregarding
     * logical size */
    int fs = SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
    int set = (windowflags ^ fs) & fs;
    SDL_SetWindowFullscreen(this->window, set);
    SDL_RenderSetLogicalSize(this->renderer, 4, 3);
}

void TV::save_frame(std::string path)
{
#if HAS_IMAGE
    SDL_Surface * s = SDL_CreateRGBSurfaceWithFormatFrom(this->bmp, 
            Options.screen_width, Options.screen_height, 32, 
            4*Options.screen_width, this->pixelformat);

    IMG_SavePNG(s, path.c_str());

    SDL_FreeSurface(s);
#endif
}

uint32_t* TV::pixels() const {
    if (!this->bmp) {
        int pitch;
        SDL_LockTexture(this->texture[this->texture_n], NULL, 
                (void**)&this->bmp, &pitch);
    }

    return this->bmp;
}

void TV::render_with_blend(int src_alpha)
{
    if (Options.opengl) {
        printf("render_with_blend not supported on opengl");
        return;
    }
    int A = this->texture_n;
    int B = (this->texture_n + 1) & 1;

    /* render single frame */
    SDL_UnlockTexture(this->texture[A]);
    this->bmp = NULL;
    SDL_SetTextureBlendMode(this->texture[A], SDL_BLENDMODE_NONE);
    SDL_SetTextureAlphaMod(this->texture[A], 255);
    SDL_RenderClear(this->renderer);
    SDL_RenderCopy(this->renderer, this->texture[A], NULL, NULL);

    SDL_SetTextureBlendMode(this->texture[B], SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(this->texture[B], src_alpha);
    SDL_RenderCopy(this->renderer, this->texture[B], NULL, NULL);

    this->texture_n = (this->texture_n + 1) & 1;
}

void TV::render_single()
{
    if (Options.opengl) {
        this->render_single_opengl();
    }
    else {
        this->render_single_regular();
    }
}

void TV::render_single_regular()
{
    /* render single frame */
    int t = this->texture_n;
    SDL_UnlockTexture(this->texture[t]);
    this->bmp = NULL;
    SDL_SetTextureBlendMode(this->texture[t], SDL_BLENDMODE_NONE);
    SDL_SetTextureAlphaMod(this->texture[t], 255);
    SDL_RenderClear(this->renderer);
    SDL_RenderCopy(this->renderer, this->texture[t], NULL, NULL);
    this->texture_n = this->texture_n + 1;
    if (this->texture_n >= TV::NTEXTURES) this->texture_n = 0;
}

void TV::render_single_opengl()
{
    const int w = Options.screen_width, h = Options.screen_height;   // 576x288
    int old_program_id;

    glBindTexture(GL_TEXTURE_2D, this->gl_textures[0]);
    if (this->gl_program_id) {
        GLuint pid = this->gl_program_id;
        glGetIntegerv(GL_CURRENT_PROGRAM, &old_program_id);
        glUseProgram(pid);

        GLuint tex0 = glGetUniformLocation(pid, "Texture0");
        glProgramUniform1i(pid, tex0, 0);

        GLuint tex0sz = glGetUniformLocation(pid, "color_texture_sz");
        glProgramUniform3f(pid, tex0sz, (float)w, (float)h, 0.0f);

        GLuint screensz = glGetUniformLocation(pid, "screen_texture_sz");
        glProgramUniform2f(pid, screensz, (float)this->gl_window_width, 
                (float)this->gl_window_height);

        GLuint filter_gain = glGetUniformLocation(pid, "filter_gain");
        glProgramUniform1f(pid, filter_gain, 1.0f);

        GLuint filter_invgain = glGetUniformLocation(pid, "filter_invgain");
        glProgramUniform1f(pid, filter_invgain, 1.6f);
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, 
            GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)this->bmp);

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity(); 
    glOrtho(0.0f, w, h, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();
 
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0,0);
    glTexCoord2f(1, 0); glVertex2f(w,0);
    glTexCoord2f(1, 1); glVertex2f(w,h);
    glTexCoord2f(0, 1); glVertex2f(0,h);
    glEnd();
    
    if (this->gl_program_id) {
        glUseProgram(old_program_id);
    }
}

/* executed: 1 if the frame was real, 0 if the frame is a skip frame */
void TV::render(int executed)
{
    static int prev_executed;
    if (!Options.novideo) {
        if (Options.blendmode == 0) {
            if (executed) render_single();
        } 
        else if (Options.blendmode == 1) {
            if (executed) {
                render_with_blend(128);
            }
        }
        else if (Options.blendmode == 2) {
            if (executed && prev_executed) {
                render_with_blend(200);
            } 
            else {
                if (executed) render_single();
            }
        }
        /* it is actually better to call SDL_RenderPresent
         * because it maintains the pace. Otherwise we use 100% CPU
         * when stopped in debugger. */
        if (Options.opengl) {
            SDL_GL_SwapWindow(window);
        } 
        else {
            SDL_RenderPresent(this->renderer);
        } 
        prev_executed = executed;
    }
}

int TV::get_refresh_rate() const
{
    return this->refresh_rate;
}

std::function<uint32_t(uint8_t,uint8_t,uint8_t)> TV::get_rgb2pixelformat() const
{
    switch(this->pixelformat) {
        case SDL_PIXELFORMAT_RGB888:
        case SDL_PIXELFORMAT_ARGB8888:
            return [](uint8_t r, uint8_t g, uint8_t b) {
                uint32_t result =
                    0xff000000 |
                    (r << (5 + 16)) |
                    (g << (5 + 8)) |
                    (b << (6 + 0));
                 return result;
            };
            break;
        case SDL_PIXELFORMAT_BGR888:
        case SDL_PIXELFORMAT_ABGR8888:
            return [](uint8_t r, uint8_t g, uint8_t b) {
                uint32_t result =
                    0xff000000 |
                    (b << (6 + 16)) |
                    (g << (5 + 8)) |
                    (r << (5 + 0));
                 return result;
            };
            break;
    }
    printf("Impossible pixelformat: %08x\n", this->pixelformat);
    return nullptr;
}

#if HAVE_OPENGL
std::string read_file(const char * filename)
{
    std::string text;
    try {
    std::ifstream vsh(filename,  std::ifstream::in);
    vsh.seekg(0, std::ios::end);
    text.reserve(vsh.tellg());
    vsh.seekg(0, std::ios::beg);
    text.assign((std::istreambuf_iterator<char>(vsh)),
            std::istreambuf_iterator<char>());
    } 
    catch (...){
        printf("Failed to load %s\n", filename);
    }
    return text;
}

std::string get_vertex_src()
{
    return read_file("../shaders/singlepass.vsh");
}

std::string get_frag_src()
{
    return read_file("../shaders/singlepass.fsh");
}

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
        fprintf(stderr, "Vertex shader compile error: %s\n", &infoLog[0]);

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
