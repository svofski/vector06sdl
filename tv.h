#pragma once

#include "globaldefs.h"
#include "SDL.h"

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
        SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
                &this->window, &this->renderer);
        this->tex_width = SCREEN_WIDTH;
        this->tex_height = SCREEN_HEIGHT;
        texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ABGR8888,
                SDL_TEXTUREACCESS_STATIC, this->tex_width, this->tex_height);

        SDL_RenderSetLogicalSize(this->renderer, 4, 3);

        this->bmp = new uint32_t[SCREEN_WIDTH * SCREEN_HEIGHT];
    }

    uint32_t* pixels() const {
        return this->bmp;
    }

    void render()
    {
        SDL_UpdateTexture(this->texture, NULL, this->bmp, 
                this->tex_width * sizeof(uint32_t));
        SDL_RenderClear(this->renderer);
        SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
        SDL_RenderPresent(this->renderer);
    }
};
