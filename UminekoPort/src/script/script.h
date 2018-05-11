#pragma once

#include <iostream>
#include <vector>
#include <atomic>

#include "../engine/graphicscontext.h"
#include "../util/binaryreader.h"
#include "scriptdecompiler.h"

struct ScriptHeader {
	uint32_t fileSize;
	uint32_t unknown;
};

struct SpriteEntry {
	char name[0x18];
	char pose[0x10];
};

struct CgEntry {
	char name[0x18];
	int16_t unknown;
};

enum class ImageType {
	Picture = 2,
	Sprite = 3
};

class Script {
public:
	Script(GraphicsContext &ctx, bool commandTest=false);
	void load(const std::string &path, Archive &archive);
	void pause() {
		if (commandTest_) return;
		paused_ = true;
		while (paused_ && !stopped_) {}
	}
	void resume() {
		paused_ = false;
	}
	void stop() {
		stopped_ = true;
	}

	int version() const {
		return version_;
	}
private:
	friend class ScriptDecompiler;

	int version_ = 0;

	ScriptDecompiler sd_;

	std::atomic<bool> paused_;
	std::atomic<bool> stopped_;
	GraphicsContext &ctx_;
	std::vector<SpriteEntry> sprites_;
	std::vector<CgEntry> cgs_;
	bool commandTest_;

	uint32_t targetOffset_ = 0;
	std::vector<uint32_t> callStack_;

	std::map<int, int16_t> variables_;

	typedef void (Script::*CommandFunction)(BinaryReader &, Archive &);
	std::vector<CommandFunction> commands_;

	void executeCommand(BinaryReader &br, Archive &archive);

	int16_t getVariable(uint16_t value) {
		if (value & 0x8000) {
			return variables_[value & ~0x8000];
		}
		return (int16_t)value;
	}

	void setVariable(uint16_t variable, uint16_t value) {
		if (variable & 0x8000 == 0) {
			throw std::runtime_error("Not a variable: " + std::to_string(variable));
		}
		variables_[variable & ~0x8000] = value;
	}

	void jump(uint32_t offset) {
		targetOffset_ = offset;
	}

	void command41(BinaryReader &br, Archive &archive) {
		auto operation = br.read<uint8_t>();
		auto variable = br.read<uint16_t>();
		auto value = getVariable(br.read<uint16_t>());
		if (operation & 0x80) {
			// Two operands?
			auto secondValue = br.read<uint16_t>();
			return; // for now
		}
		switch (operation) {
		case 0: // ???
			break;
		case 1: // set?
			setVariable(variable, value);
			break;
		case 2: // add?
			setVariable(variable, getVariable(variable) + value);
			break;
		default:
			throw std::runtime_error("Unhandled operation.");
		}
	}
	void command42(BinaryReader &br, Archive &archive) {
		br.skip(14);
	}
	void command46(BinaryReader &br, Archive &archive) {
		auto operation = br.read<uint8_t>();
		auto value = getVariable(br.read<uint16_t>());
		auto compareTo = getVariable(br.read<uint16_t>());
		auto offset = br.read<uint32_t>();

		switch (operation) {
		case 0: // equal to (confirmed!)
			if (value == compareTo) {
				jump(offset);
			}
			break;
		case 1: // Not equal to (confirmed!)
			if (value != compareTo) {
				jump(offset);
			}
			break;
		case 2: // greater or equal (confirmed!)
			if (value >= compareTo) {
				jump(offset);
			}
			break;
		case 5: // less than (confirmed!)
			if (value < compareTo) {
				jump(offset);
			}
			break;
		}
	}
	void command47(BinaryReader &br, Archive &archive) {
		auto offset = br.read<uint32_t>();
		jump(offset);
	}
	void command48(BinaryReader &br, Archive &archive) {
		auto offset = br.read<uint32_t>();
		callStack_.push_back(br.tellg());
		jump(offset);
	}
	void command49(BinaryReader &br, Archive &archive) {
		if (callStack_.size() == 0) {
			throw std::runtime_error("Cannot return without having called a function first.");
		}
		targetOffset_ = callStack_.back();
		callStack_.pop_back();
	}

	// branch_on_variable
	//  Checks the value of the first argument and jumps to the corresponding address in the argument list
	void command4A(BinaryReader &br, Archive &archive) {
		auto value = getVariable(br.read<uint16_t>());
		auto count = br.read<uint16_t>();
		std::vector<uint32_t> offsets;
		offsets.reserve(count);
		for (int i = 0; i < count; ++i) {
			offsets.push_back(br.read<uint32_t>());
		}
		if (value < 0 || value >= count)
			throw std::runtime_error("Value out of range in branch_on_variable (is this an error?).");
		else
			jump(offsets[value]);
	}
	void command4D(BinaryReader &br, Archive &archive) {
		br.skip(3);
	}
	void command4E(BinaryReader &br, Archive &archive) {
		auto unk = br.read<uint8_t>();
		if (unk == 1)
			br.skip(2);
		else if (unk == 4)
			br.skip(3);
		else
			br.skip(0);
	}
	void command80(BinaryReader &br, Archive &archive) {
		br.skip(3);
		auto count = br.read<uint8_t>() & ~0x80;
		br.skip(2 * count); // ???
	}
	void command83(BinaryReader &br, Archive &archive) {
		//br.skip(2);
		auto unk1 = br.read<uint8_t>();
		auto unk2 = br.read<uint8_t>();
		if (unk2 != 0x00)
			br.skip(0);
	}
	void command86(BinaryReader &br, Archive &archive) {
		br.skip(4);
		auto length = br.read<uint16_t>();
		auto text = br.readString(length - 1);
		//std::cout << "86 Text: \"" << text << "\"\n";
		br.skip(1);
		ctx_.pushMessage(text);
		pause();
	}
	void command87(BinaryReader &br, Archive &archive) {
		auto unk = br.read<int16_t>(); // ???
	}
	void command88(BinaryReader &br, Archive &archive) {
	}
	void command89(BinaryReader &br, Archive &archive) {
		//br.skip(3);
	}
	void command8C(BinaryReader &br, Archive &archive) {
		br.skip(5);
		br.skip(3); // ???
		auto headerSize = br.read<uint8_t>();
		auto header = br.readString(headerSize - 1);
		br.skip(1);
		auto contentSize = br.read<uint8_t>();
		auto content = br.readString(contentSize - 1);
		br.skip(1);
	}
	void command8D(BinaryReader &br, Archive &archive) {
		auto next = br.read<uint8_t>();
		next &= ~0x80;
		if (next == 0x02)
			br.skip(2);
		else if (next == 0x03)
			br.skip(4);
		else if (next == 0x0E)
			br.skip(6);
	}
	void command9C(BinaryReader &br, Archive &archive) {
		//br.skip(4);
		auto unk = br.read<uint32_t>();
		/*if (unk == 0x05) {
			br.skip(6);
		} else if (unk == 0x03 || unk == 0x04 || unk == 0x0e) {
			br.skip(7);
		} else {
			br.skip(0);
		}*/
		auto unk2 = br.read<uint32_t>(); // B4 00 00 00 - Command Byte?
	}
	void command9D(BinaryReader &br, Archive &archive) {
		br.skip(2);
	}
	void commandA0(BinaryReader &br, Archive &archive) {
		br.skip(10);
	}
	void commandA1(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}
	void commandA2(BinaryReader &br, Archive &archive) {
		br.skip(2);
	}
	void commandA3(BinaryReader &br, Archive &archive) {
		br.skip(6);
	}
	void commandA4(BinaryReader &br, Archive &archive) {
		br.skip(7); // Japanese character used in here?
	}
	void commandA6(BinaryReader &br, Archive &archive) {
		br.skip(7); // ???
	}
	void commandB0(BinaryReader &br, Archive &archive) {
		br.skip(2);
		auto length = br.read<uint8_t>();
		auto text = br.readString(length - 1);
		br.skip(1);
		//std::cout << "B0 Text: \"" << text << "\"\n";
	}
	void commandB1(BinaryReader &br, Archive &archive) {
		br.skip(3);
	}
	void commandB3(BinaryReader &br, Archive &archive) {
		// ???
	}
	void commandB4(BinaryReader &br, Archive &archive) {
		//br.skip(4);
	}
	void commandB6(BinaryReader &br, Archive &archive) {
		// ???
	}
	void commandBE(BinaryReader &br, Archive &archive) {
		br.skip(2);
	}
	void commandBF(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}
	void displayImage(BinaryReader &br, Archive &archive);
	void commandC2(BinaryReader &br, Archive &archive);
	void commandC3(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}
	void commandC9(BinaryReader &br, Archive &archive) {
		// ???
	}
	void commandCA(BinaryReader &br, Archive &archive) {
		br.skip(2);
		auto unk = br.read<uint8_t>();
		if (unk == 0x00) {
			// ...
		} else if (unk == 0x01) {
			br.skip(2);
		} else if (unk == 0x03) {
			br.skip(4);
		} else if (unk == 0x06) {
			br.skip(4);
		} else if (unk == 0x07) {
			br.skip(6);
		} else {
			br.skip(0);
		}
	}
	void commandCB(BinaryReader &br, Archive &archive) {
		br.skip(3);
	}
};