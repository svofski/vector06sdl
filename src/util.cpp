#include <stdio.h>
#include <time.h>
#include <iostream>
#include <string>
#include <streambuf>
#include <fstream>
#include <vector>
#include "util.h"

#include <unistd.h>
#include <fcntl.h>

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

        printf("File: %s\n", path.c_str());
        for (int i = 0; i < bytesread; ++i) {
            printf("%02x ", bin.at(i));
        }
        printf("\n");

        close(fd);
    }
    catch (...) {
        printf("Failed to load file: %s\n", path.c_str());
        bin.resize(0);
    }
    return bin;
}

}
