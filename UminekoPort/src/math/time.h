#pragma once

class Time {
public:
	static double deltaTime() {
		return deltaTime_;
	}

	static double fixedDeltaTime() {
		return fixedDeltaTime_;
	}

	static double totalTime() {
		return totalTime_;
	}
private:
	friend class Engine;

	static double deltaTime_;
	static double fixedDeltaTime_;
	static double totalTime_;
};