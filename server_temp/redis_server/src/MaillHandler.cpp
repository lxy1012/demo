#include "MaillHandler.h"
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

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/
const int g_iMinControlVID = 5000;//1-该范围的id表示控制ai
const int g_iMaxControlVID = 5999;//1-该范围的id表示控制ai

MaillHandler::MaillHandler(MsgQueue *pSendQueue, MsgQueue *pLogicQueue)
{
	m_pSendQueue = pSendQueue;

	m_pLogicQueue = pLogicQueue;

	m_iCheckFriendNoCDTm = 4;//首次启动4s后开始	
	m_bIniCheckFriendRoomNo = false;
	m_mapInUseCtrlAI.clear();
	m_mapControlAIID.clear();
	m_iTestCD = 15;
}

void MaillHandler::HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex)
{
	if (m_redisCt == NULL)
	{
		_log(_ERROR, "MaillHandler::HandleMsg", "error m_redisCt == NULL");
		return;
	}
	//_log(_DEBUG, "MaillHandler", "HandleMsg iMsgType[0x%x] iLen[%d] iSocketIndex[%d]", iMsgType, iLen, iSocketIndex);
	if (GAME_AUTHEN_REQ_RADIUS_MSG == iMsgType)
	{
		HandleServerAuthenReq(szMsg, iSocketIndex);
	}
	else if (iMsgType == SERVER_REDIS_MAIL_COM_MSG)
	{
		HandleComMailMsg(szMsg, iLen);
	}
	else if (iMsgType == RD_GAME_CHECK_ROOMNO_MSG)
	{
		HandleCheckRoomNoReq(szMsg, iSocketIndex, iLen);
	}
	else if (iMsgType == RD_GAME_UPDATE_ROOMNO_STATUS)
	{
		HandleUpdateRoomNoStatus(szMsg, iSocketIndex, iLen);
	}
	else if (iMsgType == RD_GET_VIRTUAL_AI_INFO_MSG)
	{
		HandleGetVirtualAiInfo(szMsg, iSocketIndex, iLen);
	}
	else if (iMsgType == RD_SET_VIRTUAL_AI_RT_MSG)
	{
		HandleSetVirtualAiInfo(szMsg, iLen);
	}
	else if (iMsgType == RD_USE_CTRL_AI_MSG)
	{
		HandleCtrlAIStatus(szMsg, iSocketIndex);
	}
}

void MaillHandler::HandleComMailMsg(char* szMsg, int iLen)
{
	SRedisMailComMsgDef* pReqMsg = (SRedisMailComMsgDef*)szMsg;
	//低6位标题(utf_8)长度，再12位(utf_8)文本长度，再8位附件道具奖励信息
	int iTitleLen = pReqMsg->iMailInfoLen & 0x3f;
	int iContenLen = (pReqMsg->iMailInfoLen >> 6) & 0xfff;
	int iAwardPropInfo = (pReqMsg->iMailInfoLen >> 18) & 0xff;
	int iMailBtnInfoLen = pReqMsg->iMailBtnInfoLen;

	char strKey[64];
	sprintf(strKey, "%s%d", KEY_USER_MAIL.c_str(), pReqMsg->iUserID);

	int iNeedLen = iTitleLen + iContenLen + iAwardPropInfo + iMailBtnInfoLen + 128;
	CharMemManage* pCharMem = GetCharMem(iNeedLen);
	if (pCharMem == NULL)
	{
		return;
	}

	int iMailID = DBGetUserMailID(pReqMsg->iUserID, true);
	char* pMailInfo = pCharMem->pBuffer;
	memset(pMailInfo, 0, sizeof(pMailInfo));

	char szTemp[8];
	bool bAddGap = false;
	if (iTitleLen > 0)
	{
		sprintf(szTemp, "%s", "{\"t\":\"");
		strcat(pMailInfo, szTemp);
		strcat(pMailInfo, szMsg + sizeof(SRedisMailComMsgDef));
		strcat(pMailInfo, "\"");
		bAddGap = true;
	}
	if (iContenLen > 0)
	{
		if (bAddGap)
		{
			strcat(pMailInfo, ",");
		}
		sprintf(szTemp, "%s", "\"c\":\"");
		strcat(pMailInfo, szTemp);
		strcat(pMailInfo, szMsg + sizeof(SRedisMailComMsgDef) + iTitleLen);
		strcat(pMailInfo, "\"");
		bAddGap = true;
	}
	int iMailRewardID = 0;
	if (iAwardPropInfo > 0)
	{
		if (bAddGap)
		{
			strcat(pMailInfo, ",");
		}
		sprintf(szTemp, "%s", "\"p\":\"");
		strcat(pMailInfo, szTemp);
		strcat(pMailInfo, szMsg + sizeof(SRedisMailComMsgDef) + iTitleLen + iContenLen);
		strcat(pMailInfo, "\"");

		char szPropInfo[128];
		memset(szPropInfo, 0, sizeof(szPropInfo));
		strcpy(szPropInfo, szMsg + sizeof(SRedisMailComMsgDef) + iTitleLen + iContenLen);
		vector<string>vcCut;
		Util::CutString(vcCut, szPropInfo, ",");
		char pStrPropInfo[5][20];
		for (int i = 0; i < 5; i++)
		{
			memset(pStrPropInfo[i], 0, sizeof(pStrPropInfo[i]));
			if (i < vcCut.size())
			{
				strcpy(pStrPropInfo[i], (char*)vcCut[i].c_str());
				strcat(pStrPropInfo[i], "_0");
			}
		}

		iMailRewardID = DBAddUserMailAward(pReqMsg->iUserID, pStrPropInfo[0], pStrPropInfo[1], pStrPropInfo[2], pStrPropInfo[3], pStrPropInfo[4], pReqMsg->iAwardType);
		if (iMailRewardID > 0)
		{
			strcat(pMailInfo, ",");
			sprintf(szTemp, "%s", "\"db\":");
			strcat(pMailInfo, szTemp);
			sprintf(szTemp, "%d", iMailRewardID);
			strcat(pMailInfo, szTemp);
		}
		//奖励标记位领取
		strcat(pMailInfo, ",");
		sprintf(szTemp, "%s", "\"r\":");
		strcat(pMailInfo, szTemp);
		sprintf(szTemp, "%d", 0);
		strcat(pMailInfo, szTemp);
		bAddGap = true;
	}
	if (iMailBtnInfoLen > 0)
	{
		if (bAddGap)
		{
			strcat(pMailInfo, ",");
		}
		sprintf(szTemp, "%s", "\"b\":\"");
		strcat(pMailInfo, szTemp);
		strcat(pMailInfo, szMsg + sizeof(SRedisMailComMsgDef) + iTitleLen + iContenLen + iAwardPropInfo);
		strcat(pMailInfo, "\"");
		bAddGap = true;
	}

	if (bAddGap)
	{
		strcat(pMailInfo, ",");
	}
	sprintf(szTemp, "%s", "\"ts\":");
	strcat(pMailInfo, szTemp);
	sprintf(szTemp, "%d}", time(NULL));
	strcat(pMailInfo, szTemp);
	if (iMailID > 0)
	{
		char strField[64];
		sprintf(strField, "MAIL_%d", iMailID);

		RedisHelper::hset(m_redisCt, strKey, strField, pMailInfo);

		//重新设定个人邮件超时时间
		int iExpireSecond = 90 * 24 * 3600;
		RedisHelper::expire(m_redisCt, strKey, iExpireSecond);
	}
	else
	{
		_log(_ERROR, "MH", "HandleComMailMsg: user[%d],iMailID[%d]", pReqMsg->iUserID, iMailID);
	}
	ReleaseCharMem(pCharMem);
}


void MaillHandler::CallBackOnTimer(int iTimeNow)
{
	if (!m_bIniCheckFriendRoomNo)
	{
		CheckFriendRoomNoIni();
		FillInUseCtrlAI();
		m_bIniCheckFriendRoomNo = true;
	}
	
	m_iCheckFriendNoCDTm--;
	if (m_iCheckFriendNoCDTm <= 0)
	{
		CheckFriendRoomNoTimer();
		CheckInUseCtrlAI();
		m_iCheckFriendNoCDTm = 300;
	}
	/*if (m_iTestCD > 0)
	{
		m_iTestCD--;
		_log(_ERROR, "MH", "CallBackOnTimer:[%d]", m_iTestCD);
		char szTempField[32];
		sprintf(szTempField, "cd_%d", m_iTestCD);
		char szTemp[32];
		sprintf(szTemp, "%d", m_iTestCD);
		RedisHelper::hset(m_redisCt, "rus_test_cd", szTempField, szTemp);
		//RedisHelper::hincrby(m_redisCt, "rus_test_hin", "test", 1);
	}	*/
}

void MaillHandler::CallBackNextDay(int iTimeBeforeFlag, int iTimeNowFlag)
{

}

int MaillHandler::DBGetUserMailID(int iUserID, bool bAddMailID)
{
	//待增加
}

int MaillHandler::DBAddUserMailAward(int iUserID, char* cPropInfo1, char* cPropInfo2, char* cPropInfo3, char* cPropInfo4, char* cPropInfo5, int iAwardType)
{
	//待增加（需要加邮件奖励记录表）
}


void MaillHandler::CheckFriendRoomNoTimer()//定时将已失效房号转移至可用房号
{
	if (stSysConfigBaseInfo.iIfMainRedis != 1)
	{
		return;
	}
	char strKey[32];
	sprintf(strKey,"%s", KEY_ROOM_USE_NO.c_str());
	long long iMaxScore = time(NULL) - 50 * 60;//最后一次更新时间是50分钟前的都可以挪走了
	vector<std::string>vecStr;
	vector<std::string>vecStrAdd;
	RedisHelper::zrangeByScore(m_redisCt, strKey, 0, iMaxScore, vecStr);
	_log(_DEBUG, "MaillHandler", "CheckFriendRoomNoTimer friend room zrangeByScore size[%d]", vecStr.size());
	if (!vecStr.empty())
	{
		for (int i = 0; i < vecStr.size(); i++)
		{
			_log(_DEBUG, "MaillHandler", "CheckFriendRoomNoTimer del [%s]", vecStr[i].c_str());
		}
		//一次最多转移300个数字
		if (vecStr.size() > 300)
		{
			vecStr.erase(vecStr.begin() + 300, vecStr.end());
		}
		//添加到可用房号中
		sprintf(strKey, "%s", KEY_ROOM_NO.c_str());
		for (int i = 0; i < vecStr.size(); i++)
		{
			if (atoi(vecStr[i].c_str()) > 0)
			{
				vecStrAdd.push_back(vecStr[i]);
			}
		}
		RedisHelper::sadd(m_redisCt, strKey, vecStrAdd);
		vecStrAdd.clear();
		//从已用房号中删除
		sprintf(strKey, "%s", KEY_ROOM_USE_NO.c_str());
		int iRt = RedisHelper::zrem(m_redisCt, strKey, vecStr);
		if (iRt != vecStr.size())
		{
			_log(_ERROR, "ELT", "CheckFriendRoomNoTimer:zrem err[%d]:[%d]", iRt, vecStr.size());
		}
		for (int i = 0; i < vecStr.size(); i++)
		{
			_log(_DEBUG, "ELT", "CheckFriendRoomNoTimer:i[%d],room[%s]", i, vecStr[i].c_str());
			sprintf(strKey, KEY_ROOM_INFO.c_str(), vecStr[i].c_str());   //在用房号房间信息
			RedisHelper::del(m_redisCt, strKey);
		}
	}
}
void MaillHandler::CheckFriendRoomNoIni()//首次启动后检测是否需要初始化一批房号
{
	if (stSysConfigBaseInfo.iIfMainRedis != 1)
	{
		return;
	}
	char strKey[32];
	sprintf(strKey, "%s", KEY_ROOM_NO.c_str());

	int iNOCnt = RedisHelper::scard(m_redisCt, strKey);
	if (iNOCnt == 0)
	{
		sprintf(strKey, "%s", KEY_ROOM_USE_NO.c_str());
		int iUseNOCnt = RedisHelper::scard(m_redisCt, strKey);
		if (iNOCnt == 0 && iUseNOCnt == 0)
		{
			int iGroupCnt = 300;//一次添加300个数字(5位数)
			for (int i = 10000; i < 100000;)
			{
				vector<string>vecStrNum;
				char cValueTemp[8];
				int iStartIndex = i;
				int iEndIndex = iStartIndex + iGroupCnt;
				if (iEndIndex > 99999)
				{
					iEndIndex = 99999;
				}
				_log(_ERROR, "ELT", "CheckFriendRoomNoIni:add:[%d--%d]", iStartIndex, iEndIndex);
				for (int j = iStartIndex; j < iEndIndex; j++)
				{
					sprintf(cValueTemp, "%d", j);
					vecStrNum.push_back(cValueTemp);
				}
				RedisHelper::sadd(m_redisCt, strKey, vecStrNum);
				i = iEndIndex;
				if (iEndIndex == 9999)
				{
					break;
				}
			}
		}
	}
}

//首次启动，从redis里获取一次正在使用的ai节点
void MaillHandler::FillInUseCtrlAI()
{
	if (stSysConfigBaseInfo.iIfMainRedis != 1)
	{
		return;
	}
	m_mapInUseCtrlAI.clear();
	char strKey[32] = { 0 };
	char cValue[128] = { 0 };
	for (int i = 0; i < sizeof(RECRUIT_GAME) / sizeof(int); i++)
	{
		int iGameID = RECRUIT_GAME[i];
		sprintf(strKey, KEY_IN_USE_CTRLAI.c_str(), iGameID);   //在用控制ai
		map<int, vector<int> >::iterator itor = m_mapInUseCtrlAI.find(iGameID);
		if (itor == m_mapInUseCtrlAI.end())
		{
			vector<int> vcAll;
			int iInfoCnt = RedisHelper::zcard(m_redisCt, strKey);
			if (iInfoCnt > 0)
			{
				vector<std::string>vecStr;
				long long iMinScore = time(NULL) - 40 * 60;
				RedisHelper::zrangeByScore(m_redisCt, strKey, iMinScore, time(NULL) + 3600 * 24, vecStr);
				for (int i = 0; i < vecStr.size(); i++)
				{
					int iRoomID = atoi(vecStr[i].c_str());
					vcAll.push_back(iRoomID);
					_log(_DEBUG, "TEST", "FillInUseCtrlAI game[%d] m_mapInUseCtrlAI push[%d]", iGameID, iRoomID);
				}
			}
			m_mapInUseCtrlAI[iGameID] = vcAll;
		}
	}
}

void MaillHandler::CheckInUseCtrlAI()
{
	if (stSysConfigBaseInfo.iIfMainRedis != 1)
	{
		return;
	}
	char strKey[32] = { 0 };
	char cValue[128] = { 0 };
	char strKeyRoomInfo[32] = { 0 };
	for (int i = 0; i < sizeof(RECRUIT_GAME) / sizeof(int); i++)
	{
		int iGameID = RECRUIT_GAME[i];
		sprintf(strKey, KEY_IN_USE_CTRLAI.c_str(), iGameID);
		map<int, vector<int> >::iterator itorUse = m_mapInUseCtrlAI.find(iGameID);
		map<int, vector<int> >::iterator itor = m_mapControlAIID.find(iGameID);
		long long iMaxScore = time(NULL) - 30 * 60;
		vector<std::string>vecStr;
		vector<std::string>vecStrAdd;
		RedisHelper::zrangeByScore(m_redisCt, strKey, 0, iMaxScore, vecStr);
		_log(_DEBUG, "MaillHandler", "CheckInUseCtrlAI friend ctrlAI zrangeByScore size[%d] game[%d]", vecStr.size(), iGameID);
		if (!vecStr.empty())
		{
			int iRt = RedisHelper::zrem(m_redisCt, strKey, vecStr);
			if (iRt != vecStr.size())
			{
				_log(_ERROR, "ELT", "CheckInUseCtrlAI:zrem err[%d]:[%d]", iRt, vecStr.size());
			}
		}
		for (int j = 0; j < vecStr.size(); j++)
		{
			int iCtrlAI = atoi(vecStr[j].c_str());
			sprintf(strKey, KEY_VIRTUAL_AI.c_str(), iCtrlAI);
			sprintf(cValue, "game%d_usering", iGameID);
			bool bExist = RedisHelper::exists(m_redisCt, strKey);
			if (bExist)
			{
				RedisHelper::hset(m_redisCt, strKey, cValue, "0");
			}
			if (itorUse != m_mapInUseCtrlAI.end())
			{
				vector<int>::iterator itorID = std::find(itorUse->second.begin(), itorUse->second.end(), iCtrlAI);
				if (itorID != itorUse->second.end())
				{
					_log(_DEBUG, "MaillHandler", "CheckInUseCtrlAI game[%d] del using ai[%d]", iGameID, iCtrlAI);
					itorUse->second.erase(itorID);
				}
			}
			if (itor != m_mapControlAIID.end())
			{
				vector<int>::iterator itorID = std::find(itor->second.begin(), itor->second.end(), iCtrlAI);
				if (itorID != itor->second.end())
				{
					_log(_DEBUG, "MaillHandler", "CheckInUseCtrlAI game[%d] add ai[%d]", iGameID, iCtrlAI);
					itor->second.push_back(iCtrlAI);
				}
			}
		}
	}
}

void MaillHandler::HandleCheckRoomNoReq(char *msgData, int iSocketIndex, int iLen)
{
	RdGameCheckRoomNoDef* pMsgReq = (RdGameCheckRoomNoDef*)msgData;
	char strKey[32];
	sprintf(strKey,"%s",KEY_ROOM_USE_NO.c_str());
	char cMember[32];
	sprintf(cMember, "%d", pMsgReq->iRoomNum);
	string strRt = RedisHelper::zscore(m_redisCt, strKey, cMember);
	long long iCreateTm = 0;
	if (strcmp(strRt.c_str(), "nil") != 0)
	{
		iCreateTm = atoll(strRt.c_str());
	}
	RdGameCheckRoomNoResDef msgRes;
	memset(&msgRes, 0, sizeof(RdGameCheckRoomNoResDef));
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = RD_GAME_CHECK_ROOMNO_MSG;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	msgRes.iRoomUserID = pMsgReq->iRoomUserID;
	msgRes.iRoomNum = pMsgReq->iRoomNum;
	time_t tmNow = time(NULL);
	if (tmNow - iCreateTm > 1800)//超过30分钟即认为房间号已失效
	{
		msgRes.iRt = 0;
		RemoveRoomNoToWaitUse(pMsgReq->iRoomNum);
	}
	else
	{
		msgRes.iRt = 1;

		//记录成功使用房号的ip及端口
		sprintf(strKey, KEY_ROOM_INFO.c_str(), pMsgReq->iRoomNum);
		map<string, string> mpField;
		mpField["landlordid"] = to_string(pMsgReq->iRoomUserID);
		mpField["serverid"] = to_string(pMsgReq->iServerID);
		RedisHelper::hmset(m_redisCt, strKey, mpField);
		RedisHelper::expire(m_redisCt, strKey, 3600);
	}
	if (m_pSendQueue)
	{
		m_pSendQueue->EnQueue((char*)&msgRes, sizeof(RdGameCheckRoomNoResDef));
	}
}

void MaillHandler::HandleUpdateRoomNoStatus(char *msgData, int iSocketIndex, int iLen)
{
	RdGameUpdateRoomNumStatusDef* pMsgReq = (RdGameUpdateRoomNumStatusDef*)msgData;
	char strKey[32];
	sprintf(strKey,"%s",KEY_ROOM_USE_NO.c_str());
	char cMember[32];
	sprintf(cMember, "%d", abs(pMsgReq->iRoomNum));
	if (pMsgReq->iIfInUse == 1)//在使用，重置时间为当前时间
	{
		time_t tmNow = time(NULL);
		RedisHelper::zadd(m_redisCt, strKey, tmNow, cMember);

		//在用房号IP端口数据有消息有效期延长
		sprintf(strKey, KEY_ROOM_INFO.c_str(), pMsgReq->iRoomNum);
		RedisHelper::expire(m_redisCt, strKey, 3600);
	}
}

void MaillHandler::RemoveRoomNoToWaitUse(int iRoomNo, int iGameID)
{
	iRoomNo = abs(iRoomNo);
	iGameID = iGameID > 0 ? iGameID : 11;
	char strKey[32];
	_log(_DEBUG, "MaillHandler", "RemoveRoomNoToWaitUse iGameID[%d] iRoomNo[%d]", iGameID, iRoomNo);
	if (iRoomNo >= 10000 && iRoomNo <= 99999)
	{
		sprintf(strKey,"%s", KEY_ROOM_USE_NO.c_str());
		char cMember[32];
		sprintf(cMember, "%d", iRoomNo);
		RedisHelper::zrem(m_redisCt, strKey, cMember);
		sprintf(strKey, "%s", KEY_ROOM_NO.c_str());
		RedisHelper::sadd(m_redisCt, strKey, cMember);
	}
	//清除在用房号所在ip及端口号
	sprintf(strKey, KEY_ROOM_INFO.c_str(), iRoomNo);
	RedisHelper::del(m_redisCt, strKey);
}


void MaillHandler::HandleServerAuthenReq(char *msgData, int iSocketIndex)
{
	GameRDAuthenReqPreDef* pMsg = (GameRDAuthenReqPreDef*)msgData;
	if (pMsg->iReqType == 3)
	{
		if (iSocketIndex < 0)//有服务器断开
		{
			return;
		}
	}
	else
	{
		_log(_ERROR, "SLT", "HandleServerAuthenReq:iReqType[%d],iServerid[%d]", pMsg->iReqType, pMsg->iServerID);
	}
}

void MaillHandler::HandleGetVirtualAiInfo(char *msgData, int iSocketIndex, int iLen)
{
	RdGetVirtualAIInfoReqDef* pMsgReq = (RdGetVirtualAIInfoReqDef*)msgData;
	int iGameID = 0;

	char cBuffer[1280] = { 0 };
	RdGetVirtualAIInfoResDef* pVirtualInfo = (RdGetVirtualAIInfoResDef*)cBuffer;
	memset(pVirtualInfo, 0, sizeof(RdGetVirtualAIInfoResDef));

	if (iLen >= sizeof(RdGetVirtualAIInfoReqDef))
	{
		iGameID = pMsgReq->iGameID;
		pVirtualInfo->iServerID = pMsgReq->iServerID;

		if (pMsgReq->iNeedNum > 0)
		{
			CallBakcGetCtrlAiInfo(pMsgReq, iSocketIndex);
			return;
		}
	}

	RDVirtualAIInfo aiInfo = GetVirtualAIInfo(iGameID, pMsgReq->iVirtualID);
	pVirtualInfo->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pVirtualInfo->msgHeadInfo.iMsgType = RD_GET_VIRTUAL_AI_INFO_MSG;
	pVirtualInfo->msgHeadInfo.iSocketIndex = iSocketIndex;

	pVirtualInfo->iDurakCnt = aiInfo.iBufferB0;
	pVirtualInfo->iHeadFrame = aiInfo.iHeadFrame;
	pVirtualInfo->iPlayCnt = aiInfo.iPlayCnt;
	pVirtualInfo->iWinCnt = aiInfo.iWinCnt;
	pVirtualInfo->iVTableID = pMsgReq->iVTableID;
	pVirtualInfo->iAchieveLevel = aiInfo.iAchieveLevel;
	pVirtualInfo->iVirtualID = aiInfo.iVirtualID;
	pVirtualInfo->iExp = aiInfo.iExp;

	char* pGameInfo = (char*)(pVirtualInfo + 1);
	int iExtraLen = 0;
	for (int i = 0; i < 10; i++)
	{
		memcpy(pGameInfo + iExtraLen, &aiInfo.iBufferA0 + i, sizeof(int));
		iExtraLen += sizeof(int);
	}
	int iStrBuffer1Len = aiInfo.strBuffer0.length();
	memcpy(pGameInfo + iExtraLen, &iStrBuffer1Len, sizeof(int));
	iExtraLen += sizeof(int);
	memcpy(pGameInfo + iExtraLen, aiInfo.strBuffer0.c_str(), iStrBuffer1Len);
	iExtraLen += iStrBuffer1Len;

	int iStrBuffer2Len = aiInfo.strBuffer1.length();
	memcpy(pGameInfo + iExtraLen, &iStrBuffer2Len, sizeof(int));
	iExtraLen += sizeof(int);
	memcpy(pGameInfo + iExtraLen, aiInfo.strBuffer1.c_str(), iStrBuffer2Len);
	iExtraLen += iStrBuffer2Len;

	m_pSendQueue->EnQueue(cBuffer, sizeof(RdGetVirtualAIInfoResDef) + iExtraLen);
}

RDVirtualAIInfo MaillHandler::GetVirtualAIInfo(int iGameID, int iVirtualID)
{
	RDVirtualAIInfo aiInfo;
	aiInfo.clear();

	char strKey[32] = { 0 };
	sprintf(strKey,KEY_VIRTUAL_AI.c_str(), iVirtualID);
	bool bExist = RedisHelper::exists(m_redisCt, strKey);

	iGameID = iGameID == 0 ? g_iGameHZXL : iGameID;

	_log(_DEBUG, "MAIL", "GetVirtualAIInfo key[%s]", strKey);

	if (bExist)
	{
		map<string, string> mapField;
		mapField["id"] = "";
		mapField["all_play"] = "";
		mapField["all_win"] = "";
		mapField["achieve_point"] = "";
		char cField[128] = { 0 };

		if (iGameID > 0)
		{
			sprintf(cField, "game%d_iWin", iGameID);
			mapField[cField] = "";
			sprintf(cField, "game%d_iPlayCnt", iGameID);
			mapField[cField] = "";

			for (int i = 0; i < 5; i++)
			{
				sprintf(cField, "game%d_iBufferA%d", iGameID, i);
				mapField[cField] = "";

				sprintf(cField, "game%d_iBufferB%d", iGameID, i);
				mapField[cField] = "";

				if (i < 2)
				{
					sprintf(cField, "game%d_strBuffer%d", iGameID, i);
					mapField[cField] = "";
				}
			}
		}
		bool bSucc = RedisHelper::hmget(m_redisCt, strKey, mapField);
		if (bSucc)
		{
			string strValue = mapField["id"];
			aiInfo.iVirtualID = strValue.empty() ? 0 : atoi(strValue.c_str());
			strValue = mapField["achieve_point"];
			aiInfo.iAchieveLevel = strValue.empty() ? 100 : atoi(strValue.c_str());
			strValue = mapField["head_frame"];
			aiInfo.iHeadFrame = strValue.empty() ? 0 : atoi(strValue.c_str());
			strValue = mapField["all_play"];
			aiInfo.iPlayCnt = strValue.empty() ? 0 : atoi(strValue.c_str());
			strValue = mapField["all_win"];
			aiInfo.iWinCnt = strValue.empty() ? 0 : atoi(strValue.c_str());

			if (iGameID > 0)
			{
				sprintf(cField, "game%d_iWin", iGameID);
				strValue = mapField[cField];
				aiInfo.iWinCnt = strValue.empty() ? 0 : atoi(strValue.c_str());

				sprintf(cField, "game%d_iPlayCnt", iGameID);
				strValue = mapField[cField];
				aiInfo.iPlayCnt = strValue.empty() ? 0 : atoi(strValue.c_str());

				for (int i = 0; i < 5; i++)
				{
					sprintf(cField, "game%d_iBufferA%d", iGameID, i);
					strValue = mapField[cField];
					*(&aiInfo.iBufferA0) = strValue.empty() ? 0 : atoi(strValue.c_str());
				}

				for (int i = 0; i < 5; i++)
				{
					sprintf(cField, "game%d_iBufferB%d", iGameID, i);
					strValue = mapField[cField];
					*(&aiInfo.iBufferB0) = strValue.empty() ? 0 : atoi(strValue.c_str());
				}

				sprintf(cField, "game%d_strBuffer0", iGameID);
				aiInfo.strBuffer0 = mapField[cField];
				sprintf(cField, "game%d_strBuffer1", iGameID);
				aiInfo.strBuffer1 = mapField[cField];
			}
		}
	}
	else   //不存在则插入一个
	{
		char cField[128] = { 0 };

		map<string, string> mapField;
		char cValue[128] = { 0 };
		sprintf(cValue, "%d", iVirtualID);
		mapField["id"] = cValue;
		aiInfo.iVirtualID = iVirtualID;

		int iAllNum = 50 + rand() % 300;
		sprintf(cValue, "%d", iAllNum);
		aiInfo.iPlayCnt = iAllNum;
		mapField["all_play"] = cValue;
		mapField["game11_iPlayCnt"] = cValue;

		int iWinRate = 30 + rand() % 41;
		int iWinNum = iAllNum*iWinRate / 100;;
		sprintf(cValue, "%d", iWinNum);
		aiInfo.iWinCnt = iWinNum;
		mapField["all_win"] = cValue;
		mapField["game11_iWin"] = cValue;

		mapField["game11_iBufferB0"] = cValue;

		int iAchievel = GetVirtualAchivel(iAllNum);
		sprintf(cValue, "%d", iAchievel);
		mapField["achieve_point"] = cValue;
		aiInfo.iAchieveLevel = iAchievel;

		//根据局数等信息设置经验值iexp
		int iMainLv = aiInfo.iAchieveLevel / 100;
		int iSubLv = aiInfo.iAchieveLevel % 100;
		int lv[] = { 2,3,4,7,8,9,10,11,13,14,14,15,17,18,19,21,22,25 };
		int iExp[] = { 0.5,1.5,3,5,8,14,22,31,43,58,74,92,113,137,164,194,226,266,311,359,415,475,539,611,686,776,881,1001,1136 };
		int idx = (iMainLv - 1) * 3 + iSubLv - 1;
		if (idx < sizeof(lv))
		{
			int iCurLv = lv[idx];
			int iCurExp = iExp[iCurLv - 2];
			aiInfo.iExp = iCurExp * 3600 + 10;
		}
		sprintf(cValue, "%d", aiInfo.iExp);
		mapField["exp"] = cValue;

		int iHeadImg = 0;
		int iRandRate = rand() % 100;//30的概率有头像框
		if (iRandRate < 30 && iAchievel / 100 > 1)
		{
			int iRandAchv = rand() % ((iAchievel - 100) / 100);
			if (iAchievel == 602)
			{
				iHeadImg = 231 + iRandAchv;
			}
			else if (iRandAchv > 0)
			{
				iHeadImg = 231 + iRandAchv - 1;
			}
			else
			{
				iHeadImg = 231;
			}
		}
		sprintf(cValue, "%d", iHeadImg);
		mapField["head_frame"] = cValue;
		aiInfo.iHeadFrame = iHeadImg;
		RedisHelper::hmset(m_redisCt, strKey, mapField);
	}

	RedisHelper::expire(m_redisCt, strKey, 3600 * 24 * 15);
	return aiInfo;
}

void MaillHandler::CallBakcGetCtrlAiInfo(RdGetVirtualAIInfoReqDef * pMsgReq, int iSocketIndex)
{
	int iGameID = pMsgReq->iGameID;
	map<int, vector<int>>::iterator itor = m_mapControlAIID.find(iGameID);
	map<int, vector<int>>::iterator itorUse = m_mapInUseCtrlAI.find(iGameID);

	_log(_DEBUG, "TEST", "CallBakcGetCtrlAiInfo game[%d] id[%d]server[%d] need[%d]", iGameID, pMsgReq->iVirtualID, pMsgReq->iServerID, g_iMinControlVID, g_iMaxControlVID, pMsgReq->iNeedNum);
	if (itor == m_mapControlAIID.end())  //首次使用，初始化一批ai
	{
		_log(_DEBUG, "TEST", "CallBakcGetCtrlAiInfo game[%d] m_mapControlAIID is null", iGameID);
		vector<int>vcControlAIID;
		vector<int>vcID;
		for (int i = g_iMinControlVID; i <= g_iMaxControlVID; i++)
		{
			if (itorUse == m_mapInUseCtrlAI.end())
			{
				_log(_ALL, "TEST", "CallBakcGetCtrlAiInfo game[%d] push AI [%d]", iGameID, i);
				vcControlAIID.push_back(i);
			}
			else if (std::find(itorUse->second.begin(), itorUse->second.end(), i) == itorUse->second.end())  //正在用的不放进来
			{
				_log(_ALL, "TEST", "CallBakcGetCtrlAiInfo game[%d] push ai[%d]", iGameID, i);
				vcControlAIID.push_back(i);
			}
			else
			{
				_log(_ALL, "TEST", "CallBakcGetCtrlAiInfo game[%d] push error ai[%d]", iGameID, i);
			}
		}
		_log(_DEBUG, "TEST", "CallBakcGetCtrlAiInfo game[%d] vcControlAIID size[%d]", iGameID, vcControlAIID.size());
		while (!vcControlAIID.empty())
		{
			int iRandIndex = rand() % vcControlAIID.size();//虚拟AI打乱用
			vcID.push_back(vcControlAIID[iRandIndex]);
			vcControlAIID.erase(vcControlAIID.begin() + iRandIndex);
		}
		_log(_DEBUG, "TEST", "CallBakcGetCtrlAiInfo game[%d] m_mapControlAIID init end vcID[%d]", iGameID, vcID.size());

		m_mapControlAIID[iGameID] = vcID;
		itor = m_mapControlAIID.find(iGameID);
	}

	char cBuffer[2048] = { 0 };
	RdGetVirtualAIRtMsgDef* pMsgRes = (RdGetVirtualAIRtMsgDef*)cBuffer;
	memset(cBuffer, 0, sizeof(cBuffer));
	pMsgRes->iServerID = pMsgReq->iServerID;
	pMsgRes->iVTableID = pMsgReq->iVTableID;
	pMsgRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgRes->msgHeadInfo.iMsgType = RD_GET_VIRTUAL_AI_RES_MSG;
	pMsgRes->msgHeadInfo.iSocketIndex = iSocketIndex;

	vector<int> vcRet;
	if (pMsgReq->iVirtualID == 0 && itor->second.size() > pMsgReq->iNeedNum)   //找一个未使用的ai返回
	{
		if (itorUse == m_mapInUseCtrlAI.end())
		{
			m_mapInUseCtrlAI[iGameID] = vcRet;
			itorUse = m_mapInUseCtrlAI.find(iGameID);
		}

		for (int i = 0; i < pMsgReq->iNeedNum; i++)
		{
			if (find(vcRet.begin(), vcRet.end(), itor->second[0]) == vcRet.end())
			{
				int iCtrlAI = itor->second[0];
				vcRet.push_back(iCtrlAI);
				itor->second.erase(itor->second.begin());

				if (find(itorUse->second.begin(), itorUse->second.end(), iCtrlAI) == itorUse->second.end())
				{
					itorUse->second.push_back(iCtrlAI);
				}
			}
		}
	}

	if (vcRet.empty() || vcRet.size() != pMsgReq->iNeedNum)  //没有ai用了，直接返回失败
	{
		m_pSendQueue->EnQueue(cBuffer, sizeof(RdGetVirtualAIRtMsgDef));
		return;
	}

	char strKey[32] = { 0 };
	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	char* pMsgExtra = (char*)(pMsgRes + 1);
	int iExtraLen = 0;
	for (int i = 0; i < vcRet.size(); i++)
	{
		RDVirtualAIInfo aiInfo = GetVirtualAIInfo(iGameID, vcRet[i]);
		if (aiInfo.iVirtualID > 0)
		{
			pMsgRes->iRetNum++;

			memcpy(pMsgExtra + iExtraLen, &aiInfo.iVirtualID, sizeof(int));
			iExtraLen += sizeof(int);
			memcpy(pMsgExtra + iExtraLen, &aiInfo.iAchieveLevel, sizeof(int));
			iExtraLen += sizeof(int);
			memcpy(pMsgExtra + iExtraLen, &aiInfo.iHeadFrame, sizeof(int));
			iExtraLen += sizeof(int);
			memcpy(pMsgExtra + iExtraLen, &aiInfo.iExp, sizeof(int));
			iExtraLen += sizeof(int);
			memcpy(pMsgExtra + iExtraLen, &aiInfo.iPlayCnt, sizeof(int));
			iExtraLen += sizeof(int);
			memcpy(pMsgExtra + iExtraLen, &aiInfo.iWinCnt, sizeof(int));
			iExtraLen += sizeof(int);
			memcpy(pMsgExtra + iExtraLen, &aiInfo.iBufferA0, sizeof(int)*10);
			iExtraLen += sizeof(int) * 10;
			int iLen0 = aiInfo.strBuffer0.length();
			memcpy(pMsgExtra + iExtraLen, &iLen0, sizeof(int));
			iExtraLen += sizeof(int);
			memcpy(pMsgExtra + iExtraLen, aiInfo.strBuffer0.c_str(), iLen0);
			iExtraLen += iLen0;
			int iLen1 = aiInfo.strBuffer1.length();
			memcpy(pMsgExtra + iExtraLen, &iLen1, sizeof(int));
			iExtraLen += sizeof(int);
			memcpy(pMsgExtra + iExtraLen, aiInfo.strBuffer1.c_str(), iLen1);
			iExtraLen += iLen1;

			//占用标识
			sprintf(strKey,KEY_VIRTUAL_AI.c_str(), vcRet[i]);
			char cField[32] = { 0 };
			sprintf(cField, "game%d_usering", iGameID);
			RedisHelper::hset(m_redisCt, strKey, cField, "1");

			time_t tmNow = time(NULL);
			char strKeyUse[32] = { 0 };
			sprintf(strKeyUse, KEY_IN_USE_CTRLAI.c_str(), iGameID);
			sprintf(cField, "%d", vcRet[i]);
			RedisHelper::zadd(m_redisCt, strKeyUse, tmNow, cField);
		}
	}

	m_pSendQueue->EnQueue(cBuffer, sizeof(RdGetVirtualAIRtMsgDef) + iExtraLen);

}

void MaillHandler::HandleSetVirtualAiInfo(char *msgData, int iLen)
{
	RdSetVirtualAIRtMsgDef * pMsgReq = (RdSetVirtualAIRtMsgDef*)msgData;
	char strKey[32] = { 0 };
	sprintf(strKey,KEY_VIRTUAL_AI.c_str(), pMsgReq->iVirtualID);
	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	if (bExist)
	{
		//局数增加刷新成就等级
		string str = RedisHelper::hget(m_redisCt, strKey, "all_play");
		if (str.size() > 0)
		{
			int iAllCnt = atoi(str.c_str());
			int iAchievel = GetVirtualAchivel(iAllCnt);
			char cValue[32] = { 0 };
			sprintf(cValue, "%d", iAchievel);
			RedisHelper::hset(m_redisCt, strKey, "achieve_point", cValue);
		}

		if (iLen < sizeof(RdSetVirtualAIRtMsgDef) || pMsgReq->iGameID == 0)
		{
			RedisHelper::hincrby(m_redisCt, strKey, "all_play", pMsgReq->iAddPlayCnt);
			RedisHelper::hincrby(m_redisCt, strKey, "all_win", pMsgReq->iAddWinCnt);
			RedisHelper::hincrby(m_redisCt, strKey, "all_durak", pMsgReq->iAddDurakCnt);
			return;
		}

		char cFeild[128] = { 0 };

		sprintf(cFeild, "game%d_iWin", pMsgReq->iGameID);
		RedisHelper::hincrby(m_redisCt, strKey, cFeild, pMsgReq->iAddWinCnt);
		sprintf(cFeild, "game%d_iPlayCnt", pMsgReq->iGameID);
		RedisHelper::hincrby(m_redisCt, strKey, cFeild, pMsgReq->iAddPlayCnt);

		RedisHelper::hincrby(m_redisCt, strKey, "exp", pMsgReq->iExp);

		sprintf(cFeild, "game%d_iBufferA0", pMsgReq->iGameID);
		RedisHelper::hset(m_redisCt, strKey, cFeild, to_string(pMsgReq->iBufferA0));
		sprintf(cFeild, "game%d_iBufferA1", pMsgReq->iGameID);
		RedisHelper::hset(m_redisCt, strKey, cFeild, to_string(pMsgReq->iBufferA1));
		sprintf(cFeild, "game%d_iBufferA2", pMsgReq->iGameID);
		RedisHelper::hset(m_redisCt, strKey, cFeild, to_string(pMsgReq->iBufferA2));
		sprintf(cFeild, "game%d_iBufferA3", pMsgReq->iGameID);
		RedisHelper::hset(m_redisCt, strKey, cFeild, to_string(pMsgReq->iBufferA3));
		sprintf(cFeild, "game%d_iBufferA4", pMsgReq->iGameID);
		RedisHelper::hset(m_redisCt, strKey, cFeild, to_string(pMsgReq->iBufferA4));

		sprintf(cFeild, "game%d_iBufferB0", pMsgReq->iGameID);
		RedisHelper::hincrby(m_redisCt, strKey, cFeild, pMsgReq->iBufferB0);
		sprintf(cFeild, "game%d_iBufferB1", pMsgReq->iGameID);
		RedisHelper::hincrby(m_redisCt, strKey, cFeild, pMsgReq->iBufferB1);
		sprintf(cFeild, "game%d_iBufferB2", pMsgReq->iGameID);
		RedisHelper::hincrby(m_redisCt, strKey, cFeild, pMsgReq->iBufferB2);
		sprintf(cFeild, "game%d_iBufferB3", pMsgReq->iGameID);
		RedisHelper::hincrby(m_redisCt, strKey, cFeild, pMsgReq->iBufferB3);
		sprintf(cFeild, "game%d_iBufferB4", pMsgReq->iGameID);
		RedisHelper::hincrby(m_redisCt, strKey, cFeild, pMsgReq->iBufferB4);

		char cVal[512] = {0};
		sprintf(cFeild, "game%d_strBuffer0", pMsgReq->iGameID);
		memcpy(cVal, msgData + sizeof(RdSetVirtualAIRtMsgDef), pMsgReq->isBuffer1Len);
		RedisHelper::hset(m_redisCt, strKey, cFeild, cVal);

		sprintf(cFeild, "game%d_strBuffer1", pMsgReq->iGameID);
		memset(cVal, 0, sizeof(cVal));
		memcpy(cVal, msgData + sizeof(RdSetVirtualAIRtMsgDef) + pMsgReq->isBuffer1Len, pMsgReq->isBuffer2Len);
		RedisHelper::hset(m_redisCt, strKey, cFeild, cVal);
	}
}

int MaillHandler::GetVirtualAchivel(int iAllCnt)
{
	int iCnt[18] = {0, 80, 200, 500, 800, 1000, 1500, 2000, 2700, 3500, 4000, 5000, 6000, 7500, 9000, 12000, 15000, 20000};
	for (int i = 0; i < 18; i++)
	{
		int iMin = 0;
		if (i > 0)
		{
			iMin = iCnt[i - 1];
		}
		if (iAllCnt > iMin && iAllCnt <= iCnt[i])
		{
			int iMainLv = i / 3 + 1;
			int iSubLv = i % 3 + 1;
			return iMainLv * 100 + iSubLv;;
		}
	}
	return 603;
}


void MaillHandler::HandleCtrlAIStatus(char *msgData, int iSocketIndex)
{
	RdCtrlAIStatusMsgDef* pMsgReq = (RdCtrlAIStatusMsgDef*)msgData;
	if (pMsgReq->iGameID > 0)  //释放控制ai
	{
		_log(_DEBUG, "TEST", "HandleCtrlAIStatus game[%d] ai[%d]", pMsgReq->iGameID, pMsgReq->iVirtualID);
		map<int, vector<int> >::iterator itorUse = m_mapInUseCtrlAI.find(pMsgReq->iGameID);
		if (itorUse != m_mapInUseCtrlAI.end())
		{
			vector<int>::iterator itorSub = find(itorUse->second.begin(), itorUse->second.end(), pMsgReq->iVirtualID);
			if (itorSub != itorUse->second.end())
			{
				itorUse->second.erase(itorSub);
			}
		}

		map<int, vector<int> >::iterator itor = m_mapControlAIID.find(pMsgReq->iGameID);
		if (itor != m_mapControlAIID.end())
		{
			if (std::find(itor->second.begin(), itor->second.end(), pMsgReq->iVirtualID) == itor->second.end())
			{
				itor->second.push_back(pMsgReq->iVirtualID);
			}

			char strKey[32] = { 0 };
			sprintf(strKey,KEY_VIRTUAL_AI.c_str(), pMsgReq->iVirtualID);
			bool bExist = RedisHelper::exists(m_redisCt, strKey);
			if (bExist)
			{
				char cField[32] = { 0 };
				sprintf(cField, "game%d_usering", pMsgReq->iGameID);
				RedisHelper::hset(m_redisCt, strKey, cField, "0");
			}

			sprintf(strKey, KEY_IN_USE_CTRLAI.c_str(), pMsgReq->iGameID);
			char cMember[32];
			sprintf(cMember, "%d", pMsgReq->iVirtualID);
			RedisHelper::zrem(m_redisCt, strKey, cMember);
		}
	}
}
