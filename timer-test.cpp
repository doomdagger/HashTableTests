#include "timer.h"
#include <cstdio>

int main (int argc, const char ** argv)
{
	Timer timer;
	timer.Start();

	int sum = 0;
	for (int i = 0; i < 1000000000; ++i)
	{
		sum += (1 + i + i*i);
	}

	timer.Stop();

	printf("Sum: %d\nTime: %0.2f ms\n", sum, timer.msAccumulated);

	return 0;
}

