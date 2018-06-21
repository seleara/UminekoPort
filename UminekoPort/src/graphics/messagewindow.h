#pragma once

#include <deque>

#include "texture.h"
#include "sprite.h"
#include "font.h"

class Archive;
class AudioManager;

class MessageWindow {
public:
	void init(Archive &archive, AudioManager &audio);

	void advance();
	bool done() const;

	void push(const std::string &text);
	void hide();

	bool visible() const;

	int currentSegment() const;

	void waitForMessageSegment(int segment) {
		isWaitingForMessageSegment_ = true;
		doneWaitingForMessageSegment_ = false;
		waitForMessageSegment_ = segment;
	}

	bool doneWaitingForMessageSegment() const {
		return doneWaitingForMessageSegment_;
	}

	void update();
	void render();
private:
	friend class GraphicsContext;

	void addText(std::string text);
	void setVisible(bool visible);

	Sprite msgSprite_;
	Transform msgTransform_;
	std::deque<std::string> messages_;
	bool done_ = true;
	bool visible_ = false;

	AudioManager *audio_;

	Text text_;

	bool isWaitingForMessageSegment_ = false, doneWaitingForMessageSegment_ = false;
	int waitForMessageSegment_ = -1;
};