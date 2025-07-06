#ifndef TIMER_H
#define TIMER_H

#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include "thread.h"

typedef long int TIME;

typedef struct timeout_t 
{
	struct timeout_t *next;
	struct timeout_t *prev;
	TIME when;
	void (*func) (void *);
	void *what;
	int continuous;
	int interval;
	int is_used;
}timeout;

typedef enum
{
	TIMER_SINGLE,
	TIMER_REPEAT
}TimerMethodDef;

class TimerTask : public Thread
{
public:/*private:*/
//	timeout 		*timeouts;
//	timeout 		*free_timeouts;
	int Run();
	void Dispatch();
public:
	TimerTask();
	~TimerTask();
	timeout* AddTimeout(TIME when,void (*where)(void *),void *what,int cont, int interval);
	timeout* AddTimeoutNoLock(TIME when,void (*where)(void *),void *what,int cont, int interval);
	void CancelTimeout(timeout *q);
	void CancelTimeoutNoLock(timeout *q);
};

class EST_Timer
{
private:
	int timer_interval;
	void *notifyee;
	void (*func)(void *);
	TimerTask	*timer_task;
	int is_active;

public:
	EST_Timer(TimerTask *timer_task);
	~EST_Timer();
	void SetInterval(int interval);
	void SetNotify(void (*foo)(void *p), void *master);
	void Start(TimerMethodDef method);
	void Start();
	void Stop();
	int IsActive();
	timeout *my_t;
	
};
#endif
