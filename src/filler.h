#pragma once

#include "globaldefs.h"

#include "memory.h"
#include "io.h"
#include "tv.h"

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

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
    uint32_t pixel32_grouped;
    uint32_t * mem32;

    Memory & memory;
    IO & io;
    TV & tv;
public:
    bool brk;
    bool irq;

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
        uint32_t x = this->pixel32 = this->mem32[0x2000 + addr];

        // h/t Code generator for bit permutations
        // http://programming.sirrida.de/calcperm.php
        // Input:
        // 31 23 15 7 30 22 14 6 29 21 13 5 28 20 12 4 27 19 11 3 26 18 10 2 25 17 9 1  24 16 8 0
        // LSB, indices refer to source, Beneš/BPC
        x = bit_permute_step(x, 0x00550055, 9);  // Bit index swap+complement 0,3
        x = bit_permute_step(x, 0x00003333, 18);  // Bit index swap+complement 1,4
        x = bit_permute_step(x, 0x000f000f, 12);  // Bit index swap+complement 2,3
        x = bit_permute_step(x, 0x000000ff, 24);  // Bit index swap+complement 3,4

        this->pixel32_grouped = x;
    }

    int shiftOutPixels()
    {
//        uint32_t p = this->pixel32;
//        // msb of every byte in p stands for bit plane
//        uint32_t modeless = (p >> 4 & 8) | (p >> 13 & 4) | (p >> 22 & 2) | (p >> 31 & 1);
//        // shift left
//        this->pixel32 = (p << 1);// & 0xfefefefe; -- unnecessary
//        return modeless;
        uint32_t modeless = this->pixel32_grouped >> 28;
        this->pixel32_grouped <<= 4;
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

    int fill(int clocks, int commit_time, int commit_time_pal, bool updateScreen) 
    {
        if (commit_time || commit_time_pal || 
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

#if ZEALOUS_LOCALITY
    int fill1(int clocks, int commit_time, int commit_time_pal, bool updateScreen) 
    {
        uint32_t * bmp = this->tv.pixels();
        int clk;

        int ofs = this->bmpofs;
        int raster_pixel_loc = this->raster_pixel;
        int raster_line_loc = this->raster_line;
        bool vborder_loc = this->vborder;
        bool visible_loc = this->visible;
        bool mode512_loc = this->mode512;

        // clocks=16/32/48/64/80/96..
        for (clk = 0; clk < clocks && !this->brk; clk += 2) {
            // offset for matching border/palette writes and the raster -- test:bord2
            const int rpixel = raster_pixel_loc - 24;
            const bool border = vborder_loc || 
                /* hborder */ (rpixel < (768-512)/2) || (rpixel >= (768 - (768-512)/2));
            const int index = this->getColorIndex(rpixel, border);
            if (clk == commit_time) {
                this->io.commit(); // regular i/o writes (border index); test: bord2
                mode512_loc = this->mode512;        
            }
            if (clk == commit_time_pal) {
                this->io.commit_palette(index); // palette writes; test: bord2
            }
            if (visible_loc) {
                const int bmp_x = raster_pixel_loc - this->center_offset;
                if (bmp_x >= 0 && bmp_x < this->screen_width) {
                    if (mode512_loc && !border) {
                        bmp[ofs++] = this->io.Palette(index & 0x03);
                        bmp[ofs++] = this->io.Palette(index & 0x0c);
                    } else {
                        uint32_t p = this->io.Palette(index);
                        bmp[ofs++] = p;
                        bmp[ofs++] = p;
                    }
                }
            }
            // 22 vsync + 18 border + 256 picture + 16 border = 312 lines
            raster_pixel_loc += 2;
            if (raster_pixel_loc == 768) {
                this->brk = this->advanceLine(raster_pixel_loc, raster_line_loc,
                        vborder_loc, visible_loc,
                        updateScreen);
            }
            // load scroll register at this precise moment -- test:scrltst2
            if (raster_line_loc == 22 + 18 && raster_pixel_loc == 150) {
                this->fb_row = this->io.ScrollStart();
            }
            // irq time -- test:bord2
            else if (raster_line_loc == 0 && raster_pixel_loc == 176) {
                this->irq = true;
            }
        } 

        this->bmpofs = ofs;
        this->raster_pixel = raster_pixel_loc;
        this->raster_line = raster_line_loc;
        this->vborder = vborder_loc;
        this->visible = visible_loc;
        return clk;
    }

    bool advanceLine(int & _raster_pixel, int & _raster_line, 
            bool & _vborder, bool & _visible, bool updateScreen) {
        _raster_pixel = 0;
        _raster_line += 1;
        this->fb_row -= 1;
        if (!_vborder && this->fb_row < 0) {
            this->fb_row = 0xff;
        }
        // update vertical border only when line changes
        _vborder = (_raster_line < 40) || (_raster_line >= (40 + 256));
        // turn on pixel copying after blanking area
        _visible = _visible || 
            (updateScreen && _raster_line == this->first_visible_line);
        if (_raster_line == 312) {
            _raster_line = 0;
            _visible = false; // blanking starts
            //printf("fill1: %d fill2: %d sum=%d\n", fill1_count, fill2_count,
            //        fill1_count + fill2_count);
            fill1_count = fill2_count = 0;
            return true;
        }
        return false;
    }
#else
    int fill1(int clocks, int commit_time, int commit_time_pal, bool updateScreen) {
        uint32_t * bmp = this->tv.pixels();
        int clk;

        for (clk = 0; clk < clocks && !this->brk; clk += 2) {
            // offset for matching border/palette writes and the raster -- test:bord2
            const int rpixel = this->raster_pixel - 24;
            bool border = this->vborder || 
                /* hborder */ (rpixel < (768-512)/2) || (rpixel >= (768 - (768-512)/2));
    
            int index = this->getColorIndex(rpixel, border);
            if (clk == commit_time) {
                this->io.commit(); // regular i/o writes (border index); test: bord2
            }
            if (clk == commit_time_pal) {
                this->io.commit_palette(index); // palette writes; test: bord2
            }
            if (this->visible) {
                const int bmp_x = this->raster_pixel - this->center_offset;
                if (bmp_x >= 0 && bmp_x < this->screen_width) {
                    if (this->mode512 && !border) {
                        bmp[this->bmpofs++] = this->io.Palette(index & 0x03);
                        bmp[this->bmpofs++] = this->io.Palette(index & 0x0c);
                    } else {
                        uint32_t p = this->io.Palette(index);
                        bmp[this->bmpofs++] = p;
                        bmp[this->bmpofs++] = p;
                    }
                }
            }
            // 22 vsync + 18 border + 256 picture + 16 border = 312 lines
            this->raster_pixel += 2;
            if (this->raster_pixel == 768) {
                this->advanceLine(updateScreen);
            }
            // load scroll register at this precise moment -- test:scrltst2
            if (this->raster_line == 22 + 18 && this->raster_pixel == 150) {
                this->fb_row = this->io.ScrollStart();
            }
            // irq time -- test:bord2
            else if (this->raster_line == 0 && this->raster_pixel == 176) {
                this->irq = true;
            }
        } 
        return clk;
    }

    void advanceLine(bool updateScreen) {
        this->raster_pixel = 0;
        this->raster_line += 1;
        this->fb_row -= 1;
        if (!this->vborder && this->fb_row < 0) {
            this->fb_row = 0xff;
        }
        // update vertical border only when line changes
        this->vborder = (this->raster_line < 40) || (this->raster_line >= (40 + 256));
        // turn on pixel copying after blanking area
        this->visible = this->visible || 
            (updateScreen && this->raster_line == this->first_visible_line);
        if (this->raster_line == 312) {
            this->raster_line = 0;
            this->visible = false; // blanking starts
            this->brk = true;
        }
    }
#endif

    /* simple fill, no out instructions underway, mode 256 */
    int fill2(int clocks)
    {
        uint32_t * const bmp = this->tv.pixels();
        int clk;

        int ofs = this->bmpofs;

        // clocks=16/32/48/64/80/96..

        int rpixel = this->raster_pixel - 24;
        this->raster_pixel += clocks;

        for (clk = 0; clk < clocks; clk += 16) {
            uint32_t p0 = this->getColorIndex(rpixel, false); rpixel += 2;
            uint32_t p1 = this->getColorIndex(rpixel, false); rpixel += 2;
            uint32_t p2 = this->getColorIndex(rpixel, false); rpixel += 2;
            uint32_t p3 = this->getColorIndex(rpixel, false); rpixel += 2;
            uint32_t p4 = this->getColorIndex(rpixel, false); rpixel += 2;
            uint32_t p5 = this->getColorIndex(rpixel, false); rpixel += 2;
            uint32_t p6 = this->getColorIndex(rpixel, false); rpixel += 2;
            uint32_t p7 = this->getColorIndex(rpixel, false); rpixel += 2;

#if __ARM_NEON
            uint32x4_t d0,d1,d2,d3;

            p0 = this->io.Palette(p0);
            p1 = this->io.Palette(p1);
            d0 = vsetq_lane_u32(p0, d0, 0);
            d0 = vsetq_lane_u32(p0, d0, 1);
            d0 = vsetq_lane_u32(p1, d0, 2);
            d0 = vsetq_lane_u32(p1, d0, 3);

            p2 = this->io.Palette(p2);
            p3 = this->io.Palette(p3);
            d1 = vsetq_lane_u32(p2, d1, 0);
            d1 = vsetq_lane_u32(p2, d1, 1);
            d1 = vsetq_lane_u32(p3, d1, 2);
            d1 = vsetq_lane_u32(p3, d1, 3);

            p4 = this->io.Palette(p4);
            p5 = this->io.Palette(p5);
            d2 = vsetq_lane_u32(p4, d2, 0);
            d2 = vsetq_lane_u32(p4, d2, 1);
            d2 = vsetq_lane_u32(p5, d2, 2);
            d2 = vsetq_lane_u32(p5, d2, 3);

            p6 = this->io.Palette(p6);
            p7 = this->io.Palette(p7);
            d3 = vsetq_lane_u32(p6, d3, 0);
            d3 = vsetq_lane_u32(p6, d3, 1);
            d3 = vsetq_lane_u32(p7, d3, 2);
            d3 = vsetq_lane_u32(p7, d3, 3);


            vst1q_u32(&bmp[ofs], d0); ofs+= 4;
            vst1q_u32(&bmp[ofs], d1); ofs+= 4;
            vst1q_u32(&bmp[ofs], d2); ofs+= 4;
            vst1q_u32(&bmp[ofs], d3); ofs+= 4;
#else
            p0 = this->io.Palette(p0);
            p1 = this->io.Palette(p1);
            p2 = this->io.Palette(p2);
            p3 = this->io.Palette(p3);
            p4 = this->io.Palette(p4);
            p5 = this->io.Palette(p5);
            p6 = this->io.Palette(p6);
            p7 = this->io.Palette(p7);

            bmp[ofs++] = p0; bmp[ofs++] = p0;
            bmp[ofs++] = p1; bmp[ofs++] = p1;
            bmp[ofs++] = p2; bmp[ofs++] = p2;
            bmp[ofs++] = p3; bmp[ofs++] = p3;
            bmp[ofs++] = p4; bmp[ofs++] = p4;
            bmp[ofs++] = p5; bmp[ofs++] = p5;
            bmp[ofs++] = p6; bmp[ofs++] = p6;
            bmp[ofs++] = p7; bmp[ofs++] = p7;
#endif

        } 

        this->bmpofs = ofs;
        return clk;
    }

    /* simple fill, no out instructions underway, mode 512 */
    int fill3(int clocks)
    {
        uint32_t * const bmp = this->tv.pixels();
        int clk;

        int ofs = this->bmpofs;

        // clocks=16/32/48/64/80/96..

        int rpixel = this->raster_pixel - 24;
        this->raster_pixel += clocks;

        int index;
        for (clk = 0; clk < clocks; clk += 16) {
#if __ARM_NEON
            uint32x4_t d0,d1,d2,d3;

            index = this->getColorIndex(rpixel, false); rpixel += 2;
            d0 = vsetq_lane_u32(this->io.Palette(index & 0x03), d0, 0);
            d0 = vsetq_lane_u32(this->io.Palette(index & 0x0c), d0, 1);
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            d0 = vsetq_lane_u32(this->io.Palette(index & 0x03), d0, 2);
            d0 = vsetq_lane_u32(this->io.Palette(index & 0x03), d0, 3);

            index = this->getColorIndex(rpixel, false); rpixel += 2;
            d1 = vsetq_lane_u32(this->io.Palette(index & 0x03), d1, 0);
            d1 = vsetq_lane_u32(this->io.Palette(index & 0x0c), d1, 1);
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            d1 = vsetq_lane_u32(this->io.Palette(index & 0x03), d1, 2);
            d1 = vsetq_lane_u32(this->io.Palette(index & 0x03), d1, 3);


            index = this->getColorIndex(rpixel, false); rpixel += 2;
            d2 = vsetq_lane_u32(this->io.Palette(index & 0x03), d2, 0);
            d2 = vsetq_lane_u32(this->io.Palette(index & 0x0c), d2, 1);
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            d2 = vsetq_lane_u32(this->io.Palette(index & 0x03), d2, 2);
            d2 = vsetq_lane_u32(this->io.Palette(index & 0x03), d2, 3);


            index = this->getColorIndex(rpixel, false); rpixel += 2;
            d3 = vsetq_lane_u32(this->io.Palette(index & 0x03), d3, 0);
            d3 = vsetq_lane_u32(this->io.Palette(index & 0x0c), d3, 1);
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            d3 = vsetq_lane_u32(this->io.Palette(index & 0x03), d3, 2);
            d3 = vsetq_lane_u32(this->io.Palette(index & 0x03), d3, 3);

            vst1q_u32(&bmp[ofs], d0); ofs+= 4;
            vst1q_u32(&bmp[ofs], d1); ofs+= 4;
            vst1q_u32(&bmp[ofs], d2); ofs+= 4;
            vst1q_u32(&bmp[ofs], d3); ofs+= 4;
#else
            //
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            bmp[ofs++] = this->io.Palette(index & 0x03);
            bmp[ofs++] = this->io.Palette(index & 0x0c);
            //
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            bmp[ofs++] = this->io.Palette(index & 0x03);
            bmp[ofs++] = this->io.Palette(index & 0x0c);
            //
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            bmp[ofs++] = this->io.Palette(index & 0x03);
            bmp[ofs++] = this->io.Palette(index & 0x0c);
            //
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            bmp[ofs++] = this->io.Palette(index & 0x03);
            bmp[ofs++] = this->io.Palette(index & 0x0c);
            //
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            bmp[ofs++] = this->io.Palette(index & 0x03);
            bmp[ofs++] = this->io.Palette(index & 0x0c);
            //
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            bmp[ofs++] = this->io.Palette(index & 0x03);
            bmp[ofs++] = this->io.Palette(index & 0x0c);
            //
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            bmp[ofs++] = this->io.Palette(index & 0x03);
            bmp[ofs++] = this->io.Palette(index & 0x0c);
            //
            index = this->getColorIndex(rpixel, false); rpixel += 2;
            bmp[ofs++] = this->io.Palette(index & 0x03);
            bmp[ofs++] = this->io.Palette(index & 0x0c);
#endif
        } 

        this->bmpofs = ofs;
        return clk;
    }

    int fill4(int clocks)
    {
        uint32_t * const bmp = this->tv.pixels();
        int clk;

        int ofs = this->bmpofs;
        this->raster_pixel += clocks;

        uint32_t p = this->io.Palette(this->getColorIndex(0, true));
        uint64_t p64 = p | (uint64_t)p<<32;
        for (clk = 0; clk < clocks; clk += 16) {
            *(uint64_t*)&bmp[ofs] = p64; ofs += 2;
            *(uint64_t*)&bmp[ofs] = p64; ofs += 2;
            *(uint64_t*)&bmp[ofs] = p64; ofs += 2;
            *(uint64_t*)&bmp[ofs] = p64; ofs += 2;
            *(uint64_t*)&bmp[ofs] = p64; ofs += 2;
            *(uint64_t*)&bmp[ofs] = p64; ofs += 2;
            *(uint64_t*)&bmp[ofs] = p64; ofs += 2;
            *(uint64_t*)&bmp[ofs] = p64; ofs += 2;
        } 

        this->bmpofs = ofs;
        return clk;
    }


};
