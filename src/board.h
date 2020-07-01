#pragma once

#include <stdio.h>
#include <vector>
#include <functional>
#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
#include "SDL.h"
#else
#include "event.h"
#endif
#include "i8080.h"
#include "filler.h"
#include "sound.h"
#include "tv.h"
#include "cadence.h"
#include "breakpoint.h"
#include "serialize.h"

class Board;

void i8080_hal_bind(Memory & _mem, IO & _io, Board & _board);
void create_timer();

class Board
{
public:
    enum ResetMode {
        BLKSBR,
        BLKVVOD,
        LOADROM
    };

private:
    int between;
    int instr_time;
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

    std::vector<uint8_t> boot;

    int debugging;
    int debugger_interrupt;
    std::vector<Breakpoint> breakpoints;
    std::vector<Watchpoint> memory_watchpoints;
    std::vector<Watchpoint> io_watchpoints;

public:
    std::function<void(void)> poll_debugger;
    std::function<void(void)> onbreakpoint;
    
    std::function<void(void)> onframetimer;
    
    struct {
        std::function<void(int)> frame;
        std::function<void(int)> jump;
    } hooks;

    // a hack to pass return value from io.onread 
    int ioread;

private:
    void refresh_watchpoint_listeners(void);

private:
    void init_bootrom();

public:
    Board(Memory & _memory, IO & _io, PixelFiller & _filler, Soundnik & _snd, 
            TV & _tv, WavPlayer & _tape_player);

    void init();
    void reset(Board::ResetMode blkvvod);    // true: power-on reset, false: boot loaded prog
    int get_frame_no() const { return frame_no; }
    void handle_quit();
    bool terminating() const { return io.the_keyboard().terminate; };
    void interrupt(bool on);

    void handle_event(SDL_Event & event);
    void handle_keyup(SDL_KeyboardEvent & key);
    void handle_keydown(SDL_KeyboardEvent & key);
    void handle_window_event(SDL_Event & event);
    void toggle_fullscreen() { tv.toggle_fullscreen(); }
    void render_frame(int frame, bool executed); 
    void pause_sound(bool topause) { soundnik.pause((int)topause); }
    int execute_frame_with_cadence(bool update_screen, bool use_cadence);
    void single_step(bool update_screen);

    TV & get_tv() const { return tv; }
    Soundnik & get_soundnik() const { return soundnik; }

public:
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
    void check_watchpoint(uint32_t addr, uint8_t value, int how);

    void serialize(std::vector<uint8_t> & to);
    bool deserialize(std::vector<uint8_t> & from);
    void serialize_self(SerializeChunk::stype_t & to) const;
    void deserialize_self(SerializeChunk::stype_t::iterator from, uint32_t size);

private:
    /* Fuses together inner CPU logic and Vector-06c interrupt logic */
    bool check_interrupt();
    int execute_frame(bool update_screen);
    bool cadence_allows();
    void dump_memory(int start, int count);
};


