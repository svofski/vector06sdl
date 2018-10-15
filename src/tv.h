#pragma once

#include <string>
#include <functional>
#include "globaldefs.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "options.h"

#if HAS_IMAGE
extern "C" DECLSPEC int SDLCALL IMG_SavePNG(SDL_Surface *surface, const char *file);
#endif

class TV
{
private:
    SDL_Window * window;
    SDL_Renderer * renderer;
    SDL_Texture * texture[2];
    uint32_t * bmp;
    int tex_width;
    int tex_height;
    int refresh_rate;
    int texture_n;

    uint32_t pixelformat;

    SDL_GLContext gl_context;
    GLuint gl_textures[2];

private:
    void render_with_blend(int src_alpha);
    void render_single();
    void render_single_regular();
    void render_single_opengl();
    void init_regular();
    void init_opengl();
    void init_gl_textures();
    void window_resized(SDL_Event & event);

public:
    TV();
    ~TV();
    int probe();
    void init();
    void toggle_fullscreen();
    void save_frame(std::string path);
    uint32_t* pixels() const;
    std::function<uint32_t(uint8_t,uint8_t,uint8_t)> get_rgb2pixelformat() const;
    /* executed: 1 if the frame was real, 0 if the frame is a skip frame */
    void render(int executed);
    int get_refresh_rate() const;
    void handle_window_event(SDL_Event & event);
};
