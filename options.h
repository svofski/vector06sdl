#pragma once

struct _options
{
    std::string romfile;
    int rom_org;
    std::string wavfile;
};

extern _options Options;

void options(int argc, char ** argv);
