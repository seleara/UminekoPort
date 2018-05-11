#pragma once

#include "texture.h"

class Mask {
public:
	void load(const std::string &path, Archive &archive);
private:
	Texture texture_;
};