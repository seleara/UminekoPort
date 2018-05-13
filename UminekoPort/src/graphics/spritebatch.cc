#include "spritebatch.h"
#include "sprite.h"
#include "../math/transform.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "texture.h"
#include "shader.h"

void SpriteBatch::add(const Sprite &sprite, const Transform &transform) {
	sprites_.emplace_back(sprite, transform);
}

void SpriteBatch::clear() {
	sprites_.clear();
}

void SpriteBatch::render(const std::string &shaderName) {
	if (sprites_.size() == 0) return;
	auto textures = upload();
	Shader shader;
	shader.loadCache(shaderName);

	//std::cout << textures.size() << std::endl;

	glDisable(GL_DEPTH_TEST);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	shader.bind();
	vertexBuffer_.bind();
	vertexBuffer_.setAttribute(0, 2, 8, 0);
	vertexBuffer_.setAttribute(1, 2, 8, 2);
	vertexBuffer_.setAttribute(2, 4, 8, 4);
	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < textures.size(); ++i) {
		auto p = textures[i];
		auto *p2 = (i >= textures.size() - 1) ? nullptr : &textures[i + 1];
		size_t start = p.first;
		size_t end = (p2) ? p2->first : sprites_.size();

		p.second.bind();
		//std::cout << "Draw " << (start * 6) << ", " << ((end - start) * 6) << std::endl;
		vertexBuffer_.draw(Primitives::Triangles, start * 6, (end - start) * 6);
	}
	vertexBuffer_.release();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);

	glEnable(GL_DEPTH_TEST);
}

std::vector<std::pair<size_t, Texture>> SpriteBatch::upload() {
	std::vector<std::pair<size_t, Texture>> textures;
	if (sprites_.size() == 0) return textures;
	int vs = (2 + 2 + 4);
	int stride = vs * 6;
	float *buffer = new float[sprites_.size() * stride];

	// Sort the sprites by texture
	std::sort(sprites_.begin(), sprites_.end(), [](const SpritePair &a, const SpritePair &b) -> bool {
		float az = a.second.position.z;
		float bz = b.second.position.z;
		auto aid = a.first.texture().id();
		auto bid = b.first.texture().id();
		return (az < bz) || ((az == bz) && (aid < bid));
	});

	auto setBuf = [&](int spriteIndex, int triangleIndex, float x, float y, int tx, int ty, const glm::vec4 &c) {
		auto offset = spriteIndex * stride + triangleIndex * vs;
		buffer[offset + 0] = x;
		buffer[offset + 1] = y;
		buffer[offset + 2] = static_cast<float>(tx);
		buffer[offset + 3] = static_cast<float>(ty);
		buffer[offset + 4] = c.r;
		buffer[offset + 5] = c.g;
		buffer[offset + 6] = c.b;
		buffer[offset + 7] = c.a;
	};
	int i = 0;
	const auto &windowSize = glm::ivec2(1920, 1080);//Window::main().size();
	for (const auto &st : sprites_) {
		const auto &s = st.first;
		const auto &t = st.second;
		//const auto &r = glm::rect(s.offset().x, s.offset().y, s.offset().x + s.size().x, s.offset().y + s.size().y);
		const auto &dim = s.texture().size();
		const auto &rect = s.rect();
		/*glm::vec4 rect;
		rect.x = s.rect().x + s.rect().z * s.textureRect.x;
		rect.y = s.rect().y + s.rect().w * s.textureRect.y;
		rect.z = s.rect().z * s.textureRect.z;
		rect.w = s.rect().w * s.textureRect.w;*/
		glm::vec2 origin(0, 0);
		glm::vec2 posMod(1, 1);
		switch (s.anchor) {
		case Anchor::TopLeft:
			break;
		case Anchor::Top:
			origin.x = windowSize.x / 2;
			break;
		case Anchor::TopRight:
			origin.x = windowSize.x;
			posMod.x = -1;
			break;
		case Anchor::Left:
			origin.y = windowSize.y / 2;
			break;
		case Anchor::Center:
			origin.x = windowSize.x / 2;
			origin.y = windowSize.y / 2;
			break;
		case Anchor::Right:
			origin.x = windowSize.x;
			origin.y = windowSize.y / 2;
			posMod.x = -1;
			break;
		case Anchor::BottomLeft:
			origin.y = windowSize.y;
			posMod.y = -1;
			break;
		case Anchor::Bottom:
			origin.x = windowSize.x / 2;
			origin.y = windowSize.y;
			posMod.y = -1;
			break;
		case Anchor::BottomRight:
			origin.x = windowSize.x;
			origin.y = windowSize.y;
			posMod.x = -1;
			posMod.y = -1;
			break;
		}
		auto pivot = s.calcPivot();
		glm::vec2 topLeft(origin.x + posMod.x * t.position.x - pivot.x * t.scale.x, origin.y + posMod.y * t.position.y - pivot.y * t.scale.y);
		glm::vec2 bottomRight(topLeft.x + (rect.z - rect.x) * t.scale.x, topLeft.y + (rect.w - rect.y) * t.scale.y);
		glm::vec2 topRight(bottomRight.x, topLeft.y);
		glm::vec2 bottomLeft(topLeft.x, bottomRight.y);
		glm::vec2 offsetPivot = topLeft + glm::vec2(pivot);
		const auto &c = s.color;
		auto z = t.position.z;
		if (textures.size() == 0 || (textures.back().second.id() != s.texture().id()))
			textures.emplace_back(i, s.texture());

		/*glm::mat4 rot;
		rot = glm::translate(rot, -glm::vec3(offsetPivot, 0));
		rot = rot * glm::toMat4(s.transform().rotation);
		rot = glm::translate(rot, glm::vec3(offsetPivot, 0));*/

		glm::mat4 rotMat = glm::toMat4(t.rotation);
		glm::mat4 centerMat = glm::translate(glm::mat4(), glm::vec3(offsetPivot, 0));
		glm::mat4 centerBackMat = glm::translate(glm::mat4(), -glm::vec3(offsetPivot, 0));
		//glm::mat4 transMat = glm::translate(glm::mat4(), transform_.position);
		//glm::mat4 scaleMat = glm::scale(glm::mat4(), transform_.scale);
		glm::mat4 modelMat = centerMat * rotMat * centerBackMat/* * transMat * scaleMat*/;

		topLeft = glm::vec2(modelMat * glm::vec4(topLeft, 0, 1));
		bottomRight = glm::vec2(modelMat * glm::vec4(bottomRight, 0, 1));
		topRight = glm::vec2(modelMat * glm::vec4(topRight, 0, 1));
		bottomLeft = glm::vec2(modelMat * glm::vec4(bottomLeft, 0, 1));

		setBuf(i, 0, bottomLeft.x, bottomLeft.y, rect.x, rect.w, c);
		setBuf(i, 1, bottomRight.x, bottomRight.y, rect.z, rect.w, c);
		setBuf(i, 2, topLeft.x, topLeft.y, rect.x, rect.y, c);
		setBuf(i, 3, topLeft.x, topLeft.y, rect.x, rect.y, c);
		setBuf(i, 4, bottomRight.x, bottomRight.y, rect.z, rect.w, c);
		setBuf(i, 5, topRight.x, topRight.y, rect.z, rect.y, c);

		++i;
	}

	vertexBuffer_.allocate(sprites_.size() * stride);
	vertexBuffer_.copy(buffer, 0, sprites_.size() * stride, 0);
	vertexBuffer_.upload();

	delete[] buffer;

	return textures;
}