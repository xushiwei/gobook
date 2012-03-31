#ifndef GOROUTINE_FIBER_UCONTEXT_H
#define GOROUTINE_FIBER_UCONTEXT_H

#include <ucontext.h>
#include <stdlib.h>

// -------------------------------------------------------------------------
// type FiberContext

namespace detail {

struct FiberContext
{
	void* param; // must be first member!
	ucontext_t context;
};

} // namespace detail

// -------------------------------------------------------------------------
// type Fiber

#ifndef CALLBACK
#define CALLBACK
#endif

typedef void* LPVOID;
typedef void (*LPFIBER_START_ROUTINE)(LPVOID lpFiberParameter);

typedef struct FiberInstance{}* Fiber;

inline Fiber createFiber(
	LPFIBER_START_ROUTINE lpStartAddress,
	LPVOID lpParameter = NULL, size_t dwStackSize = 0)
{
	using namespace detail;

	typedef void (*p_func)();

	FiberContext* fiber = (FiberContext*)malloc(sizeof(FiberContext));
	const size_t stackSize = (dwStackSize ? dwStackSize : 4096);

	getcontext(&fiber->context);

	fiber->context.uc_stack.ss_sp = malloc(stackSize);
	fiber->context.uc_stack.ss_size = stackSize;
	fiber->param = lpParameter;

	makecontext(&fiber->context, (p_func)lpStartAddress, 1, lpParameter);

	return (Fiber)fiber;
}

inline void destroyFiber(Fiber lpFiber)
{
	ucontext_t& context = ((detail::FiberContext*)lpFiber)->context;
	free(context.uc_stack.ss_sp);
	free(lpFiber);
}

inline void switchToFiber(Fiber self, Fiber switchTo)
{
	swapcontext(
		&((detail::FiberContext*)self)->context,
		&((detail::FiberContext*)switchTo)->context);
}

inline void* getFiberData(Fiber fiber)
{
	return *(void**)fiber;
}

// -------------------------------------------------------------------------
// class ThreadToFiber

class ThreadToFiber
{
private:
	detail::FiberContext fiber;

public:
	Fiber convert(LPVOID lpParameter = NULL)
	{
		getcontext(&fiber.context);
		fiber.param = lpParameter;
		return (Fiber)&fiber;
	}

	void unconvert()
	{
	}
};

// -------------------------------------------------------------------------

#endif /* GOROUTINE_FIBER_UCONTEXT_H */

