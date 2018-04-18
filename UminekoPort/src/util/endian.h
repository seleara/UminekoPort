#pragma once

#include <cstdint>

template <typename T>
inline void swapEndian(T &v) {
	auto tmp = v;
	for (int i = 0; i < sizeof(T); ++i) {
		*(reinterpret_cast<char *>(&tmp) + i) = *(reinterpret_cast<char *>(&v) + (sizeof(T) - 1 - i));
	}
	v = tmp;
}

template <>
inline void swapEndian(uint8_t &v) {
	// Do nothing for 8-bit value
}