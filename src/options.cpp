#include <iostream>
#include <string>
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "globaldefs.h"
#include "options.h"

_options Options = 
{
    .romfile = "",
    .rom_org = 256,
    .wavfile = "",
    .max_frame = -1,
    .vsync = true,
    .screen_width = DEFAULT_SCREEN_WIDTH,
    .screen_height = DEFAULT_SCREEN_HEIGHT,
    .border_width = DEFAULT_BORDER_WIDTH,
    .center_offset = DEFAULT_CENTER_OFFSET,
};

void options(int argc, char ** argv)
{
    try {
        std::string conf = Options.get_config_path();
        Options.load(conf);
        printf("Loaded options from %s\n", conf.c_str());
    }
    catch(...) {}

    namespace po = boost::program_options;
    po::options_description descr("Parameters");
    descr.add_options()
        ("help,h", "call for help")
        ("rom", po::value<std::string>(), "rom file to load")
        ("org", po::value <int>(), "rom origin address (default 0x100)")
        ("wav", po::value<std::string>(), "wav file to load (not implemented)")
        ("fdd", po::value<std::vector<std::string>>(), "fdd floppy image (multiple up to 4)")
        ("log", po::value<std::string>(), "fdd,audio print debug info from systems")
        ("autostart", "autostart based on RUS/LAT blinkage")
        ("max-frame", po::value<int>(), "run emulation for this many frames then exit")
        ("save-frame", po::value<std::vector<int>>(), "save frame with these numbers (multiple)")
        ("novideo", "do not output video")
        ("nosound", "stay silent")
        ("nofdc", "detach floppy disk controller")
        ("window", "run in a window, not fullscreen")
        ("bootpalette", "init palette to yellow/blue colours before running a rom")
        ("vsync", "(default) use display vsync")
        ("novsync", "try to make do without vsync")
        ("nofilter", "bypass audio filters")
        ("yres", po::value<int>(), "number of visible lines (default 288)")
        ("border-width", po::value<int>(), "width of horizontal border (default 32)")
        ("record-audio", po::value<std::string>(), "record audio output to file")
        ("saveconfig", "save config to ~/.v06x.conf")
        ;
        
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

        if (vm.count("org")) {
            Options.rom_org = vm["org"].as<int>();
            printf("ROM will be loaded at origin: 0x%04x\n", Options.rom_org);
        }

        if (vm.count("wav")) {
            Options.wavfile = vm["wav"].as<std::string>();
            printf("Specified WAV file: %s\n", Options.wavfile.c_str());
        }

        if (vm.count("max-frame")) {
            Options.max_frame = vm["max-frame"].as<int>();
            printf("Will exit after frame #%d\n", Options.max_frame);
        }

        if (vm.count("save-frame")) {
            Options.save_frames = vm["save-frame"].as<std::vector<int>>();
            printf("Will save frames #");
            for (unsigned i = 0; i < Options.save_frames.size(); ++i) {
                printf("%d%c", Options.save_frames[i], 
                        (i+1) == Options.save_frames.size() ? '\n' : ',');
            }
        }

        if (vm.count("fdd")) {
            Options.fddfile = vm["fdd"].as<std::vector<std::string>>();
            if (Options.fddfile.size() > 4) {
                printf("Warning: only 4 fdd images can be specified at once. "
                        "Truncated.\n");
                Options.fddfile.resize(4);
            }
        }

        Options.nofdc = vm.count("nofdc") > 0;

        if (vm.count("novideo")) {
            Options.novideo = true;
            printf("Will not display video\n");
        }

        if (vm.count("nosound")) {
            Options.nosound = true;
            printf("Will not make a sound\n");
        }

        if (vm.count("bootpalette")) {
            Options.bootpalette = true;
            printf("The palette will be initialized to yellow/blue\n");
        }

        Options.autostart = vm.count("autostart") > 0;
        if (Options.autostart) {
            printf("Will autostart on RUS/LAT blinkage\n");
        }

        Options.window = Options.window || vm.count("window") > 0;
        if (Options.window) {
            printf("Will run in a window\n");
        }

        if (vm.count("vsync")) {
            Options.vsync = true;
        }
        if (vm.count("novsync")) {
            Options.vsync = false;
        }
        printf("Will%suse VSYNC\n", Options.vsync ? " " : " not ");

        if (vm.count("yres") > 0) {
            Options.screen_height = vm["yres"].as<int>();
            printf("Vertical resolution set to %d\n", Options.screen_height);
        }

        if (vm.count("border-width") > 0) {
            Options.border_width = vm["border-width"].as<int>();
            if (Options.border_width < 0) Options.border_width = 0;
            if (Options.border_width > 104) Options.border_width = 104;
            Options.screen_width = 512 + 2 * Options.border_width;
            Options.center_offset = 152 - Options.border_width;
            printf("Border width = %d\n", Options.border_width);
        }

        if (vm.count("nofilter")) {
            Options.nofilter = true;
        }

        if (vm.count("log")) {
            Options.parse_log(vm["log"].as<std::string>());
        }

        if (vm.count("record-audio")) {
            Options.audio_rec_path = vm["record-audio"].as<std::string>();
        }

        if (vm.count("saveconfig")) {
            std::string conf = Options.get_config_path();
            try {
                Options.save(conf);
                printf("Persistable options saved to %s\n", conf.c_str());
            } 
            catch(...) {
                printf("There was an error saving config to %s\n", conf.c_str());
            }
        }
    }
    catch(po::error & err) {
        std::cerr << err.what() << std::endl;
        std::cerr << descr << std::endl;
        exit(1);
    }
}

void _options::parse_log(const std::string & opt)
{
    std::vector<std::string> cats;
    boost::split(cats, opt, boost::is_any_of(","), boost::token_compress_on);
    for (int i = 0; i < cats.size(); ++i) {
        if (cats[i] == "fdc") {
            log.fdc = true;
        } 
        else if (cats[i] == "~fdc") {
            log.fdc = false;
        }
        else if (cats[i] == "audio") {
            log.audio = true;
        }
        else if (cats[i] == "~audio") {
            log.audio = false;
        }
        else {
            printf("Ignored unknown log category: %s\n", cats[i].c_str());
            continue;
        }
    }
}

std::string _options::path_for_frame(int n)
{
    using namespace boost::filesystem;
    std::string & fullname = this->romfile;
    if (fullname.length() == 0) {
        fullname = this->wavfile;
        if (fullname.length() == 0) {
            if (this->fddfile.size() > 0) {
                fullname = this->fddfile[0];
            }
            if (fullname.length() == 0) {
                fullname = std::string("boots.bin");
            }
        }
    }

    path rompath(fullname);
    //printf("rompath: %s\n", rompath.string().c_str());
    std::string result = std::string("out/") + rompath.filename().stem().string() + 
        std::string("_") + std::to_string(n) + std::string(".png");

    return result;
}

void _options::load(const std::string & filename)
{
    using boost::property_tree::ptree;
    ptree pt;

    read_json(filename, pt);

    window = pt.get<bool>("video.window");
    vsync = pt.get<bool>("video.vsync");
    screen_height = pt.get<int>("video.yres");
    border_width = pt.get<int>("video.border_width");
    nofilter = pt.get<bool>("audio.bypass_filter");
    nofdc = pt.get<bool>("peripheral.nofdc");
    log.fdc = pt.get<bool>("log.fdc");
    log.audio = pt.get<bool>("log.audio");
}

void _options::save(const std::string & filename)
{
    using boost::property_tree::ptree;
    ptree pt;

    pt.put("video.window", window);
    pt.put("video.vsync", vsync);
    pt.put("video.yres", screen_height);
    pt.put("video.border_width", border_width);
    pt.put("audio.bypass_filter", nofilter);
    pt.put("peripheral.nofdc", nofdc);
    pt.put("log.fdc", log.fdc);
    pt.put("log.audio", log.audio);
    write_json(filename, pt);
}

std::string _options::get_config_path(void)
{
    std::string config("v06x.conf");
    char const * home = getenv("HOME");
    if (home || (home = getenv("USERPROFILE"))) {
        config = std::string(home) + "/." + config;
    } 
    else {
        char const * hdrive = getenv("HOMEDRIVE");
        char const * hpath = getenv("HOMEPATH");
        if (hdrive && hpath) {
            config = std::string(hdrive) + hpath + "/." + config;
        } 
    }
    return config;
}
