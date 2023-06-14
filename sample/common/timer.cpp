#include "timer.h"

float current_time()
{
	using namespace std::chrono;
	static uint64_t start = 0;
	if (start == 0) start = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	return (float)(ms - start) / 1000.f;
}