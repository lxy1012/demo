#ifndef _THREAD_H_
#define _THREAD_H_

#ifndef _REENTRANT
#define  _REENTRANT
#endif

#include <pthread.h>
#include <sched.h>

typedef enum				/*Detach state*/
{
	JOINABLE,				
	DETACHED				
}DetachStateDef;

typedef enum
{
	NORMAL,
	HIGH
}PriorityDef;

class Thread
{
private:
	pthread_t			thread_id;
	virtual int Run()=0;			/*Execbute function of the thread*/	
	static void ThreadMain(void* arg);	
public:	
	pthread_attr_t		thread_attr;/*The attribution of the thread*/
	Thread();						/* Default construction,DETACHED */
	Thread(PriorityDef priority);	
	Thread(DetachStateDef detach_state);
	Thread(DetachStateDef detach_state,PriorityDef priority);
	virtual ~Thread();	
	void Start();					/*start the thread with Run()*/
	void Stop();					/*Force to terminate the thread*/
	void ReStart();
	pthread_t GetThreadId();	
};
#endif
