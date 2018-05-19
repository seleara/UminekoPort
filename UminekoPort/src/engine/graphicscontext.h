#pragma once

#include <mutex>

#include "../window/window.h"
#include "../graphics/framebuffer.h"
#include "../graphics/texture.h"
#include "../graphics/spritebatch.h"
#include "../graphics/sprite.h"
#include "../math/transform.h"
#include "../math/time.h"
#include "../graphics/font.h"
#include "../graphics/messagewindow.h"

enum class GraphicsLayerType {
	None,
	Default,
	Bup
};

struct GraphicsLayerFilter {
	enum Flags {
		None = 0,
		Sepia = 1,
		Inverted = 2,
		Grayscale = 4,
		White = 8, // ???
	};
};

enum class GraphicsLayerBlendMode {
	None = 0,
	Add = 1,
	Subtract = 2
};

struct GraphicsLayerProperties {
	Sprite sprite;
	Transform transform;
	glm::ivec2 offset;
	GraphicsLayerFilter::Flags filter;
	GraphicsLayerBlendMode blendMode;
};

struct GraphicsLayer {
	GraphicsLayerType type = GraphicsLayerType::None;
	Texture texture;
	std::string texturePath;
	std::string bupPose;
	bool dirty = false;

	GraphicsLayerProperties newProperties;
	GraphicsLayerProperties properties;
};

struct ShaderTransition {
	glm::vec4 progress; // x
};

class GraphicsContext {
public:
	GraphicsContext(Window &window, Archive &archive, AudioManager &audio);

	void resize();

	void wait(uint32_t frames) {
		waitTime_ = frames / 60.0;
		waiting_ = true;
	}

	bool waitingDone() const {
		return waiting_ && waitTime_ <= 0.0;
	}

	void stopWait() {
		waitTime_ = 0.0;
		waiting_ = false;
	}

	void transition(uint32_t frames) {
		useMask_ = false;
		isTransitioning_ = true;
		transitionSpeed_ = frames / 60.0;
		transitionProgress_ = 0;
	}

	void transition(const std::string &maskFilename, uint32_t frames) {
		useMask_ = true;
		maskDirty_ = true;
		maskFilename_ = maskFilename;
		isTransitioning_ = true;
		transitionSpeed_ = frames / 60.0;
		transitionProgress_ = 0;
	}

	bool transitionDone() const {
		return isTransitioning_ && transitionProgress_ >= 1.0;
	}

	void endTransitionMode() {
		isTransitioning_ = false;
		transitionProgress_ = 0;
		transitionSpeed_ = 0;
	}

	void pushMessage(const std::string &text) {
		msg_.addText(text);
		msg_.setVisible(true);
	}

	void hideMessage() {
		msg_.setVisible(false);
	}

	void advance() {
		msg_.advance();
		if (isWaitingForMessageSegment_) {
			if (waitForMessageSegment_ == -1 && msg_.done()) {
				doneWaitingForMessageSegment_ = true;
				isWaitingForMessageSegment_ = false;
			} else if (waitForMessageSegment_ + 1 == msg_.currentSegment()) {
				doneWaitingForMessageSegment_ = true;
				isWaitingForMessageSegment_ = false;
			}
		}
	}

	void waitForMessageSegment(int segment) {
		isWaitingForMessageSegment_ = true;
		doneWaitingForMessageSegment_ = false;
		waitForMessageSegment_ = segment;
	}

	bool doneWaitingForMessageSegment() const {
		return doneWaitingForMessageSegment_;
	}

	bool messageDone() const {
		return msg_.done();
	}

	GraphicsLayerProperties layerProperties(int layer) {
		return layers_[layer].newProperties;
	}

	void setLayerProperties(int layer, GraphicsLayerProperties properties) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		auto &l = newLayers_[layer];
		l.newProperties = std::move(properties);
	}

	void clearLayer(int layer) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		newLayers_[layer].type = GraphicsLayerType::None;
	}

	void setLayer(int layer, Texture texture) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		auto &l = newLayers_[layer];
		l.type = GraphicsLayerType::Default;
		l.texture = texture;
		l.dirty = false;
	}

	void setLayer(int layer, const std::string &path) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		auto &l = newLayers_[layer];
		l.type = GraphicsLayerType::Default;
		l.texturePath = path;
		l.dirty = true;
	}

	void setLayerBup(int layer, const std::string &name, const std::string &pose) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		auto &l = newLayers_[layer];
		l.texturePath = "bustup/" + name + ".bup";
		l.bupPose = pose;
		l.type = GraphicsLayerType::Bup;
		l.dirty = true;
	}

	void applyLayers() {
		std::unique_lock<std::mutex> lock(graphicsMutex_);
		std::cout << "APPLY LAYERS" << std::endl;
		// implement transition stuff later
		for (int i = 0; i < layers_.size(); ++i) {
			layers_[i] = newLayers_[i];
		}
	}

	void update();

	void render();
private:
	MessageWindow msg_;
	std::mutex graphicsMutex_;
	std::vector<GraphicsLayer> layers_;
	std::vector<GraphicsLayer> newLayers_; // new state
	Window &window_;
	Archive &archive_;
	AudioManager &audio_;

	bool isTransitioning_ = false, useMask_ = false;
	double transitionSpeed_ = 0;
	double transitionProgress_ = 0;

	bool maskDirty_ = false;
	std::string maskFilename_;

	//GLuint prevTexture_, nextTexture_;
	//Texture prevTexture_, nextTexture_;
	//GLuint prevFramebuffer_, nextFramebuffer_;
	//GLuint prevDepthRb_, nextDepthRb_;
	Framebuffer prevFramebuffer_, nextFramebuffer_;
	Texture transitionMask_;

	bool waiting_ = false;
	double waitTime_ = 0.0;

	bool isWaitingForMessageSegment_ = false, doneWaitingForMessageSegment_ = false;
	int waitForMessageSegment_ = -1;
};