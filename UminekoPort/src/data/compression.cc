#include "compression.h"

std::vector<uint8_t> DataCompression::decompress10_6(const uint8_t *compressedData, size_t compressedSize, size_t decompressedSize) {
	std::vector<uint8_t> literals;
	literals.resize(decompressedSize);

	int bytesRead = 0, bytesWritten = 0;
	uint8_t *litPtr = literals.data();
	const uint8_t *readPtr = compressedData;

	while (bytesRead < compressedSize) {
		auto ctrl = *(readPtr++);
		++bytesRead;
		/*std::cout << "[";
		for (int i = 0; i < 8; ++i) {
		std::cout << (int)((ctrl >> (7 - i)) & 1) << (i < 7 ? " " : "");
		}
		std::cout << "] ";*/
		for (int i = 0; i < 8; ++i) {
			auto type = (ctrl >> i) & 0x1;
			if (type == 0) { // Literal byte
				++bytesRead;
				if (bytesRead > compressedSize) break; // The compressed data can end prematurely
				auto val = *(readPtr++);
				*(litPtr++) = val;
				++bytesWritten;
			} else { // Copy from decompressed output
				bytesRead += 2;
				if (bytesRead > compressedSize) break; // The compressed data can end prematurely

				// The back reference is a 16-bit value laid out like this, from most significant bit to least significant bit:
				// AAAA AAAA BBCC CCCC
				// The relative offset is then the 10-bit value "BBAAAAAA" + 3, and the number of bytes to copy is the 6-bit value "CCCCCC"
				//const auto backRefLow = *(readPtr++);
				//auto backRef = ((*(readPtr++) << 8) & 0xff00) | (backRefLow & 0xff);
				auto backRef = *(uint16_t *)readPtr;
				readPtr += 2;
				int offset = (((backRef >> 8) & 0xff) | (((backRef >> 6) & 0x3) << 8)) + 1;
				int count = (backRef & 0x3f) + 3;
				int absOffset = bytesWritten - offset;
				for (int i = 0; i < count; ++i) {
					auto copyVal = literals[absOffset + i];
					*(litPtr++) = copyVal;
					++bytesWritten;
				}
			}
		}
		//std::cout << "\n";
	}

	return literals;
}

std::vector<uint8_t> DataCompression::decompress12_4(const uint8_t *compressedData, size_t compressedSize, size_t decompressedSize) {
	std::vector<uint8_t> literals;
	literals.resize(decompressedSize);

	int bytesRead = 0, bytesWritten = 0;
	uint8_t *litPtr = literals.data();
	const uint8_t *readPtr = compressedData;

	while (bytesRead < compressedSize) {
		auto ctrl = *(readPtr++);
		++bytesRead;
		for (int i = 0; i < 8; ++i) {
			auto type = (ctrl >> i) & 0x1;
			if (type == 0) { // Literal byte
				++bytesRead;
				if (bytesRead > compressedSize) break; // The compressed data can end prematurely
				auto val = *(readPtr++);
				*(litPtr++) = val;
				++bytesWritten;
			} else { // Copy from decompressed output
				bytesRead += 2;
				if (bytesRead > compressedSize) break; // The compressed data can end prematurely

				// The back reference is a 16-bit value laid out like this, from most significant bit to least significant bit:
				// AAAA AAAA BBBB CCCC
				// The relative offset is then the 12-bit value "BBBBAAAAAA" + 3, and the number of bytes to copy is the 4-bit value "CCCC"
				//const auto backRefLow = *(readPtr++);
				//auto backRef = ((*(readPtr++) << 8) & 0xff00) | (backRefLow & 0xff);
				auto backRef = *(uint16_t *)readPtr;
				readPtr += 2;
				int offset = ((backRef >> 8) & 0xff) | (((backRef >> 4) & 0xf) << 8) + 1;
				int count = (backRef & 0xf) + 3;
				int absOffset = bytesWritten - offset;
				for (int i = 0; i < count; ++i) {
					auto copyVal = literals[absOffset + i];
					*(litPtr++) = copyVal;
					++bytesWritten;
				}
			}
		}
	}

	return literals;
}