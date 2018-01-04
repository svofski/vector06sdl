#pragma once

#include <string>
#include "globaldefs.h"
#include "SDL.h"
#include "options.h"

extern "C" DECLSPEC int SDLCALL IMG_SavePNG(SDL_Surface *surface, const char *file);

class TV
{
private:
    SDL_Window * window;
    SDL_Renderer * renderer;
    SDL_Texture * texture;
    uint32_t * bmp;
    int tex_width;
    int tex_height;

public:
    TV() 
    {
    }

    ~TV()
    {
        delete[] bmp;
    }

    void init()
    {
        this->bmp = new uint32_t[SCREEN_WIDTH * SCREEN_HEIGHT];

        if (Options.novideo) {
            return;
        }

        SDL_Init(SDL_INIT_VIDEO);
        SDL_DisplayMode display_mode;
        SDL_GetCurrentDisplayMode(0, &display_mode);

        printf("Current display mode: %dx%d\n", display_mode.w, display_mode.h);

        SDL_CreateWindowAndRenderer(display_mode.w, display_mode.h, 
                SDL_WINDOW_SHOWN /*|SDL_WINDOW_FULLSCREEN_DESKTOP*/,
                &this->window, &this->renderer);
        SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN);
        this->tex_width = SCREEN_WIDTH;
        this->tex_height = SCREEN_HEIGHT;
        this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ABGR8888,
                SDL_TEXTUREACCESS_STATIC, this->tex_width, this->tex_height);

        SDL_RenderSetLogicalSize(this->renderer, 4, 3);
    }

    void save_frame(std::string path)
    {
        SDL_Surface * s = SDL_CreateRGBSurfaceWithFormatFrom(this->bmp, 
                SCREEN_WIDTH, SCREEN_HEIGHT, 32, 4*SCREEN_WIDTH, 
                SDL_PIXELFORMAT_ABGR8888);

        IMG_SavePNG(s, path.c_str());

        SDL_FreeSurface(s);
    }

    uint32_t* pixels() const {
        return this->bmp;
    }

    void render()
    {
        if (!Options.novideo) {
            SDL_UpdateTexture(this->texture, NULL, this->bmp, 
                    this->tex_width * sizeof(uint32_t));
            SDL_RenderClear(this->renderer);
            SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
            SDL_RenderPresent(this->renderer);
        }
    }
};
