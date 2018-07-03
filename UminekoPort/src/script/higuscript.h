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
};