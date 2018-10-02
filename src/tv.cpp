#include <string>
#include "globaldefs.h"
#include "SDL.h"
#include "options.h"
#include "tv.h"

#if HAS_IMAGE
extern "C" DECLSPEC int SDLCALL IMG_SavePNG(SDL_Surface *surface, const char *file);
#endif

TV::TV() 
{
}

TV::~TV()
{
    delete[] bmp;
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
    this->bmp = new uint32_t[Options.screen_width * Options.screen_height];

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

    uint32_t window_pixelformat = SDL_GetWindowPixelFormat(this->window);
    this->pixelformat = SDL_PIXELFORMAT_ABGR8888;
    switch (window_pixelformat) {
        /* in theory we should follow native format, but on linux vm
         * I'm getting native format RGB888, which gets internally
         * converted into ARGB8888 anyway */
        case SDL_PIXELFORMAT_RGB888:
        case SDL_PIXELFORMAT_ARGB8888:
            printf("Native pixelformat: ARGB8888\n");
            this->pixelformat = SDL_PIXELFORMAT_ARGB8888;
            break;
        case SDL_PIXELFORMAT_BGR888:
        case SDL_PIXELFORMAT_ABGR8888:
            printf("Native pixelformat: ABGR8888\n");
            this->pixelformat = SDL_PIXELFORMAT_ABGR8888;
            break;
        //case SDL_PIXELFORMAT_RGB888:
        //    printf("Native pixelformat: RGB888\n");
        //    this->pixelformat = SDL_PIXELFORMAT_RGB888;
        //    break;
        //case SDL_PIXELFORMAT_BGR888:
        //    printf("Native pixelformat: BGR888\n");
        //    this->pixelformat = SDL_PIXELFORMAT_BGR888;
        //    break;
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

    this->tex_width = Options.screen_width;
    this->tex_height = Options.screen_height;
    this->texture[0] = SDL_CreateTexture(this->renderer, this->pixelformat,
            SDL_TEXTUREACCESS_STATIC, this->tex_width, this->tex_height);

    this->texture[1] = SDL_CreateTexture(this->renderer, this->pixelformat,
            SDL_TEXTUREACCESS_STATIC, this->tex_width, this->tex_height);
    this->texture_n = 0;

    SDL_RenderSetLogicalSize(this->renderer, window_width, window_height);
}

bool TV::handle_keyboard_event(SDL_KeyboardEvent & event)
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

    void TV::handle_window_event(SDL_Event & event)
    {
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
        return this->bmp;
    }

    void TV::render_with_blend(int src_alpha)
    {
        int next_texture = (this->texture_n + 1) & 1;
        SDL_UpdateTexture(this->texture[next_texture], NULL, this->bmp, 
                this->tex_width * sizeof(uint32_t));
        SDL_RenderClear(this->renderer);
        SDL_SetTextureBlendMode(this->texture[this->texture_n], SDL_BLENDMODE_NONE);
        SDL_SetTextureAlphaMod(this->texture[this->texture_n], 255);
        SDL_RenderCopy(this->renderer, this->texture[this->texture_n], NULL, NULL);
        SDL_SetTextureBlendMode(this->texture[next_texture], SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(this->texture[next_texture], src_alpha);
        SDL_RenderCopy(this->renderer, this->texture[next_texture], NULL, NULL);
        this->texture_n = next_texture;
    }

    void TV::render_single()
    {
        /* render single frame */
        this->texture_n = (this->texture_n + 1) & 1;
        SDL_UpdateTexture(this->texture[this->texture_n], NULL, this->bmp,
                this->tex_width * sizeof(uint32_t));
        SDL_SetTextureBlendMode(this->texture[this->texture_n], SDL_BLENDMODE_NONE);
        SDL_SetTextureAlphaMod(this->texture[this->texture_n], 255);
        SDL_RenderClear(this->renderer);
        SDL_RenderCopy(this->renderer, this->texture[this->texture_n], NULL, NULL);
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
            /* if (executed) */ SDL_RenderPresent(this->renderer);
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
