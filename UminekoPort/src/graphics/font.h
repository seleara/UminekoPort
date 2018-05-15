#pragma once

#include <string>
#include <vector>

class Archive;

struct Glyph {
	uint8_t xOffset;
	uint8_t yOffset;
	uint8_t width;
	uint8_t height;
	uint8_t xAdvance;
	uint8_t yAdvance;

	uint8_t unk1, unk2, unk3;

	std::vector<uint8_t> pixels;
};

class Font {
public:
	void load(const std::string &filename, Archive &archive);
private:

	std::vector<Glyph> glyphs_;
};