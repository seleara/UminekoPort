#pragma once

#include "uniformbuffer.h"

class GLUniformBuffer {
public:
	struct UniformBlockInfo {
		uint32_t bindingPoint;
		GLuint buffer;
		size_t size;
		void *data;

		~UniformBlockInfo() {
			if (data)
				delete[] data;
		}

		friend GLUniformBuffer;
	};

	static void createUniformBuffer(const std::string &name, uint32_t bindingPoint, void *data, size_t size) {
		std::cout << "First: " << *((static_cast<float *>(data)) + 0) << std::endl;
		std::cout << "UBO size: " << size << std::endl;
		blocks_[name] = UniformBlockInfo{ bindingPoint, 0, size, nullptr };
		auto &block = blocks_[name];
		block.data = data;
		std::cout << "First: " << *((static_cast<float *>(data)) + 0) << std::endl;
		std::cout << "First: " << *((static_cast<float *>(block.data)) + 0) << std::endl;
		glCreateBuffers(1, &(block.buffer));
		glNamedBufferData(block.buffer, size, 0, GL_DYNAMIC_DRAW);
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 4; ++j) {
				for (int k = 0; k < 4; ++k) {
					std::cout << *(static_cast<float *>(block.data) + (i * 16) + j * 4 + k) << " ";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
		}
	}

	static void *uniformBuffer(const std::string &name) {
		auto iter = blocks_.find(name);
		if (iter == blocks_.end()) return nullptr;
		return iter->second.data;
	}

	static UniformBlockInfo *uniformBlock(const std::string &name) {
		auto iter = blocks_.find(name);
		if (iter == blocks_.end()) return nullptr;
		return &iter->second;
	}

	static void updateUniformBuffer(const std::string &name) {
		auto iter = blocks_.find(name);
		if (iter == blocks_.end()) return;
		auto &block = iter->second;
		glNamedBufferSubData(block.buffer, 0, block.size, block.data);
	}

	static void bindUniformBuffer(const std::string &name) {
		auto iter = blocks_.find(name);
		if (iter == blocks_.end()) return;
		auto &block = iter->second;
		glBindBufferRange(GL_UNIFORM_BUFFER, block.bindingPoint, block.buffer, 0, block.size);
	}
private:
	static std::map<std::string, UniformBlockInfo> blocks_;
};