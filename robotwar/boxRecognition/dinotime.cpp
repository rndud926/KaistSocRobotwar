#include "dinotime.h"


DWatch::DWatch()
{
	timer = time(NULL);

#ifdef _WIN32
	localtime_s(&t, &timer);
#else
	t = localtime(&timer);
#endif

#ifdef _WIN32
	sprintf_s(m_TextTime, "%04d%02d%02d%02d%02d%02d", t.tm_year+1900, t.tm_mon+1, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec);
#else
	sprintf(m_TextTime, "%04d%02d%02d%02d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec);
#endif

#ifdef _WIN32
	m_swFreq.LowPart = m_swFreq.HighPart = 0;
	m_swStart = m_swFreq;
	m_swEnd = m_swFreq;
	m_fTimeforDuration = 0;
	QueryPerformanceFrequency(&m_swFreq);
#endif

}

DWatch::~DWatch()
{
}

void DWatch::Start()
{
#ifdef _WIN32 // Windows
	QueryPerformanceCounter(&m_swStart);
#else // Linux
	gettimeofday(&start_point, NULL);
#endif
}

void DWatch::End()
{
#ifdef _WIN32
	QueryPerformanceCounter(&m_swEnd);
	m_fTimeforDuration = (m_swEnd.QuadPart - m_swStart.QuadPart)/(float)m_swFreq.QuadPart;
#else
	gettimeofday(&end_point, NULL); 
	operating_time = (double)(end_point.tv_sec)+(double)(end_point.tv_usec)/1000000.0-(double)(start_point.tv_sec)-(double)(start_point.tv_usec)/1000000.0;
#endif
}
