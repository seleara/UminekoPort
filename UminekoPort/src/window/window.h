#pragma once

#include <deque>
#include <string>
#include <codecvt>
#include <locale>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "input.h"

struct WindowEvent {
	enum class Type {
		None,
		Resized,
		MouseMoved,
		MouseScrolled,
		MousePressed,
		MouseReleased,
		KeyPressed,
		KeyReleased,
		KeyRepeated,
		Text
	} type;
	union {
		glm::vec2 size;
		glm::vec2 position;
	};
	glm::vec2 delta;
	union {
		KeyCode key;
		MouseButton button;
	};
	std::string text;
	WindowEvent() {}
};

class Window {
public:
	void create(int width, int height, const std::string &title);
	bool isOpen() const;
	bool pollEvents(WindowEvent &event);
	void clear(const glm::vec4 &color);
	void swapBuffers();
protected:
	void pushEvent(WindowEvent &&event);
	bool popEvent(WindowEvent &event);
private:
	std::deque<WindowEvent> eventQueue_;
	GLFWwindow *window_;

	static void resizeCallback_(GLFWwindow *window, int width, int height) {
		auto *ptr = (static_cast<Window *>(glfwGetWindowUserPointer(window)));
		ptr->resizeCallback(width, height);
	}

	void resizeCallback(int width, int height) {
		size_ = glm::vec2(width, height);
		fboSize_ = (width <= height) ? glm::vec2(width, width * (9.0 / 16.0)) : glm::vec2(height * (16.0 / 9.0), height);
		WindowEvent e;
		e.type = WindowEvent::Type::Resized;
		e.size = size_;

		//glViewport(0, 0, width, height);
		glViewport(0, 0, fboSize_.x, fboSize_.y);

		//int maxSamples;
		//glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

		//int samples = glm::min(maxSamples, Config::instance().samples());
		//if (samples > 0) --samples;

		/*if (Config::instance().samples() > maxSamples) {
		std::cerr << "Warning: MSAA specified with " << Config::instance().samples() << " samples, only " << maxSamples << " supported." << std::endl;
		}*/

		// Multisampling
		/*glBindRenderbuffer(GL_RENDERBUFFER, multisampleDepthRb_);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multisampleTex_);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, width, height, true);*/

		// Singlesampling
		glBindRenderbuffer(GL_RENDERBUFFER, singlesampleDepthRb_);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fboSize_.x, fboSize_.y);
		glBindTexture(GL_TEXTURE_2D, singlesampleTex_);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fboSize_.x, fboSize_.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		eventQueue_.push_back(std::move(e));
	}

	static void charCallback_(GLFWwindow *window, unsigned int codePoint) {
		auto *ptr = (static_cast<Window *>(glfwGetWindowUserPointer(window)));
		ptr->charCallback(codePoint);
	}

	void charCallback(unsigned int codePoint) {
		WindowEvent e;
		e.type = WindowEvent::Type::Text;
		std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> converter;
		e.text = converter.to_bytes(codePoint);
		eventQueue_.push_back(std::move(e));
	}

	static void keyCallback_(GLFWwindow *window, int key, int scancode, int action, int mods) {
		auto *ptr = (static_cast<Window *>(glfwGetWindowUserPointer(window)));
		ptr->keyCallback(key, scancode, action, mods);
	}

	void keyCallback(int key, int scancode, int action, int mods) {
		WindowEvent e;
		if (action == GLFW_PRESS)
			e.type = WindowEvent::Type::KeyPressed;
		else if (action == GLFW_RELEASE)
			e.type = WindowEvent::Type::KeyReleased;
		else if (action == GLFW_REPEAT)
			e.type = WindowEvent::Type::KeyRepeated;
		else
			return;
		e.key = static_cast<KeyCode>(key);
		eventQueue_.push_back(std::move(e));
	}

	static void mouseButtonCallback_(GLFWwindow *window, int button, int action, int mods) {
		auto *ptr = (static_cast<Window *>(glfwGetWindowUserPointer(window)));
		ptr->mouseButtonCallback(button, action, mods);
	}

	void mouseButtonCallback(int button, int action, int mods) {
		WindowEvent e;
		if (action == GLFW_PRESS)
			e.type = WindowEvent::Type::MousePressed;
		else if (action == GLFW_RELEASE)
			e.type = WindowEvent::Type::MouseReleased;
		else
			return;
		e.button = static_cast<MouseButton>(button);
		e.position = mousePosition_;
		eventQueue_.push_back(std::move(e));
	}

	static void mouseMovedCallback_(GLFWwindow *window, double xPos, double yPos) {
		auto *ptr = (static_cast<Window *>(glfwGetWindowUserPointer(window)));
		ptr->mouseMovedCallback(xPos, yPos);
	}

	void mouseMovedCallback(double xPos, double yPos) {
		WindowEvent e;
		e.type = WindowEvent::Type::MouseMoved;
		e.position = glm::vec2(xPos, yPos);
		e.delta = glm::vec2(xPos - mousePosition_.x, yPos - mousePosition_.y);
		mousePosition_ = e.position;
		eventQueue_.push_back(std::move(e));
	}

	static void mouseScrolledCallback_(GLFWwindow *window, double xOffset, double yOffset) {
		auto *ptr = (static_cast<Window *>(glfwGetWindowUserPointer(window)));
		ptr->mouseScrolledCallback(xOffset, yOffset);
	}

	void mouseScrolledCallback(double xOffset, double yOffset) {
		WindowEvent e;
		e.type = WindowEvent::Type::MouseScrolled;
		e.delta = glm::vec2(xOffset, yOffset);
		eventQueue_.push_back(std::move(e));
	}

	static bool checkFramebufferStatus() {
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status == GL_FRAMEBUFFER_COMPLETE) return true;
		if (status == GL_FRAMEBUFFER_UNDEFINED) {
			std::cerr << "OpenGL Error: Framebuffer does not exist." << std::endl;
			return false;
		}
		if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
			std::cerr << "OpenGL Error: One or more framebuffer attachment points are incomplete." << std::endl;
			return false;
		}
		if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
			std::cerr << "OpenGL Error: Framebuffer doesn't have any attachments." << std::endl;
			return false;
		}
		if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) {
			std::cerr << "OpenGL Error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER." << std::endl;
			return false;
		}
		if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) {
			std::cerr << "OpenGL Error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER." << std::endl;
			return false;
		}
		if (status == GL_FRAMEBUFFER_UNSUPPORTED) {
			std::cerr << "OpenGL Error: Internal formats of framebuffer attachments are not supported." << std::endl;
			return false;
		}
		if (status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) {
			std::cerr << "OpenGL Error: Different sample count for different attachments of multisampled framebuffer, or fixed sample locations are disabled." << std::endl;
			return false;
		}
		if (status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS) {
			std::cerr << "OpenGL Error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS." << std::endl;
			return false;
		}
		std::cerr << "Unknown framebuffer error." << std::endl;
		return false;
	}

	glm::ivec2 size_;
	glm::ivec2 fboSize_;
	glm::vec2 mousePosition_;

	// Single sample rendering
	GLuint singlesampleTex_;
	GLuint singlesampleFbo_;
	GLuint singlesampleDepthRb_;
};