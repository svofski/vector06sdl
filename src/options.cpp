#include <iostream>
#include <string>
#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/predicate.hpp"
#endif
#include "globaldefs.h"
#include "options.h"

_options Options = 
{
    .bootromfile = "",
    .romfile = "",
    .rom_org = 256,
    .pc = 0,
    .wavfile = "",
    .eddfile = {},
    .max_frame = -1,
    .vsync = false,
#if defined(__ANDROID__) || defined(__GODOT__)
    .novideo = true,
#else
    .novideo = false,
#endif
    .screen_width = DEFAULT_SCREEN_WIDTH,
    .screen_height = DEFAULT_SCREEN_HEIGHT,
    .border_width = DEFAULT_BORDER_WIDTH,
    .center_offset = DEFAULT_CENTER_OFFSET,

    // timer, beeper, ay, covox, global
    .volume = {0.1f, 0.1f, 0.1f, 0.1f, 1.5f},
    .enable = {/* timer ch0..2 */ true, true, true,
               /* ay ch0..2 */    true, true, true},
    .nofilter = false,
};

#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)

void options(int argc, char ** argv)
{
    Options.gl.use_shader = true;
    Options.gl.default_shader = true;
    Options.gl.filtering = true;

    try {
        std::string conf = Options.get_config_path();
        Options.load(conf);
        printf("Loaded options from %s\n", conf.c_str());
    }
    catch(...) {}

    std::vector<std::string> nothing;

    namespace po = boost::program_options;
    po::options_description descr("Parameters");
    descr.add_options()
        ("help,h", "call for help")
        ("bootrom", po::value<std::string>(), "alternative bootrom")
        ("rom", po::value<std::string>(), "rom file to load")
        ("org", po::value<int>(), "rom origin address (default 0x100)")
        ("pc", po::value<std::string>(), "initial pc (default 0)")
        ("wav", po::value<std::string>(), "wav file to load (not implemented)")
        ("fdd", po::value<std::vector<std::string>>(), "fdd floppy image (multiple up to 4)")
        ("edd", po::value<std::vector<std::string>>(), "edd ramdisk image (multiple up to 16)")
        ("log", po::value<std::string>(), "fdc,audio,video print debug info from systems")
        ("autostart", "autostart based on RUS/LAT blinkage")
        ("max-frame", po::value<int>(), "run emulation for this many frames then exit")
        ("save-frame", po::value<std::vector<int>>(), "save frame with these numbers (multiple)")
        ("novideo", "do not output video")
        ("opengl", po::value<std::vector<std::string>>()->
             required()->implicit_value(nothing, ""), 
         "use OpenGL for rendering, can take parameters:\n"
         "disable - disable OpenGL (useful to override config setting)\n"
         "shader:filename - user shaders in filename.{vsh,fsh}\n"
         "shader:default - use default builtin shader\n"
         "shader:none - disable shaders\n"
         "filtering:no - disable texture filtering (use nearest)\n")
        ("nosound", "stay silent")
        ("volume-timer", po::value<float>()->
            default_value(Options.volume.timer), "timer sound volume")
        ("volume-beeper", po::value<float>()->
            default_value(Options.volume.beeper), "beeper/tape sound volume")
        ("volume-ay", po::value<float>()->
            default_value(Options.volume.ay), "AY volume")
        ("volume-covox", po::value<float>()->
            default_value(Options.volume.covox), "Covox volume")
        ("volume", po::value<float>()->
            default_value(Options.volume.global), "Global volume")
        ("nofdc", "detach floppy disk controller")
        ("window", "run in a window, not fullscreen")
        ("bootpalette", "init palette to yellow/blue colours before running a rom")
        ("vsync", "(default) use display vsync")
        ("novsync", "try to make do without vsync")
        ("nofilter", "bypass audio filters")
        ("blendmode", po::value<int>(), "blend mode: 0=none, 1=blend fields, 2=equalize fps conversion")
        ("yres", po::value<int>(), "number of visible lines (default 288)")
        ("border-width", po::value<int>(), "width of horizontal border (default 32)")
        ("record-audio", po::value<std::string>(), "record audio output to file")
        ("saveconfig", "save config to ~/.v06x.conf")
#if HAVE_GPERFTOOLS
        ("profile", "collect CPU profile data in v06x.prof\n"
            "examine data using pprof -http 0.0.0.0:5555 ./v06x v06x.prof")
#endif
        ("script", po::value<std::vector<std::string>>(), "chai script file (multiple)")
        ("scriptargs", po::value<std::vector<std::string>>(), "chai script arguments (multiple)")
        ;
        
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, descr), vm);

        if (vm.count("help")) {
            std::cout << descr << std::endl;
            exit(0);
        }

        if (vm.count("bootrom")) {
            Options.bootromfile = vm["bootrom"].as<std::string>();
            printf("Specified bootrom file: %s\n", Options.bootromfile.c_str());
        }

        if (vm.count("rom")) {
            Options.romfile = vm["rom"].as<std::string>();
            printf("Specified ROM file: %s\n", Options.romfile.c_str());
        }

        if (vm.count("org")) {
            Options.rom_org = vm["org"].as<int>();
            printf("ROM will be loaded at origin: 0x%04x\n", Options.rom_org);
        }

        if (vm.count("pc")) {
            std::stringstream bs;
            bs << std::hex << vm["pc"].as<std::string>();
            bs >> Options.pc;
            
            printf("Will jump to PC=0x%04x\n", Options.pc);
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

        if (vm.count("blendmode")) {
            Options.blendmode = vm["blendmode"].as<int>();
            printf("Using blend mode %d\n", Options.blendmode);
        }

        if (vm.count("fdd")) {
            Options.fddfile = vm["fdd"].as<std::vector<std::string>>();
            if (Options.fddfile.size() > 4) {
                printf("Warning: only 4 fdd images can be specified at once. "
                        "Truncated.\n");
                Options.fddfile.resize(4);
            }
        }

        if (vm.count("edd")) {
            Options.eddfile = vm["edd"].as<std::vector<std::string>>();
            if (Options.eddfile.size() > 1) {
                printf("Warning: only 1 kvaz is currently supported."
                        " Stay tuned for more.\n");
                Options.eddfile.resize(1);
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

        Options.profile = vm.count("profile") > 0;

        if (vm.count("opengl") > 0) {
            Options.opengl = vm.count("opengl") > 0;

            static const std::string p_shader("shader:");
            static const std::string p_filtering("filtering:");

            auto glopts = vm["opengl"].as<std::vector<std::string>>();
            for (auto o : glopts) {
                if (boost::algorithm::starts_with(o, p_shader)) {
                    auto rem = o.substr(p_shader.length());
                    if (rem == "none") {
                        Options.gl.use_shader = false;
                        Options.gl.default_shader = false;
                        continue;
                    }
                    if (rem == "default") { 
                        Options.gl.default_shader = true;
                        continue;
                    }
                    Options.gl.shader_basename = rem;
                    Options.gl.default_shader = false;
                    printf("Will use shaders: %s.vsh and %s.fsh\n",
                            Options.gl.shader_basename.c_str(),
                            Options.gl.shader_basename.c_str());
                }
                else if (boost::algorithm::starts_with(o, p_filtering)) {
                    Options.gl.filtering = 
                        o.substr(p_filtering.length()) == "yes";
                }
                else if (o == "disable") {
                    Options.opengl = false;
                }
            }
        }

        if (vm.count("volume-timer") > 0) {
            Options.volume.timer = vm["volume-timer"].as<float>();
            printf("volume.timer: %f\n", Options.volume.timer);
        }
        if (vm.count("volume-beeper") > 0) {
            Options.volume.beeper = vm["volume-beeper"].as<float>();
        }
        if (vm.count("volume-ay") > 0) {
            Options.volume.ay = vm["volume-ay"].as<float>();
        }
        if (vm.count("volume-covox") > 0) {
            Options.volume.covox = vm["volume-covox"].as<float>();
        }
        if (vm.count("volume") > 0) {
            Options.volume.global = vm["volume"].as<float>();
            printf("volume.global: %f\n", Options.volume.global);
        }

        if (vm.count("script")) {
            Options.scriptfiles = vm["script"].as<std::vector<std::string>>();
            printf("Script files: ");
            for (auto & scriptfile : Options.scriptfiles) {
                printf("%s ", scriptfile.c_str());
            }
            printf("\n");
        }

        if (vm.count("scriptargs")) {
            Options.scriptargs = vm["scriptargs"].as<std::vector<std::string>>();
            printf("Script args: ");
            for (auto & scriptarg : Options.scriptargs) {
                printf("%s ", scriptarg.c_str());
            }
            printf("\n");
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
    for (size_t i = 0; i < cats.size(); ++i) {
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
        else if (cats[i] == "video") {
            log.video = true;
        }
        else if (cats[i] == "~video") {
            log.video = false;
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

    bootromfile = pt.get<std::string>("bootromfile", "");
    window = pt.get<bool>("video.window");
    vsync = pt.get<bool>("video.vsync");
    screen_height = pt.get<int>("video.yres");
    border_width = pt.get<int>("video.border_width");
    blendmode = pt.get<int>("video.blendmode");
    nofilter = pt.get<bool>("audio.bypass_filter");
    nofdc = pt.get<bool>("peripheral.nofdc");
    log.fdc = pt.get<bool>("log.fdc");
    log.audio = pt.get<bool>("log.audio");

    opengl = pt.get<bool>("video.opengl");
    gl.shader_basename = pt.get<std::string>("video.opengl_shader_basename");
    gl.use_shader = pt.get<bool>("video.opengl_use_shader");
    gl.default_shader = pt.get<bool>("video.opengl_default_shader");
    gl.filtering = pt.get<bool>("video.opengl_filtering");


    volume.timer = pt.get<float>("audio.volume.timer");
    volume.beeper = pt.get<float>("audio.volume.beeper");
    volume.ay = pt.get<float>("audio.volume.ay");
    volume.covox = pt.get<float>("audio.volume.covox");
    volume.global = pt.get<float>("audio.volume");
}

void _options::save(const std::string & filename)
{
    using boost::property_tree::ptree;
    ptree pt;

    pt.put("bootromfile", bootromfile);
    pt.put("video.window", window);
    pt.put("video.vsync", vsync);
    pt.put("video.yres", screen_height);
    pt.put("video.border_width", border_width);
    pt.put("video.blendmode", blendmode);
    pt.put("audio.bypass_filter", nofilter);
    pt.put("peripheral.nofdc", nofdc);
    pt.put("log.fdc", log.fdc);
    pt.put("log.audio", log.audio);

    pt.put("video.opengl", opengl);
    if (gl.shader_basename.length() > 0) {
        pt.put("video.opengl_shader_basename", gl.shader_basename);
    }
    pt.put("video.opengl_use_shader", gl.use_shader);
    pt.put("video.opengl_default_shader", gl.default_shader);
    pt.put("video.opengl_filtering", gl.filtering);

    pt.put("audio.volume.timer", volume.timer);
    pt.put("audio.volume.beeper", volume.beeper);
    pt.put("audio.volume.ay", volume.ay);
    pt.put("audio.volume.covox", volume.covox);
    pt.put("audio.volume", volume.global);

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

#else
std::string _options::path_for_frame(int n)
{
    return "frame" + std::to_string(n);
}
#endif

