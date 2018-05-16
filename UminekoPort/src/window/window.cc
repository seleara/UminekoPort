#include "window.h"

#include <iostream>

#include "../imgui/glimgui.h"

void Window::create(int width, int height, const std::string &title) {
	size_.x = width;
	size_.y = height;

	if (!glfwInit()) {
		std::cerr << "Unable to initialize GLFW." << std::endl;
		throw std::runtime_error("Unable to initialize GLFW.");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
	window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!window_) {
		std::cerr << "Unable to initialize GLFW." << std::endl;
		throw std::runtime_error("Unable to initialize GLFW.");
	}

	glfwMakeContextCurrent(window_);
	glfwSwapInterval(1);

	glfwSetWindowUserPointer(window_, this);
	glfwSetWindowSizeCallback(window_, &Window::resizeCallback_);
	glfwSetCharCallback(window_, &Window::charCallback_);
	glfwSetKeyCallback(window_, &Window::keyCallback_);
	glfwSetMouseButtonCallback(window_, &Window::mouseButtonCallback_);
	glfwSetCursorPosCallback(window_, &Window::mouseMovedCallback_);
	glfwSetScrollCallback(window_, &Window::mouseScrolledCallback_);

	//ImGui_ImplGlfwGL3_Init(window_, true);

	auto glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW:" << glewGetErrorString(glewErr) << std::endl;

		throw std::runtime_error((char *)glewGetErrorString(glewErr));
	}

	ImGui_ImplGlfwGL3_Init(window_, false);

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_DEBUG_OUTPUT);
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glFrontFace(GL_CCW);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	fboSize_ = (width <= height) ? glm::vec2(width, width * (9.0 / 16.0)) : glm::vec2(height * (16.0 / 9.0), height);

	// Singlesampling
	/*glGenTextures(1, &singlesampleTex_);
	glBindTexture(GL_TEXTURE_2D, singlesampleTex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fboSize_.x, fboSize_.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenFramebuffers(1, &singlesampleFbo_);
	glBindFramebuffer(GL_FRAMEBUFFER, singlesampleFbo_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, singlesampleTex_, 0);

	glGenRenderbuffers(1, &singlesampleDepthRb_);
	glBindRenderbuffer(GL_RENDERBUFFER, singlesampleDepthRb_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fboSize_.x, fboSize_.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, singlesampleDepthRb_);

	if (!checkFramebufferStatus()) throw std::runtime_error("Framebuffer error.");

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, singlesampleFbo_);*/

	singlesampleFbo_.create(fboSize_.x, fboSize_.y);
	bindFramebuffer();

	glDisable(GL_MULTISAMPLE);

}

bool Window::isOpen() const {
	return !glfwWindowShouldClose(window_);
}

bool Window::pollEvents(WindowEvent &event) {
	glfwPollEvents();
	return popEvent(event);
}

void Window::clear(const glm::vec4 &color) {
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::swapBuffers() {
	//glfwSwapBuffers(window_);
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);   // Make sure no FBO is set as the draw framebuffer
	Framebuffer::bindDrawNull();

	//if (previousMultisampling_) {
	//	glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampleFbo_); // Make sure your multisampled FBO is the read framebuffer
	//} else {
		//glBindFramebuffer(GL_READ_FRAMEBUFFER, singlesampleFbo_);
		singlesampleFbo_.bindRead();
	//}
	glDrawBuffer(GL_BACK);                       // Set the back buffer as the draw buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	auto wx = size_.x / 2.0 - fboSize_.x / 2.0;
	auto wy = size_.y / 2.0 - fboSize_.y / 2.0;
	wx = 0;
	wy = 0;
	glBlitFramebuffer(0, 0, static_cast<GLint>(fboSize_.x), static_cast<GLint>(fboSize_.y), wx, wy, static_cast<GLint>(wx + fboSize_.x), static_cast<GLint>(wy + fboSize_.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glfwSwapBuffers(window_);

	/*bool multisampling = Config::instance().multisampling();
	if (multisampling != previousMultisampling_) {
		previousMultisampling_ = multisampling;
	}*/

	//if (multisampling) {
	//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, multisampleFbo_);
	//} else {
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, singlesampleFbo_);
	Framebuffer::bindReadNull();
	singlesampleFbo_.bindDraw();
	//}
}

void Window::setTitle(const std::string &title) {
	glfwSetWindowTitle(window_, title.c_str());
}

void Window::pushEvent(WindowEvent &&event) {
	eventQueue_.push_back(std::move(event));
}

bool Window::popEvent(WindowEvent &event) {
	if (eventQueue_.size() > 0) {
		event = eventQueue_.front();
		eventQueue_.pop_front();
		return true;
	}
	return false;
}

void Window::resizeCallback(int width, int height) {
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
	/*glBindRenderbuffer(GL_RENDERBUFFER, singlesampleDepthRb_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fboSize_.x, fboSize_.y);
	glBindTexture(GL_TEXTURE_2D, singlesampleTex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fboSize_.x, fboSize_.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);*/
	singlesampleFbo_.resize(fboSize_.x, fboSize_.y);

	eventQueue_.push_back(std::move(e));
}