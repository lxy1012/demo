#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "log.h"
#include "net.h"
#include "msg.h"
#include "msg.h"
#include "main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "conf.h"
#include "SQLWrapper.h"
#include "Dispatcher.h"

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/

Dispatcher::Dispatcher()
{
	m_pQueueInner = NULL;
}

Dispatcher::~Dispatcher()
{
}

int Dispatcher::Initialize(MsgQueue *pInnerQueue, CMySQLConnection* pMySql)
{
	m_pQueueInner = pInnerQueue;
	m_pMySql = pMySql;
}

int Dispatcher::Run()
{
	char *szDequeuBuffer = new char[m_pQueueInner->max_msg_len];
	int iDequeueLen = 0;
	
	time_t tmNowTime = time(NULL);
	time_t TimePrev = tmNowTime;

	while(true)
	{
		iDequeueLen = m_pQueueInner->DeQueue(szDequeuBuffer, m_pQueueInner->max_msg_len, 1);
		if (iDequeueLen > 0)
		{
			HandleDispatherNetMsg(szDequeuBuffer, iDequeueLen);
		}

		tmNowTime = time(NULL);

		OnTimer(tmNowTime - TimePrev, tmNowTime);

		TimePrev = tmNowTime;
	}
	return 0;
}

void Dispatcher::HandleDispatherNetMsg(void* pMsgData, int iMsgLen)
{
	MsgHeadDef *msgHead = (MsgHeadDef*)pMsgData;
	
	if (msgHead->iMsgType == DISPATCH_USER_EVENT_LOG_MSG)
	{
		HandleUserEventLogMsg(pMsgData, iMsgLen);
	}
	else
	{
		_log(_ERROR, "DP", "HandleDispatherNetMsg error iMsgType[0x%x]", msgHead->iMsgType);
	}
}

void Dispatcher::HandleUserEventLogMsg(void* pMsgData, int iMsgLen)
{
	DispatchUserEventLogMsgDef* pEventLog = (DispatchUserEventLogMsgDef*)pMsgData;

	_log(_ERROR, "DP", "HandleUserEventLogMsg uid[%d] iEventId[%d] [%lld][%d][%d]", pEventLog->iUserId, pEventLog->iEventId, pEventLog->llMoney, pEventLog->iDiamond, pEventLog->iIntegral);

	CSQLWrapperInsert hSQLInsert("user_event_logs", m_pMySql->GetMYSQLHandle());
	hSQLInsert.SetFieldValueNumber("Userid", pEventLog->iUserId);
	hSQLInsert.SetFieldValueNumber("EventID", pEventLog->iEventId);
	hSQLInsert.SetFieldValueNumber("Money", pEventLog->llMoney);
	hSQLInsert.SetFieldValueNumber("Diamond", pEventLog->iDiamond);
	hSQLInsert.SetFieldValueNumber("Integral", pEventLog->iIntegral);
	hSQLInsert.SetFieldValueNumber("GameIntegral", pEventLog->iGameIntegral);
	hSQLInsert.SetFieldValueNumber("HisIntegral", pEventLog->iHisIntegral);
	hSQLInsert.SetFieldValueNumber("AllCharge", pEventLog->iTotalCharge);
	hSQLInsert.SetFieldValueNumber("AllChargeMoney", pEventLog->iTotalChargeMoney);
	hSQLInsert.SetFieldValueNumber("AllChargeDiamond", pEventLog->iTotalChargeDiamond);
	hSQLInsert.SetFieldValueNumber("AllRelief", pEventLog->iAllRelief);
	hSQLInsert.SetFieldValueNumber("AllGameTime", pEventLog->iGameTime);
	hSQLInsert.SetFieldValueNumber("AllGameCnt", pEventLog->iAllGameCnt);
	hSQLInsert.SetFieldValueNumber("his_relief_num", pEventLog->iHisReliefCnt);
	hSQLInsert.SetFieldValueNumber("game_win_num", pEventLog->iGameWinNum);
	hSQLInsert.SetFieldValueNumber("regist_day", pEventLog->iRegDayNum);
	const std::string& strSQLInsert = hSQLInsert.GetFinalSQLString();
	m_pMySql->RunSQL(strSQLInsert, true);
}

void Dispatcher::OnTimer(int iGapTime, int tmNowTime)
{
	static time_t iTimeTestMySql = 0;

	iTimeTestMySql += iGapTime;
	if (iTimeTestMySql > 60)
	{
		iTimeTestMySql = 0;
		TestMysqlConnection();
	}
}

void Dispatcher::TestMysqlConnection()
{
	//每分钟检测下数据库连接状态
	bool bConnectOK = true;
	CMySQLTableResult hMySQLTableResult;
	m_pMySql->RunSQL("SELECT 1 FROM DUAL", hMySQLTableResult);
	const CMySQLTableRecord& hMySQLTableRecord = hMySQLTableResult.GetOneRecord();
	if (!hMySQLTableRecord.IsAvailable())
	{
		bConnectOK = false;
	}
	if (1 != hMySQLTableRecord.GetFieldAsInt32("1"))
	{
		bConnectOK = false;
	}
	if (!bConnectOK)
	{
		MySQLConfig stMySQLConfig;
		stMySQLConfig.strHost = stSysConfigBaseInfo.szDBGameHost;
		stMySQLConfig.strDBName = stSysConfigBaseInfo.szDBGameDataBase;
		stMySQLConfig.nPort = stSysConfigBaseInfo.iDBGamePort;
		stMySQLConfig.strUser = stSysConfigBaseInfo.szDBGame;
		stMySQLConfig.strPassword = stSysConfigBaseInfo.szDBGamePasswd;
		// ProbeMySQLConnection can tell the status of mysql
		CMySQLConnection::ProbeMySQLConnection(stMySQLConfig);
		CMySQLConnection hMySQLConnection;
		hMySQLConnection.SetConnectionConfig(stMySQLConfig);
		hMySQLConnection.SetConnectionName("newgcuser2");
		bool bReConnectOK = hMySQLConnection.OpenConnection();
		if (!bReConnectOK)
		{
			_log(_ERROR, "DP", "TestMysqlConnection error mySql[%s]:[%s]", stSysConfigBaseInfo.szDBGameDataBase, stSysConfigBaseInfo.szDBGame);
			exit(0);
		}
		m_pMySql->CloseConnection();
		m_pMySql = &hMySQLConnection;
	}
}