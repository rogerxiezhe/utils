#ifndef THREAD_TASK_H
#define THREAD_TASK_H

namespace
{
	enum TASK_PRIORITY
	{
		TASK_PRIORITY_MAX = 1,
		TASK_PRIORITY_NORMAL = 16,
		TASK_PRIORITY_MIN = 32,
	};
}

class CTask
{
public:
	CTask()
	{
		m_nPriority = TASK_PRIORITY_MIN;
	}

	CTask(const int nPriority) : m_nPriority(nPriority) {}

	virtual ~CTask() {}

	void SetPriority(const int nValue)
	{
		if (nValue < TASK_PRIORITY_MAX)
		{
			m_nPriority = TASK_PRIORITY_MAX;
		}
		else if (nValue > TASK_PRIORITY_MIN)
		{
			m_nPriority = TASK_PRIORITY_MIN;
		}
		else
		{
			m_nPriority = nValue;
		}
	}

	int GetPriority() const
	{
		return m_nPriority;
	}

	virtual void Run() = 0;

protected:
	int m_nPriority;
};

#endif // !THREAD_TASK_H

