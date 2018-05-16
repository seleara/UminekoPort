#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../math/transform.h"
#include "../graphics/texture.h"

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
	static Font &global();
	void load(const std::string &filename, Archive &archive);

	const Glyph &getGlyph(uint16_t code) const;
private:
	static std::unique_ptr<Font> global_;
	std::vector<Glyph> glyphs_;
};

enum class TextEntryType {
	Glyph = 0,
	LineBreak = 1,
	PushKey = 2,
	Voice = 3,
	Ruby = 4
};

struct TextEntry {
	TextEntryType type;
};

struct TextGlyph : public TextEntry {
	const Glyph *fontGlyph;
	glm::vec4 uvs;
};

struct TextVoice : public TextEntry {
	std::string filename;
};

struct TextRuby : public TextEntry {
	std::string furigana;
	std::vector<std::unique_ptr<TextEntry>> glyphs;
};

class Text {
public:
	void setFont(const Font &font);
	void setText(const std::string &text);
	void setWrap(int width);
	const std::string &text();

	void advance();
	int currentSegment() const;
	bool done() const;

	static std::string convert(const std::string &text);
	static inline bool isSJISDoubleByte(uint8_t c) {
		return (c >= 0x81 && c < 0xa0) || (c >= 0xe0);
	}

	Transform &transform();
	void render();
private:
	void renderFontTexture();

	std::vector<std::unique_ptr<TextEntry>> glyphs_;

	const Font *font_;
	Transform transform_;
	std::string text_;
	Texture fontTex_;
	bool isDirty_ = false;
	int wrapWidth_ = 0;
	int currentSegment_ = 0, segments_ = 0;
	bool isDone_ = false; // When the user has advanced through the whole text
};