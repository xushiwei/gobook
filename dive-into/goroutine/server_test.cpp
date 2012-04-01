#define WIN32_LEAN_AND_MEAN 
#include "socket.h"
#include <stdio.h>

void CALLBACK session(LPVOID lpParam)
{
	FiberSetup p(lpParam);

	printf("Fiber::session start\n");

	SocketObject s = *(SocketObject*)p.val;
	delete (SocketObject*)p.val;

	for (;;) {
		char buf[64];
		size_t n = s.readSome(p.self, buf, sizeof(buf));
		if (n == 0)
			break;
		s.write(p.self, buf, n);
	}

	printf("Fiber::session term\n");
}

void CALLBACK server(LPVOID lpParam)
{
	FiberSetup p(lpParam);

	printf("Fiber::server start\n");

	SocketObject l = listenSocket(p.self, ":9999");
	if (!l.good()) {
		printf("listenSocket failed!\n");
		return;
	}

	for (;;) {
		SocketObject s = l.accept(p.self);
		if (!s.good()) {
			printf("accept failed!\n");
			continue;
		}
		spawnFiber(p.self, session, new SocketObject(s));
	}

	printf("Fiber::server term\n");
}

int main()
{
	SocketInit sockInit;
	IoService ios;
	ios.run(server);
	return 0;
}
