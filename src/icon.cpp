#include "SDL.h"

#if USED_XXD
extern "C" unsigned char * icon64_rgba;
extern "C" unsigned int icon64_rgba_len;
#else

extern "C" uint8_t * _binary_icon64_rgba_start;
extern "C" uint8_t * _binary_icon64_rgba_end;
extern "C" size_t    _binary_icon64_rgba_size;

#define icon64_rgba (_binary_icon64_rgba_start)
#define icon64_rgba_len (&_binary_icon64_rgba_size)

#endif

void icon_set(SDL_Window * w)
{
    SDL_Surface * surface;
    uint8_t * src = (uint8_t *) &icon64_rgba;

    surface = SDL_CreateRGBSurfaceWithFormatFrom(src, 128, 128, 32, 128*4,
            SDL_PIXELFORMAT_ABGR8888);

    SDL_SetWindowIcon(w, surface);
    SDL_FreeSurface(surface);
}
