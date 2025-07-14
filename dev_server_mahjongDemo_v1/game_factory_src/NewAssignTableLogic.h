#ifndef __NEW_ASSIGN_TABLE_LOGIC__
#define __NEW_ASSIGN_TABLE_LOGIC__

#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>      //sleep()
#include <stdlib.h>      //rand()
#include <vector>
#include <map>
using namespace std;
 
#include "factorytableitem.h"


typedef struct OneTableUsers  //已经配好桌的一组玩家
{
	int iUserID[10];
	int iIfFreeRoom[10];//玩家的房间类型
	bool bIfInServer[10];//是否都在本服，被解散后本服的玩家需要重新请求配桌
	int iWaitSec;//等待人齐倒计时，时间到了也重新申请配桌
	int iTableNum;//配好桌后就先分配好桌号，并将本服玩家加入到该桌
	int iNeedPlayerNum;//本桌玩法需要玩家数量
	int iTablePlayType;
}OneTableUsersDef;

typedef struct NAChangeServerUser  //需要换服用户的记录
{
	int iUserID;
	int iSocketIndex;
	int iNewServerIP;
	int iNewServerPort;
	int iNewInnerIP;
	int iNewInnerPort;
	int iRoomNo;
}NAChangeServerUserDef;

typedef struct ClientChangeUser  //需要换服用户的记录
{
	int iUserID;
	int iSocketIndex;
	int iRoomNo;
	int iServerID;
}ClientChangeUserDef;

typedef struct VirtualTable
{
	int iVTableID;
	int iMinPlayerNum;
	int iUserID[10];
	bool bNextReadyOK[10];//下局还在桌上的用户是否准备好
	FactoryPlayerNodeDef *pNodePlayers[10];//注意使用前确保节点正常
	int iPlayType;
	int iCDSitTime;//发送入座请求时间
	int iIfSendAssignReq;//是否已经发送过配桌请求
	int iCTLSendCTLAIInfoTime;//每次倒计时接送发送随机个数个AI信息
	int iAIInfoSendDone;		//已完成数量
}VirtualTableDef;


const int g_iMinControlVID = 5000;//1-该范围的id表示控制ai
const int g_iMaxControlVID = 5999;//1-该范围的id表示控制ai

const int g_iMinVirtualID = 9000;
const int g_iMaxVirtualID = 9999;

const int g_iMaxFdRoomID = 9999;

const int g_iControlRoomID = -999;//标记进入ai控制的桌

class GameLogic;
class NewAssignTableLogic
{
	
public:
	NewAssignTableLogic();
	~NewAssignTableLogic();
	void Ini(GameLogic *pGameLogic);
	void HandleNetMsg(int iMsgType,void *pMsgData,int iMsgLen = 0);

	int GetIfHaveRoomNum(int iUserID);
	void CheckIfNeedSitDirectly(int iUserID);//登录成功后判断是否是等待换服的玩家，是的话，直接入座
	int CheckIfInWaitTable(int iUserID,bool bLeave);
	void CallBackUserNeedAssignTable(FactoryPlayerNodeDef *nodePlayers,bool bFromSitReq = false);

	void CallBackOneSec(long iTime);

	void CallBackTableSitOk(int iUserID);

	void CallBackTableOK(int iUserID[]);

	void CallBackUserLeave(int iUserID,int iVTableID);

	void CallBackUserChangeServer(int iUserID);

	void CallBackHandleUserGetRoomRes(void *pMsgData,int iMsgLen);

	bool CheckIfUserChangeServer(int iUserID);

	void CallBackFreeVTableID(int iVTableID);//通知中心服务器回收虚拟桌号
	void CallBackDissolveTableToVTable(FactoryTableItemDef *pTableItem);
	void CallBackNextVTableReady(int iVTableID, FactoryPlayerNodeDef *nodePlayers);//判断下局在虚拟桌等待继续的玩家是否可以继续参与配桌

	VirtualTableDef* FindVTableInfo(int iVTableID);

	VirtualTableDef* FindAIControlVTableInfo(int iUserID,int &iVCIndex);

	bool CheckUserIfVAI(int iUserID);

	VirtualTableDef* CallBackPlayWithContolAI(FactoryPlayerNodeDef *nodePlayers,bool bReadyOK, FactoryPlayerNodeDef *pAllNode[10]);//设置玩家和控制AI玩
	void CallBackTableNeedAssignTable(VirtualTableDef* pVTable, FactoryPlayerNodeDef * pLeftPlayer);   //整桌有效玩家重新去中心配桌
	void ReleaseRdCtrlAI(int iVirtualID);
private:
	GameLogic *m_pGameLogic;
	vector<OneTableUsersDef>m_vcTableUsers;
	map<int,NAChangeServerUserDef>m_mapChangeServerUsers;
	map<int,ClientChangeUserDef>m_mapClientChangeUsers;

	void DismissWaitTableByIndex(int iIndex);

	void CallBackUserNeedAssignTable(int iUserID);
	
	void HandleGetUserSitMsg(void *pMsgData);
	void HandleUserSitErrMsg(void *pMsgData);
	void HandleUserChangeReq(void *pMsgData);
	void HandleRequireUserLeaveReq(void* pMsgData);
	void HandleVirtualAssignMsg(void *pMsgData);
	void HandleGetVirtualAIMsg(void *pMsgData,int iMsgLen);
	void HandleGetVirtualAIResMsg(void *pMsgData, int iMsgLen);
	//配置
	//等待其他服玩家换服的时间
	int m_iTableWaitSec;
	int m_iConfCDTime;
	int m_iWaitTableResSec;
	int m_iRateMethod;//计算几率方法（默认0 即赢次数/总次数 1=（赢次数+平次数）/总次数）
	int m_iLowRate;//几率低于多少算低利率(默认45%)
	int m_iHightRate;//几率高于多少算高利率(默认55%)
	void ReadConf();

	void HandleClientChangeReq(void * pMsgData);

	char m_cBuffer[1024];

	vector<VirtualTableDef*>m_vcFreeVTables;
	vector<VirtualTableDef*>m_vcAIControlVTables;//用于AI控制专用虚拟桌
	VirtualTableDef* GetFreeVTable(int iVTableID);
	void AddToFreeVTables(VirtualTableDef* pVTable);
	map<int, VirtualTableDef*>m_mapAllVTables;
	void CheckRemoveVTableUser(int iVTableID,int iUserID, bool bNoticeOthers,bool bNoticeSelf = true,bool bEraseVTable = true,bool bClearVTable = true);
	void SendVirtualTableInfo(VirtualTableDef* pVirtualTable,int iPointedUser = 0);
	void SendVirtualTableInfoForCTL(VirtualTableDef* pVirtualTable, int SendNum);
	void ResetTableNumExtra(OneTableUsersDef *pOneTableInfo, int iNextVTableID, int iCurNum);
	void GetUserVTableInfo(int iUserID,int &iVTableID,int &iNumExtra);

	void SendGetVirtualAIInfoReq(int iVirtualID,int iVTableID,int iNeedNum = 0);
	
	bool CheckVTableReadySatus(VirtualTableDef* pVTable,bool bTimeOut);
	bool CallBackCheckAddControlAI(VirtualTableDef* pVTable);
	bool IfDynamicSeatGame();   //是否动态人数游戏
};


#endif //__ACTIVITY_COMPETE_LOGIC__
