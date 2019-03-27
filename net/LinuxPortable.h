#ifndef _SYTEM_LINUXPORTABLE_H
#define _SYTEM_LINUXPORTABLE_H

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <sched.h>
#include <pthread.h>

#define MAX_PAHT 260

struct port_date_time_t
{
	int nYear;
	int nMonth;
	int nDay;
	int nHour;
	int nMinute;
	int nSecond;
	int nMilliseconds;
	int nDayOfWeek;
};

struct port_uuid_t
{
	unsigned long Data1;
	unsigned long Data2;
	unsigned long Data3;
	unsigned char Data4[8];
};

inline bool Port_GetLocalTime(port_date_time_t* dt)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	tm* pt = localtime(&tv.tv_sec);

	dt->nYear = pt->tm_year + 1900;
	dt->nMonth = pt->tm_mon + 1;
	dt->nDay = pt->tm_mday;
	dt->nHour = pt->tm_hour;
	dt->nMinute = pt->tm_min;
	dt->nSecond = pt->tm_sec;
	dt->nMilliseconds = tv.tv_usec / 1000;
	dt->nDayOfWeek = pt->tm_wday;

	return true;
}

inline unsigned int Port_GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

inline void Port_Sleep(unsigned int ms)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = ms * 1000 * 1000;

	nanosleep(&ts, NULL);
}

inline const char* Port_CreateGuid(char* buffer, size_t size)
{
	port_uuid_t id;
	uuid_generate((unsigned char*)&id);

	snprintf(buffer, size -1, 
	"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
	id.Data1, id.Data2, id.Data3,
	id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
	id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);

	buffer[size - 1] = 0;

	return buffer;
}

inline void CopyString(char* pDest, size_t byte_size, const char* pSrc)
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

inline bool Port_InitDaemon()
{
	int pid = fork();
	if (pid > 0)
	{
		// main process, finish
		exit(0);
	}
	else if (pid < 0)
	{
		// fork fail, return
		exit(1);
	}

	// first sonprocess tobe session leader
	setsid();
	
	pid = fork();
	if (pid > 0)
	{
		// main process, finish
		exit(0);
	}
	else if (pid < 0)
	{
		// fork fail, return
		exit(1);
	}

	for (int i = 0; i < NOFILE; ++i)
	{
		close(i);
	}

	umask(0);

	return true;
	
}


#endif
