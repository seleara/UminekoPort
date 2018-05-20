#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>

#include "endian.h"

namespace detail {

struct MemoryBuffer : public std::basic_streambuf<char> {
	MemoryBuffer(const char *begin, size_t size);
protected:
	std::streampos seekpos(std::streampos pos, std::ios_base::openmode which = std::ios_base::in) override;
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in) override;
};

}

class BinaryReader {
public:
	BinaryReader();
	explicit BinaryReader(std::istream &is);
	BinaryReader(const char *data, size_t size);
	~BinaryReader();
	BinaryReader(BinaryReader &&other);
	BinaryReader &operator=(BinaryReader &&other);

	void wrap(const char *data, size_t size);

	// Stream redirect functions
	std::istream &seekg(std::streampos pos);
	std::istream &seekg(std::streamoff off, std::ios_base::seekdir way);

	std::streampos tellg();

	std::istream &skip(std::streamoff off);

	void read(char *buffer, size_t size);

	template <typename T>
	T read() {
		T val;
		is_->read(reinterpret_cast<char *>(&val), sizeof(T));
		return val;
	}

	template <typename T>
	T readBE() {
		T val;
		is_->read(reinterpret_cast<char *>(&val), sizeof(T));
		swapEndian(val);
		return val;
	}

	std::string readString();
	std::string readString(size_t length);
private:
	bool ownsStream_;
	std::unique_ptr<detail::MemoryBuffer> memoryBuffer_;
	std::istream *is_;
};

class Bitstream {
public:
	Bitstream();

	void wrap(const char *data, size_t size);

	void seek(size_t bitOffset);

	template <typename T>
	T read(size_t bits) {
		// TODO: Take precautions against bits > sizeof(T) * 8
		size_t byteOffset = bitOffset_ / 8;
		int m = bitOffset_ % 8;
		//std::cout << (int)(data_[byteOffset]);
		//std::cout << (int)(data_[byteOffset + 1]);
		T val = *reinterpret_cast<const T *>(data_ + byteOffset);
		swapEndian(val);
		bitOffset_ += bits;
		val >>= (sizeof(T) * 8 - bits - m);
		val &= ((1 << bits) - 1);
		/*size_t remaining = bits - sizeof(T) * 8;
		while (remaining > 0) {
		size_t byteOffset = bitOffset_ / 8;
		T val2 = *(T *)(data_ + byteOffset);
		bitOffset_ += bits;
		val &=
		}*/
		return val;
	}
private:
	const char *data_;
	size_t size_;
	size_t bitOffset_;
};