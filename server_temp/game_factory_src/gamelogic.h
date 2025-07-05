#ifndef __GAMELOGIC_H__
#define __GAMELOGIC_H__

//系统头文件
#include <stdio.h>
#include <map>
#include <vector>
using namespace std;

//项目相关头文件
#include "msgqueue.h"
#include "hashtable.h"
#include "factorymessage.h"
#include "factorytableitem.h"
#include "NewAssignTableLogic.h"
#include "factoryplayernode.h"
#include "GameEvent.h"
#include "TaskLogic.h"

#define BACK_UP_TABLE    350     //备份桌子，350-399
#define MAX_TABLE_NUM    420     //桌数,应该够了吧

//公有状态定义(所有可能的状态)
#define PS_WAIT_USERINFO	 1		 //等待RADIUS认证回应用户信息，
#define PS_WAIT_DESK		 2		 //等待坐下（找寻座位）
#define PS_WAIT_READY		 3       //等待按下“开始”
#define PS_WAIT_DEMAND		 4		 //等待叫牌
#define PS_WAIT_BASE		 5		 //等待埋牌
#define PS_WAIT_SEND		 6       //等待出牌
#define	PS_WAIT_DEAL		 7		 //等待发牌结束同步(ZJH,SH,TDK)
#define PS_WAIT_NEXTJU		 8   	 //16张麻将 一局结束但一将未结束 
#define PS_WAIT_RESERVE_TWO  9		 //预留2


class PropHandle;
class ClientEpollSockThread;
class ServerEpollSockThread;
class CMainIni;
class GameEvent;
class GameLogic
{
public:
	GameLogic();
	virtual ~GameLogic();

	//初始化函数
	virtual int Initialize(CMainIni* pMainIni, ServerBaseInfoDef* pServerBaseInfo);

	int m_iUpdateCenterServerPlayerNum;//更新中心服务器的人数
	int m_iSendZeroOnlineCnt;//服务器进入关闭或维护后发送0人数次数
	int m_iUpdateGameNum;//更新游戏牌编号记录局数
	//在每个游戏的子类一定要在构造函数里面赋值的成员变量
	int	m_iMaxPlayerNum;//桌上最大的玩家数

	int m_iUseringTabNum;  //当前使用中桌子数量
	int m_iLastSendTbNum;  //上次上报桌子数量	
	void SendLeftTabNum(int iAdd, bool bMastSend = false);  //当前使用桌子数量发生变化调用

	//每个游戏必须重载，并且每个游戏都相同的
	virtual void SendAllPlayerInfoRes(FactoryPlayerNodeDef* pPlayerNode) {}//发送玩家列表函数,必须重载
	virtual FactoryTableItemDef* GetTableItem(int iRoomID, int iTableIndex) { return NULL; };//取得桌指针，必须重载
	virtual void* GetFreeNode() { return NULL; };//取得空闲节点.必须重载.
	virtual void ReleaseNode(void* pNode) {};//释放节点.必须重载
	virtual void RemoveTablePlayer(FactoryTableItemDef* pTableItem, int iTableNumExtra) {};//从桌结构移除玩家节点,必须重载
	virtual void JudgeAgainLoginTime(int iTime) {};//判断重入时间，必须重载
	virtual void FreshRoomOnline() {};//刷新服务器在线人数,必须重载
	virtual void AllUsersSendNoitce(void* pDate, int iType) {}//向游戏中所有用户发送公告通知


	//每个游戏必须重载，根据自己的需要添加函数内容
	virtual void HandleOtherGameNetMessage(int iMsgType, int iStatusCode, int iUserID, void* pMsgData) {}//游戏特殊消息继承处理(需根据不同消息来源和消息类型结合处理)
	virtual void CallBackReadyReq(void* pPlayerNode, void* pTableItem, int iRoomID) {}//所有玩家都点了开始游戏后的处理函数，必须重载
	virtual void HandleAgainLoginReq(int iUserID, int iSocketIndex) {}//掉线重入通知
	virtual void HandlePlayStateDisconnect(int iStatusCode, int iUserID, void* pMsgData) {}//游戏状态下掉线也要重载处理.因为有些游戏状态没办法做重入
	virtual void AutoSendCards(int iUserID) {}//自动出牌
	virtual bool IfKickFormerGameAgainLogin() { return true; };//游戏过程重复登录进游戏，是否踢掉前面登录的端

	//根据自己的需要，可以重载函数
	virtual bool CallbackFirstHandleRadiusServerMsg(char* pMsgData, int iMsgLen) { return false; };//返回true底层就不在处理，有子类接管
	virtual void CallbackHandleRadiusServerMsg(char* pMsgData, int iMsgLen) {};

	virtual bool CallbackFirstHandleRoomServerMsg(char* pMsgData, int iMsgLen) { return false; };//返回true底层就不在处理，有子类接管
	virtual void CallbackHandleRoomServerMsg(char* pMsgData, int iMsgLen) {};

	virtual bool CallbackFirstHandleRedisServerMsg(char* pMsgData, int iMsgLen) { return false; };//返回true底层就不在处理，有子类接管
	virtual void CallbackHandleRedisServerMsg(char* pMsgData, int iMsgLen) {};

	virtual bool CallbackFirstHandleCenterServerMsg(char* pMsgData, int iMsgLen) { return false; };//返回true底层就不在处理，有子类接管
	virtual void CallbackHandleCenterServerMsg(char* pMsgData, int iMsgLen) {};

	virtual bool CallbackFirstHandleLogServerMsg(char* pMsgData, int iMsgLen) { return false; };//返回true底层就不在处理，有子类接管
	virtual void CallbackHandleLogServerMsg(char* pMsgData, int iMsgLen) {};

	virtual bool CallbackFirstHandleBullServerMsg(char* pMsgData, int iMsgLen) { return false; };//返回true底层就不在处理，有子类接管
	virtual void CallbackHandleBullServerMsg(char* pMsgData, int iMsgLen) {};

	virtual void HandleOther1ServerMsg(char* pMsgData, int iMsgLen) { };//完全有子类根据自己额外连接的服务器进行处理
	virtual void HandleOther2ServerMsg(char* pMsgData, int iMsgLen) { };//完全有子类根据自己额外连接的服务器进行处理

	virtual void CallBackHalfHour(){}//半个小时的定时器
	virtual void CallBackFiveMinutes(){}//5分钟的定时器
	virtual void CallBackTenMinutes() {}//10分钟的定时器
	virtual void CallBackFifteenMinutes() {}//15分钟的定时器
	virtual void CallBackOneMinute() {};//每分钟定时器
	virtual void CallBackSkipToDay() {};//跨天
	virtual void DoOneHour(){};//一个小时的定时器
	virtual void DoTenSecTime(){};//十秒的定时器
	virtual void OneSecTime(){};//1秒的定时器

	virtual void OnUserInfoDisconnect(int iUserID,void *pMsgData = NULL);//等待Radius回应时掉线
	virtual void OnFindDeskDisconnect(int iUserID,void *pMsgData, bool bNeedChangeServer = false);//找座位时候断开
	virtual void OnReadyDisconnect(int iUserID, void *pMsgData = NULL); //等待开始时候断开
	virtual void ResetTableState(void *pTableItem){}//逃跑时重置每个游戏桌子特殊值的
	virtual int CallBackFaceGroundAuthen(FactoryPlayerNodeDef *nodePlayers, FaceGroundAuthenDef* pMsgRes) { return 0; }  //切换到前台重新认证时二次确认消息填充，返回消息长度
	virtual void CallBackBeforeAuthByRadius(FactoryPlayerNodeDef *nodePlayers) {};  //客户端向radius认证前回调
	virtual void CallBackRDUserInfo(FactoryPlayerNodeDef *nodePlayers){};//登录成功后的回调（子类可在此获取登录成功后才可以获取的游戏信息）
	virtual void CallBackDisconnect(FactoryPlayerNodeDef *nodePlayers){};//部分类型游戏在离开游戏前需要用到清楚玩家节点的信息
	virtual void CallBackRDSendAccountReq(FactoryPlayerNodeDef *nodePlayers, GameUserAccountReqDef*msgAccountReq, GameUserAccountGameDef* pAccountGameInfo){};//计费时调用下子类的
	virtual void AgainLoginOtherHandle(FactoryPlayerNodeDef *nodePlayers);
	virtual void HandleUserGameChargeAndRefresh(void * pMsgData);
	virtual bool CallBackAuthenSuccExtra(AuthenResDef *pAuthenRes) { return true; };  //认证成功后发送给客户端的认证额外消息填充
	//掉线重入后的其他处理函数
		
	virtual void CallBackSitSuccedDownRes(FactoryTableItemDef *pTableItem,FactoryPlayerNodeDef *nodePlayers){};//玩家坐下后回应每个游戏需要的信息
	virtual void CallBackSitSuccedBeforePlayerInfoNotice(FactoryTableItemDef *pTableItem,FactoryPlayerNodeDef *nodePlayers){}; 
	virtual void CallBackHandleTruss(FactoryTableItemDef *pTableItem,FactoryPlayerNodeDef *nodePlayers, int iTableNum = 0){};//处理捆绑玩家，该函数是在逃跑消息函数中处理，nodePlayers是逃跑玩家节点
		
	virtual bool CheckGameServerVersionNum(AuthenReqDef* pAuthenInfo);//校验客户端版本信息
	virtual int CheckAuthExtraFlag(const char *pMsgData) { return 0; }  //校验登陆认证中cExtraFlag1和cExtraFlag2的合法性，返回踢出类型
	virtual bool JudgeIsPlayWithAI(FactoryPlayerNodeDef* nodePlayers, bool bAllCheck = false) ;
	//游戏在用户验证成功以及继续时，是否有额外的踢出判断（注意自己特有的踢出类型从100起），如需踢出，填充pMsgKick即可
	virtual void CallBackJudgeKickOut(FactoryPlayerNodeDef *nodePlayers, KickOutServerDef* pMsgKick) {};
	virtual void CallBackGameOver(FactoryTableItemDef * pTableItem, FactoryPlayerNodeDef * nodePlayers, bool bNeedAssign = false);//bNeedAssign本局结束后是否需要立即去中心服务器配桌

	virtual bool IfNeedFriends() { return false; }  //是否需要获取好友列表
	virtual bool IfNeedPlatformProp() { return true; }//是否需要获取平台道具，默认需要获取
	virtual void IfNeedGameProp(int iGameId[]) {};//是否需要获取指定gameid专属的道具（最多5个游戏）
	virtual void IfNeedPointedProp(int iPropID[]) {};//是否需要获取指定的非平台类型的道具（最多获取5个，和IfNeedGameProp不能同时生效）
	virtual bool IfNeedLatestPlayer() { return true; }//是否需要获取近期同桌玩家信息，默认需要获取
	virtual bool IfNeedMatchInfo(FactoryPlayerNodeDef* nodePlayers) { return false; }//是否需要获取比赛信息
	virtual bool IfNeedTaskInfo(){ return true; }//是否需要获取任务信息，默认需要获取
	virtual bool IfNeedDecorateInfo() { return true; }//是否需要获取任务信息，默认需要获取
	virtual bool IfNeedComTaskInfo() { return true; }//是否需要获取通用信息，默认需要获取
	virtual bool IfNeedCheckLimit(FactoryPlayerNodeDef* nodePlayers) { return true; }//是否需要检测上下限
	virtual bool IfNeedRecentTask() { return false; } //是否需要获取近期奖券任务记录
	virtual bool CallBackJudgePlayType(int &iPlayType) { return true; } //玩家认证消息中palyType合法性校验
	virtual int GetNormalPlayType() { return 0; }  //快速开始或者未携带玩法的玩家，匹配ai后赋值玩法
	virtual void CallBackInteractPropSucc(UseInteractPropResDef useInfo) {}
	virtual int CallBackRecentExtra() { return 0; }  //同桌玩家信息记录额外字段填充

	virtual bool IfNoticeNeedAchieve() { return false; }  //通知客户端所有玩家信息时是否需要成就信息

	void OnTimer(int iSecGap, time_t now);//iSecGap距离上次调用的时间差
	void HandleRadiusServerMsg(char* pMsgData, int iMsgLen);//处理radius服务器的消息
	void HandleRoomServerMsg(char* pMsgData, int iMsgLen);//处理room服务器的消息
	void HandleRedisServerMsg(char* pMsgData, int iMsgLen);//处理redis服务器的消息
	void HandleCenterServerMsg(char* pMsgData, int iMsgLen);//处理Center服务器的消息
	void HandleLogServerMsg(char* pMsgData, int iMsgLen);//处理Log服务器的消息
	void HandleGameNetMessage(int iMsgType, int iStatusCode, int iUserID, void* pMsgData);//处理客户端主端口消息
	void HandleBullServerMsg(char* pMsgData, int iMsgLen);  //处理bull服务器消息
	void HandleCheckRoomIDMsg(void * pMsgData);
	virtual void HandleGameAllEpollMessage(int iMsgType, int iStatusCode, int iUserID, void* pMsgData) {}   //群发线程处理
	virtual bool IfSinglePlayerGame() { return false; }  //是否单人游戏

	time_t GetPointDayStartTimestamp(int iYear, int iMonth, int iDay);

	void RefreshConfInfo();//定时更新所需要的游戏配置文件	
	void SkipToDay();//跨天(注意是23:59:59调用，不是24小时)
	void CLSendAllClient(void *msg,int iMsgType,int iMsgLen,int iRoomID,int iUserID = 0,int iType = 0,int iAllSocketIndex = -1);//群发，iType消息类型，0表示群发所有人 1针对指定分组(一桌/指定编号的一组)群发 2则针对指定端口单发
	
	//有触发消息事件的时候，参数为：消息指针，消息长度
	virtual int OnEvent(void *pMsg, int iLength);
		
	virtual void GetTablePlayerInfo(FactoryTableItemDef *pTableItem,FactoryPlayerNodeDef *nodePlayers =NULL);
	void SetUserShowMoney(PlayerInfoResNorDef* playerInfoNor, FactoryPlayerNodeDef * nodePlayers, int idx);
	//取得桌上所有玩家的信息	

	//所有队列
	MsgQueue* m_pQueToClient;  //用于与客户端通信队列
	MsgQueue* m_pQueToAllClient;//用于与客户端群发通信队列

	MsgQueue* m_pQueToRadius;  //用于与Radius通信队列
	MsgQueue* m_pQueToLog;//用于与LOG服务器通信队列
	MsgQueue* m_pQueToCenterServer;	//用于与中心服务器通信队列
	MsgQueue* m_pQueToBull;//用于与bull服务器通信队列
	MsgQueue* m_pQueToAct;	//用于和活动服务器通信队列
	MsgQueue* m_pQueToRedis;	//用于和redis通信队列
	MsgQueue* m_pQueToRoom;//用于和room通信队列
	MsgQueue* m_pQueToOther1;//用于和其他底层服务器通信队列1
	MsgQueue* m_pQueToOther2;//用于和其他底层服务器通信队列2

	MsgQueue* m_pQueLogic;//所有进入logic判断的消息队列

	//所有线程信息
	ClientEpollSockThread* m_pClientEpoll;
	ClientEpollSockThread* m_pClientAllEpoll;

	ServerEpollSockThread* m_pRadiusEpoll;  //用于与Radius通信线程
	ServerEpollSockThread* m_pLogEpoll;//用于与LOG服务器通信线程
	ServerEpollSockThread* m_pCenterServerEpoll;//用于与中心服务器通信线程
	ServerEpollSockThread* m_pBullEpoll;//用于与bull服务器通信线程
	ServerEpollSockThread* m_pActEpoll;	//用于和活动服务器通信线程
	ServerEpollSockThread* m_pRedisEpoll;//用于和redis通信线程
	ServerEpollSockThread* m_pRoomEpoll;//用于和room通信线程
	ServerEpollSockThread* m_pOther1Epoll;	//用于和其他底层服务器1通信线程
	ServerEpollSockThread* m_pOther2Epoll;	//用于和其他底层服务器2通信线程

	//发送计费请求（注意调用该接口时用户节点内的money，integral，diamond必须是已经加入需要计费部分的）
	bool RDSendAccountReq(GameUserAccountReqDef*msgAccountReq, GameUserAccountGameDef* pAccountGameInfo,bool bForceSend = false); //返回值本次是否发送了计费请求
	
	//参数说明：1.发送对象节点的SOCKET下标，2.信息结构，3.消息类型，4.消息长度
	void CLSendSimpleNetMessage(int iIndex,void *msg,int iMsgType,int iMsgLen);//发向客户端的消息	
	void CLSendPlayerInfoNotice(int iIndex, int iTablePlayerNum);   //发送桌上玩家信息,在调用前必须先填充桌玩家结构数组m_PlayerInfoDesk[10]
	bool CheckUserDecorateTm(FactoryPlayerNodeDef* nodePlayers);

	//新配桌流程相关
	int GetValidTableNum();	
	bool CallBackUsersSit(int iUserID, int iIfFreeRoom, int iTableNum, char cTableNumExtra, int iTablePlayerNum, int iRealNum = 0);
	virtual long long GetSimulateMoney(int iRoomIndex, int iOtherRoomIdx, long long iInMoney) { return 0; };//获取某个玩家在指定房间显示的钱
	virtual bool CheckTableCanSit(OneTableUsersDef *pOneTableInfo, int iNextVTableID, int iCurNum) { return false; }  //检查当前iNextVTableID桌子上玩家是否可以入座真实桌子
	//虚拟桌上玩家是否已经开局
	virtual bool CallBackCheckVTableSatus(VirtualTableDef *pVTabl, FactoryPlayerNodeDef *nodePlayers) { return false; };
	//当前玩家的带入金额是否大于0
	virtual bool JudgPlayerStatus(int iUserID) { return false; };
	//判断换桌玩家身上金额是否满足限制
	virtual bool JudgPlayerBringInMoney(int iUserID) { return true; };
	//换桌玩家判断带入
	virtual bool SpeJudgPlayerBringMoney(int iUserID) { return true; };
	//新配桌流程相关 end

	//玩家结点HASH表
	HashTable   hashIndexTable;		//以SocketIndex为索引
	HashTable   hashUserIDTable;	//以UserID为索引
	
	PlayerInfoResNorDef m_PlayerInfoDesk[10];//桌的玩家信息数组（临时整合用）
	int m_iPlayerInfoDeskNum;
	char m_cPlayerInfoBuff[10240];
	int m_iPlayerInfoBuffLen;

	char m_cGameLogBuff[10240];
	int m_iGameLogBuffLen;
	
	NewAssignTableLogic* m_pNewAssignTableLogic;

	map<int, FactoryTableItemDef*>m_mapTableWaitNext;//在等待倒计时内开始下一局的桌子
	//记时用
	int m_iTimerHalfHourCnt;
	int m_iTimerFiveMinCnt;
	int m_iTimerTenMinCnt;
	int m_iFifteenMinCnt;
	int m_iOneMinCnt;
	int m_iTimerHourCnt;
	int m_iTimerOneDayCDCnt;
	int m_iTimerTenSecCnt;
	int m_iTodayDayFlag;//今日时间编号如20220128

	char m_cBuffForAccount[2048];
	int m_iBuffAccountLen;

	virtual void OnWaitLoginAgin(int iUserID, void *pMsgData = NULL);
	
	static ServerBaseInfoDef* m_pServerBaseInfo;
	
	void HandleRDAccountRes(int iUserID, void *pMsgData); //处理计分响应
	void HandleRDUserPropsRes(void* pMsgData);
	//用户道具列表回应
	void HandleTestNetMsg(int iUserID,void *pMsgData);
	//获得道具信息后转发给客户端
	virtual void SendPropInfoToClient(FactoryPlayerNodeDef* nodePlayers);

	virtual void HandleAuthenReq(int iSocketIndex, void *pMsgData);        //玩家登陆验证请求
	virtual void HandleSitDownReq(int iUserID,void *pMsgData);		//玩家坐下请求
	virtual void HandleNormalEscapeReq(int iUserID, void *pMsgData);   //玩家正常逃跑
	virtual void HandleNormalEscapeReq(FactoryPlayerNodeDef *nodePlayers, void *pMsgData);	
	virtual void HandleErrorEscapeReq(int iUserID, void *pMsgData);		//玩家强制逃跑		
	virtual void HandleRDUserInfoRes(int iUserID, char *pMsgData);	//radius回应用户信息
	int GetDayTimeFlag(const time_t & theTime);
	int GetDayGap(int iDateFlag1, int iDateFlag2);
	time_t GetPointDayStartTimestamp(int iDateFlag);
	virtual bool HandleReadyReq(int iUserID, void *pMsgData);        //玩家按下“开始”
	virtual void SendGameLogInfo(FactoryTableItemDef *pTableItem, GameLogOnePlayerLogDef* pGameLogInfo = NULL, int iNum = 0);	//局结束的时候发送游戏记录,只需要在句结束的时候调用.															  
	virtual void SetAIBaseStat(FactoryPlayerNodeDef * nodePlayers);
	virtual void SetUserBaseStat(FactoryPlayerNodeDef * nodePlayers);
	virtual void SendGameOneLogInfo(FactoryTableItemDef *pTableItem, FactoryPlayerNodeDef *nodePlayers, GameLogOnePlayerLogDef* pGameLogInfo = NULL);	//玩家个人结算后时候发送游戏记录，结算后可能离桌，这里调用															  
	void SendGameReplayData(FactoryTableItemDef* pTableItem);//局结束的时候发送游戏重放记录	(目前仅判断发送牌局编号）
	void SendGetLogLevelSyncMsg(int iSeverType, int iServerId);

	void C36STRAddOne(char* pStr, int iLength);//36进制的字符串累加
	virtual void HandleUrgeCard(int iUserID,void *pMsgData);//处理催牌消息

	void HandleLeaveReq(int iUserID,void *pMsgData);//处理主动离开游戏
		
	void SendGameRefreshUserInfoReq(FactoryPlayerNodeDef *nodePlayers,bool bNeedConvertCharge, bool bNeedAccount = true);//发送游戏中刷新用户信息请求
	void HandleGameRefreshUserInfoRes(void *pMsgData);//游戏中刷新用户信息
	virtual void CallBackGameRefreshUserInfo(FactoryPlayerNodeDef *nodePlayers,int iConvertCharge){};//iType为0表示成功 非0值表示错误类型

	void FillRDAccountReq(GameUserAccountReqDef* pAccountMsg, GameUserAccountGameDef* pGameAccountMsg, FactoryPlayerNodeDef* nodePlayers, int iIfQuit);
	
	//cKickType若不是g_iTokenError等底层定义好的编号，g_iTokenError用g_iGameLogicKick，cKickSubType用于区分游戏自己的踢出类型
	void SendKickOutMsg(FactoryPlayerNodeDef *nodePlayers, char cKickType, char cClueType, char cKickSubType);//发送踢人消息, cClueType只是提示，1是离开游戏，2退出游戏返回大厅界面
	void SendKickOutWithMsg(FactoryPlayerNodeDef* nodePlayers, char* cKickClueMsg, char cKickSubType, char cClueType);//游戏逻辑踢出，带踢出内容
	
	void UpdateCServerPlayerNum(int iNum, bool bForceSend = false);//iNum = 1表示增加一个人，iNum = -1表示减少一个人
	void UpdateRoomNum(int iNum,int iRoomIndex,char cLoginType);//iNum = 1表示增加一个人，iNum = -1表示减少一个人
	
	
	virtual bool JudageRoomCloseKickPlayer(FactoryPlayerNodeDef *nodePlayers);
	time_t m_tmLastCheckKick;		
	
	int SetAgainPlayerInfo(char *pDate);//设置掉线重入时玩家信息，返回值是填充信息的长度
	
	virtual bool JudgeMidwayLeave(int iUserID, void *pMsgData){return false;}//判断是否游戏允许中途离开
	virtual bool CallBackSpecialSitDown(FactoryPlayerNodeDef *nodePlayers) { return false; }  //是否有自己的单独入座逻辑
	virtual bool CheckIfPlayWithControlAI(FactoryPlayerNodeDef *nodePlayers){return false; }//是否本局要和控制ai对局
	virtual void CallBackUserLeaveTable(FactoryTableItemDef *pTableItem,FactoryPlayerNodeDef *nodePlayers, int iTableNum,int iTableNumExtra,bool bDis, char cLeaveType = 0){}//离开桌子的重载函数
		
	void SetCallBackReadyBaseInfo(FactoryTableItemDef *pTableItem,FactoryPlayerNodeDef *nodePlayers);//游戏调用基类初始化桌基准信息,必须调用
	void SetAgainLoginBaseNew(AgainLoginResBaseDef *pMsg,FactoryTableItemDef *pTableItem,FactoryPlayerNodeDef *nodePlayers,int iSocketIndex);
	/*结算调用 iGameAmount（正/负，只表示游戏本身输赢(不包括桌费和中途已经计费过的,也不包括后面额外发放和额外回收的))
	iExtraAddAmount表示其他尚未计费的加游戏币(如被其他玩家替交的罚分)
	iExtraDecAmount表示其他尚未计费的扣游戏币(如罚分，用正值表示要扣的)*/
	int SetAccountReqRadiusDef(GameUserAccountReqDef*pMsg, GameUserAccountGameDef* pGameAccount,FactoryTableItemDef *pTableItem,FactoryPlayerNodeDef *nodePlayers,long long llGameAmount,long long llExtraAddAmount,long long llExtraDecAmount,int iAddDayCnt = 1);
	 
	void SetUserProp(FactoryPlayerNodeDef* nodePlayers, GameUserOnePropDef& stOneProp);
	void AddUserProp(FactoryPlayerNodeDef* nodePlayers,int iPropID,int iPropNum,int iLastTime, int iGameID=99);
	GameUserOnePropDef* GetUserPropInfo(FactoryPlayerNodeDef* nodePlayers, int iPropID);
	int GetPropDiamondValue(int iPropID);//返回-1表示没有找到配置值
	map<int, int>m_mapPropDiamodValue;//道具的钻石价值

	/*游戏过程中对游戏币/奖券/钻石进行计费,注意以下特殊计费是调用后再增加用户节点内的，因为日志要记录开始值,iMoneyType=1福券 2游戏币 3钻石 4奖券,iLogType和iLogExtraInfo用于日志记录*/
	void SendSpecialAcount(FactoryPlayerNodeDef* nodePlayers,int iMoneyType,int iAddNum,int iLogType, int	iLogExtraInfo = 0);

	void AddExtraMoneyDetail(FactoryPlayerNodeDef * nodePlayers, int iType, int iAddNum);
	
	void SetUserNameHide(FactoryPlayerNodeDef *nodePlayers,char *szUserName);//隐藏玩家名称
	
	virtual int JudgeKickOutServer(FactoryPlayerNodeDef *nodePlayers,bool bSendKickMsg,bool bCheckServerSate, bool bCheckMaxLmt);//判断服务踢人函数
	virtual int CheckSpecialLimit(FactoryPlayerNodeDef *nodePlayers) { return 0; }  //部分游戏需要自己的检测逻辑，重写此函数即可
	
	//长时间没有操作剔除玩家
	bool m_bIfStartKickOutTime;//是否开启剔除玩家操作（默认关闭，由子类设定）
	virtual void JudgePlayerKickOutTime(int iTime){};//判断玩家长时间没有操作剔除	
	virtual void CallBackGameNetDelay(int iMsgType,int iStatusCode,int iUserID, void *pMsgData,int iDelayUs){}
	bool m_bCheckTableRecMsg;//是否每5分钟检测一次桌子上玩家最近一次接受信息时间（桌上有人，且超过10分钟没有任何信息，该桌子可能卡了，解散吧），默认开启
	virtual void CheckTableRecMsgOutTime(time_t tmNow);//子类有需要可以继承重写

	virtual void CallBackGetRoomInfo(){}//获得房间信息后，子类做回调处理
	virtual void CallBackGetUserInfoExtra(FactoryPlayerNodeDef *nodePlayers){}//获得玩家额外信息回调函数
	virtual void CallBackGetPropsSuccess(FactoryPlayerNodeDef* nodePlayers) {};//获取道具成功后回调函数	
	
	void SetClientSockTableID(int iSocketIndex, int iTableID);//设置主线程指定Socket的桌(组)节点编号
	void SetClientALLSockTableID(int iSocketIndex, int iTableID);//设置群发线程指定Socket的桌(组)节点编号
	void SendMsgToClientTable(void *pMsg, int iLen, unsigned short iMsgType, int iTableID);//iTableID为桌或者组编号
	void SendMsgToAllClientTable(void *pMsg, int iLen, unsigned short iMsgType, int iTableID);//iTableID为桌或者组编号

	void SendGetServerConfParams(vector<string>&vcKey);//vcKey不要超过50
	void HandleGetParamsRes(void *pMsgData);
	void HandleClientUseInteractPropReq(void *pMsgData);
	void SendPropLog(int iUserID,char* szUserName,int iPropID,int iAddNum,int iBeginNum,int iLogType,int iLogExtraInfo = 0);//发送道具流水日志
	void SendMainMonetaryLog(int iUserID, char* szUserName, int iMoneyType, long long llAddNum, long long llBeginNum, int iLogType, int iLogExtraInfo = 0);
	void SendGetRoomInfo(bool bOnlyUpdateOnline);//向room服务器获取房间信息&更新在线人数
	
	void GetUserNickName(char* szNickName, FactoryPlayerNodeDef *nodePlayers);

	virtual void CallBackBeforeSendTableUsersInfo(int iIndex){};
	virtual void CallBackSetAIInfo(FactoryPlayerNodeDef *nodePlayers, FactoryPlayerNodeDef *nodeAI, int iType) {};//初始化AI的游戏相关字段
	virtual void CallBackSerUserRedisLoginFlag(FactoryPlayerNodeDef* nodePlayers,int& iFlag) {};//子类有需求可以重新设定通过redis获取用户信息的flag请求标记
	void SendGameBaseStatInfo(int iTmFlag);  //游戏基础统计信息上报
	void AddComEventData(int iEventID, int iSubID, long long iAddNum = 1);
	virtual void AddComEventDataWithValid(int iUserID,int iEventID, int iSubID, int iAddNum = 1) {};
	GameEvent* m_pGameEvent;                //上报redis事件统计管理器
	TaskLogic* m_pTaskLogic;

	//每半小时以及跨天会发送一次游戏基础统计信息
	RdGameBaseStatInfoDef m_stGameBaseStatInfo;//游戏基础统计信息

	//处理redis消息
	void HandleRedisGetUserDecorateInfo(void* pMsgData);
	//处理room服务器消息
	void HandleGetAllRoomInfo(void* pMsgData);
	void HandleLogLevelSyncMsg(void* pMsgData, int iMsgLen);
	void SendServerInfoToCenter();//游戏服务器信息发生变化或重连中心服务器后给中心服务器重发本服信息

	//各个游戏自己的特殊任务处理 iAddTaskComplete增量 iHaveComplete累计量
	virtual bool CallBackJudgeTask(FactoryPlayerNodeDef* pPlayerNode, int iTaskSubType, int &iAddTaskComplete, int &iHaveComplete) { return false; }
	//向radius服务器发送玩家认证请求
	void AuthByRadiusMsg(FactoryPlayerNodeDef *nodePlayers);

	//通知redis记录近期同游玩家
	void SendRedisTogetherUser(FactoryTableItemDef * pTableItem);

	map<int, string>m_mapVNick;
	vector<string>m_vcAllNick;
	virtual void SetAIBaseUserInfo(FactoryPlayerNodeDef * nodePlayers);
	virtual void SetAIUserGameInfo(FactoryPlayerNodeDef * nodePlayers,int index, void *pMsgRes,int iMsgLen) {};  //有个游戏自己ai的战绩等游戏信息
	virtual void CallBackSetRobotNick(char* szNick, int &iGender, int iUserID = -1);//机器人node创建后昵称赋值
	virtual void GetPointedTxtContent(char* szFileName, map<int, string>& mapTxtContent);
	virtual void CallBackAfterCenterReconnect() {};
	virtual void CallBackGrowupEventRes(int iUserID, int iEventID, int iGrowNumInfo, char* szGrowStrInfo) {};
	virtual void UpdateGrowupEventInfo(int iUserID, int iEventID, int iGrowNumInfo, char * szGrowStrInfo, int iLen);
	void GetUserRoomTask(FactoryPlayerNodeDef * nodePlayers, int iAddNum = 0);  //iAddNum=0获取， >0 更新
	virtual void HandleUserRoomTaskRes(void * pMsgData);
	virtual void HandleUserRecentIntegralTask(void * pMsgData);
	virtual void HandleUserDayInfo(void * pMsgData);
	virtual int CallBackUserMatchInfo(FactoryPlayerNodeDef * nodePlayers) { return true; }
	virtual void CallBackUserRoomTaskInfo(FactoryPlayerNodeDef * nodePlayers) {};
	virtual void HandleGrowupEventInfo(void * pMsgData);
	virtual void CallBackFriendRoomAgainLogin(FactoryPlayerNodeDef * nodePlayers) {};
	void RefreshUserGrowUpEvent(FactoryPlayerNodeDef * pPlayer, int iGrowEvent, int iGrowNum, char * szGrowStr, int iLen = -1);
	void HandleFriendListRes(void * pMsgData);

	virtual bool CallBackNeedRateTable() { return false; }   //是否需要获取奖券任务概率表
	virtual bool CallBackNeedTaskInfo() { return false; }   //是否需要获取奖券任务列表
	virtual void CallBackDayGapAfterRdAccount(FactoryPlayerNodeDef * nodePlayers) {};//计费后判断是跨天，游戏可继承重置游戏的每日数据等信息
	virtual void CallBackFillRDAccountReq(GameUserAccountReqDef* pAccountMsg, GameUserAccountGameDef* pGameAccountMsg, FactoryPlayerNodeDef* nodePlayers, int iIfQuit) {};//子类调用，避免部分延迟计费的游戏内，离开时候buff值被重置
   //控制ai专用ID
	vector<int>m_vcControlAIID;
	FactoryPlayerNodeDef* GetControlAINode(FactoryPlayerNodeDef *nodePlayer);
	int GetVirtualAchivel(int iAllCnt);
	void ReleaseControlAINode(FactoryPlayerNodeDef *nodePlayer);
	void HandleAchieveTaskComp(void * pMsgData);
	void SendGrowupEventInfoReq(int iUserID, int iGrowEventId);
};
#endif //__GAMELOGIC_H__