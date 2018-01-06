#pragma once

#include <stdio.h>
#include <vector>
#include "i8080.h"
#include "filler.h"
#include "sound.h"


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

    bool irq;
    bool iff;

    Memory & memory;
    IO & io;
    PixelFiller & filler;
    Soundnik & soundnik;

public:
    Board(Memory & _memory, IO & _io, PixelFiller & _filler, Soundnik & _snd) 
        : memory(_memory),
          io(_io), filler(_filler), soundnik(_snd), 
          iff(false)
    {
    }

    void init()
    {
        i8080_hal_bind(memory, io, *this);
        create_timer();
    }

    void reset(bool blkvvod)    // true: power-on reset, false: boot loaded prog
    {
        if (blkvvod) {
            //uint8_t * src = (uint8_t *) &_binary_boots_bin_start;
            //size_t size = (size_t)(&_binary_boots_bin_size);
            uint8_t * src = (uint8_t *) &boots_bin;
            size_t size = (size_t) boots_bin_len;
            vector<uint8_t> boot(size);
            for (int i = 0; i < size; ++i) {
                boot[i] = src[i];
            }
            this->memory.attach_boot(boot);
            printf("Board::reset() attached boot, size=%zu\n", size);
        } else {
            this->memory.detach_boot();
            printf("Board::reset() detached boot\n");
            //this->dump_memory(0x100, 0x100);
        }

        last_opcode = 0;
        irq = false;
        i8080_init();
    }

    void interrupt(bool on)
    {
        this->iff = on;
    }

    void check_interrupt()
    {
        if (this->irq && this->iff) {
            this->irq = false;
            if (this->last_opcode == 0x76) {
                i8080_jump(i8080_pc() + 1);
                //this.CPU.pc += 1;
            }
            //printf("check_interrupt\n");
            i8080_execute(0xf3);    // di
            i8080_execute(0xff);    // rst7
            this->instr_time += 16;
        }
    }

    void execute_frame(bool update_screen)
    {
        int cycles_count = 0;
        this->filler.reset();

        //for (;cycles_count < 59904;) {
        this->between = 0;
        for (; !this->filler.brk;) {
            this->check_interrupt();
            this->filler.irq = this->filler.irq && this->irq;
            this->instr_time += i8080_instruction(&last_opcode);
            if (last_opcode == 0xd3) {
                this->commit_time = this->instr_time - 5;
                this->commit_time = this->commit_time * 4 + 4;
                this->commit_time_pal = this->commit_time - 20;
            }

            int clk = this->filler.fill(this->instr_time << 2, this->commit_time,
                    this->commit_time_pal, update_screen);

            //printf("instr_time=%d clk=%d\n", instr_time, clk);
            this->irq = this->iff && this->filler.irq;
            int wrap = this->instr_time - (clk >> 2);
            int step = this->instr_time - wrap;
            for (int g = step/2; --g >= 0;) {
                this->soundnik.soundStep(2, this->io.TapeOut(), this->io.Covox());
            }
            this->between += step;
            this->instr_time = wrap;
            this->commit_time -= clk;
            this->commit_time_pal -= clk;
        }
        //printf("between = %d\n", this->between);
    }

    void loop_frame()
    {
        SDL_Event event;
        bool frame = false;
        while(!frame) {
            if (SDL_WaitEvent(&event)) {
                printf("event.type=%d\n", event.type);
                switch(event.type) {
                    case SDL_USEREVENT:
                        execute_frame(true);
                        frame = true;
                        break;
                    case SDL_KEYDOWN:
                        this->io.the_keyboard().key_down(event.key);
                        break;
                    case SDL_KEYUP:
                        this->io.the_keyboard().key_up(event.key);
                        break;
                    case SDL_QUIT:
                        this->io.the_keyboard().terminate = true;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    void dump_memory(int start, int count)
    {
        for (int i = start; i < start + count; ++i) {
            printf("%02x ", this->memory.read(i, false));
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
    }
};
