#pragma once

#include <string>
#include <vector>

struct _options
{
    std::string bootromfile;
    std::string romfile;
    int rom_org;
    std::string wavfile;
    std::vector<std::string> eddfile;
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

    bool autostart;
    bool window;        /* run in a window */
    bool nofilter;      /* bypass audio filter */
    int blendmode;      /* 0: no blend, 1: mix in doubled frames */

    bool opengl;
    struct _opengl_opts {
        /* e.g. myshader for myshader.vsh/fsh for use shader */
        std::string shader_basename; 
        bool use_shader;
        bool default_shader;
        bool filtering;
    } gl;

    struct _volume {
        float timer;
        float beeper;
        float ay;
        float covox;
        float global;
    } volume;

    struct _log {
        bool fdc;
        bool audio;
        bool video;
    } log;

    bool profile;       /* enable gperftools CPU profiler */

    bool vsync_enable;  /* true if window has mouse focus */

    std::vector<std::string> scriptfiles;
    std::vector<std::string> scriptargs;

    std::string path_for_frame(int n);
    std::vector<std::string> fddfile;
    std::vector<int> save_frames;
    std::string audio_rec_path;

    void load(const std::string & filename);
    void save(const std::string & filename);
    std::string get_config_path(void);
    void parse_log(const std::string & opt);
};

extern _options Options;

void options(int argc, char ** argv);
