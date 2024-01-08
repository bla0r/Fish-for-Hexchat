#ifndef __UTILS_H__
#define __UTILS_H__
#include <string>
#include <vector>
const int ERR_BUF_SIZE = 256;

namespace util {
	bool caseInsensitiveCompare(const std::string& str1, const std::string& str2);
	std::vector<std::string> split(const std::string&, const std::string&);
	void remove_bad_chars(std::string&);
	std::string Base64_Encode(const std::string&);
	std::string Base64_Decode(const std::string&);
	void StrTrimRight(std::string&);
};
#endif
