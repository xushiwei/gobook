#ifndef GOROUTINE_SERVICE_H
#define GOROUTINE_SERVICE_H

// -------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#include "service/windows.h"
#else
#include "service/linux.h"
#endif

// -------------------------------------------------------------------------

#endif /* GOROUTINE_SERVICE_H */

