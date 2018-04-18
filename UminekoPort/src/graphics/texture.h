#pragma once

#include <map>
#include <memory>
#include <string>
#include <set>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "../data/archive.h"

class TextureResource {
public:
	~TextureResource() {
		glDeleteTextures(1, &texture_);
	}
	void load(const std::string &path, Archive &archive);
	void loadBup(const std::string &path, Archive &archive, const std::string &pose);
	void loadTxa(const std::string &path, Archive &archive, const std::string &tex);
private:
	friend class TextureWrapper;

	GLuint texture_;
	glm::ivec2 size_;
};

class TextureCache {
public:
	static std::shared_ptr<TextureResource> load(const std::string &path, Archive &archive);
	static std::shared_ptr<TextureResource> loadBup(const std::string &path, Archive &archive, const std::string &pose);
	static std::shared_ptr<TextureResource> loadTxa(const std::string &path, Archive &archive, const std::string &tex);
private:
	//static std::set<std::string> cacheCounter_;
	static std::map<std::string, std::shared_ptr<TextureResource>> cache_;
};

class TextureWrapper {
public:
	TextureWrapper() {
	}
	
	void load(const std::string &path, Archive &archive) {
		resource_ = TextureCache::load(path, archive);
	}
	void loadBup(const std::string &path, Archive &archive, const std::string &pose) {
		resource_ = TextureCache::loadBup(path, archive, pose);
	}
	void loadTxa(const std::string &path, Archive &archive, const std::string &tex) {
		resource_ = TextureCache::loadTxa(path, archive, tex);
	}
	void bind() {
		glBindTexture(GL_TEXTURE_RECTANGLE, resource_->texture_);
	}
	void release() {
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	}
	GLuint id() const {
		return resource_->texture_;
	}
	const glm::ivec2 &size() const {
		return resource_->size_;
	}

	bool valid() const {
		if (resource_) return true;
		return false;
	}
private:
	std::shared_ptr<TextureResource> resource_;
};

typedef TextureWrapper Texture;