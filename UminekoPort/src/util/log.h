#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>

class LogBuffer : public std::streambuf {
public:
	LogBuffer() : std::streambuf() {
		buffer_.resize(1024);
		start_ = buffer_.data();
		end_ = start_ + buffer_.size();
		setp(start_, end_);
	}

	/*std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
		switch (way) {
		case std::ios_base::beg:
			put_ = (char *)off;
			break;
		case std::ios_base::cur:
			put_ += off;
			break;
		case std::ios_base::end:
			put_ = end_ - off;
			break;
		}
		return std::streampos(std::streamoff(-1));
	}*/

	char *ptr() const {
		return pbase();
	}

	char *putptr() const {
		return pptr();
	}

	/*std::streamsize xsputn(const char *s, std::streamsize n) override {
		std::copy(s, s + n, buffer_.data());
		return n;
	}

	int sync() override {
		return 0;
	}*/

	int overflow(int c = -1) override {
		if (c == -1) return -1;
		if (pptr() == epptr()) {
			buffer_.resize(buffer_.size() * 2);
			start_ = buffer_.data();
			end_ = start_ + buffer_.size();
			setp(start_, end_);
		}
		*pptr() = c;
		return sputc(c);
	}
private:
	char *start_, *end_;
	std::vector<char> buffer_;
};

class LogStream : public std::ostream {
public:
	LogStream() : buffer_(), std::ostream(&buffer_) {
	}

	LogStream(LogStream &&other) : std::ostream(std::move(other)), buffer_(std::move(other.buffer_)) {}

	LogStream &operator=(LogStream &&other) {
		std::ostream::operator=(std::move(other));
		buffer_ = std::move(other.buffer_);
		return *this;
	}

	char *ptr() const {
		return buffer_.ptr();
	}

	size_t count() const {
		return buffer_.putptr() - buffer_.ptr();
	}
private:
	LogBuffer buffer_;
};

class Log {
public:
	static Log &create(const std::string &identifier);
	static Log &get(const std::string &identifier);

	template <typename ...Args>
	Log &print(Args && ...args) {
		std::lock_guard<std::mutex> lock(logMutex_);
		typedef std::chrono::duration<int, std::ratio<86400>> days;
		auto now = std::chrono::system_clock::now();
		auto tp = now.time_since_epoch();
		auto d = std::chrono::duration_cast<days>(tp);
		tp -= d;
		auto h = std::chrono::duration_cast<std::chrono::hours>(tp);
		tp -= h;
		auto m = std::chrono::duration_cast<std::chrono::minutes>(tp);
		tp -= m;
		auto s = std::chrono::duration_cast<std::chrono::seconds>(tp);
		*output_ << '[' << h.count() << ':' << m.count() << ':' << s.count() << "] ";
		auto &ref = print_(std::forward<Args>(args)...);
		*output_ << '\n';
		return ref;
	}

	std::string_view view() {
		//std::string_view v(output_.ptr(), output_.count());
		std::string_view v(buffer(), count());
		return v;
	}

	char *buffer() {
		return buffer_.ptr();
	}

	size_t count() {
		return buffer_.putptr() - buffer_.ptr();
	}
	Log();
	Log(const Log &other) = delete;
	Log(Log &&other);
	Log &operator=(const Log &other) = delete;
	Log &operator=(Log &&other);
private:

	template <typename none = void>
	Log &print_() {
		return *this;
	}

	template <typename Head, typename ...Tail>
	Log &print_(Head &&head, Tail && ...tail) {
		*output_ << std::forward<Head>(head);
		return print_(std::forward<Tail>(tail)...);
	}

	std::string identifier_;
	LogBuffer buffer_;
	//LogStream output_;
	std::unique_ptr<std::ostream> output_;

	static std::map<std::string, Log> logs_;
	std::mutex logMutex_;
};
