#pragma once

#define UMI_CLOCK_USE_CHRONO

#ifdef UMI_CLOCK_USE_CHRONO

#include <chrono>

#else

#include <Windows.h>
#undef min
#undef max

namespace detail {

const long long frequency = []() -> long long {
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}();

}

#endif

class Clock {
public:
	Clock() {
		reset();
	}

	/*double seconds() const {
	auto cur = now();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(cur - start_).count() / 1000000000.0;
	}*/

	double reset();
private:
#ifdef UMI_CLOCK_USE_CHRONO
	std::chrono::time_point<std::chrono::steady_clock> start_;
#else
	typedef std::chrono::time_point<Clock> TimePoint;

	TimePoint now() const {
		LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		return TimePoint(Duration(count.QuadPart * static_cast<Rep>(Period::den) / frequency));
	}

	typedef long long Rep;
	typedef std::nano Period;
	typedef std::chrono::duration<Rep, Period> Duration;
	long long start_;
#endif
};