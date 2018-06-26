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

struct MaskEntry {
	char name[0xC];
};

struct SpriteEntry {
	char name[0x18];
	char pose[0x10];
};

struct CgEntry {
	char name[0x18];
	int16_t unknown;
};

struct UmiAnimEntry {
	char name[0x20];
	int16_t unknown;
	int16_t unknown2;
};

struct ChiruAnimEntry {
	char name[0x24];
	int16_t unknown;
	int16_t unknown2;
};

struct BGMEntry {
	char name[0xC];
	char title[0x28];
};

struct SEEntry {
	char name[0x18];
};

enum class ImageType {
	Picture = 2,
	Sprite = 3
};

class AudioManager;

class Script {
public:
	Script(GraphicsContext &ctx, AudioManager &audio, bool commandTest=false);
	void load(const std::string &path, Archive &archive);
	void pause() {
		if (commandTest_) return;
		paused_ = true;

		std::unique_lock<std::mutex> lock(pauseMutex_);
		cv_.wait(lock, [&]() { return !paused_ || stopped_; });
	}
	void resume() {
		paused_ = false;
		cv_.notify_one();
	}
	void stop() {
		stopped_ = true;
		cv_.notify_one();
	}

	int version() const {
		return version_;
	}

	void decompile() {
		sd_.decompile(path_, data_, scriptOffset_);
	}
private:
	friend class ScriptDecompiler;

	std::mutex pauseMutex_;
	std::condition_variable cv_;

	std::string path_;
	std::vector<unsigned char> data_;
	int version_ = 0;
	uint32_t scriptOffset_ = 0;

	ScriptDecompiler sd_;

	std::atomic<bool> paused_;
	std::atomic<bool> stopped_;
	GraphicsContext &ctx_;
	AudioManager &audio_;
	std::vector<MaskEntry> masks_;
	std::vector<SpriteEntry> sprites_;
	std::vector<CgEntry> cgs_;
	std::vector<UmiAnimEntry> umiAnims_;
	std::vector<ChiruAnimEntry> chiruAnims_;
	std::vector<BGMEntry> bgms_;
	std::vector<SEEntry> ses_;
	bool commandTest_;

	uint32_t targetOffset_ = 0;
	std::vector<uint32_t> callStack_;
	std::vector<uint16_t> varStack_; // separate from callstack?

	std::map<int, int16_t> variables_;

	typedef void (Script::*CommandFunction)(BinaryReader &, Archive &);
	std::vector<CommandFunction> commands_;

	void executeCommand(BinaryReader &br, Archive &archive);

	void setupCommandsUmineko();
	void setupCommandsHigurashi();

	int16_t getVariable(uint16_t value) {
		// Negative values that aren't variables also work so not 100% sure where the "cut-off point" is
		if ((value >> 0xC) == 0x8) {
			return variables_[value & ~0x8000];
		}
		return (int16_t)value;
	}

	void setVariable(uint16_t variable, uint16_t value) {
		if ((variable >> 0xC) != 0x8) {
			throw std::runtime_error("Not a variable: " + std::to_string(variable));
		}
		variables_[variable & ~0x8000] = value;
	}

	void jump(uint32_t offset) {
		targetOffset_ = offset;
	}

	std::string readString8(BinaryReader &br) {
		auto strSize = br.read<uint8_t>();
		auto str = br.readString(strSize - 1);
		br.skip(1);
		return str;
	}

	std::string readString16(BinaryReader &br) {
		auto strSize = br.read<uint16_t>();
		auto str = br.readString(strSize - 1);
		br.skip(1);
		return str;
	}

	/**
	 * [41] set_variable(op:u8, var:u16, val:u16, ...)
	 * Perform arithmetic operations and store the result in the specified variable
	 *  op=, ???, op+=, op-=, op*=, op/=
	 */
	void set_variable(BinaryReader &br, Archive &archive) {
		auto operation = br.read<uint8_t>();
		auto variable = br.read<uint16_t>();
		auto value = getVariable(br.read<uint16_t>());
		int16_t op1, op2;
		if (operation & 0x80) {
			// Two operands (confirmed?)
			auto secondValue = getVariable(br.read<uint16_t>());
			op1 = value;
			op2 = secondValue;
			return; // for now
		} else {
			op1 = getVariable(variable);
			op2 = value;
		}
		switch (operation) {
		case 0: // set (confirmed?)
			if (operation & 0x80) throw std::runtime_error("Cannot set with 2 operands (I think?)");
			setVariable(variable, value);
			break;
		case 1: // same as 0? appears to be set
			if (operation & 0x80) throw std::runtime_error("Cannot set(?) with 2 operands (I think?)");
			setVariable(variable, value); // ???
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
	void command42(BinaryReader &br, Archive &archive) {
		br.skip(14);
	}

	/**
	 * [46] jump_if
	 */
	void jump_if(BinaryReader &br, Archive &archive) {
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
		case 2: // greater than or equal to (confirmed!)
			if (value >= compareTo) {
				jump(offset);
			}
			break;
		case 3: // greater than (confirmed!)
			if (value > compareTo) {
				jump(offset);
			}
			break;
		case 4: // less than or equal to (confirmed!)
			if (value <= compareTo) {
				jump(offset);
			}
			break;
		case 5: // less than (confirmed!)
			if (value < compareTo) {
				jump(offset);
			}
			break;
		case 6: // ???
			break;
		default:
			throw std::runtime_error("Unsupported jump_if operation: " + std::to_string(operation));
		}
	}

	/**
	 * [47] jump(offset:u32)
	 */
	void jump(BinaryReader &br, Archive &archive) {
		auto offset = br.read<uint32_t>();
		jump(offset);
	}

	/**
	 * [48] call(offset:u32)
	 */
	void call(BinaryReader &br, Archive &archive) {
		auto offset = br.read<uint32_t>();
		callStack_.push_back(static_cast<uint32_t>(br.tellg()));
		jump(offset);
	}

	/**
	 * [49] return
	 */
	void return_(BinaryReader &br, Archive &archive) {
		if (callStack_.size() == 0) {
			throw std::runtime_error("Cannot return without having called a function first.");
		}
		targetOffset_ = callStack_.back();
		callStack_.pop_back();
	}

	/**
	 * [4A] branch_on_variable(var:u16, count:u16, offset1:u32[, offset2:u32...])
	 * Checks the value of the first argument and jumps to the corresponding address in the argument list
	 */
	void branch_on_variable(BinaryReader &br, Archive &archive) {
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

	/**
	 * [4D] push(count:u8, val1:u16[, val2:u16...])
	 */
	void push(BinaryReader &br, Archive &archive) {
		auto count = br.read<uint8_t>();
		for (int i = 0; i < count; ++i) {
			varStack_.push_back(getVariable(br.read<uint16_t>()));
		}
	}

	/**
	* [4E] pop(count:u8, var1:u16[, var2:u16...])
	*/
	void pop(BinaryReader &br, Archive &archive) {
		auto count = br.read<uint8_t>();
		// Think this pop order is correct
		for (int i = 0; i < count; ++i) {
			if (varStack_.size() == 0)
				throw std::out_of_range("Cannot pop from empty stack.");
			auto var = br.read<uint16_t>();
			if ((var & 0x8000) == 0)
				throw std::runtime_error("Cannot pop into a non-variable argument.");
			setVariable(var, varStack_.back());
			varStack_.pop_back();
		}
	}

	/**
	 * [80] unlock_content(id:u16)
	 */
	void unlock_content(BinaryReader &br, Archive &archive) {
		/*br.skip(3);
		auto count = br.read<uint8_t>() & ~0x80;
		br.skip(2 * count); // ???*/
		auto id = getVariable(br.read<uint16_t>());
	}

	void command81(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}

	/**
	 * [83] wait(frames:u16)
	 */
	void wait(BinaryReader &br, Archive &archive) {
		auto frames = br.read<uint16_t>();
		ctx_.wait(frames);
		pause();
	}

	void command85(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}

	/**
	 * [86] display_text(id:u16, unk:u8, pause:u8, text:str16)
	 */
	void display_text(BinaryReader &br, Archive &archive) {
		ctx_.applyLayers();
		auto msgId = getVariable(br.read<uint16_t>());
		br.skip(1); // ???
		auto shouldPause = br.read<uint8_t>();
		auto text = readString16(br);
		ctx_.message().push(text);
		if (shouldPause)
			pause();
	}

	/**
	 * [87] wait_msg_advance(segment:u16)
	 */
	void wait_msg_advance(BinaryReader &br, Archive &archive);

	/**
	 * [88] return_to_message()
	 */
	void return_to_message(BinaryReader &br, Archive &archive);

	/**
	 * [89] hide_text()
	 */
	void hide_text(BinaryReader &br, Archive &archive) {
		ctx_.message().hide();
	}

	/**
	 * [8C] show_choices(unk:u32, var:u16, unk2:i16, title:str8, choices:splitstr8)
	 */
	void show_choices(BinaryReader &br, Archive &archive) {
		br.skip(5);
		br.skip(3); // ???
		auto title = readString8(br);
		auto choices = readString8(br);
	}

	/**
	 * [8D] do_transition(...)
	 */
	void do_transition(BinaryReader &br, Archive &archive);

	/**
	 * [9C] play_bgm(id:u16, unk:u16, volume:u32)
	 */
	void play_bgm(BinaryReader &br, Archive &archive);

	/**
	 * [9D] stop_bgm(frames:u16)
	 */
	void stop_bgm(BinaryReader &br, Archive &archive);

	void command9E(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}
	/**
	 * [A0] play_se(channel:u16, unk:u16, volume:u32)
	 */
	void play_se(BinaryReader &br, Archive &archive);
	void commandA0_higu(BinaryReader &br, Archive &archive) {
		// same as umi B0?
		set_title(br, archive);
	}

	/**
	 * [A1] stop_se(channel:u16, frames:u16)
	 */
	void stop_se(BinaryReader &br, Archive &archive);

	/**
	 * [A2] stop_all_se(frames:u16)
	 */
	void stop_all_se(BinaryReader &br, Archive &archive);

	/**
	 * [A3] set_se_volume(channel:u16, volume:u16, frames?:u16)
	 */
	void set_se_volume(BinaryReader &br, Archive &archive);

	void commandA4(BinaryReader &br, Archive &archive) {
		br.skip(7); // Japanese character used in here?
	}

	/**
	 * [A6] shake(unk1:u16, unk2:u16)
	 */
	void shake(BinaryReader &br, Archive &archive) {
		auto unk1 = getVariable(br.read<uint16_t>());
		auto unk2 = getVariable(br.read<uint16_t>());
	}

	/**
	 * [B0] set_title(unk:u16, title:string)
	 */
	void set_title(BinaryReader &br, Archive &archive) {
		auto unk = br.read<uint16_t>();
		auto str = readString8(br);
		// ???
	}

	/**
	 * [B1] play_movie(id:u16)
	 */
	void play_movie(BinaryReader &br, Archive &archive) {
		auto movieId = getVariable(br.read<uint16_t>());
	}

	/**
	 * [B2] movie_related_B2(unk:u16)
	 */
	void movie_related_B2(BinaryReader &br, Archive &archive) {
		br.skip(2);
	}

	/**
	 * [B3] movie_related_B3()
	 */
	void movie_related_B3(BinaryReader &br, Archive &archive) {
	}

	/**
	 * [B4] movie_related_B4()
	 */
	void movie_related_B4(BinaryReader &br, Archive &archive) {
	}

	/**
	 * [B6] autosave()
	 */
	void autosave(BinaryReader &br, Archive &archive) {
	}

	/**
	 * [BE] unlock_trophy(id:u16)
	 */
	void unlock_trophy(BinaryReader &br, Archive &archive) {
		auto trophyId = getVariable(br.read<uint16_t>());
	}

	void commandBF(BinaryReader &br, Archive &archive) {
		br.skip(4);
	}

	/**
	 * [C1] display_image(...)
	 */
	void display_image(BinaryReader &br, Archive &archive);

	/**
	 * [C2] set_layer_property(layer:u16, property:u16, unk:u8, ...)
	 */
	void set_layer_property(BinaryReader &br, Archive &archive);
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