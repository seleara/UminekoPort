#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

// string_view additions
std::string operator+(const std::string &s, const std::string_view &sv);
std::string operator+(const std::string_view &sv, const std::string &s);

class FastStringUtilHashMap {
public:
	FastStringUtilHashMap() {
		map_.resize(0x100);
	}

	void clear() {
		std::fill(map_.begin(), map_.end(), 0);
	}

	void set(char c) {
		map_[c] = 1;
	}

	bool has(char c) {
		return map_[c];
	}
private:
	std::vector<int> map_;
};

class StringUtil {
public:
	typedef std::vector<char> Separators;
	static Separators Whitespace;

	static std::vector<std::string> split(const std::string &s, const Separators &sep = Whitespace);
	static std::vector<std::string_view> StringUtil::splitRef(const std::string &s, const StringUtil::Separators &sep = Whitespace);

	static std::string utf8_substr(const std::string &str, unsigned int start, unsigned int leng) {
		if (leng == 0) { return ""; }
		size_t ix, min = std::string::npos, max = std::string::npos;
		unsigned int c, i, q;
		for (q = 0, i = 0, ix = str.length(); i < ix; i++, q++)
		{
			if (q == start) { min = i; }
			if (q <= start + leng || leng == std::string::npos) { max = i; }

			c = (unsigned char)str[i];
			if (
				//c>=0   &&
				c <= 127) i += 0;
			else if ((c & 0xE0) == 0xC0) i += 1;
			else if ((c & 0xF0) == 0xE0) i += 2;
			else if ((c & 0xF8) == 0xF0) i += 3;
			//else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
			//else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
			else return "";//invalid utf8
		}
		if (q <= start + leng || leng == std::string::npos) { max = i; }
		if (min == std::string::npos || max == std::string::npos) { return ""; }
		return str.substr(min, max - min);
	}

	static size_t utf8_length(const std::string &str) {
		if (str == "") return 0;
		size_t len = 0;
		const char *p = str.c_str();
		while (*p) {
			char c = *p;
			if ((c & 0x80) == 0) {
				len += 1;
				++p;
			} else if ((c & 0xe0) == 0xc0) {
				len += 1;
				p += 2;
			} else if ((c & 0xf0) == 0xe0) {
				len += 1;
				p += 3;
			} else if ((c & 0xf8) == 0xf0) {
				len += 1;
				p += 4;
			}
		}
		return len;
	}

	static inline void trim(std::string &str) {
		ltrim(str);
		rtrim(str);
	}

	static inline void ltrim(std::string &str) {
		str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int c) {
			return !std::isspace(c);
		}));
	}

	static inline void rtrim(std::string &str) {
		str.erase(std::find_if(str.rbegin(), str.rend(), [](int c) {
			return !std::isspace(c);
		}).base(), str.end());
	}

	static inline void toLower(std::string &str) {
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	}

	static inline std::string baseName(std::string str) {
		auto baseNamePos = str.rfind('\\');
		if (baseNamePos == std::string::npos) {
			baseNamePos = str.rfind('/');
		}
		if (baseNamePos != std::string::npos) {
			str.erase(0, baseNamePos + 1);
		}
		return str;
	}

	static inline std::string folder(std::string str) {
		auto slashPos = str.rfind('/');
		if (slashPos == std::string::npos) {
			slashPos = str.rfind('\\');
		}
		if (slashPos != std::string::npos)
			str = str.substr(0, slashPos + 1);
		return str;
	}
private:
	static FastStringUtilHashMap splitRefMap_;
};