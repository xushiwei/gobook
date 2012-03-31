#ifndef GOROUTINE_FIBER_H
#define GOROUTINE_FIBER_H

// -------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#include "fiber/windows.h"
#else
#include "fiber/ucontext.h"
#endif

// -------------------------------------------------------------------------

#endif /* GOROUTINE_FIBER_H */

