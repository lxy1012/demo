#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include "thread.h"
#include "hashtable.h"
#include "msgqueue.h"
#include "msg.h"
#include "MySQLConnection.h"

#define DISPATCH_USER_EVENT_LOG_MSG		0x01	//内部用户关键事件打点消息

typedef struct DispatchUserEventLogMsg
{
	MsgHeadDef msgHead;
	int iEventId;
	int iUserId;
	long long llMoney;
	int iDiamond;
	int iIntegral;
	int iGameIntegral;
	int iHisIntegral;
	int iTotalCharge;
	long long iTotalChargeMoney;
	int iTotalChargeDiamond;
	int iTotalChargeCnt;
	long long iAllRelief;
	int iGameTime;
	int iAllGameCnt;
	int iHisReliefCnt;
	int iGameWinNum;
	int iRegDayNum;
}DispatchUserEventLogMsgDef;

class Dispatcher : public Thread
{
public:
	Dispatcher();
	~Dispatcher();

	int Initialize(MsgQueue *pInnerQueue, CMySQLConnection* pMySql);

private:	
	int Run();

	void HandleDispatherNetMsg(void* pMsgData, int iMsgLen);
	void HandleUserEventLogMsg(void* pMsgData, int iMsgLen);

	void OnTimer(int iGapTime, int tmNowTime);
	void TestMysqlConnection();  //检测数据库连接

	MsgQueue *m_pQueueInner;     //Dispatcher的消息检测队列
	CMySQLConnection* m_pMySql;
};

#endif