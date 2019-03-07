#ifndef _UTILS_TICKTIMER_H
#define _UTILS_TICKTIMER_H
#include "LinuxPortable.h"

// 低精度定时器
class CTickTimer
{
public:
	CTickTimer()
	{
		m_nLastMilliseconds = 0;
	}

	void Initialize()
	{
		m_nLastMilliseconds = Port_GetTickCount();
	}

	// 返回逝去的毫秒数
	int GetElapseMillisec(int except = 0)
	{
		unsigned int cur = Port_GetTickCount();
		unsigned int dw = cur - m_nLastMilliseconds;

		if (dw >= (unsigned int)except)
		{
			m_nLastMilliseconds = cur;
		}

		return dw;
	}
private:
	unsigned int m_nLastMilliseconds;
};
#endif
