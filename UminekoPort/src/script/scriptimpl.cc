#include "scriptimpl.h"

#include "../engine/engine.h"
#include "../util/binaryreader.h"
#include "../audio/audio.h"
#include "script.h"

ScriptImpl::ScriptImpl(Script &script) : script_(script) {}

void ScriptImpl::readMasks(BinaryReader &br, uint32_t offset) {
	br.seekg(offset);
	auto maskCount = br.read<uint32_t>();
	masks_.resize(maskCount);
	for (uint32_t i = 0; i < maskCount; ++i) {
		auto &mask = masks_[i];
		mask.name = br.readString(0xc).c_str();
	}
}

void ScriptImpl::readCgs(BinaryReader &br, uint32_t offset) {
	br.seekg(offset);
	auto cgCount = br.read<uint32_t>();
	cgs_.resize(cgCount);
	for (uint32_t i = 0; i < cgCount; ++i) {
		auto &cg = cgs_[i];
		cg.name = br.readString(0x18).c_str();
		cg.unknown = br.read<int16_t>();
	}
}

void ScriptImpl::readBgms(BinaryReader &br, uint32_t offset) {
	br.seekg(offset);
	auto bgmCount = br.read<uint32_t>();
	bgms_.resize(bgmCount);
	for (uint32_t i = 0; i < bgmCount; ++i) {
		auto &bgm = bgms_[i];
		bgm.name = br.readString(0xc).c_str();
		bgm.title = br.readString(0x28).c_str();
	}
}

void ScriptImpl::readSes(BinaryReader &br, uint32_t offset) {
	br.seekg(offset);
	auto seCount = br.read<uint32_t>();
	ses_.resize(seCount);
	for (uint32_t i = 0; i < seCount; ++i) {
		auto &se = ses_[i];
		se.name = br.readString(0x18).c_str();
	}
}

void ScriptImpl::set_variable(BinaryReader &br, Archive &archive) {
	auto operation = br.read<uint8_t>();
	auto variable = br.read<uint16_t>();
	auto value = br.read<uint16_t>();
	int16_t op1, op2;
	if (operation & 0x80) {
		auto secondValue = br.read<uint16_t>();
		script_.setVariable(operation & ~0x80, variable, value, secondValue);
	} else {
		script_.setVariable(operation, variable, value);
	}
}

void ScriptImpl::command42(BinaryReader &br, Archive &archive) {
	br.skip(14);
}

void ScriptImpl::jump_if(BinaryReader &br, Archive &archive) {
	auto operation = br.read<uint8_t>();
	auto value = script_.getVariable(br.read<uint16_t>());
	auto compareTo = script_.getVariable(br.read<uint16_t>());
	auto offset = br.read<uint32_t>();

	switch (operation) {
	case 0: // equal to (confirmed!)
		if (value == compareTo) {
			script_.jump(offset);
		}
		break;
	case 1: // Not equal to (confirmed!)
		if (value != compareTo) {
			script_.jump(offset);
		}
		break;
	case 2: // greater than or equal to (confirmed!)
		if (value >= compareTo) {
			script_.jump(offset);
		}
		break;
	case 3: // greater than (confirmed!)
		if (value > compareTo) {
			script_.jump(offset);
		}
		break;
	case 4: // less than or equal to (confirmed!)
		if (value <= compareTo) {
			script_.jump(offset);
		}
		break;
	case 5: // less than (confirmed!)
		if (value < compareTo) {
			script_.jump(offset);
		}
		break;
	case 6: // ???
		break;
	default:
		throw std::runtime_error("Unsupported jump_if operation: " + std::to_string(operation));
	}
}

void ScriptImpl::jump(BinaryReader &br, Archive &archive) {
	auto offset = br.read<uint32_t>();
	script_.jump(offset);
}

void ScriptImpl::call(BinaryReader &br, Archive &archive) {
	auto offset = br.read<uint32_t>();
	script_.callStack_.push_back(static_cast<uint32_t>(br.tellg()));
	script_.jump(offset);
}

void ScriptImpl::return_(BinaryReader &br, Archive &archive) {
	if (script_.callStack_.size() == 0) {
		throw std::runtime_error("Cannot return without having called a function first.");
	}
	script_.targetOffset_ = script_.callStack_.back();
	script_.callStack_.pop_back();
}

void ScriptImpl::branch_on_variable(BinaryReader &br, Archive &archive) {
	auto value = script_.getVariable(br.read<uint16_t>());
	auto count = br.read<uint16_t>();
	std::vector<uint32_t> offsets;
	offsets.reserve(count);
	for (int i = 0; i < count; ++i) {
		offsets.push_back(br.read<uint32_t>());
	}
	if (value < 0 || value >= count)
		throw std::runtime_error("Value out of range in branch_on_variable (is this an error?).");
	else
		script_.jump(offsets[value]);
}

void ScriptImpl::push(BinaryReader &br, Archive &archive) {
	auto count = br.read<uint8_t>();
	for (int i = 0; i < count; ++i) {
		script_.varStack_.push_back(script_.getVariable(br.read<uint16_t>()));
	}
}

void ScriptImpl::pop(BinaryReader &br, Archive &archive) {
	auto count = br.read<uint8_t>();
	// Think this pop order is correct
	for (int i = 0; i < count; ++i) {
		if (script_.varStack_.size() == 0)
			throw std::out_of_range("Cannot pop from empty stack.");
		auto var = br.read<uint16_t>();
		if ((var & 0x8000) == 0)
			throw std::runtime_error("Cannot pop into a non-variable argument.");
		script_.setVariable(var, script_.varStack_.back());
		script_.varStack_.pop_back();
	}
}

void ScriptImpl::unlock_content(BinaryReader &br, Archive &archive) {
	/*br.skip(3);
	auto count = br.read<uint8_t>() & ~0x80;
	br.skip(2 * count); // ???*/
	auto id = script_.getVariable(br.read<uint16_t>());
}

void ScriptImpl::wait(BinaryReader &br, Archive &archive) {
	auto frames = br.read<uint16_t>();
	script_.ctx_.wait(frames);
	script_.pause();
}

void ScriptImpl::command85(BinaryReader &br, Archive &archive) {
	br.skip(4);
}

void ScriptImpl::display_text(BinaryReader &br, Archive &archive) {
	script_.ctx_.applyLayers();
	auto msgId = script_.getVariable(br.read<uint16_t>());
	br.skip(1); // ???
	auto shouldPause = br.read<uint8_t>();
	auto text = script_.readString16(br);
	script_.ctx_.message().push(text);
	if (shouldPause)
		script_.pause();
}

void ScriptImpl::wait_msg_advance(BinaryReader &br, Archive &archive) {
	auto segment = br.read<int16_t>();
	script_.ctx_.message().waitForMessageSegment(segment);
	script_.pause();
}

void ScriptImpl::return_to_message(BinaryReader &br, Archive &archive) {
	//script_.ctx_.returnToMessage();
}

void ScriptImpl::hide_text(BinaryReader &br, Archive &archive) {
	script_.ctx_.message().hide();
}

void ScriptImpl::show_choices(BinaryReader &br, Archive &archive) {
	br.skip(4); // Always 0?
	auto targetVar = br.read<uint16_t>();
	br.skip(2); // ???
	auto title = script_.readString8(br);
	auto choices = script_.readString8(br);
}

void ScriptImpl::do_transition(BinaryReader &br, Archive &archive) {
	uint8_t unknown = 0, unknown2 = 0;
	if (Engine::game == "chiru") {
		unknown = br.read<uint8_t>();
		if (unknown != 0)
			unknown2 = br.read<uint8_t>();
	}
	auto next = br.read<uint8_t>();
	next &= ~0x80;
	if (unknown != 0) {
		// ...
	} else if (next == 0x02) { // fade
		auto frames = script_.getVariable(br.read<uint16_t>());
		script_.ctx_.transition(frames);
		script_.pause();
	} else if (next == 0x03) { // mask
		auto maskId = script_.getVariable(br.read<uint16_t>());
		auto frames = script_.getVariable(br.read<uint16_t>());
		script_.ctx_.transition("mask/" + std::string(masks_[maskId].name) + ".msk", frames);
		script_.pause();
	} else if (next == 0x0C)
		br.skip(4);
	else if (next == 0x0E) {
		br.skip(6);
	}

	//if (unknown != 0)
	//	br.skip(2);
	script_.ctx_.applyLayers();
}

void ScriptImpl::play_bgm(BinaryReader &br, Archive &archive) {
	auto bgmId = script_.getVariable(br.read<uint16_t>());
	auto unk1 = br.read<uint16_t>();
	auto volume = br.read<uint32_t>(); // B4 00 00 00 - volume?

	script_.audio_.playBGM("bgm/" + std::string(bgms_[bgmId].name) + ".at3", volume / 255.0f);
}

void ScriptImpl::stop_bgm(BinaryReader &br, Archive &archive) {
	auto frames = script_.getVariable(br.read<uint16_t>());

	script_.audio_.stopBGM(frames);
}

void ScriptImpl::play_se(BinaryReader &br, Archive &archive) {
	auto channel = br.read<uint16_t>();
	auto seId = br.read<uint16_t>();
	auto unk = br.read<uint16_t>();
	auto volume = br.read<uint32_t>();

	script_.audio_.playSE(channel, "se/" + std::string(ses_[seId].name) + ".at3", volume / 255.0f);
}

void ScriptImpl::stop_se(BinaryReader &br, Archive &archive) {
	auto channel = script_.getVariable(br.read<uint16_t>());
	auto frames = script_.getVariable(br.read<uint16_t>());

	script_.audio_.stopSE(channel, frames);
}

void ScriptImpl::stop_all_se(BinaryReader &br, Archive &archive) {
	auto frames = script_.getVariable(br.read<uint16_t>());

	script_.audio_.stopAllSE(frames);
}

void ScriptImpl::set_se_volume(BinaryReader &br, Archive &archive) {
	auto index = script_.getVariable(br.read<uint16_t>());
	auto volume = br.read<uint16_t>() / 255.0f;
	auto framesMaybe = br.read<uint16_t>();
	script_.audio_.setSEVolume(index, volume);
}

void ScriptImpl::set_title(BinaryReader &br, Archive &archive) {
	auto unk = br.read<uint16_t>();
	auto str = script_.readString8(br);
	// ???
}

void ScriptImpl::play_movie(BinaryReader &br, Archive &archive) {
	auto movieId = script_.getVariable(br.read<uint16_t>());
}

void ScriptImpl::movie_related_B2(BinaryReader &br, Archive &archive) {
	br.skip(2);
}

void ScriptImpl::movie_related_B3(BinaryReader &br, Archive &archive) {
}

void ScriptImpl::movie_related_B4(BinaryReader &br, Archive &archive) {
}

void ScriptImpl::autosave(BinaryReader &br, Archive &archive) {
}

void ScriptImpl::unlock_trophy(BinaryReader &br, Archive &archive) {
	auto trophyId = script_.getVariable(br.read<uint16_t>());
}

void ScriptImpl::commandBF(BinaryReader &br, Archive &archive) {
	br.skip(4);
}

void ScriptImpl::display_image(BinaryReader &br, Archive &archive) {
	auto layer = br.read<uint16_t>(); // Layer?
	auto type = (ImageType)br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();
	if (unk3 == 0) {
		script_.ctx_.clearLayer(layer);
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
		auto texturePath = "bustup/" + sprites_[spriteId].name + ".bup";
		script_.ctx_.setLayerBup(layer, sprites_[spriteId].name, sprites_[spriteId].pose);
		//pause();
		//} else if (layer == 0x01 || layer == 0x02 || layer == 0x03) {
	} else if (type == ImageType::Picture) {
		std::cout << "Displaying CG(?) " << cgs_[spriteId].name << ". (" << std::hex << br.tellg() << std::dec << ")\n";
		auto texturePath = "picture/" + cgs_[spriteId].name + ".pic";
		script_.ctx_.setLayer(layer, texturePath);
		//pause();
	} else {
		std::cerr << "Unknown image type " << (int)type << " in display_image.\n";
	}
}

void ScriptImpl::set_layer_property(BinaryReader &br, Archive &archive) {
	auto layer = script_.getVariable(br.read<uint16_t>());
	auto prop = br.read<uint16_t>();
	auto unk3 = br.read<uint8_t>();
	if (unk3 == 0x0) {
		// ...
	} else if (unk3 == 0x01) {
		auto props = script_.ctx_.layerProperties(layer);
		auto value = script_.getVariable(br.read<uint16_t>());
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
		script_.ctx_.setLayerProperties(layer, props);
	} else if (unk3 == 0x06) {
		br.skip(4);
	} else if (unk3 == 0x07) {
		br.skip(6);
	} else {
		//br.skip(2);
	}
}

void ScriptImpl::commandC3(BinaryReader &br, Archive &archive) {
	br.skip(4);
}

void ScriptImpl::commandC9(BinaryReader &br, Archive &archive) {
	// ???
}

void ScriptImpl::commandCA(BinaryReader &br, Archive &archive) {
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

void ScriptImpl::commandCB(BinaryReader &br, Archive &archive) {
	br.skip(3);
}