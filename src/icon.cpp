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
    //uint32_t poxels[64*64*4];
    uint8_t * src = (uint8_t *) &icon64_rgba;
    size_t size = (size_t) icon64_rgba_len;


    surface = SDL_CreateRGBSurfaceWithFormatFrom(src, 128, 128, 32, 128*4,
            SDL_PIXELFORMAT_ABGR8888);
    printf("pixels=%x size=%d\n", src, size);
    for(int i = 0; i < 32; ++i) {
        printf("%x ", src[i]);
    }
    printf("\n");
    fflush(stdout);

    SDL_SetWindowIcon(w, surface);
    SDL_FreeSurface(surface);
}
