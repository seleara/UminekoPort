#pragma once

#include <string>

#include "../math/clock.h"

class Engine {
public:
	void run();

	static const std::string game;

private:
	Clock clock;
	double dt_ = 0.01;
	double frameTime_ = 0;
	double accumulator_ = 0;
	double fpsUpdateFreq_ = 0;
};