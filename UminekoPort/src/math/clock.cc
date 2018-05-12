#include "clock.h"

#ifdef UMI_CLOCK_USE_CHRONO
double Clock::reset() {
	auto cur = std::chrono::high_resolution_clock::now();
	auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(cur - start_).count() / 1000000000.0;
	//std::cout << "Step: " << now.time_since_epoch().count() << ", Frametime: " << dur << std::endl;
	start_ = cur;
	return dur;
}
#else
double Clock::reset() {
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	//return TimePoint(Duration(count.QuadPart * static_cast<Rep>(Period::den) / frequency));
	long long ns = ((count.QuadPart - start_) * static_cast<Rep>(Period::den) / frequency);
	double dur = ns / 1000000000.0;
	start_ = count.QuadPart;
	return dur;
}
#endif