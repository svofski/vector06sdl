#pragma once

#include <string>
#include "globaldefs.h"
#include "SDL.h"
#include "options.h"

#if HAS_IMAGE
extern "C" DECLSPEC int SDLCALL IMG_SavePNG(SDL_Surface *surface, const char *file);
#endif

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

    int probe()
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

            SDL_Log("Mode %i\tbpp %i\t%s\t%i x %i", i,
                    SDL_BITSPERPIXEL(f), SDL_GetPixelFormatName(f), mode.w, mode.h);
        }
    }

    void init()
    {
        this->bmp = new uint32_t[SCREEN_WIDTH * SCREEN_HEIGHT];

        if (Options.novideo) {
            return;
        }

        SDL_Init(SDL_INIT_VIDEO);
        probe();

        if (Options.window) {
            SDL_CreateWindowAndRenderer(800, 600,
                    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE,
                    &this->window, &this->renderer);
        } 
        else {
            SDL_DisplayMode display_mode;
            SDL_GetCurrentDisplayMode(0, &display_mode);

            printf("Current display mode: %dx%d\n", display_mode.w, display_mode.h);

            //int options = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
            //if (Options.vsync) {
            //    options |= SDL_RENDERER_PRESENTVSYNC;
            //}

            //this->window = SDL_CreateWindow("Вектор-06ц", 0, 0, display_mode.w,
            //        display_mode.h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
            this->window = SDL_CreateWindow("Вектор-06ц", 0, 0, 656, 288,
                    SDL_WINDOW_FULLSCREEN);
                    //SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
            this->renderer = SDL_CreateRenderer(this->window, -1, 
                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

            //SDL_CreateWindowAndRenderer(display_mode.w, display_mode.h, 
            //        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE/*|SDL_WINDOW_FULLSCREEN_DESKTOP*/,
            //        &this->window, &this->renderer);
            SDL_SetWindowFullscreen(this->window, 
#if __MACOSX__
                    SDL_WINDOW_FULLSCREEN
#else
                    SDL_WINDOW_FULLSCREEN_DESKTOP
#endif
                    );
        }

        this->tex_width = SCREEN_WIDTH;
        this->tex_height = SCREEN_HEIGHT;
        this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ABGR8888,
                SDL_TEXTUREACCESS_STATIC, this->tex_width, this->tex_height);

        SDL_RenderSetLogicalSize(this->renderer, 4, 3);
    }

    bool handle_keyboard_event(SDL_KeyboardEvent & event)
    {
        switch (event.keysym.scancode) 
        {
            case SDL_SCANCODE_RETURN:
#if __WIN32__
                if (event.keysym.mod & KMOD_ALT) {	
#else 
                if (event.keysym.mod & KMOD_GUI) {
#endif
                    this->toggle_fullscreen();
                    return true;
                }
                break;
            default:
                break;
        }
        return false;
    }

    void handle_window_event(SDL_Event & event)
    {
    }

    void toggle_fullscreen()
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

    void save_frame(std::string path)
    {
#if HAS_IMAGE
        SDL_Surface * s = SDL_CreateRGBSurfaceWithFormatFrom(this->bmp, 
                SCREEN_WIDTH, SCREEN_HEIGHT, 32, 4*SCREEN_WIDTH, 
                SDL_PIXELFORMAT_ABGR8888);

        IMG_SavePNG(s, path.c_str());

        SDL_FreeSurface(s);
#endif
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
