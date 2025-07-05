#ifndef __Mail_Handler_H__
#define __Mail_Handler_H__

#include "MsgHandleBase.h"
#include "msg.h"
#include <string>
#include <vector>
#include <map>
#include "msgqueue.h"	//消息队列
using namespace  std;

//用户个人邮件
const string KEY_USER_MAIL = "MJ_USER_MAIL:";
//系统邮件
const string KEY_SYS_MAIL = "MJ_SYS_MAIL";
//好友房信息
const string KEY_ROOM_NO = "MJ_FRIEND_ROOM_NO";          //可用房号
const string KEY_ROOM_USE_NO = "MJ_FRIEND_ROOM_USE_NO";  //在用房号
const string KEY_ROOM_INFO = "MJ_FRIEND_ROOM_INFO:%d";   //房间信息
const string KEY_IN_USE_CTRLAI = "MJ_GAME%d_IN_USE_CTRLAI";

const int RECRUIT_GAME[3] = { g_iGameHZXL,g_iGameXLCH, g_iGameXZDD };   //当前支持好友房的游戏
 
//虚拟ai
const string KEY_VIRTUAL_AI = "MJ_VIRTUAL_AI:%d";

typedef struct RDVirtualAIInfo
{
	int iVirtualID;
	int iAchieveLevel;//成就等级
	int iHeadFrame;//头像框
	int iExp;     //经验值
	int iPlayCnt;//总玩局数
	int iWinCnt;//总赢局数
	int iBufferA0;
	int iBufferA1;
	int iBufferA2;
	int iBufferA3;
	int iBufferA4;
	int iBufferB0;
	int iBufferB1;
	int iBufferB2;
	int iBufferB3;
	int iBufferB4;
	string strBuffer0;
	string strBuffer1;
	void clear()
	{
		iVirtualID = 0;
		iAchieveLevel = 0;//成就等级
		iHeadFrame = 0;//头像框
		iExp = 0;     //经验值
		iPlayCnt = 0;//总玩局数
		iWinCnt = 0;//总赢局数
		iBufferA0 = 0;
		iBufferA1 = 0;
		iBufferA2 = 0;
		iBufferA3 = 0;
		iBufferA4 = 0;
		iBufferB0 = 0;
		iBufferB1 = 0;
		iBufferB2 = 0;
		iBufferB3 = 0;
		iBufferB4 = 0;
		strBuffer0 = "";
		strBuffer1 = "";
	}
}RDVirtualAIInfo;

class MaillHandler:public MsgHandleBase
{
public:
	MaillHandler(MsgQueue *pSendQueue, MsgQueue *pLogicQueue);

	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen,int iSocketIndex);
	virtual vector<int> GetHandleMsgTypes(){};

	virtual void CallBackOnTimer(int iTimeNow);
	virtual void CallBackNextDay(int iTimeBeforeFlag, int iTimeNowFlag);

protected:

	void HandleComMailMsg(char* szMsg, int iLen);
	void HandleCheckRoomNoReq(char *msgData, int iSocketIndex, int iLen);
	//检测房号是否有效
	void HandleUpdateRoomNoStatus(char *msgData, int iSocketIndex, int iLen);//更新房间号状态
	
	void HandleServerAuthenReq(char * msgData, int iSocketIndex);
	void HandleGetVirtualAiInfo(char * msgData, int iSocketIndex, int iLen);
	RDVirtualAIInfo GetVirtualAIInfo(int iGameID, int iVirtualID);
	void CallBakcGetCtrlAiInfo(RdGetVirtualAIInfoReqDef* pMsgReq, int iSocketIndex);
	void HandleSetVirtualAiInfo(char * msgData, int iSocketIndex);
	int GetVirtualAchivel(int iAllCnt);
	void HandleCtrlAIStatus(char * msgData, int iSocketIndex);
	//来自center的修改房间信息

	int DBGetUserMailID(int iUserID, bool bAddMailID);
	int DBAddUserMailAward(int iUserID, char* cPropInfo1, char* cPropInfo2, char* cPropInfo3, char* cPropInfo4, char* cPropInfo5, int iAwardType);
	MsgQueue *m_pSendQueue;

	void CheckFriendRoomNoTimer();//定时将已失效房号转移至可用房号
	void CheckFriendRoomNoIni();//首次启动后检测是否需要初始化一批房号
	
	void RemoveRoomNoToWaitUse(int iRoomNo, int iGameID = 0);//将房间号从在用房间号挪至可用房间号

	void FillInUseCtrlAI();
	void CheckInUseCtrlAI();

	int m_iCheckFriendNoCDTm;//暂定每5分钟处理一次房号
	bool m_bIniCheckFriendRoomNo;

	MsgQueue *m_pLogicQueue;

	map<int, vector<int> > m_mapControlAIID;  //可用的控制ai，key=gameid
	map<int, vector<int> > m_mapInUseCtrlAI;   //在用控制ai
	int m_iTestCD;
};

#endif // !__RedisThread_H__
