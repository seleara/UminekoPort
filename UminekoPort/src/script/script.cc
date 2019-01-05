#include "script.h"

#include <iostream>
#include <iomanip>

#include "../engine/engine.h"
#include "../util/binaryreader.h"
#include "../audio/audio.h"

#include "scriptimpl.h"
#include "umiscript.h"
#include "chiruscript.h"
#include "higuscript.h"
#include "scriptdecompiler.h"

Script::Script(GraphicsContext &ctx, AudioManager &audio, bool commandTest) : ctx_(ctx), audio_(audio), commandTest_(commandTest), sd_(*this) {}

Script::~Script() {}

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

	switch (version_) {
	case 0x01:
		if (Engine::game == "umi")
			impl_ = std::make_unique<UmiScript>(*this);
		else
			impl_ = std::make_unique<ChiruScript>(*this);
		break;
	case 0x41:
		impl_ = std::make_unique<HiguScript>(*this);
		break;
	default:
		std::cerr << "Warning: Script version 0x" << std::hex << version_ << std::dec << " not recognized. Setting up commands for version 0x01, but they will probably be incorrect.";
		impl_ = std::make_unique<UmiScript>(*this);
		break;
	}

	impl_->setupCommands();
	impl_->load(br);

	sd_.setup();
	//sd_.decompile(path, data, scriptOffset);
	//decompile();

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
	CommandFunction cf = impl_->commands_[cmd];
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
	cf(br, archive);
}

MaskEntry Script::getMask(uint32_t id) {
	return impl_->masks_[id];
}

CgEntry Script::getCg(uint32_t id) {
	return impl_->cgs_[id];
}

SpriteEntry Script::getSprite(uint32_t id) {
	return impl_->sprites_[id];
}

AnimEntry Script::getAnim(uint32_t id) {
	if (version_ != 0x01)
		throw std::runtime_error("Script does not have animations.");
	return ((UmiScript *)impl_.get())->anims_[id];
}

BGMEntry Script::getBgm(uint32_t id) {
	return impl_->bgms_[id];
}

SEEntry Script::getSe(uint32_t id) {
	return impl_->ses_[id];
}

void Script::setVariable(uint8_t operation, uint16_t variable, uint16_t value) {
	auto op1 = getVariable(variable);
	auto op2 = getVariable(value);
	switch (operation) {
	case 0: // set (confirmed?)
		setVariable(variable, op2);
		break;
	case 1: // same as 0? appears to be set
		setVariable(variable, op2); // ???
		break;
	case 2: // add (confirmed?)
		setVariable(variable, op1 + op2);
		break;
	case 3: // subtract (confirmed?)
		setVariable(variable, op1 - op2);
		break;
	case 4: // multiply (confirmed?)
		setVariable(variable, op1 * op2);
		break;
	case 5: // divide (confirmed?)
		setVariable(variable, op1 / op2);
		break;
	default:
		throw std::runtime_error("Unhandled operation.");
	}
}

void Script::setVariable(uint8_t operation, uint16_t variable, uint16_t left, uint16_t right) {
	auto op1 = getVariable(left);
	auto op2 = getVariable(right);
	switch (operation) {
	case 2: // add (confirmed?)
		setVariable(variable, op1 + op2);
		break;
	case 3: // subtract (confirmed?)
		setVariable(variable, op1 - op2);
		break;
	case 4: // multiply (confirmed?)
		setVariable(variable, op1 * op2);
		break;
	case 5: // divide (confirmed?)
		setVariable(variable, op1 / op2);
		break;
	default:
		throw std::runtime_error("Unhandled operation.");
	}
}