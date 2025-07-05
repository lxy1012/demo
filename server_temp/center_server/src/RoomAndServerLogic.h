#ifndef __ROOM_AND_SERVER_LOGIC_H__
#define __ROOM_AND_SERVER_LOGIC_H__

#include <string>
#include <vector>
#include <map>
using namespace  std;
#include "msg.h"
#include "msgqueue.h"

//每个下线服务器的开放时间等信息信息
typedef struct OneGameServerDetailInfo
{
	int    iServerID;
	int iIP;
	short sPort;
	int iBeginTime;
	int iOpenTime;
	int iMaxNum;
	int iMaxTab;
	int iServerState;//房间状态(0关闭，1正常，2维护)
	int iCurrentNum;//当前在线人数
	int iUsedTabNum; //在用桌子数量
	int iSocketIndex;
	int iServerCrowdRate;
	int iInnerIP;
	short sInnerPort;
}OneGameServerDetailInfoDef;

typedef struct AssignRoomConf
{
	int iIfFreeRoom;
	int iRoomState;
	int iIPLimit;
}AssignRoomConfDef;

class AssignTableHandler;
class ClientAuthenHandler;
class RoomAndServerLogic
{
public:
	enum
	{
		CALL_ASSIGN = 1,
		CALL_CLIENT = 2,
	};

	RoomAndServerLogic(int type, void *pCallBase);

	static bool JudgeServerOpen(int iBeginTime, int iLongTime, int iNowTime);
	static bool SortFullServer(OneGameServerDetailInfoDef *pFirst, OneGameServerDetailInfoDef *pSecond);

	int IfRoomHaveIPLimit(int iIfFreeRoom);
	int JudgeIfServerCanEnter(int iServerID);//返回值0关闭，1正常，2维护

	//来自room服务器的当前中心服务器的配置处理
	void HandleGameRoomInfoReqMsg(char* msgData, int iSocketIndex);

	//来自游戏服务器的开放时间和在线人数等信息
	int	HandleGameServerAuthenMsg(char* msgData, int iSocketIndex);
	void HandleRefreshLeftTabNum(char * msgData, int iSocketIndex);
	//游戏服务器登陆/断开，如果是断开返回断开serverid
	void HandleGameServerDetailMsg(char* msgData, int iSocketIndex);//收到服务器详细信息
	void HandleGameServerSysOnlineMsg(char* msgData, int iSocketIndex);//更新当前服务器人数

	void SetSendQueue(MsgQueue* pSendQueue);

	OneGameServerDetailInfoDef* GetServerInfo(int iServerID);
	OneGameServerDetailInfoDef* GetServerInfo(uint iIP, short sPort);
	OneGameServerDetailInfoDef* GetCommendServerInfo();
	OneGameServerDetailInfoDef* GetMinimumServerInfo();
	AssignRoomConfDef* GetAssignRoomConf(int iRoomType);
	OneGameServerDetailInfoDef* GetMaintainServerInfo();//返回在维护的服务器信息，方便维护号测试

	int m_iCenterServerState;//当前中心服务器的状态
	int m_iAllPlayerNum;  //当前总在线人数
protected:
	void SortAllSubServers();
	map<int, OneGameServerDetailInfoDef>m_mapSubGameServerInfo;//下服务器的地址和人数等信息
	map<int, AssignRoomConfDef>m_mapRoomConfInfo;//first=ifFreeRoom
	vector<OneGameServerDetailInfoDef*>m_vcAllSubGameServer;
	char m_cCallFrom[32];
	MsgQueue *m_pSendQueue;
	AssignTableHandler* m_pAssign = NULL;
	ClientAuthenHandler* m_pClient = NULL;
};

#endif
