#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include "LinuxLock.h"

class CTaskQueue
{
private:
	typedef void (*CHANGE_FUNC)(void* context, int src, int type, void* pdata, size_t len);
	struct msg_vector_t;

public:
	CTaskQueue();
	~CTaskQueue();

	bool IsEmpty() const { return m_nMsgCount == 0; }
	bool IsFull() const { return CheckOverflow(); }

	size_t GetMessageCount() const { return m_nMsgCount; }
	size_t GetBufferLimit() const { return m_nBufferLimit; }
	void SetBufferLimit(size_t value) { m_nBufferLimit = value; }

	size_t GetShrinkSize() const { return m_nShrinkSize; }
	void SetShrinkSize(size_t value) { m_nShrinkSize = value; }

	// clear forward msg vector
	void ClearForwVector();
	// release forward msg vector
	void ReleaseForwVector();
	void SwapMsgVector();

	bool Put(int src, int type, const void* pdata, size_t len);
	bool Put2(int src, int type, const void* pdata1, size_t len1,
		const void* pdata2, size_t len2);
	bool Put3(int src, int type, const void* pdata1, size_t len1,
		const void* pdata2, size_t len2, const void* pdata3, size_t len3);

	// get msg from backward msgvector
	bool Peek(size_t& it, int& src, int& type, const void*& pdata, size_t& len) const;

	void Change(CHANGE_FUNC func, void* context);

private:
	CTaskQueue(const CTaskQueue&);
	CTaskQueue& operator=(const CTaskQueue&);

	// check buffer overflow
	bool CheckOverflow() const;
	
private:
	CLockUtil m_Lock;
	size_t m_nMsgCount;
	size_t m_nBufferLimit;
	size_t m_nShrinkSize;
	msg_vector_t* m_pForwVector;
	msg_vector_t* m_pBackVector;
};

#endif