#pragma once

#include <functional>

class scriptnik_engine;

class Scriptnik {
    friend class scriptnik_engine;
private:
    scriptnik_engine * engine;

    std::string text;
    std::vector<std::string> args;

public:
    std::function<int(const std::string &)> loadwav;
    std::function<int(const std::string &)> scancode_from_name;
    std::function<void(int)> keydown;
    std::function<void(int)> keyup;

    std::function<void(int,int,int)> insert_breakpoint;
    std::function<void()> debugger_attached;
    std::function<void()> debugger_detached;
    std::function<void()> debugger_break;
    std::function<void()> debugger_continue;
    std::function<int(const std::string&)> read_register;
    std::function<void(const std::string&, int)> set_register;
    std::function<int(int,int)> read_memory;
    std::function<void(int,int,int)> write_memory;

public:
    Scriptnik();
    ~Scriptnik();

    int append_from_file(const std::string & filename);
    int append_arg(const std::string &);
    void start();

    void onframe(int frame);
    void onwavfinished(int dummy);
    void onbreakpoint();
};


