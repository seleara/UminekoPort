#include "higuscript.h"

HiguScript::HiguScript(Script &script) : ScriptImpl(script) {}

void HiguScript::load(BinaryReader &br) {
	script_.scriptOffset_ = br.read<uint32_t>();

	// Resource offsets
	auto maskOffset = br.read<uint32_t>();
	auto cgOffset = br.read<uint32_t>();
	auto spriteOffset = br.read<uint32_t>();
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

	readBgms(br, bgmOffset);

	readSes(br, seOffset);
}

void HiguScript::readSprites(BinaryReader &br, uint32_t offset) {
	br.seekg(offset);
	auto spriteCount = br.read<uint32_t>();
	sprites_.resize(spriteCount);
	for (uint32_t i = 0; i < spriteCount; ++i) {
		SpriteEntry &e = sprites_[i];
		e.name = br.readString(0x18).c_str();
		e.pose = br.readString(0x12).c_str();
	}
}

void HiguScript::setupCommands() {
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
	commands_[0x82] = std::bind(&HiguScript::command82, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x83] = std::bind(&ScriptImpl::wait, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x85] = std::bind(&ScriptImpl::command85, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x86] = std::bind(&ScriptImpl::display_text, this, std::placeholders::_1, std::placeholders::_2);
	//commands_[0x87] = std::bind(&ScriptImpl::wait_msg_advance, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x88] = std::bind(&ScriptImpl::return_to_message, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x89] = std::bind(&ScriptImpl::hide_text, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x8A] = std::bind(&HiguScript::command8A, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x8D] = std::bind(&ScriptImpl::show_choices, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x8E] = std::bind(&ScriptImpl::do_transition, this, std::placeholders::_1, std::placeholders::_2); // Probably
	commands_[0x95] = std::bind(&HiguScript::command95, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x9C] = std::bind(&ScriptImpl::play_bgm, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x9D] = std::bind(&ScriptImpl::stop_bgm, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0x9E] = std::bind(&ScriptImpl::command9E, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA0] = std::bind(&ScriptImpl::set_title, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA1] = std::bind(&ScriptImpl::stop_se, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA2] = std::bind(&ScriptImpl::stop_all_se, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA3] = std::bind(&ScriptImpl::set_se_volume, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA4] = std::bind(&ScriptImpl::commandA4, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA5] = std::bind(&HiguScript::commandA5, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xA6] = std::bind(&HiguScript::play_voice_maybe, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xAE] = std::bind(&HiguScript::commandAE, this, std::placeholders::_1, std::placeholders::_2);
	//commands_[0xB0] = std::bind(&ScriptImpl::set_title, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB1] = std::bind(&ScriptImpl::play_movie, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB2] = std::bind(&ScriptImpl::movie_related_B2, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB3] = std::bind(&ScriptImpl::movie_related_B3, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB4] = std::bind(&HiguScript::commandB4, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xB6] = std::bind(&ScriptImpl::autosave, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xBE] = std::bind(&ScriptImpl::unlock_trophy, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xBF] = std::bind(&ScriptImpl::commandBF, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xC1] = std::bind(&HiguScript::display_image, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xC2] = std::bind(&HiguScript::set_layer_property, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xC3] = std::bind(&ScriptImpl::commandC3, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xC9] = std::bind(&ScriptImpl::commandC9, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xCA] = std::bind(&ScriptImpl::commandCA, this, std::placeholders::_1, std::placeholders::_2);
	commands_[0xCB] = std::bind(&ScriptImpl::commandCB, this, std::placeholders::_1, std::placeholders::_2);
}

void HiguScript::command82(BinaryReader &br, Archive &archive) {
	auto unk = br.read<uint16_t>();
}

void HiguScript::command8A(BinaryReader &br, Archive &archive) {
	auto unk = br.read<uint32_t>();
}

void HiguScript::command95(BinaryReader &br, Archive &archive) {
	auto unk = br.read<uint16_t>();
	auto unk2 = br.read<uint16_t>();
	auto unk3 = br.read<uint16_t>();
	auto unk4 = br.read<uint32_t>();
}

void HiguScript::commandA5(BinaryReader &br, Archive &archive) {
	// ...
}

void HiguScript::play_voice_maybe(BinaryReader &br, Archive &archive) {
	auto path = script_.readString8(br);
	auto unk = br.read<uint16_t>();
	auto unk2 = br.read<uint16_t>();
}

void HiguScript::commandAE(BinaryReader &br, Archive &archive) {
	auto unk = br.read<uint16_t>();
	auto unk2 = br.read<uint16_t>();
}

void HiguScript::commandB4(BinaryReader &br, Archive &archive) {
	auto unk = br.read<uint16_t>();
}

void HiguScript::display_image(BinaryReader &br, Archive &archive) {
	auto layer = br.read<uint16_t>(); // Layer?
	auto type = (ImageType)br.read<uint16_t>();
	auto unk3 = br.read<uint16_t>();
	auto unk4 = br.read<uint8_t>();
	if (unk4 == 0) {
		script_.ctx_.clearLayer(layer);
		return;
	}
	if (type == ImageType::None) return;
	//std::cout << "[C1: " << (int)layer << "|" << (int)type << "|" << (int)unk3 << "]\n";
	//if (layer == 0x08 || layer == 0x09 || layer == 0x0a) {
	if (type == ImageType::Sprite) {
		auto spriteId = script_.getVariable(br.read<uint16_t>());
		std::cout << "Displaying sprite " << sprites_[spriteId].name << "_" << sprites_[spriteId].pose << ".\n";
		/*Texture texture;
		texture.load(, archive);*/
		auto texturePath = "bustup/" + sprites_[spriteId].name + ".bup";
		script_.ctx_.setLayerBup(layer, sprites_[spriteId].name, sprites_[spriteId].pose);
		//pause();
		//} else if (layer == 0x01 || layer == 0x02 || layer == 0x03) {
	} else if (type == ImageType::Picture) {
		auto spriteId = script_.getVariable(br.read<uint16_t>());
		std::cout << "Displaying CG(?) " << cgs_[spriteId].name << ". (" << std::hex << br.tellg() << std::dec << ")\n";
		auto texturePath = "picture/" + cgs_[spriteId].name + ".pic";
		script_.ctx_.setLayer(layer, texturePath);
		//pause();
	} else if (type == ImageType::Type1) {
		auto width = br.read<uint16_t>();
		auto height = br.read<uint16_t>();
		//...
	} else {
		std::cerr << "Unknown image type " << (int)type << " in display_image.\n";
	}
}

void HiguScript::set_layer_property(BinaryReader &br, Archive &archive) {
	auto layer = script_.getVariable(br.read<uint16_t>());
	auto prop = br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();
	if (unk3 == 0x0) {
		// ...
	} else if (unk3 == 0x01) {
		auto props = script_.ctx_.layerProperties(layer);
		auto value = script_.getVariable(br.read<uint16_t>());
		switch (prop) {
		case 3:
			props.sprite.color.a = (value & 0xff) / 255.0f;
			break;
		case 4:
			props.sprite.color.r = (value & 0xff) / 255.0f;
			break;
		case 5:
			props.sprite.color.g = (value & 0xff) / 255.0f;
			break;
		case 6:
			props.sprite.color.b = (value & 0xff) / 255.0f;
			break;
		/*case 5:
			props.filter = (GraphicsLayerFilter::Flags)(value & 0xf);
			break;
		case 6:
			if (value > 2) {
				throw std::runtime_error("Unhandled blend mode, investigate.");
			}
			props.blendMode = (GraphicsLayerBlendMode)value;
			break;*/
		case 7:
			props.offset.x = value;
			break;
		case 8:
			props.offset.y = value;
		default: // to be implemented
			break;
		}
		script_.ctx_.setLayerProperties(layer, props);
	} else if (unk3 == 0x06) {
		br.skip(4);
	} else if (unk3 == 0x07) {
		br.skip(6);
	} else {
		//br.skip(2);
	}
}