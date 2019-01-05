#include "gluniformbuffer.h"

#include <iostream>

GLUniformBuffer::UniformBlockInfo::~UniformBlockInfo() {
	if (data)
		delete[] data;
}

GLUniformBuffer::UniformBlockInfo *GLUniformBuffer::createUniformBuffer(const std::string &name, uint32_t bindingPoint, void *data, size_t size) {
	std::cout << "First: " << *((static_cast<float *>(data)) + 0) << std::endl;
	std::cout << "UBO size: " << size << std::endl;
	blocks_[name] = UniformBlockInfo { bindingPoint, 0, size, nullptr };
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
	return &block;
}

void *GLUniformBuffer::uniformBuffer(const std::string &name) {
	auto iter = blocks_.find(name);
	if (iter == blocks_.end()) return nullptr;
	return iter->second.data;
}

GLUniformBuffer::UniformBlockInfo *GLUniformBuffer::uniformBlock(const std::string &name) {
	auto iter = blocks_.find(name);
	if (iter == blocks_.end()) return nullptr;
	return &iter->second;
}

void GLUniformBuffer::updateUniformBuffer(const std::string &name) {
	auto iter = blocks_.find(name);
	if (iter == blocks_.end()) return;
	auto &block = iter->second;
	glNamedBufferSubData(block.buffer, 0, block.size, block.data);
}

void GLUniformBuffer::bindUniformBuffer(const std::string &name) {
	auto iter = blocks_.find(name);
	if (iter == blocks_.end()) return;
	auto &block = iter->second;
	glBindBufferRange(GL_UNIFORM_BUFFER, block.bindingPoint, block.buffer, 0, block.size);
}