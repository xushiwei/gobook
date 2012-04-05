#define WIN32_LEAN_AND_MEAN 
#include "socket.h"
#include <stdio.h>

void CALLBACK client(LPVOID lpParam)
{
	FiberSetup p(lpParam);

	printf("Fiber::client start\n");

	SocketObject s = dialSocket(p.self, "localhost:9999");
	if (!s.good())
	{
		printf("dialSocket failed!\n");
		return;
	}

	for (int i = 0; i < 5; ++i)
	{
		s.write(p.self, "Hello!", 6);

		char buf[64];
		size_t n = s.readSome(p.self, buf, sizeof(buf) - 1);
		buf[n] = '\0';
		printf("read: %s\n", buf);
	}

	s.close();
	printf("Fiber::client term\n");
}

int main()
{
	SocketInit sockInit;
	IoService ios;
	ios.run(client);
	return 0;
}
