// StopWatch.h: declares the interface for the "StopWatch" class
//

#if !defined (__VK_MERKLE_ROOTS_STOP_WATCH__)
#define __VK_MERKLE_ROOTS_STOP_WATCH__

#if !defined (_WIN32)
// Includes
//

// POSIX Headers
#include <time.h>
#endif

// Class(es)
//

class StopWatch {
public:
	// Constructor/destructor
	StopWatch();
	virtual ~StopWatch();

	bool Start(void);

	double Elapsed(void) const;

private:
#if defined (_WIN32)
	double m_frequency;
	__int64 m_started;
#else
    struct timespec m_started;
#endif
};

#endif // __VK_MERKLE_ROOTS_STOP_WATCH__
