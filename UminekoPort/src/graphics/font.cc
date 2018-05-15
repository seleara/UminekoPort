#include "font.h"

#include "../data/archive.h"
#include "../util/binaryreader.h"

void Font::load(const std::string &filename, Archive &archive) {
	auto data = archive.read(filename);
	BinaryReader br((const char *)data.data(), data.size());
	
	auto magic = br.readString(4);
	if (magic != "FNT3") {
		throw std::runtime_error("File is not a valid font file.");
	}

	auto fileSize = br.read<uint32_t>();
	auto unknown1 = br.read<uint32_t>();
	auto unknown2 = br.read<uint32_t>();

	const int glyphCount = 8180;
	std::vector<uint32_t> offsets;
	offsets.reserve(glyphCount);
	for (int i = 0; i < glyphCount; ++i) {
		offsets.push_back(br.read<uint32_t>());
	}

	for (int i = 0; i < glyphCount; ++i) {
		br.seekg(offsets[i]);

		auto &glyph = glyphs_.emplace_back();
		glyph.xOffset = br.read<uint8_t>();
		glyph.yOffset = br.read<uint8_t>();
		glyph.width = br.read<uint8_t>();
		glyph.height = br.read<uint8_t>();
		glyph.xAdvance = br.read<uint8_t>();
		glyph.yAdvance = br.read<uint8_t>();

		glyph.unk1 = br.read<uint8_t>();
		glyph.unk2 = br.read<uint8_t>();
		glyph.unk3 = br.read<uint8_t>();

		glyph.pixels.resize(glyph.width * glyph.height);
	}

	archive.writeImage("export/glyph_maru.png", glyphs_[98].pixels.data(), glyphs_[98].width, glyphs_[98].height, glyphs_[98].width, 1);
}