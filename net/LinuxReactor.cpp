#ifdef __linux__
#include "LinuxSocket.h"
#include "LinuxReactor.h"
#include "TickTimer.h"
#include <sys/socket.h>
#include <sys/epoll.h>

#define EPOLLRDHUP 0x2000

#define INIT_EVENTS_PER_LOOP 32
#define MAX_EVENTS_PER_LOOP 8192
#define TYPE_CONNECTOR 0
#define TYPE_BROADCAST 1
#define TYPE_LISTENER 2
typedef unsigner __int64 uint64_t;
#define MAKE_EV_DATA(type, index, sock)\
		((uint64_t((type << 24) + index) << 32) + uint64_t(sock))

inline CopyString(char* pDest, size_t byte_size, char* pSrc)
{
	const size_t SIZE1 = strlen(pSrc) + 1;

	if (SIZE1 <= byte_size)
	{
		memcpy(pDest, pSrc, SIZE1);
	}
	else
	{
		memcpy(pDest, pSrc, byte_size - 1);
		pDest[byte_size - 1] = 0;
	}
}

enum IO_STATE
{
	IO_STATE_UNKNOWN,
	IO_STATE_WAITCLOSING,
	IO_STATE_CLOSING,
	IO_STATE_MAX,
};

CReactor::CReactor()
{
	m_nEventsPerLoop = INIT_EVENTS_PER_LOOP;
	m_pEvents = new struct epoll_event[m_nEventsPerLoop];
	m_Epoll = -1;
	m_fMinTime = 1.0f;
	m_fCheckTime = 0.0f;
	m_pTimer = new CTickTimer;
}

CReactor::~CReactor()
{
	Stop();
	delete m_pTimer;
	delete[] m_pEvents;
}

bool CReactor::Start()
{
	m_pTimer->Initialize();
	m_Epoll = epoll_create(32000);

	if (-1 == m_Epoll)
	{
		printf("epoll_create failed");
		return false;
	}

	return true;
}

bool CReactor::Stop()
{
	for (size_t k1 = 0; k1 < m_BroadCasts.size(); ++k1)
	{
		if (m_BroadCasts[k1])
			DeleteBroadCast((int)k1);
	}

	for (size_t k2 = 0; k2 < m_Listeners.size(); ++k2)
	{
		if (m_Listeners[k2])
			DeleteListener((int)k2);
	}

	for (size_t k3 = 0; k3 < m_Connectors.size(); ++k3)
	{
		if (m_Connectors[k3])
			DeleteConnector((int)k3);
	}

	m_Closings.clear();
	m_ClosingFrees.clear();

	for (size_t k4 = 0; k4 < m_Timers.size(); ++k4)
	{
		if (m_Timers[k4])
			DeleteTimer((int) k4);
	}

	if (m_Epoll != -1)
	{
		close(m_Epoll);
		m_Epoll = -1;
	}

	return true;
}

int CReactor::CreateBroadCast(const char * local_addr, const char * broadcast_addr, int port, size_t in_buf_len, broadcast_callback cb, void * context)
{
	if (NULL == local_addr || NULL == broadcast_addr || port <= 0)
	{
		printf("%s, param error\n", __FUNCTION__);
		return -1;
	}

	port_socket_t sock;
	if (!Port_SocketOpenUdp(&sock))
	{
		printf("%s, OpenUdp fail\n", __FUNCTION__);
		return -1;
	}

	if (!Port_SocketSetNonBlocking(sock))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		// log
		Port_SocketClose(sock);
		return -1;
	}

	if (!Port_SocketSetReuseAddr(sock))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		// log
		Port_SocketClose(sock);
		return -1;
	}

	if (!Port_SocketSetBlocking(sock))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));	
		// log
		Port_SocketClose(sock);
		return -1;
	}

	const char* bind_addr =Port_GetBroadcastBindAddr(local_addr, broadcast_addr);

	if (!Port_SocketBind(sock, bind_addr, port))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));	
		// log
		Port_SocketClose(sock);
		return -1;
	}

	broadcast_t* pBroad = new broadcast_t;
	memset(pBroad, 0, sizeof(broadcast_t));
	pBroad->Socket = sock;
	CopyString(pBroad->strLocalAddr, sizeof(pBroad->strLocalAddr), local_addr);
	CopyString(pBroad->strBroadAddr, sizeof(pBroad->strBroadAddr), broadcast_addr);
	pBroad->nPort = port;
	pBroad->nInBufferLen = in_buf_len;
	pBroad->pInBuf = new char[in_buf_len];
	pBroad->BroadcastCallback = cb;
	pBroad->pContext = context;

	size_t index;

	if (m_BroadCastFrees.empty())
	{
		index = m_BroadCasts.size();
		m_BroadCasts.push_back(pBroad);
	}
	else
	{
		index = m_BroadCastFrees.back();
		m_BroadCastFrees.pop_back();
		m_BroadCasts[index] = pBroad;
	}
	
	struct epoll_event ev;

	ev.data.u64 = MAKE_EV_DATA(TYPE_BROADCAST, index, sock);
	ev.events = EPOLLIN | EPOLLRDHUP;

	if (epol_ctl(m_Epoll, EPOLL_CTL_ADD, sock, &ev) != 0)
	{
		// log
		DeleteBroadCast((int)index);
		return -1;
	}

	return (int)index;

}

bool CReactor::DeleteBroadCast(int broadcast_id)
{
	if (broadcast_id >= m_BroadCasts.size()) return false;

	broadcast_t* pBroad = m_BroadCasts[broadcast_id];
	if (NULL == pBroad) return false;

	if (pBroad->Socket != PORT_INVALID_SOCKET)
	{
		// remove epoll
		struct epoll_event ev;
		ev.data.fd = pBroad->Socket;
		ev.events = 0;

		if (epoll_ctl(m_Epoll, EPOLL_CTL_DEL, pBroad->Socket, &ev) != 0)
		{
			// log	
		}

		Port_SocketClose(pBroad->Socket);
	}

	delete[] pBroad->pInBuf;
	delete pBroad;
	m_BroadCasts[broadcast_id] = NULL;
	m_BroadCastFrees.push_back(broadcast_id);
}

bool CReactor::SendBroadCast(int broadcast_id, const void * pdata, size_t len)
{
	if (broadcast_id >= m_BroadCasts.size() || !pdata) return false;
	broadcast_t* pBroad = m_BroadCasts[broadcast_id];
	if (!pBroad) return false;

	int res = Port_SocketSendTo(pBroad->Socket, (const char*)pdata, len, pBroad->strBroadAddr, pBroad->nPort);
	if (res < 0)
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		//log
		return false;
	}

	return true;
}

int CReactor::CreateListener(const char* addr, int port, int backlog,
		size_t in_buf_len, size_t out_buf_len, size_t out_buf_max,
		accept_callback accept_cb, close_callback close_cb,
		receive_callback receive_cb, void* context, size_t accept_num,
		int use_buffer_list, int wsabufs, compress_callback compress_cb, bool keep_alive)
{
	if (!addr || port <= 0) return -1;

	port_socket_t sock;
	if (!Port_SocketOpenTcp(&sock))
	{
		//log
		return -1;
	}

	if (!Port_SocketSetNonBlocking(sock))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		//log
		Port_SocketClose(sock);
		return -1;
	}

	if (keep_alive && !Port_SocketSetKeepAlive(sock))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		//log
		Port_SocketClose(sock);
		return -1;
	}

	if (!Port_SocketSetReuseAddr(sock))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		//log
		Port_SocketClose(sock);
		return -1;
	}

	if (!Port_SocketBind(sock, addr, port))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		//log
		Port_SocketClose(sock);
		return -1;	
	}

	if (!Port_SocketListen(sock, backlog))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		//log
		Port_SocketClose(sock);
		return -1;		
	}

	listener_t* pListener = new listener_t;
	memset(pListener, 0, sizeof(listener_t));

	pListener->Socket = sock;
	CopyString(pListener->strAddr, sizeof(pListener->strAddr), addr);
	pListener->nPort = port;
	pListener->nInBufferLen = in_buf_len;
	pListener->nOutBufferLen = out_buf_len;
	pListener->nOutBufferMax = out_buf_max;
	pListener->AcceptCallback = accept_cb;
	pListener->CloseCallback = close_cb;
	pListener->ReceiveCallback = receive_cb;
	pListener->pContext = context;

	size_t index;

	if (m_ListenerFrees.empty())
	{
		index = m_Listeners.size();
		m_Listeners.push_back(pListener);
	}
	else
	{
		index = m_ListenerFrees.back();
		m_ListenerFrees.pop_back();
		m_Listeners[index] = pListener;
	}

	// addto epoll
	struct epoll_event ev;
	ev.data.u64 = MAKE_EV_DATA(TYPE_LISTENER, index, sock);
	ev.events = EPOLLIN;

	if (epoll_ctl(m_Epoll, EPOLL_CTL_ADD, sock, &ev) != 0)
	{
		// log 
		DeleteListener((int)index);
		return -1;
	}

	return (int)index;
}

bool CReactor::DeleteListener(int listener_id)
{
	if (listener_id >= m_Listeners.size()) return false;

	listener_t* pListener = m_Listeners[listener_id];
	if (!pListener) return false;

	if (pListener->Socket != PORT_INVALID_SOCKET)
	{
		// remove epoll
		struct epoll_event ev;

		ev.data.fd = pListener->Socket;
		ev.events = 0;

		if (epoll_ctl(m_Epoll, EPOLL_CTL_DEL, pListener->Socket, &ev) != 0)
		{
			// log remove fail			
		}
		Port_SocketClose(pListener->Socket);
	}

	delete pListener;
	m_Listeners[listener_id] = NULL;
	m_ListenerFrees.push_back((size_t)listener_id);

	return true;
}

int CReactor::GetListenerSock(int listener_id)
{
	if (size_t(listener_id) >= m_Listeners.size()) return PORT_INVALID_SOCKET;

	listener_t* pListener = m_Listeners[listener_id];
	if (!pListener) return PORT_INVALID_SOCKET;

	return pListener->Socket;
}

int CReactor::GetListenerPort(int listener_id)
{
	if (size_t(listener_id) >= m_Listeners.size()) return -1;

	listener_t* pListener = m_Listeners[listener_id];
	if (!pListener) return -1;

	sockaddr_in sa;
	socklen_t len = sizeof(sockaddr);

	int res = getsockname(pListener->Socket, (sockaddr*)&sa, &len);
	if (-1 == res) return -1;

	return ntohs(sa.sin_port);
}

int CReactor::GetConnectorSock(int connector_id)
{
	if (size_t(connector_id) >= m_Connectors.size()) return PORT_INVALID_SOCKET;

	connector_t* pConnector = m_Connectors[connector_id];
	if (!pConnector) return PORT_INVALID_SOCKET;

	return pConnector->Socket;
}

int CReactor::CreateConnector(const char* addr, int port, size_t in_buf_len,
		size_t out_buf_len, size_t out_buf_max, int timeout,
		connect_callback conn_cb, close_callback close_cb,
		receive_callback recv_cb, void* context, int use_buffer_list, int bufs, compress_callback compress_cb)
{
	if (!addr || port <= 0) return -1;

	port_socket_t sock;
	if (!Port_SocketOpenTcp(sock))
	{
		// log
		return -1;
	}

	if (!Port_SocketSetNonBlocking(sock))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		// log
		Port_SocketClose(sock);
		return -1;
	}

	connector_t* pConnector = new connector_t;
	if (!pConnector) return -1;
	memset(pConnector, 0, sizeof(connector_t));

	pConnector->Socket = sock;
	CopyString(pConnector->strAddr, sizeof(pConnector->strAddr), addr);
	pConnector->nPort = port;
	pConnector->nInBufferLen = in_buf_len;
	pConnector->nOutBufferLen = out_buf_len;
	pConnector->nOutBufferMax = out_buf_max;
	pConnector->nTimeout = timeout;
	pConnector->ConnectCallback = conn_cb;
	pConnector->CloseCallback = close_cb;
	pConnector->ReceiveCallback = recv_cb;
	pConnector->pContext = context;
	pConnector->pInBuf = new char[in_buf_len];
	pConnector->pOutBuf = new char[out_buf_len];
	pConnector->nSendEmpty = 1;
	pConnector->pSendBegin = pConnector->pOutBuf;

	size_t index;
	if (m_ConnectorFrees.empty())
	{
		index = m_Connectors.size();
		m_Connectors.push_back(pConnector);
	}
	else
	{
		index = m_ConnectorFrees.back();
		m_ConnectorFrees.pop_back();
		m_Connectors[index] = pConnector;
	}

	pConnector->nEvData = MAKE_EV_DATA(TYPE_CONNECTOR, index, sock);
	// add epoll
	struct epollevent ev;
	ev.data.u64 = pConnector->nEvData;
	ev.events = EPOLLOUT | EPOLLRDHUP;
	if (epoll_ctl(m_Epoll, EPOLL_CTL_ADD, sock, &ev) != 0)
	{
		// log
		DeleteConnector((int)index);
		return -1;
	}

	if (timeout > 0)
	{
		socklen_t len = sizeof(pConnector->tvTimeout);
		int res = getsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&pConnector->tvTimeout, &len);
		if (-1 == res)
		{
			char info[128];
			Port_SocketGetError(info, sizeof(info));
			// log
			DeleteConnector((int)index);
			return -1;
		}

		// 设置连接超时时间
		struct timeval tv = {timeout, 0};
		res = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
		if (-1 == res)
		{
			char info[128];
			Port_SocketGetError(info, sizeof(info));
			// log
			DeleteConnector((int)index);
			return -1;
		}
	}

	if (!Port_SocketConnect(sock, addr, port))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		// log
		DeleteConnector((int)index);
		return -1;		
	}
	
	return (int)index;
	
}

bool CReactor::DeleteConnector(int connector_id)
{
	if (connect_id >= m_Connectors.size()) return false;

	connector_t* pConnector = m_Connectors[connector_id];
	if (NULL == pConnector) return false;

	if (pConnector->Socket != PORT_INVALID_SOCKET)
	{
		// delete epoll event
		struct epoll_event ev;
		ev.data.fd = pConnector->Socket;
		ev.events = 0;

		if (epoll_ctl(m_Epoll, EPOLL_CTL_DEL, pConnector->Socket, &ev) != 0)
		{
			// log 
		}

		Port_SocketClose(pConnector->Socket);
	}

	delete[] pConnector->pOutBuf;
	delete[] pConnector->pInBuf;
	delete pConnector;
	m_Connectors[connector_id] = NULL;
	m_ConnectorFrees.push_back(connector_id);

	return true;
}

bool CReactor::ShutDownConnector(int connector_id, int wait_seconds)
{
	if (connect_id >= m_Connectors.size()) return false;

	connector_t* pConnector = m_Connectors[connector_id];
	if (NULL == pConnector) return false;

	if ((wait_seconds > 0) && (pConnector->nSendRemain > 0))
	{
		pConnector->nState = IO_STATE_WAITCLOSING;
		pConnector->fCounter = 0.0f;
		pConnector->nTimeout = wait_seconds;
		return true;
	}

	return ShutDown(connector_id, (float)wait_seconds);
}

bool CReactor::GetConnected(int connector_id)
{
	if (connect_id >= m_Connectors.size()) return false;

	connector_t* pConnector = m_Connectors[connector_id];
	if (NULL == pConnector) return false;

	return (NULL == pConnector->ConnectCallback);
}

// 扩充发送缓冲区
static bool expand_out_buffer(CReactor::connector_t* pConnect, size_t need_size, bool force)
{
	size_t send_remain = pConnect->nSendRemain;

	if (need_size < pConnect->nSendOffset)
	{
		if (send_remain > 0)
		{
			if (send_remain > 0x100000) // log big memory move

			memmove(pConnect->pOutBuf, pConnect->pSendBegin, send_remain);			
		}
		
		pConnect->pSendBegin = pConnect->pOutBuf;
		pConnect->nSendOffset = 0;
	}
	else
	{
		size_t new_size = pConnect->nOutBufferLen * 2;
		if (new_size < need_size + send_remain)
		{
			new_size = need_size + send_remain;
		}

		if (pConnect->nOutBufferMax > 0 && new_size > pConnect->nOutBufferMax)
		{
			if (!force) return false;
		}

		char* pNewBuf = new char[new_size];
		if (send_remain > 0)
		{
			memcpy(pNewBuf, pConnect->pSendBegin, send_remain);
		}

		delete[] pConnect->pOutBuf;
		pConnect->pOutBuf = pNewBuf;
		pConnect->nOutBufferLen = new_size;
		pConnect->pSendBegin = pNewBuf;
		pConnect->nSendOffset = 0;
	}

	return true;
}

bool CReactor::ForceSend(connector_t * pConnect, size_t need_size)
{
	size_t send_size = 0;
	port_socket_t sock = pConnect->Socket;

	while (pConnect->nSendRemain > 0)
	{
		int res = send(sock, pConnect->pSendBegin, pConnect->nSendRemain, 0);

		if (-1 == res)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			{
				struct pollfd pfd;
				pfd.fd = sock;
				pfd.events = POLLOUT;
				pfd.revents = 0;

				if (poll(&pfd, 1, -1) != 1)
				{
					// log poll failed
					return false;
				}

				continue;
			}
			else
			{
				char info[128];
				Port_SocketGetError(info, sizeof(info));
				// log
				return false;
			}
		}
		
		assert(size_t(res) <= pConnect->nSendRemain);

		pConnect->pSendBegin += res;
		pConnect->nSendOffset += res;
		pConnect->nSendRemain -= res;

		send_size += res;
		if (send_size >= need_size)
		{
			break;
		}		
	}

	// 发送完成,统计置位
	if (0 == pConnect->nSendRemain)
	{
		pConnect->nSendEmpty = 1;
		pConnect->pSendBegin = pConnect->pOutBuf;
		pConnect->nSendOffset = 0;

		// 现在只关注读取
		struct epoll_event ev;
		ev.data.u64 = pConnect->nEvData;
		ev.events = EPOLLIN | EPOLLRDHUP;

		if (epoll_ctl(m_Epoll, EPOLL_CTL_MOD, sock, &ev) != 0)
		{
			// log epoll modify fail
			return false;
		}
	}

	return true;
}

bool CReactor::Send(int connector_id, const void * pdata, size_t len, bool force)
{
	if (connector_id >= m_Connectors.size()) return false;
	connector_t* pConnect = m_Connectors[connector_id];
	if (!pConnect) return false;

	if (pConnect->nState >= IO_STATE_WAITCLOSING) return false;

	size_t used_size = pConnect->nSendOffset + pConnect->nSendRemain;

	if (pConnect->nOutBufferLen - used_size < len)
	{
		if (!expand_out_buffer(pConnect, len, force))
		{
			return false;
		}

		used_size = pConnect->nSendOffset + pConnect->nSendRemain;
	}

	memcpy(pConnect->pOutBuf + used_size, pdata, len);
	pConnect->nSendRemain += len;

	if (pConnect->nSendEmpty == 1)
	{
		struct epoll_ev ev;
		ev.data.u64 = pConnect->nEvData;
		ev.events =EPOLLIN | EPOLLOUT | EPOLLRDHUP;

		if (epoll_ctl(m_Epoll, EPOLL_CTL_MOD, pConnect->Socket, &ev) != 0)
		{
			return false;
		}

		pConnect->nSendEmpty = 0;
	}

	return true;
}

bool CReactor::Send2(int connector_id, const void * pdata1, size_t len1, const void * pdata2, size_t len2, bool force)
{
	if (connector_id >= m_Connectors.size()) return false;
	connector_t* pConnect = m_Connectors[connector_id];
	if (!pConnect) return false;

	if (pConnect->nState >= IO_STATE_WAITCLOSING) return false;

	size_t len = len1 + len2;
	size_t used_size = pConnect->nSendOffset + pConnect->nSendRemain;

	if (pConnect->nOutBufferLen - used_size < len)
	{
		if (!expand_out_buffer(pConnect, len, force))
		{
			return false;
		}

		used_size = pConnect->nSendOffset + pConnect->nSendRemain;
	}

	char* p = pConnect->pOutBuf + pConnect->nSendRemain;
	memcpy(p, pdata1, len1);
	p += len1;
	memcpy(p, pdata2, len2);
	pConnect->nSendRemain += len;

	if (pConnect->nSendEmpty == 1)
	{
		struct epoll_ev ev;
		ev.data.u64 = pConnect->nEvData;
		ev.events =EPOLLIN | EPOLLOUT | EPOLLRDHUP;

		if (epoll_ctl(m_Epoll, EPOLL_CTL_MOD, pConnect->Socket, &ev) != 0)
		{
			return false;
		}

		pConnect->nSendEmpty = 0;
	}

	return true;	
}

int CReactor::CreateTimer(float seconds, timer_callback cb, void * context)
{
	if (seconds < 0.001f)
	{
		return -1;
	}

	timer_t* pTimer = new time_t;

	pTimer->fSeconds = seconds;
	pTimer->fCounter = 0.0f;
	pTimer->TimerCallBack = cb;
	pTimer->pContext = context;

	size_t index;
	if (m_TimerFrees.empty())
	{
		index = m_Timers.size();
		m_Timers.push_back(pTimer);
	}
	else
	{
		index = m_TimerFrees.back();
		m_TimerFrees.pop_back();
		m_Timers[index] = pTimer;
	}

	UpdateMinTime();

	return (int)index;
}

bool CReactor::DeleteTimer(int timer_id)
{
	if (timer_id >= m_Timers.size()) return false;

	timer_t* pTimer = m_Timers[timer_id];
	if (NULL == pTimer) return false;
	
	delete pTimer;
	m_Timers[timer_id] = NULL;
	m_TimerFrees.push_back(timer_id);

	UpdateMinTime();
	return true;
}

void CReactor::UpdateMinTime()
{
	float min_time = 1.0f;
	size_t timer_size = m_Timers.size();

	for (size_t i = 0; i < timer_size; ++i)
	{
		timer_t* pTimer = m_Timers[i];

		if (NULL == pTimer) continue;

		if (pTimer->fSeconds < min_time)
		{
			min_time = pTimer->fSeconds;
		}
	}

	if (min_time < 1.0f)
	{
		m_fMinTime = min_time;
	}
	else
	{
		m_fMinTime = 1.0f;
	}
}

bool CReactor::CloseConnect(size_t index)
{
	if (index >= m_Connectors.size()) return false;

	connector_t* pConnect = m_Connectors[index];
	if (NULL == pConnect) return false;

	close_callback cb = pConnect->CloseCallback;
	void* context = pConnect->pContext;
	int port = pConnect->nPort;
	char addr[32];
	CopyString(addr, sizeof(addr), pConnect->strAddr);
	DeleteConnector((int)index);

	cb(context, (int)index, addr, port);

	return true;
}

bool CReactor::ShutDown(size_t index, float timeout)
{
	if (index >= m_Connectors.size()) return false;

	connector_t* pConnect = m_Connectors[index];
	if (NULL == pConnect) return false;

	if (PORT_INVALID_SOCKET == pConnect->Socket) return false;

	if (!Port_SocketShutDownSend(pConnect->Socket))
	{
		char info[128];
		Port_SocketGetError(info, sizeof(info));
		//log
		CloseConnect(index);
		return false;
	}

	pConnect->nTimeout = timeout;
	pConnect->fCounter = 0.0f;
	pConnect->nState = IO_STATE_WAITCLOSING;

	if (0 == pConnect->nTimeout)
	{
		pConnect->nTimeout = 30;
	}

	if (m_ClosingFrees.empty())
	{
		m_Closings.push_back((int)index);
	}
	else
	{
		size_t pos = m_ClosingFrees.back();
		m_ClosingFrees.pop_back();
		m_Closings[pos] = (int)index;
	}

	return true;
}

void CReactor::EventLoop()
{
	int nfds = epoll_wait(m_Epoll, m_pEvents, m_nEventsPerLoop, (int)(m_fMinTime * 1000.0f));

	if (nfds)
	{
		for (int n = 0; n < nfds; ++n)
		{
			struct epoll_event* ev = &m_pEvents[n];
			int events = ev->events;
			uint64_t ev_data = ev->data.u64;
			unsigned int index = ev_data >> 32;
			unsigned int sock = ev_data & 0xFFFFFFFF;

			if (events & EPOLLIN)
			{
				ProcessRead(index, sock);
			}

			if (events & EPOLLOUT)
			{
				ProcessWrite(index, sock);
			}

			if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
			{
				ProcessError(index, sock, events);
			}
		}

		if (nfds == m_nEventsPerLoop)
		{
			if (m_nEventsPerLoop < MAX_EVENTS_PER_LOOP)
			{
				// 通讯繁忙时增加每次检测的事件数量
				size_t new_event_num = m_nEventsPerLoop * 2;
				struct epoll_event* new_events = new struct epoll_event[new_event_num];

				if(new_events)
				{
					delete[] m_pEvents;
					m_pEvents = new_events;
					m_nEventsPerLoop = new_event_num;
				}
			}
		}
	}
	else if (nfds < 0)
	{
		if (errno != EINTR) // log "epoll wait error"
	}

	float elapse = (float)m_pTimer->GetElapseMilisec() * 0.001f;

	// 检查所有的定时器
	if (!m_Timers.empty())
	{
		size_t timer_size = m_Timers.size();
		for (size_t i = 0; i < timer_size; ++i)
		{
			timer_t* pTimer = m_Timers[i];

			if (NULL == pTimer) continue;

			pTimer->fCounter += elapse;

			if (pTimer->fCounter >= pTimer->fSeconds)
			{
				float seconds = pTimer->fCounter;

				pTimer->fCounter = 0.0f;
				pTimer->TimerCallBack(pTimer->pContext, (int)i, seconds);
			}
		}
	}

	m_fCheckTime += elapse;
	if (m_fCheckTime > 1.0f)
	{
		CheckClosing();
		m_fCheckTime = 0.0f;
	}
}

bool CReactor::ProcessWrite(size_t index, int sock)
{
	assert((index >> 24) & 0xFF == 0);

	connector_t* pConnect = m_Connectors[index];
	if (NULL == pConnect) return false;

	if (pConnect->Socket != sock) return false;

	if (pConnect->ConnectCallback)
	{
		if (pConnect->nTimeout > 0)
		{
			// 恢复发送超时时间
			int res = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&pConnect->tvTimeout, sizeof(pConnect->tvTimeout));
			if (-1 == res)
			{
				char info[128];
				Port_SocketGetError(info, sizeof(info));
				// log
			}
		}
		connect_callback cb = pConnect->ConnectCallback;
		// 连接成功，只回调一次
		pConnect->ConnectCallback = NULL;
		// 连接成功，通知回调
		cb(pConnect->pContext, (int)index, 1, pConnect->strAddr, pConnect->nPort);
	}

	if (pConnect->nSendRemain > 0)
	{
		size_t send_len = pConnect->nSendRemain;
		if (send_len > 0x100000) send_len = 0x100000;
		int res = send(sock, pConnect->pSendBegin, send_len, 0);
		if (-1 == res)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) res = 0;
			else
			{
				char info[128];
				Port_SocketGetError(info, sizeof(info));
				CloseConnect(inde);
				return false;
			}
		}

		assert(size_t(res) <= send_len);

		pConnect->pSendBegin += res;
		pConnect->nSendOffset += res;
		pConnect->nSendRemain -= res;
	}

	if (0 == pConnect->nSendRemain)
	{
		if (pConnect->nOutBufferMax > 0 && pConnect->nOutBufferLen > pConnect->nOutBufferMax)
		{
			// shrink buffer
			char* pNewBuf = new char[pConnect->nOutBufferMax];
			delete[] pConnect->pOutBuf;
			pConnect->nOutBufferLen = pConnect->nOutBufferMax;
			pConnect->pOutBuf = pNewBuf;
		}

		// 所有消息都已发送
		pConnect->nSendEmpty = 1;
		pConnect->pSendBegin = pConnect->pOutBuf;
		pConnect->nSendOffset = 0;

		if (IO_STATE_WAITCLOSING == pConnect->nState)
		{
			ShutDown(index, pConnect->nTimeout);
		}

		struct epoll_event ev;
		ev.data.u64 = pConnect->nEvData;
		ev.events = EPOLLIN | EPOLLRDHUP;

		if (epoll_ctl(m_Epoll, EPOLL_CTL_MOD, sock, &ev) != 0)
		{
			return false;
		}
	}

	return true;
}

bool CReactor::ProcessRead(size_t index, int sock)
{
	int io_type = (index >> 24) & 0xFF;

	if (io_type == TYPE_CONNECTOR)
	{
		connector_t* pConnect = m_Connectors[index];
		if (NULL == pConnect) return false;

		if (pConnect->Socket != sock) return false;

		int res = recv(sock, pConnect->pInBuf, pConnect->nInBufferLen, 0);
		if (-1 == res)
		{
			char info[128];
			Port_SocketGetError(info, sizeof(info));
			// log
			CloseConnect(index);
			return false;
		}

		if (0 == res)
		{
			CloseConnect(index);
			return false;
		}

		pConnect->ReceiveCallback(pConnect->pContext, (int)index, pConnect->pInBuf, res);

		return true;
	}

	if (io_type == TYPE_LISTENER)
	{
		index &= 0x00FFFFFF;
		listener_t* pListener = m_Listeners[index];
		if (NULL == pListener) return false;
		if (pListener->Socket != sock) return false;

		sockaddr_in sa;
		socklen_t len = sizeof(sockaddr);

		port_socket_t new_sock = accept(sock, (sockaddr*)&sa, &len);
		if (PORT_INVALID_SOCKET == new_sock)
		{
			char info[128];
			Port_SocketGetError(info, sizeof(info));
			// log
			return false;
		}

		if (!Port_SocketSetNonBlocking(new_sock))
		{
			char info[128];
			Port_SocketGetError(info, sizeof(info));
			//log
			Port_SocketClose(new_sock);
			return false;
		}

		connector_t* pConnect = new connector_t;
		memset(pConnect, 0, sizeof(connector_t));

		pConnect->Socket = new_sock;
		pConnect->nInBufferLen = pListener->nInBufferLen;
		pConnect->nOutBufferLen = pListener->nOutBufferLen;
		pConnect->nOutBufferMax = pListener->nOutBufferMax;
		CopyString(pConnect->strAddr, sizeof(pConnect->strAddr), inet_ntoa(sa.sin_addr));
		pConnect->nSendEmpty = 1;
		pConnect->pInBuf = new char[pListener->nInBufferLen];
		pConnect->pOutBuf = new char[pListener->nOutBufferLen];
		pConnect->pSendBegin = pConnect->pOutBuf;
		pConnect->nPort = ntohs(sa.sin_port);
		pConnect->CloseCallback = pListener->CloseCallback;
		pConnect->ReceiveCallback = pListener->ReceiveCallback;
		pConnect->pContext = pListener->pContext;
		size_t connect_index;

		if (m_ConnectorFrees.empty())
		{
			connect_index = m_Connectors.size();
			m_Connectors.push_back(pConnect);
		}
		else
		{
			connect_index = m_ConnectorFrees.back();
			m_ConnectorFrees.pop_back();
			m_Connectors[connect_index] = pConnect;
		}

		pConnect->nEvData = MAKE_EV_DATA(io_type, connect_index, new_sock);
		struct event ev;
		ev.data.u64 = pConnect->nEvData;
		ev.events = EPOLLIN | EPOLLRDHUP;

		if (epoll_ctl(m_Epoll, EPOLL_CTL_ADD, new_sock, &ev) != 0)
		{
			DeleteConnector((int)connect_index);
			return false;
		}

		pListener->AcceptCallback(pListener->pContext, (int)index, (int)connect_index, pConnect->strAddr, pConnect->nPort);
		return true;
		
	}

	if (io_type == TYPE_BROADCAST)
	{
		index &= 0x00FFFFFF;
		broadcast_t* pBroad = m_BroadCasts[index];
		if (NULL == pBroad) return false;
		if (pBroad->Socket != sock) return false;

		size_t read_size;
		char remote_addr[32];
		int remote_port;

		if (!Port_SocketReceiveFrom(sock, pBroad->pInBuf, pBroad->nInBufferLen, 
		remote_addr, sizeof(remote_addr), remote_port, &read_size))
		{
			char info[128];
			Port_SocketGetError(info, sizeof(info));
			// log
			return false;
		}

		pBroad->BroadcastCallback(pBroad->pContext, (int)index, remote_addr, 
			remote_port, pBroad->pInBuf, read_size);
		return true;
	}

	return false;
}

bool CReactor::ProcessError(size_t index, int sock, int events)
{
	int io_type = (index >> 24) & 0xFF;

	if (io_type == TYPE_CONNECTOR)
	{
		connector_t* pConnect = m_Connectors[index];
		if (NULL == pConnect) return false;

		if (pConnect->Socket != sock) return false;

		if (pConnect->ConnectCallback)
		{
			//connect fail
			pConnect->ConnectCallback(pConnect->pContext, (int)index, 0, pConnect->strad, pConnect->nPort);
			DeleteConnector((int)index);
		}
		else
		{
			if (events & EPOLLERR)
			{
				// log socket error
			}

			if (events & EPOLLHUP)
			{
				// log socket hangup
			}

			if (events & EPOLLRDHUP)
			{
				// log socket read hangup
			}

			CloseConnect(index);
		}

		return true;
	}

	if (io_type == TYPE_LISTENER)
	{
		// log listen error
		return true;
	}

	if (io_type == TYPE_BROADCAST)
	{
		// log broadcast error
		return true;
	}

	return false;
}

void CReactor::CheckClosing()
{
	size_t closing_size = m_Closings.size();
	if (closing_size > 0)
	{
		for (size_t k = 0; k < closing_size; ++k)
		{
			int closing_index = m_Closings[k];

			if (closing_index < 0) continue;

			connector_t* pConnect = m_Connectors[closing_index];

			if (NULL == pConnect)
			{
				m_Closings[k] = -1;
				m_ClosingFrees.push_back(k);
				continue;
			}

			if (pConnect->nState != IO_STATE_CLOSING)
			{
				// 状态已经切换，连接已经不存在
				m_Closings[k] = -1;
				m_ClosingFrees.push_back(k);
				continue;				
			}

			pConnect->fCounter += m_fCheckTime;

			if (pConnect->fCounter > pConnect->nTimeout)
			{
				// 关闭超时连接
				m_Closings[k] = -1;
				m_ClosingFrees.push_back(k);

				CloseConnect(closing_index);
			}
		}

		if (m_Closings.size() == m_ClosingFrees.size())
		{
			m_Closings.clear();
			m_ClosingFrees.clear();
		}
	}
}

bool CReactor::Dump(const char * file_name)
{
	FILE* fp =fopen(file_name, "wb");
	if (NULL == fp)
	{
		return false;
	}

	fprintf(fp, "events per loop is %d\r\n", m_nEventsPerLoop);
	fprintf(fp, "minimum timer is %f\r\n", (double)m_fMinTime);
	fprintf(fp, "\r\n");

	for (size_t k1 = 0; k1 < m_BroadCasts.size(); ++k1)
	{
		broadcast_t* p = m_BroadCasts[k1];
		if (NULL == p) continue;

		fprintf(fp, "broadcast local addr: %s, broad addr: %s, port: %d, "
		"in buffer len: %d\r\n", p->strLocalAddr, p->strBroadAddr, p->nPort, p->nInBufferLen);
	}

	fprintf(fp, "\r\n");

	for (size_t k2 = 0; k2 < m_Listeners.size(); ++k2)
	{
		listener_t* p = m_Listeners[k2];
		if (NULL == p) continue;

		fprintf(fp, "listener addr: %s, port: %d, inbuf_len: %d, "
		"out buffer len: %d, out buffer max: %d\r\n", p->strAddr, p->nPort,
		p->nInBufferLen, p->nOutBufferLen, p->nOutBufferMax);
	}

	fprintf(fp, "\r\n");

	for (size_t k3 = 0; k3 < m_Connectors.size(); ++k3)
	{
		connector_t* p = m_Connectors[k3];
		if (NULL == p) continue;

		fprintf(fp, "connect addr: %s, port: %d, in buffer len: %d, "
		"out buffer len: %d, out buffer max: %d, send empty: %d,"
		"send remain: %d, send offset: %d\r\n", p->strAddr, p->nPort,
		p->nInBufferLen, p->nOutBufferLen, p->nOutBufferMax,
		p->nSendEmpty, p->nSendRemain, p->nSendOffset);
	}
	fprintf(fp, "\r\n");

	for (size_t k4 = 0; k4 < m_Timers.size(); ++k4)
	{
		timer_t* p = m_Timers[k4];
		if (NULL == p) continue;

		fprintf(fp, "timer seconds: %f, counter: %f\r\n", (double)p->fSeconds, (double)p->fCounter);
	}
	fprintf("\r\n");

	fclose(fp);
	return true;
}
#endif