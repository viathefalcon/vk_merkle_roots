// StopWatch.cpp: defines the implementation of the "StopWatch" class
//

// Includes
//

#if defined (_WIN32)
// Windows Headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Declarations
#include "StopWatch.h"

// Class(es)
//

#if defined (_WIN32)
StopWatch::StopWatch(): m_started( 0 ) { }
#else
StopWatch::StopWatch() { }
#endif

StopWatch::~StopWatch() { }

bool StopWatch::Start() {

#if defined (_WIN32)
	LARGE_INTEGER li;
	if (::QueryPerformanceFrequency( &li )){
		m_frequency = static_cast<double>( li.QuadPart ) / 1000.0;

		::QueryPerformanceCounter( &li );
		m_started = li.QuadPart;
		return true;
	}
	return false;
#else
    const int r = ::clock_gettime( CLOCK_MONOTONIC, &m_started );
    return (r == 0);
#endif
}

double StopWatch::Elapsed(void) const {

#if defined (_WIN32)
	LARGE_INTEGER li;
	::QueryPerformanceCounter( &li );
	return static_cast<double>( li.QuadPart - m_started ) / m_frequency;
#else
    struct timespec now;
    ::clock_gettime( CLOCK_MONOTONIC, &now );
    auto nanos = (now.tv_sec - m_started.tv_sec) * (long)1e9 + (now.tv_nsec - m_started.tv_nsec);
    return static_cast<double>( nanos ) / 1000000.0;
#endif
}
