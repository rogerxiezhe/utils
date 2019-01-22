#ifndef UTILS_WINTHREAD_H
#define UTILS_WINTHREAD_H

#include <windows.h>
#include <process.h>

class CThread
{
private:
	typedef void(__cdecl* LOOP_FUNC)(void*);
	typedef bool(__cdecl* INIT_FUNC)(void*);
	typedef bool(__cdecl* SHUT_FUNC)(void*);

public:
	static void __cdecl WorkerProc(void* lpParameter)
	{
		CThread* pthis = (CThread*)lpParameter;
		LOOP_FUNC loop_func = pthis->m_LoopFunc;
		void* context = pthis->m_pContext;
		int sleep_ms = pthis->m_nSleep;

		if (pthis->m_InitFunc)
		{
			pthis->m_InitFunc(context);
		}

		while (!pthis->m_bQuit)
		{
			loop_func(context);

			if (sleep_ms >= 0)
			{
				Sleep(sleep_ms);
			}
		}

		if (pthis->m_ShutFunc)
		{
			pthis->m_ShutFunc(context);
		}
	}

	CThread(LOOP_FUNC loop_func, INIT_FUNC init_func, SHUT_FUNC shut_func,
		void* context, int sleep_ms, int stack_size)
	{
		m_LoopFunc = loop_func;
		m_InitFunc = init_func;
		m_ShutFunc = shut_func;
		m_pContext = context;
		m_nSleep = sleep_ms;
		m_nStackSize = stack_size;
		m_bQuit = false;
		m_hThread = NULL;
	}

	~CThread() {}

	void SetQuit(bool value) { m_bQuit = value; }
	bool GetQuit() const { return m_bQuit; }

	// 启动线程
	bool Start()
	{
		m_bQuit = false;
		m_hThread = (HANDLE)_beginthread(WorkerProc, m_nStackSize, this);
		return (m_hThread != NULL);
	}

	// 停止线程
	bool Stop()
	{
		m_bQuit = true;
		if (m_hThread != NULL)
		{
			WaitThreadExit(m_hThread);
			m_hThread = NULL;
		}
	}

	// 暂停线程
	bool Suspend()
	{
		if (m_hThread != NULL)
		{
			SuspendThread(m_hThread);
			return true;
		}

		return false;
	}

	// 继续线程
	bool Resume()
	{
		if (m_hThread != NULL)
		{
			ResumeThread(m_hThread);
			return true;
		}

		return false;
	}

private:
	CThread();
	CThread(const CThread&);
	CThread& operator=(const CThread&);
	
	bool WaitThreadExit(HANDLE handle)
	{
		while (true)
		{
			DWORD res = WaitForSingleObject(handle, 1000);

			if (res == WAIT_OBJECT_0)
			{
				return true;
			}
			else if (res == WAIT_TIMEOUT)
			{
				DWORD exit_code;

				if (GetExitCodeThread(handle, &exit_code) == false)
				{
					return false;

				}

				if (exit_code != STILL_ACTIVE)
				{
					break;
				}
			}
			else
			{
				return false;
			}
		}

		return true;
	}

private:
	LOOP_FUNC m_LoopFunc;
	INIT_FUNC m_InitFunc;
	SHUT_FUNC m_ShutFunc;
	void* m_pContext;
	int m_nSleep;
	int m_nStackSize;
	bool m_bQuit;
	HANDLE m_hThread;
};

class CThreadWaiter
{
public:
	CThreadWaiter()
	{
		m_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	}

	~CThreadWaiter()
	{
		CloseHandle(m_hEvent);
	}

	bool Wait(int ms)
	{
		if (ms < 0)
		{
			WaitForSingleObject(m_hEvent, INFINITE);
		}
		else
		{
			DWORD res = WaitForSingleObject(m_hEvent, ms);
			if (WAIT_TIMEOUT == res)
			{
				return false;
			}
		}

		return true;
	}

	// 唤醒
	bool Signal()
	{
		return SetEvent(m_hEvent) == true;
	}

private:
	HANDLE m_hEvent;
};

#endif // !UTILS_WINTHREAD_H
