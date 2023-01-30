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

	typedef enum {
		STR2INT_SUCCESS = 0,
		STR2INT_OVERFLOW,
		STR2INT_UNDERFLOW,
		STR2INT_INCONVERTIBLE
	} str2int_errno;

	str2int_errno str2int(int *out, char *s, int base);
}
#endif // !STRING_UTILS_H
