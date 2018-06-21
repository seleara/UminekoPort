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
#include "../graphics/framebuffer.h"

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

struct Png;

class Window {
public:
	void create(int width, int height, const std::string &title);
	bool isOpen() const;
	bool pollEvents(WindowEvent &event);
	void clear(const glm::vec4 &color);
	void blitFramebuffer();
	void swapBuffers();
	void setTitle(const std::string &title);
	void setIcon(const Png &png);
	const glm::ivec2 fboSize() const {
		return fboSize_;
	}
	void bindFramebuffer() {
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, singlesampleFbo_);
		singlesampleFbo_.bindDraw();
	}
	void setFullscreen(bool fullscreen);
	bool isFullscreen();
protected:
	void pushEvent(WindowEvent &&event);
	bool popEvent(WindowEvent &event);
private:
	friend class GraphicsContext;

	std::deque<WindowEvent> eventQueue_;
	GLFWwindow *window_;

	static void resizeCallback_(GLFWwindow *window, int width, int height) {
		auto *ptr = (static_cast<Window *>(glfwGetWindowUserPointer(window)));
		ptr->resizeCallback(width, height);
	}

	void resizeCallback(int width, int height);

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

	glm::ivec2 size_;
	glm::ivec2 fboSize_;
	glm::vec2 mousePosition_;

	glm::ivec2 windowSize_;
	glm::ivec2 windowPos_;

	// Single sample rendering
	//GLuint singlesampleTex_;
	//GLuint singlesampleFbo_;
	//GLuint singlesampleDepthRb_;
	Framebuffer singlesampleFbo_;
};