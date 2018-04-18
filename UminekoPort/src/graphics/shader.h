#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "../data/archive.h"

enum class UniformType {
	None,
	Int,
	Vec2,
	Vec3,
	Vec4
};

struct UniformInfo {
	UniformType type = UniformType::None;
	int intValue;
	glm::vec4 vecValue;
};

class ShaderResource {
public:
	void load(const std::string &path) {
		source_ = preprocessShaderSource(path);

		auto vs = glCreateShader(GL_VERTEX_SHADER);
		auto fs = glCreateShader(GL_FRAGMENT_SHADER);

		if (!compileShader(vs, source_, "UMINEKO_SHADER_VERTEX") || !compileShader(fs, source_, "UMINEKO_SHADER_FRAGMENT")) {
			throw std::runtime_error("Unable to compile shader.");
		}

		program_ = glCreateProgram();
		glAttachShader(program_, vs);
		glAttachShader(program_, fs);
		glLinkProgram(program_);
		if (!errorCheckProgram(program_)) {
			glDetachShader(program_, vs);
			glDetachShader(program_, fs);
			glDeleteShader(vs);
			glDeleteShader(fs);
			glDeleteProgram(program_);
			throw std::runtime_error("Unable to link program.");
		}
		glDetachShader(program_, vs);
		glDetachShader(program_, fs);
		glDeleteShader(vs);
		glDeleteShader(fs);
	}
	void bind() {
		glUseProgram(program_);
	}
	void release() {
		glUseProgram(0);
	}
	int uniformLocation(const std::string &name) const {
		return glGetUniformLocation(program_, name.c_str());
	}

	// Avoid using if possible, it's better to use explicit locations instead of looking them up.
	template <typename T>
	void setUniform(const std::string &uniform, const T &value) {
		setUniform(uniformLocation(uniform), value);
	}

	template <typename T>
	void setUniform(uint32_t position, const T &value) {
		static_assert(false); // Unimplemented base function
	}

	template <>
	void setUniform<int>(uint32_t position, const int &value) {
		if (isCached(position)) {
			if (getCached<int>(position) == value) return;
		}
		cache(position, value);
		setUniform1i(position, value);
	}

	template <>
	void setUniform<bool>(uint32_t position, const bool &value) {
		setUniform<int>(position, value);
	}

	template <>
	void setUniform<glm::vec2>(uint32_t position, const glm::vec2 &value) {
		if (isCached(position)) {
			if (getCached<glm::vec2>(position) == value) return;
		}
		cache(position, value);
		setUniform2fv(position, value);
	}

	template <>
	void setUniform<glm::vec3>(uint32_t position, const glm::vec3 &value) {
		if (isCached(position)) {
			if (getCached<glm::vec3>(position) == value) return;
		}
		cache(position, value);
		setUniform3fv(position, value);
	}

	template <>
	void setUniform<glm::vec4>(uint32_t position, const glm::vec4 &value) {
		if (isCached(position)) {
			if (getCached<glm::vec4>(position) == value) return;
		}
		cache(position, value);
		setUniform4fv(position, value);
	}
protected:
	GLuint program_;

	bool compileShader(GLuint shader, const std::string &source, const std::string &guard);
	bool errorCheckShader(GLuint shader);
	bool errorCheckProgram(GLuint program);

	virtual void setUniform1i(uint32_t position, int value) {
		glUniform1i(position, value);
	}
	virtual void setUniform2fv(uint32_t position, const glm::vec2 &value) {
		glUniform2fv(position, 1, &value[0]);
	}
	virtual void setUniform3fv(uint32_t position, const glm::vec3 &value) {
		glUniform3fv(position, 1, &value[0]);
	}
	virtual void setUniform4fv(uint32_t position, const glm::vec4 &value) {
		glUniform4fv(position, 1, &value[0]);
	}
	// This method is very picky with #include directives, should rewrite it and make it more robust later
	std::string preprocessShaderSource(const std::string &path);

	std::string source_;
	std::vector<UniformInfo> uniforms_;

	template <typename T>
	void cache(uint32_t position, const T &value) {
		if (uniforms_.size() >= position) {
			uniforms_.resize(position + 1);
		}
		UniformInfo &info = uniforms_[position];
		if constexpr (std::is_same<T, int>::value) {
			info.type = UniformType::Int;
			info.intValue = value;
		} else if constexpr (std::is_same<T, glm::vec2>::value) {
			info.type = UniformType::Vec2;
			info.vecValue = glm::vec4(value, 0, 0);
		} else if constexpr (std::is_same<T, glm::vec3>::value) {
			info.type = UniformType::Vec3;
			info.vecValue = glm::vec4(value, 0);
		} else if constexpr (std::is_same<T, glm::vec4>::value) {
			info.type = UniformType::Vec4;
			info.vecValue = value;
		} else {
			throw std::runtime_error("Unsupported uniform type.");
		}
	}

	bool isCached(uint32_t position) {
		if (position >= uniforms_.size() || uniforms_[position].type == UniformType::None) return false;
		return true;
	}

	template <typename T>
	T getCached(uint32_t position) {
		const auto &info = uniforms_[position];
		if constexpr (std::is_same<T, int>::value) {
			return info.intValue;
		} else if constexpr (std::is_same<T, glm::vec2>::value) {
			return glm::vec2(info.vecValue);
		} else if constexpr (std::is_same<T, glm::vec3>::value) {
			return glm::vec3(info.vecValue);
		} else if constexpr (std::is_same<T, glm::vec4>::value) {
			return info.vecValue;
		}
		throw std::runtime_error("Unsupported uniform type.");
	}
private:
	friend class OpenGLViewer;
};

class ShaderWrapper {
public:
	void load(const std::string &path);
	void bind();
	void release();
	int uniformLocation(const std::string &name) const;

	template <typename T>
	void setUniform(uint32_t position, const T &value) {
		resource_->setUniform(position, value);
	}

	void saveCache(const std::string &cacheName);
	void loadCache(const std::string &cacheName);
private:
	std::shared_ptr<ShaderResource> resource_;
};

class ShaderCache {
public:
	static std::shared_ptr<ShaderResource> load(std::string path) {
		std::transform(path.begin(), path.end(), path.begin(), ::tolower);
		auto it = resources_.find(path);
		if (it == resources_.end()) {
			std::shared_ptr<ShaderResource> resource = std::make_shared<ShaderResource>();
			resource->load(path);
			resources_.insert({ path, resource });
			return resource;
		}
		return it->second;
	}
	static void unload(const std::string &path) {

	}

	static std::shared_ptr<ShaderResource> loadCache(const std::string &cacheName) {
		auto iter = cache_.find(cacheName);
		if (iter == cache_.end()) {
			return nullptr;
		}
		return iter->second;
	}

	static void saveCache(const std::string &cacheName, std::shared_ptr<ShaderResource> resource) {
		cache_[cacheName] = resource;
	}
private:
	static std::unordered_map<std::string, std::shared_ptr<ShaderResource>> resources_;
	static std::unordered_map<std::string, std::shared_ptr<ShaderResource>> cache_;
};

typedef ShaderWrapper Shader;