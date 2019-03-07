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
#endif
