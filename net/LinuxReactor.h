#ifndef _NET_LINUXREACTOR_H
#define _NET_LINUXREACTOR_H
#ifdef __linux__
#include "ArrayPod.h"

typedef void (*broadcast_callback)(void* context, int broadcast_id, const char* addr, int port, const void* pdata, size_t len);
typedef void (*accept_callback)(void* context, int listener_id, int connector_id, const char* pdata, int port);
typedef void (*connect_callback)(void* context, int connector_id, int succeed, const char* addr, int port);
typedef void (*close_callback)(void* context, int connector_id, const char* addr, int port);
typedef void (*receive_callback)(void* context, int connector_id, const void* pdata, size_t len);
typedef bool (*compress_callback)(void* context, const void* pdata, size_t& len);
typedef void (*timer_callback)(void* context, int timer_id, float seconds);

class CTickTimer;
struct epoll_event;

class CReactor
{
public:
	struct broadcast_t
	{
		port_socket_t Socket;
		char strLocalAddr[32];
		char strBroadAddr[32];
		int nPort;
		size_t nInBufferLen;
		char* pInBuf;
		broadcast_callback BroadcastCallback;
		void* pContext;
	};

	struct listener_t
	{
		port_socket_t Socket;
		char strAddr[32];
		int nPort;
		size_t nInBufferLen;
		size_t nOutBufferLen;
		size_t nOutBufferMax;
		accept_callback AcceptCallback;
		close_callback CloseCallback;
		receive_callback ReceiveCallback;
		void* pContext;
	};

	struct connector_t
	{
		port_socket_t Socket;
		uint64_t nEvData;
		char strAddr[32];
		int nPort;
		size_t nInBufferLen;
		size_t nOutBufferLen;
		size_t nOutBufferMax;
		connect_callback ConnectCallback;
		close_callback CloseCallback;
		receive_callback ReceiveCallback;
		void* pContext;
		int nTimeout;
		struct timeval tvTimeout;
		float fCounter;
		int nState;
		int nSendEmpty;
		char* pOutBuf;
		char* pSendBegin;
		size_t nSendRemain;
		size_t nSendOffset;
		char* pInBuf;
	};

struct timer_t
{
	float fSeconds;
	float fCounter;
	timer_callback TimerCallBack;
	void* pContext;
};

public:
	CReactor();
	~CReactor();

	bool Start();
	bool Stop();

	int CreateBroadCast(const char* local_addr, const char* broadcast_addr,
		int port, size_t in_buf_len, broadcast_callback cb, void* context);

	int DeleteBroadCast(int broadcast_id);

	bool SendBroadCast(int broadcast_id, const void* pdata, size_t len);

	int CreateListener(const char* addr, int port, int backlog,
		size_t in_buf_len, size_t out_buf_len, size_t out_buf_max,
		accept_callback accept_cb, close_callback close_cb,
		receive_callback receive_cb, void* context, size_t accept_num,
		int use_buffer_list, int wsabufs = 8, compress_callback compress_cb = NULL,
		bool keep_alive = true);

	bool DeleteListener(int listener_id);

	int GetListenerSock(int listener_id);

	int GetListenerPort(int listener_id);

	int GetConnectorSock(int connector_id);

	int CreateConnector(const char* addr, int port, size_t in_buf_len,
		size_t out_buf_len, size_t out_buf_max, int timeout,
		connect_callback conn_cb, close_callback close_cb,
		receive_callback recv_cb, void* context, int use_buffer_list, int bufs = 0, compress_callback compress_cb = NULL);

	bool DeleteConnector(int connector_id);
	bool ShutDownConnector(int connector_id, int wait_seconds = 0);
	bool GetConnected(int connector_id);
	bool Send(int connector_id, const void* pdata, size_t len, bool force);
	bool Send2(int connector_id, const void* pdata1, size_t len1, 
		const void* pdata2, size_t len2, bool force);

	int CreateTimer(float seconds, timer_callback cb, void* context);
	bool DeleteTimer(int timer_id);

	void EventLoop();
	bool Dump(const char* file_name);

private:
	void UpdateMinTime();
	bool CloseConnect(size_t index);
	bool ShutDown(size_t index, float timeout);
	bool ForceSend(connector_t* pConnect, size_t need_size);

	bool ProcessRead(size_t index, int sock);
	bool ProcessWrite(size_t index, int sock);
	bool ProcessError(size_t index, int sock, int events);
	void CheckClosing();

private:
	size_t m_nEventsPerLoop;
	struct epoll_event* m_pEvents;
	int m_Epoll;
	float m_fMinTime;
	float m_fCheckTime;
	CTickTimer* m_pTimer;
	TArrayPod<broadcast_t*, 1> m_BroadCasts;
	TArrayPod<size_t, 1> m_BroadCastFrees;
	TArrayPod<listener_t*, 1> m_Listeners;
	TArrayPod<size_t, 1> m_ListenerFrees;
	TArrayPod<connector_t*, 1> m_Connectors;
	TArrayPod<size_t, 1> m_ConnectorFrees;
	TArrayPod<timer_t*, 1> m_Timers;
	TArrayPod<size_t, 1> m_TimerFrees;
	TArrayPod<int, 1> m_Closings;
	TArrayPod<size_t, 1> m_ClosingFrees;
};
#endif
#endif