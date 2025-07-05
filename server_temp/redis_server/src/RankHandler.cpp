#include "RankHandler.h"
#include "RedisHelper.h"
#include <time.h>
#include "Util.h"
#include "log.h"
#include "ServerLogicThread.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include "conf.h"
#include <algorithm>
#include "SQLWrapper.h"

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/

RankHandler::RankHandler(MsgQueue *pSendQueue, MsgQueue *pLogicQueue, MsgQueue *pEventQueue)
{
	m_pSendQueue = pSendQueue;
	m_pLogicQueue = pLogicQueue;
	m_pEventQueue = pEventQueue;
}

void RankHandler::HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex)
{
	if (m_redisCt == NULL)
	{
		_log(_ERROR, "RH::HandleMsg", "error m_redisCt == NULL");
		return;
	}

	_log(_DEBUG, "RH", "RankHandler HandleMsg iMsgType[0x%x] iLen[%d] iSocketIndex[%d]", iMsgType, iLen, iSocketIndex);

	if (RD_GET_RANK_CONF_INFO_MSG == iMsgType)
	{
		HandleGetRankConfInfoMsg(szMsg, iLen, iSocketIndex);
	}
	else if (RD_GET_USER_RANK_INFO_MSG == iMsgType)
	{
		HandleGetUserRankInfoMsg(szMsg, iLen, iSocketIndex);
	}
	else if (RD_UPDATE_USER_RANK_INFO_MSG == iMsgType)
	{
		HandleUpdateUserRankInfoMsg(szMsg, iLen, iSocketIndex);
	}
	else if (REDIS_GET_PARAMS_MSG == iMsgType)
	{
		HandleGetParamsMsg(szMsg, iSocketIndex);
	}
}

void RankHandler::CallBackNextDay(int iTimeBeforeFlag, int iTimeNowFlag)
{
}

void RankHandler::CallBackOnTimer(int iTimeNow)
{
	static int iLastUpdateRankConf = 0;
		
	iLastUpdateRankConf--;
	if (iLastUpdateRankConf < 0)
	{
		iLastUpdateRankConf = 10 * 60;

		OnTimerGetRankConfInfo();
		OnTimerGetParamsKeyValue();
	}

	if (stSysConfigBaseInfo.iIfMainRedis == 1)
	{
		OnTimerHandleWaitRankUser();
		OnTimerCheckRankIfEnd();
		OnTimerCheckWaitEndInfo();
		OnTimerWriteRKHistoryData();
	}
}

void RankHandler::OnTimerGetRankConfInfo()
{
	CSQLWrapperSelect hSQLSelect("rank_activity_conf", m_pMySqlGame->GetMYSQLHandle());
	hSQLSelect.AddQueryField("RankId");
	hSQLSelect.AddQueryField("RankMode");
	hSQLSelect.AddQueryField("BeginTime");
	hSQLSelect.AddQueryField("EndTime");
	hSQLSelect.AddQueryField("MaxRankNum");
	hSQLSelect.AddQueryField("JifenConf");
	hSQLSelect.AddQueryField("BuffExtra");
	hSQLSelect.AddQueryField("RankAward");
	hSQLSelect.AddQueryField("rank_time_type");
	hSQLSelect.AddQueryConditionNumber("IfStart", 1, CSQLWrapper::ERelationType::Equal);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlGame->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "RH", "OnTimerGetRankConfInfo error");
	}
	else
	{
		m_mapRankInfo.clear();

		map<int, DKRankConfigDef> mapRankInfoNew;	//新排行配置信息
		const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			DKRankConfigDef conf;
			conf.clear();
			conf.iRankId = hRecord.GetFieldAsInt32("RankId");
			conf.iRankMode = hRecord.GetFieldAsInt32("RankMode");
			conf.iBeginTime = Util::ConvertTimeStamp((char*)hRecord.GetFieldAsString("BeginTime").c_str());
			conf.iEndTime = Util::ConvertTimeStamp((char*)hRecord.GetFieldAsString("EndTime").c_str());
			conf.iMaxRankNum = hRecord.GetFieldAsInt32("MaxRankNum");
			strcpy(conf.szJifenConf, hRecord.GetFieldAsString("JifenConf").c_str());
			strcpy(conf.szBuffExtra, hRecord.GetFieldAsString("BuffExtra").c_str());
			conf.iTimeType = hRecord.GetFieldAsInt32("rank_time_type");

			char szRankAward[256] = { 0 };
			strcpy(szRankAward, hRecord.GetFieldAsString("RankAward").c_str());

			std::vector<std::string> strRankAward;
			Util::CutString(strRankAward, szRankAward, "&");		//格式：1_1#1_10000#2_200#50_10&2_5#1_5000#2_100#50_5

			for (int i = 0; i < strRankAward.size(); i++)
			{
				std::vector<std::string> strAwardTemp;
				Util::CutString(strAwardTemp, strRankAward.at(i).c_str(), "#");		//格式：1_1#1_10000#2_200#50_10

				DKRankAwardDef stRankAward;

				int iValue1, iValue2;
				for (int j = 0; j < strAwardTemp.size(); j++)
				{
					int rt = sscanf(strAwardTemp.at(j).c_str(), "%d_%d", &iValue1, &iValue2);
					if (rt == 2)
					{
						if (j == 0)
						{
							stRankAward.iMinRankNum = iValue1;
							stRankAward.iMaxRankNum = iValue2;
						}
						else
						{
							PropInfoDef prop;
							prop.iPropId = iValue1;
							prop.iPropNum = iValue2;

							stRankAward.vcAwardInfo.push_back(prop);
						}
					}
				}

				conf.allRankAward.push_back(stRankAward);
			}

			m_mapRankInfo[conf.iRankId] = conf;
			
			_log(_ERROR, "RH", "OnTimerGetRankConfInfo num[%d] rid[%d] begin[%d] end[%d] maxNum[%d]", \
				m_mapRankInfo.size(), conf.iRankId, conf.iBeginTime, conf.iEndTime, conf.iMaxRankNum);

			_log(_ERROR, "RH", "OnTimerGetRankConfInfo jifen[%s] extra[%s] awardNum[%d]", \
				conf.szJifenConf, conf.szBuffExtra, conf.allRankAward.size());
		}
	}
}

DKRankConfigDef* RankHandler::findRankConfInfo(int iRankId)
{
	map<int, DKRankConfigDef>::iterator pos = m_mapRankInfo.find(iRankId);
	if (pos != m_mapRankInfo.end())
	{
		return &pos->second;
	}

	return NULL;
}

void RankHandler::HandleGetRankConfInfoMsg(char* msgData, int iLen, int iSocketIndex)
{
	RdGetRankConfInfoReqDef* msg = (RdGetRankConfInfoReqDef*)msgData;

	_log(_ERROR, "RH", "HandleGetRankConfInfoMsg iRankId[%d]", msg->iRankId);

	char szBuffer[1024] = { 0 };

	RdGetRankConfInfoResDef* msgRes = (RdGetRankConfInfoResDef*)szBuffer;

	msgRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes->msgHeadInfo.iMsgType = RD_GET_RANK_CONF_INFO_MSG;	
	msgRes->msgHeadInfo.iSocketIndex = iSocketIndex;

	msgRes->iRankId = msg->iRankId;

	int iMsgLen = sizeof(RdGetRankConfInfoResDef);

	DKRankConfigDef* config = findRankConfInfo(msg->iRankId);
	if (config != NULL)
	{	
		msgRes->iBeginTm = config->iBeginTime;
		msgRes->iEndTm = config->iEndTime;
		msgRes->iJifenLen = strlen(config->szJifenConf);
		msgRes->iExtraLen = strlen(config->szBuffExtra);

		if (msgRes->iJifenLen > 0)
		{
			memcpy(szBuffer + iMsgLen, config->szJifenConf, msgRes->iJifenLen);
			iMsgLen += msgRes->iJifenLen;
		}
		if (msgRes->iExtraLen > 0)
		{
			memcpy(szBuffer + iMsgLen, config->szBuffExtra, msgRes->iExtraLen);
			iMsgLen += msgRes->iExtraLen;
		}
	}

	if (m_pSendQueue)
	{
		m_pSendQueue->EnQueue(szBuffer, iMsgLen);
	}
}

void RankHandler::HandleGetUserRankInfoMsg(char* msgData, int iLen, int iSocketIndex)
{
	RdGetUserRankInfoReq* msg = (RdGetUserRankInfoReq*)msgData;

	_log(_ERROR, "RH", "HandleGetUserRankInfoMsg iUserId[%d] iRankId[%d]", msg->iUserId, msg->iRankId);
	
	DKRankConfigDef* config = findRankConfInfo(msg->iRankId);
	if (config != NULL)
	{
		int iPeriods = GetRankPeriodID(msg->iRankId, time(NULL));

		_log(_ERROR, "RH", "HandleGetUserRankInfoMsg iUserId[%d] iRankId[%d] iPeriods[%d]", msg->iUserId, msg->iRankId, iPeriods);		

		char szBuffer[1024] = { 0 };
		RdGetUserRankInfoResDef *msgRes = (RdGetUserRankInfoResDef*)szBuffer;

		msgRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgRes->msgHeadInfo.iMsgType = RD_GET_USER_RANK_INFO_MSG;
		msgRes->msgHeadInfo.iSocketIndex = iSocketIndex;
		msgRes->iUserId = msg->iUserId;
		msgRes->iRankId = msg->iRankId;

		int iMsgLen = sizeof(RdGetUserRankInfoResDef);

		RankUserInfoDef userInfo = GetRankUserInfo(msg->iUserId, msg->iRankId, iPeriods);
		msgRes->iJifen = userInfo.iJifenNum;

		if (userInfo.strExtra.length() > 0)
		{
			msgRes->iExtraLen = userInfo.strExtra.length();
			memcpy(szBuffer + iMsgLen, userInfo.strExtra.c_str(), userInfo.strExtra.length());

			iMsgLen += userInfo.strExtra.length();
		}

		if (m_pSendQueue)
		{
			m_pSendQueue->EnQueue(szBuffer, iMsgLen);
		}
	}
}

void RankHandler::HandleUpdateUserRankInfoMsg(char* msgData, int iLen, int iSocketIndex)
{
	RdUpdateUserRankInfoReq* msg = (RdUpdateUserRankInfoReq*)msgData;

	_log(_ERROR, "RH", "HandleUpdateUserRankInfoMsg iUserId[%d] Nick[%s] iRankID[%d] addJifen[%d]", msg->iUserID, msg->szNickName, msg->iRankID, msg->iAddJifen);

	RdUpdateUserRankInfoResDef msgRes;
	memset(&msgRes, 0, sizeof(RdUpdateUserRankInfoResDef));

	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = RD_UPDATE_USER_RANK_INFO_MSG;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	msgRes.iUserId = msg->iUserID;
	msgRes.iRankId = msg->iRankID;

	int iMsgLen = sizeof(RdUpdateUserRankInfoResDef);

	DKRankConfigDef* config = findRankConfInfo(msg->iRankID);
	if (config != NULL)
	{
		time_t tmNow = time(NULL);
		if (tmNow >= config->iBeginTime && tmNow < config->iEndTime)
		{
			int iPeriodID = GetRankPeriodID(msg->iRankID, tmNow);			

			RankUserInfoDef userInfo = GetRankUserInfo(msg->iUserID, msg->iRankID, iPeriodID);

			int iJifenNum = userInfo.iJifenNum;	//当前积分

			iJifenNum += msg->iAddJifen;
			iJifenNum = max(iJifenNum, 0);	//不能小于0

			msgRes.iJifen = iJifenNum;

			char szExtra[128] = { 0 };
			if (msg->iExtraLen > 0)
			{
				memcpy(szExtra, msgData + sizeof(RdUpdateUserRankInfoReq), msg->iExtraLen);
			}

			char szValue[256] = { 0 };	//格式：jifen_headimage_headframeid_extra_nick
			sprintf(szValue, "%d_%d_%d_%s_%s", iJifenNum, msg->iHeadImage, msg->iHeadFrameId, szExtra, msg->szNickName);

			SetRankUserInfo(msg->iUserID, config->iRankId, iPeriodID, szValue, 30 * 24 * 3600);	//保存30天

			_log(_ERROR, "RH", "HandleUpdateUserRankInfoMsg iUserID[%d] iRankID[%d] iPeriodID[%d] iJifenNum[%d]", msg->iUserID, msg->iRankID, iPeriodID, iJifenNum);

			//更新当前排行榜单
			if (config->iRankMode == RankHandler::RankMode::INC_DEC)
			{
				char strKey[64] = { 0 };
				sprintf(strKey, KEY_USER_RANK_LIST1.c_str(), msg->iRankID, iPeriodID);

				bool bExist = RedisHelper::exists(m_redisCt, strKey);

				char szMember[256] = { 0 };
				sprintf(szMember, "%d", msg->iUserID);

				time_t timeN = time(NULL);
				time_t timeW = Util::GetWeekStartTimestamp();

				//本周剩余的时间
				time_t timeL = (7 * 24 * 3600) - (timeN - timeW);

				double dJifen = (double)iJifenNum + (double)timeL / 1000000.0;	//score=-jifen.time

				_log(_ERROR, "RH", "HandleUpdateUserRankInfoMsg iUserID[%d] iRankID[%d] dJifen[%lf]", msg->iUserID, msg->iRankID, dJifen);

				RedisHelper::zadddouble(m_redisCt, strKey, -dJifen, szMember);

				if (!bExist)
				{
					RedisHelper::expire(m_redisCt, strKey, 30 * 24 * 3600);	//30天
				}
			}
			else if (config->iRankMode == RankHandler::RankMode::ONLY_ADD)
			{

			}
		}
	}

	if (m_pSendQueue)
	{
		m_pSendQueue->EnQueue(&msgRes, iMsgLen);
	}
}

void RankHandler::OnTimerCheckRankIfEnd()
{
	time_t tmNow = time(NULL);

	static time_t tmLast = tmNow;
	
	map<int, DKRankConfigDef>::iterator iter = m_mapRankInfo.begin();
	while (iter != m_mapRankInfo.end())
	{
		int iPeriodIdOld = GetRankPeriodID(iter->second.iRankId, tmLast);
		int iPeriodIdNew = GetRankPeriodID(iter->second.iRankId, tmNow);
		if (iPeriodIdNew > iPeriodIdOld)	//本期排行结束了
		{
			DKWaitEndRankInfoDef waitEndInfo;
			waitEndInfo.clear();
			waitEndInfo.iRankId = iter->second.iRankId;
			waitEndInfo.iRankMode = iter->second.iRankMode;
			waitEndInfo.iPeriods = iPeriodIdOld;
			waitEndInfo.iMaxPlayer = iter->second.iMaxRankNum;
			waitEndInfo.allRankAward.assign(iter->second.allRankAward.begin(), iter->second.allRankAward.end());
			waitEndInfo.iEndTime = tmNow + 30;	//刚刚结束先不要结算 预留30秒时间 再结算
			m_mapWaitEndRank[waitEndInfo.iRankId] = waitEndInfo;

			_log(_ERROR, "RH", "OnTimerCheckRankIfEnd iRankId[%d] iPeriodIdOld[%d] iPeriodIdNew[%d] tmLast[%d] tmNow[%d]", iter->second.iRankId, iPeriodIdOld, iPeriodIdNew, tmLast, tmNow);
		}

		iter++;
	}

	tmLast = tmNow;
}

void RankHandler::OnTimerCheckWaitEndInfo()
{	
	time_t tmNow = time(NULL);

	map<int, DKWaitEndRankInfoDef>::iterator iter = m_mapWaitEndRank.begin();
	while (iter != m_mapWaitEndRank.end())
	{
		if (tmNow >= iter->second.iEndTime)
		{	
			char szValue[32] = { 0 };

			if (iter->second.iRankMode == RankHandler::RankMode::ONLY_ADD)
			{				
				HandleWaitRankUser(iter->second.iRankId, iter->second.iPeriods);	//排行结算前 再处理一次待排行信息
			}

			vector<RankUserInfoDef> vcRankUser = GetNowRankUserList(iter->second.iRankId, iter->second.iPeriods);
			for (int i = 0; i < vcRankUser.size(); i++)
			{
				DKRankHistory stHis;
				stHis.clear();
				stHis.iRankId = iter->second.iRankId;
				stHis.iPeriods = iter->second.iPeriods;
				stHis.iUserID = vcRankUser[i].iUserID;
				stHis.iRankNum = vcRankUser[i].iRankNum;
				stHis.iJifenNum = vcRankUser[i].iJifenNum;
				stHis.strAwardInfo1 = "";
				stHis.strAwardInfo2 = "";

				//排行奖励配置
				vector<DKRankAward> allRankAward = iter->second.allRankAward;
				for (int j = 0; j < allRankAward.size(); j++)
				{
					if (stHis.iRankNum >= allRankAward.at(j).iMinRankNum && stHis.iRankNum <= allRankAward.at(j).iMaxRankNum)
					{
						for (int k = 0; k < allRankAward.at(j).vcAwardInfo.size(); k++)
						{
							sprintf(szValue, "%d_%d", allRankAward.at(j).vcAwardInfo.at(k).iPropId, allRankAward.at(j).vcAwardInfo.at(k).iPropNum);
							if (k == 0)
							{
								stHis.strAwardInfo1 = szValue;
							}
							else if (k == 1)
							{
								stHis.strAwardInfo2 = szValue;
							}
							else
							{
								break;
							}
						}
					}
				}

				//上榜的玩家可能没配奖励 不要发放奖励资格了
				if (!stHis.strAwardInfo1.empty())
				{
					m_vcWaitHis.push_back(stHis);
				}
			}			

			iter = m_mapWaitEndRank.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void RankHandler::OnTimerWriteRKHistoryData()
{
	vector<DKRankHistoryDef> vcRKHis;

	int iHandleCnt = 0;
	while (!m_vcWaitHis.empty())
	{		
		vcRKHis.push_back(m_vcWaitHis.at(0));
		m_vcWaitHis.erase(m_vcWaitHis.begin());

		iHandleCnt++;
		if (iHandleCnt >= 30)	//一次最多处理30条
		{
			break;
		}
	}

	if (!vcRKHis.empty())
	{
		RecordUserRankHistoryInfo(vcRKHis);
	}
}

void RankHandler::RecordUserRankHistoryInfo(vector<DKRankHistoryDef> vcRKHis)
{
	int tmNow = time(NULL);
	int expTime = tmNow + 15 * 24 * 3600;	//15天有效期
	for (int i = 0; i < vcRKHis.size(); i++)
	{
		CSQLWrapperInsert hSQLInsert("user_rank_history", m_pMySqlUser->GetMYSQLHandle());
		hSQLInsert.SetFieldValueNumber("Userid", vcRKHis.at(i).iUserID);
		hSQLInsert.SetFieldValueNumber("Randkid", vcRKHis.at(i).iRankId);
		hSQLInsert.SetFieldValueNumber("PeriodNum", vcRKHis.at(i).iPeriods);
		hSQLInsert.SetFieldValueNumber("RankNum", vcRKHis.at(i).iRankNum);
		hSQLInsert.SetFieldValueNumber("JifenNum", vcRKHis.at(i).iJifenNum);
		hSQLInsert.SetFieldValueString("AwardInfo1", vcRKHis.at(i).strAwardInfo1);
		hSQLInsert.SetFieldValueString("AwardInfo2", vcRKHis.at(i).strAwardInfo2);
		hSQLInsert.SetFieldValueTimestamp("ExpirateTime", expTime);

		const std::string& strSQLInsert = hSQLInsert.GetFinalSQLString();
		m_pMySqlUser->RunSQL(strSQLInsert, true);

		_log(_ERROR, "RH", "RecordUserRankHistoryInfo iRankNum[%d] iUserID[%d] iRankId[%d] iPeriods[%d] iJifenNum[%d] AwardInfo1[%s]", vcRKHis.at(i).iRankNum, \
			vcRKHis.at(i).iUserID, vcRKHis.at(i).iRankId, vcRKHis.at(i).iPeriods, vcRKHis.at(i).iJifenNum, vcRKHis.at(i).strAwardInfo1.c_str());
	}
}

void RankHandler::AddComEvent(int iGameID, int iMainID, int iSubID, int iAddNum)
{
	SRedisComStatMsgDef comEvent;
	memset(&comEvent, 0, sizeof(SRedisComStatMsgDef));
	comEvent.msgHeadInfo.iMsgType = RD_REPORT_COMMON_EVENT_MSG;
	comEvent.msgHeadInfo.cVersion = MESSAGE_VERSION;
	comEvent.cllEventAddCnt[0] = iAddNum;
	comEvent.iEventID = iMainID + (iGameID << 16);
	comEvent.iSubEventID[0] = iSubID;
	int iRes = m_pEventQueue->EnQueue((void*)&comEvent, sizeof(SRedisComStatMsgDef));
	if (iRes != 0)
	{
		_log(_ERROR, "EnQueue_log", "AddComEvent iGameID[%d] MainID[%d] iSubID[%d] iAddNum[%d]", iGameID, iMainID, iSubID, iAddNum);
	}
	
}

RankUserInfoDef RankHandler::GetRankUserInfo(int iUserID, int iRankID, int iPeriodID)
{
	RankUserInfoDef stUserInfo;
	stUserInfo.clear();

	stUserInfo.iUserID = iUserID;
	stUserInfo.iRankID = iRankID;
	stUserInfo.iPeriodID = iPeriodID;

	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_RANK_INFO.c_str(), iRankID, iPeriodID);

	char strField[64] = { 0 };
	sprintf(strField, "%d", iUserID);

	string strRt = RedisHelper::hget(m_redisCt, strKey, strField);		//格式：jifen_headimage_headframeid_extra_nick
	if (!strRt.empty())
	{
		std::vector<std::string> vcVal;
		Util::CutString(vcVal, strRt, "_");
		if (vcVal.size() >= 5)
		{
			stUserInfo.iJifenNum = atoi(vcVal.at(0).c_str());
			stUserInfo.iHeadImg = atoi(vcVal.at(1).c_str());
			stUserInfo.iHeadFrameId = atoi(vcVal.at(2).c_str());
			stUserInfo.strExtra = vcVal.at(3);					
			stUserInfo.strNick = "";			
			for (int i = 4; i < vcVal.size(); i++)	//Nick可能带着下划线 特殊处理一下
			{
				if (i > 4)
				{
					stUserInfo.strNick += "_";
				}
				stUserInfo.strNick += vcVal.at(i);
			}
		}
	}

	_log(_ERROR, "RH", "GetRankUserInfo iUserID[%d] iRankID[%d] iPeriodID[%d] [%d][%d][%d] strExtra[%s]", stUserInfo.iUserID, stUserInfo.iRankID, stUserInfo.iPeriodID, stUserInfo.iJifenNum, stUserInfo.iHeadImg, stUserInfo.iHeadFrameId, stUserInfo.strExtra.c_str());

	return stUserInfo;
}

void RankHandler::SetRankUserInfo(int iUserID, int iRankID, int iPeriodID, string strVal, int expire)
{
	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_RANK_INFO.c_str(), iRankID, iPeriodID);

	char strField[64] = { 0 };
	sprintf(strField, "%d", iUserID);

	bool bExist = RedisHelper::exists(m_redisCt, strKey);

	RedisHelper::hset(m_redisCt, strKey, strField, strVal);

	if (!bExist)
	{
		RedisHelper::expire(m_redisCt, strKey, expire);
	}
}

MinUserInfoDef RankHandler::GetRankMinUserInfo(int iRankID, int iPeriodID)
{
	MinUserInfoDef minUser;
	minUser.clear();
	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_RANK_MIN_INFO.c_str(), iRankID, iPeriodID);

	string strRt = RedisHelper::get(m_redisCt, strKey);	//value:jifen_extra   -- 积分_额外信息
	if (strRt.length() > 0)
	{
		std::vector<std::string> vcVal;
		Util::CutString(vcVal, strRt, "_");
		if (vcVal.size() >= 2)
		{
			minUser.iJifenNum = atoi(vcVal.at(0).c_str());
			minUser.strExtra = vcVal.at(1);
		}
	}

	return minUser;
}

void RankHandler::SetRankMinUserInfo(int iRankID, int iPeriodID, string strVal, int expire)
{
	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_RANK_MIN_INFO.c_str(), iRankID, iPeriodID);

	bool bExist = RedisHelper::exists(m_redisCt, strKey);

	RedisHelper::set(m_redisCt, strKey, strVal);	//value:jifen_extra   -- 积分_额外信息

	if (!bExist)
	{
		RedisHelper::expire(m_redisCt, strKey, expire);
	}
}

void RankHandler::addWaitRankUserInfo(int iRankID, int iPeriodID, int iUserID, string strVal, int expire)
{
	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_WAIT_RANK.c_str(), iRankID, iPeriodID);

	char strField[64] = { 0 };
	sprintf(strField, "%d", iUserID);

	bool bExist = RedisHelper::exists(m_redisCt, strKey);

	RedisHelper::hset(m_redisCt, strKey, strField, strVal);

	if (!bExist)
	{
		RedisHelper::expire(m_redisCt, strKey, expire);
	}

	_log(_ERROR, "RH", "addWaitRankUserInfo iRankID[%d] iPeriodID[%d] iUserID[%d] strVal[%s]", iRankID, iPeriodID, iUserID, strVal.c_str());
}

int RankHandler::GetWaitRankListNum(int iRankID, int iPeriodID)
{
	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_WAIT_RANK.c_str(), iRankID, iPeriodID);

	int iItemNum = RedisHelper::hlen(m_redisCt, strKey);

	return iItemNum;
}

void RankHandler::OnTimerHandleWaitRankUser()
{
	int tmNow = time(NULL);

	map<int, DKRankConfigDef>::iterator iter = m_mapRankInfo.begin();
	while (iter != m_mapRankInfo.end())
	{
		if (iter->second.iRankMode == RankHandler::RankMode::ONLY_ADD)
		{
			int iRankID = iter->second.iRankId;
			int iPeriodID = GetRankPeriodID(iRankID, tmNow);

			bool bNeedRank = false;		//是否需要重新排行

			int tmLast = GetLastWaitRankTime(iRankID);
			if (tmNow - tmLast > 5 * 60)	 //5min排行一次
			{
				bNeedRank = true;
			}
			else
			{
				int iItemNum = GetWaitRankListNum(iRankID, iPeriodID);
				if (iItemNum > 500)		//数据超过500 也排行一次
				{
					bNeedRank = true;
				}
			}

			if (bNeedRank)
			{
				HandleWaitRankUser(iRankID, iPeriodID);
			}
		}

		iter++;
	}
}

bool SortRankUser(RankUserInfoDef first, RankUserInfoDef second)
{
	if (first.iJifenNum > second.iJifenNum)
	{
		return true;
	}

	if ((first.iJifenNum == second.iJifenNum) && (first.iLastTime < second.iLastTime))
	{
		return true;
	}

	return false;
}

void RankHandler::HandleWaitRankUser(int iRankID, int iPeriodID)
{
	DKRankConfigDef* config = findRankConfInfo(iRankID);
	if (config == NULL || config->iRankMode != RankHandler::RankMode::ONLY_ADD)
	{
		return;
	}	

	int tmNow = time(NULL);

	//取出待处理排行信息 重新排行一次
	vector<RankUserInfoDef> vcRankUserWait = GetWaitRankUserList(iRankID, iPeriodID);
	if (vcRankUserWait.size() > 0)
	{
		vector<RankUserInfoDef> vcRankUserNow = GetNowRankUserList(iRankID, iPeriodID);
		for (int i = 0; i < vcRankUserWait.size(); i++)
		{
			bool bFindOK = false;
			for (int j = 0; j < vcRankUserNow.size(); j++)
			{
				if (vcRankUserWait[i].iUserID == vcRankUserNow[j].iUserID)	//已经上榜的玩家，覆盖一下数据
				{
					bFindOK = true;
					if (vcRankUserWait[i].iJifenNum > vcRankUserNow[j].iJifenNum)
					{
						vcRankUserNow[j] = vcRankUserWait[i];
					}
					break;
				}
			}

			if (!bFindOK)
			{
				vcRankUserNow.push_back(vcRankUserWait[i]);
			}
		}

		if (vcRankUserNow.size() > 1)
		{
			sort(vcRankUserNow.begin(), vcRankUserNow.end(), SortRankUser);		//按积分排序
		}

		SetNowRankUserList(iRankID, iPeriodID, vcRankUserNow, 90 * 24 * 3600);	//保存90天		

		_log(_ERROR, "RH", "OnTimerHandleWaitRankUser itemNum[%d][%d]", vcRankUserWait.size(), vcRankUserNow.size());
	}
	else
	{
		_log(_ERROR, "RH", "OnTimerHandleWaitRankUser no wait info");
	}

	SetLastWaitRankTime(iRankID, tmNow);	//更新一下上次排行时间
}

vector<RankUserInfoDef> RankHandler::GetWaitRankUserList(int iRankID, int iPeriodID, bool bDelete/* = true */)
{
	vector<RankUserInfoDef> vcRankUser;

	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_WAIT_RANK.c_str(), iRankID, iPeriodID);

	map<string, string> mapValue;
	bool bSucc = RedisHelper::hgetAll(m_redisCt, strKey, mapValue);
	if (bSucc)
	{		
		map<string, string>::iterator iter = mapValue.begin();
		while (iter != mapValue.end())
		{
			RankUserInfoDef singleUser;
			singleUser.clear();
			singleUser.iRankID = iRankID;
			singleUser.iPeriodID = iPeriodID;
			singleUser.iUserID = atoi(iter->first.c_str());

			std::vector<std::string> vcVal;
			Util::CutString(vcVal, iter->second, "_");
			if (vcVal.size() >= 6)	//jifen_headimage_headframeid_extra_tm_nick
			{
				singleUser.iJifenNum = atoi(vcVal.at(0).c_str());
				singleUser.iHeadImg = atoi(vcVal.at(1).c_str());
				singleUser.iHeadFrameId = atoi(vcVal.at(2).c_str());
				singleUser.strExtra = vcVal.at(3);
				singleUser.iLastTime = atoi(vcVal.at(4).c_str());
				for (int i = 5; i < vcVal.size(); i++)	//Nick可能带着下划线 特殊处理一下
				{
					if (i > 5)
					{
						singleUser.strNick += "_";
					}
					singleUser.strNick += vcVal.at(i);
				}

				vcRankUser.push_back(singleUser);
			}

			iter++;
		}
		
		if (bDelete)
		{
			RedisHelper::del(m_redisCt, strKey);	//待排行信息处理完了 数据清掉吧
		}
	}

	_log(_ERROR, "RH", "GetWaitRankUserList iRankID[%d], listSize[%d] deleteKey[%s]", iRankID, vcRankUser.size(), bDelete ? "true" : "false");

	return vcRankUser;
}

vector<RankUserInfoDef> RankHandler::GetNowRankUserList(int iRankID, int iPeriodID)
{
	vector<RankUserInfoDef> vcRankUser;

	DKRankConfigDef* config = findRankConfInfo(iRankID);
	if (config == NULL)
	{
		return vcRankUser;
	}

	if (config->iRankMode == RankHandler::RankMode::INC_DEC)	//增减排行模式
	{
		char szKey[64] = { 0 };
		sprintf(szKey, KEY_USER_RANK_LIST1.c_str(), iRankID, iPeriodID);

		long long llItemNum = RedisHelper::zcard(m_redisCt, szKey);
		if (llItemNum > 0)
		{
			int iHangdleCnt = config->iMaxRankNum - 1;
			vector<std::string> vecStr;
			RedisHelper::zrangeByIndex(m_redisCt, szKey, 0, iHangdleCnt, vecStr);
			for (int i = 0; i < vecStr.size(); i++)
			{
				int iUserID = atoi(vecStr.at(i).c_str());

				RankUserInfoDef userInfo = GetRankUserInfo(iUserID, iRankID, iPeriodID);

				userInfo.iRankNum = i + 1;		//已经是排序好了的

				vcRankUser.push_back(userInfo);
			}
		}
	}
	else if (config->iRankMode == RankHandler::RankMode::ONLY_ADD)	//累计排行模式
	{
		char strKey[64] = { 0 };
		sprintf(strKey, KEY_USER_RANK_LIST2.c_str(), iRankID, iPeriodID);

		map<string, string> mapValue;
		bool bSucc = RedisHelper::hgetAll(m_redisCt, strKey, mapValue);
		if (bSucc)
		{
			map<string, string>::iterator iter = mapValue.begin();
			while (iter != mapValue.end())
			{
				RankUserInfoDef singleUser;
				singleUser.clear();
				singleUser.iRankID = iRankID;
				singleUser.iPeriodID = iPeriodID;
				singleUser.iRankNum = atoi(iter->first.c_str());

				std::vector<std::string> vcVal;
				Util::CutString(vcVal, iter->second, "_");
				if (vcVal.size() >= 7)	//userid_jifen_headimage_headframeid_extra_tm_nick
				{
					singleUser.iUserID = atoi(vcVal.at(0).c_str());
					singleUser.iJifenNum = atoi(vcVal.at(1).c_str());
					singleUser.iHeadImg = atoi(vcVal.at(2).c_str());
					singleUser.iHeadFrameId = atoi(vcVal.at(3).c_str());
					singleUser.strExtra = vcVal.at(4);
					singleUser.iLastTime = atoi(vcVal.at(5).c_str());
					for (int i = 6; i < vcVal.size(); i++)	//Nick可能带着下划线 特殊处理一下
					{
						if (i > 6)
						{
							singleUser.strNick += "_";
						}
						singleUser.strNick += vcVal.at(i);
					}

					vcRankUser.push_back(singleUser);
				}

				iter++;
			}
		}
	}	

	_log(_ERROR, "RH", "GetNowRankUserList iRankID[%d] iPeriodID[%d] itemNum[%d]", iRankID, iPeriodID, vcRankUser.size());

	return vcRankUser;
}

void RankHandler::SetNowRankUserList(int iRankID, int iPeriodID, vector<RankUserInfoDef> vcRankUser, int expire)
{	
	DKRankConfigDef* config = findRankConfInfo(iRankID);
	if (config == NULL || config->iRankMode == RankHandler::RankMode::INC_DEC)
	{
		return;
	}

	if (vcRankUser.empty())
	{
		return;
	}

	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_RANK_LIST2.c_str(), iRankID, iPeriodID);

	RedisHelper::del(m_redisCt, strKey);

	char strField[64] = { 0 };	

	int iRankNum = 1;
	int iMinJifenNum = 0;
	for (int i = 0; i < vcRankUser.size(); i++)
	{
		sprintf(strField, "%d", iRankNum);

		char szValue[256] = { 0 };	//格式：userid_jifen_headimage_headframeid_extra_tm_nick
		sprintf(szValue, "%d_%d_%d_%d_%s_%d_%s", vcRankUser[i].iUserID, vcRankUser[i].iJifenNum, vcRankUser[i].iHeadImg, vcRankUser[i].iHeadFrameId, vcRankUser[i].strExtra.c_str(), vcRankUser[i].iLastTime, vcRankUser[i].strNick.c_str());

		RedisHelper::hset(m_redisCt, strKey, strField, szValue);

		iRankNum++;

		if (iRankNum > config->iMaxRankNum)
		{
			iMinJifenNum = vcRankUser[i].iJifenNum;		//记录榜上最后一名的玩家积分
			break;
		}
	}

	RedisHelper::expire(m_redisCt, strKey, expire);

	//更新排行榜最低积分
	if (iMinJifenNum > 0)
	{
		char szMinUser[64] = { 0 };
		sprintf(szMinUser, "%d_%s", iMinJifenNum, "null");	//value:jifen_extra   -- 积分_额外信息

		SetRankMinUserInfo(iRankID, iPeriodID, szMinUser, 90 * 24 * 3600);		//保存90天
	}

	_log(_ERROR, "RH", "SetNowRankUserList itemNum[%d] iMinJifenNum[%d]", vcRankUser.size(), iMinJifenNum);
}

int RankHandler::GetRankTestConf(int iRankID)
{
	int iTestTime = 0;

	char szKey[64] = { 0 };
	sprintf(szKey, "rank%d_gap_time", iRankID);

	GetValueInt(&iTestTime, szKey, "redis_test.conf", "System Base Info", "0");

	return iTestTime;
}

int RankHandler::GetRankPeriodID(int iRankID, int tmNow)
{
	int iPeriodID = 0;

	DKRankConfigDef* config = findRankConfInfo(iRankID);
	if (config != NULL)
	{		
		if (config->iTimeType == RankHandler::TimeType::CONF_TIME)	//按配置时间结算1次
		{
			if (tmNow >= config->iEndTime)
			{
				iPeriodID = 1;	//只结算1次的话，就算第1期吧
			}
		}
		else if (config->iTimeType == RankHandler::TimeType::WEEK_TIME)	//每周结算1次
		{
			iPeriodID = Util::GetGapWeekNum(config->iBeginTime, tmNow) + 1;
		}
		else if (config->iTimeType == RankHandler::TimeType::MOUTH_TIME)	//每月结算1次
		{
			iPeriodID = Util::GetGapMonthNum(config->iBeginTime, tmNow) + 1;
		}

		//testcode
		int iTestTime = GetRankTestConf(iRankID);
		if (iTestTime > 0)
		{
			iPeriodID = (tmNow - config->iBeginTime) / iTestTime + 1;
		}
	}	

	return iPeriodID;
}

int RankHandler::GetLastWaitRankTime(int iRankID)
{
	map<int, int>::iterator pos = m_mapWaitTime.find(iRankID);
	if (pos != m_mapWaitTime.end())
	{
		return pos->second;
	}
	
	return 0;
}

void RankHandler::SetLastWaitRankTime(int iRankID, int tmNow)
{
	m_mapWaitTime[iRankID] = tmNow;
}

void RankHandler::OnTimerGetParamsKeyValue()
{
	vector<string>vcKey;
	//需要获取的参数在这里push
	if (vcKey.empty())
	{
		return;
	}
	string strKey = "";
	for (int i = 0; i < vcKey.size(); i++)
	{
		strKey.append(vcKey.at(i));
		if (i < vcKey.size() - 1)
		{
			strKey.append("&");
		}
	}

	CSQLWrapperSelect hSQLSelect("params", m_pMySqlUser->GetMYSQLHandle());
	hSQLSelect.AddQueryField("ParamKey");
	hSQLSelect.AddQueryField("ParamValue");
	hSQLSelect.AddQueryConditionStr("ParamKey", strKey, CSQLWrapper::ERelationType::StringIn);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlUser->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "TH", "OnTimerGetParamsKeyValue find params info err");
		return;
	}

	const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
	if (vecRecordList.empty())
	{
		_log(_ERROR, "TH", "OnTimerGetParamsKeyValue params no find");
		return;
	}

	for (const CMySQLTableRecord& hRecord : vecRecordList)
	{
		string strKey = hRecord.GetFieldAsString("ParamKey");
		string strValue = hRecord.GetFieldAsString("ParamValue");

		m_mapParamsKeyVal[strKey] = strValue;
	}
}

string RankHandler::findParamsValue(string key)
{
	map<string, string>::iterator itor = m_mapParamsKeyVal.find(key);
	if (itor != m_mapParamsKeyVal.end())
	{
		return itor->second;
	}

	return "";
}

void RankHandler::HandleGetParamsMsg(char* msgData, int iSocketIndex)
{
	GameGetParamsReqDef* pMsgReq = (GameGetParamsReqDef*)msgData;

	char* pKey = msgData + sizeof(GameGetParamsReqDef);
	vector<ParamsConfDef> vcParams;
	for (int i = 0; i < pMsgReq->iKeyNum; i++)
	{
		ParamsConfDef stParamsConf;
		memset(&stParamsConf, 0, sizeof(ParamsConfDef));
		memcpy(stParamsConf.cKey, pKey, 32);
		vcParams.push_back(stParamsConf);
		pKey += 32;
	}
	time_t tmNow = time(NULL);
	string strNeedFromDB = "";
	int iNeedFromDBNum = 0;
	int iSucGetConfNum = 0;
	for (int i = 0; i < vcParams.size(); i++)
	{
		map<string, ParamsConfDef>::iterator pos = m_mapParmsConf.find(vcParams[i].cKey);
		if (pos != m_mapParmsConf.end() && pos->second.iLastTime > tmNow)
		{
			strcpy(vcParams[i].cValue, pos->second.cValue);
			iSucGetConfNum++;
		}
		else
		{
			if (iNeedFromDBNum > 0)
			{
				strNeedFromDB.append("&");
			}
			strNeedFromDB.append(vcParams[i].cKey);
			iNeedFromDBNum++;
		}
	}

	while (iNeedFromDBNum > 0)
	{
		CSQLWrapperSelect hSQLSelect("params", m_pMySqlUser->GetMYSQLHandle());
		hSQLSelect.AddQueryField("ParamKey");
		hSQLSelect.AddQueryField("ParamValue");
		if (iNeedFromDBNum == 1)
		{
			hSQLSelect.AddQueryConditionStr("ParamKey", strNeedFromDB);
		}
		else
		{
			hSQLSelect.AddQueryConditionStr("ParamKey", strNeedFromDB, CSQLWrapper::ERelationType::StringIn);
		}

		CMySQLTableResult hMySQLTableResult;
		if (!m_pMySqlUser->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult, true))
		{
			_log(_ERROR, "TH", "HandleGetParamsMsg find prop[%s] info err", (char*)strNeedFromDB.c_str());
			break;
		}
		const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
		if (vecRecordList.empty())
		{
			_log(_ERROR, "TH", "HandleGetParamsMsg no find conf[%s] info", (char*)strNeedFromDB.c_str());
			break;
		}
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			string strTempKey = hRecord.GetFieldAsString("ParamKey");
			string strTempValue = hRecord.GetFieldAsString("ParamValue");
			for (int i = 0; i < vcParams.size(); i++)
			{
				if (strcmp(vcParams[i].cKey, strTempKey.c_str()) == 0)
				{
					strcpy(vcParams[i].cValue, strTempValue.c_str());

					iSucGetConfNum++;
					//加入缓存
					map<string, ParamsConfDef>::iterator pos = m_mapParmsConf.find(vcParams[i].cKey);
					if (pos != m_mapParmsConf.end())
					{
						memset(pos->second.cValue, 0, 128);
						strcpy(pos->second.cValue, vcParams[i].cValue);
						pos->second.iLastTime = tmNow + 600;//缓存时间600s

						_log(_DEBUG, "TH", "HandleGetParamsMsg use cache key[%s] value[%s]", pos->first.c_str(), pos->second.cValue);
					}
					else
					{
						vcParams[i].iLastTime = tmNow + 600;//缓存时间600s
															//m_mapParmsConf.emplace(vcParams[i].cKey, vcParams[i]);
						m_mapParmsConf.insert(std::make_pair(vcParams[i].cKey, vcParams[i]));
					}
					break;
				}
			}
		}
		break;
	}
	if (iSucGetConfNum > 0)
	{
		char cBuffer[2048]; //扩充到2048 最多可支持查询12个key
		memset(cBuffer, 0, sizeof(cBuffer));
		int iBuffeLen = sizeof(GameGetParamsResDef);
		GameGetParamsResDef *pParamsRes = (GameGetParamsResDef*)cBuffer;
		pParamsRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
		pParamsRes->msgHeadInfo.iMsgType = REDIS_GET_PARAMS_MSG;
		pParamsRes->msgHeadInfo.iSocketIndex = iSocketIndex;

		char* pChar = cBuffer + iBuffeLen;
		for (int i = 0; i < vcParams.size(); i++)
		{
			if (strlen(vcParams[i].cValue) > 0)
			{
				pParamsRes->iKeyNum++;
				strcpy(pChar, vcParams[i].cKey);
				pChar += 32;
				strcpy(pChar, vcParams[i].cValue);
				pChar += 128;
				iBuffeLen += 160;
			}
		}
		m_pSendQueue->EnQueue(cBuffer, iBuffeLen);
	}
}