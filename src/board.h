#pragma once

#include <stdio.h>
#include <vector>
#include <functional>
#include "i8080.h"
#include "filler.h"
#include "sound.h"
#include "tv.h"
#include "cadence.h"
#include "breakpoint.h"

class Board;

void i8080_hal_bind(Memory & _mem, IO & _io, Board & _board);
void create_timer();

class Board
{
private:
    int between;
    int instr_time;
    int commit_time;
    int commit_time_pal;
    int last_opcode;
    int frame_no;

    const int * cadence_frames; 
    int cadence_length = 1;
    int cadence_seq = 0;

    bool irq;
    bool inte;          /* CPU INTE pin */
    bool irq_carry;     /* imitates cpu waiting after T2 when INTE */

    Memory & memory;
    IO & io;
    PixelFiller & filler;
    Soundnik & soundnik;
    TV & tv;
    WavPlayer & tape_player;

    int debugging;
    int debugger_interrupt;
    std::vector<Breakpoint> breakpoints;

public:
    std::function<void(void)> poll_debugger;
    std::function<void(void)> onbreakpoint;

public:
    Board(Memory & _memory, IO & _io, PixelFiller & _filler, Soundnik & _snd, 
            TV & _tv, WavPlayer & _tape_player);

    void init();
    void reset(bool blkvvod);    // true: power-on reset, false: boot loaded prog
    void interrupt(bool on);
    /* Fuses together inner CPU logic and Vector-06c interrupt logic */
    bool check_interrupt();
    void execute_frame(bool update_screen);
    void single_step(bool update_screen);
    int loop_frame();
    bool cadence_allows();
    int loop_frame_vsync();
    int loop_frame_userevent();
    void handle_event(SDL_Event & event);
    void dump_memory(int start, int count);
    std::string read_memory(int start, int count);
    void write_memory_byte(int addr, int value);
    /* AA FF BB CC DD EE HH LL 00 00 00 00 SS PP 
     * 00 00 00 00 00 00 00 00 00 00 PP CC */
    std::string read_registers();
    void write_registers(uint8_t * regs);
    void debugger_attached();
    void debugger_detached();
    void debugger_break();
    void debugger_continue();
    std::string insert_breakpoint(int type, int addr, int kind);
    std::string remove_breakpoint(int type, int addr, int kind);
    bool check_breakpoint();
};
