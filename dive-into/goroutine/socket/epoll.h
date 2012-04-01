#ifndef GOROUTINE_SOCKET_EPOLL_H
#define GOROUTINE_SOCKET_EPOLL_H

#include "../service.h"

// -------------------------------------------------------------------------

class SocketInit {};

// -------------------------------------------------------------------------

namespace detail {

inline long getAddress(const char* p, const char* pend)
{
	if (p == pend) {
		return 0;
	}

	char* hostname = (char*)malloc(pend - p);
	memcpy(hostname, p, pend - p);

	struct addrinfo* addrlist;
	int s = 0;
	if ((s = getaddrinfo(hostname, NULL, NULL, &addrlist)) != 0) {
		free(hostname);
		return 0;
	}
	long result = ntohl(((struct sockaddr_in*)addrlist->ai_addr)->sin_addr.s_addr);
	freeaddrinfo(addrlist);
	free(hostname);
	return result;
}

inline initSockaddr(SOCKADDR_IN& si, const char* host)
{
	long ip = 0;
	int port = 0;
	const char* hostend = strchr(host, ':');
	if (hostend != NULL)
	{
		ip = getAddress(host, hostend);
		port = atoi(hostend + 1);
	}

	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	si.sin_addr.s_addr = htonl(ip);
}

} // namespace detail

// -------------------------------------------------------------------------

#endif /* GOROUTINE_SOCKET_EPOLL_H */
