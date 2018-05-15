#include "sprite.h"

Sprite::Sprite() {}

void Sprite::setTexture(Texture texture) {
	texture_ = texture;
	textureRect = glm::ivec4(0, 0, texture_.size().x, texture_.size().y);
}

const Texture &Sprite::texture() const {
	return texture_;
}

const glm::ivec2 Sprite::calcPivot() const {
	const auto tx = (textureRect.z - textureRect.x);
	const auto ty = (textureRect.w - textureRect.y);
	switch (pivot) {
	case Pivot::TopLeft:
		return pivotOffset;
	case Pivot::Top:
		return glm::ivec2(tx / 2 + pivotOffset.x, pivotOffset.y);
	case Pivot::TopRight:
		return glm::ivec2(tx - pivotOffset.x, pivotOffset.y);
	case Pivot::Left:
		return glm::ivec2(pivotOffset.x, ty / 2 + pivotOffset.y);
	case Pivot::Center:
		return glm::ivec2(tx / 2 + pivotOffset.x, ty / 2 + pivotOffset.y);
	case Pivot::Right:
		return glm::ivec2(tx - pivotOffset.x, ty / 2 + pivotOffset.y);
	case Pivot::BottomLeft:
		return glm::ivec2(pivotOffset.x, ty - pivotOffset.y);
	case Pivot::Bottom:
		return glm::ivec2(tx / 2 + pivotOffset.x, ty - pivotOffset.y);
	case Pivot::BottomRight:
		return glm::ivec2(tx - pivotOffset.x, ty - pivotOffset.y);
	}
}

const glm::vec4 Sprite::rect() const {
	//return glm::vec4(0, 0, texture.size().x, texture.size().y);
	return textureRect;
}