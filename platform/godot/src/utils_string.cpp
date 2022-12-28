// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "utils_string.h"
#include <sstream>

auto utils::split(const std::string& _s, const char _delim)
->std::vector<std::string>
{
	std::vector<std::string> result;
	std::stringstream ss(_s);
	std::string item;

	while (std::getline(ss, item, _delim)) {
		result.push_back(item);
	}

	return result;
}

auto utils::str_to_strW(const std::string& s)
-> const std::wstring
{
	std::wstringstream cls;
	cls << s.c_str();
	return cls.str();
}

auto utils::strW_to_str(const std::wstring& ws)
-> const std::string
{
	const std::string s(ws.begin(), ws.end());
	return s;
}