#pragma once

#include <mutex>

#include "../window/window.h"
#include "../graphics/texture.h"
#include "../graphics/spritebatch.h"
#include "../graphics/sprite.h"
#include "../math/transform.h"

enum class GraphicsLayerType {
	None,
	Default,
	Bup
};

struct GraphicsLayerProperties {
	Sprite sprite;
	Transform transform;
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
	void setText(const std::string &text) {
		done_ = false;
		text_ = text;
	}
	void advance() {
		done_ = true;
	}
	bool done() const {
		return done_;
	}
private:
	friend class GraphicsContext;
	Sprite msgSprite_;
	Transform msgTransform_;
	std::string text_;
	bool done_ = true;
};

class GraphicsContext {
public:
	GraphicsContext(Window &window, Archive &archive) : window_(window), archive_(archive) {
		layers_.resize(0x20);
		layers_[0x01].newProperties.sprite.anchor = Anchor::Bottom;
		layers_[0x01].newProperties.sprite.pivot = Pivot::Bottom;
		layers_[0x01].newProperties.transform.position = glm::vec3(0, -90, 0);
		layers_[0x02].newProperties.sprite.anchor = Anchor::Bottom;
		layers_[0x02].newProperties.sprite.pivot = Pivot::Bottom;
		layers_[0x02].newProperties.transform.position = glm::vec3(0, -90, 0);
		layers_[0x03].newProperties.sprite.anchor = Anchor::Bottom;
		layers_[0x03].newProperties.sprite.pivot = Pivot::Bottom;
		layers_[0x03].newProperties.transform.position = glm::vec3(0, -90, 0);

		layers_[0x08].newProperties.sprite.anchor = Anchor::BottomLeft;
		layers_[0x08].newProperties.sprite.pivot = Pivot::BottomLeft;
		layers_[0x08].newProperties.transform.position = glm::vec3(134, -164, 0);
		layers_[0x09].newProperties.sprite.anchor = Anchor::Bottom;
		layers_[0x09].newProperties.sprite.pivot = Pivot::Bottom;
		layers_[0x09].newProperties.transform.position = glm::vec3(0, -164, 0);
		layers_[0x0a].newProperties.sprite.anchor = Anchor::BottomRight;
		layers_[0x0a].newProperties.sprite.pivot = Pivot::BottomRight;
		layers_[0x0a].newProperties.transform.position = glm::vec3(-64, -164, 0);

		msg_.init(archive);
	}

	void pushMessage(const std::string &text) {
		msg_.setText(text);
	}

	void advance() {
		msg_.advance();
	}

	bool messageDone() const {
		return msg_.done();
	}

	GraphicsLayerProperties layerProperties(int layer) {
		return layers_[layer].properties;
	}

	void setLayerProperties(int layer, GraphicsLayerProperties properties) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		layers_[layer].newProperties = properties;
	}

	void clearLayer(int layer) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		layers_[layer].type = GraphicsLayerType::None;
	}

	void setLayer(int layer, Texture texture) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		layers_[layer].type = GraphicsLayerType::Default;
		layers_[layer].texture = texture;
		layers_[layer].dirty = false;
	}

	void setLayer(int layer, const std::string &path) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		layers_[layer].type = GraphicsLayerType::Default;
		layers_[layer].texturePath = path;
		layers_[layer].dirty = true;
	}

	void setLayerBup(int layer, const std::string &name, const std::string &pose) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		layers_[layer].texturePath = "bustup/" + name + ".bup";
		layers_[layer].bupPose = pose;
		layers_[layer].type = GraphicsLayerType::Bup;
		layers_[layer].dirty = true;
	}

	void render() {
		std::unique_lock<std::mutex> lock(graphicsMutex_);
		for (auto &layer : layers_) {
			if (layer.dirty) {
				if (layer.type == GraphicsLayerType::Default) {
					layer.texture.load(layer.texturePath, archive_);
				} else if (layer.type == GraphicsLayerType::Bup) {
					layer.texture.loadBup(layer.texturePath, archive_, layer.bupPose);
				}
				layer.dirty = false;
			}
			layer.properties = layer.newProperties;
		}
		SpriteBatch batch;
		for (int i = 0; i < layers_.size(); ++i) {
			auto &layer = layers_[i];
			if (layer.type != GraphicsLayerType::None && layer.texture.valid()) {
				layer.properties.sprite.setTexture(layer.texture);
				batch.add(layer.properties.sprite, layer.properties.transform);
			}
		}
		if (!msg_.done()) {
			batch.add(msg_.msgSprite_, msg_.msgTransform_);
		}
		lock.unlock();
		batch.render();
	}
private:
	MessageWindow msg_;
	std::mutex graphicsMutex_;
	std::vector<GraphicsLayer> layers_;
	Window &window_;
	Archive &archive_;
};