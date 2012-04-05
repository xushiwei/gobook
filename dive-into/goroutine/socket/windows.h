#ifndef GOROUTINE_SOCKET_WINDOWS_H
#define GOROUTINE_SOCKET_WINDOWS_H

#include "../service.h"

#include <stdlib.h>
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock")

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
	if (p == pend)
	{
		return 0;
	}
	return 0x7F000001; // notimpl
}

inline void initSockaddr(SOCKADDR_IN& si, const char* host)
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

typedef BOOL (*WINAPI LPFN_ACCEPTEX)(
  SOCKET sListenSocket,
  SOCKET sAcceptSocket,
  LPVOID lpOutputBuffer,
  DWORD dwReceiveDataLength,
  DWORD dwLocalAddressLength,
  DWORD dwRemoteAddressLength,
  LPDWORD lpdwBytesReceived,
  LPOVERLAPPED lpOverlapped
);

inline LPFN_ACCEPTEX getAcceptEx(SOCKET s)
{
	static LPFN_ACCEPTEX lpfnAcceptEx;
	if (lpfnAcceptEx == NULL)
	{
		DWORD dwBytes = 0;
		GUID guidAcceptEx = {0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}; // WSAID_ACCEPTEX
		WSAIoctl(
			s, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidAcceptEx, sizeof(guidAcceptEx),
			&lpfnAcceptEx, sizeof(lpfnAcceptEx), &dwBytes, NULL, NULL);
	}
	return lpfnAcceptEx;
}

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
		detail::OverlappedIo* o = new detail::OverlappedIo;
		memset(o, 0, sizeof(*o));
		o->fiber = self;

		DWORD dwFlags = 0;
		WSABUF wsaBuf = { cb, (char*)buf };
		if (WSARecv(s, &wsaBuf, 1, NULL, &dwFlags, &o->Overlapped, NULL) == SOCKET_ERROR)
		{
			const DWORD nRet = WSAGetLastError();
			if (nRet != ERROR_IO_PENDING)
			{
				delete o;
				close();
				return 0;
			}
		}
		getIoService(self)->yield(self);

		return o->result;
	}

	size_t writeSome(Fiber self, const void* buf, size_t cb)
	{
		detail::OverlappedIo* o = new detail::OverlappedIo;
		memset(o, 0, sizeof(*o));
		o->fiber = self;

		WSABUF wsaBuf = { cb, (char*)buf };
		if (WSASend(s, &wsaBuf, 1, NULL, 0, &o->Overlapped, NULL) == SOCKET_ERROR)
		{
			const DWORD nRet = WSAGetLastError();
			if (nRet != ERROR_IO_PENDING)
			{
				delete o;
				close();
				return 0;
			}
		}
		getIoService(self)->yield(self);

		return o->result;
	}

	size_t write(Fiber self, const void* buf, size_t cb)
	{
		size_t total = 0;
		while (cb > 0) {
			size_t n = writeSome(self, buf, cb);
			if (n == 0)
				break;
			total += n;
			buf = (char*)buf + n;
			cb -= n;
		}
		return total;
	}

	SocketObject accept(Fiber self)
	{
		SOCKET sdAccept = detail::createSocket();

		//
		// pay close attention to these parameters and buffer lengths
		//
		char buffer[SOCKADDR_BUFSIZE*2];
		detail::OverlappedAccept* o = new detail::OverlappedAccept;
		ZeroMemory(o, sizeof(*o));
		o->fiber = self;

		DWORD dwRecvNumBytes = 0;
		detail::LPFN_ACCEPTEX lpfnAcceptEx = detail::getAcceptEx(s);
		if (
			lpfnAcceptEx(
				s, sdAccept, buffer, 0,	SOCKADDR_BUFSIZE, SOCKADDR_BUFSIZE,
				&dwRecvNumBytes, &o->Overlapped) == SOCKET_ERROR
			)
		{
			const DWORD nRet = WSAGetLastError();
			if (nRet != ERROR_IO_PENDING)
			{
				closesocket(sdAccept);
				delete o;
				return INVALID_SOCKET;
			}
		}
		IoService* service = getIoService(self);
		service->yield(self);

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

		service->bindIoReadWrite((HANDLE)sdAccept);
		return sdAccept;
	}
};

// -------------------------------------------------------------------------

inline SocketObject listenSocket(Fiber self, const char* host)
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

inline SocketObject dialSocket(Fiber self, const char* host)
{
	SOCKET sd = detail::createSocket();

	SOCKADDR_IN si;
	detail::initSockaddr(si, host);

	int nRet = connect(sd, (sockaddr*)&si, sizeof(si));
	if (SOCKET_ERROR != nRet)
	{
		getIoService(self)->bindIoReadWrite((HANDLE)sd);
		return sd;
	}

	closesocket(sd);
	return INVALID_SOCKET;
}

// -------------------------------------------------------------------------

#endif /* GOROUTINE_SOCKET_WINDOWS_H */
