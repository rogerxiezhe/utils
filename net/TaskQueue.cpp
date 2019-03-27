#include "TaskQueue.h"
#include <string.h>

struct CTaskQueue::msg_vector_t
{
private:
	struct header_t
	{
		size_t len;
		int nSrc;
		int nType;
	};

public:
	msg_vector_t()
	{
		m_pBuffer = NULL;
		m_nBuffSize = 0;
		m_nBuffUsed = 0;
	}

	~msg_vector_t()
	{
		if (m_pBuffer)
		{
			delete[] m_pBuffer;
		}
	}

	size_t GetBuffSize() const { return m_nBuffSize; }
	size_t GetBuffUsed() const { return m_nBuffUsed; }

	void Swap(msg_vector_t& src)
	{
		char* temp_buffer = src.m_pBuffer;
		size_t temp_buffsize = src.m_nBuffSize;
		size_t temp_buffused = src.m_nBuffUsed;

		src.m_pBuffer = m_pBuffer;
		src.m_nBuffSize = m_nBuffSize;
		src.m_nBuffUsed = m_nBuffUsed;

		m_pBuffer = temp_buffer;
		m_nBuffSize = temp_buffsize;
		m_nBuffUsed = temp_buffused;
	}

	void clear()
	{
		m_nBuffUsed = 0;
	}

	void ReleaseAll()
	{
		if (m_pBuffer) delete[] m_pBuffer;

		m_pBuffer = NULL;
		m_nBuffSize = 0;
		m_nBuffUsed = 0;
	}

	void Shrink(size_t size)
	{
		if (m_nBuffUsed > 0) return;

		if (m_nBuffSize > size)
		{
			ReleaseAll();
		}
	}

	bool Put(int src, int type, const void* pdata, size_t len)
	{
		if (NULL == pdata) return false;
		
		char* p = PutHeader(len, src, type);

		if (len > 0)
		{
			memcpy(p, pdata, len);
		}

		m_nBuffUsed += len + sizeof(header_t);
		return true;
	}

	bool Put2(int src, int type, const void* pdata1, size_t len1, 
		const void* pdata2, size_t len2)
	{
		if (NULL == pdata1 || NULL == pdata2) return false;
		
		char* p = PutHeader(len1 + len2, src, type);

		if (len1 > 0)
		{
			memcpy(p, pdata1, len1);
		}

		if (len2 > 0)
		{
			memcpy(p + len1, pdata2, len2);
		}

		m_nBuffUsed += len1 + len2 + sizeof(header_t);

		retrun true;
	}

	bool Put3(int src, int type, const void* pdata1, size_t len1,
		const void* pdata2, size_t len2, const void* pdata3, size_t len3)
	{
		if (NULL == pdata1 || NULL == pdata2 || NULL == pdata3) return false;
		
		char* p = PutHeader(len1 + len2 + len3, src, type);

		if (len1 > 0)
		{
			memcpy(p, pdata1, len1);
		}

		if (len2 > 0)
		{
			memcpy(p + len1, pdata2, len2);
		}

		if (len3 > 0)
		{
			memcpy(p + len1 + len2, pdata3, len3);
		}

		m_nBuffUsed += len1 + len2 + len3 + sizeof(header_t);

		return true;
	}

	bool Peek(size_t& it, int& src, int& type, const void*& pdata, size_t& len) const 
	{
		size_t pos = it + sizeof(header_t);
		if (pos > m_nBuffUsed) return false;

		header_t* p = (header_t*)(m_pBuffer + it);
		if ((pos + p->len) > m_nBuffUsed) return false;

		it = pos + p->len;
		src = p->nSrc;
		type = p->nType;
		pdata = m_pBuffer + pos;
		len = p->len;

		return true;
	}
	
private:
	msg_vector_t(const msg_vector_t&);
	msg_vector_t& operator=(const msg_vector_t&);

	char* PutHeader(size_t len, int src, int type)
	{
		const size_t total_len = len + sizeof(header_t);
		size_t new_used = m_nBuffUsed + total_len;

		if (new_used > m_nBuffSize)
		{
			size_t newSize = m_nBuffSize << 1;
			if (newSize < new_used)
			{
				newSize = new_used << 1;
			}

			char* new_buffer = new char[newSize];
			memcpy(new_buffer, m_pBuffer, m_nBuffUsed);
			if (m_pBuffer) delete[] m_pBuffer;

			m_pBuffer = new_buffer;
			m_nBuffSize = newSize;
		}

		header_t* p = (header_t*)(m_pBuffer + m_nBuffUsed);
		p->len = len;
		p->nSrc = src;
		p->nType = type;

		return (char*)p + sizeof(header_t);
	}

	
private:
	char* m_pBuffer;
	size_t m_nBuffSize;
	size_t m_nBuffUsed;
};

CTaskQueue::CTaskQueue()
{
	m_nMsgCount = 0;
	m_nBufferLimit = 32 * 1024 * 1024;
	m_nShrinkSize = 0;
	m_pForwVector = new msg_vector_t;
	m_pBackVector = new msg_vector_t;
}

CTaskQueue::~CTaskQueue()
{
	delete m_pForwVector;
	delete m_pBackVector;
}

void CTaskQueue::ClearForwVector()
{
	CAutoLock autolock(m_Lock);

	m_pForwVector->clear();
	m_nMsgCount = 0;
}

void CTaskQueue::ReleaseForwVector()
{
	CAutoLock autolock(m_Lock);

	m_pForwVector.ReleaseAll();
	m_nMsgCount = 0;
}

void CTaskQueue::SwapMsgVector()
{
	CAutoLock autolock(m_Lock);

	m_pForwVector->Swap(*m_pBackVector);
	m_pForwVector->clear();
	m_nMsgCount = 0;


	if (m_nShrinkSize > 0)
	{
		m_pForwVector->Shrink(m_nShrinkSize);
	}
}

bool CTaskQueue::CheckOverflow()
{
	if (m_nBufferLimit > 0 && m_pForwVector->GetBuffUsed() >= m_nBufferLimit) return true;
	return false;
}

bool CTaskQueue::Put(int src, int type, const void * pdata, size_t len)
{
	CAutoLock autolock(m_Lock);

	if (CheckOverflow()) return false;

	if (!m_pForwVector->Put(src, type, pdata, len)) return false;

	m_nMsgCount++;
	return true;
}

bool CTaskQueue::Put2(int src, int type, const void * pdata1, size_t len1, const void * pdata2, size_t len2)
{
	CAutoLock autolock(m_Lock);

	if (CheckOverflow()) return false;

	if (!m_pForwVector->Put2(src, type, pdata1, len1, pdata2, len2)) return false;

	m_nMsgCount++;
	return true;
}

bool CTaskQueue::Put3(int src, int type, const void * pdata1, size_t len1, const void * pdata2,
	size_t len2, const void * pdata3, size_t len3)
{
	CAutoLock autolock(m_Lock);

	if (CheckOverflow()) return false;

	if (!m_pForwVector->Put3(src, type, pdata1, len1, pdata2, len2, pdata3, len3)) return false;

	m_nMsgCount++;
	return true;
}

bool CTaskQueue::Peek(size_t& it, int& src, int& type, const void*& pdata, size_t& len)
{
	return m_pBackVector->Peek(it, src, type, pdata, len);
}

void CTaskQueue::Change(CHANGE_FUNC func, void * context)
{
	if (NULL == func) return;

	if (0 == m_nMsgCount) return;

	CAutoLock autolock(m_Lock);

	size_t it = 0;
	int src, type;
	const void* pdata;
	size_t len;

	while (m_pForwVector->Peek(it, src, type, pdata, len))
	{
		func(context, src, type, (void*)pdata, len);
	}
}