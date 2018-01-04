#include <iostream>
#include "boost/program_options.hpp"

#include "options.h"

_options Options = 
{
    .romfile = "",
    .rom_org = 256,
    .wavfile = "",
};

void options(int argc, char ** argv)
{
    namespace po = boost::program_options;
    po::options_description descr("Parameters");
    descr.add_options()
        ("help,h", "call for help")
        ("rom", po::value<std::string>(), "rom file to load")
        ("org", po::value <int>(), "rom origin address (default 0x100)")
        ("wav", po::value<std::string>(), "wav file to load");
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, descr), vm);

        if (vm.count("help")) {
            std::cout << descr << std::endl;
            exit(0);
        }

        if (vm.count("rom")) {
            Options.romfile = vm["rom"].as<std::string>();
            printf("Specified ROM file: %s\n", Options.romfile.c_str());
        }
    }
    catch(po::error & err) {
        std::cerr << err.what() << std::endl;
        std::cerr << descr << std::endl;
        exit(1);
    }
}


