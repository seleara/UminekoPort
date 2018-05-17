#pragma once

#include <GL/glew.h>

#include "texture.h"

class Framebuffer {
public:
	void create(int width, int height);

	void resize(int width, int height);

	void bindDraw();
	void bindRead();
	static void bindDrawNull();
	static void bindReadNull();

	static Framebuffer *getBound();

	Texture &texture();
private:
	friend class Window;

	static bool checkFramebufferStatus();

	Texture texture_;
	GLuint fbo_, depthRb_;

	static Framebuffer *current_, *currentRead_;
	static glm::ivec2 backBufferSize_;
};