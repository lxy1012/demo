#ifndef __Task_Handler_H__
#define __Task_Handler_H__

#include "MsgHandleBase.h"
#include "msg.h"
#include "msgqueue.h"	//消息队列
#include <string>
#include <vector>
#include <map>
using namespace  std;

typedef struct BKTaskConf
{
	int iID;
	int iTaskID;
	int iMainType;//0日常任务 1周任务
	int iSubType;//DayWeekTaskType
	int iGameID[20];//最多预留20个游戏,99表示所有游戏
	int iRoomType[20];//指定游戏对应的指定房间类型（0表示所有房间）
	char szTaskName[60];
	int iTaskNeedComplete;
	int iTaskAwardProp1;
	int iTaskAwardNum1;
	int iTaskAwardProp2;
	int iTaskAwardNum2;
}BKTaskConfDef;
typedef vector<BKTaskConfDef> VecBKTaskConf;

typedef struct BKTaskLog
{
	int iTaskType;//0日常任务 1周任务 2成长任务, 1000,完成所有日任务，2000完成所有周任务
	int iID;
	int iCompTaskUser;//任务完成人数
}BKTaskLogDef;

typedef struct AchieveLevel
{
	int iLevel;				//奖励档位
	int iNeedNum;			//需要完成值
	int iAchievePoints;		//奖励成就点
};

typedef struct DKAchieveTask
{
	int iTaskID;
	int iTaskType;	//1连续登录 2累计分享 3获得第一名 4连续第一名 5加好友 6好友对局
	vector<int> vcGameId;	//参与任务的游戏id
	vector<AchieveLevel> vcTaskAward;	//任务各档位奖励
}DKAchieveTaskDef;

typedef struct IntegralTaskTable
{
	int iGameID;
	int iTableGrade;
	int iMinIntegral;
	int iMaxIntegral;
	int iWeight[6];
}IntegralTaskTableDef;

typedef struct IntegralTaskConf
{
	int iTaskID;
	string strTaskName;
	int iPlayType;
	int iWeight[6];
	int iAwardType;
	int iAwardNum[6];
	int iTaskType;
	int iTaskValue;
}IntegralTaskConfDef;

typedef struct LastMonthRecharge
{
	int iVacIndex;
	int iTimeFlag;//充值日期
	int iVacValue;//对应日期总充值金额
};

//一些需要在游戏中完成的活动类任务（后续有新任务在这里持续加）
enum class COMTASK
{
	TEST_COMTASK = 1,  //测试通用任务类型
	ACHIEVE = 2,       //成就任务
	ACT_INVITE = 3,    //拉新活动任务
};

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

	ALL_HU = 18,  //游戏内总胡牌次数
	CONTINUE_FIRST = 19,//连胜（连续获得第一名）
	MAGIC_CNT = 20, //使用魔法表情次数

	COMPLETE_ALL_WEEK = 21,//完成所有周任务
	GET_INTEGRAL_INGMAE = 22,//游戏中获得多少奖券
	GAME_TIME = 23,//游戏时间达到多少分钟

	CONTINUE_LOGIN = 24,//连续登录多少天
	SHARE_CNT = 25,//累计分享多少次
};

//一些常量定义
const string KEY_USER_DAY_TASK = "MJ_USER_DAY_TASK:";               //用户日常任务
const string KEY_USER_WEEK_TASK = "MJ_USER_WEEK_TASK:";             //用户周任务
const string KEY_BK_TASK_LOG = "MJ_TASK_TOTAL_LOG:";                //任务统计日志
const string KEY_USER_DAY_INFO = "MJ_USER_DAY_INFO:%d";             //用户当日信息
const string KEY_RECENT_GAME_INFO = "MJ_RECENT_GAME_INFO:%d_%d";		//玩家历史游戏数据
const string KEY_ROOM_TASK = "MJ_ROOM_TASK_INFO:%d_%d_%d";          //房间任务
const string KEY_INTEGRAL_TASK_HIS = "MJ_GAME%d_INTEGRAL_TASK_HIS:%d";  //奖券任务历史触发记录
const string KEY_USER_RECENT_VAC = "MJ_USER_RECENT_VAC:%d";//玩家近期充值信息
class TaskHandler:public MsgHandleBase
{
public:
	TaskHandler(MsgQueue *pSendQueue, MsgQueue *pEventGetQueue);

	void GetBKTaskConf();

	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen,int iSocketIndex);
	virtual vector<int> GetHandleMsgTypes(){};

	virtual void CallBackOnTimer(int iTimeNow);

	map<int,BKTaskConfDef>m_mapBKDayTask;
	map<int,BKTaskConfDef>m_mapBKWeekTask;
	map<int, DKAchieveTaskDef> m_mapDKAchieveTask;	//durak成就任务
	map<int, int> m_mapActInvite;
	map<int, string> m_mapRoomTask;

	vector<BKTaskLogDef>m_vcTaskLog;	
protected:

	void HandleGetUserInfoReq(char* szMsg, int iLen, int iSocketIndex);
	void HandleGameResultMsg(char* szMsg, int iLen,int iSocketIndex);
	void HandleComTaskCompInfo(char* szMsg, int iLen, int iSocketIndex);
	void HandleTogetherUserMsg(char * szMsg, int iLen, int iSocketIndex);
	void HandleGetRoomTaskReq(char * szMsg, int iLen, int iSocketIndex);
	void HandleGetIntegralTaskInfoMsg(char *msgData, int iLen, int iSocketIndex);
	void HandleSetIntegralTaskHisMsg(char * msgData, int iLen, int iSocketIndex);
	void HandleRefreshUserDayInfo(char * msgData, int iLen, int iSocketIndex);
	void HandleGetLastMonthsVacNum(char * msgData, int iLen, int iSocketIndex);
	void UpdateUserAchieveInfo(RdComOneTaskCompInfoDef* pTaskInfo, int iUserID, int iSocketIndex); //更新成就任务信息
	
	int JudgeComTestTask(int iUserID, int iReqGameID, char * cBuffer, int &iMsgLen);
	void UpdateActInviteInfo(RdComOneTaskCompInfoDef * pTaskInfo, int iUserID, int iSocketIndex);
	void UpdateUserActInviteRed(int iUserID, bool bRed);
	void GetParamsConf();
	int CheckActInviteInfo(int iUserID, int iGameID, char * cBuffer, int & iMsgLen);
	int CheckGetUserAchieveInfoReq(int iUserID, int iGameID, char * cBuffer, int & iMsgLen); //获取成就任务信息
	void CheckUserDayAndWeekTask(int iGameID, int iRoomType,int iUserID, int iSocketIndex);

	int JudgeIfNeedRefreshTask(int iLastRefreshTm,bool bDayTask);

	bool JudgeRefreshDayOrWeekTask(int iUserID,int iGameID,int iRoomType,bool bDayTask,RdUserTaskInfoDef userTask[],int iArrayLen,int &iUserTaskNum);

	bool IfWeekHaveCompAllWeekTask();

	void AddCompTaskLog(int iType,int iID);

	void CheckGetUserDecoratePropInfo(int iGameID, int iUserID, int iSocketIndex);//获取用户装扮信息
	void CheckGetComTaskInfo(int iReqGameID, int iUserID, int iSocketIndex);
	void CheckGetRecentTask(int iReqGameID, int iUserID, int iSocketIndex);
	void CheckGetUserDayInfo(int iReqGameID, int iUserID, int iSocketIndex);  //玩家当日数据	
	void DBGetBKTaskConf(map<int, BKTaskConfDef>& mapDayTask, map<int, BKTaskConfDef>& mapWeekTask);
	void ReportTaskCommonEvent(vector<CommonEvent> vcCommonEvent);

	vector< LastMonthRecharge > GetLastMonthsVac(redisContext *context, int iUserID, int iType,int iFlag,int & iAllInfullNum,int & iAllInfullCount);

	void OnTimerGetAchieveTask();
	DKAchieveTaskDef* findDKAchieveTask(int iTaskID);

	MsgQueue *m_pSendQueue;
	MsgQueue *m_pEventGetQueue;

	int m_iGetTaskConfCDTime;
	int m_iSendLogCDTime;
	int m_iActInviteNeed;
};
#endif // !__RedisThread_H__
