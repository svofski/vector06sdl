#pragma once

#include <string>
#include <vector>

struct _options
{
    std::string romfile;
    int rom_org;
    std::string wavfile;
    std::vector<std::string> fddfile;
    int max_frame;
    std::vector<int> save_frames;
    bool novideo;
    bool nosound;
    bool nofdc;
    bool bootpalette;
    bool log_fdd;
    bool autostart;

    std::string path_for_frame(int n);
};

extern _options Options;

void options(int argc, char ** argv);
