#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>

#include "endian.h"

namespace {

struct MemoryBuffer : public std::basic_streambuf<char> {
	MemoryBuffer(const char *begin, size_t size) {
		char *ptr(const_cast<char *>(begin));
		this->setg(ptr, ptr, ptr + size);
	}

protected:
	std::streampos seekpos(std::streampos pos, std::ios_base::openmode which = std::ios_base::in) override {
		this->setg(this->eback(), this->eback() + pos, this->egptr());
		return pos;
	}

	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in) override {
		switch (way) {
		case std::ios_base::beg:
			this->setg(this->eback(), this->eback() + off, this->egptr());
			break;
		case std::ios_base::cur:
			this->gbump(static_cast<int>(off));
			break;
		case std::ios_base::end:
			this->setg(this->eback(), this->egptr() - off, this->egptr());
			break;
		}
		return std::streampos(this->gptr() - this->eback());
	}
};

}

class BinaryReader {
public:
	explicit BinaryReader(std::istream &is) : ownsStream_(false), is_(&is) {
	}

	BinaryReader(const char *data, size_t size) : ownsStream_(true) {
		memoryBuffer_ = std::make_unique<MemoryBuffer>(data, size);
		is_ = new std::istream(memoryBuffer_.get(), std::ios_base::binary);
		is_->rdbuf(memoryBuffer_.get());
		//std::cout << is_->good() << std::endl;
	}

	~BinaryReader() {
		if (ownsStream_) {
			delete is_;
		}
	}

	// Stream redirect functions
	std::istream &seekg(std::streampos pos) {
		return is_->seekg(pos);
	}

	std::istream &seekg(std::streamoff off, std::ios_base::seekdir way) {
		return is_->seekg(off, way);
	}

	std::streampos tellg() {
		return is_->tellg();
	}

	std::istream &skip(std::streamoff off) {
		return is_->seekg(off, std::ios_base::cur);
	}

	void read(char *buffer, size_t size) {
		is_->read(buffer, size);
	}

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

	std::string readString() {
		/*char c;
		is_->read(&c, 1);
		std::stringstream ss;
		while (is_->good() && c != 0) {
		ss << c;
		is_->read(&c, 1);
		}
		return ss.str();*/
		std::string val;
		std::getline(*is_, val, '\0');
		return val;
	}

	std::string readString(size_t length) {
		char *s = new char[length];
		is_->read(s, length);
		std::string val(s, length);
		delete[] s;
		return val;
	}
private:
	bool ownsStream_ = false;
	std::unique_ptr<MemoryBuffer> memoryBuffer_;
	std::istream *is_;
};

class Bitstream {
public:
	Bitstream() : bitOffset_(0) {}

	void wrap(const char *data, size_t size) {
		data_ = data;
		size_ = size;
		bitOffset_ = 0;
	}

	void seek(size_t bitOffset) {
		bitOffset_ = bitOffset;
	}

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