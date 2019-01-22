#ifndef UTILS_LINUXTHREAD_H
#define UTILS_LINUXTHREAD_H

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

class CThread
{
private:
	typedef void(__cdecl* LOOP_FUNC)(void*);
	typedef bool(__cdecl* INIT_FUNC)(void*);
	typedef bool(__cdecl* SHUT_FUNC)(void*);

	static void* WorkerProc(void* lpParameter)
	{
		CThread* pthis = (CThread*)lpParameter;
		LOOP_FUNC loop_func = pthis->m_LoopFunc;
		void* pContext = pthis->m_pContext;
		int sleep_ms = pthis->m_nSleep;

		if (pthis->m_InitFunc)
		{
			pthis->m_InitFunc(pContext);
		}

		while (!pthis->m_bQuit)
		{
			loop_func(pContext);
			if (sleep_ms > 0)
			{
				struct timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = sleep_ms * 1000000;
				nanosleep(&ts, NULL);
			}
			else
			{
				sched_yield();
			}
		}

		if (pthis->m_ShutFunc)
		{
			pthis->m_ShutFunc(pContext);
		}

		return (void*)0;
	}

public:
	CThread(LOOP_FUNC loop_func, INIT_FUNC init_func, SHUT_FUNC shut_func, void* context, int sleep_ms, int stack_size)
	{
		m_LoopFunc = loop_func;
		m_InitFunc = init_func;
		m_ShutFunc = shut_func;
		m_pContext = context;
		m_nSleep = sleep_ms;
		m_nStackSize = stack_size;
		m_bQuit = false;
		m_hThread = -1;
	}

	~CThread() {}

	void SetQuit(bool value) { m_bQuit = value; }
	bool GetQuit() const { return m_bQuit; }
	
	bool Start()
	{
		m_bQuit = false;
		int res = pthread_create(&m_hThread, NULL, WorkerProc, this);
		return (0 == res);
	}

	bool Stop()
	{
		m_bQuit = true;
		if (m_hThread != -1)
		{
			pthread_join(m_hThread, NULL);
			m_hThread = -1;
		}

		return true;
	}

	// 暂停线程
	bool Suspend()
	{
		return false;
	}

	// 继续线程
	bool Resume()
	{
		return false;
	}



private:
	CThread();
	CThread(const CThread&);
	CThread& operator=(const CThread&);

private:
	LOOP_FUNC m_LoopFunc;
	INIT_FUNC m_InitFunc;
	SHUT_FUNC m_ShutFunc;
	void* m_pContext;
	int m_nSleep;
	int m_nStackSize;
	bool m_bQuit;
	pthread_t m_hThread;
};

// 线程等待
class CThreadWaiter
{
public:
	CThreadWaiter()
	{
		pthread_cond_init(&m_cond, NULL);
		pthread_mutex_init(&m_mutex, NULL);
	}

	~CThreadWaiter()
	{
		pthread_cond_destroy(&m_cond);
		pthread_mutex_destroy(&m_mutex);
	}

	// 线程等待
	bool Wait(int ms)
	{
		bool result = false;

		if (ms < 0)
		{
			// 无限时间等待
			pthread_mutex_lock(&m_mutex);
			int res = pthread_cond_wait(&m_cond, &m_mutex);
			if (0 == res)
			{
				result = true;
			}

			pthread_mutex_unlock(&m_mutex);
		}
		else
		{
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_nsec += ms * 1000000;
			if (ts.tv_nsec > 999999999)
			{
				ts.tv_sec += 1;
				ts.tv_nsec -= 1000000000;
			}

			pthread_mutex_lock(&m_mutex);
			int res = pthread_cond_timewait(&m_cond, &m_mutex, &ts);

			if (0 == res)
			{
				result = true;
			}

			pthread_mutex_unlock(&mutex);
		}

		return result;
	}

	// 唤醒
	bool Signal()
	{
		pthread_cond_signal(&m_cond);
	}

	// 暂停线程
	bool Suspend()
	{
		return false;
	}

	// 继续线程
	bool Resume()
	{
		return false;
	}

private:
	pthread_cond_t m_cond;
	pthread_mutex_t m_mutex;
};
#endif // !UTILS_LINUXTHREAD_H


