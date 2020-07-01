#pragma once

typedef int SDL_Scancode;

#include "godot_scancodes.h"

enum SDL_PixelFormat {
    SDL_PIXELFORMAT_ARGB8888,
    SDL_PIXELFORMAT_RGB888,
    SDL_PIXELFORMAT_BGR888,
    SDL_PIXELFORMAT_ABGR8888,
};

enum SDL_EventType {
    SDL_KEYDOWN,
    SDL_KEYUP,
    SDL_WINDOWEVENT,
    SDL_QUIT,

    SDL_WINDOWEVENT_RESIZED,
    SDL_WINDOWEVENT_EXPOSED,
    SDL_WINDOWEVENT_SIZE_CHANGED,
};

struct SDL_KeySym {
    SDL_Scancode scancode;
};

struct SDL_KeyboardEvent {
    SDL_KeySym keysym;
};

struct SDL_WindowEvent {
    SDL_EventType event;
};

struct SDL_Event {
    SDL_EventType type;
    union {
        SDL_KeyboardEvent key;
        SDL_WindowEvent window;
    };
};
