#define WIN32_LEAN_AND_MEAN 
#include "service.h"
#include <stdio.h>

void CALLBACK test(LPVOID lpParam)
{
	FiberSetup p(lpParam);

	printf("Fiber::test start\n");	
	postQuitMessage(p.self);
	printf("Fiber::test term\n");
}

void CALLBACK init(LPVOID lpParam)
{
	FiberSetup p(lpParam);

	printf("Fiber::init start\n");
	spawnFiber(p.self, test);
	scheduleFiber(p.self); // Ö÷¶¯³öÈÃcpu
	printf("Fiber::init term\n");
}

int main()
{
	IoService ios;
	ios.run(init);
	return 0;
}
