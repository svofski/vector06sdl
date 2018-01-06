#include <iostream>
#include <fstream>
#include <iterator>

#include "memory.h"
#include "io.h"
#include "tv.h"
#include "board.h"
#include "options.h"
#include "keyboard.h"
#include "8253.h"
#include "sound.h"

Memory memory;
Keyboard keyboard;
I8253 timer;
TimerWrapper tw(timer);
Soundnik soundnik(tw);
IO io(memory, keyboard, timer);
TV tv;
PixelFiller filler(memory, io, tv);
Board board(memory, io, filler, soundnik);

void load_rom()
{
    std::ifstream is(Options.romfile, std::ifstream::binary);
    if (is) {
        is.seekg(0, is.end);
        size_t length = is.tellg();
        is.seekg(0, is.beg);
        
        std::vector<uint8_t> rom_data;
        rom_data.resize(length);
        is.read((char *) rom_data.data(), length);

        memory.init_from_vector(rom_data, Options.rom_org);
    }
}

int main(int argc, char ** argv)
{
    options(argc, argv);

    atexit(SDL_Quit);

    board.init();
    tv.init();
    soundnik.init();
    if (Options.bootpalette) {
        io.yellowblue();
    }

    keyboard.onreset = [](bool blkvvod) {
        board.reset(blkvvod);
    };

    board.reset(true);

    if (Options.romfile.length() != 0) {
        load_rom();
        board.reset(false);
    }

    soundnik.pause(0);

    for(int i = 0;; ++i) {
        board.loop_frame();
        tv.render();

//        if (i == 64) {
//            printf("Unpausing audio\n");
//            soundnik.pause(0);
//        }

        if (Options.save_frames.size() && i == Options.save_frames[0])
        {
            tv.save_frame(Options.path_for_frame(i));
            Options.save_frames.erase(Options.save_frames.begin());
        }

        if (keyboard.terminate || i == Options.max_frame) {
            break;
        }
    }

    //board.dump_memory(0x8000, 0x8000);
}
