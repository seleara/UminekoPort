#include "string.h"

#include <iostream>
#include <sstream>
#include <unordered_set>

std::string operator+(const std::string &s, const std::string_view &sv) {
	std::string ret(s);
	ret += sv;
	return ret;
}

std::string operator+(const std::string_view &sv, const std::string &s) {
	std::string ret(s);
	ret += sv;
	return ret;
}

StringUtil::Separators StringUtil::Whitespace = { ' ', '\t', '\n', '\r' };

std::vector<std::string> StringUtil::split(const std::string &s, const StringUtil::Separators &sep) {
	std::vector<std::string> tokens;
	int bufferStart = 0;
	int bufferSize = 0;

	for (int i = 0; i < s.size(); ++i) {
		char c = s[i];
		if (std::find(sep.begin(), sep.end(), c) != sep.end()) {
			if (bufferSize == 0) {
				++bufferStart;
				continue;
			}
			tokens.push_back(s.substr(bufferStart, bufferSize));
			bufferSize = 0;
			bufferStart = i + 1;
		} else {
			++bufferSize;
		}
	}
	if (bufferSize > 0) {
		tokens.push_back(s.substr(bufferStart, bufferSize));
	}

	return tokens;
}

FastStringUtilHashMap StringUtil::splitRefMap_;

std::vector<std::string_view> StringUtil::splitRef(const std::string &s, const StringUtil::Separators &sep) {
	std::vector<std::string_view> tokens;
	int bufferStart = 0;
	int bufferSize = 0;

	splitRefMap_.clear();
	for (auto c : sep) {
		splitRefMap_.set(c);
	}

	for (int i = 0; i < s.size(); ++i) {
		char c = s[i];
		//if (std::find(sep.begin(), sep.end(), c) != sep.end()) {
		if (splitRefMap_.has(c)) {
			if (bufferSize == 0) {
				++bufferStart;
				continue;
			}
			//tokens.push_back(s.substr(bufferStart, bufferSize));
			tokens.emplace_back(s.c_str() + bufferStart, bufferSize);
			bufferSize = 0;
			bufferStart = i + 1;
		} else {
			++bufferSize;
		}
	}
	if (bufferSize > 0) {
		//tokens.push_back(s.substr(bufferStart, bufferSize));
		tokens.emplace_back(s.c_str() + bufferStart, bufferSize);
	}

	return tokens;
}