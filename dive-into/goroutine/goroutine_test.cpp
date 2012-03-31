#define WIN32_LEAN_AND_MEAN 
#include "socket.h"
#include <stdio.h>

void CALLBACK test(LPVOID lpParam)
{
	printf("Fiber::test start\n");

	FiberSetup p(lpParam);

	postQuitMessage(p.self);

	printf("Fiber::test term\n");
}

void CALLBACK init(LPVOID lpParam)
{
	printf("Fiber::init start\n");

	FiberSetup p(lpParam);

	spawnFiber(p.self, test);

	printf("Fiber::init term\n");
}

int main()
{
	IoService ios;
	ios.run(init);
	return 0;
}
