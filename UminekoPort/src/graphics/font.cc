#include "font.h"

#include <iostream>
#include <iomanip>

#include <GL/glew.h>

#include "../data/archive.h"
#include "../data/compression.h"
#include "../util/binaryreader.h"
#include "../data/vertexbuffer.h"
#include "../graphics/shader.h"
#include "../graphics/uniformbuffer.h"
#include "../math/time.h"

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

		glyph.compressedSize = *(uint16_t *)readPtr;
		readPtr += 2;

		glyph.pixels.resize(glyph.width * glyph.height);

		if (glyph.compressedSize == 0)
			return glyph;

		auto expandNibble = [](int nibble) -> uint8_t {
			return static_cast<uint8_t>(((nibble << 4) & 0xf0) | (nibble & 0xf));
		};

		auto modWidth = (glyph.width % 2 == 0) ? glyph.width : (glyph.width + 1);
		auto literals = DataCompression::decompress10_6(readPtr, glyph.compressedSize, modWidth * glyph.height / 2);

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
	progress_ = 0.0f;

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
	progress_ = 0.0f;
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

void Text::update() {
	progress_ += Time::deltaTime();
	if (progress_ >= 1.0f)
		progress_ = 1.0f;
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

	auto &textData = UniformBuffer::uniformBuffer<ShaderTextData>("text");
	textData->progress.x = progress_;
	textData.update();

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	struct GlyphVertices {
		struct GlyphVertex {
			glm::vec2 pos;
			glm::vec2 uv;
			glm::vec4 color;
			float fadein;
		} vert[6];
	};

	std::vector<GlyphVertices> verts;
	
	float xAdvance = 0.0f, yAdvance = 0.0f;
	//for (int i = 0; i < text_.size(); ++i) {

	auto setupVertices = [&](float baseX, float baseY, const glm::vec4 &uvs, const Glyph &fg, float sizeMod, const glm::vec4 &color, float fadeinLeft, float fadeinRight) {
		GlyphVertices gv;
		gv.vert[0].pos.x = baseX;
		gv.vert[0].pos.y = baseY;
		gv.vert[0].uv.x = uvs.x;
		gv.vert[0].uv.y = uvs.y;
		gv.vert[0].color = color;
		gv.vert[0].fadein = fadeinLeft;

		gv.vert[1].pos.x = baseX;
		gv.vert[1].pos.y = baseY + fg.height * sizeMod;
		gv.vert[1].uv.x = uvs.x;
		gv.vert[1].uv.y = uvs.w;
		gv.vert[1].color = color;
		gv.vert[1].fadein = fadeinLeft;

		gv.vert[2].pos.x = baseX + fg.width * sizeMod;
		gv.vert[2].pos.y = baseY;
		gv.vert[2].uv.x = uvs.z;
		gv.vert[2].uv.y = uvs.y;
		gv.vert[2].color = color;
		gv.vert[2].fadein = fadeinRight;

		gv.vert[3] = gv.vert[2];

		gv.vert[4] = gv.vert[1];

		gv.vert[5].pos.x = baseX + fg.width * sizeMod;
		gv.vert[5].pos.y = baseY + fg.height * sizeMod;
		gv.vert[5].uv.x = uvs.z;
		gv.vert[5].uv.y = uvs.w;
		gv.vert[5].color = color;
		gv.vert[5].fadein = fadeinRight;

		return gv;
	};

	auto addGlyph = [&](const std::unique_ptr<TextEntry> &glyph, float fadeinLeft, float fadeinRight) {
		if (glyph->type != TextEntryType::Glyph) return false;

		if (wrapWidth_ > 0 && xAdvance >= wrapWidth_) {
			yAdvance += 80.0f;
			xAdvance = 0;
		}

		const auto &tg = *(TextGlyph *)glyph.get();
		const auto &fg = *tg.fontGlyph;

		float baseX = transform_.position.x + xAdvance + fg.xOffset;
		float baseY = transform_.position.y + yAdvance + fg.yOffset;

		for (int y = -1; y < 2; ++y) {
			for (int x = -1; x < 2; ++x) {
				GlyphVertices gv = setupVertices(baseX + x * 2.0f, baseY + y * 2.0f, tg.uvs, fg, 1.0f, glm::vec4(0, 0, 0, 1), fadeinLeft, fadeinRight);
				verts.push_back(std::move(gv));
			}
		}

		GlyphVertices gv = setupVertices(baseX, baseY, tg.uvs, fg, 1.0f, glm::vec4(1), fadeinLeft, fadeinRight);
		verts.push_back(std::move(gv));

		xAdvance += fg.xAdvance;
		//yAdvance += fg.yAdvance;

		return false;
	};

	auto addFurigana = [&](const std::unique_ptr<TextEntry> &glyph, int xStart, float fadeinLeft, float fadeinRight) {
		if (glyph->type != TextEntryType::Glyph) return false;

		const auto &tg = *(TextGlyph *)glyph.get();
		const auto &fg = *tg.fontGlyph;

		auto baseX = transform_.position.x + xStart + fg.xOffset;
		auto baseY = transform_.position.y + yAdvance + fg.yOffset - 80.0f;

		for (int y = -1; y < 2; ++y) {
			for (int x = -1; x < 2; ++x) {
				GlyphVertices gv = setupVertices(baseX + x * 0.8f, baseY + y * 0.8f, tg.uvs, fg, 0.4f, glm::vec4(0, 0, 0, 1), fadeinLeft, fadeinRight);
				verts.push_back(std::move(gv));
			}
		}

		GlyphVertices gv = setupVertices(baseX, baseY, tg.uvs, fg, 0.4f, glm::vec4(1), fadeinLeft, fadeinRight);
		verts.push_back(std::move(gv));

		return false;
	};

	int currentSegmentGlyphCount = 0;
	int pushKeyCount = 0;
	{
		std::lock_guard<std::mutex> lock(textMutex_);
		for (auto &glyph : glyphs_) {
			if (glyph->type == TextEntryType::PushKey) {
				if (pushKeyCount > currentSegment_) {
					break;
				}
				++pushKeyCount;
			}
			if (pushKeyCount == currentSegment_) {
				if (glyph->type == TextEntryType::Glyph)
					++currentSegmentGlyphCount;
				else if (glyph->type == TextEntryType::Ruby) {
					for (auto &rg : ((TextRuby *)glyph.get())->glyphs) {
						++currentSegmentGlyphCount;
					}
				}
			}
		}
	}

	int currentSegmentGlyphIndex = 0;
	pushKeyCount = 0;
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
			float startX = xAdvance;
			for (const auto &g : tr.glyphs) {
				if (pushKeyCount < currentSegment_) {
					addGlyph(g, 0.0f, 0.0f);
				} else {
					float fadeinLeft = (currentSegmentGlyphIndex - 1) / (float)currentSegmentGlyphCount;
					float fadeinRight = (currentSegmentGlyphIndex) / (float)currentSegmentGlyphCount;
					addGlyph(g, fadeinLeft, fadeinRight);
					++currentSegmentGlyphIndex;
				}
			}
			float endX = xAdvance;
			if (tr.furigana.size() == 0) return false; // Weird

			float glyphWidth = endX - startX;
			float glyphMiddle = startX + glyphWidth / 2.0f;

			float furiganaWidth = 0;
			for (const auto &g : tr.furigana) {
				const auto &tg = *(TextGlyph *)g.get();
				furiganaWidth += tg.fontGlyph->xOffset + tg.fontGlyph->xAdvance;
			}
			float furiganaStartX = glyphMiddle - (furiganaWidth * 0.4f) / 2;

			for (const auto &g : tr.furigana) {
				if (pushKeyCount < currentSegment_) {
					addFurigana(g, furiganaStartX, 0.0f, 0.0f);
				} else {
					float fadeinLeft = (currentSegmentGlyphIndex - 1) / (float)currentSegmentGlyphCount;
					float fadeinRight = (currentSegmentGlyphIndex) / (float)currentSegmentGlyphCount;
					addFurigana(g, furiganaStartX, fadeinLeft, fadeinRight);
				}
				const auto &tg = *(TextGlyph *)g.get();
				furiganaStartX += tg.fontGlyph->xAdvance * 0.4f;
			}

			return false;
		}

		if (pushKeyCount < currentSegment_) {
			return addGlyph(glyph, 0.0f, 0.0f);
		}
		float fadeinLeft = (currentSegmentGlyphIndex - 1) / (float)currentSegmentGlyphCount;
		float fadeinRight = (currentSegmentGlyphIndex) / (float)currentSegmentGlyphCount;
		bool result = addGlyph(glyph, fadeinLeft, fadeinRight);
		++currentSegmentGlyphIndex;
		return result;
	};

	{
		std::lock_guard<std::mutex> lock(textMutex_);
		for (auto &glyph : glyphs_) {
			if (checkGlyph(glyph)) break;
		}
	}

	VertexBuffer<float> vb;
	vb.upload(verts.data(), verts.size() * 9 * 6);
	vb.bind();
	vb.setAttribute(0, 2, 9, 0);
	vb.setAttribute(1, 2, 9, 2);
	vb.setAttribute(2, 4, 9, 4);
	vb.setAttribute(3, 1, 9, 8);

	glActiveTexture(GL_TEXTURE0);
	fontTex_.bind();

	glDepthMask(GL_FALSE);

	vb.draw(Primitives::Triangles, 0, verts.size() * 6);

	glDepthMask(GL_TRUE);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

void Text::setupGlyphs() {
	std::map<uint16_t, glm::vec4> uvMap;

	const int spacing = 8;
	int xAdvance = spacing, yAdvance = spacing;
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

			if (xAdvance + tg.fontGlyph->width + spacing >= texWidth) {
				xAdvance = 0;
				yAdvance += maxLineHeight + spacing;
				maxLineHeight = 0;
			}

			tg.uvs.x = xAdvance;
			tg.uvs.y = yAdvance;
			tg.uvs.z = xAdvance + tg.fontGlyph->width;
			tg.uvs.w = yAdvance + tg.fontGlyph->height;

			xAdvance += tg.fontGlyph->width + spacing;

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
	std::lock_guard<std::mutex> lock(textMutex_);
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