#include <stdio.h>
#include <map>
#include <functional>
#include <iostream>
#include <string>
#include <streambuf>
#include <fstream>


#include <chaiscript/chaiscript.hpp>
#include <chaiscript/dispatchkit/function_call.hpp>

#include "scriptnik.h"
#include "util.h"

#include "SDL_keyboard.h"

std::vector<int> read_intvector(const std::string & path)
{
    std::vector<int> dst;
    auto bytes = util::load_binfile(path);
    dst.resize(bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i) dst[i] = bytes[i];
    return dst;
}


struct scriptnik_engine {
    chaiscript::ChaiScript chai;
    Scriptnik & s;

    typedef std::function<void(int)> callback;
    std::map<std::string, callback> callbacks;

    int add_callback(const std::string &t_name, 
            const callback & t_func)
    {
        callbacks[t_name] = t_func;
        return 0;
    }

    scriptnik_engine(Scriptnik & scr) : s(scr) {
    }

    void register_api() { 
        using namespace chaiscript;
        using namespace std;
        chai.add(fun([this](const string & name) {
                    return s.loadwav(name);
                    }),
                "loadwav");
        chai.add(fun([this](int scancode) { s.keydown(scancode); }), "keydown");
        chai.add(fun([this](int scancode) { s.keyup(scancode); }), "keyup");
        chai.add(fun([this](const string & name) { 
                        return (int)SDL_GetScancodeFromName(name.c_str());
                    }), 
                "scancode_from_name");

        chai.add(fun(&add_callback, this), "add_callback");

        chai.add(bootstrap::standard_library::vector_type<std::vector<std::string>>("VectorString"));
        chai.add(bootstrap::standard_library::vector_type<std::vector<int>>("VectorInt"));

        chai.add_global(chaiscript::var(s.args), "scriptargs"); 

        chai.add(fun([this](int type, int addr, int kind) {
                    s.insert_breakpoint(type, addr, kind);
                    }), "insert_breakpoint");
        chai.add(fun([this]() {
                    s.debugger_attached();
                    }), "debugger_attached");
        chai.add(fun([this]() {
                    s.debugger_detached();
                    }), "debugger_detached");
        chai.add(fun([this]() {
                    s.debugger_break();
                    }), "debugger_break");
        chai.add(fun([this]() {
                    s.debugger_continue();
                    }), "debugger_continue");
        chai.add(fun([this](const std::string & name) {
                    return s.read_register(name);
                    }), "read_register");
        chai.add(fun([this](const std::string & name, int value) {
                    return s.set_register(name, value);
                    }), "set_register");
        chai.add(fun([this](int addr, int stackrq) {
                    return (int)s.read_memory(addr, stackrq);
                    }), "read_memory");
        chai.add(fun([this](int addr, int w8, int stackrq) {
                    s.write_memory(addr, w8, stackrq);
                    }), "write_memory");
        chai.add(fun([this](const std::string & filename) {
                    return ::read_intvector(filename);
                    }), "read_file");
    }

    void invoke(const std::string & named, int arg) 
    {
        try {
            auto kolbask = this->callbacks[named];
            if (kolbask != nullptr) {
                kolbask(arg);
            }
        } 
        catch (const chaiscript::exception::eval_error &ee) {
            printf("%s\n", ee.pretty_print().c_str());
        }
        catch (const chaiscript::Boxed_Value &e) {
            printf("Unhandled exception thrown of type %s\n", 
                    e.get_type_info().name().c_str());
        }
        catch (const chaiscript::exception::load_module_error &e) {
            printf("Unhandled module load error %s\n", e.what());
        }
        catch (std::exception &e) {
            printf("Unhandled standard exception: %s\n", e.what());
        }
        catch (...) {
            printf("Unhandled unknown exception");
        }
    }
};

Scriptnik::Scriptnik() 
{
    engine = new scriptnik_engine(*this);
    text = "";
}

Scriptnik::~Scriptnik()
{
    delete engine;
}

void Scriptnik::onframe(int frame)
{
    engine->invoke("frame", frame);
}

void Scriptnik::onwavfinished(int dummy)
{
    engine->invoke("wavloaded", dummy);
}

void Scriptnik::onbreakpoint()
{
    engine->invoke("breakpoint", 0);
}

int Scriptnik::append_from_file(const std::string & filename)
{
    this->text = this->text + util::read_file(filename);
    return this->text.length();
}

int Scriptnik::append_arg(const std::string & arg)
{
    this->args.push_back(std::string(arg));
    return this->args.size();
}

void Scriptnik::start()
{
    engine->register_api();

    try {
        engine->chai.eval(this->text);
    } 
    catch (const chaiscript::exception::eval_error &ee) {
        printf("%s\n", ee.pretty_print().c_str());
    }
    catch (const chaiscript::Boxed_Value &e) {
      printf("Unhandled exception thrown of type %s\n", e.get_type_info().name().c_str());
    }
    catch (const chaiscript::exception::load_module_error &e) {
      printf("Unhandled module load error %s\n", e.what());
    }
    catch (std::exception &e) {
      printf("Unhandled standard exception: %s\n", e.what());
    }
    catch (...) {
      printf("Unhandled unknown exception");
    }
}
