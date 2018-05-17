#include "framebuffer.h"

#include <iostream>

Framebuffer *Framebuffer::current_ = nullptr;
Framebuffer *Framebuffer::currentRead_ = nullptr;
glm::ivec2 Framebuffer::backBufferSize_;

void Framebuffer::create(int width, int height) {
	texture_.create(width, height, true);
	//glGenTextures(1, &prevTexture_.resource_->texture_);
	//glBindTexture(GL_TEXTURE_RECTANGLE, prevTexture_.resource_->texture_);
	//glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, window_.fboSize().x, window_.fboSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	//prevTexture_.resource_->size_.x = window_.fboSize().x;
	//prevTexture_.resource_->size_.y = window_.fboSize().y;

	glGenFramebuffers(1, &fbo_);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, texture_.id(), 0);

	glGenRenderbuffers(1, &depthRb_);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRb_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRb_);

	if (!checkFramebufferStatus()) throw std::runtime_error("Framebuffer error.");

	if (current_)
		current_->bindDraw();
	else
		bindDrawNull();
	if (currentRead_)
		currentRead_->bindRead();
	else
		bindReadNull();
}

void Framebuffer::resize(int width, int height) {
	glBindRenderbuffer(GL_RENDERBUFFER, depthRb_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glBindTexture(GL_TEXTURE_2D, texture_.id());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	texture_.resource_->size_.x = width;
	texture_.resource_->size_.y = height;

	if (current_ == this)
		glViewport(0, 0, texture_.size().x, texture_.size().y);
}

void Framebuffer::bindDraw() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
	glViewport(0, 0, texture_.size().x, texture_.size().y);
	current_ = this;
}

void Framebuffer::bindRead() {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
	currentRead_ = this;
}

void Framebuffer::bindDrawNull() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, backBufferSize_.x, backBufferSize_.y);
	current_ = nullptr;
}

void Framebuffer::bindReadNull() {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	currentRead_ = nullptr;
}

Framebuffer *Framebuffer::getBound() {
	return current_;
}

Texture &Framebuffer::texture() {
	return texture_;
}

bool Framebuffer::checkFramebufferStatus() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE) return true;
	if (status == GL_FRAMEBUFFER_UNDEFINED) {
		std::cerr << "OpenGL Error: Framebuffer does not exist." << std::endl;
		return false;
	}
	if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
		std::cerr << "OpenGL Error: One or more framebuffer attachment points are incomplete." << std::endl;
		return false;
	}
	if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
		std::cerr << "OpenGL Error: Framebuffer doesn't have any attachments." << std::endl;
		return false;
	}
	if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) {
		std::cerr << "OpenGL Error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER." << std::endl;
		return false;
	}
	if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) {
		std::cerr << "OpenGL Error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER." << std::endl;
		return false;
	}
	if (status == GL_FRAMEBUFFER_UNSUPPORTED) {
		std::cerr << "OpenGL Error: Internal formats of framebuffer attachments are not supported." << std::endl;
		return false;
	}
	if (status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) {
		std::cerr << "OpenGL Error: Different sample count for different attachments of multisampled framebuffer, or fixed sample locations are disabled." << std::endl;
		return false;
	}
	if (status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS) {
		std::cerr << "OpenGL Error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS." << std::endl;
		return false;
	}
	std::cerr << "Unknown framebuffer error." << std::endl;
	return false;
}