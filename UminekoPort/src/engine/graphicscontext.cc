#include "graphicscontext.h"

#include "../graphics/shader.h"
#include "../graphics/uniformbuffer.h"

GraphicsContext::GraphicsContext(Window &window, Archive &archive, AudioManager &audio) : window_(window), archive_(archive), audio_(audio) {
	prevFramebuffer_.create(window_.fboSize().x, window_.fboSize().y);

	nextFramebuffer_.create(window_.fboSize().x, window_.fboSize().y);

	layers_.resize(0x20);
	for (auto &layer : layers_) {
		layer.newProperties.sprite.anchor = Anchor::Bottom;
		layer.newProperties.sprite.pivot = Pivot::Bottom;
	}
	newLayers_.resize(0x20);
	std::copy(layers_.begin(), layers_.end(), newLayers_.begin());

	msg_.init(archive, audio);

	Shader gcShader, transitionShader;
	gcShader.load("shaders/gc.glsl");
	gcShader.saveCache("gc");
	transitionShader.load("shaders/gc_transition.glsl");
	transitionShader.saveCache("gc_transition");

	UniformBuffer::createUniformBuffer<ShaderTransition>("trans", 2);
	UniformBuffer::bindUniformBuffer("trans");
	auto trans = UniformBuffer::uniformBuffer<ShaderTransition>("trans");
	trans->progress.x = 0;
	trans.update();
}

void GraphicsContext::resize() {
	prevFramebuffer_.resize(window_.fboSize().x, window_.fboSize().y);
	nextFramebuffer_.resize(window_.fboSize().x, window_.fboSize().y);
}

void GraphicsContext::update() {
	if (waiting_) {
		waitTime_ -= Time::deltaTime();
	}
	if (isTransitioning_) {
		transitionProgress_ += Time::deltaTime() / transitionSpeed_;
	}

	auto trans = UniformBuffer::uniformBuffer<ShaderTransition>("trans");
	trans->progress.x = transitionProgress_;
	trans.update();

	msg_.update();
}

void GraphicsContext::render() {
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

	auto addToBatch = [&](GraphicsLayer &layer) {
		if (layer.type != GraphicsLayerType::None && layer.texture.valid()) {
			layer.properties.sprite.setTexture(layer.texture);
			layer.properties.transform.position.x = static_cast<float>(layer.properties.offset.x);
			layer.properties.transform.position.y = static_cast<float>(layer.properties.offset.y);
			batch.add(layer.properties.sprite, layer.properties.transform);
		}
	};

	// Render previous state
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFramebuffer_);
	prevFramebuffer_.bindDraw();
	for (int i = 0; i < layers_.size(); ++i) {
		auto &layer = layers_[i];
		addToBatch(layer);
	}
	batch.render();
	batch.clear();

	// Render next state
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, nextFramebuffer_);
	nextFramebuffer_.bindDraw();
	for (int i = 0; i < newLayers_.size(); ++i) {
		auto &layer = newLayers_[i];
		addToBatch(layer);
	}
	batch.render();
	batch.clear();

	window_.bindFramebuffer();

	glDisable(GL_DEPTH_TEST);
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	auto x2 = 1920.0f;
	auto y2 = 1080.0f;
	auto tx2 = 1.0f;
	auto ty2 = 1.0f;
	float vertices[] = {
		0.0f, 0.0f, 0.0f, ty2, 1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, y2, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		x2, 0.0f, tx2, ty2, 1.0f, 1.0f, 1.0f, 1.0f,
		x2, y2, tx2, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	VertexBuffer<float> vb;
	vb.upload(vertices, (2 + 2 + 4) * 4);
	vb.bind();
	vb.setAttribute(0, 2, 8, 0);
	vb.setAttribute(1, 2, 8, 2);
	vb.setAttribute(2, 4, 8, 4);

	Shader shader;
	if (isTransitioning_) {
		shader.loadCache("gc_transition");
		shader.bind();

		glActiveTexture(GL_TEXTURE0);
		prevFramebuffer_.texture().bind();

		glActiveTexture(GL_TEXTURE0 + 1);
		nextFramebuffer_.texture().bind();

		auto trans = UniformBuffer::uniformBuffer<ShaderTransition>("trans");
		if (useMask_) {
			if (maskDirty_) {
				transitionMask_.loadMsk(maskFilename_, archive_, true);
				maskDirty_ = false;
			}
			trans->progress.y = 1.0f;

			glActiveTexture(GL_TEXTURE0 + 2);
			transitionMask_.bind();
		} else {
			trans->progress.y = 0.0f;
		}
		trans.update();

		vb.draw(Primitives::TriangleStrip, 0, 4);
		vb.release();

	} else {
		shader.loadCache("gc");
		shader.bind();

		glActiveTexture(GL_TEXTURE0);
		prevFramebuffer_.texture().bind();

		vb.draw(Primitives::TriangleStrip, 0, 4);
		vb.release();
	}
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
	glEnable(GL_DEPTH_TEST);

	//lock.unlock();
	msg_.render();
}