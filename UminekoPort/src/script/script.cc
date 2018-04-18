#include "script.h"

#include <iostream>
#include <iomanip>

#include "../util/binaryreader.h"

Script::Script(GraphicsContext &ctx, bool commandTest) : ctx_(ctx), commandTest_(commandTest) {
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
	commands_[0x86] = &Script::command86;
	commands_[0x87] = &Script::command87;
	commands_[0x88] = &Script::command88;
	commands_[0x89] = &Script::command89;
	commands_[0x8C] = &Script::command8C;
	commands_[0x8D] = &Script::command8D;
	commands_[0x9C] = &Script::command9C;
	commands_[0xA0] = &Script::commandA0;
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
	auto data = archive.read(path);
	BinaryReader br((char *)&data[0], data.size());
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

	auto scriptOffset = br.read<uint32_t>();

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

	br.seekg(spriteOffset);
	auto spriteCount = br.read<uint32_t>();
	sprites_.resize(spriteCount);
	br.read((char *)&sprites_[0], spriteCount * sizeof(SpriteEntry));

	br.seekg(cgOffset);
	auto cgCount = br.read<uint32_t>();
	cgs_.resize(cgCount);
	br.read((char *)&cgs_[0], cgCount * sizeof(CgEntry));

	/*for (const auto &s : sprites_) {
		std::cout << s.name << "\n";
	}*/

	br.seekg(scriptOffset);
	while (!stopped_) {
		executeCommand(br, archive);
	}
}

void Script::executeCommand(BinaryReader &br, Archive &archive) {
	auto cmd = br.read<uint8_t>();
	CommandFunction cf = commands_[cmd];
	uint64_t curPos = br.tellg();
	if (!cf) {
		br.seekg(curPos - 0x30);
		unsigned char *buffer = new unsigned char[0x60];
		br.read((char *)buffer, 0x60);
		std::cerr << std::hex;
		for (int i = 0; i < 0x6; ++i) {
			for (int j = 0; j < 0x10; ++j) {
				std::cerr << std::setw(2) << std::setfill('0') << (int)buffer[i * 0x10 + j] << " ";
			}
			std::cerr << "\n";
		}
		std::cerr << std::dec << std::setw(1) << std::setfill(' ');
		delete[] buffer;
		std::cerr << "Unrecognized Command Byte 0x" << std::hex << (int)cmd << std::dec << " at 0x" << std::hex << (curPos - 1) << std::dec << ".\n";
		std::cerr << "";
	}
	std::cout << "CB 0x" << std::hex << (int)cmd << std::dec << "\n";
	(this->*cf)(br, archive);
}

void Script::displayImage(BinaryReader &br, Archive &archive) {
	auto unk1 = br.read<uint16_t>(); // Layer?
	auto unk2 = br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();
	if (unk3 == 0) {
		ctx_.clearLayer(unk1);
		return;
	}
	if (unk3 == 0x2D) {
		br.skip(8);
	}
	uint32_t spriteId = 0;
	if (unk3 == 1) {
		spriteId = br.read<uint16_t>();
	}
	std::cout << "[C1: " << (int)unk1 << "|" << (int)unk2 << "|" << (int)unk3 << "]\n";
	if (unk1 == 0x08 || unk1 == 0x09 || unk1 == 0x0a) {
		std::cout << "Displaying sprite " << sprites_[spriteId].name << "_" << sprites_[spriteId].pose << ".\n";
		/*Texture texture;
		texture.load(, archive);*/
		auto texturePath = "bustup/" + std::string(sprites_[spriteId].name) + ".bup";
		ctx_.setLayerBup(unk1, sprites_[spriteId].name, sprites_[spriteId].pose);
		//pause();
	} else if (unk1 == 0x01 || unk1 == 0x02 || unk1 == 0x03) {
		std::cout << "Displaying CG(?) " << cgs_[spriteId].name << ". (" << std::hex << br.tellg() << std::dec << ")\n";
		auto texturePath = "picture/" + std::string(cgs_[spriteId].name) + ".pic";
		ctx_.setLayer(unk1, texturePath);
		//pause();
	} else {
		std::cerr << "Unknown unk1 " << (int)unk1 << " in C1.\n";
	}
}

void Script::commandC2(BinaryReader &br, Archive &archive) {
	auto layer = br.read<uint16_t>();
	auto unk2 = br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();
	if (unk3 == 0x0) {
		// ...
	} else if (unk3 == 0x06) {
		br.skip(4);
	} else if (unk3 == 0x07) {
		br.skip(6);
	} else {
		br.skip(2);
	}
}