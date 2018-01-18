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
#include "ay.h"
#include "wav.h"

static size_t islength(std::ifstream & is)
{
    is.seekg(0, is.end);
    size_t result = is.tellg();
    is.seekg(0, is.beg);
    return result;
}

void load_rom(Memory & memory)
{
    std::ifstream is(Options.romfile, std::ifstream::binary);
    if (is) {
        size_t length = islength(is);

        std::vector<uint8_t> rom_data;
        rom_data.resize(length);
        is.read((char *) rom_data.data(), length);

        memory.init_from_vector(rom_data, Options.rom_org);
    }
}

void load_one_disk(FD1793 & fdc, int index, std::string & path)
{
    std::ifstream is(path, std::ifstream::binary);
    if (is) {
        size_t length = islength(is);

        std::vector<uint8_t> fdd_data;
        fdd_data.resize(length);
        is.read((char *) fdd_data.data(), length);
        
        fdc.loadDsk(index, path.c_str(), fdd_data);
    }
}

void load_wav(Wav & wav, std::string & path)
{
    std::ifstream is(path, std::ifstream::binary);
    if (is) {
        size_t length = islength(is);

        std::vector<uint8_t> wav_data;
        wav_data.resize(length);
        is.read((char *) wav_data.data(), length);
        
        wav.set_bytes(wav_data);
    }
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

int main(int argc, char ** argv)
{
    options(argc, argv);

    filler.init();
    soundnik.init();    // this may switch the audio output off
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
    else if (Options.wavfile.length() != 0) {
        load_wav(wav, Options.wavfile);
    }
    else {
        load_disks(fdc);
    }

    atexit(SDL_Quit);

    const int startframe = Options.vsync ? 20 : -1;
    if (startframe == -1) {
        // soundnik provides frame sync evens when there is no vsync
        // so it must be started, or we're going to be stuck waiting for event
        soundnik.pause(0);
    }
    for(int i = 0;; ++i) {
        board.loop_frame();
        if (i == startframe) {
            soundnik.pause(0);
            printf("Starting audio\n");
        }
        tv.render();

        if (Options.save_frames.size() && i == Options.save_frames[0])
        {
            tv.save_frame(Options.path_for_frame(i));
            Options.save_frames.erase(Options.save_frames.begin());
        }

        if (keyboard.terminate || i == Options.max_frame) {
            break;
        }
    }
}
