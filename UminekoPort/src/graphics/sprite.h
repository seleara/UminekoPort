#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "texture.h"

enum class Anchor {
	TopLeft,
	Top,
	TopRight,
	Left,
	Center,
	Right,
	BottomLeft,
	Bottom,
	BottomRight
};

enum class Pivot {
	TopLeft,
	Top,
	TopRight,
	Left,
	Center,
	Right,
	BottomLeft,
	Bottom,
	BottomRight
};

struct Sprite {
	Sprite() {}
	//Texture texture;
	Anchor anchor = Anchor::TopLeft;
	Pivot pivot = Pivot::TopLeft;
	glm::vec4 textureRect = glm::vec4(0, 0, 1, 1);
	glm::ivec2 pivotOffset;
	glm::vec4 color = glm::vec4(1, 1, 1, 1);

	void setTexture(Texture texture) {
		texture_ = texture;
		textureRect = glm::ivec4(0, 0, texture_.size().x, texture_.size().y);
	}

	const Texture &texture() const {
		return texture_;
	}

	const glm::ivec2 calcPivot() const {
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

	const glm::vec4 rect() const {
		//return glm::vec4(0, 0, texture.size().x, texture.size().y);
		return textureRect;
	}

private:
	Texture texture_;
};