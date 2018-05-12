#include "engine.h"

#include "../data/archive.h"
#include "../script/script.h"
#include "../window/window.h"
#include "../graphics/shader.h"
#include "../graphics/uniformbuffer.h"
#include "../math/time.h"

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
	else if (game == "higu")
		arc.open("higurashi_data/DATA.ROM");
	//arc.explore();

	Window window;
	window.create(1600, 900, "Umineko Port");

	GraphicsContext ctx(window, arc);

	Shader shader;
	shader.load("shaders/2d.glsl");
	shader.saveCache("2d");

	UniformBuffer::createUniformBuffer<Matrices>("mvp2d", 1);
	UniformBuffer::bindUniformBuffer("mvp2d");
	auto mvp2d = UniformBuffer::uniformBuffer<Matrices>("mvp2d");
	mvp2d->projection = glm::ortho(0.0f, 1920.0f, 1080.0f, 0.0f);
	mvp2d.update();

	Script script(ctx, false);
	std::thread scriptThread([&]() {
		script.load("main.snr", arc);
	});

	bool skipping = false;

	WindowEvent event;
	while (window.isOpen()) {
		while (window.pollEvents(event)) {
			if (event.type == WindowEvent::Type::KeyReleased) {
				if (event.key == KeyCode::S) {
					skipping = false;
				}
				if (event.key == KeyCode::X) {
					arc.explore();
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
					}
				}
			}
		}

		frameTime = clock.reset();
		if (frameTime > 0.25)
			frameTime = 0.25;
		Time::totalTime_ += frameTime;
		Time::deltaTime_ = frameTime;
		accumulator += frameTime;

		fpsUpdateFreq += frameTime;
		if (fpsUpdateFreq >= 0.5) {
			double fps = 1.0 / frameTime;
			//std::stringstream ss;
			//ss << this->game << " - FPS: " << fps << " (frame time " << frameTime << ")";
			//window.setTitle(ss.str());
			fpsUpdateFreq = 0;
		}

		while (accumulator >= dt) {
			accumulator -= dt;

			// fixed update
		}
		ctx.update();
		if (ctx.waitingDone()) {
			script.resume();
			ctx.stopWait();
		}

		window.clear(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		ctx.render();
		window.swapBuffers();
	}

	script.stop();
	scriptThread.join();
}