#ifndef GOROUTINE_SOCKET_H
#define GOROUTINE_SOCKET_H

// -------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#include "socket/windows.h"
#else
#include "socket/epoll.h"
#endif

// -------------------------------------------------------------------------

#endif /* GOROUTINE_SOCKET_H */
