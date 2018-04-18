#pragma once

#include <vector>
#include <iostream>

#include <GL/glew.h>

enum class BufferUsage {
	Stream,
	Static,
	Dynamic
};

enum class Primitives {
	Triangles,
	TriangleStrip,
	TriangleFan,
	Quads
};

enum class VertexBufferType {
	Array,
	Element
};

template <typename T>
class VertexBuffer {
public:
	VertexBuffer() : buffer_(0) {}
	~VertexBuffer() {
		if (buffer_) {
			glDeleteBuffers(1, &buffer_);
		}
	}

	void allocate(size_t size) {
		data_.resize(size);
	}

	void copy(const T *source, size_t sourceOffset, size_t size, size_t destOffset) {
		std::copy(source + sourceOffset, source + sourceOffset + size, data_.begin() + destOffset);
	}

	void clear() {
		data_.clear();
	}

	void upload() {
		if (!buffer_) {
			glCreateBuffers(1, &buffer_);
		}
		glNamedBufferData(buffer_, data_.size() * sizeof(T), data_.data(), GL_DYNAMIC_DRAW);
	}

	void create() {
		if (!buffer_) {
			glGenBuffers(1, &buffer_);
		}
	}

	void upload(const void *buffer, size_t size) {
		create();
		bind();
		glBufferData(type_ == VertexBufferType::Array ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, size * sizeof(T), buffer, GL_DYNAMIC_DRAW);
	}

	void bind() {
		glBindBuffer(type_ == VertexBufferType::Array ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, buffer_);
	}

	void release() {
		glBindBuffer(type_ == VertexBufferType::Array ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void setType(VertexBufferType type) {
		type_ = type;
	}

	// For floating point data
	template <typename U = T, typename std::enable_if<std::is_floating_point<U>::value>::type* = nullptr>
	void setAttribute(size_t index, size_t size, size_t stride, size_t offset, bool normalized = false) {
		glVertexAttribPointer(static_cast<GLuint>(index), static_cast<GLint>(size), GL_FLOAT, normalized ? GL_TRUE : GL_FALSE, static_cast<GLsizei>(stride * sizeof(U)), reinterpret_cast<void *>(offset * sizeof(U)));
		glEnableVertexAttribArray(static_cast<GLuint>(index));
	}

	// For integer data
	template <typename U = T, typename std::enable_if<!std::is_floating_point<U>::value>::type* = nullptr>
	void setAttribute(size_t index, size_t size, size_t stride, size_t offset, bool normalized = false) {
		glVertexAttribPointer(static_cast<GLuint>(index), static_cast<GLint>(size), GL_INT, normalized ? GL_TRUE : GL_FALSE, static_cast<GLsizei>(stride * sizeof(U)), reinterpret_cast<void *>(offset * sizeof(U)));
		glEnableVertexAttribArray(static_cast<GLuint>(index));
	}

	void draw(Primitives type, size_t start, size_t count) {
		GLenum glType;
		switch (type) {
		case Primitives::Triangles:
		default:
			glType = GL_TRIANGLES;
			break;
		case Primitives::TriangleStrip:
			glType = GL_TRIANGLE_STRIP;
			break;
		case Primitives::TriangleFan:
			glType = GL_TRIANGLE_FAN;
			break;
		case Primitives::Quads:
			glType = GL_QUADS;
			break;
		}
		glDrawArrays(glType, static_cast<GLint>(start), static_cast<GLsizei>(count));
	}

	template <typename U>
	void draw(Primitives type, size_t start, size_t count, VertexBuffer<U> &indexBuffer) {
		GLenum glType;
		switch (type) {
		case Primitives::Triangles:
		default:
			glType = GL_TRIANGLES;
			break;
		case Primitives::TriangleStrip:
			glType = GL_TRIANGLE_STRIP;
			break;
		case Primitives::TriangleFan:
			glType = GL_TRIANGLE_FAN;
			break;
		case Primitives::Quads:
			glType = GL_QUADS;
			break;
		}
		indexBuffer.bind();
		glDrawElements(glType, static_cast<GLsizei>(count), GL_UNSIGNED_INT, (void *)(start * sizeof(T)));
	}
private:
	VertexBufferType type_ = VertexBufferType::Array;
	GLuint buffer_;
	std::vector<T> data_;
};