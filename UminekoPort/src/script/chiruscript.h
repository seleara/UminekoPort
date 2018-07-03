#pragma once

#include "umiscript.h"

class ChiruScript : public UmiScript {
public:
	ChiruScript(Script &script);
	void load(BinaryReader &br) override;
protected:
	friend class Script;
};