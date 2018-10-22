#pragma once

#include <stdio.h>
#include <vector>
#include <functional>
#include <boost/thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/concurrent_queues/sync_priority_queue.hpp>
#include "SDL.h"
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
    std::vector<Watchpoint> memory_watchpoints;
    std::vector<Watchpoint> io_watchpoints;

public:
    std::function<void(void)> poll_debugger;
    std::function<void(void)> onbreakpoint;
    
    std::function<void(void)> onframetimer;

private:
    void refresh_watchpoint_listeners(void);

public:
    Board(Memory & _memory, IO & _io, PixelFiller & _filler, Soundnik & _snd, 
            TV & _tv, WavPlayer & _tape_player);

    void init();
    void reset(bool blkvvod);    // true: power-on reset, false: boot loaded prog
    void interrupt(bool on);
    /* Fuses together inner CPU logic and Vector-06c interrupt logic */
    bool check_interrupt();
    int execute_frame(bool update_screen);
    int execute_frame_with_cadence(bool update_screen, bool use_cadence);
    void single_step(bool update_screen);
    void render_frame(int frame, bool executed); 
    bool cadence_allows();
    int get_frame_no() const { return frame_no; }

    void handle_event(SDL_Event & event);
    void handle_keyup(SDL_KeyboardEvent & key);
    void handle_keydown(SDL_KeyboardEvent & key);
    void handle_window_event(SDL_Event & event);
    void handle_quit();
    bool terminating() const { return io.the_keyboard().terminate; };
    void toggle_fullscreen() { tv.toggle_fullscreen(); }

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
    void check_watchpoint(uint32_t addr, uint8_t value, int how);

    void pause_sound(bool topause) { soundnik.pause((int)topause); }
};

class Emulator {
private:
    enum event_type {
        EXECUTE_FRAME,
        KEYDOWN,
        KEYUP,
        QUIT,

        RENDER,
    };

    struct threadevent {
        event_type type;
        int data;
        int frame_no;
        SDL_KeyboardEvent key;
        threadevent() {}
        threadevent(event_type t, int d) : type(t), data(d) {}
        threadevent(event_type t, int d, int frameno) : type(t), data(d),
            frame_no(frameno) {}
        threadevent(event_type t, int d, SDL_KeyboardEvent k) : 
            type(t), data(d), key(k) {}

        bool operator <(const threadevent& other) const
        {
            return false;
        }
    };

    boost::thread thread;
    Board & board;

    boost::sync_queue<threadevent> ui_to_engine_queue;
    boost::sync_priority_queue<threadevent> engine_to_ui_queue;

public:
    Emulator(Board & borat);
    void threadfunc();
    void handle_threadevent(threadevent & ev);
    void handle_renderqueue(SDL_Event & event, bool & stopping);
    void start_emulator_thread();
    void run_event_loop();
    void join_emulator_thread();
    void inject_timer_event();
    bool handle_keyboard_event(SDL_KeyboardEvent & event);
    int wait_event(SDL_Event * event, int timeout);
};
