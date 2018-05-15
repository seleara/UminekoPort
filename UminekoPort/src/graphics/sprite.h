#pragma once

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
	Sprite();
	//Texture texture;
	Anchor anchor = Anchor::TopLeft;
	Pivot pivot = Pivot::TopLeft;
	glm::vec4 textureRect = glm::vec4(0, 0, 1, 1);
	glm::ivec2 pivotOffset;
	glm::vec4 color = glm::vec4(1, 1, 1, 1);

	void setTexture(Texture texture);
	const Texture &texture() const;

	const glm::ivec2 calcPivot() const;

	const glm::vec4 rect() const;

private:
	Texture texture_;
};