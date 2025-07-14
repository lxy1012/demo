#ifndef __Common_Handler_H__
#define __Common_Handler_H__

#include "MsgHandleBase.h"
#include "msg.h"
#include "msgqueue.h"	//消息队列
#include <string>
#include <vector>
#include <map>
using namespace  std;


class CommonHandler :public MsgHandleBase
{
public:
	CommonHandler(MsgQueue *pSendQueue);

	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen,int iSocketIndex);
	virtual vector<int> GetHandleMsgTypes(){};

	virtual void CallBackOnTimer(int iTimeNow);

	

protected:

	MsgQueue *m_pSendQueue;

	//调用这几个接口时  请求方可以自定义 key value 值 服务器只负责存储和获取数据
	void HandleCommonSetByKey(char* szMsg, int iLen, int iSocketIndex);
	void HandleCommonSetByHash(char* szMsg, int iLen, int iSocketIndex);
	void HandleCommonGetByKey(char* szMsg, int iLen, int iSocketIndex);
	void HandleCommonGetByHash(char* szMsg, int iLen, int iSocketIndex);
	void HandleCommonSetByHashMap(char* szMsg, int iLen, int iSocketIndex);
	void HandleCommonGetByHashMap(char* szMsg, int iLen, int iSocketIndex);
	void HandleCommonHGetAll(char* szMsg, int iLen, int iSocketIndex);
	void HandleCommonHDel(char* szMsg, int iLen, int iSocketIndex);
	void HandleSortSetZAdd(char* szMsg, int iLen, int iSocketIndex);
	void HandleSortSetZRangeByScore(char* szMsg, int iLen, int iSocketIndex);
	void HandleSortSetZIncrby(char* szMsg, int iLen, int iSocketIndex);
	void HandleListLPush(char* szMsg, int iLen, int iSocketIndex);
	void HandleListLRange(char* szMsg, int iLen, int iSocketIndex);
	void HandleHashIncrby(char* szMsg, int iLen, int iSocketIndex);
	void HandleCommonSetKeyExpireTime(char* szMsg, int iLen, int iSocketIndex);
};
#endif // !__RedisThread_H__
