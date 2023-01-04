#pragma once

#include "globaldefs.h"

#include "memory.h"
#include "vio.h"
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
#if USE_BIT_PERMUTE
    uint32_t pixel32_grouped;
#endif
    uint32_t * mem32;
    uint32_t * pixels;

    Memory & memory;
    IO & io;
    TV & tv;

    int fill1_count, fill2_count;

public:
    bool brk;
    bool irq;
    int irq_clk;
    auto get_raster_pixel() const -> const int;
    auto get_raster_line() const -> const int;

public:
    PixelFiller(Memory & _mem, IO & _io, TV & _tv);
    void init();
    void reset();
    void fetchPixels();
    int shiftOutPixels();
    int getColorIndex(int rpixel, bool border);

    int fill(int clocks, int commit_time, int commit_time_pal, bool updateScreen);
    int fill1(int clocks, int commit_time, int commit_time_pal, bool updateScreen);
    int fill2(int clocks);
    int fill3(int clocks);
    int fill4(int clocks);
    void advanceLine(bool updateScreen);
};
