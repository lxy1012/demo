#ifndef __COMMON_THREAD_H__
#define __COMMON_THREAD_H__

#include "thread.h"
#include "msgqueue.h"
#include "MsgHandleBase.h"
#include <vector>

using namespace std;

class SystemConfigBaseInfo;
class CommonThread : public Thread
{
public:
	CommonThread(SystemConfigBaseInfo* conf,MsgHandleBase* pMsgHandler,char* szThreadName);
	void SetQueue(MsgQueue* msgQueue);
private:
	int Run();

	MsgQueue* m_pMsgQueue;
	SystemConfigBaseInfo* m_SystemConfigBaseInfoDef;
	int m_iTestTime;
	MsgHandleBase* m_pMsgHandler;
	int m_iDayLeftSecond;
	char m_szThreadName[32];
};
#endif
