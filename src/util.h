#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <algorithm> 
#include <cctype>
#include <locale>

namespace util {

std::string read_file(const std::string & filename);
size_t islength(std::ifstream & is);
size_t filesize(const std::string & filename);
std::vector<uint8_t> load_binfile(const std::string path);

// trim from start (in place)
static inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                         [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string& s)
{
    s.erase(std::find_if(
              s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); })
              .base(),
      s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s)
{
    trim(s);
    return s;
}

void str_toupper(std::string & s);

std::tuple<std::string,std::string,std::string>
split_path(const std::string & path);

char printable_char(int c);

std::string tmpname(const std::string & basename);

int careful_rename(std::string const& from, std::string const& to);

}
