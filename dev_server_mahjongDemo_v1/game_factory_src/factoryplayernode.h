#ifndef __FACTORY_PLAYER_NODE__
#define __FACTORY_PLAYER_NODE__

#include <time.h>
#include <vector>
#include "factorymessage.h"
#include <cstring>
using namespace std;

typedef struct PropInfo		//道具信息
{
	int iPropID;
	int iPropNum;
	int iLastTime;
	int iPropUsed;//道具可使用状态
}PropInfoDef;

typedef struct OtherMoney   //混服中自己视角其他玩家金币数量
{
	int iUserID;
	long long llMoney;
}OtherMoneyDef;

typedef struct RoomTaskInfo
{
	int iRoomType;
	int iCompNum;
	int iNeedNum;
	int iAwardID;
	int iAwardNum;
}RoomTaskInfoDef;

//用户断开类型
enum class UserDisconnectType {
	LEAVE_BY_SELF= 1, /* 主动离开 */
	KICK_BY_SYS = 2, /* 2被系统踢出去 */
	NET_DIS = 3 /*3，网络异常*/
};

//FactoryPlayerNodeDef类中有容器，不可以直接memset!!!
class FactoryPlayerNodeDef
{
public:
	FactoryPlayerNodeDef()
	{
		
	}
	 ~FactoryPlayerNodeDef()
	{
		
	}
	virtual FactoryPlayerNodeDef* operator->() 
	{
		return this;
	}
	//游戏内对用户节点绝对不可以memset，必须调用Reset()
	virtual void Reset()
	{
		iUserID = 0;
		memset(szUserName, 0, sizeof(szUserName));
		memset(szUserToken, 0, sizeof(szUserToken));
		iSocketIndex = 0;
		iAllSocketIndex = 0;
		iStatus = 0;
		cRoomNum = 0;
		memset(szNickName, 0, sizeof(szNickName));
		cVipType = 0;	
		iNowVerType = 0;
		iMoney = 0;
		iIntegral = 0;
		iSpreadID = 0;
		iEnterRoomType = 0;
		memset(szIP, 0, sizeof(szIP));
		memset(szMac, 0, sizeof(szMac));
		iAgentID = 0;
		iSpeMark = 0;
		iHeadImg = 0;
		iGender = 0;
		iDiamond = 0;
		iAllGameTime = 0;

		tmRegisterTime = 0;
		iRegType = 0;
		iExp = 0;
		iTotalCharge = 0;

		cLoginFlag1 = 0;
		cLoginFlag2 = 0;
		iPlayType = 0;

		iDisNum = 0;
		iLeaveGameTime = 0;

		tmLastAccount = 0;
		tmTestLastRecMsg = 0;

		bWaitExit = false;
		bIfAINode = false;
		cLoginType = 0;
		bTempMidway = false;
		vector<GameUserOnePropDef>().swap(vcUserProp);
		memset(cLastGameNum, 0, sizeof(cLastGameNum));
		cErrorType = 0;

		iGameTime = 0;
		iWinNum = 0;
		iLoseNum = 0;
		iAllNum = 0;
		iWinMoney = 0;
		iLoseMoney = 0;
		iGetIntegral = 0;
		iFirstGameTime = 0;
		iLastGameTime = 0;
		iGameExp = 0;
		iBuffA0 = 0;
		iBuffA1 = 0;
		iBuffA2 = 0;
		iBuffA3 = 0;
		iBuffA4 = 0;
		iBuffB0 = 0;
		iBuffB1 = 0;
		iBuffB2 = 0;
		iBuffB3 = 0;
		iBuffB4 = 0;
		iContinueWin = 0;
		iContinueLose = 0;
		iDayWinNum = 0;
		iDayLoseNum = 0;
		iDayAllNum = 0;
		iDayWinMoney = 0;
		iDayLoseMoney = 0;
		iDayIntegral = 0;
		iRecentPlayCnt = 0;
		iRecentWinCnt = 0;
		iRecentLoseCnt = 0;
		iRecentWinMoney = 0;
		iRecentLoseMoney = 0;

		iRDMoney = 0;
		iRDDiamond = 0;
		iRDIntegral = 0;
		iRDGameTime = 0;
		iRDExp = 0;
		iIfHaveGamAccount = 0;
		memset(&stRDGameAccount, 0, sizeof(GameUserAccountGameDef));
												
		llMemMoney = 0;
		iMemDiamond = 0;
		iMemIntegral = 0;
		iMemGameTime = 0;
		iMemExp = 0;
		iMemGameAccountCnt = 0;
		memset(&stMemGameAccount, 0, sizeof(GameUserAccountGameDef));
												 
		iBeginMoney = 0;		
		iBeginIntegral = 0;
		iBeginDiamond = 0;
		iBeginExp = 0;
		iBeginGameTime = 0;	

		iExtraAddMoney = 0;     
		iExtraLoseMoney = 0;     
		cExtraMoneyDetail[160] = 0; 
		memset(cExtraMoneyDetail, 0, sizeof(cExtraMoneyDetail));

		vector<RdUserTaskInfoDef>().swap(vcUserDayWeekTask);
		vector<RdComOneTaskInfoDef>().swap(vcUserComTask);
												  
		iLatestPlayerIndex = 0;
		memset(iLatestPlayer, 0, sizeof(iLatestPlayer));
		bGapDaySitReq = false;
		iNCAGameRate = 0;
		iHeadFrameID = 0;
		iHeadFrameTm = 0;
		iClockPropID = 0;
		iClockPropTm = 0;
		iChatBubbleID = 0;
		iChatBubbleTm = 0;

		iGetRdTaskInfoRt = 0;
		iGetRdLatestPlayerRt = 0;
		iGetRdDecorateRt = 0;
		iGetRdComTaskRt = 0;

		memset(szHeadUrlThird, 0, sizeof(szHeadUrlThird));
		iAchieveLevel = 0;
		iHisIntegral = 0;
		iVTableID = 0;
		memset(otherMoney, 0, sizeof(otherMoney));
		//memset(iFriends, 0, sizeof(iFriends));
		memset(&roomTask, 0, sizeof(roomTask));
		bGetLoginOK = false;
		ClearGameStart();
		bIfWaitLoginTime = false;
		ClearGameEnd();
		iDayTaskIntegral = 0;
		memset(iRecentTask, 0, sizeof(iRecentTask));
		iLastCheckGameTm = 0;
	}

	//一局结束后需要清理的数据
	void ClearGameEnd()
	{
		cTableNum = 0;
		cTableNumExtra = 0;
		bAIContrl = false;
		iExtraLoseMoney = 0;
		iExtraAddMoney = 0;
	}

	//一局开始前需要清理的数据
	void ClearGameStart()
	{
		bIfDis = false;
		iGameLeaveSec = 0;
		iWaitLoginTime = 0;
		iGameResultForTask = 0;
		iGameMagicCnt = 0;
		llGameRtAmount = 0;
		iGameGetIntegral = 0;
		iIntoGameTime = 0;
		cDisconnectType = 0;
		if (iSocketIndex != -1)
		{
			bIfWaitLoginTime = false;
		}
		memset(cExtraMoneyDetail, 0, sizeof(cExtraMoneyDetail));
	}

	FactoryPlayerNodeDef *pNext;	//指向前一节点
	FactoryPlayerNodeDef *pPrev;	//指向后一节点
	int  iUserID;           //用户ID
	char szUserName[20];	//用户帐号
	char szUserToken[32];	//用户TOKEN
	int  iSocketIndex;      //SocketIndex
	int  iAllSocketIndex;   //SocketIndex（群发线程）
	int  iStatus;           //状态
	char cRoomNum;			//所在的房间,从1开始算，对应ServerBaseInfo中stRoom
	int  cTableNum;         //所在的桌，从1开始算，对应每个游戏的m_tbItem
	char cTableNumExtra;    //所在桌的位置，0，1，2,4
	char szNickName[64];    //玩家昵称	
	char cVipType;			//VIP类型	
	int iNowVerType;		//手机游戏功能版本号 低16位大厅版本 高16位游戏版本
	int iFirstMoney;//一级货币福券
	long long iMoney;		//金钱
	int iIntegral;			//奖券
	int  iSpreadID;			//推广员id
	int  iEnterRoomType;	//进入的房间类型
	char szIP[20];			//玩家IP
	char szMac[40];			//mac地址
	int iAgentID;			//渠道ID
	int iSpeMark;			//账号特殊标记
	int	iHeadImg;			//头像ID
	int	iGender;			//性别0未知1女2男
	int	iDiamond;			//钻石
	int	iAllGameTime;		//总在线时长

	time_t tmRegisterTime;	//注册时间
	int iRegType;			//注册类型
	int	iExp;				//平台经验值
	int	iTotalCharge;		//总充值金额

	char cLoginFlag1;		//客户端登录携带标记1
	char cLoginFlag2;		//客户端登录携带标记2  //低位起第一位所有游戏通用(是否返回前台的认证)，不可占用
	int  iPlayType;			//游戏玩法（登录时传递，每个游戏根据需要通过位移运算自行设定）

	int	iWaitLoginTime;		//掉线续玩的等待时间
	bool bIfWaitLoginTime;		//是否在等待重入状态
	bool bWaitExit;         //是否发了逃跑在等待退出游戏
	int	  iDisNum;//游戏过程中掉线次数
	time_t	iLeaveGameTime;//掉线时候的时间
	char cDisconnectType;//掉线类型0没有掉线，1在游戏中主动离开，2被系统踢出去, 3，网络异常（对应UserDisconnectType内枚举）
	int iGameLeaveSec;//本局游戏总掉线累积秒数

	time_t tmLastAccount;//上次发送计费的时间	
	time_t tmTestLastRecMsg;

	bool bIfAINode;		//是否是AI节点
	bool bAIContrl;     //是否AI控分状态
	bool bIfDis;		//记录在一局游戏中是否掉线
	char cLoginType;	//平台类型0android,1iphone.2ipad,3pc
	
	bool bTempMidway;	//是否是中途离开的临时玩家节点

	vector<GameUserOnePropDef> vcUserProp;//玩家道具	
		
	char cLastGameNum[20];//记录上次游戏的牌局编号
	
	time_t iIntoGameTime;//玩家进入游戏时间，用于防作弊玩家同时进入游戏分配到同桌
	
	char cErrorType;//异常标记,需要被踢出,对应g_iTokenError等错误类型

	//对应游戏信息
	int	iGameTime;//本游戏时间
	int	iWinNum;//总赢局数
	int	iLoseNum;//总输局数
	int	iAllNum;//总局数
	long long iWinMoney;//总赢游戏币
	long long iLoseMoney;//总输游戏币
	int	iGetIntegral;//游戏内总获得奖券数
	int	iFirstGameTime;//首次游戏时间
	int	iLastGameTime;//上次游戏时间
	int	iGameExp;//游戏经验值
	int	iBuffA0;//游戏控制值A0（赋值型）
	int	iBuffA1;//游戏控制值A1（赋值型）
	int	iBuffA2;//游戏控制值A2（赋值型）
	int	iBuffA3;//游戏控制值A3（赋值型）
	int	iBuffA4;//游戏控制值A4（赋值型）
	int	iBuffB0;//游戏控制值B0（增加型）
	int	iBuffB1;//游戏控制值B1（增加型）
	int	iBuffB2;//游戏控制值B2（增加型）
	int	iBuffB3;//游戏控制值B3（增加型）
	int	iBuffB4;//游戏控制值B4（增加型）
	int	iContinueWin;//连赢次数
	int	iContinueLose;//连输次数
	int	iDayWinNum;//今日总赢局数
	int	iDayLoseNum;//今日总输局数
	int	iDayAllNum;//当日总游戏局数
	long long iDayWinMoney;//今日游戏总赢游戏币
	long long iDayLoseMoney;//今日游戏总输游戏币
	int	iDayIntegral;//今日在该游戏获得奖券数目
	int	iRecentPlayCnt;//近期总玩次数
	int	iRecentWinCnt;//近期赢次数
	int	iRecentLoseCnt;//近期输次数
	long long iRecentWinMoney;//近期游戏赢分(不包括桌费)
	long long iRecentLoseMoney;//近期游戏输分(不包括桌费)
	
	//计费部分begin
	
	//单局需要计费部分begin
	long long	iRDMoney;//需要计费的游戏币
	int	iRDDiamond;//需要计费的星钻
	int	iRDIntegral;//需要计费的奖券
	int	iRDGameTime;//游戏时长(秒)
	int	iRDExp;
	int iIfHaveGamAccount;
	GameUserAccountGameDef stRDGameAccount; //游戏待计费部分(若有需将iIfHaveGamAccount标记为1)
	//单局需要计费部分end

	//累积缓存需要计费部分begin
	long long llMemMoney;//需要计费的游戏币
	int	iMemDiamond;//需要计费的星钻
	int	iMemIntegral;//需要计费的奖券
	int	iMemGameTime;//游戏时长(秒)
	int	iMemExp;
	int iMemGameAccountCnt;//游戏部分待计费次数
	GameUserAccountGameDef stMemGameAccount; //游戏待计费部分
	//累积缓存需要计费部分end
	//计费部分end
	
	//开局信息begin
	long long iBeginMoney;		//局开始的时候的钱，为记录用
	int iBeginIntegral;
	int	iBeginDiamond;
	int	iBeginExp;
	time_t iBeginGameTime;	//单局游戏的开始时间，与发送RDACCOUNT的时间对比下就是游戏时间了
	//开局信息end	

	long long iExtraAddMoney;     //游戏途中非游戏结算逻辑获取的Money，记录用
	long long iExtraLoseMoney;     //游戏途中非游戏结算逻辑消耗的Money，记录用
	char cExtraMoneyDetail[160];    //额外计费逻辑中消耗类型，记录用

	int iGameResultForTask;//当局游戏结果低4位（0初始化，1赢，2输，3平局），再4位本局名次
	long long llGameRtAmount;//本局游戏输赢信息
	int iGameGetIntegral;//游戏获得多少奖券
	int iGameMagicCnt;   //本局游戏使用魔法表情数量

	vector<RdUserTaskInfoDef> vcUserDayWeekTask;//用户的日常&周长任务
	vector<RdComOneTaskInfoDef> vcUserComTask;//活动通用任务信息

	//配桌相关
	int iLatestPlayerIndex;//近期同桌玩家索引用
	int iLatestPlayer[10];//近期同桌玩家
	bool bGapDaySitReq;//是否是今日首次入座
	int iNCAGameRate;//新配桌机制中游戏几率
	//配桌相关_end

	/*****道具装扮信息****/
	int iHeadFrameID;//头像框道具id(0没有)
	int iHeadFrameTm;//头像框道具有效期(每局结算后底层会判断一次是否失效)
	int iClockPropID;//闹钟装扮id(0没有)
	int iClockPropTm;//闹钟道具有效期(每局结算后底层会判断一次是否失效)
	int iChatBubbleID;//聊天气泡道具id
	int iChatBubbleTm;//聊天气泡道具失效期(每局结算后底层会判断一次是否失效)
	/*****道具装扮信息end****/

	int iGetRdTaskInfoRt;//登录后通过redis获取任务信息结果(0初始化，1获取成功，2等待获取中/无需获取)
	int iGetRdLatestPlayerRt;//登录通过redis获取近期同桌玩家结果(0初始化，1获取成功，2等待获取中/无需获取)
	int iGetRdDecorateRt;//登录通过redis获取装扮结果(0初始化，1获取成功，2等待获取中/无需获取)
	int iGetRdComTaskRt;//登录通过redis活动通用结果(0初始化，1获取成功，2等待获取中/无需获取)

	//第三方昵称及头像
	char szHeadUrlThird[512];

	int iAchieveLevel;//成就等级
	int iHisIntegral; //历史奖券数量

	int iVTableID;//配桌时所在虚拟桌号(负值表示需要在入座请求内发送至中心服务器)

	OtherMoneyDef otherMoney[10];      //玩家自己视角看到同桌玩家的钱
	//int iFriends[200];   //好友列表,暂时屏蔽
	RoomTaskInfoDef roomTask;                          //玩家已触发的房间任务
	int iRecentTask[3];                       //最近任务记录

	bool bGetLoginOK;
	int iDayTaskIntegral;       //今日游戏内任务获取奖券数量

	int iLastCheckGameTm;
};

#endif //__FACTORY_PLAYER_NODE__