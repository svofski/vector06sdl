#pragma once

#include "globaldefs.h"

#include "memory.h"
#include "io.h"
#include "tv.h"

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

//#define USE_BIT_PERMUTE 1

class PixelFiller
{
private:
    bool mode512;
    int raster_pixel;   // horizontal pixel counter
    int raster_line;    // raster line counter
    int fb_column;      // frame buffer column
    int fb_row;         // frame buffer row
    bool vborder;       // vertical border flag
    bool visible;       // visible area flag
    int bmpofs;         // bitmap offset for current pixel
    int border_index;
    int first_visible_line;
    int center_offset;
    int screen_width;

    uint32_t pixel32;
#if USE_BIT_PERMUTE
    uint32_t pixel32_grouped;
#endif
    uint32_t * mem32;

    Memory & memory;
    IO & io;
    TV & tv;
public:
    bool brk;
    bool irq;
    int irq_clk;

public:
    PixelFiller(Memory & _mem, IO & _io, TV & _tv): 
        memory(_mem), io(_io), tv(_tv)
    {
        this->mode512 = false;
        this->mem32 = (uint32_t *) this->memory.buffer();
        this->pixel32 = 0;  // 4 bytes of bit planes
        this->border_index = 0;

        this->reset();

        this->io.onborderchange = [this](int border) {
            //printf("onborderchange: %x\n", border);
            this->border_index = border;
        };

        this->io.onmodechange = [this](bool mode) {
            this->mode512 = mode;
        };
    }

    void init()
    {
        this->first_visible_line = 312 - Options.screen_height;
        this->center_offset = Options.center_offset;
        this->screen_width = Options.screen_width;
    }

    void reset()
    {
        this->raster_pixel = 0;   // horizontal pixel counter
        this->raster_line = 0;    // raster line counter
        this->fb_column = 0;      // frame buffer column
        this->fb_row = 0;         // frame buffer row
        this->vborder = true;     // vertical border flag
        this->visible = false;    // visible area flag
        this->bmpofs = 0;         // bitmap offset for current pixel
        this->brk = false;
        this->irq = false;
    }

    uint32_t bit_permute_step(uint32_t x, uint32_t m, uint32_t shift) {
        uint32_t t;
        t = ((x >> shift) ^ x) & m;
        x = (x ^ t) ^ (t << shift);
        return x;
    }

    void fetchPixels() 
    {
        size_t addr = ((this->fb_column & 0xff) << 8) | (this->fb_row & 0xff);
        this->pixel32 = this->mem32[0x2000 + addr];

#if USE_BIT_PERMUTE
        // h/t Code generator for bit permutations
        // http://programming.sirrida.de/calcperm.php
        // Input:
        // 31 23 15 7 30 22 14 6 29 21 13 5 28 20 12 4 27 19 11 3 26 18 10 2 25 17 9 1  24 16 8 0
        // LSB, indices refer to source, Beneš/BPC
        uint32_t x = this->pixel32;
        x = bit_permute_step(x, 0x00550055, 9);  // Bit index swap+complement 0,3
        x = bit_permute_step(x, 0x00003333, 18);  // Bit index swap+complement 1,4
        x = bit_permute_step(x, 0x000f000f, 12);  // Bit index swap+complement 2,3
        x = bit_permute_step(x, 0x000000ff, 24);  // Bit index swap+complement 3,4

        this->pixel32_grouped = x;
#endif
    }

    int shiftOutPixels()
    {
//        uint32_t p = this->pixel32;
//        // msb of every byte in p stands for bit plane
//        uint32_t modeless = (p >> 4 & 8) | (p >> 13 & 4) | (p >> 22 & 2) | (p >> 31 & 1);
//        // shift left
//        this->pixel32 = (p << 1);// & 0xfefefefe; -- unnecessary
//        return modeless;
#if USE_BIT_PERMUTE
        uint32_t modeless = this->pixel32_grouped >> 28;
        this->pixel32_grouped <<= 4;
#else
        uint32_t p = this->pixel32;
        // msb of every byte in p stands for bit plane
        uint32_t modeless = (p >> 4 & 8) | (p >> 13 & 4) | (p >> 22 & 2) | (p >> 31 & 1);
        // shift left
        this->pixel32 = (p << 1);// & 0xfefefefe; -- unnecessary
#endif
        return modeless;
    }

    int getColorIndex(int rpixel, bool border) {
        if (border) {
            this->fb_column = 0;
            return this->border_index;
        } else {
            if ((rpixel & 0x0f) == 0) {
                this->fetchPixels();
                ++this->fb_column;
            }
            return this->shiftOutPixels();
        }
    }

    int fill1_count, fill2_count;

#define TESTTABLE 0

    int fill(int clocks, int commit_time, int commit_time_pal, bool updateScreen) 
    {
        if (TESTTABLE || commit_time || commit_time_pal || 
                this->raster_line == 22 + 18 || 
                this->raster_line == 0 ||
                this->raster_line == 311 ||
                this->raster_pixel <= (768-512)/2 + clocks ||
                this->raster_pixel + clocks >= 768-(768-512)/2)
        {
            fill1_count += clocks;
            return fill1(clocks, commit_time, commit_time_pal, updateScreen);
        } else {
            fill2_count += clocks;
            if (!this->visible) {
                this->raster_pixel += clocks;
                return clocks;
            }
            else if (this->vborder) {
                return fill4(clocks);
            }
            else if (this->mode512) {
                return fill3(clocks);
            } 
            else {
                return fill2(clocks);
            }
        }
    }


    int fill1(int clocks, int commit_time, int commit_time_pal, bool updateScreen);
    int fill2(int clocks);
    int fill3(int clocks);
    int fill4(int clocks);
    void advanceLine(bool updateScreen);
};
