#include "engine.h"

#include "../data/archive.h"
#include "../script/script.h"
#include "../window/window.h"
#include "../graphics/shader.h"
#include "../graphics/uniformbuffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <thread>

void Engine::run() {
	Archive arc;
	arc.open("data/DATA.ROM");
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

		window.clear(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		ctx.render();
		window.swapBuffers();
	}

	script.stop();
	scriptThread.join();
}