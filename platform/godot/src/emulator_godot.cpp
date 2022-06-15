#include "emulator.h"
#include "pthread.h"
#include <sched.h>
#include <sys/time.h>
//#include <sys/resource.h>
#include "util.h"

Emulator::Emulator(Board & borat) : board(borat)
{
}

Emulator::~Emulator()
{
}

void Emulator::execute_frame()
{
    for (int i = 0; i < N_SCANCODES; ++i) {
        if (this->keydowns[i]) {
            SDL_KeyboardEvent ev;
            ev.keysym.scancode = this->keydowns[i];
            board.handle_keydown(ev);
        }
        if (this->keyups[i]) {
            SDL_KeyboardEvent ev;
            ev.keysym.scancode = this->keyups[i];
            board.handle_keyup(ev);
        }
        this->keydowns[i] = this->keyups[i] = 0;
    }
    int executed;
    if (Options.vsync && Options.vsync_enable) {
        executed = board.execute_frame_with_cadence(true, true);
    }
    else {
        executed = board.execute_frame_with_cadence(true, false);
    }
}

void Emulator::export_pixel_bytes(uint8_t * dst)
{
    memcpy(dst, board.get_tv().pixels(), pixel_bytes_size());
}

void Emulator::export_audio_frame(float * dst, size_t framesize)
{
    Soundnik * s = &board.get_soundnik();
    s->callback(s, (uint8_t *)dst, framesize * sizeof(float));
}

size_t Emulator::pixel_bytes_size() {
    return (size_t) (Options.screen_width * Options.screen_height * 4);
}

void Emulator::keydown(int scancode) {
    for (int i = 0; i < N_SCANCODES; ++i) {
        if (this->keydowns[i] == 0 || this->keydowns[i] == scancode) {
            this->keydowns[i] = scancode;
            break;
        }
    }
}

void Emulator::keyup(int scancode) {
    for (int i = 0; i < N_SCANCODES; ++i) {
        if (this->keyups[i] == 0 || this->keyups[i] == scancode) {
            this->keyups[i] = scancode;
        }
    }
}

void Emulator::set_joysticks(int joy_0e, int joy_0f)
{
    board.set_joysticks(joy_0e, joy_0f);
}

void Emulator::start_emulator_thread()
{
}

void Emulator::save_state(vector <uint8_t> &to) {
    this->board.serialize(to);
}

bool Emulator::restore_state(vector <uint8_t> &from) {
    return this->board.deserialize(from);
}
