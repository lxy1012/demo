#ifndef __CLIENT_AUTHEN_Handler_H__
#define __CLIENT_AUTHEN_Handler_H__

#include "MsgHandleBase.h"

#include <string>
#include <vector>
#include <map>
using namespace  std;
#include "msg.h"
#include "msgqueue.h"
#include "RoomAndServerLogic.h"
#include "AssignTableHandler.h"

class ClientAuthenHandler :public MsgHandleBase
{
public:
	ClientAuthenHandler(MsgQueue *pSendQueue);
	~ClientAuthenHandler();

	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex);
	virtual vector<int> GetHandleMsgTypes() {};
	virtual void CallBackOnTimer(int iTimeNow);
protected:
	//来自room服务器的当前中心服务器的配置处理
	void HandleGameRoomInfoReqMsg(char* msgData, int iSocketIndex);

	//来自游戏服务器的开放时间和在线人数等信息
	void HandleGameServerAuthenMsg(char* msgData, int iSocketIndex);//游戏服务器登陆/断开
	void HandleGameServerDetailMsg(char* msgData, int iSocketIndex);//收到服务器详细信息
	void HandleGameServerSysOnlineMsg(char* msgData, int iSocketIndex);//更新当前服务器人数

	//客户端用户登录请求
	void HandleAuthenReqRadius(char *msgData, int iSocketIndex);

	MsgQueue *m_pSendQueue;
	RoomAndServerLogic* m_pRoomServerLogic;

	char m_szBuffer[1024];
};

#endif
