#ifndef __ASSIGN_TABLE_Handler_H__
#define __ASSIGN_TABLE_Handler_H__

#include "MsgHandleBase.h"
#include "RoomAndServerLogic.h"
#include <string>
#include <vector>
#include <map>
#include <queue>
using namespace  std;
#include "msg.h"
#include "msgqueue.h"
#include "hashtable.h"

#define HIGH_RATE 0
#define MID_RATE 1
#define LOW_RATE 2


typedef struct WaitTableUserInfo
{
	int iUserID;
	int iInServerID;//当前所在游戏服务器id
	int iTableStatus;//当前等待配桌状态（0 等待配桌 1已被配桌 2成功开桌 3已离开）
	int iReqAssignCnt;//请求分配桌子次数
	int iLastPlayerIndex;//近期同桌玩家索引
	int iLatestPlayUser[10];//近期玩家，最多保留10个
	char szIP[20];
	int iNowSection;//当前所在几率档位 0高档 1中档 2低档
	int iIfFreeRoom;//当前所在房间类型
	int iAllGameCnt;//近期总游戏局数
	int iAllWinCnt;//近期总赢局数
	int iAllLoseCnt;//近期总输局数
	int iMarkLv;
	int iPlayType;//玩法类型区分（只匹配相同玩法的玩家）
	int iMinPlayer;//最少需要玩家数量
	int iMaxPlayer;//最多需要玩家数量=iMinPlayer即固定玩家数量
	int iVipType;
	int iSpeMark;
	int iHeadImg;//玩家头像
	int iFrameID;
	int iFirstATHTime;//开始请求配桌时间
	char szNickName[60];//玩家昵称
	int iVTableID;//所在虚拟桌编号，0表示不在虚拟桌
	int iNVTableID;//所在待开局的桌号
}WaitTableUserInfoDef;


typedef struct VirtualTable
{
	int iVTableID;
	int iPlayType;//房间玩法
	int iUserID[10];//虚拟桌上玩家id(最多10个)
	int iFreeTm;  //上次释放时间
}VirtualTableDef;


typedef struct AssignFindUser
{
	int iUserNum;
	WaitTableUserInfoDef * pUser[10];
}AssignFindUserDef;

typedef struct CheckAssignTable
{
	VirtualTableDef* pVTable;
	int iTableAllUser;//桌子上原来所有人数
	int iTableAssignUser;//桌子上可以被配桌的玩家数
	vector<AssignFindUserDef>vcAssignUser;
}CheckAssignTableDef;

const int g_iMinVirtualID = 9000;//虚拟AI的id范围
const int g_iMaxVirtualID = 9999;

const int g_iMaxUseIntValue = 2147483640;

typedef struct VirtualAIInfo
{
	int iVirtualID;
	int iLatestTableUesr[10];//近期同过桌的真实玩家id
	int iLatestIndex;//近期同桌索引，按照0-9循环利用
	int iInVTableID;
}VirtualAIInfoDef;

class AssignTableHandler :public MsgHandleBase
{
public:
	AssignTableHandler(MsgQueue *pSendQueue);
	~AssignTableHandler();
	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex);
	virtual vector<int> GetHandleMsgTypes() {};

	virtual void CallBackOnTimer(int iTimeNow, int iTimeGap);
	virtual void CallBackNextDay(int iTimeBeforeFlag, int iTimeNowFlag);

	void SetRedisQueue(MsgQueue* pQueueForRedis);
protected:
	//来自room服务器的当前中心服务器的配置处理
	void HandleGameRoomInfoReqMsg(char* msgData, int iSocketIndex);

	//来自游戏服务器的开放时间和在线人数等信息
	void HandleGameServerAuthenMsg(char* msgData, int iSocketIndex);//游戏服务器登陆/断开
	void HandleGameServerDetailMsg(char* msgData, int iSocketIndex);//收到服务器详细信息
	void HandleGameServerSysOnlineMsg(char* msgData, int iSocketIndex);//更新当前服务器人数
	void HandleRefreshLeftTabNum(char* msgData, int iSocketIndex);

	//来自游戏服务器的配桌相关请求
	void HandleGetUserSitMsg(char *msgData, int iLen, int iSocketIndex);
	int GetPlayTypeByIdx(int i);
	
	//服务器请求给玩家配桌
	void HandleUserSitErrMsg(char *msgData, int iSocketIndex, int iLen);//服务器通知配桌玩家有异常处理
	void HandleUserAssignErrMsg(char *msgData, int iSocketIndex, int iLen);
	void HandleAssignTableOkMsg(char *msgData);
	void HandleUserLeaveMsg(char *msgData, int iSocketIndex);
	void HandleFreeVTableMsg(char *msgData, int iSocketIndex);

	int GetUserRate(WaitTableUserInfoDef *pUserInfo);
	void JudgeUserGameRateStat(WaitTableUserInfoDef *pUserInfo);

	void RemoveUserFromWait(int iUserID);

	void EnQueueSendMsg(char* _msg, int iLen, int iSocketIndex);
	void InitConf();

	bool CheckIfCanSitInOtherVTable(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int &iFindUserIndex, VirtualTableDef* pVTable,vector<AssignFindUserDef>&vcTempAssign, int iMaxRoomGap=2);
	void FindPointedVTableCanAssignUser(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int &iFindUserIndex, VirtualTableDef* pVTable, vector<AssignFindUserDef>&vcTempAssign, int iMaxRoomGap = 2);
	bool FindAllCanAssignUser(WaitTableUserInfoDef* pWaituser,WaitTableUserInfoDef* pFindUser[],int &iFindUserIndex, vector<CheckAssignTableDef>&vcAssignTable,int iMaxRoomGap);//iMaxRoomGap最大允许房间差,0只能同房间，1允许相邻房间，2允许跨2级房（当前仅有初级和高级）
	void TogetherUsersWithAssignTable(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int &iFindUserIndex, vector<CheckAssignTableDef>&vcAssignTable);
	
	string GetAssignUserLogInfo(AssignFindUserDef* pAssignUser);
	string GetAssignUsersLogInfo(vector<AssignFindUserDef>&vcTempAssign);
	string GetAssignTableLogInfo(CheckAssignTableDef* pAssignTable);
	string GetFindUserLogInfo(vector<WaitTableUserInfoDef*>&vcFindUser);
	string GetFindUserLogInfo(WaitTableUserInfoDef* pFindUser[], int iFindUserIndex);

	void SetWaitUserPlayType(WaitTableUserInfoDef* pWaituser,int iPlayType);
	void CallBackStartGame(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int iFindUserIndex,int iAINum, int iLastVAIIndex,int iLastVirtualAI[],int iSocketIndex);

	WaitTableUserInfoDef* GetFreeWaitUsers(int iUserID);
	void AddToFreeWaitUsers(WaitTableUserInfoDef* pWaitUser);
	WaitTableUserInfoDef* FindWaitUser(int iUserID);

	void SendNCAssignStatInfo(int iTimeFlag);
	bool CheckIfHisSameTable(WaitTableUserInfoDef* pWaituser1, WaitTableUserInfoDef* pWaituser2);
	bool CheckIfRoomTypeLimit(WaitTableUserInfoDef* pWaituser1, WaitTableUserInfoDef* pWaituser2);
	bool CheckIfUsersCanSitTogether(WaitTableUserInfoDef* pWaitUser, WaitTableUserInfoDef* pJudgeUser);

	map<int, WaitTableUserInfoDef*>m_mapAllWaitTableUser;
	
	int m_iForbidSameTableNum;//禁止和最近多少个已同桌过玩家同桌，默认10个
	int m_iLowRate;//几率低于多少算低利率(默认45%)
	int m_iHightRate;//几率高于多少算高利率(默认55%)
	int m_iRateMethod;//计算几率方法（默认0 即赢次数/总次数 1=（赢次数+平次数）/总次数）
	int m_iHighSitWithLow;//是否高低档可以匹配（默认可以）
	int m_iForbidRoom8With5;//是否禁止vip房和初级房同桌
	int m_iForbidRoom8With6;//是否禁止vip房和中级房同桌
	int m_iForbidRoom7With5;//是否禁止高级房和初级房同桌
	int m_iForbidRoom6With5;//是否禁止中级房和初级房同桌
	int m_iForbidRoom7With6;//是否禁止高级房和中级房同桌
	int m_iForbidMacthRoom;//是否禁止比赛房初赛和决赛同桌
	int m_iIfForbidPreMixRoom;//是否禁止提前混服（=0可以在开局前提前让玩家混服坐在一起，即客户端可以提前看到用户进出，否则就是一次性配齐混服的玩家开局）
	int m_iSameTmLimitSec;//入座请求时差多少s内不匹配在一起，及避免同时发入座请求的坐在一起（默认-1不开启）
	int m_iMaxMark9Num;//桌上最多允许mark9玩家数量（-1未开启，0表示mark9的不能和其他人同桌，其他正值如1表示一桌最多1个mark9）
	int m_iFindSameRoomSec;//等待时间<=m_iFindSameRoomSec时只找同房间
	int m_iFindNeighborRoomSec;//m_iFindSameRoomSec<等待时间<=m_iFindNeighborRoomSec可找相邻房间,>m_iFindNeighborRoomSec可找跨2级房间

	int m_iReadConfCD;
	int m_iATHTime;

	RoomAndServerLogic* m_pRoomServerLogic;
	MsgQueue *m_pSendQueue;
	NCAssignStaInfoDef m_stAssignStaInfo;
	//内部缓存用
	vector<WaitTableUserInfoDef*>m_vcFreeWaitUsers;
	int m_iRoomType[5];//目前只留了4，5，6，7，8这五种房间
	int m_iSetRecordLogCDConf;//同步统计信息配置时间
	int m_iRecordLogCDTm;//同步统计信息倒计时
	MsgQueue *m_pQueueForRedis;

	int m_iAssignAI;       //是否可匹配ai
	int m_iWaitAISec[3];   //可配桌ai的最小匹配秒数（m_iWaitAICnt次配桌后依旧没有配桌成功，同ai入座开局），-1表示不匹配ai
	int m_iWaitAIOnline[2];  //可配桌ai服务器总人数区间
	int m_iPutAIRate[3];   //每轮请求时人数不足时，加入虚拟AI入桌的万分比概率(每轮只增加一个虚拟AI,且不能超过可开局人数)
	
	char m_szBuffer[1024];

	//虚拟AI
	vector<VirtualAIInfoDef*> m_vcFreeVirtulAI;//所有可用虚拟AI队列
	HashTable   m_hashVirtual;

	VirtualAIInfoDef* GetFreeVirtualAI(bool bCanCreate = false); //取得可用虚拟AI
	void ReleaseVirtualAI(VirtualAIInfoDef* pVirtualAI);//释放虚拟AI
	void ReleaseVirtualAI(int iVirtualID);//释放虚拟AI

	bool CheckIfCanSitWithVirtualAI(int iTableRealUser[], VirtualAIInfoDef* pAIInfo, int iVirtualID);
	VirtualAIInfoDef* GetFreeValidVirtualAI(int iTableRealUser[]);//iTableRealuser表示桌上真实玩家，需要让每个真实玩家不能和近期有过同桌的ai相遇
	void SetVirtualAITableUser(int iTableUser[]);//和AI有过同桌记录的真实玩家都要被记录到虚拟AI近期同桌信息中，短期内不让玩家再看到这些虚拟AI

	int GetPlayNum(int iPlayType);
	void AddFindPlayerFromVTable(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[],int &iFindUserIndex,int iLastVirtualAI[],int &iLastVAIIndex);
	//虚拟桌
	vector<VirtualTableDef*>m_vcFreeVTables;
	VirtualTableDef* GetFreeVTable(int iVTableID = 0);
	void AddToFreeVTables(VirtualTableDef* pVTable);
	int m_iVTableID;
	map<int, VirtualTableDef*>m_mapAllVTables;
	VirtualTableDef* FindVTable(int iVTableID);
	void CheckRemoveVTableUser(WaitTableUserInfoDef* pUser);
	void RemoveVTableUser(VirtualTableDef* pVTable,int iUserID,bool bEraseFromMap);
	void AddUserToVTable(int iUserID,WaitTableUserInfoDef* pUser,int iVTableID);
	void AddUserToVTable(int iUserID, WaitTableUserInfoDef* pUser, VirtualTableDef* pVTable);
	int GetUserVTableAssignUser(int iVTableID,int iAssignUser[],int &iAssignCnt, VirtualTableDef** pVTable);
	int GetUserVTableAssignUser(VirtualTableDef* pVTable);

	void HandleGetParamsMsg(void * szMsg, int iSocketIndex);
	void GetServerConfParams(vector<string>& vcKey);
	void GetAssignConf();
	
	//新增控制AI信息的请求和获取的转发
	void HandleSendGetVirtualAIInfoReq(char *msgData, int iLen);	//收到游戏服务器请求获取虚拟AI信息 ，转发给主redis
	void HandleGetVirtualAIMsg(char *msgData, int iLen);			//收到主Redis回应的虚拟AI信息，转发给到游戏服务器
	void HandleReleaseRdCTLAINodeReq(char *msgData, int iLen);		//收到游戏服释放控制AI节点消息 ，转发给主Redis

	bool IfDynamicSeatGame();   //是否动态人数游戏
};

#endif
