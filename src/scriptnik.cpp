#include <stdio.h>
#include <map>
#include <functional>
#include <iostream>
#include <string>
#include <streambuf>
#include <fstream>
#include <any>


#include <chaiscript/chaiscript.hpp>
#include <chaiscript/dispatchkit/function_call.hpp>

#include "scriptnik.h"
#include "util.h"

#if !defined(__GODOT__)
#include "SDL_keyboard.h"
#endif

#if defined(__GODOT__)
#include "godot_scancodes.h"
#endif

std::vector<int> read_intvector(const std::string & path)
{
    std::vector<int> dst;
    auto bytes = util::load_binfile(path);
    dst.resize(bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i) dst[i] = bytes[i];
    return dst;
}

// -- a couple of string utilities from ChaiScript extras
/**
 * Convert the given string to lowercase letters.
 */
std::string toLowerCase(const std::string& subject)
{
    std::string result(subject);
    std::transform(result.begin(), result.end(), result.begin(),
      [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**
 * Convert the given string to uppercase letters.
 */
std::string toUpperCase(const std::string& subject)
{
    std::string result(subject);
    std::transform(result.begin(), result.end(), result.begin(),
      [](unsigned char c) { return std::toupper(c); });
    return result;
}

struct scriptnik_engine {
    chaiscript::ChaiScript chai;
    Scriptnik & s;

    typedef std::function<void(int)> callback;
    std::map<std::string, callback> callbacks;

    typedef std::function<void(std::string)> callback_string;
    std::map<std::string, callback_string> callbacks_string;

    int add_callback(const std::string &t_name, const callback & t_func)
    {
        callbacks[t_name] = t_func;
        return 0;
    }

    int add_callback_string(const std::string &t_name, const callback_string & t_func)
    {
        callbacks_string[t_name] = t_func;
        return 0;
    }

    scriptnik_engine(Scriptnik & scr) : s(scr) {
    }

    void register_api() { 
        using namespace chaiscript;
        using namespace std;
        chai.add(fun([this](const string & msg) {
                    s.console_puts(msg);
                    }), "puts");
        chai.add(fun([this](const string & name) {
                    return s.loadwav(name);
                    }),
                "loadwav");
        chai.add(fun([this](int scancode) { s.keydown(scancode); }), "keydown");
        chai.add(fun([this](int scancode) { s.keyup(scancode); }), "keyup");
        chai.add(fun([this](const string & name) { 
#if !defined(__GODOT__)
                        return (int)SDL_GetScancodeFromName(name.c_str());
#else
                        return GetScancodeFromName(name.c_str());
#endif
                    }), 
                "scancode_from_name");

        chai.add(fun(&scriptnik_engine::add_callback, this), "add_callback");
        chai.add(fun(&scriptnik_engine::add_callback_string, this), "add_callback_string");

        chai.add(bootstrap::standard_library::vector_type<std::vector<std::string>>("VectorString"));
        chai.add(bootstrap::standard_library::vector_type<std::vector<int>>("VectorInt"));

        chai.add_global(chaiscript::var(s.args), "scriptargs"); 

        //
        chai.add(fun([this](int type, int addr, int kind) {
                    s.insert_breakpoint(type, addr, kind);
                    }), "insert_breakpoint");
        chai.add(fun([this](int type, int addr, int kind) {
                    s.remove_breakpoint(type, addr, kind);
                    }), "remove_breakpoint");

        // new names for script hooks
        chai.add(fun([this]() {
                    s.script_attached();
                    }), "script_attached");
        chai.add(fun([this]() {
                    s.script_detached();
                    }), "script_detached");
        chai.add(fun([this]() {
                    s.script_break();
                    }), "script_break");
        chai.add(fun([this]() {
                    s.script_continue();
                    }), "script_continue");

        // keep the old names too for the time being
        chai.add(fun([this]() {
                    s.script_attached();
                    }), "debugger_attached");
        chai.add(fun([this]() {
                    s.script_detached();
                    }), "debugger_detached");
        chai.add(fun([this]() {
                    s.script_break();
                    }), "debugger_break");
        chai.add(fun([this]() {
                    s.script_continue();
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
        chai.add(fun([this]() {
                    s.finalizing = true;
                    }), "finalize");
        //chai.add(type_conversion<std::any, std::string>([](const std::any &from)
        //            { return std::any_cast<std::string>(from); }));
        //chai.add(type_conversion<std::any, std::string>([](const std::any &from)
        //            { return std::any_cast<int>(from); }));
        chai.add(fun(toUpperCase), "toUpperCase");
        chai.add(fun(toLowerCase), "toLowerCase");

        chai.add(fun([this](const std::string & path, const std::string & mode)
                    {
                    s.file_dialog_requested = true;
                    s.file_dialog_path = path;
                    s.file_dialog_mode = mode;
                    }), "open_file_dialog");
    }

    void invoke(const std::string & named, std::any arg) 
    {
        try {
            if (arg.type() == typeid(std::string)) {
                callback_string kolbask = callbacks_string[named];
                if (kolbask != nullptr) {
                    std::string s = std::any_cast<std::string>(arg);
                    kolbask(s);
                }
            }
            else if (arg.type() == typeid(int)) {
                callback kolbask = callbacks[named];
                if (kolbask != nullptr) {
                    int i = std::any_cast<int>(arg);
                    kolbask(i);
                }
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

Scriptnik::Scriptnik() : api_registered(false), finalizing(false)
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

void Scriptnik::on_file_dialog_result(const std::string & path)
{
    printf("Scriptnik::on_file_dialog_result: invoking callback path=%s\n", path.c_str());
    engine->invoke("on_file_dialog_result", path);
}

int Scriptnik::append_from_file(const std::string & filename)
{
    this->text = this->text + util::read_file(filename);
    return this->text.length();
}

int Scriptnik::set_string(const std::string & text)
{
    this->text = text;
    return this->text.length();
}

void Scriptnik::clear_args()
{
    this->args.clear();
}

int Scriptnik::append_arg(const std::string & arg)
{
    this->args.push_back(std::string(arg));
    return this->args.size();
}

void Scriptnik::start()
{
    if (!api_registered) {
        engine->register_api();
        api_registered = true;
    }

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

bool 
Scriptnik::is_file_dialog_requested(std::string & path, std::string & mode)
{
    bool result = this->file_dialog_requested;
    this->file_dialog_requested = false;
    path = this->file_dialog_path;
    mode = this->file_dialog_mode;
    return result;
}
