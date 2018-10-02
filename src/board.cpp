#include <stdio.h>
#include <vector>
#include <functional>
#include <algorithm>
#include "i8080.h"
#include "filler.h"
#include "sound.h"
#include "tv.h"
#include "cadence.h"
#include "breakpoint.h"
#include "board.h"

#if USED_XXD
// boots.o made using xxd already has okay symbols
extern "C" unsigned char * boots_bin;
extern "C" unsigned int boots_bin_len;
#else

extern "C" uint8_t * _binary_boots_bin_start;
extern "C" uint8_t * _binary_boots_bin_end;
extern "C" size_t    _binary_boots_bin_size;

#define boots_bin (_binary_boots_bin_start)
#define boots_bin_len (&_binary_boots_bin_size)

#endif

Board::Board(Memory & _memory, IO & _io, PixelFiller & _filler, Soundnik & _snd, 
        TV & _tv, WavPlayer & _tape_player) 
    : memory(_memory), io(_io), filler(_filler), soundnik(_snd), tv(_tv), 
    tape_player(_tape_player), 
    debugging(0), debugger_interrupt(0)
{
    this->inte = false;
}

void Board::init()
{
    i8080_hal_bind(memory, io, *this);
    cadence::set_cadence(this->tv.get_refresh_rate(), cadence_frames, 
            cadence_length);
    io.rgb2pixelformat = tv.get_rgb2pixelformat();
    create_timer();
}

void Board::reset(bool blkvvod)    // true: power-on reset, false: boot loaded prog
{
    if (blkvvod) {
        //uint8_t * src = (uint8_t *) &_binary_boots_bin_start;
        //size_t size = (size_t)(&_binary_boots_bin_size);
        uint8_t * src = (uint8_t *) &boots_bin;
        size_t size = (size_t) boots_bin_len;
        vector<uint8_t> boot(size);
        for (unsigned i = 0; i < size; ++i) {
            boot[i] = src[i];
        }
        this->memory.attach_boot(boot);
        printf("Board::reset() attached boot, size=%u\n", (unsigned int)size);
    } else {
        this->memory.detach_boot();
        printf("Board::reset() detached boot\n");
        //this->dump_memory(0x100, 0x100);
    }

    last_opcode = 0;
    irq = false;
    i8080_init();
}

void Board::interrupt(bool on)
{
    this->inte = on;
    this->irq &= on;
}

/* Fuses together inner CPU logic and Vector-06c interrupt logic */
bool Board::check_interrupt()
{
    if (this->irq && i8080_iff()) {
        this->interrupt(false);     // lower INTE which clears INT request on D65.2
        if (this->last_opcode == 0x76) {
            i8080_jump(i8080_pc() + 1);
        }
        i8080_execute(0xff);    // rst7
        this->instr_time += 16;

        return true;
    }

    return false;
}
#define F1 8
#define F2 370
//#define DBG_FRM(a,b,bob) if (frame_no>=a && frame_no<=b) {bob;}
#define DBG_FRM(a,b,bob) {};
int Board::execute_frame(bool update_screen)
{
    if (this->poll_debugger) this->poll_debugger();
    if (this->debugger_interrupt) return 0;

    ++this->frame_no;
    this->filler.reset();

    this->irq_carry = false; // imitates cpu waiting after T2 when INTE

    // 59904 
    this->between = 0;
    DBG_FRM(F1,F2, printf("--- %d ---\n", this->frame_no));
    for (; !this->filler.brk && !this->debugger_interrupt;) {
        this->check_interrupt();
        this->filler.irq = false;
        //DBG_FRM(F1,F2,printf("%05d %04x: ", this->between + this->instr_time, i8080_pc()));
        if (this->debugging && this->check_breakpoint()) {
            this->debugger_interrupt = true;
            if (this->onbreakpoint) this->onbreakpoint();
            break;
        }

        this->single_step(update_screen);
    }
    //printf("between = %d\n", this->between);
    return 1;
}

void Board::single_step(bool update_screen)
{
    this->instr_time += i8080_instruction(&this->last_opcode);
    //DBG_FRM(F1,F2,printf("%02x irq=%d inte=%d\n", this->last_opcode, 
    //            this->irq, this->inte));
    if (this->last_opcode == 0xd3) {
        this->commit_time = this->instr_time - 5;
        this->commit_time = this->commit_time * 4 + 4;
        this->commit_time_pal = this->commit_time - 20;
    }

    int clk = this->filler.fill(this->instr_time << 2, this->commit_time,
            this->commit_time_pal, update_screen);

    DBG_FRM(F1,F2, if(this->filler.irq) {
            printf("irq_clk=%d\n", this->filler.irq_clk);
            });

    /* Interrupt logic 
     *  interrupt request is "pushed through" by VSYNC on /C if INTE (D65.2)
     *  int request is cleared by INTE low, which is DI or INTA
     *
     *  EI instruction sets INTE high, but holds acknowledge until an
     *  instruction after. A long string of EI holds on interrupt requests
     *  indefinitely (test: vst: Ei=7fab)
     *
     *  an instruction that has 5 T-states would leave the CPU waiting
     *  for 3 clock cycles. Interrupt happening during that period 
     *  will not be served until after this command is executed.
     *  test: vst MovR=1d37, MovM=1d36, C*-N=0e9b)
     */
    if (this->filler.irq) {
        int thresh = i8080_cycles();
        /* Adjust threshold of the last M-cycle of long instructions */
        /* test: vst */
        switch(thresh) {
            case 11:    thresh = 15; break; // T533
            case 13:    thresh = 15; break; // T4333 
            case 17:    thresh = 23; break; // T53333
            case 18:    thresh = 21; break; // T43335
        }
        if (this->filler.irq_clk > thresh * 4) {
            this->irq_carry = true;
        } else {
            this->irq |= this->inte && this->filler.irq;
        }
    } 
    else if (this->irq_carry) {
        this->irq_carry = false;
        this->irq |= this->inte;
    }

    int wrap = this->instr_time - (clk >> 2);
    int step = this->instr_time - wrap;
    if (this->frame_no > 60) {
        this->tape_player.advance(step);
    }
    for (int g = step/2; --g >= 0;) {
        this->soundnik.soundStep(2, this->io.TapeOut(), this->io.Covox(),
                this->tape_player.sample());
    }
    this->between += step;
    this->instr_time = wrap;

    /* commit time is always 32, commit_time_pal is always 12 */
    /* not sure if it's even possible for a commit not to finish in one
     * fill operation, but keeping this for the time being */
    this->commit_time -= clk;
    if (this->commit_time < 0) {
        this->commit_time = 0;
    }
    this->commit_time_pal -= clk;
    if (this->commit_time_pal < 0) {
        this->commit_time_pal = 0;
    }
}

int Board::loop_frame()
{
    if (Options.vsync /* && !this->debugger_interrupt*/ ) {
        return loop_frame_vsync();
    }
    else {
        return loop_frame_userevent();
    }
}

#if 0
int measured_framerate;
uint32_t ticks_start;

int  Board::measure_framerate()
{
    if (this->measured_framerate == 0) {
        if (this->ticks_start == 0 && this->frame_no == 60) {
            ticks_start = SDL_GetTicks();
        } else if (this->ticks_start != 0) {
            uint32_t ticks_now = SDL_GetTicks();
            if (ticks_now - this->ticks_start >= 1000) {
                this->measured_framerate = this->frame_no - 61;
                printf("Measured FPS: %d\n", this->measured_framerate);
                set_cadence(this->measured_framerate);
            }
        }
    }
    return this->measured_framerate;
}
#endif

bool Board::cadence_allows()
{
    if (this->cadence_frames) {
        int seq = this->cadence_seq++;
        if (this->cadence_seq == cadence_length) {
            this->cadence_seq = 0;
        }
        return this->cadence_frames[seq] != 0;
    }
    return true;
}

int Board::loop_frame_vsync()
{
#if 0
    measure_framerate();
#endif
    SDL_Event event;
    int result = 0;
    do {
        if (cadence_allows()) {
            result = this->execute_frame(true);
        }
        while (SDL_PollEvent(&event)) {
            handle_event(event);
        }
    } while(0);
    return result;
}

int Board::loop_frame_userevent()
{
    SDL_Event event;
    bool frame = false;
    while(!frame) {
        if (SDL_WaitEvent(&event)) {
            switch(event.type) {
                case SDL_USEREVENT:
                    this->execute_frame(true);
                    frame = true;
                    break;
                default:
                    handle_event(event);
                    break;
            }
        }
    }
    return 1;
}

void Board::handle_event(SDL_Event & event)
{
    //printf("handle_event: event.type=%d\n", event.type);
    switch(event.type) {
        case SDL_KEYDOWN:
            if (!this->tv.handle_keyboard_event(event.key)) {
                this->io.the_keyboard().key_down(event.key);
            }
            break;
        case SDL_KEYUP:
            this->io.the_keyboard().key_up(event.key);
            break;
        case SDL_WINDOWEVENT:
            this->tv.handle_window_event(event);
            break;
        case SDL_QUIT:
            this->io.the_keyboard().terminate = true;
            break;
        default:
            break;
    }
}

void Board::dump_memory(int start, int count)
{
    for (int i = start; i < start + count; ++i) {
        printf("%02x ", this->memory.read(i, false));
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
}

std::string Board::read_memory(int start, int count)
{
    char buf[count * 2 + 1];
    for (int i = start, k = 0; i < start + count; ++i, k+=2) {
        sprintf(&buf[k], "%02x", this->memory.read(i, false));
    }
    return std::string(buf);
}

void Board::write_memory_byte(int addr, int value)
{
    this->memory.write(addr, value, false);
}

/* AA FF BB CC DD EE HH LL 00 00 00 00 SS PP 
 * 00 00 00 00 00 00 00 00 00 00 PP CC */
std::string Board::read_registers()
{
    char buf[26 * 2 + 1];
    int o = 0;

    sprintf(&buf[o], "%02x%02x", i8080_regs_a(), i8080_regs_f()); o += 4;
    sprintf(&buf[o], "%02x%02x", i8080_regs_c(), i8080_regs_b());   o += 4;
    sprintf(&buf[o], "%02x%02x", i8080_regs_e(), i8080_regs_d());   o += 4;
    sprintf(&buf[o], "%02x%02x", i8080_regs_l(), i8080_regs_h());   o += 4;
    sprintf(&buf[o], "00000000");                                   o += 8;
    sprintf(&buf[o], "%02x%02x", i8080_regs_sp() & 0377,
            (i8080_regs_sp() >> 8) & 0377);                         o += 4;
    sprintf(&buf[o], "00000000000000000000");                       o += 20;
    sprintf(&buf[o], "%02x%02x", i8080_pc() & 0377, 
            (i8080_pc() >> 8) & 0377);

    return std::string(buf);
}

void Board::write_registers(uint8_t * regs)
{
    i8080_setreg_a(regs[0]);
    i8080_setreg_f(regs[1]);
    i8080_setreg_c(regs[2]);
    i8080_setreg_b(regs[3]);
    i8080_setreg_e(regs[4]);
    i8080_setreg_d(regs[5]);
    i8080_setreg_l(regs[6]);
    i8080_setreg_h(regs[7]);
    i8080_setreg_sp(regs[12] | (regs[13] << 8));
    i8080_jump(regs[24] | (regs[25] << 8));
}

void Board::debugger_attached()
{
    this->debugging = 1;
    this->debugger_break();
}

void Board::debugger_detached()
{
    this->debugging = 0;
    this->debugger_continue();
}

void Board::debugger_break()
{
    this->debugger_interrupt = 1;
}

void Board::debugger_continue()
{
    this->debugger_interrupt = 0;
}

void Board::check_watchpoint(uint32_t addr, uint8_t value, int how)
{
    //if (addr == 0x100) {
    //    printf("check_watchpoint how=%d\n", how);
    //}
    auto found = std::find_if(this->memory_watchpoints.begin(),
            this->memory_watchpoints.end(), 
            [addr, how](Watchpoint const & item) {
                if (item.type == Watchpoint::ACCESS || item.type == how) {
                    return addr >= item.addr && addr < (item.addr + item.length);
                }
                return false;
            });
    if (found != this->memory_watchpoints.end()) {
        this->debugger_interrupt = true;
        if (this->onbreakpoint) this->onbreakpoint();
    }
}

std::string Board::insert_breakpoint(int type, int addr, int kind)
{
    auto check_wp_read = 
        [this](uint32_t addr, uint32_t phys, bool stack, uint8_t value) {
            this->check_watchpoint(addr, value, Watchpoint::READ);
        };
    auto check_wp_write = 
        [this](uint32_t addr, uint32_t phys, bool stack, uint8_t value) {
            this->check_watchpoint(addr, value, Watchpoint::WRITE);
        };

    auto add_memory_watchpoint = 
        [this,check_wp_read,check_wp_write](Watchpoint w) {
        this->memory_watchpoints.push_back(w);

        printf("memory.onwrite, memory.onread reset\n");
        this->memory.onwrite = this->memory.onread = nullptr;
        for (auto &w : this->memory_watchpoints) {
            if (this->memory.onwrite == nullptr && 
                (w.type == Watchpoint::WRITE || w.type == Watchpoint::ACCESS)) {
                this->memory.onwrite = check_wp_write;
                printf("memory.onwrite set up\n");
            }
            if (this->memory.onread == nullptr &&
                (w.type == Watchpoint::READ || w.type == Watchpoint::ACCESS)) {
                this->memory.onread = check_wp_read;
                printf("memory.onread set up\n");
            }
        }
    };

    switch (type) {
        case 0: // software breakpoint
        case 1: // hardware breakpoint
            this->breakpoints.push_back(Breakpoint(addr, kind));
            printf("added breakpoint @%04x, kind=%d\n", addr, kind);
            return "OK";
        case 2: // write watchpoint @ addr, kind = number of bytes to watch
            add_memory_watchpoint(Watchpoint(Watchpoint::WRITE, addr, kind));
            printf("added write watchpoint @%04x, len=%d\n", addr, kind);
            return "OK";
        case 3: // read watchpoint
            add_memory_watchpoint(Watchpoint(Watchpoint::READ, addr, kind));
            printf("added read watchpoint @%04x, len=%d\n", addr, kind);
            return "OK";
        case 4: // access watchpoint
            add_memory_watchpoint(Watchpoint(Watchpoint::ACCESS, addr, kind));
            printf("added access watchpoint @%04x, len=%d\n", addr, kind);
            return "OK";
        default:
            break;
    }
    return ""; // not supported
}

std::string Board::remove_breakpoint(int type, int addr, int kind)
{
    auto del_memory_watchpoint = [this](Watchpoint w)
    {
        auto v = this->memory_watchpoints;
        v.erase(std::remove(v.begin(), v.end(), w), v.end());
    };

    Breakpoint needle(addr, kind);
    switch (type) {
        case 0:
        case 1:
            {
            auto & v = this->breakpoints;
            v.erase(std::remove(v.begin(), v.end(), needle), v.end());
            printf("deleted breakpoint @%04x, kind=%d\n", addr, kind);
            }
            return "OK";
        case 2:
            del_memory_watchpoint(Watchpoint(Watchpoint::WRITE, addr, kind));
            return "OK";
        case 3:
            del_memory_watchpoint(Watchpoint(Watchpoint::READ, addr, kind));
            return "OK";
        case 4:
            del_memory_watchpoint(Watchpoint(Watchpoint::ACCESS, addr, kind));
            return "OK";
    }
    return ""; // not supported
}

bool Board::check_breakpoint()
{
    return std::find(this->breakpoints.begin(), this->breakpoints.end(),
            Breakpoint(i8080_pc(), 1)) != this->breakpoints.end();
}

