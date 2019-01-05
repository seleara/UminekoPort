#include "log.h"

std::map<std::string, Log> Log::logs_;

Log &Log::create(const std::string &identifier) {
	auto &logref = logs_.emplace(std::make_pair(identifier, Log()));
	logref.first->second.identifier_ = identifier;
	return logref.first->second;
}

Log &Log::get(const std::string &identifier) {
	auto &iter = logs_.find(identifier);
	if (iter == logs_.end()) {
		throw std::runtime_error("Log does not exist.");
	}
	return iter->second;
}

Log::Log() : identifier_("<Untitled Log>") {
	output_ = std::make_unique<std::ostream>(&buffer_);
}

Log::Log(Log &&other) : identifier_(std::move(other.identifier_)), output_(std::move(other.output_)) {
	other.identifier_ = "";
}

Log &Log::operator=(Log &&other) {
	identifier_ = std::move(other.identifier_);
	other.identifier_ = "";
	output_ = std::move(other.output_);
	return *this;
}