#include "graphicscontext.h"

#include "../graphics/shader.h"
#include "../graphics/uniformbuffer.h"

GraphicsContext::GraphicsContext(Window &window, Archive &archive) : window_(window), archive_(archive) {
	// Prev framebuffer
	prevTexture_.create(window_.fboSize().x, window_.fboSize().y);
	//glGenTextures(1, &prevTexture_.resource_->texture_);
	//glBindTexture(GL_TEXTURE_RECTANGLE, prevTexture_.resource_->texture_);
	//glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, window_.fboSize().x, window_.fboSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	//prevTexture_.resource_->size_.x = window_.fboSize().x;
	//prevTexture_.resource_->size_.y = window_.fboSize().y;

	glGenFramebuffers(1, &prevFramebuffer_);
	glBindFramebuffer(GL_FRAMEBUFFER, prevFramebuffer_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, prevTexture_.resource_->texture_, 0);

	glGenRenderbuffers(1, &prevDepthRb_);
	glBindRenderbuffer(GL_RENDERBUFFER, prevDepthRb_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_.fboSize().x, window_.fboSize().y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, prevDepthRb_);

	if (!window_.checkFramebufferStatus()) throw std::runtime_error("Framebuffer error.");

	// Next framebuffer
	nextTexture_.create(window_.fboSize().x, window_.fboSize().y);
	//glGenTextures(1, &nextTexture_.resource_->texture_);
	//glBindTexture(GL_TEXTURE_RECTANGLE, nextTexture_.resource_->texture_);
	//glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, window_.fboSize().x, window_.fboSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	//nextTexture_.resource_->size_.x = window_.fboSize().x;
	//nextTexture_.resource_->size_.y = window_.fboSize().y;

	glGenFramebuffers(1, &nextFramebuffer_);
	glBindFramebuffer(GL_FRAMEBUFFER, nextFramebuffer_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, nextTexture_.resource_->texture_, 0);

	glGenRenderbuffers(1, &nextDepthRb_);
	glBindRenderbuffer(GL_RENDERBUFFER, nextDepthRb_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_.fboSize().x, window_.fboSize().y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, nextDepthRb_);

	if (!window_.checkFramebufferStatus()) throw std::runtime_error("Framebuffer error.");

	layers_.resize(0x20);
	for (auto &layer : layers_) {
		layer.newProperties.sprite.anchor = Anchor::Bottom;
		layer.newProperties.sprite.pivot = Pivot::Bottom;
	}
	newLayers_.resize(0x20);
	std::copy(layers_.begin(), layers_.end(), newLayers_.begin());

	msg_.init(archive);

	Shader transitionShader;
	transitionShader.load("shaders/2d_transition.glsl");
	transitionShader.saveCache("2d_transition");

	UniformBuffer::createUniformBuffer<ShaderTransition>("trans", 2);
	UniformBuffer::bindUniformBuffer("trans");
	auto trans = UniformBuffer::uniformBuffer<ShaderTransition>("trans");
	trans->progress.x = 0;
	trans.update();
}

void GraphicsContext::resize() {
	glBindRenderbuffer(GL_RENDERBUFFER, prevDepthRb_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_.fboSize().x, window_.fboSize().y);
	glBindTexture(GL_TEXTURE_2D, prevTexture_.id());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, window_.fboSize().x, window_.fboSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	prevTexture_.resource_->size_.x = window_.fboSize().x;
	prevTexture_.resource_->size_.y = window_.fboSize().y;

	glBindRenderbuffer(GL_RENDERBUFFER, nextDepthRb_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_.fboSize().x, window_.fboSize().y);
	glBindTexture(GL_TEXTURE_2D, nextTexture_.id());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, window_.fboSize().x, window_.fboSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	nextTexture_.resource_->size_.x = window_.fboSize().x;
	nextTexture_.resource_->size_.y = window_.fboSize().y;
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
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFramebuffer_);
	for (int i = 0; i < layers_.size(); ++i) {
		auto &layer = layers_[i];
		addToBatch(layer);
	}
	batch.render();
	batch.clear();

	// Render next state
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, nextFramebuffer_);
	for (int i = 0; i < newLayers_.size(); ++i) {
		auto &layer = newLayers_[i];
		addToBatch(layer);
	}
	batch.render();
	batch.clear();

	window_.bindFramebuffer();
	if (isTransitioning_) {
		auto x2 = window_.fboSize().x;
		auto y2 = window_.fboSize().y;
		float vertices[] = {
			0.0f, 0.0f, 0.0f, y2, 1.0f, 1.0f, 1.0f, 1.0f,
			0.0f, y2, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			x2, 0.0f, x2, y2, 1.0f, 1.0f, 1.0f, 1.0f,
			x2, y2, x2, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		};
		
		glDisable(GL_DEPTH_TEST);

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		Shader shader;
		shader.loadCache("2d_transition");
		shader.bind();

		glActiveTexture(GL_TEXTURE0);
		prevTexture_.bind();

		glActiveTexture(GL_TEXTURE0 + 1);
		nextTexture_.bind();

		VertexBuffer<float> vb;
		vb.upload(vertices, sizeof(vertices));

		vb.bind();
		vb.setAttribute(0, 2, 8, 0);
		vb.setAttribute(1, 2, 8, 2);
		vb.setAttribute(2, 4, 8, 4);
		vb.draw(Primitives::TriangleStrip, 0, 4);
		vb.release();

		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);

		glEnable(GL_DEPTH_TEST);
	} else {
		auto x2 = window_.fboSize().x;
		auto y2 = window_.fboSize().y;
		float vertices[] = {
			0.0f, 0.0f, 0.0f, y2, 1.0f, 1.0f, 1.0f, 1.0f,
			0.0f, y2, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			x2, 0.0f, x2, y2, 1.0f, 1.0f, 1.0f, 1.0f,
			x2, y2, x2, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		};

		glDisable(GL_DEPTH_TEST);

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		Shader shader;
		shader.loadCache("2d");
		shader.bind();

		glActiveTexture(GL_TEXTURE0);
		prevTexture_.bind();

		VertexBuffer<float> vb;
		vb.upload(vertices, sizeof(vertices));

		vb.bind();
		vb.setAttribute(0, 2, 8, 0);
		vb.setAttribute(1, 2, 8, 2);
		vb.setAttribute(2, 4, 8, 4);
		vb.draw(Primitives::TriangleStrip, 0, 4);
		vb.release();

		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);

		glEnable(GL_DEPTH_TEST);
	}

	//lock.unlock();
	if (msg_.visible()) {
		batch.add(msg_.msgSprite_, msg_.msgTransform_);
	}
	batch.render();
}