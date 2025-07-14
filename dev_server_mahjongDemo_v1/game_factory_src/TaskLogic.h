#pragma once
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>      //sleep()
#include <stdlib.h>      //rand()
#include <vector>
#include <map>
#include "hashtable.h"
#include "factoryplayernode.h"
#include "factorytableitem.h"
using namespace std;

//用户日常&周长任务类型
enum class DayWeekTaskType
{
	DAY_LOGIN = 1,//每日登陆
	COMPLETE_GAME = 2,//累计局数
	SHARE = 3,//分享
	CHARGE = 4,//充值
	CREATE_AND_PLAY = 5,//创建房间并开局
	PLAY_WITH_FRIEND = 6,//好友一起游戏

	WEEK_LOGIN = 7,//一周登陆指定天数
	HIT_BACK_IN_GAME = 8,//游戏中进行反击
	ACCUMLATE_FIRST = 9,//获胜次数（第一名次数）
	ACCUMLATE_CHARGE = 10,  //累充
	ADD_FRIEND = 11,//新增指定数量好友
	PRESENT_FRIEND = 12,//给好友赠送次数

	WIN_MONEY_INGMAE = 13,//游戏中赢得多少游戏币
	ROOMTYPE5_PLAY = 14, //初级房游戏局数
	ROOMTYPE6_PLAY = 15, //中级房游戏局数
	ROOMTYPE7_PLAY = 16, //高级房游戏局数
	ROOMTYPE8_PLAY = 17, //VIP房间游戏局数

	ALL_DEFENCE = 18,  //游戏内全防守次数
	CONTINUE_FIRST = 19,//连胜（连续获得第一名）
	MAGIC_CNT = 20, //使用魔法表情次数

	COMPLETE_ALL_WEEK = 21,//完成所有周任务
	GET_INTEGRAL_INGMAE = 22,//游戏中获得多少奖券
	GAME_TIME = 23,//游戏时间达到多少分钟

	CONTINUE_LOGIN = 24,//连续登录多少天
	SHARE_CNT = 25,//累计分享多少次
	ACCUMLATE_WIN = 26,  //累计获胜
	CONTINUE_WIN = 27,   //连续获胜
	BET_GAME_CNT = 28,  //历史累计下注局数
};

typedef struct IntegralTaskTable
{
	int iTableGrade;    //表等级
	int iMinIntegral;   //最低历史奖券
	int iMaxIntegral;   //最高历史奖券
	int iWeight[6];       //奖励权重
}IntegralTaskTableDef;

typedef struct IntegralTaskInfo
{
	int iTaskID;        //任务编号
	int iPlayType;          //玩法屏蔽 第1位屏蔽不可转移玩法
	int iTriggerWeight;    //触发权重
	int iAwardType;        //奖励类型
	int iAwardNum[6];        //奖励数量
	int iTaskType;         //任务类型
	int iTaskValue;        //任务参数
	string strTaskNameCH;    //任务名称（中文）
	string strTaskNameRU;    //任务名称（俄文）
}IntegralTaskInfoDef;

class GameLogic;
class TaskLogic
{
public:
	TaskLogic(GameLogic *pGameLogic);
	~TaskLogic();

	void CallBackOneSec(long iTime);

	void HandleGetRedisTaskMsg(void * pMsgData);

	void HandleGetUserRdComTask(void * pMsgData);

	void HandleIntegralTaskInfo(void * pMsgData);

	void JudgeRdComTask(FactoryTableItemDef* pTableItem);
	void JudgeRdComTask(FactoryTableItemDef* pTableItem, FactoryPlayerNodeDef *nodePlayers);

	void JudgeUserRedisTask(FactoryTableItemDef* pTableItem);
	void JudgeUserRedisTask(FactoryTableItemDef* pTableItem, FactoryPlayerNodeDef *nodePlayers);

	void CheckFriendship(FactoryTableItemDef* pTableItem, FactoryPlayerNodeDef *nodePlayers);

	void SetNeedRateTable(bool bNeed) { m_bNeedRateTable = bNeed; }
	void SetNeedIntegralTask(bool bNeed) { m_bNeedIntegralTask = bNeed; }

	map<int, IntegralTaskTableDef> m_mapIntegralTable;
	map<int, map<int, IntegralTaskInfoDef> > m_mapIntegralTask;
private:
	void ReadConf();

	GameLogic * m_pGameLogic;
	char m_cTaskBuff[2048];
	int m_iTimeReadConf;
	bool m_bNeedRateTable;
	bool m_bNeedIntegralTask;
	map<int, map<int, IntegralTaskInfoDef> > m_mapTmpTask;   //消息过长，缓存在这里，全部接收成功后再给m_mapIntegralTask
};