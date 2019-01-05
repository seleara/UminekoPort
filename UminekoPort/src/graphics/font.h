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

	uint16_t compressedSize;

	std::vector<uint8_t> pixels;

	bool initialized = false;
};

class Font {
public:
	static Font &global();
	void load(const std::string &filename, Archive &archive);

	const Glyph &getGlyph(uint16_t code);
private:
	const Glyph &initGlyph(uint32_t index);
	static std::unique_ptr<Font> global_;

	enum class FontVersion {
		Fnt3, Fnt4
	} version_;

	std::vector<uint32_t> offsets_;
	std::vector<unsigned char> data_;
	std::vector<Glyph> glyphs_;
	
	std::mutex fontMutex_;
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
	std::vector<std::unique_ptr<TextEntry>> furigana;
	std::vector<std::unique_ptr<TextEntry>> glyphs;
};

struct ShaderTextData {
	glm::vec4 progress;
};

class Text {
public:
	void setFont(Font &font);
	void setText(const std::string &text);
	void setWrap(int width);
	const std::string &text();

	void advance();
	int currentSegment() const;
	bool done() const;

	bool hasVoice() const;
	const std::string &getVoice() const;

	static std::string convert(const std::string &text);
	static inline bool isSJISDoubleByte(uint8_t c) {
		return (c >= 0x81 && c < 0xa0) || (c >= 0xe0);
	}

	Transform &transform();
	void update();
	void render();
private:
	void setupGlyphs();
	void renderFontTexture();

	std::vector<std::unique_ptr<TextEntry>> glyphs_;

	Font *font_;
	Transform transform_;
	std::string text_;
	Texture fontTex_;
	bool isDirty_ = false;
	int wrapWidth_ = 0;
	int currentSegment_ = 0, segments_ = 0;
	bool isDone_ = false; // When the user has advanced through the whole text
	float progress_ = 0;

	TextVoice *currentVoice_ = nullptr;

	std::mutex textMutex_;

	static constexpr int texWidth = 2048;
	static constexpr int texHeight = 1024;
};