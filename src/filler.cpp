#include "globaldefs.h"
#include "filler.h"

PixelFiller::PixelFiller(Memory & _mem, IO & _io, TV & _tv): 
    memory(_mem), io(_io), tv(_tv)
{
    this->mode512 = false;
    this->mem32 = (uint32_t *) this->memory.buffer();
    this->pixel32 = 0;  // 4 bytes of bit planes
    this->border_index = 0;
    this->raster_pixel = 0;

    this->reset();

    this->io.onborderchange = [this](int border) {
        //printf("onborderchange: %x\n", border);
        this->border_index = border;
    };

    this->io.onmodechange = [this](bool mode) {
        this->mode512 = mode;
    };
}

void PixelFiller::init()
{
    this->first_visible_line = 312 - Options.screen_height;
    this->center_offset = Options.center_offset;
    this->screen_width = Options.screen_width;
}

void PixelFiller::reset()
{
    // It is tempting to reset the pixel count but the beam is reset in 
    // advanceLine(), don't do that here.
    //this->raster_pixel = 0;   // horizontal pixel counter

    this->raster_line = 0;    // raster line counter
    this->fb_column = 0;      // frame buffer column
    this->fb_row = 0;         // frame buffer row
    this->vborder = true;     // vertical border flag
    this->visible = false;    // visible area flag
    this->bmpofs = 0;         // bitmap offset for current pixel
    this->brk = false;
    this->irq = false;
    this->pixels = this->tv.pixels();
}

#if USE_BIT_PERMUTE
static uint32_t bit_permute_step(uint32_t x, uint32_t m, uint32_t shift) {
    uint32_t t;
    t = ((x >> shift) ^ x) & m;
    x = (x ^ t) ^ (t << shift);
    return x;
}
#endif

void PixelFiller::fetchPixels() 
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

int PixelFiller::shiftOutPixels()
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

int PixelFiller::getColorIndex(int rpixel, bool border) {
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

#define TESTTABLE 0

int PixelFiller::fill(int clocks, int commit_time, 
        int commit_time_pal, bool updateScreen) 
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
            return 0;
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

int PixelFiller::fill1(int clocks, int commit_time, int commit_time_pal, bool updateScreen) {
    uint32_t * bmp = this->pixels;
    int clk;
    int afterbrk = 0;
    int index = 0;

    for (clk = 0; clk < clocks; clk += 2, afterbrk += this->brk ? 2 : 0) {
        // offset for matching border/palette writes and the raster -- test:bord2
        const int rpixel = this->raster_pixel - 24;
        bool border = this->vborder || 
            /* hborder */ (rpixel < (768-512)/2) || (rpixel >= (768 - (768-512)/2));

        index = this->getColorIndex(rpixel, border);
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
        // irq time -- test:bord2, vst (MovR=1d37, MovM=1d36)
        else if (this->raster_line == 0 && this->raster_pixel == 174) {
            this->irq = true;
            this->irq_clk = clk;
        }
    } 

    if (clk == commit_time) {
        this->io.commit(); // regular i/o writes (border index); test: bord2
    }
    if (clk == commit_time_pal) {
        this->io.commit_palette(index); // palette writes; test: bord2
    }

    if (afterbrk) {
        afterbrk -= 2;
    }
    return afterbrk;
}

void PixelFiller::advanceLine(bool updateScreen) {
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

/* simple fill, no out instructions underway, mode 256 */
int PixelFiller::fill2(int clocks)
{
    uint32_t * const bmp = this->pixels;
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
    return 0;
}

/* simple fill, no out instructions underway, mode 512 */
int PixelFiller::fill3(int clocks)
{
    uint32_t * const bmp = this->pixels;
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
    return 0;
}

int PixelFiller::fill4(int clocks)
{
    uint32_t * const bmp = this->pixels;
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
    return 0;
}


