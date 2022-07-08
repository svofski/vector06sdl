// this is only useful in the full SDL verion
// MSC is only supported for godot so no need to read files
#if !defined(_MSC_VER)

#include <stdio.h>
#include <time.h>
#include <iostream>
#include <string>
#include <streambuf>
#include <fstream>
#include <vector>
#include <tuple>
#include "util.h"

#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <cstdio>

namespace util {

std::string read_file(const std::string & filename)
{
    std::string text;
    try {
        std::ifstream vsh(filename,  std::ifstream::in);
        vsh.seekg(0, std::ios::end);
        text.reserve(vsh.tellg());
        vsh.seekg(0, std::ios::beg);
        text.assign((std::istreambuf_iterator<char>(vsh)),
                std::istreambuf_iterator<char>());
    } 
    catch (...){
        printf("Failed to load %s\n", filename.c_str());
    }
    return text;
}

size_t islength(std::ifstream & is)
{
    is.seekg(0, is.end);
    size_t result = is.tellg();
    is.seekg(0, is.beg);
    return result;
}

size_t filesize(const std::string & filename)
{
    try {
        std::ifstream is(filename, std::ifstream::in);
        is.seekg(0, is.end);
        return is.tellg();
    }
    catch (...) {
    }
    return 0;
}

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

std::vector<uint8_t> load_binfile(const std::string path_)
{
    const ssize_t chunk_sz = 65536;

    std::vector<uint8_t> bin;

    std::string path = trim_copy(path_);
    if (path.size() == 0) {
        return bin;
    }

    try {
        // unfortunately, ifstream cannot open /dev/fd files, so
        // we're using posix api here
        // also no seeking in the stream, read in chunks
        int fd = open(path.c_str(), O_RDONLY | O_BINARY);
        if (fd < 0) {
            throw std::runtime_error("cannot open file");
        }
        ssize_t bytesread = 0;
        for (;;) {
            bin.resize(bytesread + chunk_sz);
            ssize_t n = read(fd, (char *)bin.data() + bytesread, chunk_sz);
            if (n < 0) {
                throw std::runtime_error("read error");
            }
            bytesread += n;
            if (n < chunk_sz) {
                break;
            }
        }
        bin.resize(bytesread);

        close(fd);
    }
    catch (...) {
        printf("Failed to load file: %s\n", path.c_str());
        bin.resize(0);
    }
    return bin;
}

std::tuple<std::string,std::string,std::string> 
split_path(const std::string & path)
{
    std::string dir, stem, ext;
    std::string name;

    name = path;
    for (int i = path.size() - 1; i >= 0; --i) {
        if (path[i] == '/' || path[i] == '\\') {
            name = path.substr(i + 1);
            dir = path.substr(0, i + 1);
            break;
        }
    }

    size_t dot = name.rfind('.');
    if (dot != std::string::npos) {
        ext = name.substr(dot);
        name = name.substr(0, dot);
    }

    return {dir, name, ext};
}

char printable_char(int c)
{
    return (c < 32 || c > 128) ? '.' : c;
}

static std::string rndchars(int len)
{
    std::stringstream ss;
    for (int i = 0; i < len; ++i) {
        ss.put('a' + (rand() % 26));
    }
    return ss.str();
}

std::string tmpname(const std::string & basename)
{
    for (int i = 0; i < 256; ++i) {
        std::string name(basename + "$" + rndchars(6));
        FILE * f = std::fopen(name.c_str(), "r");
        if (f == nullptr) {
            std::fclose(f);
            return name;
        }
    }
    return basename + "$$$";
}

void str_toupper(std::string & s)
{
    for (auto & c : s) {
        c = std::toupper(c);
    }
}

int careful_rename(std::string const& from, std::string const& to)
{
    int res = rename(from.c_str(), to.c_str()); 
    if (res != 0) {
        // windows can't rename if file exists, no atomic replace
        //printf("%s: cannot rename %s to %s (code %d)\n",
        //        __FUNCTION__, from.c_str(), to.c_str(), errno);

        std::string bak = util::tmpname(to);
        do {
            res = rename(to.c_str(), bak.c_str());
            if (0 != res) {
                //printf("%s: cannot rename %s to %s, giving up\n",
                //        __PRETTY_FUNCTION__, to.c_str(), bak.c_str());
                break;
            }
            res = rename(from.c_str(), to.c_str());
            if (0 != res) {
                //printf("%s: failed to rename %s to %s by all means\n",
                //        __PRETTY_FUNCTION__, from.c_str(), to.c_str());
            }
        } while  (0);

        unlink(bak.c_str());
    }

    return res;
}


}
#endif
