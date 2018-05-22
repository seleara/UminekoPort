#pragma once

#include <vector>

class DataCompression {
public:
	static std::vector<uint8_t> decompress12_4(const uint8_t *compressedData, size_t compressedSize, size_t decompressedSize);
	static std::vector<uint8_t> decompress10_6(const uint8_t *compressedData, size_t compressedSize, size_t decompressedSize);
};