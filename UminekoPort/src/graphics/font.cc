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
	std::lock_guard<std::mutex> lock(fontMutex_);
	data_ = std::move(archive.read(filename));
	BinaryReader br((const char *)data_.data(), data_.size());
	
	auto magic = br.readString(4);
	if ((magic != "FNT3") && (magic != "FNT4")) {
		throw std::runtime_error("File is not a valid font file.");
	}

	auto fileSize = br.read<uint32_t>();
	auto unknown1 = br.read<uint32_t>();
	auto unknown2 = br.read<uint32_t>();

	const int glyphCount = 8180;
	offsets_.reserve(glyphCount);
	for (int i = 0; i < glyphCount; ++i) {
		offsets_.push_back(br.read<uint32_t>());
	}
	glyphs_.resize(glyphCount);

	//archive.writeImage("export/glyph_maru.png", glyphs_[98].pixels.data(), glyphs_[98].width, glyphs_[98].height, glyphs_[98].width, 1);
	//auto fwzero = getGlyph(0x82f1);
	//archive.writeImage("export/glyph_fwzero.png", fwzero.pixels.data(), fwzero.width, fwzero.height, fwzero.width, 1);
}

const Glyph &Font::getGlyph(uint16_t code) {
	auto first = ((code >> 8) & 0xff);
	if (first >= 0x81 && first < 0xa0) {
		auto second = code & 0xff;
		auto index = 0x60 + (first - 0x81) * 0xbc;
		if (second <= 0x80)
			return initGlyph(index + (second - 0x40));
		else if (second <= 0xfc)
			return initGlyph(index + (second - 0x41));

		return initGlyph(0);
	}

	if (first >= 0x20 && first <= 0x7f)
		return initGlyph(first - 0x20);

	return initGlyph(0);
}

const Glyph &Font::initGlyph(uint32_t index) {
	std::lock_guard<std::mutex> lock(fontMutex_);
	auto &glyph = glyphs_[index];

	if (!glyph.initialized) {
		glyph.initialized = true;
		//std::cout << "Reading glyph #" << i << "\n===================\n";
		//br.seekg(offsets[i]);
		uint8_t *readPtr = data_.data() + offsets_[index];

		glyph.xOffset = *(readPtr++);//br.read<uint8_t>();
		glyph.yOffset = *(readPtr++);
		glyph.width = *(readPtr++);
		glyph.height = *(readPtr++);
		glyph.xAdvance = *(readPtr++);
		glyph.yAdvance = *(readPtr++);

		const auto compressedLow = *(readPtr++);
		glyph.compressedSize = ((*(readPtr++) << 8) & 0xff00) | (compressedLow & 0xff);

		glyph.pixels.resize(glyph.width * glyph.height);

		if (glyph.compressedSize == 0)
			return glyph;

		auto expandNibble = [](int nibble) -> uint8_t {
			return static_cast<uint8_t>(((nibble << 4) & 0xf0) | (nibble & 0xf));
		};

		std::vector<uint8_t> literals;
		auto modWidth = (glyph.width % 2 == 0) ? glyph.width : (glyph.width + 1);
		literals.resize(modWidth * glyph.height / 2);
		int bytesRead = 0, bytesWritten = 0;
		uint8_t *litPtr = literals.data();
		while (bytesRead < glyph.compressedSize) {
			auto ctrl = *(readPtr++);
			++bytesRead;
			/*std::cout << "[";
			for (int i = 0; i < 8; ++i) {
			std::cout << (int)((ctrl >> (7 - i)) & 1) << (i < 7 ? " " : "");
			}
			std::cout << "] ";*/
			for (int i = 0; i < 8; ++i) {
				auto type = (ctrl >> i) & 0x1;
				if (type == 0) { // Literal byte, encoding 2 pixels as 2 4-bit values
					++bytesRead;
					if (bytesRead > glyph.compressedSize) break; // The compressed data can end prematurely
					auto val = *(readPtr++);
					*(litPtr++) = val;
					++bytesWritten;
				} else {
					bytesRead += 2;
					if (bytesRead > glyph.compressedSize) break; // The compressed data can end prematurely
					const auto backRefLow = *(readPtr++);
					auto backRef = ((*(readPtr++) << 8) & 0xff00) | (backRefLow & 0xff);
					int offset = (((backRef >> 8) & 0xff) | (((backRef >> 6) & 0x3) << 8)) + 1;
					int count = (backRef & 0x3f) + 3;
					int absOffset = bytesWritten - offset;
					for (int i = 0; i < count; ++i) {
						auto copyVal = literals[absOffset + i];
						*(litPtr++) = copyVal;
						++bytesWritten;
					}
				}
			}
			//std::cout << "\n";
		}

		int nibbleCount = 0;
		int literalIndex = 0;
		uint8_t cache = literals[0];
		auto getNibble = [&]() -> int {
			if (nibbleCount == 2) {
				cache = literals[++literalIndex];
				nibbleCount = 0;
			}
			if (nibbleCount == 0) {
				++nibbleCount;
				return (cache >> 4) & 0xf;
			} else if (nibbleCount == 1) {
				++nibbleCount;
				return cache & 0xf;
			}
		};

		int pixelPtr = 0;
		for (int i = 0; i < glyph.pixels.size(); ++i) {
			glyph.pixels[pixelPtr++] = expandNibble(getNibble());
			if (glyph.width % 2 != 0 && pixelPtr % glyph.width == 0)
				getNibble();
		}
	}

	return glyph;
}

void Text::setFont(Font &font) {
	std::lock_guard<std::mutex> lock(textMutex_);
	font_ = &font;
}

void Text::setText(const std::string &text) {
	std::lock_guard<std::mutex> lock(textMutex_);
	text_ = text;
	currentSegment_ = 0;
	isDone_ = false;
	isDirty_ = true;
	currentVoice_ = nullptr;

	setupGlyphs();

	int pushKeyCount = 0;
	for (auto &glyph : glyphs_) {
		if (glyph->type == TextEntryType::Voice) {
			currentVoice_ = (TextVoice *)glyph.get();
		}
		if (glyph->type == TextEntryType::PushKey) {
			if (pushKeyCount >= currentSegment_) {
				break;
			}
			++pushKeyCount;
			currentVoice_ = nullptr;
			continue;
		}
	}
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
	} else {
		int pushKeyCount = 0;
		std::lock_guard<std::mutex> lock(textMutex_);
		for (auto &glyph : glyphs_) {
			if (glyph->type == TextEntryType::Voice) {
				currentVoice_ = (TextVoice *)glyph.get();
			}
			if (glyph->type == TextEntryType::PushKey) {
				if (pushKeyCount >= currentSegment_) {
					break;
				}
				++pushKeyCount;
				currentVoice_ = nullptr;
				continue;
			}
		}
	}
}

int Text::currentSegment() const {
	return currentSegment_;
}

bool Text::done() const {
	return isDone_;
}

bool Text::hasVoice() const {
	return currentVoice_ != nullptr;
}

const std::string &Text::getVoice() const {
	return currentVoice_->filename;
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

	auto addGlyph = [&](const std::unique_ptr<TextEntry> &glyph) {
		if (glyph->type != TextEntryType::Glyph) return false;

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

		return false;
	};

	auto addFurigana = [&](const std::unique_ptr<TextEntry> &glyph, int xStart) {
		if (glyph->type != TextEntryType::Glyph) return false;

		const auto &tg = *(TextGlyph *)glyph.get();
		const auto &fg = *tg.fontGlyph;
		GlyphVertices gv;

		auto baseX = transform_.position.x + xStart + fg.xOffset;
		auto baseY = transform_.position.y + yAdvance + fg.yOffset - 80.0f;

		gv.vert[0].pos.x = baseX;
		gv.vert[0].pos.y = baseY;
		gv.vert[0].uv.x = tg.uvs.x;
		gv.vert[0].uv.y = tg.uvs.y;
		gv.vert[0].color = glm::vec4(1);

		gv.vert[1].pos.x = baseX;
		gv.vert[1].pos.y = baseY + fg.height * 0.4f;
		gv.vert[1].uv.x = tg.uvs.x;
		gv.vert[1].uv.y = tg.uvs.w;
		gv.vert[1].color = glm::vec4(1);

		gv.vert[2].pos.x = baseX + fg.width * 0.4f;
		gv.vert[2].pos.y = baseY;
		gv.vert[2].uv.x = tg.uvs.z;
		gv.vert[2].uv.y = tg.uvs.y;
		gv.vert[2].color = glm::vec4(1);

		gv.vert[3] = gv.vert[2];

		gv.vert[4] = gv.vert[1];

		gv.vert[5].pos.x = baseX + fg.width * 0.4f;
		gv.vert[5].pos.y = baseY + fg.height * 0.4f;
		gv.vert[5].uv.x = tg.uvs.z;
		gv.vert[5].uv.y = tg.uvs.w;
		gv.vert[5].color = glm::vec4(1);

		verts.push_back(std::move(gv));

		return false;
	};

	auto checkGlyph = [&](const std::unique_ptr<TextEntry> &glyph) {
		if (glyph->type == TextEntryType::LineBreak) {
			yAdvance += 80.0f;
			xAdvance = 0;
			return false;
		}
		if (glyph->type == TextEntryType::PushKey) {
			if (pushKeyCount >= currentSegment_) {
				return true;
			}
			++pushKeyCount;
			return false;
		}
		if (glyph->type == TextEntryType::Ruby) {
			const auto &tr = *(TextRuby *)glyph.get();
			auto startX = xAdvance;
			for (const auto &g : tr.glyphs) {
				addGlyph(g);
			}
			auto endX = xAdvance;
			if (tr.furigana.size() == 0) return false; // Weird

			int glyphWidth = endX - startX;
			int glyphMiddle = startX + glyphWidth / 2;

			int furiganaWidth = 0;
			for (const auto &g : tr.furigana) {
				const auto &tg = *(TextGlyph *)g.get();
				furiganaWidth += tg.fontGlyph->xOffset + tg.fontGlyph->xAdvance;
			}
			int furiganaStartX = glyphMiddle - (furiganaWidth * 0.4f) / 2;

			for (const auto &g : tr.furigana) {
				addFurigana(g, furiganaStartX);
				const auto &tg = *(TextGlyph *)g.get();
				furiganaStartX += tg.fontGlyph->xAdvance * 0.4f;
			}

			return false;
		}

		return addGlyph(glyph);
	};

	{
		std::lock_guard<std::mutex> lock(textMutex_);
		for (auto &glyph : glyphs_) {
			if (checkGlyph(glyph)) break;
		}
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

void Text::setupGlyphs() {
	std::map<uint16_t, glm::vec4> uvMap;

	int xAdvance = 4, yAdvance = 4;
	int maxLineHeight = 0;

	bool inRuby = false;
	bool inRubyKanji = false;
	bool inRubyBeforeKanji = false;
	bool inRubyAfterKanji = false;

	TextRuby *curRuby = nullptr;

	segments_ = 1;

	glyphs_.clear();
	// 南條　輝正rv19/11900001.「…………また。kv19/11900002.…お酒をbたしな.<嗜>.まれましたな？」
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
			ve->filename = "voice/";
			for (;;) {
				auto c = text_[++i];
				if (c == '.') break;
				ve->filename += c;
			}
			ve->filename += ".at3";
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
			continue;
		} else if (inRubyAfterKanji && c == '.') {
			curRuby = nullptr;
			inRubyAfterKanji = false;
			continue;
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

		// Check if the glyph has already been rendered
		auto iter = uvMap.find(code);
		if (iter != uvMap.end()) {
			tg.uvs = iter->second;
		} else {
			if (tg.fontGlyph->height > maxLineHeight) {
				maxLineHeight = tg.fontGlyph->height;
			}

			if (xAdvance + tg.fontGlyph->width + 4 >= texWidth) {
				xAdvance = 0;
				yAdvance += maxLineHeight + 4;
				maxLineHeight = 0;
			}

			tg.uvs.x = xAdvance;
			tg.uvs.y = yAdvance;
			tg.uvs.z = xAdvance + tg.fontGlyph->width;
			tg.uvs.w = yAdvance + tg.fontGlyph->height;

			xAdvance += tg.fontGlyph->width + 4;

			uvMap.insert({ code, tg.uvs });
		}

		if (curRuby && inRubyKanji) {
			curRuby->glyphs.push_back(std::move(entry));
		} else if (curRuby && inRuby) {
			curRuby->furigana.push_back(std::move(entry));
		} else {
			glyphs_.push_back(std::move(entry));
		}
	}
}

void Text::renderFontTexture() {
	if (fontTex_.id() == 0)
		fontTex_.create(texWidth, texHeight);

	auto addGlyph = [&](const TextGlyph &tg) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
 		fontTex_.subImage(tg.uvs.x, tg.uvs.y, tg.fontGlyph->width, tg.fontGlyph->height, 1, tg.fontGlyph->pixels);
	};

	for (const auto &glyph : glyphs_) {
		if (glyph->type == TextEntryType::Ruby) {
			const auto *tr = (TextRuby *)glyph.get();
			for (const auto &rglyph : tr->glyphs) {
				addGlyph(*(TextGlyph *)rglyph.get());
			}
		} else if (glyph->type == TextEntryType::Glyph) {
			const auto *tg = (TextGlyph *)glyph.get();
			addGlyph(*tg);
		}
	}
	//auto curFB = Framebuffer::getBound();
	//fontTex_.bindDraw();
	
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