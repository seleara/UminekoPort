#pragma once

#include "scriptimpl.h"

class HiguScript : public ScriptImpl {
public:
	HiguScript(Script &script);
	void load(BinaryReader &br) override;
protected:
	friend class Script;

	void readSprites(BinaryReader &br, uint32_t offset);

	void setupCommands() override;

public:

	void command82(BinaryReader &br, Archive &archive);
	void command8A(BinaryReader &br, Archive &archive);
	void command95(BinaryReader &br, Archive &archive);
	void commandA5(BinaryReader &br, Archive &archive);
	void play_voice_maybe(BinaryReader &br, Archive &archive);
	void commandAE(BinaryReader &br, Archive &archive);
	void commandB4(BinaryReader &br, Archive &archive);
	void display_image(BinaryReader &br, Archive &archive);
	void set_layer_property(BinaryReader &br, Archive &archive);
};