#include "texture.h"

#include <iostream>

//std::set<std::string> TextureCache::cacheCounter_;
std::map<std::string, std::shared_ptr<TextureResource>> TextureCache::cache_;

void TextureResource::create(int width, int height, bool normalized) {
	normalized_ = normalized;
	glGenTextures(1, &texture_);
	auto texEnum = normalized_ ? GL_TEXTURE_2D : GL_TEXTURE_RECTANGLE;
	glBindTexture(texEnum, texture_);
	glTexParameteri(texEnum, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(texEnum, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texEnum, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(texEnum, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(texEnum, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	size_.x = width;
	size_.y = height;
}

void TextureResource::subImage(int x, int y, int width, int height, int bpp, const std::vector<unsigned char> &pixels) {
	auto texEnum = normalized_ ? GL_TEXTURE_2D : GL_TEXTURE_RECTANGLE;
	glBindTexture(texEnum, texture_);
	GLenum formatEnum;
	switch (bpp) {
	case 4:
		formatEnum = GL_RGBA;
		break;
	case 3:
		formatEnum = GL_RGB;
		break;
	case 2:
		formatEnum = GL_RG;
		break;
	case 1:
		formatEnum = GL_RED;
		break;
	default:
		throw std::runtime_error("Unsupported bpp.");
	}
	glTexSubImage2D(texEnum, 0, x, y, width, height, formatEnum, GL_UNSIGNED_BYTE, pixels.data());
}

void TextureResource::load(const std::string &path, Archive &archive) {
	auto pic = archive.getPic(path);
	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_RECTANGLE, texture_);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, pic.width, pic.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pic.pixels.data());
	glObjectLabel(GL_TEXTURE, texture_, static_cast<GLsizei>(path.size()), path.c_str());
	size_.x = pic.width;
	size_.y = pic.height;
}

void TextureResource::load(const char *pixels, int width, int height, int bpp, bool normalized) {
	normalized_ = normalized;
	glGenTextures(1, &texture_);
	auto texEnum = normalized_ ? GL_TEXTURE_2D : GL_TEXTURE_RECTANGLE;
	glBindTexture(texEnum, texture_);
	glTexParameteri(texEnum, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(texEnum, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texEnum, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(texEnum, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLenum internalFormat, format;
	switch (bpp) {
	case 1: {
		internalFormat = GL_RED;
		format = GL_RED;
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
		glTexParameteriv(texEnum, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		break;
	}
	case 4:
		internalFormat = GL_RGBA;
		format = GL_BGRA;
		break;
	default:
		throw std::runtime_error("Unsupported BPP value.");
	}
	glTexImage2D(texEnum, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
	size_.x = width;
	size_.y = height;
}

void TextureResource::loadBup(const std::string &path, Archive &archive, const std::string &pose) {
	auto bup = archive.getBup(path);
	const Bup::SubEntry *entry = nullptr;
	std::cout << "Requested Pose: " << pose << "\n";
	for (const auto &s : bup.subentries) {
		if (s.name == pose) {
			entry = &s;
		}
		std::cout << "Pose: " << s.name << "\n";
	}
	if (!entry) {
		throw std::runtime_error("Invalid Bup pose. Got " + pose + ".");
	}
	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_RECTANGLE, texture_);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, entry->width, entry->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, entry->pixels.data());
	std::string label = path + "_" + pose;
	glObjectLabel(GL_TEXTURE, texture_, static_cast<GLsizei>(label.size()), label.c_str());
	size_.x = entry->width;
	size_.y = entry->height;
}

void TextureResource::loadTxa(const std::string &path, Archive &archive, const std::string &tex) {
	auto txa = archive.getTxa(path);
	const Txa::SubEntry *entry = nullptr;
	std::cout << "Requested Texture: " << tex << "\n";
	for (const auto &s : txa.subentries) {
		if (s.name == tex) {
			entry = &s;
		}
		std::cout << "Texture: " << s.name << "\n";
	}
	if (!entry) {
		throw std::runtime_error("Invalid Txa texture. Got " + tex + ".");
	}
	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_RECTANGLE, texture_);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, entry->width, entry->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, entry->pixels.data());
	std::string label = path + "_" + tex;
	glObjectLabel(GL_TEXTURE, texture_, static_cast<GLsizei>(label.size()), label.c_str());
	size_.x = entry->width;
	size_.y = entry->height;
}

void TextureResource::loadMsk(const std::string &path, Archive &archive, bool normalized) {
	normalized_ = normalized;
	auto texEnum = normalized_ ? GL_TEXTURE_2D : GL_TEXTURE_RECTANGLE;
	auto msk = archive.getMsk(path);
	glGenTextures(1, &texture_);
	glBindTexture(texEnum, texture_);
	glTexParameteri(texEnum, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(texEnum, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texEnum, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(texEnum, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(texEnum, 0, GL_RED, msk.width, msk.height, 0, GL_RED, GL_UNSIGNED_BYTE, msk.pixels.data());
	glObjectLabel(GL_TEXTURE, texture_, static_cast<GLsizei>(path.size()), path.c_str());
	size_.x = msk.width;
	size_.y = msk.height;
}

std::shared_ptr<TextureResource> TextureCache::create(int width, int height, bool normalized) {
	auto resource = std::make_shared<TextureResource>();
	resource->create(width, height, normalized);
	return resource;
}

std::shared_ptr<TextureResource> TextureCache::load(const std::string &path, Archive &archive) {
	auto iter = cache_.find(path);
	if (iter == cache_.end()) {
		auto resource = std::make_shared<TextureResource>();
		resource->load(path, archive);
		if (cache_.size() > 100) {
			for (auto iter = cache_.cbegin(); iter != cache_.cend();) {
				if (iter->second.use_count() == 1) {
					cache_.erase(iter++);
				} else {
					++iter;
				}
			}
		}
		cache_.insert({ path, resource });
		return resource;
	}
	return iter->second;
}

std::shared_ptr<TextureResource> TextureCache::load(const char *pixels, int width, int height, int bpp, bool normalized) {
	auto resource = std::make_shared<TextureResource>();
	resource->load(pixels, width, height, bpp, normalized);
	return resource;
}

std::shared_ptr<TextureResource> TextureCache::loadBup(const std::string &path, Archive &archive, const std::string &pose) {
	auto identifier = path + "_" + pose;
	auto iter = cache_.find(identifier);
	if (iter == cache_.end()) {
		auto resource = std::make_shared<TextureResource>();
		resource->loadBup(path, archive, pose);
		if (cache_.size() > 100) {
			for (auto iter = cache_.cbegin(); iter != cache_.cend();) {
				if (iter->second.use_count() == 1) {
					cache_.erase(iter++);
				} else {
					++iter;
				}
			}
		}
		cache_.insert({ identifier, resource });
		return resource;
	}
	return iter->second;
}

std::shared_ptr<TextureResource> TextureCache::loadTxa(const std::string &path, Archive &archive, const std::string &tex) {
	auto identifier = path + "_" + tex;
	auto iter = cache_.find(identifier);
	if (iter == cache_.end()) {
		auto resource = std::make_shared<TextureResource>();
		resource->loadTxa(path, archive, tex);
		if (cache_.size() > 100) {
			for (auto iter = cache_.cbegin(); iter != cache_.cend();) {
				if (iter->second.use_count() == 1) {
					cache_.erase(iter++);
				} else {
					++iter;
				}
			}
		}
		cache_.insert({ identifier, resource });
		return resource;
	}
	return iter->second;
}

std::shared_ptr<TextureResource> TextureCache::loadMsk(const std::string &path, Archive &archive, bool normalized) {
	auto identifier = path;
	auto iter = cache_.find(identifier);
	if (iter == cache_.end()) {
		auto resource = std::make_shared<TextureResource>();
		resource->loadMsk(path, archive, normalized);
		if (cache_.size() > 100) {
			for (auto iter = cache_.cbegin(); iter != cache_.cend();) {
				if (iter->second.use_count() == 1) {
					cache_.erase(iter++);
				} else {
					++iter;
				}
			}
		}
		cache_.insert({ identifier, resource });
		return resource;
	}
	return iter->second;
}

const glm::ivec2 TextureWrapper::nullSize_;