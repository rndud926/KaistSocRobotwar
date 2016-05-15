/*
 * Written by Park, ByeongJu
 * Date : 2013.11.05
 * Support OS : Linux, Windows
 * Function :
 *		- We can get text of current time.
 *		- We can get processing time in millisecond.
 * Usage :
 *
 *		// get text of current date & time
 *		DWatch watch;
 *		printf("%s\n", watch.GetTextTime());
 *
 *		// processing time check
 *		DWatch watch;
 *		watch.Start();
 *		(processing...)
 *		watch.End();
 *		watch.GetDurationSecond(); or watch.GetDurationMilliSecond();
 *		
 *
 */

#ifndef __DINOTIME_H__
#define __DINOTIME_H__

#ifdef _WIN32 // Windows
	#include <stdio.h>
	#include <time.h>
	#include <stdlib.h>
	#include <Windows.h>
	#include <Winnt.h>
#else // Linux
	#include <sys/time.h>
	#include <stdio.h>
	#include <time.h>
#endif

class DWatch
{
public:
	DWatch();
	virtual ~DWatch();

public:
	void Start();
	void End();
	const float GetDurationSecond() const
	{
#ifdef _WIN32
		return m_fTimeforDuration;
#else
		return operating_time;
#endif

	}
	const float GetDurationMilliSecond() const
	{
#ifdef _WIN32
		return m_fTimeforDuration*1000.f;
#else
		return operating_time*1000.f;
#endif
	}
	const char *GetTextTime() const { return m_TextTime; }

protected:
	// related string of current time
	char m_TextTime[15];

	// related current time
	time_t			timer;

#ifdef _WIN32 // Windows
	// related stop-watch
	LARGE_INTEGER	m_swFreq, m_swStart, m_swEnd;
	float			m_fTimeforDuration;

	// related current time
	struct tm t;

#else // Linux
	// related stop-watch
	struct timeval	start_point, end_point;
	float			operating_time;

	// related current time
	struct tm		*t;

#endif
};

#endif // __DINOTIME_H__
