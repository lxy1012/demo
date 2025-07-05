#include "NCenterHandler.h"
#include "RedisHelper.h"
#include <time.h>
#include "Util.h"
#include "log.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include "conf.h"

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/


NCenterHandler::NCenterHandler(MsgQueue *pSendQueue)
{
	m_pSendQueue = pSendQueue;
	for (int i = 0; i < 5; i++)
	{
		m_iRoomType[i] = 4 + i;
	}
}

void NCenterHandler::HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex)
{
	if (m_redisCt == NULL)
	{
		_log(_ERROR, "AssignTableHandler::HandleMsg", "error m_redisCt == NULL");
		return;
	}
	else if (iMsgType == GAME_REDIS_GET_USER_INFO_MSG)
	{
		HandleGetUserInfoReq(szMsg, iLen,iSocketIndex);
	}
	else if (iMsgType == REDID_NCENTER_SET_LATEST_USER_MSG)
	{
		HandleSetLatestUserMsg(szMsg, iSocketIndex);
	}
}


void NCenterHandler::CallBackOnTimer(int iTimeNow)
{
	
}

void  NCenterHandler::HandleGetUserInfoReq(char* szMsg, int iLen, int iSocketIndex)
{
	GameRedisGetUserInfoReqDef* pMsgReq = (GameRedisGetUserInfoReqDef*)szMsg;
	//信息请求标记位，第1位是否需要近期同桌信息，第2位是否获取该游戏任务信息，第3位是否获取当前装扮信息，第4位是否需要获取通用活动任务信息
	int iIfNeedAssignInfo = pMsgReq->iReqFlag & 0x01;
	if (iIfNeedAssignInfo == 0)
	{
		return;
	}
	char cBuffer[1024];
	memset(cBuffer,0,sizeof(cBuffer));
	int iBufferLen = sizeof(RDNCGetUserRoomResDef);
	RDNCGetUserRoomResDef * pMsgRes = (RDNCGetUserRoomResDef*)cBuffer;
	pMsgRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgRes->msgHeadInfo.iMsgType = REDIS_RETURN_USER_ROOM_MSG;
	pMsgRes->msgHeadInfo.iSocketIndex = iSocketIndex;
	pMsgRes->iUserID = pMsgReq->iUserID;
	
	char strKey[64];
	sprintf(strKey, "mj_game_%d_assign:%d", pMsgReq->iGameID, pMsgRes->iUserID);
	bool bExsist = RedisHelper::exists(m_redisCt, strKey);
	if (!bExsist)
	{
		//_log(_ERROR, "NCenterHandler", "HandleGetUserRoomInfoMsg user[%d],game[%d],server[%d] roomInfo not exsist", pMsgReq->iUserID, pMsgReq->iGameID, pMsgReq->iServerID);
		if (m_pSendQueue)
			m_pSendQueue->EnQueue(cBuffer, iBufferLen);
		return;
	}
	char strField[64];
	string strFieldTemp[11];
	map<string, string> mapFieldTemp;
	int iIndex = 0;
	strFieldTemp[iIndex++] = "last_player_index";
	for (int i = 0; i < 10; i++)
	{
		sprintf(strField, "latest_play_user%d", i);
		strFieldTemp[iIndex + i] = strField;
	}
	iIndex += 10;
	for (int i = 0; i < iIndex; i++)
	{
		mapFieldTemp[strFieldTemp[i]] = "nil";
	}
	RedisHelper::hmget(m_redisCt, strKey, mapFieldTemp);
	int *pInt = &pMsgRes->iLastPlayerIndex;
	for (int i = 0; i < iIndex; i++)
	{
		_log(_DEBUG, "CSM", "HandleGetUserRoomInfoMsg user[%d],i[%d],key[%s],value[%s]", pMsgReq->iUserID, i,strFieldTemp[i].c_str(), mapFieldTemp[strFieldTemp[i]].c_str());
		if (!mapFieldTemp[strFieldTemp[i]].empty() && strcmp(mapFieldTemp[strFieldTemp[i]].c_str(),"nil") != 0)
		{
			int iValue = atoi(mapFieldTemp[strFieldTemp[i]].c_str());
			*pInt = iValue;
		}
		pInt++;
	}
	_log(_DEBUG, "NCenterHandler", "HandleGetUserRoomInfoMsg user[%d],playIndex[%d],player[%d,%d,%d]", pMsgReq->iUserID, pMsgRes->iLastPlayerIndex, pMsgRes->iLatestPlayUser[0], pMsgRes->iLatestPlayUser[1], pMsgRes->iLatestPlayUser[2]);
	if (m_pSendQueue)
		m_pSendQueue->EnQueue(cBuffer, iBufferLen);
}


void  NCenterHandler::HandleSetLatestUserMsg(char *msgData, int iSocketIndex)
{
	RDNCSetLatestUserDef* pMsgReq = (RDNCSetLatestUserDef*)msgData;
	char strKey[64];
	char cFiled[32];
	char cValue[32];
	map<string, string> mapFieldTemp;
	for (int i = 0; i < 10; i++)
	{
		if (pMsgReq->iUserID[i] > 0)
		{
			mapFieldTemp.clear();
			int iIndex = pMsgReq->iLatestIndex[i];
			sprintf(strKey, "mj_game_%d_assign:%d", pMsgReq->iJudgeGameID, pMsgReq->iUserID[i]);
			if (iIndex < 0)
			{
				iIndex = 0;
			}
			else if (iIndex > 9)
			{
				iIndex = 9;
			}
			for (int j = 0; j < 5; j++)
			{
				if (pMsgReq->iUserID[j] > 0 && pMsgReq->iUserID[j] != pMsgReq->iUserID[i])
				{
					sprintf(cFiled, "latest_play_user%d", iIndex);
					sprintf(cValue,"%d", pMsgReq->iUserID[j]);
					mapFieldTemp[cFiled] = cValue;
					iIndex++;
					if (iIndex > 9)
					{
						iIndex = 0;
					}
				}
			}
			sprintf(cValue, "%d", iIndex);
			mapFieldTemp["last_player_index"] = cValue;
			RedisHelper::hmset(m_redisCt, strKey, mapFieldTemp);
			RedisHelper::expire(m_redisCt, strKey, 7*24*3600);
		}
	}
	/*_log(_DEBUG, "NCenterHandler", "HandleSetLatestUserMsg user[%d:%d,%d:%d,%d:%d,%d:%d,%d:%d]", pMsgReq->iUserID[0], pMsgReq->iLatestIndex[0],
		pMsgReq->iUserID[1], pMsgReq->iLatestIndex[1],
		pMsgReq->iUserID[2], pMsgReq->iLatestIndex[2],
		pMsgReq->iUserID[3], pMsgReq->iLatestIndex[3],
		pMsgReq->iUserID[4], pMsgReq->iLatestIndex[4]);*/
}