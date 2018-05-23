#include "engine.h"

#include "../data/archive.h"
#include "../script/script.h"
#include "../window/window.h"
#include "../graphics/shader.h"
#include "../graphics/uniformbuffer.h"
#include "../math/time.h"
#include "../audio/audio.h"
#include "../imgui/glimgui.h"
#include "../graphics/font.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <thread>

const std::string Engine::game = "umi";

void Engine::run() {
	Archive arc;
	if (game == "umi")
		arc.open("data/DATA.ROM");
	else if (game == "chiru")
		arc.open("chiru_data/DATA.ROM");
	else if (game == "higu") {
		arc.open("higurashi_data/DATA.ROM");
		arc.explore();
	}

	Window window;
	window.create(1600, 900, "Umineko Port");

	AudioManager audio(arc);

	GraphicsContext ctx(window, arc, audio);

	Shader preloadShader;
	preloadShader.load("shaders/2d.glsl");
	preloadShader.saveCache("2d");
	preloadShader.load("shaders/text.glsl");
	preloadShader.saveCache("text");

	UniformBuffer::createUniformBuffer<Matrices>("mvp2d", 1);
	UniformBuffer::bindUniformBuffer("mvp2d");
	auto mvp2d = UniformBuffer::uniformBuffer<Matrices>("mvp2d");
	mvp2d->projection = glm::ortho(0.0f, 1920.0f, 1080.0f, 0.0f);
	mvp2d.update();

	UniformBuffer::createUniformBuffer<ShaderTextData>("text", 3);
	UniformBuffer::bindUniformBuffer("text");
	auto text = UniformBuffer::uniformBuffer<ShaderTextData>("text");
	text->progress.x = 0.0f;
	text.update();

	if (game == "umi" || game == "chiru") {
		Font::global().load("default.fnt", arc);
	} else if (game == "higu") {
		Font::global().load("gothic.fnt", arc);
	}

	Script script(ctx, audio, false);
	std::thread scriptThread([&]() {
		script.load("main.snr", arc);
	});

	bool skipping = false;

	Time::fixedDeltaTime_ = dt_;

	WindowEvent event;
	while (window.isOpen()) {
		while (window.pollEvents(event)) {
			if (event.type == WindowEvent::Type::Resized) {
				ctx.resize();
			}
			if (event.type == WindowEvent::Type::KeyReleased) {
				if (event.key == KeyCode::F) {
					window.setFullscreen(!window.isFullscreen());
				}
				if (event.key == KeyCode::S) {
					skipping = false;
				}
				if (event.key == KeyCode::X) {
					arc.explore();
				}
				if (event.key == KeyCode::D) {
					script.decompile();
				}
			}
			if (skipping) {
				ctx.advance();
				if (ctx.messageDone()) {
					script.resume();
				}
			}
			if (event.type == WindowEvent::Type::KeyPressed) {
				if (event.key == KeyCode::S) {
					skipping = true;
				}
				if (event.key == KeyCode::Space || event.key == KeyCode::Return) {
					ctx.advance();
					if (ctx.messageDone()) {
						script.resume();
					} else if (ctx.doneWaitingForMessageSegment()) {
						script.resume();
					}
				}
			}
		}

		frameTime_ = clock.reset();
		if (frameTime_ > 0.25)
			frameTime_ = 0.25;
		Time::totalTime_ += frameTime_;
		Time::deltaTime_ = frameTime_;
		accumulator_ += frameTime_;

		fpsUpdateFreq_ += frameTime_;
		if (fpsUpdateFreq_ >= 0.5) {
			double fps = 1.0 / frameTime_;
			//std::stringstream ss;
			//ss << this->game << " - FPS: " << fps << " (frame time " << frameTime << ")";
			//window.setTitle(ss.str());
			fpsUpdateFreq_ = 0;
		}

		while (accumulator_ >= dt_) {
			accumulator_ -= dt_;

			// fixed update
		}
		ctx.update();
		if (ctx.waitingDone()) {
			script.resume();
			ctx.stopWait();
		}
		if (ctx.transitionDone()) {
			script.resume();
			ctx.endTransitionMode();
		}

		window.clear(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		ctx.render();
		window.blitFramebuffer();

		Framebuffer::bindDrawNull();
		ImGui_ImplGlfwGL3_NewFrame();
		audio.drawDebug();
		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
		window.bindFramebuffer();

		window.swapBuffers();
	}

	script.stop();
	scriptThread.join();
}