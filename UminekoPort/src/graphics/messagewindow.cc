#include "messagewindow.h"

#include "../audio/audiomanager.h"
#include "../data/archive.h"
#include "../engine/engine.h"
#include "spritebatch.h"

void MessageWindow::init(Archive &archive, AudioManager &audio) {
	Texture msgTex;
	if (Engine::game != "higu") msgTex.loadTxa("msgwnd.txa", archive, "msgwnd");
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

	audio_ = &audio;
}

void MessageWindow::addText(std::string text) {
	done_ = false;
	messages_.push_back(std::move(text_.convert(text)));
	if (messages_.size() == 1) {
		text_.setText(messages_.front());

		// Note: A voice line can appear in the middle of a segment of text without a key press in-between.
		// In that case, the text display waits for the previous voice to play before continuing
		// TODO: Implement this
		if (text_.hasVoice()) {
			audio_->playVoice(text_.getVoice());
		}
	}
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
		} else {
			if (text_.hasVoice()) {
				audio_->playVoice(text_.getVoice());
			}
		}
	}

	if (isWaitingForMessageSegment_) {
		if (waitForMessageSegment_ == -1 && done()) {
			doneWaitingForMessageSegment_ = true;
			isWaitingForMessageSegment_ = false;
		} else if (waitForMessageSegment_ + 1 == currentSegment()) {
			doneWaitingForMessageSegment_ = true;
			isWaitingForMessageSegment_ = false;
		}
	}
}

bool MessageWindow::done() const {
	return done_;
}

void MessageWindow::push(const std::string &text) {
	addText(text);
	setVisible(true);
}

void MessageWindow::hide() {
	setVisible(false);
}

void MessageWindow::setVisible(bool visible) {
	visible_ = visible;
}

bool MessageWindow::visible() const {
	return visible_;
}

int MessageWindow::currentSegment() const {
	return text_.currentSegment();
}

void MessageWindow::update() {
	if (!visible()) return;
	text_.update();
}

void MessageWindow::render() {
	if (!visible()) return;
	SpriteBatch batch;
	batch.add(msgSprite_, msgTransform_);
	batch.render();
	text_.render();
}