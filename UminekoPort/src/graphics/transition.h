#pragma once

#include <string>

#include "../data/archive.h"
#include "uniformbuffer.h"
#include "shader.h"

struct ShaderTransition {
	glm::vec4 progress; // x
};

class Transition {
public:
	Transition(Archive &archive) : archive_(archive) {}

	void transition(uint32_t frames) {
		useMask_ = false;
		isTransitioning_ = true;
		transitionSpeed_ = frames / 60.0;
		transitionProgress_ = 0;
	}

	void transition(const std::string &maskFilename, uint32_t frames) {
		useMask_ = true;
		maskDirty_ = true;
		maskFilename_ = maskFilename;
		isTransitioning_ = true;
		transitionSpeed_ = frames / 60.0;
		transitionProgress_ = 0;
	}

	bool transitionDone() const {
		return isTransitioning_ && transitionProgress_ >= 1.0;
	}

	bool isTransitioning() const {
		return isTransitioning_;
	}

	void endTransitionMode() {
		isTransitioning_ = false;
		transitionProgress_ = 0;
		transitionSpeed_ = 0;
	}

	void update() {
		if (isTransitioning_) {
			transitionProgress_ += Time::deltaTime() / transitionSpeed_;
		}

		auto trans = UniformBuffer::uniformBuffer<ShaderTransition>("trans");
		trans->progress.x = static_cast<float>(transitionProgress_);
		trans.update();
	}

	void mix(Framebuffer &previous, Framebuffer &next, VertexBuffer<float> &vertexBuffer) {
		Shader shader;
		shader.loadCache("gc_transition");
		shader.bind();

		glActiveTexture(GL_TEXTURE0);
		previous.texture().bind();

		glActiveTexture(GL_TEXTURE0 + 1);
		next.texture().bind();

		auto trans = UniformBuffer::uniformBuffer<ShaderTransition>("trans");
		if (useMask_) {
			if (maskDirty_) {
				transitionMask_.loadMsk(maskFilename_, archive_, true);
				maskDirty_ = false;
			}
			trans->progress.y = 1.0f;

			glActiveTexture(GL_TEXTURE0 + 2);
			transitionMask_.bind();
		} else {
			trans->progress.y = 0.0f;
		}
		trans.update();

		vertexBuffer.draw(Primitives::TriangleStrip, 0, 4);
	}
private:
	bool isTransitioning_ = false;
	double transitionSpeed_ = 0;
	double transitionProgress_ = 0;

	bool useMask_ = false;
	bool maskDirty_ = false;
	std::string maskFilename_;

	Texture transitionMask_;

	Archive &archive_;
};