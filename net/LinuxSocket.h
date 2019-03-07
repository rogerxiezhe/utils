

#ifndef _SYSTEM_LINUXSOCKET_H
#define _SYSTEM_LINUXSOCKET_H
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define PORT_INVALID_SOCKET -1


typedef int port_socket_t; 
inline bool Port_CommSystemStatup ()
{
	return true;
}


inline bool Port_CommSystemCleanUp ()
{
	return true;
}

inline bool Port_SocketOpenTcp(port_socket_t* pSockFd)
{
	port_socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sock)
	{
		return false;
	}

	*pSockFd = sock;
	return true;
}

inline bool Port_SocketOpenUdp(port_socket_t* pSockFd)
{
	port_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == sock)
	{
		return false;
	}

	*pSockFd = sock;
	return true;
}

inline bool Port_SocketClose(port_socket_t sockFd)
{
	int res = close(sockFd);
	if (-1 == res)
	{
		return false;
	}

	return true;
}

inline bool Port_SocketShutDown(port_socket_t sockFd)
{
	return shutdown(sockFd, SHUT_RDWR) == 0;
}

// 停止发送
inline bool Port_SocketShutDownSend(port_socket_t sockFd)
{
	return shutdown(sockFd, SHUT_WR) == 0;
}

inline bool Port_SocketConnect(port_socket_t sockFd, const char* addr, int port)
{
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(addr);
	sa.sin_port = htons(port);

	int res = connect(sockFd, (sockaddr*)&sa, sizeof(sa));
	if (-1 == res)
	{
		if (errno != EINPROGRESS)
		{
			return false;
		}
	}

	return true;
}

inline bool Port_SocketBind(port_socket_t sockFd, const char* addr, int port)
{
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	
	if (NULL == addr || strlen(addr) < 0)
	{
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = INADDR_ANY;
		sa.sin_port = 0;
	}
	else
	{
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = inet_addr(addr);
		sa.sin_port = htons(port);
	}

	int res = bind(sockFd, (sockaddr*)&sa, sizeof(sa));

	return res == -1;
}

inline bool Port_SocketListen(port_socket_t sockFd, int backlog)
{
	int res = listen(sockFd, backlog);
	return res == -1;
}

inline bool Port_SocketAccept(port_socket_t sockFd, port_socket_t* pCliFd, char* remote_addr, 
							size_t remote_addr_size, int* remote_port)
{
	sockaddr_in sa;
	socklen_t len = sizeof(sockaddr);

	int sock = accept(sockFd, (sockaddr*)&sa, &len);
	if (-1 == sock) return false;

	*pCliFd = sock;
	if (remote_addr)
	{
		char* addr = inet_ntoa(sa.sin_addr);
		size_t addr_size = strlen(addr);

		if (addr_size >= remote_addr_size)
		{
			memcpy(remote_addr, addr, remote_addr_size - 1);
			remote_addr[remote_addr_size - 1] = 0;
		}
		else
		{
			memcpy(remote_addr, addr, addr_size + 1);
		}
	}

	if (remote_port)
	{
		*remote_port = ntohs(sa.sin_port);
	}

	return true;
}

// 获得连接的本地地址
inline bool Port_SocketGetSockName(port_socket_t sockFd, char* addr, size_t addr_size, int* port)
{
	sockaddr_in sa;
	socklen_t len = sizeof(sockaddr);

	int res = getsockname(sockFd, (sockaddr*)&sa, &len);
	if (-1 == res) return false;

	if (addr)
	{
		char* s = inet_ntoa(sa.sin_addr);
		size_t size = strlen(s);

		if (size >= addr_size)
		{
			memcpy(addr, s, addr_size - 1);
			addr[addr_size - 1] = 0;
		}
		else
		{
			memcpy(addr, s, size + 1);
		}
	}

	if (port)
	{
		*port = ntohs(sa.sin_port);
	}

	return true;
}

// 获得连接的对端地址
inline bool Port_SocketGetPeerName(port_socket_t sockFd, char* addr, size_t addr_size, int* port)
{
	sockaddr_in sa;
	socklen_t len = sizeof(sockaddr);

	int res = getpeername(sockFd, (sockaddr*)&sa, &len);
	if (-1 == res) return false;

	if (addr)
	{
		char* s = inet_ntoa(sa.sin_addr);
		size_t size = strlen(s);

		if (size >= addr_size)
		{
			memcpy(addr, s, addr_size - 1);
			addr[addr_size - 1] = 0;
		}
		else
		{
			memcpy(addr, s, size + 1);
		}
	}

	if (port)
	{
		*port = ntohs(sa.sin_port);
	}

	return true;	
}

inline int Port_SocketSend(port_socket_t sockFd, const char* buffer, size_t size)
{
	ssize_t res = send(sockFd, buffer, size, 0);
	return (int)res;
}

inline int Port_SocketSendAsync(port_socket_t sockFd, const char* buffer, size_t size, bool* would_block)
{
	*would_block = false;
	ssize_t res = send(sockFd, buffer, size, 0);
	if (-1 == res)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			*would_block = true;
		}
		return -1;
	}

	return (int)res;
}

inline bool Port_SocketReceive(port_socket_t sockFd, char* buffer, size_t size, size_t* pReadSize)
{
	ssize_t res = recv(sockFd, buffer, size, 0);
 	if (-1 == res) return false;

 	*pReadSize = res;
 	return true;
}

inline int Port_SocketSendTo(port_socket_t sockFd, const char* buffer, size_t size, const char* addr, int port)
{
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(addr);
	sa.sin_port = htons(port);

	int res = sendto(sockFd, buffer, size, 0, (sockaddr*)&sa, sizeof(sockaddr));
	return (int)res;
}

inline bool Port_SocketReceiveFrom(port_socket_t sockFd, char* buffer, size_t size, char* remote_addr,
								size_t remote_addr_size, int* remote_port, size_t* pReadSize)
{
	sockaddr_in sa;
	socklen_t len = sizeof(sockaddr);

	int res = recvfrom(sockFd, buffer, (int)size, 0, (sockaddr*)&sa, &len);
	if (-1 == res) return false;

	*pReadSize = res;

	if (remote_addr)
	{
		char* addr = inet_ntoa(sa.sin_addr);
		size_t addr_size = strlen(addr);

		if (addr_size >= remote_addr_size)
		{
			memcpy(remote_addr, addr, remote_addr_size - 1);
			remote_addr[remote_addr_size - 1] = 0;
		}
		else
		{
			memcpy(remote_addr, addr, addr_size + 1);
		}
	}

	if (remote_port)
	{
		*remote_port = ntohs(sa.sin_port);
	}

	return true;
}

inline bool Port_SocketSelectRead(port_socket_t sockFd, int wait_ms, bool* pReadFlag, bool* pExceptFlag)
{
	struct pollfd pfd;
	pfd.fd = sockFd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	int res = poll(&pfd, 1, wait_ms);
	if (-1 == res) return false;

	*pReadFlag = false;
	*pExceptFlag = false;
	if (res > 0)
	{
		if (pfd.revents & POLLIN) *pReadFlag = true;
		if (pfd.revents & (POLLHUP | POLLERR)) *pExceptFlag = true;
	}

	return true;
}

inline bool Port_SocketSelect(port_socket_t sockFd, int wait_ms, bool* pReadFlag, bool* pWriteFlag, bool* pExceptFlag)
{
	struct pollfd pfd;
	pfd.fd = sockFd;
	pfd.events = POLLIN | POLLOUT;
	pfd.revents = 0;

	int res = poll(&pfd, 1, wait_ms);
	if (-1 == res) return false;

	*pReadFlag = false;
	*pWriteFlag = false;
	*pExceptFlag = false;

	if (res > 0)
	{
		if (pfd.revents & POLLIN) *pReadFlag = true;
		if (pfd.revents & POLLOUT) *pWriteFlag = true;
		if (pfd.revents & (POLLHUP | POLLERR)) *pExceptFlag = true;		
	}

	return true;
}

inline bool Port_SocketSetNonBlocking(port_socket_t sockFd)
{
	int flags = fcntl(sockFd, F_GETFL);

	if (-1 == flags) return false;

	if (fcntl(sockFd, F_SETFL, flags | O_NONBLOCK) == -1) return false;

	return true;
}

inline bool Port_SocketSetBlocking(port_socket_t sockFd)
{
	int flags = fcntl(sockFd, F_GETFL);

	if (-1 == flags) return false;

	if (fcntl(sockFd, F_SETFL, flags | ~O_NONBLOCK) == -1) return false;

	return true; 
}

inline bool Port_SocketSetReuseAddr(port_socket_t sockFd)
{
	int flag = 1;
	int res = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag));

	return -1 == res;
}

inline bool Port_SocketSetKeepAlive(port_socket_t sockFd)
{
	int flag = 1;
	int res = setsockopt(sockFd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flag, sizeof(flag));
	return -1 == res;
}

inline bool Port_SocketSetBroadcast(port_socket_t sockFd)
{
	int flag = 1;
	int res = setsockopt(sockFd, SOL_SOCKET, SO_BROADCAST, (const char*)&flag, sizeof(flag));
	return -1 == res;
}

inline bool Port_SocketSetNoDelay(port_socket_t sockFd)
{
	int flag = 1;
	int res = setsockopt(sockFd, SOL_SOCKET, TCP_NODELAY, (const char*)&flag, sizeof(flag));
	return -1 == res;
}

inline const char* Port_GetBroadcastBindAddr(const char* local_addr, const char* broad_addr)
{
	return broad_addr;
}

inline const char* Port_SocketGetError(char* buffer, size_t size)
{
	return strerror_r(errno, buffer, size);
}

inline int Port_SocketGetErrorId()
{
	return errno;
}
#endif

