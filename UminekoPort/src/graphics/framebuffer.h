#pragma once

#include <GL/glew.h>

#include "texture.h"
#include "../window/window.h"

class Framebuffer {
public:
	void create(int width, int height) {
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
	}

	void resize(int width, int height) {
		glBindRenderbuffer(GL_RENDERBUFFER, depthRb_);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		glBindTexture(GL_TEXTURE_RECTANGLE, texture_.id());
		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		texture_.resource_->size_.x = width;
		texture_.resource_->size_.y = height;
	}

	void bindDraw() {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
	}

	Texture &texture() {
		return texture_;
	}
private:
	static bool checkFramebufferStatus() {
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

	Texture texture_;
	GLuint fbo_, depthRb_;
};