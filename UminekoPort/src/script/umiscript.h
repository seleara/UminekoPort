#pragma once

#include "scriptimpl.h"

class UmiScript : public ScriptImpl {
public:
	UmiScript(Script &script);
	virtual void load(BinaryReader &br) override;
protected:
	friend class Script;

	std::vector<AnimEntry> anims_;

	void readSprites(BinaryReader &br, uint32_t offset);
	void readAnims(BinaryReader &br, uint32_t offset);

	void setupCommands() override;

public:

	void command82(BinaryReader &br, Archive &archive);
};