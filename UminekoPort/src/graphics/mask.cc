#include "mask.h"

#include "../util/binaryreader.h"

void load(const std::string &path, Archive &archive) {
	auto data = archive.read(path);
	BinaryReader br((char *)data.data(), data.size());
	
	auto magic = br.readString(4);
	if (magic != "MSK3") {
		throw std::runtime_error("Invalid mask magic.");
	}

	auto fileSize = br.read<uint32_t>();
	auto width = br.read<uint16_t>();
	auto height = br.read<uint16_t>();

}