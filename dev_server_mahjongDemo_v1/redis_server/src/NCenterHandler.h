#ifndef __NCENTER_HANDLER_H__
#define __NCENTER_HANDLER_H__

#include "MsgHandleBase.h"

#include <string>
#include <vector>
#include <map>
using namespace  std;
#include "msg.h"
#include "msgqueue.h"

class NCenterHandler :public MsgHandleBase
{
public:
	NCenterHandler(MsgQueue *pSendQueue);

	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex);
	virtual vector<int> GetHandleMsgTypes() {};

	virtual void CallBackOnTimer(int iTimeNow);

protected:

	MsgQueue *m_pSendQueue;

	int m_iRoomType[5];

	void HandleGetUserInfoReq(char* szMsg, int iLen, int iSocketIndex);
	void  HandleSetLatestUserMsg(char *msgData, int iSocketIndex);
};

#endif // !__RedisThread_H__
