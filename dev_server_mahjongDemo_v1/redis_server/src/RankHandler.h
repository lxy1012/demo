#ifndef __RANK_HANDLER_H__
#define __RANK_HANDLER_H__

#include "MsgHandleBase.h"
#include "msg.h"
#include <string>
#include <vector>
#include <map>
#include "msgqueue.h"	//消息队列
using namespace  std;

#define RANK_WORD_ACTIVITY_ID	1	//世界排行榜活动id


//用户个人排行信息
const string KEY_USER_RANK_INFO = "MJ_USER_RANK_INFO:%d_%d"; //rankid_periods

//增减排行实时榜
const string KEY_USER_RANK_LIST1 = "MJ_USER_RANK_LIST1:%d_%d"; //rankid_periods

//累计排行实时榜
const string KEY_USER_RANK_LIST2 = "MJ_USER_RANK_LIST2:%d_%d"; //rankid_periods

//最低排行信息（累计排行模式）
const string KEY_USER_RANK_MIN_INFO = "MJ_USER_RANK_MIN_INFO:%d_%d";	//rankid_periods

//等待排行信息（累计排行模式）
const string KEY_USER_WAIT_RANK = "MJ_USER_WAIT_RANK:%d_%d";	//rankid_periods

typedef struct PropInfo
{
	int iPropId;
	int iPropNum;
}PropInfoDef;

typedef struct DKRankAward
{
	int iMinRankNum;	//最小排名
	int iMaxRankNum;	//最大排名
	vector<PropInfoDef> vcAwardInfo;
}DKRankAwardDef;

typedef struct DKRankConfig
{
	int iRankId;	//rankid 1世界排行榜
	int iRankMode;	//排行模式 0增减排行模式 1累计排行模式
	int iTimeType;	//排行结算类型	0按配置时间结算1次 1每周结算1次 2每月结算1次
	int iBeginTime;
	int iEndTime;
	int iMaxRankNum;
	char szJifenConf[100];
	char szBuffExtra[100];
	vector<DKRankAward> allRankAward;
	void clear()
	{
		iRankId = 0;
		iRankMode = 0;
		iTimeType = 0;
		iBeginTime = 0;
		iEndTime = 0;
		iMaxRankNum = 0;
		memset(szJifenConf, 0, sizeof(szJifenConf));
		memset(szBuffExtra, 0, sizeof(szBuffExtra));
		vector<DKRankAward>().swap(allRankAward);
	}
}DKRankConfigDef;
  
typedef struct DKRankHistory
{
	int iUserID;
	int iRankId;
	int iPeriods;
	int iRankNum;
	int iJifenNum;
	string strAwardInfo1;
	string strAwardInfo2;
	void clear()
	{
		iUserID = 0;
		iRankId = 0;
		iPeriods = 0;
		iRankNum = 0;
		iJifenNum = 0;
		strAwardInfo1 = "";
		strAwardInfo2 = "";
	}
}DKRankHistoryDef;

typedef struct DKWaitEndRankInfo
{
	int iRankId;
	int iRankMode;
	int iPeriods;
	int iMaxPlayer;
	int iEndTime;
	vector<DKRankAward> allRankAward;
	void clear()
	{
		iRankId = 0;
		iRankMode = 0;
		iPeriods = 0;
		iMaxPlayer = 0;
		iEndTime = 0;
		vector<DKRankAward>().swap(allRankAward);
	}
}DKWaitEndRankInfoDef;


typedef struct RankUserInfo
{
	int iRankID;	//排行id
	int iPeriodID;	//期数id
	int iUserID;	//userid
	int iRankNum;	//排名
	int iJifenNum;	//积分
	int iHeadImg;		//头像
	int iHeadFrameId;	//头像框id
	int iLastTime;		//时间
	string strExtra;	//额外信息
	string strNick;		//昵称
	void clear()
	{
		iRankID = 0;
		iPeriodID = 0;
		iUserID = 0;
		iRankNum = 0;
		iJifenNum = 0;
		iHeadImg = 0;
		iHeadFrameId = 0;
		iLastTime = 0;
		strExtra = "";
		strNick = "";
	}
}RankUserInfoDef;

typedef struct MinUserInfo
{
	int iJifenNum;	//积分
	string strExtra;	//额外信息
	void clear()
	{
		iJifenNum = 0;
		strExtra = "";
	}
}MinUserInfoDef;

typedef struct ParamsConf
{
	char cKey[32];
	char cValue[128];
	int iLastTime;
}ParamsConfDef;


enum emGameID
{
	game11 = 11,
	game12 = 12,
	game13 = 13,
};


class RankHandler : public MsgHandleBase
{
public:
	RankHandler(MsgQueue *pSendQueue, MsgQueue *pLogicQueue, MsgQueue* pEventQueue);

	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen,int iSocketIndex);
	virtual vector<int> GetHandleMsgTypes(){};

	virtual void CallBackOnTimer(int iTimeNow);
	virtual void CallBackNextDay(int iTimeBeforeFlag, int iTimeNowFlag);

	enum RankMode
	{
		INC_DEC = 0,    //增减模式
		ONLY_ADD = 1,	//累计模式
	};

	enum TimeType
	{
		CONF_TIME = 0,		//按配置时间结算
		WEEK_TIME = 1,		//周结算1次
		MOUTH_TIME = 2,		//月结算1次
	};

protected:
	void HandleGetRankConfInfoMsg(char* msgData, int iLen, int iSocketIndex);
	void HandleGetUserRankInfoMsg(char* msgData, int iLen, int iSocketIndex);
	void HandleUpdateUserRankInfoMsg(char* msgData, int iLen, int iSocketIndex);
	
	void AddComEvent(int iGameID, int iMainID, int iSubID, int iAddNum);
	
	void OnTimerGetRankConfInfo();  //定时刷新排行配置
	void OnTimerCheckWaitEndInfo();
	void OnTimerCheckRankIfEnd();
	void OnTimerWriteRKHistoryData();
	void OnTimerHandleWaitRankUser();
	void OnTimerGetParamsKeyValue();

	void HandleWaitRankUser(int iRankID, int iPeriodID);
	void HandleGetParamsMsg(char* msgData, int iSocketIndex);
	void RecordUserRankHistoryInfo(vector<DKRankHistoryDef> vcRKHis);

	DKRankConfigDef* findRankConfInfo(int iRankId);

	RankUserInfoDef GetRankUserInfo(int iUserID, int iRankID, int iPeriodID);
	void SetRankUserInfo(int iUserID, int iRankID, int iPeriodID, string strVal, int expire);
	MinUserInfoDef GetRankMinUserInfo(int iRankID, int iPeriodID);
	void SetRankMinUserInfo(int iRankID, int iPeriodID, string strVal, int expire);
	void addWaitRankUserInfo(int iRankID, int iPeriodID, int iUserID, string strVal, int expire);
	int GetWaitRankListNum(int iRankID, int iPeriodID);
	vector<RankUserInfoDef> GetWaitRankUserList(int iRankID, int iPeriodID, bool bDelete = true);
	vector<RankUserInfoDef> GetNowRankUserList(int iRankID, int iPeriodID);
	void SetNowRankUserList(int iRankID, int iPeriodID, vector<RankUserInfoDef> vcRankUser, int expire);
	int GetRankTestConf(int iRankID);
	int GetRankPeriodID(int iRankID, int tmNow);
	int GetLastWaitRankTime(int iRankID);
	void SetLastWaitRankTime(int iRankID, int tmNow);
	string findParamsValue(string key);

	map<int, DKRankConfigDef> m_mapRankInfo;	//排行配置信息
	map<int, DKWaitEndRankInfoDef> m_mapWaitEndRank;	//等待结算的排行
	vector<DKRankHistoryDef> m_vcWaitHis;	//等待插入的排行榜奖励资格

	map<int, int> m_mapWaitTime;	//上次处理wait排行时间
	map<string, string> m_mapParamsKeyVal;		//params配置
	map<string, ParamsConfDef> m_mapParmsConf;

	MsgQueue *m_pSendQueue;
	MsgQueue *m_pLogicQueue;
	MsgQueue *m_pEventQueue;
};

#endif // !__RedisThread_H__