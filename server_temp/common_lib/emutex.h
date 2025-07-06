#ifndef _EMUTEX_H_
#define _EMUTEX_H_

#include <pthread.h>

class EMutex
{
public:
	EMutex();
	~EMutex();
	void lock();
	void unlock();
	pthread_mutex_t* getId();
private:
	mutable pthread_mutex_t myId;
};

#endif

