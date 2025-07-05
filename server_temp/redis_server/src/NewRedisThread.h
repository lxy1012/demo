#ifndef __NewRedisThread_H__
#define __NewRedisThread_H__

#include "thread.h"
#include "msgqueue.h"
#include "MsgHandleBase.h"
#include <vector>

using namespace std;

class SystemConfigBaseInfo;
class NewRedisThread : public Thread
{
public:
	NewRedisThread(int index,SystemConfigBaseInfo* conf,MsgHandleBase* pMsgHandler,char* szThreadName);
	void SetRedisQueue(MsgQueue* msgQueue);

	static redisContext* RedisLogin(int iCtxID, char* szHost, int iPort, char* szPwd,int iDBIndex);

	void SetRecordMsgQueue(bool bRecordQueue);
private:
	int 				Run();
	MsgQueue*  m_pRedisMsgQueue;
	int m_index;
	SystemConfigBaseInfo* m_SystemConfigBaseInfoDef;
	int  m_iTestTime;
	MsgHandleBase* m_pMsgHandler;

	int m_iDayLeftSecond;

	char m_szThreadName[32];
	
	bool m_bRecordQueueCount;	//是否统计队列处理信息
	unsigned long long m_llSecDeQueueCount;	//每秒成功出队列次数
	unsigned long long m_llDayDeQueueCount;	//每日成功出队列次数
};

#endif // !__NewRedisThread_H__