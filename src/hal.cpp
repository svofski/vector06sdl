#include <stdio.h>
#include <stdint.h>
#include "i8080.h"
#include "i8080_hal.h"
#include "memory.h"
#include "io.h"
#include "board.h"
#include "SDL.h"

static Memory * memory;
static IO * io;
static Board * board;

void i8080_hal_bind(Memory & _mem, IO & _io, Board & _board)
{
    memory = &_mem;
    io = &_io;
    board = &_board;
}

int i8080_hal_memory_read_byte(int addr)
{
    return memory->read(addr, false);
}

void i8080_hal_memory_write_byte(int addr, int value)
{
    return memory->write(addr, value, false);
}

int i8080_hal_memory_read_word(int addr, bool stack)
{
    return memory->read(addr, stack) | (memory->read(addr+1, stack) << 8);
}

void i8080_hal_memory_write_word(int addr, int word, bool stack)
{
    memory->write(addr, word & 0377, stack);
    memory->write(addr + 1, word >> 8, stack);
}

int i8080_hal_io_input(int port)
{
    int value = io->input(port);
    //printf("input port %02x = %02x\n", port, value);
    return value;
}

void i8080_hal_io_output(int port, int value)
{
    //printf("output port %02x=%02x\n", port, value);
    io->output(port, value);
}

void i8080_hal_iff(int on)
{
    board->interrupt(on);
}

uint32_t timer_callback(uint32_t interval, void * param)
{
    board->onframetimer();
    return(interval);
}

/* If there is no audio buffer to drive the frame rate, use the timer */
void create_timer()
{
    if (Options.nosound && Options.novideo) {
        /* Used in tests, event loop kick-spins itself without timers. */
        return;
    }

    if (Options.nosound) {
        printf("create_timer(): nosound is set, will use SDL timer for frames\n");
        SDL_Init(SDL_INIT_TIMER);
        uint32_t period = 1000 / 50;
        SDL_TimerID sometimer = SDL_AddTimer(period, timer_callback, NULL);
        if (sometimer == 0) {
            fprintf(stderr, "SDL_AddTimer %s\n", SDL_GetError());
        }
    }
}
