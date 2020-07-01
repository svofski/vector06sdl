#pragma once

#define DEFAULT_BORDER_WIDTH 32
#define DEFAULT_SCREEN_WIDTH (512 + 2*(DEFAULT_BORDER_WIDTH))
#define DEFAULT_SCREEN_HEIGHT (256 + 16 + 16)
//#define SCREEN_HEIGHT (256 + 7 + 7)
//#define FIRST_VISIBLE_LINE (312 - SCREEN_HEIGHT)
#define DEFAULT_CENTER_OFFSET (152 - DEFAULT_BORDER_WIDTH)

//#define DBG_QUEUE(x) {x;}
#define DBG_QUEUE(x) {}

#if defined(__ANDROID_NDK__) || defined(__GODOT__)
#define TV_PIXELFORMAT SDL_PIXELFORMAT_ABGR8888
#else
#define TV_PIXELFORMAT SDL_PIXELFORMAT_ARGB8888
#endif
