#pragma once

#include <string>

#include "../math/clock.h"

class Engine {
public:
	void run();

	static const std::string game;

private:
	Clock clock;
	double dt = 0.01;
	double frameTime = 0;
	double accumulator = 0;
	double fpsUpdateFreq = 0;
};