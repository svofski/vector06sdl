#include <iostream>
#include <fstream>
#include <iterator>

#include "memory.h"
#include "io.h"
#include "tv.h"
#include "board.h"
#include "emulator.h"
#include "options.h"
#include "keyboard.h"
#include "8253.h"
#include "sound.h"
#include "ay.h"
#include "wav.h"
#include "server.h"
#include "SDL.h"
#include "util.h"

#if HAVE_GPERFTOOLS
#include <gperftools/profiler.h>
#endif

#include "scriptnik.h"

void load_rom(Memory & memory)
{
    memory.init_from_vector(util::load_binfile(Options.romfile), Options.rom_org);
}

void load_one_disk(FD1793 & fdc, int index, const std::string & path)
{
    fdc.loadDsk(index, path.c_str(), util::load_binfile(path));
}

void load_wav(Wav & wav, const std::string & path)
{
    wav.set_bytes(util::load_binfile(path));
}

void load_disks(FD1793 & fdc)
{
    for (unsigned i = 0; i < Options.fddfile.size(); ++i) {
        load_one_disk(fdc, i, Options.fddfile[i]);
    }
}

/* This block must be de-staticized, but currently trying to do so breaks tests */
Memory memory;
FD1793_Real fdc;
FD1793 fdc_dummy;
Wav wav;
WavPlayer tape_player(wav);
Keyboard keyboard;
I8253 timer;
TimerWrapper tw(timer);
AY ay;
AYWrapper aw(ay);
Soundnik soundnik(tw, aw);
IO io(memory, keyboard, timer, fdc, ay, tape_player);//Options.nofdc ? fdc_dummy : fdc);
TV tv;
PixelFiller filler(memory, io, tv);
Board board(memory, io, filler, soundnik, tv, tape_player);

GdbServer gdbserver(board);

Scriptnik scriptnik;

void bootstrap_scriptnik()
{
    int script_size = 0;
    for (auto & scriptfile : Options.scriptfiles) {
        script_size += scriptnik.append_from_file(scriptfile);
    }

    for (auto & arg : Options.scriptargs) {
        scriptnik.append_arg(arg);
    }

    if (script_size) {
        printf("Bootstrapping scriptnik...\n");

        scriptnik.loadwav = [](const std::string & filename) {
            load_wav(wav, filename);
            return 0;
        };

        // probably same as lambda..
        board.hooks.frame = std::bind(&Scriptnik::onframe, &scriptnik,
                std::placeholders::_1);

        tape_player.hooks.finished = 
            std::bind(&Scriptnik::onwavfinished, &scriptnik, std::placeholders::_1);

        scriptnik.keydown = [](int scancode) {
            SDL_KeyboardEvent e;
            e.keysym.scancode = (SDL_Scancode)scancode;
            io.the_keyboard().key_down(e);
        };

        scriptnik.keyup = [](int scancode) {
            SDL_KeyboardEvent e;
            e.keysym.scancode = (SDL_Scancode)scancode;
            io.the_keyboard().key_up(e);
        };

        scriptnik.insert_breakpoint = [](int type, int addr, int kind) {
            board.ioread = -1;
            return board.insert_breakpoint(type, addr, kind);
        };
        scriptnik.debugger_attached = []() {
            board.ioread = -1;
            return board.debugger_attached();
        };
        scriptnik.debugger_detached = []() {
            board.ioread = -1;
            return board.debugger_detached();
        };
        scriptnik.debugger_break = []() {
            board.ioread = -1;
            return board.debugger_break();
        };
        scriptnik.debugger_continue = []() {
            return board.debugger_continue();
        };
        board.onbreakpoint = []() {
            scriptnik.onbreakpoint();
        };
        scriptnik.read_register = [](const std::string & reg) {
            if (reg == "a") return i8080_regs_a();
            if (reg == "f") return i8080_regs_f();
            if (reg == "b") return i8080_regs_b();
            if (reg == "c") return i8080_regs_c();
            if (reg == "d") return i8080_regs_d();
            if (reg == "e") return i8080_regs_e();
            if (reg == "h") return i8080_regs_h();
            if (reg == "l") return i8080_regs_l();
            if (reg == "sp") return i8080_regs_sp();
            if (reg == "pc") return i8080_pc();
            return i8080_pc();
        };
        scriptnik.set_register = [](const std::string & reg, int val) {
            if (reg == "a") i8080_setreg_a(val & 0xff);
            if (reg == "f") i8080_setreg_f(val & 0xff);
            if (reg == "b") i8080_setreg_b(val & 0xff);
            if (reg == "c") i8080_setreg_c(val & 0xff);
            if (reg == "d") i8080_setreg_d(val & 0xff);
            if (reg == "e") i8080_setreg_e(val & 0xff);
            if (reg == "h") i8080_setreg_h(val & 0xff);
            if (reg == "l") i8080_setreg_l(val & 0xff);
            if (reg == "sp") i8080_setreg_sp(val & 0xffff);
            if (reg == "pc") i8080_jump(val & 0xffff);
            if (reg == "ioread") board.ioread = val & 0xff;
        };
        scriptnik.read_memory = [](int addr, int stackrq) {
            return (int)memory.read(addr, (bool)stackrq);
        };
        scriptnik.write_memory = [](int addr, int w8, int stackrq) {
            memory.write(addr, w8, stackrq);
        };

        scriptnik.start();
    }
}


int main(int argc, char ** argv)
{
    options(argc, argv);
    WavRecorder rec;
    WavRecorder * prec = 0;

    if (Options.audio_rec_path.length()) {
        rec.init(Options.audio_rec_path);
        prec = &rec;
    }

    gdbserver.init();

    filler.init();
    soundnik.init(prec);    // this may switch the audio output off
    tv.init();
    board.init();
    fdc.init();
    if (Options.bootpalette) {
        io.yellowblue();
    }

    //keyboard.onreset = [&board](bool blkvvod) {
    keyboard.onreset = [](bool blkvvod) {
        board.reset(blkvvod);
    };

    if (Options.autostart) {
        int seq = 0;
        io.onruslat = [&seq](bool ruslat) {
            seq = (seq << 1) | (ruslat ? 1 : 0);
            if ((seq & 15) == 6) {
                board.reset(false);
                io.onruslat = nullptr;
            }
        };
    }

    board.reset(true);

    if (Options.romfile.length() != 0) {
        load_rom(memory);
        board.reset(false);
    }

    if (Options.wavfile.length() != 0) {
        load_wav(wav, Options.wavfile);
    }

    //if (!Options.nofdc) {
        load_disks(fdc);
    //}

    atexit(SDL_Quit);

    board.poll_debugger = [](void) {
        gdbserver.poll();
    };

#if HAVE_GPERFTOOLS
    if (Options.profile) {
        printf("Enabling profile data collection to v06x.prof\n");
        ProfilerStart("v06x.prof");
    }
#endif

    bootstrap_scriptnik();

    Emulator lator(board);
    lator.start_emulator_thread();
    lator.run_event_loop();

#if HAVE_GPERFTOOLS
    if (Options.profile) {
        ProfilerStop();
    }
#endif

    return 0;
}
