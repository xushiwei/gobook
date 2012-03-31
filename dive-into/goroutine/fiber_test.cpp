#define WIN32_LEAN_AND_MEAN 
#include "fiber.h"
#include <stdio.h>

Fiber fiberMain;
Fiber fiberTest;

void CALLBACK test(LPVOID lpParam)
{
	printf("now it is in %s\n", (char*)getFiberData(fiberTest));

	switchToFiber(fiberTest, fiberMain);
}

int main()
{
	ThreadToFiber threadToFiber;
	fiberMain = threadToFiber.convert((void*)"MainFiber");

	fiberTest = createFiber(test, (void*)"TestFiber");
	
	switchToFiber(fiberMain, fiberTest);

	printf("back to %s!\n", (char*)getFiberData(fiberMain));

	destroyFiber(fiberTest);

	threadToFiber.unconvert();
	return 0;
}
