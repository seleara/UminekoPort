#include "font.h"

#include <iostream>
#include <iomanip>

#include <GL/glew.h>

#include "../data/archive.h"
#include "../util/binaryreader.h"
#include "../data/vertexbuffer.h"
#include "../graphics/shader.h"

std::unique_ptr<Font> Font::global_;

Font &Font::global() {
	if (!global_) {
		global_ = std::make_unique<Font>();
	}
	return *global_;
}

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

		for (int j = 0; j < glyph.unk3; ++j) {

		}

		for (int y = 0; y < glyph.height; ++y) {
			for (int x = 0; x < glyph.width; ++x) {
				glyph.pixels[y * glyph.width + x] = ((x + y) % 2) * 255;
			}
		}
	}

	archive.writeImage("export/glyph_maru.png", glyphs_[98].pixels.data(), glyphs_[98].width, glyphs_[98].height, glyphs_[98].width, 1);
}

const Glyph &Font::getGlyph(uint16_t code) const {
	auto first = ((code >> 8) & 0xff);
	if (first >= 0x81 && first < 0xa0) {
		return glyphs_[0x60 + (code - 0x8140)];
	}

	return glyphs_[0];
}

void Text::setFont(const Font &font) {
	font_ = &font;
}

void Text::setText(const std::string &text) {
	text_ = text;
	currentSegment_ = 0;
	isDirty_ = true;
}

void Text::setWrap(int width) {
	wrapWidth_ = width;
}

const std::string &Text::text() {
	return text_;
}

void Text::advance() {
	++currentSegment_;
	if (currentSegment_ >= segments_) {
		isDone_ = true;
	}
}

int Text::currentSegment() const {
	return currentSegment_;
}

bool Text::done() const {
	return isDone_;
}

Transform &Text::transform() {
	return transform_;
}

void Text::render() {
	if (text_.size() == 0) return;
	if (isDirty_) {
		renderFontTexture();
		isDirty_ = false;
	}
	if (glyphs_.size() == 0) return;

	Shader shader;
	shader.loadCache("text");
	shader.bind();

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	struct GlyphVertices {
		struct GlyphVertex {
			glm::vec2 pos, uv;
			glm::vec4 color;
		} vert[6];
	};

	std::vector<GlyphVertices> verts;

	float xAdvance = 0.0f, yAdvance = 0.0f;
	//for (int i = 0; i < text_.size(); ++i) {
	int pushKeyCount = 0;
	for (auto &glyph : glyphs_) {
		if (glyph->type == TextEntryType::LineBreak) {
			yAdvance += 80.0f;
			xAdvance = 0;
			continue;
		}
		if (glyph->type == TextEntryType::PushKey) {
			if (pushKeyCount >= currentSegment_) {
				break;
			}
			++pushKeyCount;
			continue;
		}
		if (glyph->type != TextEntryType::Glyph) continue;

		if (wrapWidth_ > 0 && xAdvance >= wrapWidth_) {
			yAdvance += 80.0f;
			xAdvance = 0;
		}

		const auto &tg = *(TextGlyph *)glyph.get();
		const auto &fg = *tg.fontGlyph;
		GlyphVertices gv;
		gv.vert[0].pos.x = transform_.position.x + xAdvance + fg.xOffset;
		gv.vert[0].pos.y = transform_.position.y + yAdvance + fg.yOffset;
		gv.vert[0].uv.x = tg.uvs.x;
		gv.vert[0].uv.y = tg.uvs.y;
		gv.vert[0].color = glm::vec4(1);

		gv.vert[1].pos.x = transform_.position.x + xAdvance + fg.xOffset;
		gv.vert[1].pos.y = transform_.position.y + yAdvance + fg.yOffset + fg.height;
		gv.vert[1].uv.x = tg.uvs.x;
		gv.vert[1].uv.y = tg.uvs.w;
		gv.vert[1].color = glm::vec4(1);

		gv.vert[2].pos.x = transform_.position.x + xAdvance + fg.xOffset + fg.width;
		gv.vert[2].pos.y = transform_.position.y + yAdvance + fg.yOffset;
		gv.vert[2].uv.x = tg.uvs.z;
		gv.vert[2].uv.y = tg.uvs.y;
		gv.vert[2].color = glm::vec4(1);

		gv.vert[3] = gv.vert[2];

		gv.vert[4] = gv.vert[1];

		gv.vert[5].pos.x = transform_.position.x + xAdvance + fg.xOffset + fg.width;
		gv.vert[5].pos.y = transform_.position.y + yAdvance + fg.yOffset + fg.height;
		gv.vert[5].uv.x = tg.uvs.z;
		gv.vert[5].uv.y = tg.uvs.w;
		gv.vert[5].color = glm::vec4(1);

		verts.push_back(std::move(gv));
		
		xAdvance += fg.xAdvance;
		//yAdvance += fg.yAdvance;
	}

	VertexBuffer<float> vb;
	vb.upload(verts.data(), verts.size() * 8 * 6);
	vb.bind();
	vb.setAttribute(0, 2, 8, 0);
	vb.setAttribute(1, 2, 8, 2);
	vb.setAttribute(2, 4, 8, 4);

	glActiveTexture(GL_TEXTURE0);
	fontTex_.bind();

	vb.draw(Primitives::Triangles, 0, verts.size() * 6);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

void Text::renderFontTexture() {
	static const int texWidth = 2048;
	static const int texHeight = 1024;

	if (fontTex_.id() == 0)
		fontTex_.create(texWidth, texHeight);
	//auto curFB = Framebuffer::getBound();
	//fontTex_.bindDraw();
	std::map<uint16_t, glm::vec4> uvMap;

	int xAdvance = 0, yAdvance = 0;
	int maxLineHeight = 0;

	bool inRuby = false;
	bool inRubyKanji = false;
	bool inRubyBeforeKanji = false;
	bool inRubyAfterKanji = false;

	TextRuby *curRuby = nullptr;

	segments_ = 1;

	glyphs_.clear();

	for (int i = 0; i < text_.size(); ++i) {
		std::unique_ptr<TextEntry> entry;

		uint8_t c = text_[i];

		if (c == 'r') {
			entry = std::make_unique<TextEntry>();
			entry->type = TextEntryType::LineBreak;
			glyphs_.push_back(std::move(entry));
			continue;
		} else if (c == 'k') {
			entry = std::make_unique<TextEntry>();
			entry->type = TextEntryType::PushKey;
			glyphs_.push_back(std::move(entry));
			++segments_;
			continue;
		} else if (c == 'v') {
			entry = std::make_unique<TextVoice>();
			auto *ve = (TextVoice *)entry.get();
			ve->type = TextEntryType::Voice;
			while (text_[i] != '.') {
				ve->filename += text_[++i];
			}
			glyphs_.push_back(std::move(entry));
			continue;
		} else if (c == 'b') {
			inRuby = true;
			entry = std::make_unique<TextRuby>();
			entry->type = TextEntryType::Ruby;
			glyphs_.push_back(std::move(entry));
			curRuby = (TextRuby *)glyphs_.back().get();
			continue;
		} else if (inRuby && c == '.') {
			inRubyBeforeKanji = true;
			inRuby = false;
			continue;
		} else if (inRubyBeforeKanji && c == '<') {
			inRubyKanji = true;
			inRubyBeforeKanji = false;
			continue;
		} else if (inRubyKanji && c == '>') {
			inRubyAfterKanji = true;
			inRubyKanji = false;
		} else if (inRubyAfterKanji && c == '.') {
			curRuby = nullptr;
			inRubyAfterKanji = false;
		} else if (inRubyBeforeKanji || inRubyAfterKanji) {
			continue;
		}

		entry = std::make_unique<TextGlyph>();
		entry->type = TextEntryType::Glyph;
		TextGlyph &tg = *(TextGlyph *)entry.get();
		uint16_t code;
		if (isSJISDoubleByte(c)) {
			uint8_t low = text_[++i];
			code = (c << 8) | low;
			std::cout << std::hex << std::setw(4) << std::setfill('0') << code << std::dec << '\n';
			tg.fontGlyph = &font_->getGlyph(code);
		} else {
			code = c;
			tg.fontGlyph = &font_->getGlyph(code);
			std::cout << std::hex << std::setw(2) << std::setfill('0') << code << std::dec << '\n';
		}

		if (inRuby) continue;

		// Check if the glyph has already been rendered
		auto iter = uvMap.find(code);
		if (iter != uvMap.end()) {
			tg.uvs = iter->second;
			continue;
		}

		if (tg.fontGlyph->height > maxLineHeight) {
			maxLineHeight = tg.fontGlyph->height;
		}

		if (xAdvance + tg.fontGlyph->width >= texWidth) {
			xAdvance = 0;
			yAdvance += maxLineHeight;
			maxLineHeight = 0;
		}

		tg.uvs.x = xAdvance;
		tg.uvs.y = yAdvance;
		tg.uvs.z = xAdvance + tg.fontGlyph->width;
		tg.uvs.w = yAdvance + tg.fontGlyph->height;

		fontTex_.subImage(xAdvance, yAdvance, tg.fontGlyph->width, tg.fontGlyph->height, 1, tg.fontGlyph->pixels);

		xAdvance += tg.fontGlyph->width;

		uvMap.insert({ code, tg.uvs });

		if (curRuby) {
			curRuby->glyphs.push_back(std::move(entry));
		} else {
			glyphs_.push_back(std::move(entry));
		}
	}
	/*if (curFB) {
		curFB->bindDraw();
	} else {
		Framebuffer::bindDrawNull();
	}*/
}

std::string Text::convert(const std::string &text) {
	std::string ret;
	for (int i = 0; i < text.size(); ++i) {
		uint8_t c = text[i];
		switch (c) {
		case 0xA1: ret += "\x81\x42"; break;
		case 0xA2: ret += "\x81\x75"; break;
		case 0xA3: ret += "\x81\x76"; break;
		case 0xA4: ret += "\x81\x41"; break;
		case 0xA5: ret += "\x81\x63"; break;
		case 0xA6: ret += "\x82\xF0"; break;
		case 0xA7: ret += "\x82\x9F"; break;
		case 0xA8: ret += "\x82\xA1"; break;
		case 0xA9: ret += "\x82\xA3"; break;
		case 0xAA: ret += "\x82\xA5"; break;
		case 0xAB: ret += "\x82\xA7"; break;
		case 0xAC: ret += "\x82\xE1"; break;
		case 0xAD: ret += "\x82\xE3"; break;
		case 0xAE: ret += "\x82\xE5"; break;
		case 0xAF: ret += "\x82\xC1"; break;
		case 0xB0: ret += "\x82\x5B"; break;

		// ta-row has a small tsu in it so parse it here instead of below
		case 0xC0: ret += "\x82\xBD"; break;
		case 0xC1: ret += "\x82\xBF"; break;
		case 0xC2: ret += "\x82\xC2"; break;
		case 0xC3: ret += "\x82\xC4"; break;
		case 0xC4: ret += "\x82\xC6"; break;

		case 0xDC: ret += "\x82\xED"; break; // wa
		case 0xDD: ret += "\x82\xF1"; break; // n
		case 0xDE: ret += "\x81\x49"; break; // !
		case 0xDF: ret += "\x81\x48"; break; // ?
		default:
			if (isSJISDoubleByte(c)) {
				ret += c;
				ret += text[i + 1];
				++i;
			} else if (c >= 0xB1 && c <= 0xB5) { // aiueo
				ret += "\x82";
				ret += (char)(0xA0 + (c - 0xB1) * 2);
			} else if (c >= 0xB6 && c <= 0xBF) { // kakikukekosasisuseso
				ret += "\x82";
				ret += (char)(0xA9 + (c - 0xB6) * 2);
			} else if (c >= 0xC5 && c <= 0xC9) { // naninuneno
				ret += "\x82";
				ret += (char)(0xC8 + (c - 0xC5));
			} else if (c >= 0xCA && c <= 0xCE) { // hahihuheho
				ret += "\x82";
				ret += (char)(0xCD + (c - 0xCA) * 3);
			} else if (c >= 0xCF && c <= 0xD3) { // mamimumemo
				ret += "\x82";
				ret += (char)(0xDC + (c - 0xCF));
			} else if (c >= 0xD4 && c <= 0xD6) { // yayuyo
				ret += "\x82";
				ret += (char)(0xE2 + (c - 0xD4) * 2);
			} else if (c >= 0xD7 && c <= 0xDB) { // rarirurero
				ret += "\x82";
				ret += (char)(0xE7 + (c - 0xD7));
			} else {
				ret += c;
			}
			break;
		}
	}
	std::cout << "Converted: " << ret << '\n';
	return ret;
}