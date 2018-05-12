#pragma once

#include <deque>
#include <mutex>

#include "../window/window.h"
#include "../graphics/texture.h"
#include "../graphics/spritebatch.h"
#include "../graphics/sprite.h"
#include "../math/transform.h"
#include "../math/time.h"

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

class MessageWindow {
public:
	void init(Archive &archive) {
		Texture msgTex;
		//msgTex.loadTxa("msgwnd.txa", archive, "msgwnd");
		msgSprite_.setTexture(msgTex);
		msgSprite_.textureRect = glm::ivec4(251, 15, 1637, 277);
		//msgSprite_.textureRect = glm::vec4(0.1523f, 0.0486f, 0.9933f, 0.9618f);
		msgSprite_.anchor = Anchor::Bottom;
		msgSprite_.pivot = Pivot::Bottom;
		msgTransform_.scale *= 1.25f;
		msgTransform_.position.y = 20;
		msgTransform_.position.z = 10;
	}

	void addText(std::string text) {
		done_ = false;
		messages_.push_back(std::move(text));
	}

	void advance() {
		done_ = true;

		// if whole message is done (to be implemented)
		messages_.pop_front();
	}

	bool done() const {
		return done_;
	}

	void setVisible(bool visible) {
		visible_ = visible;
	}

	bool visible() const {
		return visible_;
	}
private:
	friend class GraphicsContext;
	Sprite msgSprite_;
	Transform msgTransform_;
	std::deque<std::string> messages_;
	bool done_ = true;
	bool visible_ = false;
};

class GraphicsContext {
public:
	GraphicsContext(Window &window, Archive &archive) : window_(window), archive_(archive) {
		layers_.resize(0x20);
		for (auto &layer : layers_) {
			layer.newProperties.sprite.anchor = Anchor::Bottom;
			layer.newProperties.sprite.pivot = Pivot::Bottom;
		}
		newLayers_.resize(0x20);
		std::copy(layers_.begin(), layers_.end(), newLayers_.begin());

		msg_.init(archive);
	}

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

	void pushMessage(const std::string &text) {
		msg_.addText(text);
		msg_.setVisible(true);
	}

	void hideMessage() {
		msg_.setVisible(false);
	}

	void advance() {
		msg_.advance();
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

	void update() {
		if (waiting_) {
			waitTime_ -= Time::deltaTime();
		}
	}

	void render() {
		std::unique_lock<std::mutex> lock(graphicsMutex_);
		auto updateLayer = [&](GraphicsLayer &layer) {
			if (layer.dirty) {
				if (layer.type == GraphicsLayerType::Default) {
					layer.texture.load(layer.texturePath, archive_);
				} else if (layer.type == GraphicsLayerType::Bup) {
					layer.texture.loadBup(layer.texturePath, archive_, layer.bupPose);
				}
				layer.dirty = false;
			}
			layer.properties = layer.newProperties;
		};
		for (auto &layer : newLayers_) {
			updateLayer(layer);
		}
		for (auto &layer : layers_) {
			updateLayer(layer);
		}
		SpriteBatch batch;
		for (int i = 0; i < layers_.size(); ++i) {
			auto &layer = layers_[i];
			if (layer.type != GraphicsLayerType::None && layer.texture.valid()) {
				layer.properties.sprite.setTexture(layer.texture);
				layer.properties.transform.position.x = static_cast<float>(layer.properties.offset.x);
				layer.properties.transform.position.y = static_cast<float>(layer.properties.offset.y);
				batch.add(layer.properties.sprite, layer.properties.transform);
			}
		}
		if (msg_.visible()) {
			batch.add(msg_.msgSprite_, msg_.msgTransform_);
		}
		lock.unlock();
		batch.render();
	}
private:
	MessageWindow msg_;
	std::mutex graphicsMutex_;
	std::vector<GraphicsLayer> layers_;
	std::vector<GraphicsLayer> newLayers_; // new state
	Window &window_;
	Archive &archive_;

	bool waiting_ = false;
	double waitTime_ = 0.0;
};