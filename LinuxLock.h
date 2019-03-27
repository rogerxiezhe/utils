#ifndef _SYSTEM_LINUXLOCK_H
#define _SYSTEM_LINUXLOCK_H

#include <pthread.h>

class CLockUtil
{
public:
	CLockUtil() 
	{ 
		pthread_mutex_init(&m_mutex, NULL);
	}
	~CLockUtil() 
	{ 
		pthread_mutex_destroy(&m_mutex);
	}
	void Lock() 
	{ 
		pthread_mutex_lock(&m_mutex); 
	}
	
	void UnLock() 
	{
		pthread_mutex_unlock(&m_mutex);
	}
	
private:
	pthread_mutex_t m_mutex;
};

class CAutoLock
{
public:
	explicit CAutoLock(CLockUtil& lock) : m_lock(lock)
	{
		m_lock.Lock();	
	}

	~CAutoLock()
	{
		m_lock.UnLock();
	}

private:
	CAutoLock();
	CAutoLock(const CAutoLock&);
	CAutoLock& operator=(const CAutoLock&);
	
private:
	CLockUtil& m_lock;
};
#endif
