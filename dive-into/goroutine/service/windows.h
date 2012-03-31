#ifndef GOROUTINE_SERVICE_WINDOWS_H
#define GOROUTINE_SERVICE_WINDOWS_H

// -------------------------------------------------------------------------

class IoService;

namespace detail {

struct FiberData
{
	IoService* service;
	void* startParam;
	Fiber self;
};

struct OverlappedIo
{
	OVERLAPPED Overlapped;
	Fiber fiber;
	size_t result;
};

struct OverlappedAccept
{
	OVERLAPPED Overlapped;
	Fiber fiber;
};

} // namespace detail

inline IoService* getIoService(Fiber self)
{
	return ((detail::FiberData*)getFiberData(self))->service;
}

// -------------------------------------------------------------------------

class IoService
{
private:
	enum { QUIT_MASK = 0x40000000 };

	Fiber self;
	ThreadToFiber threadToFiber;
	HANDLE iocp;
	DWORD quitLockRef;

private:
	Fiber doCreateFiber(LPFIBER_START_ROUTINE lpStartAddress, void* startParam = NULL, size_t dwStackSize = 0)
	{
		detail::FiberData* p = new detail::FiberData;
		p->service = this;
		p->startParam = startParam;
		p->self = createFiber(lpStartAddress, p, dwStackSize);
		++quitLockRef;
		return p->self;
	}

	void doDeleteFiber(Fiber fiberToKill)
	{
		detail::FiberData* p = (detail::FiberData*)getFiberData(fiberToKill);
		delete p;
		deleteFiber(fiberToKill);
		--quitLockRef;
	}

public:
	enum COMPLETION_KEY
	{
		ClientIoDeleteFiber	= -5,	// lp = fiberToDelete;
		ClientIoSchedule	= -4,	// dw = sn, lp = fiberToWakeup;
		ClientIoAccept		= -3,	// lp = overlapped;
		ClientIoReadWrite	= -2,	// dw = bytes, lp = overlapped;
		ClientIoQuit		= -1,
		ClientIoNoop		= 0,
	};

public:
	IoService()
	{
		self = threadToFiber.convert(NULL);
		iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		quitLockRef = 0;
	}

	void bindIoAccept(HANDLE fd)
	{
		CreateIoCompletionPort(fd, iocp, (ULONG_PTR)ClientIoAccept, 0);
	}

	void bindIoReadWrite(HANDLE fd)
	{
		CreateIoCompletionPort(fd, iocp, (ULONG_PTR)ClientIoReadWrite, 0);
	}

	void postQuitMessage()
	{
		PostQueuedCompletionStatus(iocp, 0, (ULONG_PTR)ClientIoQuit, NULL);
	}

	void postDeleteFiberMessage(Fiber self)
	{
		PostQueuedCompletionStatus(iocp, 0, (ULONG_PTR)ClientIoDeleteFiber, (LPOVERLAPPED)self);
	}

	void postScheduleFiberMessage(Fiber fiber)
	{
		PostQueuedCompletionStatus(iocp, 0, (ULONG_PTR)ClientIoSchedule, (LPOVERLAPPED)fiber);
	}

	bool processMessage(DWORD bytes, ULONG_PTR key, LPOVERLAPPED overlapped)
	{
		switch (key)
		{
		case ClientIoReadWrite:
			{
				OverlappedIo* o = (OverlappedIo*)overlapped;
				o->result = bytes;
				switchToFiber(self, o->fiber);
				delete o;
			}
			break;
		case ClientIoAccept:
			{
				OverlappedAccept* o = (OverlappedAccept*)overlapped;
				switchToFiber(self, o->fiber);
				delete o;
			}
			break;
		case ClientIoSchedule:
			switchToFiber(self, (Fiber)overlapped);
			break;
		case ClientIoDeleteFiber:
			doDeleteFiber((Fiber)overlapped);
			break;
		case ClientIoQuit:
			quitLockRef |= QUIT_MASK;
			break;
		default:
			return false;
		}
		return true;
	}

	Fiber startFiber(LPFIBER_START_ROUTINE lpStartAddress, void* startParam = NULL, size_t dwStackSize = 0)
	{
		Fiber fiber = doCreateFiber(lpStartAddress, startParam, dwStackSize);
		switchToFiber(self, fiber);
		return fiber;
	}

	void exitFiber(Fiber self)
	{
		postDeleteFiberMessage(self);
		switchToFiber(self, this->self);
	}

	void yield(Fiber self)
	{
		switchToFiber(self, this->self);
	}

public:
	void run(LPFIBER_START_ROUTINE lpStartAddress, void* startParam = NULL, size_t dwStackSize = 0)
	{
		Fiber fiber = doCreateFiber(lpStartAddress, startParam, dwStackSize);
		postScheduleFiberMessage(fiber);
		for (;;)
		{
			if (quitLockRef == QUIT_MASK)
				break;
		
			DWORD bytes;
			ULONG_PTR key = ClientIoNoop;
			LPOVERLAPPED overlapped;
			GetQueuedCompletionStatus(iocp, &bytes, &key, &overlapped, INFINITE);
			processMessage(bytes, key, overlapped);
		}
	}
};

// -------------------------------------------------------------------------

inline Fiber startFiber(Fiber self, LPFIBER_START_ROUTINE lpStartAddress, void* startParam = NULL, size_t dwStackSize = 0)
{
	return getIoService(self)->startFiber(self, lpStartAddress, startParam, dwStackSize);
}

inline void exitFiber(Fiber self)
{
	getIoService(self)->exitFiber(self);
}

// -------------------------------------------------------------------------
// class FiberSetup

class FiberSetup
{
public:
	Fiber self;
	void* val;
	IoService* service;

public:
	FiberSetup(LPVOID lpParam)
	{
		p = (detail::FiberData*)lpParam;
		self = p->self;
		val = p->startParam;
		service = p->service;
	}

	~FiberSetup()
	{
		service->exitFiber(self);
	}
};

// -------------------------------------------------------------------------

#endif /* GOROUTINE_SERVICE_WINDOWS_H */
