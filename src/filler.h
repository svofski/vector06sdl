#pragma once

#include "globaldefs.h"

#include "memory.h"
#include "io.h"
#include "tv.h"

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

    uint32_t pixel32;
    uint32_t * mem32;

    Memory & memory;
    IO & io;
    TV & tv;
public:
    bool brk;
    bool irq;

public:
    PixelFiller(Memory & _mem, IO & _io, TV & _tv): 
        memory(_mem), io(_io), tv(_tv),
        mode512(false)
    {
        //this->bmp = bmp;
        this->mem32 = (uint32_t *) _mem.buffer();
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

    void fetchPixels() 
    {
        size_t addr = ((this->fb_column & 0xff) << 8) | (this->fb_row & 0xff);
        this->pixel32 = this->mem32[0x2000 + addr];
    }

    int shiftOutPixels()
    {
        uint32_t p = this->pixel32;
        // msb of every byte in p stands for bit plane
        uint32_t modeless = (p >> 4 & 8) | (p >> 13 & 4) | (p >> 22 & 2) | (p >> 31 & 1);
        // shift left
        this->pixel32 = (p << 1);// & 0xfefefefe; -- unnecessary
        return modeless;
    }

    int fill(int clocks, int commit_time, int commit_time_pal, bool updateScreen) {
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
                const int bmp_x = this->raster_pixel - CENTER_OFFSET; // horizontal offset
                if (bmp_x >= 0 && bmp_x < SCREEN_WIDTH) {
                    if (this->mode512 && !border) {
                        bmp[this->bmpofs++] = this->io.Palette(index & 0x03);
                        bmp[this->bmpofs++] = this->io.Palette(index & 0x0c);
                    } else {
                        uint32_t p = this->io.Palette(index);
                        // test pattern for texture scaling
                        //if (this->raster_line & 1) {
                        //    p = 0xff000000;
                        //} else {
                        //    p = 0xffffffff;
                        //}

                        // Emulate vertical stripes when writing palette reg
                        //bmp[this->bmpofs++] = 
                        //    (clk == commit_time_pal + 4) ?
                        //        0xffffffff :
                        //        p;
                        bmp[this->bmpofs++] = p;
                        bmp[this->bmpofs++] = p;
                    }
                }
            }
            // 22 vsync + 18 border + 256 picture + 16 border = 312 lines
            this->raster_pixel += 2;
            if (this->raster_pixel == 768) {
                this->brk = this->advanceLine(updateScreen);
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

    bool advanceLine(bool updateScreen) {
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
            (updateScreen && this->raster_line == FIRST_VISIBLE_LINE);
        if (this->raster_line == 312) {
            this->raster_line = 0;
            this->visible = false; // blanking starts
            return true;
        }
        return false;
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
};
