#pragma once

#include <string>
#include <vector>

struct _options
{
    std::string romfile;
    int rom_org;
    std::string wavfile;
    int max_frame;
    bool novideo;
    bool nosound;
    bool nofdc;
    bool bootpalette;
    bool log_fdd;
    bool autostart;
    bool window;        /* run in a window */

    std::string path_for_frame(int n);
    std::vector<std::string> fddfile;
    std::vector<int> save_frames;
};

extern _options Options;

void options(int argc, char ** argv);
