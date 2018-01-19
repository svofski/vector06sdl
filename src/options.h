#pragma once

#include <string>
#include <vector>

struct _options
{
    std::string romfile;
    int rom_org;
    std::string wavfile;
    int max_frame;
    bool vsync;
    int screen_width;
    int screen_height;
    int border_width;
    int center_offset;

    bool novideo;
    bool nosound;
    bool nofdc;
    bool bootpalette;
    bool log_fdd;
    bool autostart;
    bool window;        /* run in a window */
    bool nofilter;      /* bypass audio filter */

    std::string path_for_frame(int n);
    std::vector<std::string> fddfile;
    std::vector<int> save_frames;
};

extern _options Options;

void options(int argc, char ** argv);
