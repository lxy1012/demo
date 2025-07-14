#ifndef __EVENT_LOG_Handler_H__
#define __EVENT_LOG_Handler_H__

#include "MsgHandleBase.h"

#include <string>
#include <vector>
#include <map>
using namespace  std;
#include "msg.h"

enum eEventID {
	LOBBY_SLOT_TASK = 99005,
	LOBBY_XAMS_TASK = 99009,
};

typedef struct ComStatMainConf
{
	int iEventID;
	int iIfStart;//事件是否开启
}ComStatMainConfDef;

typedef struct ComStatSubConf
{
	int iEventID;
	int iSubEventID;
	int iSubEventType;//0常规事件 1曲线事件
	int iSubEventGapTime;//曲线事件配置多长时间记录一次
	int iNextRecordTm;//曲线事件下次记录时间
	int iIfStart;//事件是否开启（0关闭，1开启）
}ComStatSubConfDef;
typedef vector<ComStatSubConfDef> vcComStatConf;

typedef struct onlineHistoryInfo {
	int iGameID;
	int iRoomOnline[10];
}onlineHistoryInfoDef;

class EventLogHandler :public MsgHandleBase
{
public:
	static EventLogHandler* shareEventHandler();
	virtual void HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex);
	virtual vector<int> GetHandleMsgTypes() {};
	virtual void CallBackOnTimer(int iTimeNow);
	virtual void GetDBEventConf();
	void AddTaskEventStat(char *strKey, char *strField, int iAddNum);

	map<int, vcComStatConf>m_mapAllStatConf;//所有需要统计的事件
protected:
	EventLogHandler();
	static EventLogHandler* m_pInstance;

	int m_iGetConfCDTime;

	void HandleGameComEventInfo(char* szMsg, int iLen);
	void HandleUsePropStatInfo(char * szMsg, int iLen);
	void HandleAssignTmStat(char * szMsg, int iLen);
	//通用事件统计信息	
	void HadleGameBaseStatInfo(char* szMsg, int iLen);//游戏通用次留等统计信息
	void HandleNCenterStatInfo(char* szMsg, int iLen);//中心服务器配桌统计信息
	void SysGameEvent(ComStatSubConfDef* pEventInfo);
	void ResetGameEventNextTime(ComStatSubConfDef* pEventInfo, int iHourStartTm, int iTodayEndTm,int iTmNow);
	void DBGetComStatConf(map<int, vcComStatConf>&mapStatConf);
	void DBGetComStatSubConf(int iEventID,vcComStatConf &vcConf);
	void CheckGameEvent(ComStatSubConfDef* pStatConf,map<int, vcComStatConf>& mapStatConf);
	void HandleReportCommonEventInfo(char* szMsg, int iLen);
	
	void RecordRoomHisOnlineNum();

	void CallBackAddGameEvent(int iGameID, int iEventID, vector<ComStatOneEventInfo> vcEventInfo);
};

#endif // !__RedisThread_H__
