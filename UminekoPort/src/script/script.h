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
	std::string name; // [0xC];
};

struct SpriteEntry {
	std::string name; //char name[0x18];
	std::string pose; //char pose[0x10];
};

struct CgEntry {
	std::string name; // [0x18];
	int16_t unknown;
};

struct AnimEntry {
	std::string name;
	int16_t unknown;
	int16_t unknown2;
};

struct BGMEntry {
	std::string name; // [0xC];
	std::string title; // [0x28];
};

struct SEEntry {
	std::string name; // [0x18];
};

enum class ImageType {
	Picture = 2,
	Sprite = 3
};

class AudioManager;

class Script {
public:
	Script(GraphicsContext &ctx, AudioManager &audio, bool commandTest=false);
	~Script();
	virtual void load(const std::string &path, Archive &archive);
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
	friend class ScriptImpl;
	friend class UmiScript;
	friend class ChiruScript;
	friend class HiguScript;

	std::unique_ptr<ScriptImpl> impl_;

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
	bool commandTest_;

	uint32_t targetOffset_ = 0;
	std::vector<uint32_t> callStack_;
	std::vector<uint16_t> varStack_; // separate from callstack?

	std::map<int, int16_t> variables_;

	void executeCommand(BinaryReader &br, Archive &archive);

	MaskEntry getMask(uint32_t id);
	CgEntry getCg(uint32_t id);
	SpriteEntry getSprite(uint32_t id);
	AnimEntry getAnim(uint32_t id);
	BGMEntry getBgm(uint32_t id);
	SEEntry getSe(uint32_t id);

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

	void setVariable(uint8_t operation, uint16_t variable, uint16_t value);
	void setVariable(uint8_t operation, uint16_t variable, uint16_t left, uint16_t right);
};