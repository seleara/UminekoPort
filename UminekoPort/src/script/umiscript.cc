#include "umiscript.h"

#include "../engine/engine.h"

UmiScript::UmiScript(Script &script) : ScriptImpl(script) {}

void UmiScript::load(BinaryReader &br) {
	script_.scriptOffset_ = br.read<uint32_t>();

	// Resource offsets
	auto maskOffset = br.read<uint32_t>();
	auto cgOffset = br.read<uint32_t>();
	auto spriteOffset = br.read<uint32_t>();
	auto animeOffset = br.read<uint32_t>();
	auto bgmOffset = br.read<uint32_t>();
	auto seOffset = br.read<uint32_t>();
	auto movieOffset = br.read<uint32_t>();
	auto voiceOffset = br.read<uint32_t>();
	auto unknown1Offset = br.read<uint32_t>();
	auto unknown2Offset = br.read<uint32_t>();
	auto textOffset = br.read<uint32_t>();
	auto unknown3Offset = br.read<uint32_t>();
	auto characterProfileOffset = br.read<uint32_t>();

	readMasks(br, maskOffset);

	readCgs(br, cgOffset);

	readSprites(br, spriteOffset);

	readAnims(br, animeOffset);

	readBgms(br, bgmOffset);

	readSes(br, seOffset);
}

void UmiScript::readSprites(BinaryReader &br, uint32_t offset) {
	br.seekg(offset);
	auto spriteCount = br.read<uint32_t>();
	sprites_.resize(spriteCount);
	for (uint32_t i = 0; i < spriteCount; ++i) {
		SpriteEntry &e = sprites_[i];
		e.name = br.readString(0x18).c_str();
		e.pose = br.readString(0x10).c_str();
	}
}

void UmiScript::readAnims(BinaryReader &br, uint32_t offset) {
	br.seekg(offset);
	auto animCount = br.read<uint32_t>();
	anims_.resize(animCount);
	for (uint32_t i = 0; i < animCount; ++i) {
		AnimEntry &e = anims_[i];
		e.name = br.readString(0x24).c_str();
		e.unknown = br.read<int16_t>();
		e.unknown2 = br.read<int16_t>();
	}
}

void UmiScript::setupCommands() {
	commands_.resize(0x100, nullptr);
	commands_[0x41] = std::bind(&ScriptImpl::set_variable, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x42] = std::bind(&ScriptImpl::command42, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x46] = std::bind(&ScriptImpl::jump_if, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x47] = std::bind(&ScriptImpl::jump, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x48] = std::bind(&ScriptImpl::call, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x49] = std::bind(&ScriptImpl::return_, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x4A] = std::bind(&ScriptImpl::branch_on_variable, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x4D] = std::bind(&ScriptImpl::push, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x4E] = std::bind(&ScriptImpl::pop, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x80] = std::bind(&ScriptImpl::unlock_content, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x81] = std::bind(&ScriptImpl::command81, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x82] = std::bind(&UmiScript::command82, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x83] = std::bind(&ScriptImpl::wait, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x85] = std::bind(&ScriptImpl::command85, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x86] = std::bind(&ScriptImpl::display_text, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x87] = std::bind(&ScriptImpl::wait_msg_advance, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x88] = std::bind(&ScriptImpl::return_to_message, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x89] = std::bind(&ScriptImpl::hide_text, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x8C] = std::bind(&ScriptImpl::show_choices, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x8D] = std::bind(&ScriptImpl::do_transition, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x9C] = std::bind(&ScriptImpl::play_bgm, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x9D] = std::bind(&ScriptImpl::stop_bgm, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x9E] = std::bind(&ScriptImpl::command9E, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA0] = std::bind(&ScriptImpl::play_se, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA1] = std::bind(&ScriptImpl::stop_se, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA2] = std::bind(&ScriptImpl::stop_all_se, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA3] = std::bind(&ScriptImpl::set_se_volume, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA4] = std::bind(&ScriptImpl::commandA4, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA6] = std::bind(&ScriptImpl::shake, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB0] = std::bind(&ScriptImpl::set_title, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB1] = std::bind(&ScriptImpl::play_movie, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB2] = std::bind(&ScriptImpl::movie_related_B2, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB3] = std::bind(&ScriptImpl::movie_related_B3, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB4] = std::bind(&ScriptImpl::movie_related_B4, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB6] = std::bind(&ScriptImpl::autosave, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xBE] = std::bind(&ScriptImpl::unlock_trophy, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xBF] = std::bind(&ScriptImpl::commandBF, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xC1] = std::bind(&ScriptImpl::display_image, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xC2] = std::bind(&ScriptImpl::set_layer_property, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xC3] = std::bind(&ScriptImpl::commandC3, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xC9] = std::bind(&ScriptImpl::commandC9, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xCA] = std::bind(&ScriptImpl::commandCA, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xCB] = std::bind(&ScriptImpl::commandCB, this, std::placeholders::_1, std::placeholders::_2);
}

void UmiScript::command82(BinaryReader &br, Archive &archive) {
	auto unk = br.read<uint16_t>();
	auto unk2 = br.read<uint16_t>();
}