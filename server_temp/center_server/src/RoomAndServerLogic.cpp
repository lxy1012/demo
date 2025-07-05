#include "RoomAndServerLogic.h"
#include <time.h>
#include "Util.h"
#include "log.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include "conf.h"
#include <algorithm>
#include <string.h>
#include "ClientAuthenHandler.h"
#include "AssignTableHandler.h"

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/

RoomAndServerLogic::RoomAndServerLogic(int type, void *pCallBase)
{
	m_iCenterServerState = -1;
	if (type == CALL_ASSIGN)
	{
		strcpy(m_cCallFrom, "AssignHandler");
		m_pAssign = (AssignTableHandler*)pCallBase;
	}
	else
	{
		strcpy(m_cCallFrom, "ClientHandler");
		m_pClient = (ClientAuthenHandler*)pCallBase;
	}
}

void RoomAndServerLogic::HandleGameRoomInfoReqMsg(char* msgData, int iSocketIndex)
{
	int iLastCenterServerState = m_iCenterServerState;
	GameRoomInfoResRadiusDef* pMsgPre = (GameRoomInfoResRadiusDef*)msgData;
	GameOneRoomBriefDef* pRoomBrifef = (GameOneRoomBriefDef*)(msgData + sizeof(GameRoomInfoResRadiusDef) + sizeof(GameServerIPInfoDef) * pMsgPre->iBaseServerNum);
	m_iCenterServerState = pMsgPre->iServerState;
	_log(_DEBUG, "RAS", "[%s]:HandleGameRoomInfoReqMsg room[%d],m_iCenterServerState[%d]", m_cCallFrom, pMsgPre->iRoomNum, m_iCenterServerState);
	if (iLastCenterServerState != m_iCenterServerState)
	{
		_log(_ERROR, "RAS", "[%s]:HandleGameRoomInfoReqMsg room[%d],stateChanged[%d-->%d]", m_cCallFrom, pMsgPre->iRoomNum, iLastCenterServerState,m_iCenterServerState);
	}
	//记录所有房间的必需配置信息
	for (int i = 0; i < pMsgPre->iRoomNum; i++)
	{
		_log(_DEBUG, "RAS", "[%s]:HandleGameRoomInfoReqMsg i[%d],room[%d],state[%d],iIPLimit[%d]", m_cCallFrom, i, pRoomBrifef->iRoomType, pRoomBrifef->iRoomState, pRoomBrifef->iSpeFlag1);
		map<int, AssignRoomConfDef>::iterator pos = m_mapRoomConfInfo.find(pRoomBrifef->iRoomType);
		if (pos == m_mapRoomConfInfo.end())
		{
			AssignRoomConfDef  stRoomConf;
			memset(&stRoomConf, 0, sizeof(AssignRoomConfDef));
			stRoomConf.iIfFreeRoom = pRoomBrifef->iRoomType;
			stRoomConf.iRoomState = pRoomBrifef->iRoomState;
			stRoomConf.iIPLimit = pRoomBrifef->iIfIPLimit;
			m_mapRoomConfInfo[pRoomBrifef->iRoomType] = stRoomConf;
		}
		else
		{
			AssignRoomConfDef* pRoomConf = &pos->second;
			pRoomConf->iIfFreeRoom = pRoomBrifef->iRoomType;
			pRoomConf->iRoomState = pRoomBrifef->iRoomState;
			pRoomConf->iIPLimit = pRoomBrifef->iIfIPLimit;
		}
		pRoomBrifef++;
	}
}

int RoomAndServerLogic::HandleGameServerAuthenMsg(char* msgData, int iSocketIndex)
{
	GameRDAuthenReqPreDef* pAuthenReq = (GameRDAuthenReqPreDef*)msgData;
	_log(_DEBUG, "RAS", "[%s]:HandleGameServerAuthenMsg iSocketIndex[%d],serverid[%d]", m_cCallFrom, iSocketIndex, pAuthenReq->iServerID);
	if (iSocketIndex > 0) //游戏服务器连接
	{
		map<int, OneGameServerDetailInfoDef>::iterator pos = m_mapSubGameServerInfo.find(pAuthenReq->iServerID);
		if (pos != m_mapSubGameServerInfo.end())
		{
			_log(_ERROR, "RAS", "[%s]:HandleGameServerAuthenMsg connect server[%d] have existed", m_cCallFrom, pAuthenReq->iServerID);
			m_mapSubGameServerInfo.erase(pos);
		}
		//else
		{
			OneGameServerDetailInfoDef stOneServerInfo;
			memset(&stOneServerInfo, 0, sizeof(OneGameServerDetailInfoDef));
			stOneServerInfo.iServerID = pAuthenReq->iServerID;
			m_mapSubGameServerInfo[pAuthenReq->iServerID] = stOneServerInfo;
			SortAllSubServers();
		}
	}
	else  //游戏服务器断开了
	{
		iSocketIndex = abs(iSocketIndex);
		map<int, OneGameServerDetailInfoDef>::iterator iter = m_mapSubGameServerInfo.begin();
		int iServerID = 0;
		while (iter != m_mapSubGameServerInfo.end())
		{
			if (iter->second.iSocketIndex == iSocketIndex)
			{
				iServerID = iter->first;
				break;
			}
			iter++;
		}
		if (iServerID > 0)
		{
			m_mapSubGameServerInfo.erase(iter);
			_log(_DEBUG, "RAS", "ServerDisnnect iServerID[%d]", iServerID);
			return iServerID;
		}
		else
		{
			_log(_DEBUG, "RAS", "ServerDisnnect not find iServerID[%d]", iServerID);
			return pAuthenReq->iServerID;
		}
	}
	return pAuthenReq->iServerID;
}

void RoomAndServerLogic::HandleRefreshLeftTabNum(char* msgData, int iSocketIndex)
{
	NCenterTabNumMsgDef* pTbNumMsg = (NCenterTabNumMsgDef*)msgData;
	map<int, OneGameServerDetailInfoDef>::iterator pos = m_mapSubGameServerInfo.find(pTbNumMsg->iServerID);
	_log(_DEBUG, "RAS", "[%s]:HandleRefreshLeftTabNum,iServerID[%d],usedtb[%d] max[%d]", m_cCallFrom, pTbNumMsg->iServerID, pTbNumMsg->iCurNum, pTbNumMsg->iMaxNum);
	if (pos == m_mapSubGameServerInfo.end())
	{
		_log(_ERROR, "RAS", "HandleRefreshLeftTabNum  no find server[%d]", pTbNumMsg->iServerID);
		return;
	}

	int iServerCrowdRate = 100;
	if (pTbNumMsg->iMaxNum > 0)
	{
		iServerCrowdRate = pTbNumMsg->iCurNum * 100 / pTbNumMsg->iMaxNum;
	}
	OneGameServerDetailInfoDef* pServerDetail = &pos->second;
	pServerDetail->iUsedTabNum = pTbNumMsg->iCurNum;
	pServerDetail->iMaxTab = pTbNumMsg->iMaxNum;
	pServerDetail->iServerCrowdRate = iServerCrowdRate;
	SortAllSubServers();
}

void RoomAndServerLogic::HandleGameServerDetailMsg(char* msgData, int iSocketIndex)
{
	GameSysDetailToCenterMsgDef* pDetailMsg = (GameSysDetailToCenterMsgDef*)msgData;
	map<int, OneGameServerDetailInfoDef>::iterator pos = m_mapSubGameServerInfo.find(pDetailMsg->iServerID);
	_log(_DEBUG, "RAS", "[%s]:HandleGameServerDetailMsg,iServerID[%d],time[%d,%d],stata[%d] Inner[%d][%d]", m_cCallFrom, pDetailMsg->iServerID, pDetailMsg->iBeginTime, 
		pDetailMsg->iOpenTime, pDetailMsg->iServerState, pDetailMsg->iInnerIP, pDetailMsg->sInnerPort);
	if (pos == m_mapSubGameServerInfo.end())
	{
		_log(_ERROR, "RAS", "HandleGameServerDetailMsg  no find server[%d]", pDetailMsg->iServerID);
		return;
	}
	int iOldMaxNum = pDetailMsg->iMaxNum;
	OneGameServerDetailInfoDef* pServerDetail = &pos->second;
	pServerDetail->iIP = pDetailMsg->iIP;
	pServerDetail->sPort = pDetailMsg->sPort;
	pServerDetail->iBeginTime = pDetailMsg->iBeginTime;
	pServerDetail->iOpenTime = pDetailMsg->iOpenTime;
	pServerDetail->iMaxNum = pDetailMsg->iMaxNum;
	pServerDetail->iServerState = pDetailMsg->iServerState;
	pServerDetail->iSocketIndex = iSocketIndex;
	pServerDetail->iInnerIP = pDetailMsg->iInnerIP;
	pServerDetail->sInnerPort = pDetailMsg->sInnerPort;
	if (iOldMaxNum != pServerDetail->iMaxNum)
	{
		//int iServerCrowdRate = 100;
		//if (pServerDetail->iMaxNum > 0)
		//{
		//	iServerCrowdRate = pServerDetail->iCurrentNum * 100 / pServerDetail->iMaxNum;
		//}
		//pServerDetail->iServerCrowdRate = iServerCrowdRate;
		SortAllSubServers();
	}
}

void RoomAndServerLogic::HandleGameServerSysOnlineMsg(char* msgData, int iSocketIndex)
{
	GameSysOnlineToCenterMsgDef* pMsgReq = (GameSysOnlineToCenterMsgDef*)msgData;
	map<int, OneGameServerDetailInfoDef>::iterator pos = m_mapSubGameServerInfo.find(pMsgReq->iServerID);
	_log(_DEBUG, "RAS", "[%s]:HandleGameServerSysOnlineMsg,iServerID[%d],iMaxNum[%d],nowNum[%d]", m_cCallFrom, pMsgReq->iServerID, pMsgReq->iMaxNum, pMsgReq->iNowPlayerNum);
	if (pos == m_mapSubGameServerInfo.end())
	{
		_log(_ERROR, "RAS", "HandleGameServerSysOnlineMsg  no find server[%d]", pMsgReq->iServerID);
		return;
	}
	OneGameServerDetailInfoDef* pServerDetail = &pos->second;
	int iOldOnlineNum = pServerDetail->iCurrentNum;
	int iOldMaxNum = pServerDetail->iMaxNum;
	pServerDetail->iMaxNum = pMsgReq->iMaxNum;
	pServerDetail->iCurrentNum = pMsgReq->iNowPlayerNum;
	if (iOldOnlineNum != pServerDetail->iCurrentNum || iOldMaxNum != pServerDetail->iMaxNum)
	{
		//int iServerCrowdRate = 100;
		//if (pServerDetail->iMaxNum > 0)
		//{
		//	iServerCrowdRate = pServerDetail->iCurrentNum * 100 / pServerDetail->iMaxNum;
		//}
		//pServerDetail->iServerCrowdRate = iServerCrowdRate;
		SortAllSubServers();
	}
}

int RoomAndServerLogic::IfRoomHaveIPLimit(int iIfFreeRoom)
{
	int iLimit = 0;
	map<int, AssignRoomConfDef>::iterator iter = m_mapRoomConfInfo.find(iIfFreeRoom);
	if (iter != m_mapRoomConfInfo.end())
	{
		iLimit = iter->second.iIPLimit;
	}
	return iLimit;
}

int RoomAndServerLogic::JudgeIfServerCanEnter(int iServerID)
{
	int iState = 0;
	map<int, OneGameServerDetailInfoDef>::iterator pos = m_mapSubGameServerInfo.find(iServerID);
	if (pos == m_mapSubGameServerInfo.end())
	{
		//_log(_DEBUG, "RAS", "JudgeIfServerCanEnter no find iServerID[%d]", iServerID);
		return iState;
	}
	if (pos->second.iServerState != 1)
	{
		iState = pos->second.iServerState;
		//_log(_DEBUG, "RAS", "JudgeIfServerCanEnter_2 iServerID[%d],iState[%d]", iServerID, iState);
		return iState;
	}
	bool bRt = JudgeServerOpen(pos->second.iBeginTime, pos->second.iOpenTime, time(NULL));
	if (!bRt)
	{
		iState = 0;
		//_log(_DEBUG, "RAS", "JudgeIfServerCanEnter_3 iServerID[%d],iState[%d]", iServerID, iState);
		return iState;
	}
	return pos->second.iServerState;
}

bool RoomAndServerLogic::JudgeServerOpen(int iBeginTime, int iLongTime, int iNowTime)
{
	time_t tmNowTime = iNowTime;
	struct tm tm_tTime;
	tm_tTime = *localtime(&tmNowTime);
	if (iLongTime == 24) //24小时开放的
	{
		return true;
	}
	else
	{
		int iCloseTime = iBeginTime + iLongTime;
		if (iCloseTime > 24)
		{
			iCloseTime -= 24;
			if (tm_tTime.tm_hour < iCloseTime || tm_tTime.tm_hour >= iBeginTime)
			{
				return true;
			}
		}
		else
		{
			if (iBeginTime <= tm_tTime.tm_hour && tm_tTime.tm_hour < iCloseTime)
			{
				return true;
			}
		}
	}
	return false;
}

bool RoomAndServerLogic::SortFullServer(OneGameServerDetailInfoDef* pFirst, OneGameServerDetailInfoDef* pSecond)
{
	//返回false交换位置，返回true不交换位置
	int iRate = 85;
	if ((pFirst->iServerCrowdRate < iRate && pSecond->iServerCrowdRate < iRate) || (pFirst->iServerCrowdRate >= iRate && pSecond->iServerCrowdRate >= iRate))
	{
		if (pFirst->iServerCrowdRate < pSecond->iServerCrowdRate)
		{
			return false;
		}
		return true;
	}
	if (pFirst->iServerCrowdRate >= iRate && pSecond->iServerCrowdRate < iRate)
	{
		return false;
	}
	return true;
}

OneGameServerDetailInfoDef* RoomAndServerLogic::GetServerInfo(int iServerID)
{
	map<int, OneGameServerDetailInfoDef>::iterator iterServer = m_mapSubGameServerInfo.find(iServerID);
	if (iterServer == m_mapSubGameServerInfo.end())
	{
		return NULL;
	}
	return &iterServer->second;
}

OneGameServerDetailInfoDef * RoomAndServerLogic::GetServerInfo(uint iIP, short sPort)
{
	if (iIP == 0)
	{
		return NULL;
	}
	map<int, OneGameServerDetailInfoDef>::iterator iterServer = m_mapSubGameServerInfo.begin();
	while (iterServer != m_mapSubGameServerInfo.end())
	{
		if ((iterServer->second.iIP == iIP && iterServer->second.sPort == sPort) || (iterServer->second.iInnerIP == iIP && iterServer->second.sInnerPort == sPort))
		{
			return &iterServer->second;
		}
		iterServer++;
	}
	
	return NULL;
}

OneGameServerDetailInfoDef* RoomAndServerLogic::GetCommendServerInfo()
{
	OneGameServerDetailInfoDef* pServerInfo = NULL;
	for (int i = 0; i < m_vcAllSubGameServer.size(); i++)
	{
		int iServerState = JudgeIfServerCanEnter(m_vcAllSubGameServer[i]->iServerID);
		if (iServerState == 1 && (m_vcAllSubGameServer[i]->iMaxTab == 0 || m_vcAllSubGameServer[i]->iUsedTabNum < m_vcAllSubGameServer[i]->iMaxTab))
		{
			pServerInfo = m_vcAllSubGameServer[i];
			break;
		}
	}
	_log(_DEBUG, "RAS", "GetCommendServerInfo pServerInfo[%d],allSub[%d]", pServerInfo == NULL ? 0 : pServerInfo->iServerID, m_vcAllSubGameServer.size());
	return pServerInfo;
}

OneGameServerDetailInfoDef* RoomAndServerLogic::GetMinimumServerInfo()
{
	int iMinimum = 0;
	OneGameServerDetailInfoDef* pServerInfo = NULL;
	for (int i = 0; i < m_vcAllSubGameServer.size(); i++)
	{
		int iServerState = JudgeIfServerCanEnter(m_vcAllSubGameServer[i]->iServerID);
		if (iServerState == 1)
		{
			if (iMinimum == 0 || m_vcAllSubGameServer[i]->iUsedTabNum < iMinimum)
			{
				pServerInfo = m_vcAllSubGameServer[i];
				iMinimum = pServerInfo->iUsedTabNum;
				if (i > 0)
				{
					break;
				}
			}
		}
		else
		{
			_log(_DEBUG, "RAS", "GetMinimumServerInfo server[%d],iServerState[%d]", m_vcAllSubGameServer[i]->iServerID, iServerState);
		}
	}
	_log(_DEBUG, "RAS", "GetMinimumServerInfo pServerInfo[%d],allSub[%d]", pServerInfo == NULL ? 0 : pServerInfo->iServerID, m_vcAllSubGameServer.size());
	return pServerInfo;
}

AssignRoomConfDef* RoomAndServerLogic::GetAssignRoomConf(int iRoomType)
{
	map<int, AssignRoomConfDef>::iterator iter = m_mapRoomConfInfo.find(iRoomType);
	if (iter == m_mapRoomConfInfo.end())
	{
		return NULL;
	}
	return &iter->second;
}

OneGameServerDetailInfoDef* RoomAndServerLogic::GetMaintainServerInfo()
{
	OneGameServerDetailInfoDef* pServerInfo = NULL;
	for (int i = 0; i < m_vcAllSubGameServer.size(); i++)
	{
		if (m_vcAllSubGameServer[i]->iServerState == 2)
		{
			pServerInfo = m_vcAllSubGameServer[i];
			break;
		}
	}
	_log(_DEBUG, "RAS", "GetMaintainServerInfo pServerInfo[%d],allSub[%d]", pServerInfo == NULL ? 0 : pServerInfo->iServerID, m_vcAllSubGameServer.size());
	return pServerInfo;
}


void RoomAndServerLogic::SortAllSubServers()
{
	m_iAllPlayerNum = 0;
	m_vcAllSubGameServer.clear();
	map<int, OneGameServerDetailInfoDef>::iterator pos = m_mapSubGameServerInfo.begin();
	while (pos != m_mapSubGameServerInfo.end())
	{
		m_vcAllSubGameServer.push_back(&pos->second);
		m_iAllPlayerNum += pos->second.iCurrentNum;
		pos++;
	}
	if (!m_vcAllSubGameServer.empty())
	{
		sort(m_vcAllSubGameServer.begin(), m_vcAllSubGameServer.end(), SortFullServer);
	}
}

void RoomAndServerLogic::SetSendQueue(MsgQueue * pSendQueue)
{
	m_pSendQueue = pSendQueue;
}
