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
#include "util.h"

#if USED_XXD
// boots.o made using xxd already has okay symbols
extern "C" unsigned char * boots_bin;
extern "C" unsigned int boots_bin_len;
#else

#if MSYS_MINGW32
extern "C" uint8_t * binary_boots_bin_start;
extern "C" uint8_t * binary_boots_bin_end;
extern "C" size_t    binary_boots_bin_size;

#define boots_bin (binary_boots_bin_start)
#define boots_bin_len (&binary_boots_bin_size)
#else
extern "C" uint8_t * _binary_boots_bin_start;
extern "C" uint8_t * _binary_boots_bin_end;
extern "C" size_t    _binary_boots_bin_size;

#define boots_bin (_binary_boots_bin_start)
#define boots_bin_len (&_binary_boots_bin_size)
#endif
#endif

using namespace i8080cpu;

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

void Board::init_bootrom()
{
#if !defined(_MSC_VER)
    std::vector<uint8_t> userboot = util::load_binfile(Options.bootromfile);
    if (userboot.size() > 0) {
        printf("User bootrom: %s (%d bytes)\n", Options.bootromfile.c_str(),
                (int)userboot.size());
        this->boot = userboot;
    }
    else 
#endif
    {
        // inialize bootrom using default boot
        this->boot.resize((size_t)boots_bin_len);
        uint8_t * src = (uint8_t *) &boots_bin;
        size_t size = (size_t) boots_bin_len;
        for (unsigned i = 0; i < size; ++i) {
            this->boot[i] = src[i];
        }
    }
}

void Board::reset(Board::ResetMode mode)
{
    this->soundnik.reset();

    switch (mode) {
        case ResetMode::BLKVVOD:
            if (this->boot.size() == 0) {
                this->init_bootrom();
            }
            this->memory.attach_boot(boot);
            printf("Board::reset() attached boot, size=%u\n",
                    (unsigned int)boot.size());
            break;
        case ResetMode::BLKSBR:
            this->memory.detach_boot();
            printf("Board::reset() detached boot\n");
            break;
        case ResetMode::LOADROM:
            this->memory.detach_boot();
            i8080_jump(Options.pc);
            i8080_setreg_sp(0xc300);
            printf("Board::reset() detached boot, pc=%04x sp=%04x\n",
                    i8080_pc(), i8080_regs_sp());
            break;
    }

    this->interrupt(false);
    last_opcode = 0;
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
    if (this->hooks.frame) this->hooks.frame(this->frame_no);
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
            
            printf("read_registers: %s\n", read_registers().c_str());
            printf("Board::execute_frame: this->debugger_interrupt = true;\n");

            if (this->onbreakpoint) this->onbreakpoint();
            if (this->debugger_interrupt) {
                break;
            }
        }

        this->single_step(update_screen);
    }
    //printf("between = %d\n", this->between);
    return 1;
}

int Board::execute_frame_with_cadence(bool update_screen, bool use_cadence)
{
    volatile bool c = cadence_allows();
    return (c || !use_cadence) && execute_frame(update_screen);
}

void Board::single_step(bool update_screen)
{
#if MEGATRACE
    printf("PC=%04x %02x %02x %02x A=%02x BC=%04x DE=%04x HL=%04x SP=%04x M=%02x\n", i8080_pc(),
                this->memory.read(i8080_pc(), false),
                this->memory.read(i8080_pc()+1, false),
                this->memory.read(i8080_pc()+2, false),
                i8080_regs_a(),
                i8080_regs_bc(),
                i8080_regs_de(),
                i8080_regs_hl(),
                i8080_regs_sp(),
                this->memory.read(i8080_regs_hl(), false),
                );
#endif
    if (this->instr_time && this->between == 0) {
        this->between += this->instr_time;
        this->instr_time = 0;
    }

    this->instr_time += i8080_instruction(&this->last_opcode);

    int commit_time = -1, commit_time_pal = -1;
    if (this->last_opcode == 0xd3) {
        commit_time = (this->instr_time - 4) << 2;
        commit_time_pal = commit_time - 20;
    }

    /* afterbrk counts extra 12MHz cycles spent after the screen end */
    int afterbrk12 = this->filler.fill(this->instr_time << 2, commit_time,
            commit_time_pal, update_screen);

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

    if (this->frame_no > 60) {
        this->tape_player.advance(this->instr_time);
    }
    this->soundnik.soundSteps(this->instr_time/2, this->io.TapeOut(),
            this->io.Covox(), this->tape_player.sample());

    /* Edge conditions at the end of screen:
     * if instruction time does not fit within screen time, filler returns
     * the extra cycles in afterbrk12. We remember them in this->instr_time
     * to add them to this->between in the beginning of the next frame. */
    this->between += this->instr_time;
    this->instr_time = afterbrk12 >> 2;
    this->between -= this->instr_time;

    if (debug_on_single_step) 
    {
        auto addr = i8080cpu::i8080_pc();
        auto bigaddr = memory.bigram_select(addr & 0xffff, false);
        debug_on_single_step(bigaddr);
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

void Board::handle_event(SDL_Event & event)
{
    //printf("handle_event: event.type=%d\n", event.type);
    switch(event.type) {
        case SDL_KEYDOWN:
            this->handle_keydown(event.key);
            break;
        case SDL_KEYUP:
            this->handle_keyup(event.key);
            break;
        case SDL_WINDOWEVENT:
            this->tv.handle_window_event(event);
            break;
        case SDL_QUIT:
            this->handle_quit();
            break;
        default:
            break;
    }
}

/* emulator thread */
void Board::handle_keydown(SDL_KeyboardEvent & key)
{
    this->io.the_keyboard().key_down(key);
}

/* emulator thread */
void Board::handle_keyup(SDL_KeyboardEvent & key)
{
    this->io.the_keyboard().key_up(key);
}

/* emulator thread */
void Board::handle_quit()
{
    this->io.the_keyboard().terminate = true;
}

/* ui thread */
void Board::handle_window_event(SDL_Event & event)
{
    this->tv.handle_window_event(event);
}

/* ui thread: can't use this->frame_no */
void Board::render_frame(const int frame, const bool executed)
{
    tv.render(executed);
    if (Options.save_frames.size() && frame == Options.save_frames[0])
    {
        fprintf(stderr, "Saving frame %d to %s\n",
                frame, Options.path_for_frame(frame).c_str());
        tv.save_frame(Options.path_for_frame(frame));
        Options.save_frames.erase(Options.save_frames.begin());
    }
}

auto Board::read_stack(const size_t _len) const
->std::vector<uint16_t>
{
	auto sp = i8080_regs_sp();
	std::vector<uint16_t> out;

	for (int i = 0; i < _len; i++)
	{
		uint16_t db_l = memory.get_byte(sp, true);
        sp = (sp + 1) & 0xffff;
        uint16_t db_h = memory.get_byte(sp, true);
        sp = (sp + 1) & 0xffff;
        out.push_back(db_h<<8 | db_l);
	}
	return out;
}

void Board::dump_memory(const int start, const int count)
{
    for (int i = start; i < start + count; ++i) {
        printf("%02x ", memory.read(i, false));
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
}

const std::string Board::read_memory(const int start, const int count)
{
    // VLA: char buf[count * 2 + 1];
    std::string buf;
    buf.reserve(count * 2 + 1);
    for (int i = start, k = 0; i < start + count; ++i, k+=2) {
        sprintf(&buf[k], "%02x", memory.read(i, false));
    }
    return buf;
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

auto Board::read_registers_b()
->const std::vector<int>
{
    std::vector<int> out;
    out.push_back(i8080_regs_a()<<8 | i8080_regs_f());
    out.push_back(i8080_regs_bc());
    out.push_back(i8080_regs_de());
    out.push_back(i8080_regs_hl());
    out.push_back(i8080_regs_sp());
    out.push_back(i8080_pc());

    return out;
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

int Board::is_break()
{
    return debugger_interrupt;
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

static bool iospace(uint32_t addr)
{
    return (addr & 0x80000000) != 0;
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

void Board::refresh_watchpoint_listeners()
{
    auto check_wp_read =
        [this](uint32_t addr, uint32_t phys, bool stack, uint8_t value) {
            this->check_watchpoint(addr, value, Watchpoint::READ);
        };
    auto check_wp_write =
        [this](uint32_t addr, uint32_t phys, bool stack, uint8_t value) {
            this->check_watchpoint(addr, value, Watchpoint::WRITE);
        };
    auto check_wp_ioread =
        [this](uint32_t addr, uint8_t value) {
            this->ioread = -1;
            this->check_watchpoint(0x80000000|addr, value, Watchpoint::READ);
            return this->ioread;
        };
    auto check_wp_iowrite =
        [this](uint32_t addr, uint8_t value) {
            this->check_watchpoint(0x80000000|addr, value, Watchpoint::WRITE);
        };

    printf("--- watchpoint inventory ---\n");
    this->memory.onwrite = this->memory.onread = nullptr;
    this->io.onwrite = this->io.onread = nullptr;
    for (auto &w : this->memory_watchpoints) {
        if (this->memory.onwrite == nullptr &&
            (w.type == Watchpoint::WRITE || w.type == Watchpoint::ACCESS)) {
            if (iospace(w.addr)) {
                this->io.onwrite = check_wp_iowrite;
            } else {
                this->memory.onwrite = check_wp_write;
            }
            printf("write watchpoint: %08x,%x\n", w.addr, w.length);
        }
        if (this->memory.onread == nullptr &&
            (w.type == Watchpoint::READ || w.type == Watchpoint::ACCESS)) {
            if (iospace(w.addr)) {
                this->io.onread = check_wp_ioread;
            } else {
                this->memory.onwrite = check_wp_read;
            }
            printf("read watchpoint: %08x,%x\n", w.addr, w.length);
        }
    }

    printf("--- ---\n");
}

std::string Board::insert_breakpoint(int type, int addr, int kind)
{
    auto add_memory_watchpoint = [this](Watchpoint w) {
        this->memory_watchpoints.push_back(w);
	this->refresh_watchpoint_listeners();
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
        auto & v = this->memory_watchpoints;
        v.erase(std::remove(v.begin(), v.end(), w), v.end());
	    this->refresh_watchpoint_listeners();
    };

    switch (type) {
        case 0:
        case 1:
            {
            Breakpoint needle(addr, kind);
            auto & v = this->breakpoints;
            v.erase(std::remove(v.begin(), v.end(), needle), v.end());
            printf("deleted breakpoint @%04x, kind=%d\n", addr, kind);
            }
            return "OK";
        case 2:
            printf("deleting write watchpoint @%04x, length=%d\n", addr, kind);
            del_memory_watchpoint(Watchpoint(Watchpoint::WRITE, addr, kind));
            return "OK";
        case 3:
            printf("deleting read watchpoint @%04x, length=%d\n", addr, kind);
            del_memory_watchpoint(Watchpoint(Watchpoint::READ, addr, kind));
            return "OK";
        case 4:
            printf("deleting access watchpoint @%04x, length=%d\n", addr, kind);
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

#include "serialize.h"

void Board::serialize(std::vector<uint8_t> &to) {
    this->memory.serialize(to);
    this->io.serialize(to);
    i8080cpu::serialize(to);

    this->serialize_self(to);
}

void Board::serialize_self(SerializeChunk::stype_t & to) const
{
    SerializeChunk::stype_t chunk;
    chunk.push_back(static_cast<uint8_t>(this->inte));
    chunk.push_back(static_cast<uint8_t>(this->irq));
    chunk.push_back(static_cast<uint8_t>(this->irq_carry));
    SerializeChunk::insert_chunk(to, SerializeChunk::BOARD, chunk);
}

void Board::deserialize_self(SerializeChunk::stype_t::iterator from, uint32_t size)
{
    this->inte = static_cast<bool>(*from++);
    this->irq = static_cast<bool>(*from++);
    this->irq_carry = static_cast<bool>(*from++);
}

bool Board::deserialize(std::vector<uint8_t> &from) {
    auto it = from.begin();
    uint32_t size;
    bool result = true;
    for(;it != from.end();) {
        SerializeChunk::id signature;
        auto begin = SerializeChunk::take_chunk(it, signature, size);
        it = begin + size;
        if (size > 0) {
            switch (signature) {
                case SerializeChunk::MEMORY:
                    this->memory.deserialize(begin, size);
                    break;
                case SerializeChunk::IO:
                    this->io.deserialize(begin, size);
                    break;
                case SerializeChunk::CPU:
                    i8080cpu::deserialize(begin, size);
                    break;
                case SerializeChunk::BOARD:
                    this->deserialize_self(begin, size);
                    break;
                default:
                    it = from.end();
                    result = false;
                    break;
            }
        }
    }
    return result;
}

void Board::set_joysticks(int joy_0e, int joy_0f)
{
  this->io.set_joysticks(joy_0e, joy_0f);
}
