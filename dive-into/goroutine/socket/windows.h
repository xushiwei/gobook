#ifndef GOROUTINE_SOCKET_WINDOWS_H
#define GOROUTINE_SOCKET_WINDOWS_H

#include "../service.h"

#include <winsock2.h>
#include <mswsock.h>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock.lib")

// -------------------------------------------------------------------------

class SocketInit
{
public:
	SocketInit()
	{
		WSADATA wsaData;
		const WORD wVersionRequested = MAKEWORD(2, 2);
		WSAStartup(wVersionRequested, &wsaData);
	}
	~SocketInit()
	{
		WSACleanup();
	}
};

// -------------------------------------------------------------------------

namespace detail {

inline long getAddress(const char* p, const char* pend)
{
	if (p == pend) {
		return 0x7F000001;
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

inline SOCKET createSocket()
{
	int nRet, nBufLen = 0;

	SOCKET sd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	// Disable send buffering on the socket.  Setting SO_SNDBUF
	// to 0 causes winsock to stop bufferring sends and perform
	// sends directly from our buffers, thereby reducing CPU usage.
	setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char*)&nBufLen, sizeof(nBufLen));

	// Disable receive buffering on the socket.  Setting SO_RCVBUF  
	// to 0 causes winsock to stop bufferring receive and perform 
	// receives directly from our buffers, thereby reducing CPU usage. 
	setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char*)&nBufLen, sizeof(nBufLen));

	LINGER lingerStruct;
	lingerStruct.l_onoff = 0;
	lingerStruct.l_linger = 0;
	setsockopt(sd, SOL_SOCKET, SO_LINGER, (char*)&lingerStruct, sizeof(lingerStruct));

	return sd;
}

inline LPFN_ACCEPTEX getAcceptEx(SOCKET s)
{
	static LPFN_ACCEPTEX lpfnAcceptEx;
	if (lpfnAcceptEx == NULL)
	{
		DWORD dwBytes = 0;
		GUID guidAcceptEx = WSAID_ACCEPTEX;
		WSAIoctl(
			s, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidAcceptEx, sizeof(guidAcceptEx),
			&lpfnAcceptEx, sizeof(lpfnAcceptEx), &dwBytes, NULL, NULL);
	}
	return lpfnAcceptEx;
}

struct OVERLAPPED_IO
{
	OVERLAPPED Overlapped;
	size_t result;
};

} // namespace detail

// -------------------------------------------------------------------------

class SocketObject
{
private:
	enum { SOCKADDR_BUFSIZE = sizeof(SOCKADDR_IN) + 16 };

	SOCKET s;

public:
	SocketObject(SOCKET a = INVALID_SOCKET)
		: s(a)
	{
	}
	~SocketObject()
	{
		if (s != INVALID_SOCKET)
			closesocket(s);
	}

public:
	bool good() const
	{
		return s != INVALID_SOCKET;
	}

	void close()
	{
		if (s != INVALID_SOCKET)
		{
			closesocket(s);
			s = INVALID_SOCKET;
		}
	}

	size_t readSome(Fiber self, void* buf, size_t cb)
	{
		detail::OVERLAPPED_IO o;
		memset(&o, 0, sizeof(o));

		DWORD dwFlags = 0;
		WSABUF wsaBuf = { cb, (char*)buf };
		if (WSARecv(s, &wsaBuf, 1, NULL, &dwFlags, &o.Overlapped, NULL) == SOCKET_ERROR)
		{
			const DWORD nRet = WSAGetLastError();
			if (nRet != ERROR_IO_PENDING)
			{
				close();
				return 0;
			}
		}
		getIoService(self)->yield(self);

		return o.result;
	}

	size_t writeSome(Fiber self, const void* buf, size_t cb)
	{
		detail::OVERLAPPED_IO o;
		memset(&o, 0, sizeof(o));

		WSABUF wsaBuf = { cb, (char*)buf };
		if (WSASend(s, &wsaBuf, 1, NULL, 0, &o.Overlapped, NULL) == SOCKET_ERROR)
		{
			const DWORD nRet = WSAGetLastError();
			if (nRet != ERROR_IO_PENDING)
			{
				close();
				return 0;
			}
		}
		getIoService(self)->yield(self);

		return o.result;
	}

	SocketObject accept(Fiber self)
	{
		SOCKET sdAccept = detail::createSocket();

		//
		// pay close attention to these parameters and buffer lengths
		//
		char buffer[SOCKADDR_BUFSIZE*2];
		OVERLAPPED o;
		ZeroMemory(&o, sizeof(o));

		DWORD dwRecvNumBytes = 0;
		LPFN_ACCEPTEX lpfnAcceptEx = detail::getAcceptEx(s);
		if (
			lpfnAcceptEx(
				s, sdAccept, buffer, 0,	SOCKADDR_BUFSIZE, SOCKADDR_BUFSIZE,
				&dwRecvNumBytes, &o) == SOCKET_ERROR
			)
		{
			const DWORD nRet = WSAGetLastError();
			if (nRet != ERROR_IO_PENDING)
			{
				closesocket(sdAccept);
				return INVALID_SOCKET;
			}
		}
		yield(self);

		// When the AcceptEx function returns, the socket sAcceptSocket is  
		// in the default state for a connected socket. The socket sAcceptSocket  
		// does not inherit the properties of the socket associated with  
		// sListenSocket parameter until SO_UPDATE_ACCEPT_CONTEXT is set on  
		// the socket. Use the setsockopt function to set the SO_UPDATE_ACCEPT_CONTEXT  
		// option, specifying sAcceptSocket as the socket handle and sListenSocket  
		// as the option value.  
		if (setsockopt(sdAccept, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&s, sizeof(s)) == SOCKET_ERROR)
		{
			closesocket(sdAccept);
			return INVALID_SOCKET;
		}

		getIoService(self)->bindIoReadWrite((HANDLE)sdAccept);
		return sdAccept;
	}
};

// -------------------------------------------------------------------------

inline SocketObject cerl_call listenSocket(Fiber self, const char* host)
{
	SOCKET sd = detail::createSocket();

	int value = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));

	SOCKADDR_IN si;
	detail::initSockaddr(si, host);

	int nRet = bind(sd, (sockaddr*)&si, sizeof(SOCKADDR_IN));
	if (SOCKET_ERROR != nRet)
	{
		nRet = listen(sd, 5);
		if (SOCKET_ERROR != nRet)
		{
			getIoService(self)->bindIoAccept((HANDLE)sd);
			return sd;
		}
	}

	closesocket(sd);
	return INVALID_SOCKET;
}

// -------------------------------------------------------------------------

#endif /* GOROUTINE_SOCKET_WINDOWS_H */
