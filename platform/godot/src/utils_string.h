#pragma once
#ifndef STRING_UTILS_H
#define STRING_UTILS_H
#include <string>
#include <vector>

namespace utils
{
	auto split(const std::string& _s, const char _delim)
		->std::vector<std::string>;

	const std::wstring str_to_strW(const std::string& _s);
	const std::string strW_to_str(const std::wstring& _s);
}
#endif // !STRING_UTILS_H
