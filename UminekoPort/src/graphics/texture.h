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
	void create(int width, int height, bool normalized = false);
	void load(const std::string &path, Archive &archive);
	void load(const char *pixels, int width, int height, int bpp, bool normalized = false);
	void loadBup(const std::string &path, Archive &archive, const std::string &pose);
	void loadTxa(const std::string &path, Archive &archive, const std::string &tex);
	void loadMsk(const std::string &path, Archive &archive, bool normalized = false);
private:
	friend class TextureWrapper;
	friend class Framebuffer;
	friend class GraphicsContext;

	bool normalized_ = false;
	GLuint texture_;
	glm::ivec2 size_;
};

class TextureCache {
public:
	static std::shared_ptr<TextureResource> create(int width, int height, bool normalized=false);
	static std::shared_ptr<TextureResource> load(const std::string &path, Archive &archive);
	static std::shared_ptr<TextureResource> load(const char *pixels, int width, int height, int bpp, bool normalized = false);
	static std::shared_ptr<TextureResource> loadBup(const std::string &path, Archive &archive, const std::string &pose);
	static std::shared_ptr<TextureResource> loadTxa(const std::string &path, Archive &archive, const std::string &tex);
	static std::shared_ptr<TextureResource> loadMsk(const std::string &path, Archive &archive, bool normalized = false);
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

	void create(int width, int height, bool normalized = false) {
		resource_ = TextureCache::create(width, height, normalized);
	}
	
	void load(const std::string &path, Archive &archive) {
		resource_ = TextureCache::load(path, archive);
	}
	void load(const char *pixels, int width, int height, int bpp, bool normalized = false) {
		resource_ = TextureCache::load(pixels, width, height, bpp, normalized);
	}
	void loadBup(const std::string &path, Archive &archive, const std::string &pose) {
		resource_ = TextureCache::loadBup(path, archive, pose);
	}
	void loadTxa(const std::string &path, Archive &archive, const std::string &tex) {
		resource_ = TextureCache::loadTxa(path, archive, tex);
	}
	void loadMsk(const std::string &path, Archive &archive, bool normalized = false) {
		resource_ = TextureCache::loadMsk(path, archive, normalized);
	}
	void bind() {
		auto glEnum = normalized() ? GL_TEXTURE_2D : GL_TEXTURE_RECTANGLE;
		glBindTexture(glEnum, id());
	}
	void release() {
		auto glEnum = normalized() ? GL_TEXTURE_2D : GL_TEXTURE_RECTANGLE;
		glBindTexture(glEnum, 0);
	}
	GLuint id() const {
		if (!resource_)
			return 0;
		return resource_->texture_;
	}
	const glm::ivec2 &size() const {
		if (!resource_)
			return nullSize_;
		return resource_->size_;
	}

	bool valid() const {
		if (resource_) return true;
		return false;
	}

	bool normalized() const {
		if (!resource_)
			return false;
		return resource_->normalized_;
	}
private:
	friend class Framebuffer;
	friend class GraphicsContext;

	static const glm::ivec2 nullSize_;

	std::shared_ptr<TextureResource> resource_;
};

typedef TextureWrapper Texture;