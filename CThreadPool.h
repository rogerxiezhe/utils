#ifndef UTILS_THREADPOOL_H
#define UTILS_THREADPOOL_H

#include <thread>
#include "Task.h"
using namespace std;

class CMyThread
{
public:
	friend bool operator==(const CMyThread& l_thread, const CMyThread& r_thread);
	friend bool operator!=(const CMyThread& l_thread, const CMyThread& r_thread);
	

};

class CThreadPool
{
	
};

#endif // !UTILS_THREADPOOL_H

