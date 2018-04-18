#pragma once

#include <vector>

#include "../data/vertexbuffer.h"
#include "texture.h"

struct Sprite;
struct Transform;
class TextureResource;

class SpriteBatch {
public:
	void add(const Sprite &sprite, const Transform &transform);
	void clear();
	void render();
	size_t size() const {
		return sprites_.size();
	}
private:
	std::vector<std::pair<size_t, Texture>> SpriteBatch::upload();

	typedef std::pair<Sprite, Transform> SpritePair;
	std::vector<std::pair<Sprite, Transform>> sprites_;

	VertexBuffer<float> vertexBuffer_;
};