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
	void create(int width, int height);
	void load(const std::string &path, Archive &archive);
	void loadBup(const std::string &path, Archive &archive, const std::string &pose);
	void loadTxa(const std::string &path, Archive &archive, const std::string &tex);
private:
	friend class TextureWrapper;
	friend class GraphicsContext;

	GLuint texture_;
	glm::ivec2 size_;
};

class TextureCache {
public:
	static std::shared_ptr<TextureResource> create(int width, int height);
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

	//void createEmpty() {
	//	resource_ = std::make_shared<TextureResource>();
	//}

	void create(int width, int height) {
		resource_ = TextureCache::create(width, height);
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
		glBindTexture(GL_TEXTURE_RECTANGLE, id());
	}
	void release() {
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	}
	GLuint id() const {
		if (!resource_)
			return 0;
		return resource_->texture_;
	}
	const glm::ivec2 &size() const {
		if (!resource_)
			return glm::ivec2();
		return resource_->size_;
	}

	bool valid() const {
		if (resource_) return true;
		return false;
	}
private:
	friend class GraphicsContext;

	std::shared_ptr<TextureResource> resource_;
};

typedef TextureWrapper Texture;