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
	//����room�������ĵ�ǰ���ķ����������ô���
	void HandleGameRoomInfoReqMsg(char* msgData, int iSocketIndex);

	//������Ϸ�������Ŀ���ʱ���������������Ϣ
	void HandleGameServerAuthenMsg(char* msgData, int iSocketIndex);//��Ϸ��������½/�Ͽ�
	void HandleGameServerDetailMsg(char* msgData, int iSocketIndex);//�յ���������ϸ��Ϣ
	void HandleGameServerSysOnlineMsg(char* msgData, int iSocketIndex);//���µ�ǰ����������

	//�ͻ����û���¼����
	void HandleAuthenReqRadius(char *msgData, int iSocketIndex);

	MsgQueue *m_pSendQueue;
	RoomAndServerLogic* m_pRoomServerLogic;

	char m_szBuffer[1024];
};

#endif
