#pragma once

#include <deque>

#include "texture.h"
#include "sprite.h"
#include "font.h"

class Archive;

class MessageWindow {
public:
	void init(Archive &archive);

	void addText(std::string text);

	void advance();
	bool done() const;

	void setVisible(bool visible);
	bool visible() const;

	void render();
private:
	friend class GraphicsContext;
	Sprite msgSprite_;
	Transform msgTransform_;
	std::deque<std::string> messages_;
	bool done_ = true;
	bool visible_ = false;

	Text text_;
};