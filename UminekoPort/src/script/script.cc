#include "script.h"

#include <iostream>
#include <iomanip>

#include "../engine/engine.h"
#include "../util/binaryreader.h"
#include "../audio/audio.h"

#include "scriptdecompiler.h"

Script::Script(GraphicsContext &ctx, AudioManager &audio, bool commandTest) : ctx_(ctx), audio_(audio), commandTest_(commandTest), sd_(*this) {
	commands_.resize(0x100, nullptr);
	commands_[0x41] = &Script::command41;
	commands_[0x42] = &Script::command42;
	commands_[0x46] = &Script::command46;
	commands_[0x47] = &Script::command47;
	commands_[0x48] = &Script::command48;
	commands_[0x49] = &Script::command49;
	commands_[0x4A] = &Script::command4A;
	commands_[0x4D] = &Script::command4D;
	commands_[0x4E] = &Script::command4E;
	commands_[0x80] = &Script::command80;
	commands_[0x83] = &Script::command83;
	commands_[0x85] = &Script::command85;
	commands_[0x86] = &Script::command86;
	commands_[0x87] = &Script::command87;
	commands_[0x88] = &Script::command88;
	commands_[0x89] = &Script::command89;
	commands_[0x8C] = &Script::command8C;
	commands_[0x8D] = &Script::command8D;
	commands_[0x9C] = &Script::command9C;
	commands_[0x9D] = &Script::command9D;
	commands_[0x9E] = &Script::command9E;
	if (Engine::game == "umi" || Engine::game == "chiru")
		commands_[0xA0] = &Script::commandA0_umi;
	else
		commands_[0xA0] = &Script::commandA0_higu;
	commands_[0xA1] = &Script::commandA1;
	commands_[0xA2] = &Script::commandA2;
	commands_[0xA3] = &Script::commandA3;
	commands_[0xA4] = &Script::commandA4;
	commands_[0xA6] = &Script::commandA6;
	commands_[0xB0] = &Script::commandB0;
	commands_[0xB1] = &Script::commandB1;
	commands_[0xB3] = &Script::commandB3;
	commands_[0xB4] = &Script::commandB4;
	commands_[0xB6] = &Script::commandB6;
	commands_[0xBE] = &Script::commandBE;
	commands_[0xBF] = &Script::commandBF;
	commands_[0xC1] = &Script::displayImage;
	commands_[0xC2] = &Script::commandC2;
	commands_[0xC3] = &Script::commandC3;
	commands_[0xC9] = &Script::commandC9;
	commands_[0xCA] = &Script::commandCA;
	commands_[0xCB] = &Script::commandCB;
}

void Script::load(const std::string &path, Archive &archive) {
	path_ = path;
	data_ = archive.read(path_);

	std::ofstream ofs(path_, std::ios_base::binary);
	ofs.write((char *)data_.data(), data_.size());
	ofs.close();

	BinaryReader br((char *)data_.data(), data_.size());
	auto magic = br.readString(4);
	if (magic != "SNR ") {
		throw std::runtime_error("Script file has invalid signature, expected 'SNR '.");
	}
	std::cout << "Loading script...\n";
	auto fileSize = br.read<uint32_t>();
	auto unknown1 = br.read<uint32_t>();
	auto unknown2 = br.read<uint32_t>();
	auto unknown3 = br.read<uint32_t>();
	br.skip(12);

	version_ = unknown2;

	scriptOffset_ = br.read<uint32_t>();

	// Resource offsets
	auto maskOffset = br.read<uint32_t>();
	auto cgOffset = br.read<uint32_t>();
	auto spriteOffset = br.read<uint32_t>();
	auto animeOffset = br.read<uint32_t>(); // ???
	auto bgmOffset = br.read<uint32_t>();
	auto seOffset = br.read<uint32_t>();
	auto movieOffset = br.read<uint32_t>(); // ???
	auto voiceOffset = br.read<uint32_t>();
	auto unknown1Offset = br.read<uint32_t>();
	auto unknown2Offset = br.read<uint32_t>();
	auto textOffset = br.read<uint32_t>();
	auto unknown3Offset = br.read<uint32_t>();
	auto characterProfileOffset = br.read<uint32_t>();

	br.seekg(maskOffset);
	auto maskCount = br.read<uint32_t>();
	masks_.resize(maskCount);
	br.read((char *)masks_.data(), maskCount * sizeof(MaskEntry));

	br.seekg(spriteOffset);
	auto spriteCount = br.read<uint32_t>();
	sprites_.resize(spriteCount);
	br.read((char *)sprites_.data(), spriteCount * sizeof(SpriteEntry));

	br.seekg(cgOffset);
	auto cgCount = br.read<uint32_t>();
	cgs_.resize(cgCount);
	br.read((char *)cgs_.data(), cgCount * sizeof(CgEntry));

	br.seekg(animeOffset);
	auto animCount = br.read<uint32_t>();
	if (Engine::game == "umi") {
		umiAnims_.resize(animCount);
		br.read((char *)umiAnims_.data(), animCount * sizeof(UmiAnimEntry));
	} else if (Engine::game == "chiru") {
		chiruAnims_.resize(animCount);
		br.read((char *)chiruAnims_.data(), animCount * sizeof(ChiruAnimEntry));
	}

	br.seekg(bgmOffset);
	auto bgmCount = br.read<uint32_t>();
	bgms_.resize(bgmCount);
	br.read((char *)bgms_.data(), bgmCount * sizeof(BGMEntry));

	br.seekg(seOffset);
	auto seCount = br.read<uint32_t>();
	ses_.resize(seCount);
	br.read((char *)ses_.data(), seCount * sizeof(SEEntry));

	/*for (const auto &s : sprites_) {
		std::cout << s.name << "\n";
	}*/

	sd_.setup();
	//sd_.decompile(path, data, scriptOffset);

	br.seekg(scriptOffset_);
	while (!stopped_) {
		executeCommand(br, archive);
	}
}

void Script::executeCommand(BinaryReader &br, Archive &archive) {
	if (targetOffset_) {
		br.seekg(targetOffset_);
		targetOffset_ = 0;
	}
	auto line = sd_.getFunctionLine(br);
	auto cmd = br.read<uint8_t>();
	std::cout << '(' << std::hex << std::setw(2) << std::setfill('0') << (int)cmd << std::dec << ')' << line << '\n';
	CommandFunction cf = commands_[cmd];
	uint64_t curPos = br.tellg();
	if (!cf) {
		br.seekg(curPos - 0x30);
		unsigned char buffer[0x60];
		br.read((char *)buffer, 0x60);
		std::cerr << std::hex;
		for (int i = 0; i < 0x6; ++i) {
			for (int j = 0; j < 0x10; ++j) {
				std::cerr << std::setw(2) << std::setfill('0') << (int)buffer[i * 0x10 + j] << " ";
			}
			std::cerr << "\n";
		}
		std::cerr << std::dec << std::setw(1) << std::setfill(' ');
		std::cerr << "Unrecognized Command Byte 0x" << std::hex << (int)cmd << std::dec << " at 0x" << std::hex << (curPos - 1) << std::dec << ".\n";
		std::cerr << "";
	}
	//std::cout << "CB 0x" << std::hex << (int)cmd << std::dec << "\n";
	(this->*cf)(br, archive);
}

void Script::command8D(BinaryReader &br, Archive &archive) {
	uint8_t unknown = 0, unknown2 = 0;
	if (Engine::game == "chiru") {
		unknown = br.read<uint8_t>();
		if (unknown != 0)
			unknown2 = br.read<uint8_t>();
	}
	auto next = br.read<uint8_t>();
	next &= ~0x80;
	if (next == 0x02) { // fade
		auto frames = getVariable(br.read<uint16_t>());
		ctx_.transition(frames);
		pause();
	} else if (next == 0x03) { // mask
		auto maskId = getVariable(br.read<uint16_t>());
		auto frames = getVariable(br.read<uint16_t>());
		ctx_.transition("mask/" + std::string(masks_[maskId].name) + ".msk", frames);
		pause();
	} else if (next == 0x0C)
		br.skip(4);
	else if (next == 0x0E)
		br.skip(6);

	if (unknown != 0)
		br.skip(2);
	ctx_.applyLayers();
}

void Script::command9C(BinaryReader &br, Archive &archive) {
	//br.skip(4);
	auto bgmId = getVariable(br.read<uint16_t>());
	auto unk1 = br.read<uint16_t>();
	/*if (unk == 0x05) {
	br.skip(6);
	} else if (unk == 0x03 || unk == 0x04 || unk == 0x0e) {
	br.skip(7);
	} else {
	br.skip(0);
	}*/
	auto volume = br.read<uint32_t>(); // B4 00 00 00 - volume?

	audio_.playBGM("bgm/" + std::string(bgms_[bgmId].name) + ".at3", volume / 255.0f);
}

void Script::commandA0_umi(BinaryReader &br, Archive &archive) {
	auto channel = br.read<uint16_t>();
	auto seId = br.read<uint16_t>();
	auto unk = br.read<uint16_t>();
	auto volume = br.read<uint32_t>();

	audio_.playSE(channel, "se/" + std::string(ses_[seId].name) + ".at3", volume / 255.0f);
}

void Script::displayImage(BinaryReader &br, Archive &archive) {
	auto layer = br.read<uint16_t>(); // Layer?
	auto type = (ImageType)br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();
	if (unk3 == 0) {
		ctx_.clearLayer(layer);
		return;
	}
	if (unk3 == 0x2D) {
		br.skip(8);
	}
	uint32_t spriteId = 0;
	if (unk3 == 1) {
		spriteId = br.read<uint16_t>();
	}
	//std::cout << "[C1: " << (int)layer << "|" << (int)type << "|" << (int)unk3 << "]\n";
	//if (layer == 0x08 || layer == 0x09 || layer == 0x0a) {
	if (type == ImageType::Sprite) {
		std::cout << "Displaying sprite " << sprites_[spriteId].name << "_" << sprites_[spriteId].pose << ".\n";
		/*Texture texture;
		texture.load(, archive);*/
		auto texturePath = "bustup/" + std::string(sprites_[spriteId].name) + ".bup";
		ctx_.setLayerBup(layer, sprites_[spriteId].name, sprites_[spriteId].pose);
		//pause();
	//} else if (layer == 0x01 || layer == 0x02 || layer == 0x03) {
	} else if (type == ImageType::Picture) {
		std::cout << "Displaying CG(?) " << cgs_[spriteId].name << ". (" << std::hex << br.tellg() << std::dec << ")\n";
		auto texturePath = "picture/" + std::string(cgs_[spriteId].name) + ".pic";
		ctx_.setLayer(layer, texturePath);
		//pause();
	} else {
		std::cerr << "Unknown image type " << (int)type << " in display_image.\n";
	}
}

void Script::commandC2(BinaryReader &br, Archive &archive) {
	auto layer = getVariable(br.read<uint16_t>());
	auto prop = br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();
	if (unk3 == 0x0) {
		// ...
	} else if (unk3 == 0x01) {
		auto props = ctx_.layerProperties(layer);
		auto value = getVariable(br.read<uint16_t>());
		switch (prop) {
		case 1:
			props.sprite.color.a = (value & 0xff) / 255.0f;
			break;
		case 2:
			props.sprite.color.r = (value & 0xff) / 255.0f;
			break;
		case 3:
			props.sprite.color.g = (value & 0xff) / 255.0f;
			break;
		case 4:
			props.sprite.color.b = (value & 0xff) / 255.0f;
			break;
		case 5:
			props.filter = (GraphicsLayerFilter::Flags)(value & 0xf);
			break;
		case 6:
			if (value > 2) {
				throw std::runtime_error("Unhandled blend mode, investigate.");
			}
			props.blendMode = (GraphicsLayerBlendMode)value;
			break;
		case 7:
			props.offset.x = value;
			break;
		case 8:
			props.offset.y = value;
		default: // to be implemented
			break;
		}
		ctx_.setLayerProperties(layer, props);
	} else if (unk3 == 0x06) {
		br.skip(4);
	} else if (unk3 == 0x07) {
		br.skip(6);
	} else {
		//br.skip(2);
	}
}