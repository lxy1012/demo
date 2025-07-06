#include "EventLogHandler.h"
#include "RedisHelper.h"
#include <time.h>
#include "Util.h"
#include "log.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"

#include "SQLWrapper.h"
#include "MySQLTableStruct.h"

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/

EventLogHandler *EventLogHandler::m_pInstance = nullptr;

EventLogHandler::EventLogHandler()
{
	m_iGetConfCDTime = 900;//每15分钟读一次事件配置
}

EventLogHandler * EventLogHandler::shareEventHandler()
{
	if (m_pInstance == nullptr)
	{
		try
		{
			m_pInstance = new EventLogHandler();
		}
		catch (std::bad_alloc)
		{
			return nullptr;
		}
	}
	return m_pInstance;
}

void EventLogHandler::GetDBEventConf()
{
	_log(_ERROR, "EventLogHandler", "EventLogHandler GetDBEventConf iIfMainRedis[%d]", stSysConfigBaseInfo.iIfMainRedis);
	DBGetComStatConf(m_mapAllStatConf);
	time_t iNowHour = Util::GetNowHourStartTimeStamp();
	time_t iEndTm = Util::GetTodayStartTimestamp() + 24 * 3600;
	time_t iTmNow = time(NULL);
	map<int, vcComStatConf>::iterator pos = m_mapAllStatConf.begin();
	while (pos != m_mapAllStatConf.end())
	{
		for (int i = 0; i < pos->second.size(); i++)
		{
			ResetGameEventNextTime(&pos->second[i], iNowHour, iEndTm, iTmNow);
			//_log(_ERROR, "EventLogHandler", "event[%d:%d],tm[%d],next[%d]", pos->second[i].iEventID, pos->second[i].iSubEventID, pos->second[i].iSubEventGapTime, pos->second[i].iNextRecordTm);
		}
		pos++;
	}
}

void EventLogHandler::AddTaskEventStat(char * strKey, char * strField, int iAddNum)
{
	if (m_redisCt == NULL)
	{
		_log(_ERROR, "EventLogHandler::HandleMsg", "error m_redisCt == NULL");
		return;
	}
	RedisHelper::hincrby(m_redisCt, strKey, strField, iAddNum);
	RedisHelper::expire(m_redisCt, strKey, 35 * 3600 * 24);
}

void EventLogHandler::HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex)
{
	if (m_redisCt == NULL)
	{
		_log(_ERROR, "EventLogHandler::HandleMsg", "error m_redisCt == NULL");
		return;
	}
	time_t iTimeBegin = time(NULL);

	_log(_DEBUG, "ELH", "HandleMsg-->> iMsgType[0x%x]", iMsgType);
	if (iMsgType == RD_REPORT_COMMON_EVENT_MSG)
	{
		HandleGameComEventInfo(szMsg, iLen);
	}
	else if (iMsgType == REDIS_GAME_BASE_STAT_INFO_MSG)
	{
		HadleGameBaseStatInfo(szMsg, iLen);
	}
	else if (iMsgType == REDIS_RECORD_NCENTER_STAT_INFO)
	{
		HandleNCenterStatInfo(szMsg, iLen);
	}
	else if (iMsgType == REDIS_USE_PROP_STAT_MSG)
	{
		HandleUsePropStatInfo(szMsg, iLen);
	}
	else if (iMsgType == REDIS_ASSIGN_NCENTER_STAT_INFO)
	{
		HandleAssignTmStat(szMsg, iLen);
	}
	else if (iMsgType == RD_REPORT_COMMON_EVENT_MSG)
	{
		HandleReportCommonEventInfo(szMsg, iLen);
	}
	time_t iTimeEnd = time(NULL);
	time_t iGap = iTimeEnd - iTimeBegin;
	if (iGap > 1)
	{
		_log(_ERROR, "ELH", "HandleMsg-->> iMsgType[0x%x] iGap[%d]", iMsgType, iGap);
	}
}

void EventLogHandler::ResetGameEventNextTime(ComStatSubConfDef* pEventInfo, int iHourStartTm, int iTodayEndTm,int iTmNow)
{
	if (pEventInfo->iSubEventGapTime == 0)
	{
		return;
	}
	int iGap = 60 / pEventInfo->iSubEventGapTime;
	if (60 % pEventInfo->iSubEventGapTime != 0)
	{
		iGap++;
	}
	if (true)
	{
		pEventInfo->iSubEventGapTime == 0;
	}
	int iLastTm = iHourStartTm;
	for (int j = 1; j <= iGap; j++)
	{
		int iJudgeTm = iHourStartTm + j*pEventInfo->iSubEventGapTime *60-2;//提前2s
		//_log(_DEBUG, "EHR", "pEventInfo[%d:%d],[%d],iLastTm[%d],iJudgeTm[%d],iHourStartTm[%d]", pEventInfo->iEventID, pEventInfo->iSubEventID,j,iJudgeTm, iLastTm, iJudgeTm, iHourStartTm);
		if (iTmNow >= iLastTm && iTmNow <= iJudgeTm)
		{
			pEventInfo->iNextRecordTm = iJudgeTm;
			if (pEventInfo->iNextRecordTm > iTodayEndTm)
			{
				pEventInfo->iNextRecordTm = iTodayEndTm-2;
			}
			break;
		}
		iLastTm = iJudgeTm;
	}
}

void EventLogHandler::DBGetComStatConf(map<int, vcComStatConf>& mapStatConf)
{
	if (m_pMySqlGame == NULL)
	{
		_log(_ERROR, "ELH", "DBGetComStatConf -->>>>> m_pMySqlGame is null");
		return;
	}
	CSQLWrapperSelect hSQLSelect("stat_main_conf", m_pMySqlGame->GetMYSQLHandle());
	hSQLSelect.AddQueryField("EventID");
	hSQLSelect.AddQueryField("IfStart");
	hSQLSelect.AddQueryConditionNumber("EventID", 0, CSQLWrapper::ERelationType::Greater);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlGame->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "SLT", "DBGetGameEventRecordConf err");
	}
	else
	{
		const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();		
		
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			int iIfStart = hRecord.GetFieldAsInt32("IfStart");
			if (iIfStart == 1)
			{
				int iEventID = hRecord.GetFieldAsInt32("EventID");
				vcComStatConf vcConf;
				DBGetComStatSubConf(iEventID, vcConf);
				if (!vcConf.empty())
				{
					mapStatConf[iEventID] = vcConf;
				}
				else
				{
					_log(_DEBUG, "SLT", "DBGetComStatConf mian[%d] iIfStart[%d] sub is nil !!!", iEventID, iIfStart);
				}
			}
		}
	}
}

void  EventLogHandler::DBGetComStatSubConf(int iEventID, vcComStatConf& vcConf)
{
	if (m_pMySqlGame == NULL)
	{
		return;
	}
	CSQLWrapperSelect hSQLSelect("stat_sub_conf", m_pMySqlGame->GetMYSQLHandle());
	hSQLSelect.AddQueryField("EventID");
	hSQLSelect.AddQueryField("SubEventID");
	hSQLSelect.AddQueryField("SubEventType");
	hSQLSelect.AddQueryField("SubEventTime");
	hSQLSelect.AddQueryField("IfStart");
	hSQLSelect.AddQueryConditionNumber("EventID", iEventID);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlGame->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "SLT", "DBGetComStatSubConf err");
	}
	else
	{
		const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
		string strLog;
		char cSubID[16] = { 0 };
		strLog.clear();
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			int iIfStart = hRecord.GetFieldAsInt32("IfStart");
			if (iIfStart == 1)
			{
				ComStatSubConfDef  stComConf;
				memset(&stComConf,0,sizeof(ComStatSubConfDef));
				stComConf.iEventID = iEventID;
				stComConf.iIfStart = iIfStart;
				stComConf.iSubEventID = hRecord.GetFieldAsInt32("SubEventID");
				stComConf.iSubEventType = hRecord.GetFieldAsInt32("SubEventType");
				stComConf.iSubEventGapTime = hRecord.GetFieldAsInt32("SubEventTime");
				vcConf.push_back(stComConf);
				
				memset(cSubID, 0, sizeof(cSubID));
				sprintf(cSubID, "%d,", stComConf.iSubEventID);
				strLog += cSubID;
			}
		}
		_log(_DEBUG, "SLT", "DBGetGameEventRecordConf mian[%d] sub[%s]", iEventID, strLog.c_str());
	}
}

void EventLogHandler::CallBackOnTimer(int iTimeNow)
{
	m_iGetConfCDTime--;
	if (m_iGetConfCDTime <= 0)
	{
		map<int, vcComStatConf>::iterator pos = m_mapAllStatConf.begin();
		while (pos != m_mapAllStatConf.end())
		{
			for (int i = 0; i < pos->second.size(); i++)
			{
				pos->second[i].iIfStart = 0;
			}
			pos++;
		}
		map<int, vcComStatConf>mapStatConf;
		DBGetComStatConf(mapStatConf);
		pos = mapStatConf.begin();
		while (pos != mapStatConf.end())
		{
			for (int i = 0; i < pos->second.size(); i++)
			{
				if (pos->second[i].iIfStart == 1)
				{
					CheckGameEvent(&pos->second[i], m_mapAllStatConf);
				}							
			}
			pos++;
		}
			
		time_t iNowHour = Util::GetNowHourStartTimeStamp();
		time_t iEndTm = Util::GetTodayStartTimestamp() + 24 * 3600;

		pos = m_mapAllStatConf.begin();
		while (pos != m_mapAllStatConf.end())
		{
			for (int i = 0; i < pos->second.size();)
			{
				if (pos->second[i].iIfStart == 0)
				{
					pos->second.erase(pos->second.begin() + i);
					continue;
				}
				if (pos->second[i].iNextRecordTm == 0)
				{
					ResetGameEventNextTime(&pos->second[i], iNowHour, iEndTm, iTimeNow);
				}
				i++;
			}
			pos++;
		}
		m_iGetConfCDTime = 900;
	}

	//主redis做事件曲线和人数曲线
	if (stSysConfigBaseInfo.iIfMainRedis == 1)
	{
		map<int, vcComStatConf>::iterator pos = m_mapAllStatConf.begin();
		while (pos != m_mapAllStatConf.end())
		{
			for (int i = 0; i < pos->second.size(); i++)
			{
				if (iTimeNow >= pos->second[i].iNextRecordTm && pos->second[i].iNextRecordTm > 0)
				{
					SysGameEvent(&pos->second[i]);
				}
			}
			pos++;
		}

		int iSurplus = iTimeNow % (30 * 60);	//整30分钟记录一次
		if (iSurplus == 0)
		{
			RecordRoomHisOnlineNum();
		}
	}
}

void EventLogHandler::RecordRoomHisOnlineNum()
{
	if (m_pMySqlGame == NULL)
	{
		return;
	}
	
	CSQLWrapperSelect hSQLSelect("server_online", m_pMySqlGame->GetMYSQLHandle());
	hSQLSelect.AddQueryField("ServerID");
	hSQLSelect.AddQueryField("GameID");
	hSQLSelect.AddQueryField("RoomOnlineNum0");
	hSQLSelect.AddQueryField("RoomOnlineNum1");
	hSQLSelect.AddQueryField("RoomOnlineNum2");
	hSQLSelect.AddQueryField("RoomOnlineNum3");
	hSQLSelect.AddQueryField("RoomOnlineNum4");
	hSQLSelect.AddQueryField("RoomOnlineNum5");
	hSQLSelect.AddQueryField("RoomOnlineNum6");
	hSQLSelect.AddQueryField("RoomOnlineNum7");
	hSQLSelect.AddQueryField("RoomOnlineNum8");
	hSQLSelect.AddQueryField("RoomOnlineNum9");
	hSQLSelect.AddQueryField("LastTime");

	//可以不加查询条件 这张表一共就几条数据
	/*hSQLSelect.AddQueryConditionNumber("ServerID", iServerId);
	hSQLSelect.AddQueryConditionNumber("GameID", iGameId);*/

	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlGame->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "ELH", "RecordRoomHisOnlineNum find server_online err");
		return;
	}

	const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
	if (!vecRecordList.empty())
	{
		time_t tmNow = time(NULL);
		vector<onlineHistoryInfoDef> vcHisNum;

		const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			int iLastTime = Util::ConvertTimeStamp((char*)hRecord.GetFieldAsString("LastTime").c_str());
			if (tmNow - iLastTime > 30 * 60)	//距离上次更新时间超过30min 认为该数据失效了
			{
				continue;
			}

			onlineHistoryInfoDef hisOnlineNum;
			memset(&hisOnlineNum, 0, sizeof(onlineHistoryInfoDef));

			hisOnlineNum.iGameID = hRecord.GetFieldAsInt32("GameID");
			hisOnlineNum.iRoomOnline[0] = hRecord.GetFieldAsInt32("RoomOnlineNum0");
			hisOnlineNum.iRoomOnline[1] = hRecord.GetFieldAsInt32("RoomOnlineNum1");
			hisOnlineNum.iRoomOnline[2] = hRecord.GetFieldAsInt32("RoomOnlineNum2");
			hisOnlineNum.iRoomOnline[3] = hRecord.GetFieldAsInt32("RoomOnlineNum3");
			hisOnlineNum.iRoomOnline[4] = hRecord.GetFieldAsInt32("RoomOnlineNum4");
			hisOnlineNum.iRoomOnline[5] = hRecord.GetFieldAsInt32("RoomOnlineNum5");
			hisOnlineNum.iRoomOnline[6] = hRecord.GetFieldAsInt32("RoomOnlineNum6");
			hisOnlineNum.iRoomOnline[7] = hRecord.GetFieldAsInt32("RoomOnlineNum7");
			hisOnlineNum.iRoomOnline[8] = hRecord.GetFieldAsInt32("RoomOnlineNum8");
			hisOnlineNum.iRoomOnline[9] = hRecord.GetFieldAsInt32("RoomOnlineNum9");

			bool bFind = false;
			for (int i = 0; i < vcHisNum.size(); i++)
			{
				if (vcHisNum.at(i).iGameID == hisOnlineNum.iGameID)
				{
					bFind = true;

					vcHisNum.at(i).iRoomOnline[0] += hisOnlineNum.iRoomOnline[0];
					vcHisNum.at(i).iRoomOnline[1] += hisOnlineNum.iRoomOnline[1];
					vcHisNum.at(i).iRoomOnline[2] += hisOnlineNum.iRoomOnline[2];
					vcHisNum.at(i).iRoomOnline[3] += hisOnlineNum.iRoomOnline[3];
					vcHisNum.at(i).iRoomOnline[4] += hisOnlineNum.iRoomOnline[4];
					vcHisNum.at(i).iRoomOnline[5] += hisOnlineNum.iRoomOnline[5];
					vcHisNum.at(i).iRoomOnline[6] += hisOnlineNum.iRoomOnline[6];
					vcHisNum.at(i).iRoomOnline[7] += hisOnlineNum.iRoomOnline[7];
					vcHisNum.at(i).iRoomOnline[8] += hisOnlineNum.iRoomOnline[8];
					vcHisNum.at(i).iRoomOnline[9] += hisOnlineNum.iRoomOnline[9];
				}
			}

			if (!bFind)
			{
				vcHisNum.push_back(hisOnlineNum);
			}
		}

		//统计到room_online_history表,一个游戏一次记录6条，0混服，4新手，5初级，6中级，7高级，8vip (1239不记录)
		for (int i = 0; i < vcHisNum.size(); i++)
		{
			for (int k = 0; k <= 8; k++)
			{
				if (k == 1 || k == 2 || k == 3 || k == 9)	//1239不记录
				{
					continue;
				}

				CSQLWrapperInsert hSQLInsert("room_online_history", m_pMySqlGame->GetMYSQLHandle());
				hSQLInsert.SetFieldValueNumber("GameID", vcHisNum.at(i).iGameID);
				hSQLInsert.SetFieldValueNumber("RoomType", k);
				hSQLInsert.SetFieldValueNumber("OnlineNum", vcHisNum.at(i).iRoomOnline[k]);

				const std::string& strSQLInsert = hSQLInsert.GetFinalSQLString();
				m_pMySqlGame->RunSQL(strSQLInsert, true);

				_log(_DEBUG, "ELH", "RecordRoomHisOnlineNum all gameId[%d] roomType[%d] onlineNum[%d]", vcHisNum.at(i).iGameID, k, vcHisNum.at(i).iRoomOnline[k]);
			}
		}
	}
}

void EventLogHandler::SysGameEvent(ComStatSubConfDef* pEventInfo)
{
	if (pEventInfo == NULL)
	{
		return;
	}
	int iDateFlag = Util::GetDayTimeFlag(pEventInfo->iNextRecordTm);
	char strKey[64];
	sprintf(strKey, "MJ_COM_STAT%d:%d", pEventInfo->iEventID, iDateFlag);

	char strField[64];
	sprintf(strField, "event_%d", pEventInfo->iSubEventID);

	long long iNum = 0;

	string strRt = RedisHelper::hget(m_redisCt, strKey, strField);
	if (strRt.size() > 0)
	{
		iNum = atoll(strRt.c_str());
	}
	sprintf(strKey, "MJ_COM_STAT_SEC%d_%d:%d", pEventInfo->iEventID, pEventInfo->iSubEventID,iDateFlag);
	int iSysCnt = 0;
	strRt = RedisHelper::hget(m_redisCt, strKey, "syn_cnt");
	if (strRt.size() > 0)
	{
		iSysCnt = atoi(strRt.c_str());
	}
	RedisHelper::hincrby(m_redisCt, strKey, "syn_cnt", 1);
	RedisHelper::expire(m_redisCt, strKey, 35 * 2600 * 24);
	iSysCnt++;
	char cValue[32];
	sprintf(cValue,"%d_%lld", time(NULL),iNum);
	sprintf(strField, "syn_%d", iSysCnt);
	RedisHelper::hset(m_redisCt, strKey, strField, cValue);
	time_t iNowHour = Util::GetNowHourStartTimeStamp();
	time_t iEndTm = iNowHour + 24 * 3600;
	pEventInfo->iNextRecordTm += pEventInfo->iSubEventGapTime*60;//提前2s
	if (pEventInfo->iNextRecordTm > iEndTm)
	{
		pEventInfo->iNextRecordTm = iEndTm-2;
	}
	//_log(_DEBUG, "EHR", "SysGameEvent[%d],iDateFlag[%d],iSysCnt[%d][%d],iNextRecordTm[%d]", pEventInfo->iEventID, iDateFlag, iSysCnt, iNum, pEventInfo->iNextRecordTm);
}

void EventLogHandler::HadleGameBaseStatInfo(char* szMsg, int iMsgLen)
{
	static char szBuffer[2048] = { 0 };	

	RdGameBaseStatInfoDef* pMsg = (RdGameBaseStatInfoDef*)szMsg;

	char cKey[64] = { 0 };
	time_t iTmFlag = Util::GetDayTimeFlag(pMsg->iTmFlag);
	sprintf(cKey, "mj_game_%d_base_stat:%d",pMsg->iJudgeGameID, iTmFlag);
	//_log(_DEBUG, "EHR", "HadleGameBaseStatInfo:[%d][%d][%d],[%d,%d,%d]", pMsg->iJudgeGameID, pMsg->iTmFlag,iTmFlag, pMsg->stPlatStatInfo[0].iActiveCnt, pMsg->stPlatStatInfo[0].iNewAddCnt, pMsg->stPlatStatInfo[0].iAllGameCnt);
	//_log(_DEBUG, "EHR", "HadleGameBaseStatInfo:[%d,%d,%d,%d,%d]", pMsg->stPlatStatInfo[0].iNewAddContinue2, pMsg->stPlatStatInfo[0].iNewAddContinue3, pMsg->stPlatStatInfo[0].iNewAddContinue7, pMsg->stPlatStatInfo[0].iNewAddContinue15, pMsg->stPlatStatInfo[0].iNewAddContinue30);
	
	string strTemp[8] = { "act_player", "new_player", "new_continue2", "new_continue3", "new_continue7", "new_continue15", "new_continue30", "all_num" };
	string strPlatform[3] = {"ad","ios","wxm"};

	char cField[32];
	for (int i = 0; i < 3; i++)
	{
		sprintf(cKey, "mj_game_%d_base_stat:%d", pMsg->iJudgeGameID, iTmFlag);
		if (pMsg->stPlatStatInfo[i].iActiveCnt > 0)
		{
			sprintf(cField, "%s_%s", strPlatform[i].c_str(), strTemp[0].c_str());
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stPlatStatInfo[i].iActiveCnt);
		}
		if (pMsg->stPlatStatInfo[i].iNewAddCnt > 0)
		{
			sprintf(cField, "%s_%s", strPlatform[i].c_str(), strTemp[1].c_str());
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stPlatStatInfo[i].iNewAddCnt);
		}
		
		if (pMsg->stPlatStatInfo[i].iAllGameCnt > 0)
		{
			sprintf(cField, "%s_%s", strPlatform[i].c_str(), strTemp[7].c_str());
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stPlatStatInfo[i].iAllGameCnt);
		}
		RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);

		if (pMsg->stPlatStatInfo[i].iNewAddContinue2 > 0)
		{
			//重新计算时间戳,统计到对应的天数上
			time_t iLastTm = pMsg->iTmFlag - 24 * 3600;
			int iDateFlg = Util::GetDayTimeFlag(iLastTm);
			sprintf(cKey, "mj_game_%d_base_stat:%d", pMsg->iJudgeGameID, iDateFlg);
			sprintf(cField, "%s_%s", strPlatform[i].c_str(), strTemp[2].c_str());
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stPlatStatInfo[i].iNewAddContinue2);
			RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
		}

		if (pMsg->stPlatStatInfo[i].iNewAddContinue3 > 0)
		{
			//重新计算时间戳
			time_t iLastTm = pMsg->iTmFlag - 24 * 3600*2;
			int iDateFlg = Util::GetDayTimeFlag(iLastTm);
			sprintf(cKey, "mj_game_%d_base_stat:%d", pMsg->iJudgeGameID, iDateFlg);
			sprintf(cField, "%s_%s", strPlatform[i].c_str(), strTemp[3].c_str());
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stPlatStatInfo[i].iNewAddContinue3);
			RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
		}

		if (pMsg->stPlatStatInfo[i].iNewAddContinue7 > 0)
		{
			//重新计算时间戳
			time_t iLastTm = pMsg->iTmFlag - 24 * 3600 * 6;
			int iDateFlg = Util::GetDayTimeFlag(iLastTm);
			sprintf(cKey, "mj_game_%d_base_stat:%d", pMsg->iJudgeGameID, iDateFlg);
			sprintf(cField, "%s_%s", strPlatform[i].c_str(), strTemp[4].c_str());
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stPlatStatInfo[i].iNewAddContinue7);
			RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
		}

		if (pMsg->stPlatStatInfo[i].iNewAddContinue15 > 0)
		{
			//重新计算时间戳
			time_t iLastTm = pMsg->iTmFlag - 24 * 3600 * 14;
			int iDateFlg = Util::GetDayTimeFlag(iLastTm);
			sprintf(cKey, "mj_game_%d_base_stat:%d", pMsg->iJudgeGameID, iDateFlg);
			sprintf(cField, "%s_%s", strPlatform[i].c_str(), strTemp[5].c_str());
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stPlatStatInfo[i].iNewAddContinue15);
			RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
		}

		if (pMsg->stPlatStatInfo[i].iNewAddContinue30 > 0)
		{
			//重新计算时间戳
			time_t iLastTm = pMsg->iTmFlag - 24 * 3600 * 29;
			int iDateFlg = Util::GetDayTimeFlag(iLastTm);
			sprintf(cKey, "mj_game_%d_base_stat:%d", pMsg->iJudgeGameID, iDateFlg);
			sprintf(cField, "%s_%s", strPlatform[i].c_str(), strTemp[6].c_str());
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stPlatStatInfo[i].iNewAddContinue30);
			RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
		}
	}

	//4,5,6,7,8 4种房间对应的详细信息
	sprintf(cKey, "mj_game_%d_base_stat:%d", pMsg->iJudgeGameID, iTmFlag);
	long long lUserAllWinMoney = 0;
	long long lUserAllLoseMoney = 0;
	long long lUserAllTableMoney = 0;
	long long lAllExtraSendMoney = 0;
	long long lAllExtraUseMoney = 0;
	long long lAllReliefMoney = 0;

	int iAllSendDiamond = 0;
	int iAllUseDiamond = 0;
	
	int iAllSendIntegral = 0;
	int iAllUseIntegral = 0;


	for (int i = 0; i < 5; i++)
	{
		int iRoomType = i + 5;
		if (pMsg->stRoomStat[i].iBrokenCnt  > 0)
		{
			sprintf(cField, "room%d_broken", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iBrokenCnt);
		}
		if (pMsg->stRoomStat[i].iKickCnt > 0)
		{
			sprintf(cField, "room%d_kick", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iKickCnt);
		}
		if (pMsg->stRoomStat[i].iReliefCnt > 0)
		{
			sprintf(cField, "room%d_relief_cnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iReliefCnt);
		}
		if (pMsg->stRoomStat[i].iReliefNum > 0)
		{
			sprintf(cField, "room%d_relief_num", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iReliefNum);
			lAllReliefMoney += pMsg->stRoomStat[i].iReliefNum;
		}
		if (pMsg->stRoomStat[i].iVacCnt > 0)
		{
			sprintf(cField, "room%d_vac_cnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iVacCnt);
		}
		if (pMsg->stRoomStat[i].iVacNum > 0)
		{
			sprintf(cField, "room%d_vac_num", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iVacNum);
		}
		if (pMsg->stRoomStat[i].iWinCnt > 0)
		{
			sprintf(cField, "room%d_win_cnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iWinCnt);
		}
		if (pMsg->stRoomStat[i].llAllWin > 0)
		{
			sprintf(cField, "room%d_all_win", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llAllWin);
			lUserAllWinMoney += pMsg->stRoomStat[i].llAllWin;
		}
		if (pMsg->stRoomStat[i].iLoseCnt > 0)
		{
			sprintf(cField, "room%d_lose_cnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iLoseCnt);
		}
		if (pMsg->stRoomStat[i].llAllLose > 0)
		{
			sprintf(cField, "room%d_all_lose", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llAllLose);
			lUserAllLoseMoney += pMsg->stRoomStat[i].llAllLose;
		}
		if (pMsg->stRoomStat[i].iPingCnt > 0)
		{
			sprintf(cField, "room%d_ping_cnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iPingCnt);
		}
		if (pMsg->stRoomStat[i].iTableMoney > 0)
		{
			sprintf(cField, "room%d_table_money", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iTableMoney);
			lUserAllTableMoney += pMsg->stRoomStat[i].iTableMoney;
		}
		if (pMsg->stRoomStat[i].iAIPlayCnt > 0)
		{
			sprintf(cField, "room%d_aiplay_cnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iAIPlayCnt);
		}
		if (pMsg->stRoomStat[i].iAIPlayWin > 0)
		{
			sprintf(cField, "room%d_aiplay_wincnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iAIPlayWin);

			RedisHelper::hincrby(m_redisCt, cKey, "play_ai_win_cnt", pMsg->stRoomStat[i].iAIPlayWin);
		}
		if (pMsg->stRoomStat[i].llAIPlayWinMoney > 0)
		{
			sprintf(cField, "room%d_aiplay_win", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llAIPlayWinMoney);

			RedisHelper::hincrby(m_redisCt, cKey, "play_ai_win_money", pMsg->stRoomStat[i].llAIPlayWinMoney);
		}
		if (pMsg->stRoomStat[i].iAIPlayLose > 0)
		{
			sprintf(cField, "room%d_aiplay_losecnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iAIPlayLose);

			RedisHelper::hincrby(m_redisCt, cKey, "play_ai_los_cnt", pMsg->stRoomStat[i].iAIPlayLose);
		}
		if (pMsg->stRoomStat[i].llAIPlayLoseMoney > 0)
		{
			sprintf(cField, "room%d_aiplay_lose", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llAIPlayLoseMoney);

			RedisHelper::hincrby(m_redisCt, cKey, "play_ai_los_money", pMsg->stRoomStat[i].llAIPlayLoseMoney);
		}
		if (pMsg->stRoomStat[i].iAIPlayPing > 0)
		{
			sprintf(cField, "room%d_aiplay_ping", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iAIPlayPing);
		}
		if (pMsg->stRoomStat[i].iAIControlWinCnt > 0)
		{
			sprintf(cField, "room%d_ai_win_cnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iAIControlWinCnt);
		}
		if (pMsg->stRoomStat[i].llAIControlWin > 0)
		{
			sprintf(cField, "room%d_ai_win_money", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llAIControlWin);
		}
		if (pMsg->stRoomStat[i].iAIControlLoseCnt > 0)
		{
			sprintf(cField, "room%d_ai_los_cnt", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iAIControlLoseCnt);
		}
		if (pMsg->stRoomStat[i].llAIControlLose > 0)
		{
			sprintf(cField, "room%d_ai_los_money", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llAIControlLose);
		}
		if (pMsg->stRoomStat[i].llExtraSendMoney > 0)
		{
			sprintf(cField, "room%d_extra_send_money", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llExtraSendMoney);
			lAllExtraSendMoney += pMsg->stRoomStat[i].llExtraSendMoney;
		}
		if (pMsg->stRoomStat[i].iSendDiamond > 0)
		{
			sprintf(cField, "room%d_send_diamond", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iSendDiamond);
			iAllSendDiamond += pMsg->stRoomStat[i].iSendDiamond;
		}
		if (pMsg->stRoomStat[i].llSendMoney > 0)
		{
			sprintf(cField, "room%d_send_money", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llSendMoney);
		}
		if (pMsg->stRoomStat[i].iUseDiamond > 0)
		{
			sprintf(cField, "room%d_use_diamond", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iUseDiamond);
			iAllUseDiamond += pMsg->stRoomStat[i].iUseDiamond;
		}
		if (pMsg->stRoomStat[i].llUseMoney > 0)
		{
			sprintf(cField, "room%d_use_money", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llUseMoney);
		}
		if (pMsg->stRoomStat[i].llExtraUseMoney > 0)
		{
			sprintf(cField, "room%d_extra_use_money", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].llExtraUseMoney);
			lAllExtraUseMoney += pMsg->stRoomStat[i].llExtraUseMoney;
		}
		if (pMsg->stRoomStat[i].iUseIntegral > 0)
		{
			sprintf(cField, "room%d_use_integral", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iUseIntegral);
			iAllUseIntegral += pMsg->stRoomStat[i].iUseIntegral;
		}
		if (pMsg->stRoomStat[i].iSendIntegral > 0)
		{
			sprintf(cField, "room%d_send_integral", iRoomType);
			RedisHelper::hincrby(m_redisCt, cKey, cField, pMsg->stRoomStat[i].iSendIntegral);
			iAllSendIntegral += pMsg->stRoomStat[i].iSendIntegral;
		}
	}
	RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
	//金流报表，奖券报表，钻石报表在游戏内的统计在这里汇总
	//金流报表（201-玩家游戏总赢金币，202-玩家游戏总输金币，203-玩家总桌费，204-游戏额外总发放金币，205-游戏总发放救济，206-游戏额外回收金币）
	sprintf(cKey, "MJ_PROP_%d_TOTAL_LOG:%d", 1, iTmFlag);
	long long lAllSendMoney = lUserAllWinMoney+ lAllExtraSendMoney+ lAllReliefMoney;
	long long lAllUseMoney = lUserAllLoseMoney+ lUserAllTableMoney+ lAllExtraUseMoney;
	sprintf(cField, "send_sub_%d_%d", pMsg->iJudgeGameID,201);
	RedisHelper::hincrby(m_redisCt, cKey, cField, lUserAllWinMoney);

	sprintf(cField, "use_sub_%d_%d", pMsg->iJudgeGameID, 202);
	RedisHelper::hincrby(m_redisCt, cKey, cField, lUserAllLoseMoney);

	sprintf(cField, "use_sub_%d_%d", pMsg->iJudgeGameID, 203);
	RedisHelper::hincrby(m_redisCt, cKey, cField, lUserAllTableMoney);

	sprintf(cField, "send_sub_%d_%d", pMsg->iJudgeGameID, 204);
	RedisHelper::hincrby(m_redisCt, cKey, cField, lAllExtraSendMoney);

	sprintf(cField, "send_sub_%d_%d", pMsg->iJudgeGameID, 205);
	RedisHelper::hincrby(m_redisCt, cKey, cField, lAllReliefMoney);

	sprintf(cField, "use_sub_%d_%d", pMsg->iJudgeGameID, 206);
	RedisHelper::hincrby(m_redisCt, cKey, cField, lAllExtraUseMoney);

	RedisHelper::hincrby(m_redisCt, cKey, "send_total", lAllSendMoney);
	RedisHelper::hincrby(m_redisCt, cKey, "use_total", lAllUseMoney);
	RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);

	//钻石报表(304-游戏额外发放钻石，305游戏额外回收钻石)
	sprintf(cKey, "MJ_PROP_%d_TOTAL_LOG:%d", 2, iTmFlag);
	sprintf(cField, "send_sub_%d_%d", pMsg->iJudgeGameID, 304);
	RedisHelper::hincrby(m_redisCt, cKey, cField, iAllSendDiamond);

	sprintf(cField, "use_sub_%d_%d", pMsg->iJudgeGameID, 305);
	RedisHelper::hincrby(m_redisCt, cKey, cField, iAllUseDiamond);

	RedisHelper::hincrby(m_redisCt, cKey, "send_total", iAllSendDiamond);
	RedisHelper::hincrby(m_redisCt, cKey, "use_total", iAllUseDiamond);
	RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
	

	//奖券报表(404-游戏额外发放奖券，405游戏额外回收奖券)
	sprintf(cKey, "MJ_PROP_%d_TOTAL_LOG:%d", 3, iTmFlag);

	sprintf(cField, "send_sub_%d_%d", pMsg->iJudgeGameID, 404);
	RedisHelper::hincrby(m_redisCt, cKey, cField, iAllSendIntegral);

	sprintf(cField, "use_sub_%d_%d", pMsg->iJudgeGameID, 405);
	RedisHelper::hincrby(m_redisCt, cKey, cField, iAllUseIntegral);

	RedisHelper::hincrby(m_redisCt, cKey, "send_total", iAllSendIntegral);
	RedisHelper::hincrby(m_redisCt, cKey, "use_total", iAllUseIntegral);
	RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
	
}

void EventLogHandler::HandleNCenterStatInfo(char* szMsg, int iLen)
{
	RDNCRecordStatInfoDef* pMsg = (RDNCRecordStatInfoDef*)szMsg;
	char cKey[64] = { 0 };
	time_t iTmFlag = Util::GetDayTimeFlag(pMsg->stAssignStaInfo.iTmFlag);
	char cFiled[32];
	_log(_DEBUG, "ELT", "HandleNCenterStatInfo iSendReq2&3=%d,%d,wait[%d,%d,%d],roomWait[%d,%d,%d,%d,%d]", pMsg->stAssignStaInfo.iSendReq2Num, pMsg->stAssignStaInfo.iSendReq3Num,
	pMsg->stAssignStaInfo.iHighUserCnt, pMsg->stAssignStaInfo.iMidUserCnt, pMsg->stAssignStaInfo.iLowUserCnt,
	pMsg->stAssignStaInfo.iRoomWaitCnt[0], pMsg->stAssignStaInfo.iRoomWaitCnt[1], pMsg->stAssignStaInfo.iRoomWaitCnt[2],
	pMsg->stAssignStaInfo.iRoomWaitCnt[3], pMsg->stAssignStaInfo.iRoomWaitCnt[4]);

	_log(_DEBUG, "ELT", "HandleNCenterStatInfo sameRoom[%d],rateCnt[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", pMsg->stAssignStaInfo.iRightRoomTableCnt, pMsg->stAssignStaInfo.iUserRateCnt[0],
		pMsg->stAssignStaInfo.iUserRateCnt[1], pMsg->stAssignStaInfo.iUserRateCnt[2], pMsg->stAssignStaInfo.iUserRateCnt[3], pMsg->stAssignStaInfo.iUserRateCnt[4],
		pMsg->stAssignStaInfo.iUserRateCnt[5],  pMsg->stAssignStaInfo.iUserRateCnt[6], pMsg->stAssignStaInfo.iUserRateCnt[7], pMsg->stAssignStaInfo.iUserRateCnt[8], 
		pMsg->stAssignStaInfo.iUserRateCnt[9], pMsg->stAssignStaInfo.iUserRateCnt[10]);

	sprintf(cKey, "mj_game_%d_assign_log:%d", pMsg->iGameID, iTmFlag);
	if (pMsg->stAssignStaInfo.iAbnormalSitReq > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "abnormal_sit_req", pMsg->stAssignStaInfo.iAbnormalSitReq);
	}
	if (pMsg->stAssignStaInfo.iAbnormalSitUser > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "abnormal_sit_user", pMsg->stAssignStaInfo.iAbnormalSitUser);
	}
	if (pMsg->stAssignStaInfo.iSendReq2Num > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "send_req_2_num", pMsg->stAssignStaInfo.iSendReq2Num);
	}
	if (pMsg->stAssignStaInfo.iSendReq3Num > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "send_req_3_num", pMsg->stAssignStaInfo.iSendReq3Num);
	}
	if (pMsg->stAssignStaInfo.iSendReqMoreNum > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "send_req_more_num", pMsg->stAssignStaInfo.iSendReqMoreNum);
	}
	if (pMsg->stAssignStaInfo.iNeedChangeNum > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "need_change_num", pMsg->stAssignStaInfo.iNeedChangeNum);
	}
	if (pMsg->stAssignStaInfo.iSucTableOkNum > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "suc_table_ok_num", pMsg->stAssignStaInfo.iSucTableOkNum);
	}
	for (int i = 0; i < 5; i++)
	{	
		sprintf(cFiled, "start_game_%d", i + 4);
		if (pMsg->stAssignStaInfo.iStartGameOk[i] > 0)
		{
			RedisHelper::hincrby(m_redisCt, cKey, cFiled, pMsg->stAssignStaInfo.iStartGameOk[i]);
		}
		sprintf(cFiled, "room_%d_wait_cnt", i + 4);
		if (pMsg->stAssignStaInfo.iRoomWaitCnt[i] > 0)
		{
			char cValue[32];
			sprintf(cValue, "%d", pMsg->stAssignStaInfo.iRoomWaitCnt[i]);
			RedisHelper::hset(m_redisCt, cKey, cFiled, cValue);
		}
	}
	if (pMsg->stAssignStaInfo.iHighUserCnt > 0)
	{
		char cValue[32];
		sprintf(cValue, "%d", pMsg->stAssignStaInfo.iHighUserCnt);
		RedisHelper::hset(m_redisCt, cKey, "high_user_cnt", cValue);
	}
	if (pMsg->stAssignStaInfo.iMidUserCnt > 0)
	{
		char cValue[32];
		sprintf(cValue, "%d", pMsg->stAssignStaInfo.iMidUserCnt);
		RedisHelper::hset(m_redisCt, cKey, "mid_user_cnt", cValue);
	}
	if (pMsg->stAssignStaInfo.iLowUserCnt > 0)
	{
		char cValue[32];
		sprintf(cValue, "%d", pMsg->stAssignStaInfo.iLowUserCnt);
		RedisHelper::hset(m_redisCt, cKey, "low_user_cnt", cValue);
	}
	if (pMsg->stAssignStaInfo.iRightSecTableCnt > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "right_sec_table_cnt", pMsg->stAssignStaInfo.iRightSecTableCnt);
	}
	if (pMsg->stAssignStaInfo.iRightRoomTableCnt > 0)
	{
		RedisHelper::hincrby(m_redisCt, cKey, "right_room_table_cnt", pMsg->stAssignStaInfo.iRightRoomTableCnt);
	}

	int iRateGap[9] = { 80,70,65,60,55,50,45,40,30 };
	for (int i = 0; i < 11; i++)
	{
		if (pMsg->stAssignStaInfo.iUserRateCnt[i] > 0)
		{
			if (i == 10)
			{
				RedisHelper::hincrby(m_redisCt, cKey, "enter_game_no_10", pMsg->stAssignStaInfo.iUserRateCnt[i]);
			}
			else if (i == 9)
			{
				RedisHelper::hincrby(m_redisCt, cKey, "enter_rate_min", pMsg->stAssignStaInfo.iUserRateCnt[i]);
			}
			else
			{
				sprintf(cFiled, "enter_rate_%d", iRateGap[i]);
				RedisHelper::hincrby(m_redisCt, cKey, cFiled, pMsg->stAssignStaInfo.iUserRateCnt[i]);
			}
		}
	}
	RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
}


void EventLogHandler::HandleGameComEventInfo(char* szMsg, int iLen)
{
	SRedisComStatMsgDef* pMsgReq = (SRedisComStatMsgDef*)szMsg;

	int iGameID = pMsgReq->iEventID >> 16;
	int iEventID = pMsgReq->iEventID & 0xffff;

	vector<ComStatOneEventInfo> vcEventInfo;
	for (int i = 0; i < 20; i++)
	{
		if (pMsgReq->iSubEventID[i] > 0 && pMsgReq->cllEventAddCnt[i] != 0)
		{
			ComStatOneEventInfo event;

			event.iSubEventID = pMsgReq->iSubEventID[i];
			event.llEventVal = pMsgReq->cllEventAddCnt[i];

			vcEventInfo.push_back(event);
		}
	}

	CallBackAddGameEvent(iGameID, iEventID, vcEventInfo);
}

void EventLogHandler::CallBackAddGameEvent(int iGameID, int iEventID, vector<ComStatOneEventInfo> vcEventInfo)
{
	//找下事件是否有配置并且已开启，没有的事件或已关闭的就不用记了,定时读取配置的时候已剔除了关闭的事件，无需判断是否开启
	map<int, vcComStatConf>::iterator pos = m_mapAllStatConf.find(iEventID);
	if (pos == m_mapAllStatConf.end())
	{
		_log(_ERROR, "ELH", "CallBackAddGameEvent iEvent[%d] game[%d] not conf", iEventID, iGameID);
		return;
	}

	int iDateFlag = Util::GetDayTimeFlag(time(NULL));
	char strKey[64];
	sprintf(strKey, "MJ_COM_STAT%d:%d", iEventID, iDateFlag);

	char strField[64];
	for (int i = 0; i < vcEventInfo.size(); i++)
	{		
		int iSubEventID = vcEventInfo[i].iSubEventID;
		long long llEventVal = vcEventInfo[i].llEventVal;

		bool bFind = false;
		for (int j = 0; j < pos->second.size(); j++)
		{
			if (pos->second[j].iSubEventID == iSubEventID)
			{
				bFind = true;
				break;
			}
		}
		if (bFind)
		{
			sprintf(strField, "event_%d", iSubEventID);
			long long lRt = RedisHelper::hincrby(m_redisCt, strKey, strField, llEventVal);
			_log(_DEBUG, "ELH", "CallBackAddGameEvent iEvent[%d] iGameID[%d] sub[%d] add[%lld],rt[%lld]", iEventID, iGameID, iSubEventID, llEventVal, lRt);
		}
		else
		{
			_log(_DEBUG, "ELH", "CallBackAddGameEvent iEvent[%d] iGameID[%d] sub[%d] not find!!!", iEventID, iGameID, iSubEventID);
		}
	}
	RedisHelper::expire(m_redisCt, strKey, 35 * 3600 * 24);
}

void EventLogHandler::HandleUsePropStatInfo(char* szMsg, int iLen)
{
	SUsePropStatMsgDef* pMsgReq = (SUsePropStatMsgDef*)szMsg;
	int iRoomType = pMsgReq->iRoomType & 0xf;
	int iGameID = pMsgReq->iRoomType >> 4;
	int iDateFlag = Util::GetDayTimeFlag(time(NULL));
	//记录到道具报表
	/*道具总使用	100
		新手房使用道具	104
		初级房使用道具	105
		中级房使用道具	106
		高级房使用道具	107
		vip房使用道具	108
		道具总发放	110
		新手房发放道具	114
		初级房发放道具	115
		中级房发放道具	116
		高级房发放道具	117
		vip房发放道具	118*/
	char strKey[64];
	char strField[64];
	for (int i = 0; i < 20; i++)
	{
		if (pMsgReq->iPropNum[i] == 0)
		{
			continue;
		}
		sprintf(strKey, "MJ_PROP_%d_TOTAL_LOG:%d", pMsgReq->iPropID[i],iDateFlag);
		if (pMsgReq->iPropID[i] > 0)//消耗道具
		{
			sprintf(strField, "use_sub_%d_%d", iGameID, 100+iRoomType);
			RedisHelper::hincrby(m_redisCt, strKey, strField, pMsgReq->iPropNum[i]);

			sprintf(strField, "use_sub_%d_%d", iGameID, 100);
			RedisHelper::hincrby(m_redisCt, strKey, strField, pMsgReq->iPropNum[i]);

			RedisHelper::hincrby(m_redisCt, strKey, "use_total", pMsgReq->iPropNum[i]);
		}
		else if (pMsgReq->iPropID[i] < 0)//发放道具
		{
			sprintf(strField, "send_sub_%d_%d", iGameID, 110 + iRoomType);
			RedisHelper::hincrby(m_redisCt, strKey, strField, pMsgReq->iPropNum[i]);

			sprintf(strField, "send_sub_%d_%d", iGameID, 110);
			RedisHelper::hincrby(m_redisCt, strKey, strField, pMsgReq->iPropNum[i]);

			RedisHelper::hincrby(m_redisCt, strKey, "send_total", pMsgReq->iPropNum[i]);
		}
		RedisHelper::expire(m_redisCt, strKey, 35 * 3600 * 24);
	}
	
}

void EventLogHandler::HandleAssignTmStat(char * szMsg, int iLen)
{
	RDNCAssignTmStatDef* pMsg = (RDNCAssignTmStatDef*)szMsg;

	char cKey[64] = { 0 };
	time_t iTmFlag = Util::GetDayTimeFlag(time(NULL));
	char cFiled[32];
	sprintf(cKey, "mj_game_%d_assign_log:%d", pMsg->iGameID, iTmFlag);
	int* pIntMove = (int*)(szMsg + sizeof(RDNCAssignTmStatDef));
	for (int i = 0; i < pMsg->iSubNum; i++)
	{
		int iExtraType = *pIntMove;
		pIntMove++;
		int iNewCnt = *pIntMove;
		pIntMove++;
		int iNewAverage = *pIntMove;
		pIntMove++;

		sprintf(cFiled, "assignTm_%d_cnt", iExtraType);
		string strCur = RedisHelper::hget(m_redisCt, cKey, cFiled);
		int iCntVal = 0;
		if (strCur.length() > 0)
		{
			iCntVal = atoi(strCur.c_str());
		}
		int iRembCnt = iCntVal + iNewCnt;
		if (iRembCnt == 0)
		{
			continue;
		}
		RedisHelper::hset(m_redisCt, cKey, cFiled, to_string(iRembCnt));
		int iAverageVal = 0;
		sprintf(cFiled, "assignTm_%d_tm", iExtraType);
		strCur = RedisHelper::hget(m_redisCt, cKey, cFiled);
		if (strCur.length() > 0)
		{
			iAverageVal = atoi(strCur.c_str());
		}
		if (iNewCnt + iCntVal > 0 && iNewAverage + iAverageVal > 0)
		{
			long long lNewData = iNewCnt*iNewAverage;
			long long lOldData = iCntVal*iAverageVal;
			int iRembAverage = lNewData / iRembCnt + lOldData / iRembCnt;   //防止计算越界，这里计算平均值，存在一些误差
			RedisHelper::hset(m_redisCt, cKey, cFiled, to_string(iRembAverage));
		}
	}
	RedisHelper::expire(m_redisCt, cKey, 35 * 3600 * 24);
}

void EventLogHandler::CheckGameEvent(ComStatSubConfDef* pStatConf, map<int, vcComStatConf>& mapStatConf)
{
	map<int, vcComStatConf>::iterator pos = mapStatConf.find(pStatConf->iEventID);
	if (pos == mapStatConf.end())
	{
		vcComStatConf vcConf;
		ComStatSubConfDef stOneConf;
		memset(&stOneConf,0,sizeof(ComStatSubConfDef));
		memcpy(&stOneConf, pStatConf, sizeof(ComStatSubConfDef));
		vcConf.push_back(stOneConf);
		mapStatConf[stOneConf.iEventID] = vcConf;
		return;
	}
	bool bFind = false;
	for (int i = 0; i < pos->second.size(); i++)
	{
		if (pos->second[i].iSubEventID == pStatConf->iSubEventID)
		{
			if (pStatConf->iIfStart == 0)
			{
				pos->second.erase(pos->second.begin()+i);
			}
			else
			{
				pos->second[i].iSubEventType = pStatConf->iSubEventType;
				pos->second[i].iSubEventGapTime = pStatConf->iSubEventGapTime;	
				pos->second[i].iIfStart = pStatConf->iIfStart;
			}
			bFind = true;
			break;
		}
	}
	if (!bFind)
	{
		ComStatSubConfDef stOneConf;
		memset(&stOneConf, 0, sizeof(ComStatSubConfDef));
		memcpy(&stOneConf, pStatConf, sizeof(ComStatSubConfDef));
		pos->second.push_back(stOneConf);
	}
}

void EventLogHandler::HandleReportCommonEventInfo(char* szMsg, int iLen)
{
	SRedisCommonEventMsgDef* pMsgReq = (SRedisCommonEventMsgDef*)szMsg;

	_log(_DEBUG, "ELH", "HandleReportCommonEventInfo iEventItemNum[%d]", pMsgReq->iEventItemNum);

	char strKey[64];
	char strField[64];

	int iMsgLen = sizeof(SRedisCommonEventMsgDef);
	for (int i = 0; i < pMsgReq->iEventItemNum; i++)
	{
		CommonEvent* stCommonEvent = (CommonEvent*)(szMsg + iMsgLen);

		iMsgLen += sizeof(CommonEvent);

		map<int, vcComStatConf>::iterator pos = m_mapAllStatConf.find(stCommonEvent->iMainEventID);	//找下事件是否有配置并且已开启，没有的事件或已关闭的就不用记了
		if (pos == m_mapAllStatConf.end())
		{
			_log(_DEBUG, "ELH", "HandleReportCommonEventInfo find err iMainEventID[%d]", stCommonEvent->iMainEventID);
			continue;
		}		

		bool bFindOK = false;
		for (int j = 0; j < pos->second.size(); j++)
		{
			if (pos->second[j].iSubEventID == stCommonEvent->iSubEventID)
			{
				bFindOK = true;
				break;
			}
		}

		if (bFindOK)
		{
			int iDateNum = stCommonEvent->iDateNum;
			if (iDateNum == 0)
			{
				iDateNum = Util::GetDayTimeFlag(time(NULL));	//没指定日期 就用当日的
			}

			sprintf(strKey, "MJ_COM_STAT%d:%d", stCommonEvent->iMainEventID, iDateNum);

			sprintf(strField, "event_%d", stCommonEvent->iSubEventID);
			long long lRt = RedisHelper::hincrby(m_redisCt, strKey, strField, stCommonEvent->iEventNum);

			RedisHelper::expire(m_redisCt, strKey, 35 * 3600 * 24);

			_log(_DEBUG, "ELH", "HandleReportCommonEventInfo date[%d] iGameId[%d] mainId[%d] subId[%d] num[%d]", iDateNum, stCommonEvent->iGameID, stCommonEvent->iMainEventID, stCommonEvent->iSubEventID, stCommonEvent->iEventNum);
		}
		else
		{
			_log(_DEBUG, "ELH", "HandleReportCommonEventInfo find err iMainEventID[%d] iSubEventID[%d]", stCommonEvent->iMainEventID, stCommonEvent->iSubEventID);
		}
	}
}