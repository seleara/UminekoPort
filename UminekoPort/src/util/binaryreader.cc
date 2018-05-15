#include "binaryreader.h"

namespace detail {

MemoryBuffer::MemoryBuffer(const char *begin, size_t size) {
	char *ptr(const_cast<char *>(begin));
	this->setg(ptr, ptr, ptr + size);
}

std::streampos MemoryBuffer::seekpos(std::streampos pos, std::ios_base::openmode which) {
	this->setg(this->eback(), this->eback() + pos, this->egptr());
	return pos;
}

std::streampos MemoryBuffer::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
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

}

BinaryReader::BinaryReader() : ownsStream_(false), is_(nullptr) {}

BinaryReader::BinaryReader(std::istream &is) : ownsStream_(false), is_(&is) {
}

BinaryReader::BinaryReader(const char *data, size_t size) : ownsStream_(true) {
	memoryBuffer_ = std::make_unique<detail::MemoryBuffer>(data, size);
	is_ = new std::istream(memoryBuffer_.get(), std::ios_base::binary);
	is_->rdbuf(memoryBuffer_.get());
	//std::cout << is_->good() << std::endl;
}

BinaryReader::~BinaryReader() {
	if (ownsStream_) {
		delete is_;
	}
}

void BinaryReader::wrap(const char *data, size_t size) {
	ownsStream_ = true;
	memoryBuffer_ = std::make_unique<detail::MemoryBuffer>(data, size);
	is_ = new std::istream(memoryBuffer_.get(), std::ios_base::binary);
	is_->rdbuf(memoryBuffer_.get());
}

// Stream redirect functions
std::istream &BinaryReader::seekg(std::streampos pos) {
	return is_->seekg(pos);
}

std::istream &BinaryReader::seekg(std::streamoff off, std::ios_base::seekdir way) {
	return is_->seekg(off, way);
}

std::streampos BinaryReader::tellg() {
	return is_->tellg();
}

std::istream &BinaryReader::skip(std::streamoff off) {
	return is_->seekg(off, std::ios_base::cur);
}

void BinaryReader::read(char *buffer, size_t size) {
	is_->read(buffer, size);
}

std::string BinaryReader::readString() {
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

std::string BinaryReader::readString(size_t length) {
	char *s = new char[length];
	is_->read(s, length);
	std::string val(s, length);
	delete[] s;
	return val;
}

Bitstream::Bitstream() : bitOffset_(0) {}

void Bitstream::wrap(const char *data, size_t size) {
	data_ = data;
	size_ = size;
	bitOffset_ = 0;
}

void Bitstream::seek(size_t bitOffset) {
	bitOffset_ = bitOffset;
}