#ifndef GOROUTINE_FIBER_WINDOWS_H
#define GOROUTINE_FIBER_WINDOWS_H

#include <windows.h>

// -------------------------------------------------------------------------
// type Fiber

typedef struct FiberInstance{}* Fiber;

inline Fiber createFiber(
	LPFIBER_START_ROUTINE lpStartAddress,
	LPVOID lpParameter = NULL, size_t dwStackSize = 0)
{
	if (dwStackSize == 0)
		dwStackSize = 4096;
	return (Fiber)CreateFiberEx(dwStackSize, dwStackSize, 0, lpStartAddress, lpParameter);
}

inline void destroyFiber(Fiber lpFiber)
{
	DeleteFiber(lpFiber);
}

inline void switchToFiber(Fiber self, Fiber switchTo)
{
	SwitchToFiber(switchTo);
}

inline void* getFiberData(Fiber fiber)
{
	return *(void**)fiber;
}

// -------------------------------------------------------------------------
// class ThreadToFiber

class ThreadToFiber
{
public:
	Fiber convert(LPVOID lpParameter = NULL)
	{
		return (Fiber)ConvertThreadToFiber(lpParameter);
	}

	void unconvert()
	{
		ConvertFiberToThread();
	}
};

// -------------------------------------------------------------------------

#endif /* GOROUTINE_FIBER_WINDOWS_H */

