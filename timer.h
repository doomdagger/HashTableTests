#pragma once

#if defined(WIN32)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class Timer
{
public:
	float msAccumulated;
	float ticksToMs;
	__int64 ticksAccumulated;
	__int64 tickStart;

	Timer()
	:	msAccumulated(0.0f),
		ticksToMs(0.0f),
		ticksAccumulated(0),
		tickStart(0)
	{
		__int64 freq;
		QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
		ticksToMs = 1000.0f / float(freq);
	}

	void Start()
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&tickStart);
	}

	void Stop()
	{
		__int64 tickEnd;
		QueryPerformanceCounter((LARGE_INTEGER *)&tickEnd);
		ticksAccumulated += (tickEnd - tickStart);
		msAccumulated = float(ticksAccumulated) * ticksToMs;
	}
};

#elif defined(__linux__)

#include <time.h>
#include <cstdint>

class Timer
{
public:
	float msAccumulated;
	int64_t nsecAccumulated;
	timespec tsStart;

	Timer()
	:	msAccumulated(0.0f),
		nsecAccumulated(0),
		tsStart()
	{
	}

	void Start()
	{
		clock_gettime(CLOCK_REALTIME, &tsStart);
	}

	void Stop()
	{
		timespec tsEnd;
		clock_gettime(CLOCK_REALTIME, &tsEnd);
		nsecAccumulated += 1000000000 * (tsEnd.tv_sec - tsStart.tv_sec) + (tsEnd.tv_nsec - tsStart.tv_nsec);
		msAccumulated = float(nsecAccumulated) * 1e-6f;
	}
};

#endif

