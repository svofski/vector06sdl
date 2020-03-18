#include <stdio.h>
#include <time.h>
#include <iostream>
#include <string>
#include <streambuf>
#include <fstream>
#include <vector>

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

std::vector<uint8_t> load_binfile(const std::string path)
{
    std::vector<uint8_t> bin;
    std::ifstream is(path, std::ifstream::binary);
    if (is) {
        size_t length = islength(is);
        bin.resize(length);
        is.read((char *) bin.data(), length);
    }
    return bin;
}

}
