#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <string>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "gluniformbuffer.h"

template <typename U>
class UniformBufferWrapper {
public:
	explicit UniformBufferWrapper(GLUniformBuffer::UniformBlockInfo *block) {
		block_ = block;
	}

	U *operator->() {
		return (U *)(block_->data);
	}

	void update() {
		glNamedBufferSubData(block_->buffer, 0, block_->size, block_->data);
	}

	void bind() {
		glBindBufferRange(GL_UNIFORM_BUFFER, block_->bindingPoint, block_->buffer, 0, block_->size);
	}
private:
	GLUniformBuffer::UniformBlockInfo *block_;
};

class UniformBuffer {
public:
	template <typename U>
	static void createUniformBuffer(const std::string &name, uint32_t bindingPoint) {
		std::cout << "Creating buffer data... ";
		U *data = new U();
		std::cout << "Done." << std::endl;
		createUniformBufferHelper(name, bindingPoint, data, sizeof(U));
	}

	template <typename U>
	static UniformBufferWrapper<U> uniformBuffer(const std::string &name) {
		//return *reinterpret_cast<U *>(uniformBufferHelper(name));
		return UniformBufferWrapper<U>(uniformBlockHelper(name));
	}

	static void updateUniformBuffer(const std::string &name) {
		updateUniformBufferHelper(name);
	}

	static void bindUniformBuffer(const std::string &name) {
		bindUniformBufferHelper(name);
	}
private:
	static void createUniformBufferHelper(const std::string &name, uint32_t bindingPoint, void *data, size_t size) {
		//if (Config::graphicsAPI() == GraphicsAPI::OpenGL) {
		std::cout << "First: " << *((static_cast<float *>(data)) + 0) << std::endl;
		GLUniformBuffer::createUniformBuffer(name, bindingPoint, data, size);
		//} else if (Config::graphicsAPI() == GraphicsAPI::Vulkan) {
		// TODO...
		//}
	}

	static void *uniformBufferHelper(const std::string &name) {
		//if (Config::graphicsAPI() == GraphicsAPI::OpenGL)
		return GLUniformBuffer::uniformBuffer(name);
		//else if (Config::graphicsAPI() == GraphicsAPI::Vulkan) {
		// TODO...
		//	return nullptr;
		//}
	}

	static GLUniformBuffer::UniformBlockInfo *uniformBlockHelper(const std::string &name) {
		//if (Config::graphicsAPI() == GraphicsAPI::OpenGL)
		return GLUniformBuffer::uniformBlock(name);
	}

	static void updateUniformBufferHelper(const std::string &name) {
		//if (Config::graphicsAPI() == GraphicsAPI::OpenGL)
		GLUniformBuffer::updateUniformBuffer(name);
		//else if (Config::graphicsAPI() == GraphicsAPI::Vulkan) {
		// TODO...
		//}
	}

	static void bindUniformBufferHelper(const std::string &name) {
		//if (Config::graphicsAPI() == GraphicsAPI::OpenGL)
		GLUniformBuffer::bindUniformBuffer(name);
		//else if (Config::graphicsAPI() == GraphicsAPI::Vulkan) {
		// TODO...
		//}
	}
};

struct Matrices {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;
};