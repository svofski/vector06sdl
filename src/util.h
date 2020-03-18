#pragma once

#include <string>
#include <fstream>
#include <vector>

namespace util {

std::string read_file(const std::string & filename);
size_t islength(std::ifstream & is);
std::vector<uint8_t> load_binfile(const std::string path);

}
