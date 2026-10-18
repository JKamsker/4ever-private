// TSocket.h: the few socket calls that differ between Winsock and BSD sockets.
//
// Code uses the BSD calls (socket, connect, select, recv, send, getsockopt) with Winsock's names
// for the rest (SOCKET, INVALID_SOCKET, SOCKET_ERROR, closesocket), plus the helpers below for
// the error codes, non-blocking mode and library start up.
//////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

typedef int TSOCKLEN;

// send() flags: Winsock never raises signals.
#define TSOCK_SEND_FLAGS				0

#define TSOCK_ECONNREFUSED				WSAECONNREFUSED

inline int TSocketError()
{
	return WSAGetLastError();
}

// The call would block: no data yet, or a non-blocking connect still in progress.
inline BOOL TSocketWouldBlock( int nError)
{
	return nError == WSAEWOULDBLOCK ? TRUE : FALSE;
}

inline BOOL TSocketSetNonBlocking( SOCKET sock)
{
	u_long bAsync = 1;
	return ioctlsocket( sock, FIONBIO, &bAsync) != SOCKET_ERROR ? TRUE : FALSE;
}

inline BOOL TSocketStartup()
{
	WSADATA vWSA;
	return WSAStartup( MAKEWORD( 2, 2), &vWSA) ? FALSE : TRUE;
}

inline void TSocketCleanup()
{
	WSACleanup();
}

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

typedef int SOCKET;
typedef socklen_t TSOCKLEN;
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

#define INVALID_SOCKET					(-1)
#define SOCKET_ERROR					(-1)

// send() flags: no SIGPIPE when the peer has closed the connection.
#define TSOCK_SEND_FLAGS				MSG_NOSIGNAL

#define TSOCK_ECONNREFUSED				ECONNREFUSED

inline int closesocket( SOCKET sock)
{
	return close(sock);
}

inline int TSocketError()
{
	return errno;
}

inline BOOL TSocketWouldBlock( int nError)
{
	return nError == EWOULDBLOCK || nError == EAGAIN || nError == EINPROGRESS ? TRUE : FALSE;
}

inline BOOL TSocketSetNonBlocking( SOCKET sock)
{
	int nFlags = fcntl( sock, F_GETFL, 0);
	return nFlags != -1 && fcntl( sock, F_SETFL, nFlags | O_NONBLOCK) != -1 ? TRUE : FALSE;
}

inline BOOL TSocketStartup()
{
	return TRUE;
}

inline void TSocketCleanup()
{
}

#endif
