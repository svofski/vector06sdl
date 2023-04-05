#pragma once

#include <functional>

class scriptnik_engine;

class Scriptnik {
    friend class scriptnik_engine;
private:
    scriptnik_engine * engine;

    std::string text;
    std::vector<std::string> args;

    bool api_registered;
    bool finalizing;

    bool file_dialog_requested;
    std::string file_dialog_path;
    std::string file_dialog_mode;

public:
    std::function<int(const std::string &)> loadwav;
    std::function<int(const std::string &)> scancode_from_name;
    std::function<void(int)> keydown;
    std::function<void(int)> keyup;

    std::function<void(int,int,int)> insert_breakpoint;
    std::function<void(int,int,int)> remove_breakpoint;
    std::function<void()> script_attached;
    std::function<void()> script_detached;
    std::function<void()> script_break;
    std::function<void()> script_continue;
    std::function<int(const std::string&)> read_register;
    std::function<void(const std::string&, int)> set_register;
    std::function<int(int,int)> read_memory;
    std::function<void(int,int,int)> write_memory;
    std::function<void()> finalize;
    std::function<void(const std::string&)> console_puts;

    std::function<void(const std::string&)> start_wav_recording;
    std::function<void()> stop_wav_recording;

public:
    Scriptnik();
    ~Scriptnik();

    int append_from_file(const std::string & filename);
    int set_string(const std::string & text);
    size_t length() const { return this->text.length(); }
    const std::string & script_text() const { return this->text; }
    void clear_args();
    int append_arg(const std::string &);
    void start();

    bool is_finalizing() const { return finalizing; }
    bool is_file_dialog_requested(std::string & path, std::string & mode);
    void on_file_dialog_result(const std::string & path);

    void onframe(int frame);
    void onwavfinished(int dummy);
    void onbreakpoint();
};


