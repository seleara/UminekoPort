#pragma once

#include <cstdint>
#include <map>

#include <GL/glew.h>

class GLUniformBuffer {
public:
	struct UniformBlockInfo {
		uint32_t bindingPoint;
		GLuint buffer;
		size_t size;
		void *data;

		~UniformBlockInfo();

		friend GLUniformBuffer;
	};

	static UniformBlockInfo * createUniformBuffer(const std::string &name, uint32_t bindingPoint, void *data, size_t size);
	static void *uniformBuffer(const std::string &name);
	static UniformBlockInfo *uniformBlock(const std::string &name);
	static void updateUniformBuffer(const std::string &name);
	static void bindUniformBuffer(const std::string &name);
private:
	static std::map<std::string, UniformBlockInfo> blocks_;
};