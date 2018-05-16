#include "messagewindow.h"

#include "../data/archive.h"
#include "spritebatch.h"

void MessageWindow::init(Archive &archive) {
	Texture msgTex;
	msgTex.loadTxa("msgwnd.txa", archive, "msgwnd");
	msgSprite_.setTexture(msgTex);
	msgSprite_.textureRect = glm::ivec4(251, 15, 1637, 277);
	//msgSprite_.textureRect = glm::vec4(0.1523f, 0.0486f, 0.9933f, 0.9618f);
	msgSprite_.anchor = Anchor::Bottom;
	msgSprite_.pivot = Pivot::Bottom;
	msgTransform_.scale *= 1.25f;
	msgTransform_.position.y = 20;
	msgTransform_.position.z = 10;

	text_.setFont(Font::global());
	text_.setWrap(1560);
	text_.transform().position.x = 200;
	text_.transform().position.y = 650;
}

void MessageWindow::addText(std::string text) {
	done_ = false;
	messages_.push_back(std::move(text_.convert(text)));
	if (messages_.size() == 1)
		text_.setText(messages_.front());
}

void MessageWindow::advance() {
	if (!done_) {
		//done_ = true;
		text_.advance();
		if (text_.done()) {

			// if whole message is done (to be implemented)
			messages_.pop_front();
			if (messages_.size() >= 1)
				text_.setText(messages_.front());
			else
				done_ = true;
		}
	}
}

bool MessageWindow::done() const {
	return done_;
}

void MessageWindow::setVisible(bool visible) {
	visible_ = visible;
}

bool MessageWindow::visible() const {
	return visible_;
}

void MessageWindow::render() {
	if (!visible()) return;
	SpriteBatch batch;
	batch.add(msgSprite_, msgTransform_);
	batch.render();
	text_.render();
}