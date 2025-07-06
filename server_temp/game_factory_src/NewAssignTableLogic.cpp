
#include "NewAssignTableLogic.h"

#include "log.h"

#include "conf.h"        //从文件中读入配置
#include "aeslib.h"
#include "gamefactorycomm.h"
#include "factorymessage.h"
#include "gamelogic.h"
#include <stdlib.h>

NewAssignTableLogic::NewAssignTableLogic()
{
	m_pGameLogic = NULL;
	ReadConf();
	m_iConfCDTime = 600;
}

NewAssignTableLogic::~NewAssignTableLogic()
{
}

void NewAssignTableLogic::Ini(GameLogic *pGameLogic)
{
	m_pGameLogic = pGameLogic;
}

void NewAssignTableLogic::HandleNetMsg(int iMsgType, void *pMsgData, int iMsgLen)
{
	if (iMsgType == NEW_CENTER_GET_USER_SIT_MSG)
	{
		HandleGetUserSitMsg(pMsgData);
	}
	else if (iMsgType == NEW_CNNTER_USER_SIT_ERR)
	{
		HandleUserSitErrMsg(pMsgData);
	}
	else if (iMsgType == NEW_CENTER_USER_CHANGE_MSG)
	{
		HandleUserChangeReq(pMsgData);
	}
	else if (iMsgType == SYS_CHANGE_SERVER_MSG)
	{
		HandleClientChangeReq(pMsgData);
	}
	else if (iMsgType == NEW_CENTER_REQ_USER_LEAVE_MSG)
	{
		HandleRequireUserLeaveReq(pMsgData);
	}
	else if (iMsgType == NEW_CENTER_VIRTUAL_ASSIGN_MSG)
	{
		HandleVirtualAssignMsg(pMsgData);
	}
	else if (iMsgType == RD_GET_VIRTUAL_AI_INFO_MSG)
	{
		HandleGetVirtualAIMsg(pMsgData, iMsgLen);
	}
	else if (iMsgType == RD_GET_VIRTUAL_AI_RES_MSG)
	{
		HandleGetVirtualAIResMsg(pMsgData, iMsgLen);
	}
}
int NewAssignTableLogic::GetIfHaveRoomNum(int iUserID)
{
	int iRoomNum = 0;
	for (int i = 0; i < m_vcTableUsers.size(); i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (m_vcTableUsers[i].iUserID[j] == iUserID)
			{
				iRoomNum = m_vcTableUsers[i].iIfFreeRoom[j];
				break;
			}
		}
		if (iRoomNum > 0)
		{
			break;
		}
	}
	return iRoomNum;
}

int NewAssignTableLogic::CheckIfInWaitTable(int iUserID, bool bLeave)
{
	int iIndex = -1;
	int iUserIndex = -1;
	for (int i = 0; i < m_vcTableUsers.size(); i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (m_vcTableUsers[i].iUserID[j] == iUserID)
			{
				iIndex = i;
				iUserIndex = j;
				break;
			}
		}
		if (iIndex != -1)
		{
			break;
		}
	}
	if (!bLeave)
	{
		return iIndex;
	}
	//本桌还未真正入座就有人离开了，通知中心服务器，其他人都要重新开始配桌了
	if (bLeave)
	{
		if (iIndex != -1)
		{
			m_vcTableUsers[iIndex].bIfInServer[iUserIndex] = false;
			DismissWaitTableByIndex(iIndex);
		}
	}
	return iIndex;
}

void NewAssignTableLogic::CheckIfNeedSitDirectly(int iUserID)
{
	for (int i = 0; i < m_vcTableUsers.size(); i++)
	{
		//_log(_DEBUG, "NewATable", "CheckIfNeedSitDirectly:i[%d-%d,%d-%d,%d-%d,%d-%d,%d-%d]", 
		//	m_vcTableUsers[i].iUserID[0], m_vcTableUsers[i].bIfInServer[0], 
		//	m_vcTableUsers[i].iUserID[1], m_vcTableUsers[i].bIfInServer[1],
		//	m_vcTableUsers[i].iUserID[2], m_vcTableUsers[i].bIfInServer[2],
		//	m_vcTableUsers[i].iUserID[3], m_vcTableUsers[i].bIfInServer[3],
		//	m_vcTableUsers[i].iUserID[4], m_vcTableUsers[i].bIfInServer[4]);
		for (int j = 0; j < 10; j++)
		{
			if (m_vcTableUsers[i].iUserID[j] == iUserID && !m_vcTableUsers[i].bIfInServer[j])
			{
				m_vcTableUsers[i].bIfInServer[j] = true;
				FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
				if (nodePlayers != NULL)
				{
					int iTableNum = m_vcTableUsers[i].iTableNum;
					int iOldVTableSize = m_vcTableUsers.size();
					if (iTableNum == -1)  //换服前没确定桌子，这里找一次
					{
						iTableNum = m_pGameLogic->GetValidTableNum();
						_log(_DEBUG, "NewATable", "CheckIfNeedSitDirectly user[%d] sit find table[%d]", iUserID, iTableNum);
						if (iTableNum <= 0)
						{
							_log(_ERROR, "NewATable", "CheckIfNeedSitDirectly user[%d] sit error not find table", iUserID);
						}						
					}
					bool bSitOK = m_pGameLogic->CallBackUsersSit(m_vcTableUsers[i].iUserID[j], m_vcTableUsers[i].iIfFreeRoom[j], iTableNum, j, m_vcTableUsers[i].iNeedPlayerNum);
					int iNewVTableSize = m_vcTableUsers.size();
					if (iNewVTableSize == iOldVTableSize && m_vcTableUsers[i].iTableNum == -1 && bSitOK)
					{
						_log(_DEBUG, "NewATable", "CheckIfNeedSitDirectly user[%d] sit find table[%d] succ", iUserID, iTableNum);
						m_vcTableUsers[i].iTableNum = iTableNum;
					}
					if (!bSitOK)  //如果入座失败，当前等待桌直接解散掉去重新配桌
					{
						DismissWaitTableByIndex(i);
					}
				}
				break;
			}
		}		
	}
}

void NewAssignTableLogic::DismissWaitTableByIndex(int iIndex)
{
	if (iIndex < 0 || iIndex >= m_vcTableUsers.size())
	{
		return;
	}
	char cTmp[256] = { 0 };
	for (int j = 0; j < 10; j++)
	{
		char c[16] = { 0 };
		sprintf(c, "%d_%d,", m_vcTableUsers[iIndex].iUserID[j], m_vcTableUsers[iIndex].bIfInServer[j]);
		strcat(cTmp, c);
	}
	_log(_DEBUG, "NewATable", "DismissWaitTableByIndex:iIdx[%d]user[%s]", iIndex, cTmp);

	bool bNoticeCnter = false;
	for (int j = 0; j < 10; j++)
	{
		if (m_vcTableUsers[iIndex].iUserID[j] > 0 && m_vcTableUsers[iIndex].bIfInServer[j])
		{
			FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&m_vcTableUsers[iIndex].iUserID[j], sizeof(int)));
			if (nodePlayers != NULL)
			{					
				if (nodePlayers->cTableNum > 0)//已经入座的用户要从桌子上离开
				{
					int iTableNum = nodePlayers->cTableNum;
					int iTableNumExtra = nodePlayers->cTableNumExtra;
					int iRoomIndex = nodePlayers->cRoomNum - 1;
					FactoryTableItemDef *pTableItem = m_pGameLogic->GetTableItem(iRoomIndex, iTableNum - 1);
					if (pTableItem != NULL)
					{
						m_pGameLogic->RemoveTablePlayer(pTableItem, iTableNumExtra);
					}					
					nodePlayers->cTableNum = 0;
					nodePlayers->cTableNumExtra = -1;

					//通知中心配桌失败
					if (!bNoticeCnter)
					{
						bNoticeCnter = true;
						NCenterSyncUserSitErrDef msgSitErr;
						memset(&msgSitErr, 0, sizeof(NCenterSyncUserSitErrDef));
						msgSitErr.msgHeadInfo.cVersion = MESSAGE_VERSION;
						msgSitErr.msgHeadInfo.iMsgType = NEW_CNNTER_USER_SIT_ERR;
						msgSitErr.iNVTableID = pTableItem->iNextVTableID;
						memset(msgSitErr.iSitRt, -1, sizeof(msgSitErr.iSitRt));
						memcpy(msgSitErr.iUsers, m_vcTableUsers[iIndex].iUserID, sizeof(int) * 10);
						if (m_pGameLogic->m_pQueToCenterServer)
						{
							m_pGameLogic->m_pQueToCenterServer->EnQueue(&msgSitErr, sizeof(NCenterSyncUserSitErrDef));
						}
						CallBackFreeVTableID(msgSitErr.iNVTableID);
					}
				}
				nodePlayers->iStatus = PS_WAIT_DESK;
				if (m_pGameLogic->CheckIfPlayWithControlAI(nodePlayers))  //需要进控制的玩家这里直接通知中心离开，不给中心配桌的机会，否则可能会因为进控制和中心分配冲突
				{
					CallBackUserLeave(m_vcTableUsers[iIndex].iUserID[j], nodePlayers->iVTableID);//通知中心服务器删除该玩家
					SitDownReqDef sitReq;
					memset(&sitReq, 0, sizeof(sitReq));
					m_pGameLogic->HandleSitDownReq(nodePlayers->iUserID, &sitReq);
				}
				else
				{
					CallBackUserLeave(-m_vcTableUsers[iIndex].iUserID[j], 0);//先通知中心服务器重置可配置状态
					if (nodePlayers->iUserID > g_iMaxVirtualID)
					{
						//发送入座请求
						CallBackUserNeedAssignTable(m_vcTableUsers[iIndex].iUserID[j]);
					}
				}				
			}		
		}
	}
	m_vcTableUsers.erase(m_vcTableUsers.begin() + iIndex);
}

void NewAssignTableLogic::HandleGetUserSitMsg(void *pMsgData)
{
	bool bTableOK = true;
	string strLog = "";
	char cLog[64] = { 0 };
	NCenterUserSitRes *pMsg = (NCenterUserSitRes*)pMsgData;
	int iSitRt[10] = { 0 };//1本服查无此人 2状态不对，用户已经被分配好了
	int iMaxPlayerNum = 0;
	for (int i = 0; i < 10; i++) 
	{
		if (pMsg->iUsers[i] > 0)
		{
			int iInWaitIndex = CheckIfInWaitTable(pMsg->iUsers[i],false);
			if (iInWaitIndex != -1)//被重复分配了（虚拟ai也不应该被重复分配），这次分配的直接解散
			{
				strLog = "";
				for (int j = 0; j < 10; j++)
				{
					sprintf(cLog, "%d_%d,", m_vcTableUsers[iInWaitIndex].iUserID[j], pMsg->iUsers[j]);
					strLog += cLog;
				}
				_log(_ERROR, "NewATable", "HandleGetUserSitMsg:iInWaitIndex[%d],%s", iInWaitIndex, strLog.c_str());
				iSitRt[i] = 2;
				bTableOK = false;
			}
			iMaxPlayerNum++;
		}
		else if (pMsg->iUsers[i] < 0)   //AI
		{
			iMaxPlayerNum++;
		}
	}
	_log(_DEBUG, "NewATable", "HandleGetUserSitMsg:iAllSitNum[%d]nVT[%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", iMaxPlayerNum, pMsg->iNextVTableID,
		pMsg->iUsers[0], pMsg->iUsers[1], pMsg->iUsers[2], pMsg->iUsers[3], pMsg->iUsers[4], pMsg->iUsers[5], pMsg->iUsers[6], pMsg->iUsers[7], pMsg->iUsers[8], pMsg->iUsers[9]);
	if (bTableOK)
	{
		OneTableUsersDef oneTableInfo;
		memset(&oneTableInfo, 0, sizeof(OneTableUsersDef));
		bool bAllInSever = true;
		int iUserNum = 0;
		for (int i = 0; i < 10; i++)
		{
			if (pMsg->iUsers[i] > 0)
			{
				iUserNum++;
				oneTableInfo.iUserID[i] = pMsg->iUsers[i];
				oneTableInfo.iIfFreeRoom[i] = pMsg->iIfFreeRoom[i];
				if (pMsg->iUserServerID[i] == 0 || pMsg->iUserServerID[i] == m_pGameLogic->m_pServerBaseInfo->iServerID)
				{
					//虚拟ai的节点也放在hashUserIDTable内
					FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pMsg->iUsers[i], sizeof(int)));
					if (nodePlayers)
					{
						if (pMsg->iUsers[i] > g_iMaxVirtualID && (nodePlayers->iStatus != PS_WAIT_DESK || nodePlayers->bWaitExit))
						{
							iSitRt[i] = 2;
							_log(_ERROR, "NewATable", "HandleGetUserSitMsg:user[%d]iStatus[%d] != PS_WAIT_DESK", pMsg->iUsers[i], nodePlayers->iStatus);
							bTableOK = false;
						}
						else
						{
							oneTableInfo.bIfInServer[i] = true;
						}						
					}
					else
					{
						if (CheckUserIfVAI(pMsg->iUsers[i]))//新分配的虚拟AI,本服还没有对应的ai信息
						{
							bAllInSever = false;
						}
						else
						{
							_log(_ERROR, "NewATable", "HandleGetUserSitMsg:user[%d] not in this server", pMsg->iUsers[i]);
							iSitRt[i] = 1;
							bTableOK = false;
						}				
					}
				}			
				else
				{
					bAllInSever = false;
				}
			}
		}
		int iTableNum = m_pGameLogic->GetValidTableNum();

		_log(_DEBUG, "NewATable", "HandleGetUserSitMsg:iUserNum[%d],bTableOK[%d],bAllInSever[%d],user[%d,%d,%d,%d,%d],iTableNum[%d] maxPlayerNum[%d]", iUserNum, bTableOK, bAllInSever,
			pMsg->iUsers[0], pMsg->iUsers[1], pMsg->iUsers[2], pMsg->iUsers[3], pMsg->iUsers[4], iTableNum, iMaxPlayerNum);
		if (iTableNum <= 0)//没找到可用的桌子，直接解散重新配置
		{
			_log(_ERROR, "NewATable", "HandleGetUserSitMsg user[%d,%d,%d,%d,%d] not find table!!!", pMsg->iUsers[0], pMsg->iUsers[1], pMsg->iUsers[2], pMsg->iUsers[3], pMsg->iUsers[4]);
			bTableOK = false;
		}
		FactoryTableItemDef* pTableItem = m_pGameLogic->GetTableItem(0, iTableNum-1);
		if (pTableItem != NULL)
		{
			pTableItem->iNextVTableID = pMsg->iNextVTableID;
		}

		oneTableInfo.iTablePlayType = pMsg->iTablePlayType;
		int iPlayTypeNum = oneTableInfo.iTablePlayType >> 24;   //桌子最大人数
		if (iPlayTypeNum == 0)
		{
			iPlayTypeNum = (oneTableInfo.iTablePlayType >> 16) & 0xff;  //最小人数
		}

		//根据玩家是否在虚拟桌上，调整玩家位置，并通知原虚拟桌上玩家，该玩家已离开
		if (bTableOK)
		{
			ResetTableNumExtra(&oneTableInfo, pMsg->iNextVTableID, iUserNum);
		}
		int iSuccTable = -1;
		if (iUserNum == iMaxPlayerNum && bTableOK && bAllInSever)//人齐了而且都在本服，都可以直接入座了
		{
			for (int i = 0; i < 10; i++)
			{				
				if (oneTableInfo.iUserID[i] > 0)
				{
					bool bSitOK = m_pGameLogic->CallBackUsersSit(oneTableInfo.iUserID[i], oneTableInfo.iIfFreeRoom[i], iTableNum, i, iPlayTypeNum, iUserNum);
					iSuccTable = bSitOK ? iTableNum : 0;
				}
			}
			return;
		}
		else if (iUserNum == iMaxPlayerNum && bTableOK)//要等其他玩家换服
		{
			//本服的玩家也先入座
			for (int i = 0; i < 10; i++)
			{
				if (oneTableInfo.iUserID[i] > 0 && oneTableInfo.bIfInServer[i])
				{
					bool bSitOK = m_pGameLogic->CallBackUsersSit(oneTableInfo.iUserID[i], oneTableInfo.iIfFreeRoom[i], iTableNum, i, iPlayTypeNum, iUserNum);
					iSuccTable = bSitOK ? iTableNum : 0;
				}
				else if (CheckUserIfVAI(oneTableInfo.iUserID[i]))
				{
					//需要通过redis获取对应的虚拟AI信息，等回来后再入座
					SendGetVirtualAIInfoReq(oneTableInfo.iUserID[i], 0);
				}	
			}
			oneTableInfo.iNeedPlayerNum = iUserNum;
			oneTableInfo.iTableNum = iSuccTable;   //换服玩家这里先不做换服，等待第一个换服玩家入座后赋值
			oneTableInfo.iWaitSec = m_iTableWaitSec;
			oneTableInfo.iTablePlayType = pMsg->iTablePlayType;
			m_vcTableUsers.push_back(oneTableInfo);
			_log(_DEBUG, "NewATable", "HandleGetUserSitMsg:wait table[%d] user 0:user[%d]in[%d]room[%d],1:user[%d]in[%d]room[%d]", iSuccTable,
				oneTableInfo.iUserID[0], oneTableInfo.bIfInServer[0], oneTableInfo.iIfFreeRoom[0],
				oneTableInfo.iUserID[1], oneTableInfo.bIfInServer[1], oneTableInfo.iIfFreeRoom[1]);
		}
	}
	if (!bTableOK)
	{	
		NCenterSyncUserSitErrDef msgSitErr;
		memset(&msgSitErr,0,sizeof(NCenterSyncUserSitErrDef));
		msgSitErr.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgSitErr.msgHeadInfo.iMsgType = NEW_CNNTER_USER_SIT_ERR;
		msgSitErr.iNVTableID = pMsg->iNextVTableID;
		for (int j = 0; j < 10; j++)
		{
			msgSitErr.iUsers[j] = pMsg->iUsers[j];
			if (pMsg->iUsers[j] == 0)
			{
				break;
			}
			//有异常的用户要通知中心服务器，无异常用户要申请重新配桌
			if (iSitRt[j] > 0)
			{
				msgSitErr.iSitRt[j] = iSitRt[j];
			}		
		}
		if (m_pGameLogic->m_pQueToCenterServer)
		{
			m_pGameLogic->m_pQueToCenterServer->EnQueue(&msgSitErr, sizeof(NCenterSyncUserSitErrDef));
		}
		CallBackFreeVTableID(msgSitErr.iNVTableID);
		for (int j = 0; j < 10; j++)
		{
			if (pMsg->iUserServerID[j] == m_pGameLogic->m_pServerBaseInfo->iServerID && msgSitErr.iSitRt[j] != 2)//没有配桌成功，本服的玩家直接重新请求配桌
			{
				CallBackUserNeedAssignTable(pMsg->iUsers[j]);
			}
		}
	}
}

void NewAssignTableLogic::CallBackUserNeedAssignTable(FactoryPlayerNodeDef *nodePlayers,bool bFromSitReq)
{
	if (nodePlayers == NULL)
	{
		return;
	}
	if (nodePlayers->bWaitExit || nodePlayers->iStatus > PS_WAIT_READY)
	{
		_log(_ERROR, "NewATable", "CallBackUserNeedAssignTable iUserID[%d] bWaitExit[%d] istatus[%d]", nodePlayers->iUserID, nodePlayers->bWaitExit, nodePlayers->iStatus);
		return;
	}

	int iReqAssignCnt = 1;
	int iVTableID = nodePlayers->iVTableID;
	if (iVTableID < 0)
	{
		nodePlayers->iVTableID = -iVTableID;
	}
	NCenterUserSitReqDef msgSitReq;
	memset(&msgSitReq,0,sizeof(NCenterUserSitReqDef));
	msgSitReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgSitReq.msgHeadInfo.iMsgType = NEW_CENTER_GET_USER_SIT_MSG;
	msgSitReq.iUserID = nodePlayers->iUserID;
	msgSitReq.iAllGameCnt = nodePlayers->iRecentPlayCnt;
	msgSitReq.iWinCnt = nodePlayers->iRecentWinCnt;
	msgSitReq.iLoseCnt = nodePlayers->iRecentLoseCnt;
	msgSitReq.iEnterIfFreeRoom = nodePlayers->iEnterRoomType;
	msgSitReq.iServerID = m_pGameLogic->m_pServerBaseInfo->iServerID;
	msgSitReq.iUserMarkLv = nodePlayers->iSpeMark;
	msgSitReq.iReqAssignCnt = iReqAssignCnt;
	msgSitReq.iVipType = nodePlayers->cVipType;
	msgSitReq.iLastPlayerIndex = nodePlayers->iLatestPlayerIndex;
	msgSitReq.iPlayType = nodePlayers->iPlayType;
	msgSitReq.iSpeMark = nodePlayers->iSpeMark;
	for (int i = 0; i < 10; i++)
	{
		msgSitReq.iLatestPlayUser[i] = nodePlayers->iLatestPlayer[i];
	}
	strcpy(msgSitReq.szIP, nodePlayers->szIP);
	if (nodePlayers->bGapDaySitReq)
	{
		nodePlayers->bGapDaySitReq = false;
		msgSitReq.iReqAssignCnt = 0;
	}
	memset(m_cBuffer, 0, sizeof(m_cBuffer));
	int iBufferLen = sizeof(NCenterUserSitReqNewDef);
	NCenterUserSitReqNewDef* pNewSitReq = (NCenterUserSitReqNewDef*)m_cBuffer;
	memcpy(&pNewSitReq->msgSitInfo, &msgSitReq, sizeof(NCenterUserSitReqDef));
	pNewSitReq->iHeadImg = nodePlayers->iHeadImg;
	strcpy(pNewSitReq->szNickName, nodePlayers->szNickName);
	pNewSitReq->iFrameID = nodePlayers->iHeadFrameID;
	int *pInt = (int*)(m_cBuffer + iBufferLen);
	//低位起，第3位携带补发的虚拟桌id	
	if (bFromSitReq)
	{
		pNewSitReq->iExtraInfo |= (1 << 3);
	}
	if (iVTableID < 0)
	{
		pNewSitReq->iExtraInfo |= (1<<2);
		*pInt = -iVTableID;
		pInt++;
		iBufferLen += sizeof(int);
	}
	if (m_pGameLogic->m_pQueToCenterServer)
	{
		m_pGameLogic->m_pQueToCenterServer->EnQueue(m_cBuffer, iBufferLen);
	}
	//_log(_DEBUG, "NewATable", "CallBackUserNeedAssignTable:user[%d],iVTableID[%d]", nodePlayers->iUserID, iVTableID);
}
void NewAssignTableLogic::CallBackUserNeedAssignTable(int iUserID)
{
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID, sizeof(int)));	
	if (nodePlayers == NULL)
	{
		_log(_DEBUG, "GL", "CallBackUserNeedAssignTable iUserID[%d] nodePlayers=null", iUserID);
		return;
	}
	if (nodePlayers->bIfAINode || nodePlayers->bWaitExit || nodePlayers->iStatus > PS_WAIT_DESK)
	{
		_log(_DEBUG, "GL", "CallBackUserNeedAssignTable iUserID[%d] bIfAINode[%d] bWaitExit[%d] status[%d]", iUserID, nodePlayers->bIfAINode, nodePlayers->bWaitExit, nodePlayers->iStatus);
		return;
	}
	//_log(_DEBUG, "GL", "CallBackUserNeedAssignTable--CallBackUserNeedAssignTable");
	CallBackUserNeedAssignTable(nodePlayers);
}

void NewAssignTableLogic::HandleUserSitErrMsg(void *pMsgData)//处理由中心服务器转发过来的其他服用户的异常信息
{
	NCenterSyncUserSitErrDef* pMsg = (NCenterSyncUserSitErrDef*)pMsgData;
	int iIndex = -1;
	for (int i = 0; i < 10; i++)
	{
		if (pMsg->iUsers[i] > 0)
		{
			for (int j = 0; j < m_vcTableUsers.size(); j++)
			{
				for (int m = 0; m < 10; m++)
				{
					if (m_vcTableUsers[j].iUserID[m] == pMsg->iUsers[i])
					{
						iIndex = j;
						break;
					}
				}
				if (iIndex != -1)
				{
					break;
				}
			}
			if (iIndex != -1)//一次发过来的肯定都是分配在一桌的，判断一个用户就够了
			{
				break;
			}
		}
	}
	if (iIndex == -1)
	{
		_log(_ERROR, "NewATable", "HandleUserSitErrMsg:user[%d,%d,%d,%d,%d] not in wait table", pMsg->iUsers[0], pMsg->iUsers[1], pMsg->iUsers[2], pMsg->iUsers[3], pMsg->iUsers[4]);
		return;
	}
	//在本服的玩家重新申请配桌,然后该桌解散
	for (int i = 0; i < 10; i++)
	{
		if (m_vcTableUsers[iIndex].iUserID [i]> g_iMaxVirtualID && m_vcTableUsers[iIndex].bIfInServer[i])
		{
			CallBackUserNeedAssignTable(m_vcTableUsers[iIndex].iUserID[i]);
		}
	}
	m_vcTableUsers.erase(m_vcTableUsers.begin()+ iIndex);
}

void NewAssignTableLogic::CallBackOneSec(long iTime)
{
	m_iConfCDTime--;
	if (m_iConfCDTime < 0)
	{
		ReadConf();
		m_iConfCDTime = 600;
	}
	for (int i = 0; i < m_vcTableUsers.size();)
	{
		m_vcTableUsers[i].iWaitSec--;
		if (m_vcTableUsers[i].iWaitSec <= 0)
		{
			DismissWaitTableByIndex(i);
			continue;
		}
		i++;
	}
	//直接以桌为单位进行配桌请求
	if (!m_mapAllVTables.empty())
	{
		map<int, VirtualTableDef*>::iterator iter = m_mapAllVTables.begin();
		while (iter != m_mapAllVTables.end())
		{
			VirtualTableDef* pVTable = iter->second;
			if (pVTable->iVTableID > g_iMaxFdRoomID && pVTable->iCDSitTime > 0)
			{
				pVTable->iCDSitTime--;
				//_log(_DEBUG, "NewATable", "CallBackOneSec VTable[%d],cd[%d],size[%d]", iter->first, iter->second->iCDSitTime, m_mapAllVTables.size());
				if (pVTable->iCDSitTime <= 0)//时间到了还没被分配到桌子，就再发一次
				{
					pVTable->iCDSitTime = m_iWaitTableResSec;
					int iAssginIdx = -1;
					for (int i = 0; i < 10; i++)
					{
						if (pVTable->iUserID[i] > 0 && CheckUserIfVAI(pVTable->iUserID[i]) == false)
						{
							iAssginIdx = i;
							break;
						}
					}
					//如果玩家已经不在了，不要再请求
					if (iAssginIdx >= 0)
					{
						int iUserID = pVTable->iUserID[iAssginIdx];
						FactoryPlayerNodeDef *pAssginPlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
						if (pAssginPlayers != NULL)
						{
							//_log(_DEBUG, "NewATable", "CallBackOneSec VTable[%d],iUserID[%d]", iter->first, iUserID);
							CallBackUserNeedAssignTable(iUserID);
						}
						else
						{
							_log(_ERROR, "NewATable", "CallBackOneSec VTable[%d],iUserID[%d] is null", iter->first, iUserID);
							pVTable->iUserID[iAssginIdx] = 0;
						}
					}
				}
			}	
			if (pVTable->iVTableID <= g_iMaxFdRoomID)
			{	
				if (pVTable->iVTableID < 0)
				{
					m_mapAllVTables.erase(iter++);
					_log(_DEBUG, "NewATable", "CallBackOneSec:iVTableID[%d],leftSize[%d],iter[%d]", pVTable->iVTableID, m_mapAllVTables.size(), iter== m_mapAllVTables.end()?0:iter->first);
					continue;
				}
			}
			iter++;
		}
	}
	for (int i = 0; i < m_vcAIControlVTables.size();)
	{
		if (m_vcAIControlVTables[i]->iCTLSendCTLAIInfoTime > 0)
		{
			m_vcAIControlVTables[i]->iCTLSendCTLAIInfoTime--;
			if (m_vcAIControlVTables[i]->iCTLSendCTLAIInfoTime == 0)
			{
				//判断是否全部入座完成
				if (m_vcAIControlVTables[i]->iAIInfoSendDone == m_vcAIControlVTables[i]->iMinPlayerNum -1)
				{
					//全部入座完成，通知开始游戏
					bool bStart = CheckVTableReadySatus(m_vcAIControlVTables[i], false);
					if (bStart)
					{
						m_vcAIControlVTables.erase(m_vcAIControlVTables.begin() + i);
					}
				}
				else
				{
					//当前虚拟桌没有完成入座
					m_vcAIControlVTables[i]->iCTLSendCTLAIInfoTime = rand() % 3 + 1;
					i++;
				}
			}
			else
			{
				i++;
			}
		}
		else
		{
			i++;
		}
	}

}

void NewAssignTableLogic::ReadConf()
{
	GetValueInt(&m_iTableWaitSec, "wait_other_cd_sec", "new_assign_table.conf", "System Base Info", "5");
	GetValueInt(&m_iWaitTableResSec,"wait_assgin_res_sec", "new_assign_table.conf", "System Base Info", "2");
	GetValueInt(&m_iRateMethod, "rate_method", "new_assign_table.conf", "System Base Info", "0");
	GetValueInt(&m_iLowRate, "low_rate", "new_assign_table.conf", "System Base Info", "45");
	GetValueInt(&m_iHightRate, "high_rate", "new_assign_table.conf", "System Base Info", "55");
}

void NewAssignTableLogic::HandleClientChangeReq(void *pMsgData)
{
	ClientChangeServerDef* pMsg = (ClientChangeServerDef*)pMsgData;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pMsg->iUserID, sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "NewATable", "HandleClientChangeReq:user[%d] not in this server", pMsg->iUserID);
		return;
	}

	if (nodePlayers->iStatus > PS_WAIT_READY)
	{
		KickOutServerDef kickout;
		memset(&kickout, 0, sizeof(KickOutServerDef));
		kickout.cKickType = g_iLockInOtherServer;
		kickout.cClueType = 0;//提示类型（0仅提示，1退出房间，2退出游戏返回大厅）
		m_pGameLogic->CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &kickout, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
		return;
	}

	_log(_DEBUG, "NewATable", "HandleClientChangeReq:user[%d] iNewServerID[%d] roomNO[%d] iSocketIndex[%d]", pMsg->iUserID, pMsg->iNewServerID, pMsg->iRoomNo, nodePlayers->iSocketIndex);

	ClientChangeUserDef data;
	memset(&data, 0, sizeof(ClientChangeUserDef));
	data.iRoomNo = pMsg->iRoomNo;
	data.iSocketIndex = nodePlayers->iSocketIndex;
	data.iUserID = pMsg->iUserID;
	data.iServerID = pMsg->iNewServerID;
	m_mapClientChangeUsers[pMsg->iUserID] = data;

	//用户要先发送离开计费，否则新服会提示卡房间,而且必须在收到计费回应后才能换服（否则新服的登录请求和旧服的里离开计费请求无法确定哪个会先到计费服务器）
	m_pGameLogic->OnFindDeskDisconnect(pMsg->iUserID, NULL, true);
}

void NewAssignTableLogic::HandleUserChangeReq(void *pMsgData)
{
	NCenterUserChangeMsgDef* pMsg = (NCenterUserChangeMsgDef*)pMsgData;
	int iErrRt[10] = { 0 };
	bool bHaveErr = false;
	for (int i = 0; i < 10; i++)
	{
		if (pMsg->iUserID[i] == 0)
		{
			break;
		}
		int iUserID = pMsg->iUserID[i];
		FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
		if (nodePlayers == NULL)
		{
			_log(_ERROR, "NewATable", "HandleUserChangeReq:user[%d] not in this server", iUserID);
			iErrRt[i] = 1;
			bHaveErr = true;
		}
		else if (nodePlayers->iStatus != PS_WAIT_DESK)   //如果不是等待配桌或不是好友房内未开局，直接返回失败
		{
			_log(_ERROR, "NewATable", "HandleUserChangeReq:user[%d]status[%d] err", iUserID, nodePlayers->iStatus);
			iErrRt[i] = 2;
			bHaveErr = true;
		}	
		if (iErrRt[i] == 0)
		{
			NAChangeServerUserDef stChangeServer;
			memset(&stChangeServer, 0, sizeof(NAChangeServerUserDef));
			stChangeServer.iUserID = iUserID;
			stChangeServer.iSocketIndex = nodePlayers->iSocketIndex;
			stChangeServer.iNewServerIP = pMsg->iNewServerIP;
			stChangeServer.iNewServerPort = pMsg->iNewServerPort;
			stChangeServer.iNewInnerIP = pMsg->iNewInnerIP;
			stChangeServer.iNewInnerPort = pMsg->iNewInnerPort;
			_log(_DEBUG, "NewATable", "HandleUserChangeReq:user[%d] iNewServerIP[%d] iNewServerPort[%d] iSocketIndex[%d] Inner[%d][%d]", iUserID, pMsg->iNewServerIP, pMsg->iNewServerPort, nodePlayers->iSocketIndex, pMsg->iNewInnerIP, pMsg->iNewInnerPort);
			m_mapChangeServerUsers[iUserID] = stChangeServer;
		
			//用户要先发送离开计费，否则新服会提示卡房间,而且必须在收到计费回应后才能换服（否则新服的登录请求和旧服的里离开计费请求无法确定哪个会先到计费服务器）
			m_pGameLogic->OnFindDeskDisconnect(iUserID, NULL, true);
		}
	}
	if (bHaveErr && pMsg->iNewServerSocket > 0)
	{
		NCenterSyncUserSitErrDef msgSitErr;
		memset(&msgSitErr,0,sizeof(NCenterSyncUserSitErrDef));
		msgSitErr.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgSitErr.msgHeadInfo.iMsgType = NEW_CNNTER_USER_SIT_ERR;
		msgSitErr.iSocketIndex = pMsg->iNewServerSocket;
		msgSitErr.iNVTableID = -1;
		for (int j = 0; j < 10; j++)
		{
			if (pMsg->iUserID[j] == 0)
			{
				break;
			}
			//有异常的用户要通知中心服务器
			if (iErrRt[j] > 0)
			{
				msgSitErr.iUsers[j] = pMsg->iUserID[j];
				msgSitErr.iSitRt[j] = iErrRt[j];
			}
		}
		if (m_pGameLogic->m_pQueToCenterServer)
		{
			m_pGameLogic->m_pQueToCenterServer->EnQueue(&msgSitErr, sizeof(NCenterSyncUserSitErrDef));
		}
	}
}

void  NewAssignTableLogic::HandleRequireUserLeaveReq(void* pMsgData)
{
	//因服务器关闭/维护，需要离开
	NCenterReqUserLeaveMsgDef* pMsgReq = (NCenterReqUserLeaveMsgDef*)pMsgData;
	int iUserID = pMsgReq->iUserID;
	FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(m_pGameLogic->hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "NewATable", "HandleRequireUserLeaveReq:user[%d] not in this server", iUserID);
		return;
	}
	_log(_DEBUG, "NewATable", "HandleRequireUserLeaveReq:user[%d] iReqType[%d] iNewServerIP[%d] iNewServerPort[%d]", iUserID, pMsgReq->iReqType, pMsgReq->iNewServerIP, pMsgReq->iNewServerPort);
	if (pMsgReq->iReqType == 2 && nodePlayers->iStatus == PS_WAIT_DESK)//0服务器不在开放时间内，1服务器进入维护状态且无其他可切换服务器，2服务器进入维护状态需要切换至指定服务器
	{
		//模拟下换桌请求
		NCenterUserChangeMsgDef msgSimReq;
		memset(&msgSimReq,0,sizeof(NCenterUserChangeMsgDef));
		msgSimReq.iUserID[0] = iUserID;
		msgSimReq.iNewServerIP = pMsgReq->iNewServerIP;
		msgSimReq.iNewServerPort = pMsgReq->iNewServerPort;
		msgSimReq.iNewInnerIP = pMsgReq->iNewInnerIP;
		msgSimReq.iNewInnerPort = pMsgReq->iNewInnerPort;
		HandleUserChangeReq(&msgSimReq);
	}
	else
	{
		m_pGameLogic->SendKickOutMsg(nodePlayers, pMsgReq->iReqType==0? g_iServerClosed: g_iServerMaintain,1,0);
	}
}

void NewAssignTableLogic::CallBackUserChangeServer(int iUserID)
{
	map<int, NAChangeServerUserDef>::iterator iter = m_mapChangeServerUsers.find(iUserID);
	if (iter != m_mapChangeServerUsers.end())
	{
		CenterServerRes msgRes;
		memset(&msgRes, 0, sizeof(CenterServerResDef));
		msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgRes.msgHeadInfo.iMsgType = CLIENT_GET_CSERVER_LIST_MSG;
		msgRes.iNum = 1;

		ServerInfoDef serverInfo;
		memset(&serverInfo, 0, sizeof(ServerInfoDef));
		serverInfo.iIP = iter->second.iNewServerIP;
		serverInfo.sPort = iter->second.iNewServerPort;
		serverInfo.iInnerIP = iter->second.iNewInnerIP;
		serverInfo.sInnerPort = iter->second.iNewInnerPort;
		char cMsg[256];
		memset(cMsg, 0, sizeof(cMsg));
		memcpy(cMsg, &msgRes, sizeof(CenterServerRes));
		memcpy(cMsg + sizeof(CenterServerRes), &serverInfo, sizeof(ServerInfoDef));
		_log(_ERROR, "NewATable", "CallBackUserChangeServer:user[%d] iSocketIndex[%d] new[%d][%d] inner[%d][%d]", iUserID, iter->second.iSocketIndex, iter->second.iNewServerIP, iter->second.iNewServerPort, iter->second.iNewInnerIP, iter->second.iNewInnerPort);
		m_pGameLogic->CLSendSimpleNetMessage(iter->second.iSocketIndex, cMsg, CLIENT_GET_CSERVER_LIST_MSG, sizeof(CenterServerRes) + sizeof(ServerInfoDef));
		m_mapChangeServerUsers.erase(iter);
		return;
	}
	map<int, ClientChangeUserDef>::iterator iterClient = m_mapClientChangeUsers.find(iUserID);
	if (iterClient != m_mapClientChangeUsers.end())
	{
		ClientChangeServerDef msgRes;
		memset(&msgRes, 0, sizeof(ClientChangeServerDef));
		msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgRes.msgHeadInfo.iMsgType = SYS_CHANGE_SERVER_MSG;
		msgRes.iUserID = iterClient->second.iUserID;
		msgRes.iRoomNo = iterClient->second.iRoomNo;
		msgRes.iNewServerID = iterClient->second.iServerID;
		_log(_DEBUG, "NewATable", "CallBackUserChangeServer user[%d] iSocketIndex[%d] roomNo[%d] server [%d]", msgRes.iUserID, iter->second.iSocketIndex, msgRes.iRoomNo, msgRes.iNewServerID);
		m_pGameLogic->CLSendSimpleNetMessage(iterClient->second.iSocketIndex, &msgRes, SYS_CHANGE_SERVER_MSG, sizeof(ClientChangeServerDef));
		m_mapClientChangeUsers.erase(iterClient);
		return;
	}
	_log(_ERROR, "NewATable", "CallBackUserChangeServer:user[%d] not found", iUserID);
}

void NewAssignTableLogic::CallBackUserLeave(int iUserID, int iVTableID)
{
	_log(_DEBUG, "NewATable", "CallBackUserLeave:user[%d],iVTableID[%d]", iUserID, iVTableID);
	//不管是不是等待配桌的玩家，都需要通知中心服务器其已离开状态
	NCenterUserLeaveMsgDef msg;
	memset(&msg,0,sizeof(NCenterUserLeaveMsgDef));
	msg.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msg.msgHeadInfo.iMsgType = NEW_CENTER_USER_LEAVE_MSG;
	msg.iUserID = abs(iUserID);
	if (iUserID < 0)//通知中心服务器仅重置配置状态
	{
		msg.iExtraInfo = 1;
		iUserID = abs(iUserID);
	}
	if (m_pGameLogic->m_pQueToCenterServer)
	{
		m_pGameLogic->m_pQueToCenterServer->EnQueue(&msg, sizeof(NCenterUserLeaveMsgDef));
	}
	if (iVTableID != 0)
	{		
		if (iVTableID == g_iControlRoomID)
		{
			int iVCIndex = -1;
			VirtualTable* pVTable = FindAIControlVTableInfo(iUserID, iVCIndex);
			if (pVTable != NULL)
			{
				for (int i = 0; i < 10; i++)
				{
					if (pVTable->pNodePlayers[i] != NULL)
					{
						pVTable->pNodePlayers[i]->iVTableID = 0;
						if (pVTable->pNodePlayers[i]->bIfAINode && pVTable->pNodePlayers[i]->iUserID >= g_iMinControlVID && pVTable->pNodePlayers[i]->iUserID <= g_iMaxControlVID)
						{
							m_pGameLogic->ReleaseControlAINode(pVTable->pNodePlayers[i]);
						}						
					}
				}
				m_vcAIControlVTables.erase(m_vcAIControlVTables.begin() + iVCIndex);
				AddToFreeVTables(pVTable);
			}
		}
		else
		{
			VirtualTable* pVTable = FindVTableInfo(iVTableID);
			if (pVTable == NULL)
			{
				_log(_DEBUG, "NewATable", "CallBackUser[%d]Leave:pVTable[%d] null", iUserID, iVTableID);
			}
			if (pVTable != NULL)
			{
				CheckRemoveVTableUser(iVTableID, iUserID, true);
				if (pVTable->iVTableID > 0)
				{
					CheckVTableReadySatus(pVTable, false);
				}
			}
		}				
	}
}

void NewAssignTableLogic::CallBackTableOK(int iUserID[])
{
	NCenterAssignTable0kMsgDef msg;
	memset(&msg,0,sizeof(NCenterAssignTable0kMsgDef));
	msg.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msg.msgHeadInfo.iMsgType = NEW_CENTER_ASSIGN_TABLE_OK_MSG;
	msg.iServerID = m_pGameLogic->m_pServerBaseInfo->iServerID;
	for (int j = 0; j < 10; j++)
	{
		if (iUserID[j] == 0)
		{
			break;
		}
		msg.iUserID[j] = iUserID[j];
		//改变下每个用户节点内的同桌信息
		FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID[j], sizeof(int)));
		if (nodePlayers != NULL)
		{
			int iIndex = nodePlayers->iLatestPlayerIndex;
			if (iIndex < 0)
			{
				iIndex = 0;
			}
			else if (iIndex > 9)
			{
				iIndex = 9;
			}
			for (int m = 0; m < 10; m++)
			{
				if (iUserID[m] > 0 && iUserID[m] != iUserID[j])
				{
					nodePlayers->iLatestPlayer[iIndex] = iUserID[m];
					iIndex++;
					if (iIndex > 9)
					{
						iIndex = 0;
					}
				}
			}
			nodePlayers->iLatestPlayerIndex = iIndex;
			//计算下几率和几率所在档位
			nodePlayers->iNCAGameRate = 0;
			if (nodePlayers->iRecentPlayCnt > 0)
			{
				if (m_iRateMethod == 0)
				{
					nodePlayers->iNCAGameRate = nodePlayers->iRecentWinCnt * 1000 / nodePlayers->iRecentPlayCnt;
				}
				else
				{
					nodePlayers->iNCAGameRate = (nodePlayers->iRecentPlayCnt - nodePlayers->iRecentLoseCnt) * 1000 / nodePlayers->iRecentPlayCnt;
				}
			}			
		}
	}
	if (m_pGameLogic->m_pQueToCenterServer)
	{
		m_pGameLogic->m_pQueToCenterServer->EnQueue(&msg, sizeof(NCenterAssignTable0kMsgDef));
	}
	for (int i = 0; i < m_vcTableUsers.size(); i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (m_vcTableUsers[i].iUserID[j] == iUserID[0])
			{
				//_log(_DEBUG, "NewATable", "CallBackTableOK:DismissWaitTableByIndex[%d]", i);
				m_vcTableUsers.erase(m_vcTableUsers.begin() + i);
				return;
			}
		}		
	}
}

void NewAssignTableLogic::CallBackHandleUserGetRoomRes(void *pMsgData, int iMsgLen)
{
	RDNCGetUserRoomResDef* pMsg = (RDNCGetUserRoomResDef*)pMsgData;
	int iUserID = pMsg->iUserID;
	_log(_DEBUG, "NewATable", "HandleUserGetRoomRes:user[%d] ,latest[%d:%d,%d,%d]", pMsg->iUserID, pMsg->iLastPlayerIndex, pMsg->iLatestPlayUser[0], pMsg->iLatestPlayUser[1], pMsg->iLatestPlayUser[2]);
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (nodePlayers != NULL)
	{
		nodePlayers->iLatestPlayerIndex = pMsg->iLastPlayerIndex;
		for (int i = 0; i < 10; i++)
		{
			nodePlayers->iLatestPlayer[i] = pMsg->iLatestPlayUser[i];
		}
	}
	else
	{
		_log(_ERROR, "NewATable", "HandleUserGetRoomRes:user[%d] nodePlayers == NULL", iUserID);
		return;
	}
	nodePlayers->iGetRdLatestPlayerRt = 1;
}

void NewAssignTableLogic::CallBackTableSitOk(int iUserID)
{
	for (int i = 0; i < m_vcTableUsers.size(); i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (m_vcTableUsers[i].iUserID[j] == iUserID)
			{
				m_vcTableUsers[i].iWaitSec = 30;//已经入座了，就多留30s等待开始
				return;
			}
		}
	}
}

bool NewAssignTableLogic::CheckIfUserChangeServer(int iUserID)
{
	bool bRet = false;
	for (int i = 0; i < m_vcTableUsers.size(); i++)
	{
		_log(_DEBUG, "NewATable", "CheckIfNeedSitDirectly:i[%d-%d,%d-%d,%d-%d,%d-%d,%d-%d]", 
			m_vcTableUsers[i].iUserID[0], m_vcTableUsers[i].bIfInServer[0], 
			m_vcTableUsers[i].iUserID[1], m_vcTableUsers[i].bIfInServer[1],
			m_vcTableUsers[i].iUserID[2], m_vcTableUsers[i].bIfInServer[2],
			m_vcTableUsers[i].iUserID[3], m_vcTableUsers[i].bIfInServer[3],
			m_vcTableUsers[i].iUserID[4], m_vcTableUsers[i].bIfInServer[4]);
		for (int j = 0; j < 10; j++)
		{
			if (m_vcTableUsers[i].iUserID[j] == iUserID && !m_vcTableUsers[i].bIfInServer[j])
			{
				bRet = true;
				break;
			}
		}		
	}
	return bRet;
}

void NewAssignTableLogic::HandleVirtualAssignMsg(void *pMsgData)
{
	NCenterVirtualAssignMsgDef* pMsg = (NCenterVirtualAssignMsgDef*)pMsgData;
	_log(_DEBUG, "NTL", "HandleVirtualAssignMsg iUserID[%d,%d,%d,%d,%d],vTableID[%d],minPlayer[%d]", pMsg->iUsers[0], pMsg->iUsers[1], pMsg->iUsers[2], pMsg->iUsers[3], pMsg->iUsers[4], pMsg->iVTableID, pMsg->iMinPlayerNum);
	//先遍历所有被分配的真实玩家，是否已经被换了虚拟桌
	for (int i = 0; i < 10; i++)
	{
		if (pMsg->iUsers[i] > g_iMaxVirtualID)
		{
			FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pMsg->iUsers[i], sizeof(int)));
			//_log(_DEBUG, "NTL", "HandleVirtualAssignMsg iUserID[%d], iVTableID[%d]:[%d]", pMsg->iUsers[i], nodePlayers->iVTableID, pMsg->iVTableID);
			if (nodePlayers && nodePlayers->iVTableID > 0 && nodePlayers->iVTableID != pMsg->iVTableID)
			{
				CheckRemoveVTableUser(nodePlayers->iVTableID, pMsg->iUsers[i], true);
			}
		}
	}

	//检测本次虚拟配桌玩家是否合法
	bool bSitError = false;
	NCenterUserAssignErrDef assignErr;
	memset(&assignErr, 0, sizeof(NCenterUserAssignErrDef));
	int iLeftUser = 0;
	for (int i = 0; i < 10; i++)
	{
		if (pMsg->iUsers[i] > g_iMaxVirtualID)
		{
			FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pMsg->iUsers[i], sizeof(int)));
			if (nodePlayers && nodePlayers->iStatus > PS_WAIT_READY)  //玩家已经开局了，通知中心服务器重置玩家配桌状态并把玩家从虚拟桌移掉
			{
				_log(_DEBUG, "NTL", "UserAssignErr vTb[%d] user[%d] iStatus[%d]", pMsg->iVTableID, pMsg->iUsers[i], nodePlayers->iStatus);
				assignErr.iUserID[i] = pMsg->iUsers[i];
				bSitError = true;
				pMsg->iUsers[i] = 0;   //玩家无效直接清理掉
			}
			else if(nodePlayers)
			{
				iLeftUser++;
			}
		}
	}
	if (bSitError)
	{
		assignErr.msgHeadInfo.cVersion = MESSAGE_VERSION;
		assignErr.msgHeadInfo.iMsgType = NEW_CNNTER_USER_ASSIGN_ERR;
		assignErr.iNVTableID = pMsg->iVTableID;
		if (m_pGameLogic->m_pQueToCenterServer)
		{
			m_pGameLogic->m_pQueToCenterServer->EnQueue(&assignErr, sizeof(NCenterUserAssignErrDef));
		}
		if (iLeftUser == 0)
		{
			return;
		}
	}

	VirtualTableDef* pVTable = NULL;
	map<int, VirtualTableDef*>::iterator iter = m_mapAllVTables.find(pMsg->iVTableID);
	if (iter == m_mapAllVTables.end())
	{
		pVTable = GetFreeVTable(pMsg->iVTableID);
		pVTable->iPlayType = pMsg->iPlayType;
		int iPlayCnt = 0;
		//新的虚拟桌，所有玩家入座即可
		for (int i = 0; i < 10; i++)
		{
			if (pMsg->iUsers[i] > 0)
			{
				FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pMsg->iUsers[i], sizeof(int)));				
				if (nodePlayers != NULL)
				{
					pVTable->iUserID[i] = pMsg->iUsers[i];
					pVTable->bNextReadyOK[i] = true;
					pVTable->pNodePlayers[i] = nodePlayers;
					int iOldVTableID = nodePlayers->iVTableID;
					nodePlayers->iVTableID = pMsg->iVTableID;
					iPlayCnt++;
				}
				else if (CheckUserIfVAI(pMsg->iUsers[i]))
				{
					pVTable->iUserID[i] = pMsg->iUsers[i];
					pVTable->pNodePlayers[i] = NULL;//发送虚拟桌信息的时候会对没有信息的虚拟AI先获取个人信息
					iPlayCnt++;
				}
			}
		}
		if (iPlayCnt > 0)
		{
			pVTable->iMinPlayerNum = pMsg->iMinPlayerNum;
			pVTable->iCDSitTime = m_iWaitTableResSec;
			m_mapAllVTables[pMsg->iVTableID] = pVTable;
			pVTable->iIfSendAssignReq = 1;
			//test
			/*map<int, VirtualTableDef*>::iterator iterTemp = m_mapAllVTables.begin();
			while (iterTemp != m_mapAllVTables.end())
			{
				_log(_DEBUG, "NTL", "HandleVirtualAssignMsg,new add VT[%d:%d],user[%d,%d,%d,%d]", iterTemp->first, 
					iterTemp->second->iVTableID, iterTemp->second->iUserID[0], iterTemp->second->iUserID[1], 
					iterTemp->second->iUserID[2], iterTemp->second->iUserID[3]);
				iterTemp++;
			}*/
			//test_end
			if (iPlayCnt > 1)//首次虚拟桌大于1个人才需要发
			{
				SendVirtualTableInfo(pVTable);
			}			
		}
		else
		{
			AddToFreeVTables(pVTable);
		}
		return;
	}
	pVTable = iter->second;
	pVTable->iMinPlayerNum = pMsg->iMinPlayerNum;
	pVTable->iPlayType = pMsg->iPlayType;
	//已存在的虚拟桌，对人员校验
	int iLoop = 0;
	VirtualTableDef stOldVirtualTable;
	memset(&stOldVirtualTable, 0, sizeof(VirtualTableDef));
	memcpy(&stOldVirtualTable, pVTable, sizeof(VirtualTableDef));
	while (true)
	{
		if (iLoop >= 2)
		{
			break;
		}
		//先把新增加的放到空位置
		//注意，第一轮可能没有找到位置，例如当前是ABC,BC可能中心服务器判断离开了（当然理论不应该有这种情况，BC离开时游戏服务器应该已经处理过，极端情况考虑如中心服务器有重启），新分配的是ADE)
		//第一轮有空位的先放下D,通知A玩家BC已离开，再一轮让E入座
		for (int i = 0; i < 10; i++)
		{
			if (pMsg->iUsers[i] > 0)//理论上很少会进入第二轮，不区分第一轮已经找到位置的玩家了，全部遍历一遍吧
			{
				FactoryPlayerNodeDef *nodePlayers = nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pMsg->iUsers[i], sizeof(int)));			
				if (pMsg->iUsers[i] > g_iMaxVirtualID)//真实玩家必须存在
				{					
					if (nodePlayers == NULL)
					{
						_log(_DEBUG, "NewATable", "HandleVirtualAssignMsgl[%d]:i[%d],user[%d] not find", iLoop,i, pMsg->iUsers[i]);
						pMsg->iUsers[i] = 0;
						continue;
					}
				}
				bool bFind = false;
				int iFreeIndex = -1;
				for (int j = 0; j < pVTable->iMinPlayerNum; j++)
				{
					if (pVTable->iUserID[j] == 0 && iFreeIndex == -1)
					{
						iFreeIndex = j;
					}
					if (pVTable->iUserID[j] == pMsg->iUsers[i])
					{
						bFind = true;
					}
				}
				_log(_DEBUG, "NTL", "HandleVirtualAssignMsgl[%d]:i[%d], iUsers[%d],tableMin[%d], iFreeIndex[%d], bFind[%d],iPlayType[%d:%d]", iLoop, i, pMsg->iUsers[i], pVTable->iMinPlayerNum, iFreeIndex, bFind, nodePlayers!=NULL?nodePlayers->iPlayType:-1, pVTable->iPlayType);
				if (!bFind)
				{
					if (iFreeIndex != -1)
					{												
						pVTable->iUserID[iFreeIndex] = pMsg->iUsers[i];
						pVTable->pNodePlayers[iFreeIndex] = nodePlayers;
						if (nodePlayers != NULL)
						{
							int iOldVTableID = nodePlayers->iVTableID;
							nodePlayers->iVTableID = pMsg->iVTableID;						
						}
					}
					else
					{
						_log(_DEBUG, "NewATable", "HandleVirtualAssignMsg[%d,%d,%d,%d,%d],no pos[%d,%d,%d,%d,%d]", pMsg->iUsers[0], pMsg->iUsers[1], pMsg->iUsers[2], pMsg->iUsers[3], pMsg->iUsers[4],
							pVTable->iUserID[0], pVTable->iUserID[1], pVTable->iUserID[2],
							pVTable->iUserID[3], pVTable->iUserID[4]);
					}
				}
			}
		}
		//判断之前虚拟桌上是不是有需要离开的
		if (iLoop == 0)
		{
			int  iNeedLeave = 0;
			for (int i = 0; i < 10; i++)
			{
				if (stOldVirtualTable.iUserID[i] > 0)
				{
					bool bFind = false;
					for (int j = 0; j < 10; j++)
					{
						if (pMsg->iUsers[j] == stOldVirtualTable.iUserID[i])
						{
							bFind = true;
							break;
						}
					}
					if (!bFind)
					{
						stOldVirtualTable.iUserID[i] = -stOldVirtualTable.iUserID[i];//用负值标记要离开
						iNeedLeave++;
					}
					else
					{
						FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&stOldVirtualTable.iUserID[i], sizeof(int)));						
						stOldVirtualTable.pNodePlayers[i] = nodePlayers;
						pVTable->pNodePlayers[i] = nodePlayers;//每次都重置下节点，避免后续还要找节点
					}
				}
			}
			if (iNeedLeave == 0)//没有需要离开的，那么也无需第2轮找位置了
			{
				break;
			}
			//通知原桌上其他玩家有玩家离开
			int iLeaveIndex = 0;
			for (int i = 0; i < 10; i++)
			{
				if (stOldVirtualTable.iUserID[i] < 0)
				{
					CheckRemoveVTableUser(stOldVirtualTable.iVTableID, -stOldVirtualTable.iUserID[i], (iLeaveIndex == iNeedLeave-1)?true:false);
					iLeaveIndex++;
				}
			}
		}
		iLoop++;
	}
	//test
	/*map<int, VirtualTableDef*>::iterator iterTemp = m_mapAllVTables.begin();
	while (iterTemp != m_mapAllVTables.end())
	{
		_log(_DEBUG, "NTL", "HandleVirtualAssignMsg,VT[%d:%d],user[%d,%d,%d,%d]", iterTemp->first,
			iterTemp->second->iVTableID, iterTemp->second->iUserID[0], iterTemp->second->iUserID[1],
			iterTemp->second->iUserID[2], iterTemp->second->iUserID[3]);
		iterTemp++;
	}*/
	//test_end
	//通知桌上所有人最新桌子信息
	SendVirtualTableInfo(pVTable);
}
void NewAssignTableLogic::ReleaseRdCtrlAI(int iVirtualID)
{
	RdCtrlAIStatusMsgDef pMsgReq;
	memset(&pMsgReq, 0, sizeof(pMsgReq));
	pMsgReq.iGameID = m_pGameLogic->m_pServerBaseInfo->iGameID;
	pMsgReq.iVirtualID = iVirtualID;
	pMsgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgReq.msgHeadInfo.iMsgType = RD_USE_CTRL_AI_MSG;
	if (m_pGameLogic->m_pQueToCenterServer)
	{
		m_pGameLogic->m_pQueToCenterServer->EnQueue(&pMsgReq, sizeof(pMsgReq));
	}
}

void NewAssignTableLogic::HandleGetVirtualAIMsg(void *pMsgData,int iMsgLen)
{
	RdGetVirtualAIInfoResDef* pMsgInfo = (RdGetVirtualAIInfoResDef*)pMsgData;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pMsgInfo->iVirtualID, sizeof(int)));
	bool bNewNode = false;
	if (nodePlayers == NULL)
	{
		nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->GetFreeNode());
		if (nodePlayers == NULL || pMsgInfo->iVirtualID == 0)   //获取ai信息失败处理
		{
			VirtualTableDef* pVTable = FindVTableInfo(pMsgInfo->iVTableID);
			if (pMsgInfo->iVirtualID != 0)  //获取控制ai信息失败
			{
				//释放ai节点
				ReleaseRdCtrlAI(pMsgInfo->iVirtualID);
			}
			else if(pMsgInfo->iVTableID > 0 && pMsgInfo->iPlayCnt < 0)  //控制ai用完了，控制失败
			{
				//游戏判断是否可以开始
				if (pVTable == NULL)
				{
					_log(_ERROR, "NewATable", "HandleGetVirtualAIMsg ctrl vtab[%d] not find!", pMsgInfo->iVTableID);
					return;
				}
				for (int i = 0; i < 10; i++)
				{
					if (pVTable->pNodePlayers[i] && pVTable->pNodePlayers[i]->bAIContrl && pVTable->pNodePlayers[i]->bIfAINode)
					{
						FactoryPlayerNodeDef *node = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pVTable->iUserID[i], sizeof(int)));
						CheckRemoveVTableUser(pMsgInfo->iVTableID, -pVTable->iUserID[i], true);
						m_pGameLogic->ReleaseControlAINode(node);
						ReleaseRdCtrlAI(pMsgInfo->iVirtualID);
					}
					else if(pVTable->pNodePlayers[i] && !pVTable->pNodePlayers[i]->bIfAINode)
					{
						//受控玩家标记删除
						pVTable->pNodePlayers[i]->bAIContrl = false;
					}
				}
				CheckVTableReadySatus(pVTable, false);
			}
			return;
		}
		nodePlayers->iUserID = pMsgInfo->iVirtualID;
		//对虚拟AI进行信息赋值
		nodePlayers->iSocketIndex = -1;
		nodePlayers->iExp = pMsgInfo->iExp;
		nodePlayers->iAllNum = pMsgInfo->iPlayCnt;
		nodePlayers->iWinNum = pMsgInfo->iWinCnt;
		nodePlayers->iBuffB0 = pMsgInfo->iDurakCnt;
		m_pGameLogic->CallBackSetRobotNick(nodePlayers->szNickName, nodePlayers->iGender, nodePlayers->iUserID);
		nodePlayers->iAchieveLevel = pMsgInfo->iAchieveLevel;
		nodePlayers->iHeadFrameID = pMsgInfo->iHeadFrame;
		nodePlayers->bIfAINode = true;
		if (nodePlayers->iUserID >= g_iMinControlVID && nodePlayers->iUserID <= g_iMaxControlVID)
		{
			nodePlayers->bAIContrl = true;
		}
		m_pGameLogic->hashUserIDTable.Add((void *)(&(nodePlayers->iUserID)), sizeof(int), nodePlayers);
		m_pGameLogic->SetAIBaseUserInfo(nodePlayers);
		m_pGameLogic->SetAIUserGameInfo(nodePlayers, -1, pMsgData, iMsgLen);
		bNewNode = true;
	}
	_log(_DEBUG, "NewATable", "HandleGetVirtualAIMsg[%d],VT[%d],all[%d],win[%d],buff[%d],ach[%d],bNewNode[%d],img[%d] ahv[%d]", 
		pMsgInfo->iVirtualID, pMsgInfo->iVTableID, pMsgInfo->iPlayCnt, pMsgInfo->iWinCnt, pMsgInfo->iDurakCnt, pMsgInfo->iAchieveLevel, bNewNode, nodePlayers->iHeadImg, pMsgInfo->iAchieveLevel);
	if (!bNewNode && !nodePlayers->bAIContrl)
	{
		return;
	}
	else
	{
		nodePlayers->iStatus = PS_WAIT_DESK;
	}
	//如果是之前虚拟桌需要的虚拟AI信息，给对应的虚拟桌发送入座信息
	VirtualTableDef* pVTable = NULL;
	if (pMsgInfo->iVTableID > 0 && !nodePlayers->bAIContrl)
	{
		pVTable = FindVTableInfo(pMsgInfo->iVTableID);
	}
	if (pVTable != NULL || nodePlayers->bAIContrl)
	{
		if (pVTable == NULL)
		{
			_log(_ERROR, "NewATable", "HandleGetVirtualAIMsg CTLPlayer[%d] No VTable", pMsgInfo->iVTableID);
			return;
		}
		bool bFind = false;
		
		for (int i = 0; i < 10; i++)
		{
			if (pVTable->iUserID[i] == nodePlayers->iUserID)
			{
				pVTable->pNodePlayers[i] = nodePlayers;
				nodePlayers->iVTableID = pVTable->iVTableID;
				bFind = true;
				break;
			}
		}	
		if (nodePlayers->bAIContrl)
		{
			bFind = true;
		}
		if (bFind)
		{
			if (!nodePlayers->bAIContrl)
			{
				SendVirtualTableInfo(pVTable);
			}
			else  //控制ai信息获取成功，找个空位坐下
			{
				int iFreePos = -1;
				FactoryPlayerNodeDef *nodePlayer = NULL;
				int iNowAllNum = 0;
				for (int i = 0; i < 10; i++)
				{
					if (pVTable->pNodePlayers[i] != NULL)
					{
						iNowAllNum++;
					}
					else if (iFreePos == -1)
					{
						iFreePos = i;
					}
				}
				pVTable->iUserID[iFreePos] = nodePlayers->iUserID;
				pVTable->pNodePlayers[iFreePos] = nodePlayers;
				pVTable->pNodePlayers[iFreePos]->iVTableID = pVTable->iVTableID;
				pVTable->bNextReadyOK[iFreePos] = true;
				pVTable->pNodePlayers[iFreePos]->bAIContrl = true;
				pVTable->pNodePlayers[iFreePos]->iStatus = PS_WAIT_DESK;
			
				SendVirtualTableInfo(pVTable);
				iNowAllNum++;
				_log(_DEBUG, "NewATable", "HandleGetVirtualAIMsg:user[%d],addAI[%d],iPlayGameCnt[%d-%d]", nodePlayer != NULL ? nodePlayer->iUserID : 0, nodePlayers->iUserID, iNowAllNum, pVTable->iMinPlayerNum);
				if (iNowAllNum >= pVTable->iMinPlayerNum)//人齐可以入座
				{
					bool bStart = CheckVTableReadySatus(pVTable, false);
					if (bStart)
					{
						for (int i = 0; i < 10; i++)
						{
							if (pVTable->iUserID[i] > 0)
							{
								pVTable->pNodePlayers[i]->iVTableID = 0;
							}
						}
						//将等待队列中的桌子删掉
						for (int j = 0; j < m_vcAIControlVTables.size(); j++)
						{
							bool bFind = false;
							for (int k = 0; k <10; k++)
							{
								if (m_vcAIControlVTables[j]->pNodePlayers[k] && m_vcAIControlVTables[j]->pNodePlayers[k]->iUserID == nodePlayers->iUserID)
								{
									bFind = true;
								}
							}
							if (bFind)
							{
								m_vcAIControlVTables.erase(m_vcAIControlVTables.begin() + j);
								_log(_DEBUG, "NewATable", "HandleGetVirtualAIMsg remove m_vcAIControlVTables[%d]", pMsgInfo->iVTableID);
								break;
							}
						}

						_log(_DEBUG, "NewATable", "HandleGetVirtualAIMsg[%d] m_vcAIControlVTables[%d]", pMsgInfo->iVirtualID, m_vcAIControlVTables.size());
						return;
					}
					else   //开局失败，重新拿ai信息
					{
						int iIndex = 0;
						if (FindAIControlVTableInfo(nodePlayers->iUserID, iIndex) == NULL)
						{
							m_vcAIControlVTables.push_back(pVTable);
						}
					
					}
				}
				else
				{
					int iIndex = 0;
					if (FindAIControlVTableInfo(nodePlayers->iUserID, iIndex) == NULL)
					{
						m_vcAIControlVTables.push_back(pVTable);
					}
					if (pVTable->iCDSitTime == 0)
					{
						//pVTable->iCDSitTime = 1 + rand() % 3;
						pVTable->iCDSitTime = 20;
					}
					
				}
				
			}
		}	
		_log(_DEBUG, "NewATable", "HandleGetVirtualAIMsg[%d] m_vcAIControlVTables[%d]", pMsgInfo->iVirtualID, m_vcAIControlVTables.size());
		return;
	}
	else if(pMsgInfo->iVTableID > 0)
	{
		_log(_DEBUG, "NewATable", "HandleGetVirtualAIMsg[%d],not find VT[%d]", pMsgInfo->iVirtualID, pMsgInfo->iVTableID);
	}
	_log(_DEBUG, "NewATable", "HandleGetVirtualAIMsg[%d],CheckSitDirectly[%d]", pMsgInfo->iVirtualID, m_vcTableUsers.size());
	//如果是之前等待开桌的虚拟AI
	CheckIfNeedSitDirectly(pMsgInfo->iVirtualID);
}

void NewAssignTableLogic::HandleGetVirtualAIResMsg(void * pMsgData, int iMsgLen)
{
	//游戏服收到中心服务器转发的多个AI信息
	RdGetVirtualAIRtMsg* pMsg = (RdGetVirtualAIRtMsg*)pMsgData;
	//先找对应的虚拟桌
	VirtualTableDef* pVTable = NULL;
	if (pMsg->iVTableID > 0)
	{
		int iIndex = 0;
		pVTable = FindAIControlVTableInfo(pMsg->iVTableID, iIndex);
	}
	vector<int>vcVirtualAIID;
	int iLen = sizeof(RdGetVirtualAIRtMsg);
	for (int i = 0; i < pMsg->iRetNum; i++)
	{
		int iVirtualID = 0;
		memcpy(&iVirtualID, (char*)pMsgData + iLen, sizeof(int));
		vcVirtualAIID.push_back(iVirtualID);
		iLen += 16 * sizeof(int); //16个固定int
		int iBuffer1Len = 0;
		memcpy(&iBuffer1Len, (char*)pMsgData + iLen, sizeof(int));
		iLen += sizeof(int) + iBuffer1Len;//变长string1
		int iBuffer2Len = 0;
		memcpy(&iBuffer1Len, (char*)pMsgData + iLen, sizeof(int));
		iLen += sizeof(int) + iBuffer1Len;//变长string2
	}
	_log(_DEBUG, "NewATable", "HandleGetVirtualAIResMsg pVTable[%d]", pMsg->iVTableID);
	if (pVTable == NULL)
	{
		_log(_ERROR,"NewATable","HandleGetVirtualAIResMsg pVTable[%d] NULL", pMsg->iVTableID);
		if (pMsg->iRetNum >0)
		{
			//没有指定的虚拟桌，通知中心服务器释放掉这批AI
			int iLen = sizeof(RdGetVirtualAIRtMsg);
			for (int i = 0; i <vcVirtualAIID.size(); i++)
			{
				ReleaseRdCtrlAI(vcVirtualAIID[i]);
			}
		}
		return;
	}
	if (pMsg->iRetNum == 0)
	{
		//没有可用的AI，受控玩家直接请求配桌去
		for (int i = 0; i < 10; i++)
		{
			if (pVTable->pNodePlayers[i] && !pVTable->pNodePlayers[i]->bIfAINode)
			{
				pVTable->pNodePlayers[i]->bAIContrl = false;
				CheckVTableReadySatus(pVTable, false);
				//清除队列
				for (int j = 0; j < m_vcAIControlVTables.size(); j++)
				{
					bool bFind = false;
					for (int k = 0; k < 10; k++)
					{
						if (m_vcAIControlVTables[j]->iUserID[k] == pMsg->iVTableID)
						{
							bFind = true;
							break;
						}
					}
					if (bFind)
					{
						m_vcAIControlVTables.erase(m_vcAIControlVTables.begin() + j);
						break;
					}
				}
				return;
			}
		}
	}
	//存在指定虚拟桌，初始化对应AI
	for (int i = 0; i < pMsg->iRetNum; i++)
	{
		int iUserID = vcVirtualAIID[i];
		FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
		bool bIfError = false;
		if (nodePlayers != NULL)
		{
			//当前控制Ai已经被使用，理论上不会出现这种问题
			_log(_ERROR,"NewATable","HandleGetVirtualAIResMsg VirtualAIID Used=[%d]", iUserID);
			bIfError = true;
		}
		//初始化对应AI节点
		nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->GetFreeNode());
		if (nodePlayers == NULL)
		{
			//初始化节点失败，将桌上之前的所有控制AI节点释放，并请求配桌
			_log(_ERROR,"NewATable","HandleGetVirtualAIResMsg user[%d] getfreenode fail", pMsg->iVTableID);
			bIfError = true;
		}
		if (bIfError)
		{
			for (int j = 0; j < 10; j++)
			{
				if (pVTable->pNodePlayers[j] && pVTable->pNodePlayers[j]->bAIContrl && pVTable->pNodePlayers[j]->bIfAINode)
				{
					FactoryPlayerNodeDef *node = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&pVTable->iUserID[j], sizeof(int)));
					CheckRemoveVTableUser(pMsg->iVTableID, -pVTable->iUserID[j], true);
					m_pGameLogic->ReleaseControlAINode(node);
				}
				else if (pVTable->pNodePlayers[j] && !pVTable->pNodePlayers[j]->bIfAINode)
				{
					//受控标记删除
					pVTable->pNodePlayers[j]->bAIContrl = false;
				}
			}
			CheckVTableReadySatus(pVTable, false);
			//通知释放所有占用的AI
			for (int j = 0; j <vcVirtualAIID.size(); j++)
			{
				ReleaseRdCtrlAI(vcVirtualAIID[j]);
			}
			//清除队列
			for (int j = 0; j < m_vcAIControlVTables.size(); j++)
			{
				bool bFind = false;
				for (int k = 0; k < 10; k++)
				{
					if (m_vcAIControlVTables[j]->iUserID[k] == pMsg->iVTableID)
					{
						bFind = true;
						break;
					}
				}
				if (bFind)
				{
					m_vcAIControlVTables.erase(m_vcAIControlVTables.begin() + j);
					break;
				}
			}
			return;
		}
		//初始化节点成功
		nodePlayers->iUserID = iUserID;
		m_pGameLogic->SetAIBaseUserInfo(nodePlayers);
		//动态长度消息中对应的值赋给AInode
		m_pGameLogic->SetAIUserGameInfo(nodePlayers, i, pMsgData, iMsgLen);
		//对虚拟AI进行信息赋值
		nodePlayers->iSocketIndex = -1;		
		m_pGameLogic->CallBackSetRobotNick(nodePlayers->szNickName, nodePlayers->iGender, nodePlayers->iUserID);
		nodePlayers->bIfAINode = true;
		if (nodePlayers->iUserID >= g_iMinControlVID && nodePlayers->iUserID <= g_iMaxControlVID)
		{
			nodePlayers->bAIContrl = true;
		}
		m_pGameLogic->hashUserIDTable.Add((void *)(&(nodePlayers->iUserID)), sizeof(int), nodePlayers);	
		if (nodePlayers->bAIContrl == true)
		{
			nodePlayers->iStatus = PS_WAIT_DESK;
		}
		//将node进行一次入座
		int iFreePos = -1;
		FactoryPlayerNodeDef *nodePlayer = NULL;
		int iNowAllNum = 0;
		for (int j = 0; j < 10; j++)
		{
			if (pVTable->pNodePlayers[j] != NULL)
			{
				iNowAllNum++;
			}
			else if (iFreePos == -1)
			{
				iFreePos = j;
			}
		}
		pVTable->iUserID[iFreePos] = nodePlayers->iUserID;
		pVTable->pNodePlayers[iFreePos] = nodePlayers;
		pVTable->pNodePlayers[iFreePos]->iVTableID = pVTable->iVTableID;
		pVTable->bNextReadyOK[iFreePos] = true;
		pVTable->pNodePlayers[iFreePos]->bAIContrl = true;
		pVTable->pNodePlayers[iFreePos]->iStatus = PS_WAIT_DESK;
	}
	//至此同一批次所有的AI入座成功，新增一个字段用于倒计时分批次发送入组AI的信息
	pVTable->iCTLSendCTLAIInfoTime = rand() % 3 + 1;//1-3s显示一批AI
}


void NewAssignTableLogic::SendVirtualTableInfo(VirtualTableDef* pVirtualTable,int iPointedUser)
{	
	FactoryTableItemDef stTempTable;
	memset(&stTempTable, 0, sizeof(FactoryTableItemDef));	
	for (int i = 0; i < 10; i++)
	{
		if (pVirtualTable->iUserID[i] > 0)
		{
			if (pVirtualTable->pNodePlayers[i] != NULL)
			{
				stTempTable.pFactoryPlayers[i] = pVirtualTable->pNodePlayers[i];
				int iOldNumExtra = stTempTable.pFactoryPlayers[i]->cTableNumExtra;
				stTempTable.pFactoryPlayers[i]->cTableNumExtra = i;
				//因中心服务器有重复发ai问题，导致已经被分配进桌的ai被重复分配，这里重设位置会有问题，先重置回去
				if (stTempTable.pFactoryPlayers[i]->iStatus <= PS_WAIT_DESK)
				{
					stTempTable.pFactoryPlayers[i]->cTableNumExtra = i;	
				}				
			}
			else if (pVirtualTable->iUserID[i] >= g_iMinVirtualID && pVirtualTable->iUserID[i] <= g_iMaxVirtualID)
			{
				//通过redis获取虚拟玩家的局数等信息，获取到后再次发送给桌上其他玩家
				SendGetVirtualAIInfoReq(pVirtualTable->iUserID[i], pVirtualTable->iVTableID);
			}
		}
	}
	_log(_DEBUG, "NewATable", "SendVirtualTableInfo:VT[%d],user[%d:%d,%d:%d,%d:%d,%d:%d],ptUser[%d] ,iMinPlayerNum[%d]", pVirtualTable->iVTableID, pVirtualTable->iUserID[0], pVirtualTable->pNodePlayers[0]!=NULL? pVirtualTable->pNodePlayers[0]->iUserID:0
	, pVirtualTable->iUserID[1], pVirtualTable->pNodePlayers[1] != NULL ? pVirtualTable->pNodePlayers[1]->iUserID : 0, pVirtualTable->iUserID[2], pVirtualTable->pNodePlayers[2] != NULL ? pVirtualTable->pNodePlayers[2]->iUserID : 0,
		pVirtualTable->iUserID[3],pVirtualTable->pNodePlayers[3] != NULL ? pVirtualTable->pNodePlayers[3]->iUserID : 0, iPointedUser, pVirtualTable->iMinPlayerNum);
	for (int i = 0; i < 10; i++)
	{
		if (pVirtualTable->iUserID[i] > g_iMaxVirtualID && pVirtualTable->pNodePlayers[i] != NULL)
		{
			if (iPointedUser == 0 || pVirtualTable->pNodePlayers[i]->iUserID == iPointedUser)
			{
				m_pGameLogic->GetTablePlayerInfo(&stTempTable, pVirtualTable->pNodePlayers[i]);
				m_pGameLogic->CLSendPlayerInfoNotice(pVirtualTable->pNodePlayers[i]->iSocketIndex, pVirtualTable->iMinPlayerNum);
				
				if (iPointedUser > 0)
				{
					break;
				}
			}			
		}
	}
}

void NewAssignTableLogic::SendVirtualTableInfoForCTL(VirtualTableDef * pVirtualTable, int SendNum)
{
	FactoryTableItemDef stTempTable;
	memset(&stTempTable, 0, sizeof(FactoryTableItemDef));
	for (int i = 0; i < SendNum; i++)
	{
		if (pVirtualTable->iUserID[i] > 0)
		{
			if (pVirtualTable->pNodePlayers[i] != NULL)
			{
				stTempTable.pFactoryPlayers[i] = pVirtualTable->pNodePlayers[i];
				int iOldNumExtra = stTempTable.pFactoryPlayers[i]->cTableNumExtra;
				stTempTable.pFactoryPlayers[i]->cTableNumExtra = i;
				//因中心服务器有重复发ai问题，导致已经被分配进桌的ai被重复分配，这里重设位置会有问题，先重置回去
				if (stTempTable.pFactoryPlayers[i]->iStatus > PS_WAIT_DESK)
				{
					//stTempTable.pFactoryPlayers[i]->cTableNumExtra = iOldNumExtra;
				}
			}
			else if (pVirtualTable->iUserID[i] >= g_iMinVirtualID && pVirtualTable->iUserID[i] <= g_iMaxVirtualID)
			{
				//通过redis获取虚拟玩家的局数等信息，获取到后再次发送给桌上其他玩家
				SendGetVirtualAIInfoReq(pVirtualTable->iUserID[i], pVirtualTable->iVTableID);
			}
		}
	}
	_log(_DEBUG, "NewATable", "SendVirtualTableInfoForCTL:VT[%d],user[%d:%d,%d:%d,%d:%d,%d:%d] ,iMinPlayerNum[%d]", pVirtualTable->iVTableID, pVirtualTable->iUserID[0], pVirtualTable->pNodePlayers[0] != NULL ? pVirtualTable->pNodePlayers[0]->iUserID : 0
		, pVirtualTable->iUserID[1], pVirtualTable->pNodePlayers[1] != NULL ? pVirtualTable->pNodePlayers[1]->iUserID : 0, pVirtualTable->iUserID[2], pVirtualTable->pNodePlayers[2] != NULL ? pVirtualTable->pNodePlayers[2]->iUserID : 0,
		pVirtualTable->iUserID[3], pVirtualTable->pNodePlayers[3] != NULL ? pVirtualTable->pNodePlayers[3]->iUserID : 0, pVirtualTable->iMinPlayerNum);
	for (int i = 0; i < 10; i++)
	{
		if (pVirtualTable->iUserID[i] > g_iMaxVirtualID && pVirtualTable->pNodePlayers[i] != NULL)
		{
			m_pGameLogic->GetTablePlayerInfo(&stTempTable, pVirtualTable->pNodePlayers[i]);
			m_pGameLogic->CLSendPlayerInfoNotice(pVirtualTable->pNodePlayers[i]->iSocketIndex, pVirtualTable->iMinPlayerNum);
		}
	}
}

void NewAssignTableLogic::ResetTableNumExtra(OneTableUsersDef *pOneTableInfo, int iNextVTableID, int iCurNum)
{
	map<int, int> mapTemp;
	int iVTableID[10];
	int iVTableNumExtra[10];
	for (int j = 0; j < 10; j++)
	{
		iVTableID[j] = 0;
		iVTableNumExtra[j] = -1;
	}
	int iMaxNumVTable = 0;
	int iJudgeMaxNum = 0;
	int iLandLord = 0;
	for (int i = 0; i < 10; i++)
	{
		if (pOneTableInfo->iUserID[i] > 0)
		{
			GetUserVTableInfo(pOneTableInfo->iUserID[i], iVTableID[i], iVTableNumExtra[i]);
			if (iVTableID[i] > 0)
			{
				map<int, int>::iterator iterTemp = mapTemp.find(iVTableID[i]);
				if (iterTemp == mapTemp.end())
				{
					mapTemp[iVTableID[i]] = 1;
					if (iJudgeMaxNum == 0)
					{
						iMaxNumVTable = iVTableID[i];
						iJudgeMaxNum = 1;
					}
				}
				else
				{
					iterTemp->second++;
					if (iterTemp->second > iJudgeMaxNum)
					{
						iMaxNumVTable = iVTableID[i];
						iJudgeMaxNum = iterTemp->second;
					}
				}
			}
			if (iLandLord == 0)
			{
				iLandLord = pOneTableInfo->iUserID[i];
			}
		}
	}
	if (mapTemp.empty())
	{
		return;
	}

	if (iJudgeMaxNum == 1)//没有2个或以上的人在同一个虚拟桌，无需考虑原来位置，直接都离开原来虚拟桌即可,且当前等待开局的玩家无需考虑位置
	{
		for (int i = 0; i < 10; i++)
		{
			if (pOneTableInfo->iUserID[i] > 0 && iVTableID[i] > 0)
			{
				CheckRemoveVTableUser(iVTableID[i], pOneTableInfo->iUserID[i], true);
			}
		}
		return;
	}
	//人数最多的桌子不在开局内的玩家，要通知桌上其他人
	VirtualTable* pMaxNumTable = FindVTableInfo(iMaxNumVTable);
	if (pMaxNumTable)
	{
		for (int i = 0; i < 10; i++)
		{
			if (pMaxNumTable->pNodePlayers[i] != NULL)
			{
				bool bFind = false;
				for (int j = 0; j < 10; j++)
				{
					if (pOneTableInfo->iUserID[j] == pMaxNumTable->pNodePlayers[i]->iUserID)
					{
						bFind = true;
						break;
					}
				}
				if (!bFind)
				{
					for (int j = 0; j < 10; j++)
					{
						if (i != j && pMaxNumTable->pNodePlayers[j] != NULL)
						{
							EscapeNoticeDef msgNotice;
							memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
							msgNotice.cTableNumExtra = i;
							msgNotice.iUserID = htonl(pMaxNumTable->pNodePlayers[i]->iUserID);
							msgNotice.cType = -66;
							m_pGameLogic->CLSendSimpleNetMessage(pMaxNumTable->pNodePlayers[j]->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));

							memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
							msgNotice.cTableNumExtra = j;
							msgNotice.iUserID = htonl(pMaxNumTable->pNodePlayers[j]->iUserID);
							msgNotice.cType = -66;
							m_pGameLogic->CLSendSimpleNetMessage(pMaxNumTable->pNodePlayers[i]->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
						}					
					}
				}
			}
		}
	}
	//不在人数最多桌子的用户离开原桌子
	for (int i = 0; i < 10; i++)
	{
		if (pOneTableInfo->iUserID[i] > 0 && iVTableID[i] > 0 && iVTableID[i] != iMaxNumVTable)
		{
			CheckRemoveVTableUser(iVTableID[i],pOneTableInfo->iUserID[i], true);
		}
	}
	//按照当前剩余的这个虚拟桌调整即将开局的这些玩家的位置
	OneTableUsersDef stNewOneTable;
	memset(&stNewOneTable, 0, sizeof(OneTableUsersDef));
	for (int i = 0; i < 10; i++)
	{
		if (pOneTableInfo->iUserID[i] > 0 && iMaxNumVTable == iVTableID[i])
		{
			int iPos = iVTableNumExtra[i];
			stNewOneTable.iUserID[iPos] = pOneTableInfo->iUserID[i];
			stNewOneTable.iIfFreeRoom[iPos] = pOneTableInfo->iIfFreeRoom[i];
			stNewOneTable.bIfInServer[iPos] = pOneTableInfo->bIfInServer[i];
			_log(_DEBUG, "NTL", "ResetTableNumExtra 1 stNewOneTable.iUserID[%d] = [%d]", iPos, stNewOneTable.iUserID[iPos]);
			//找到需要的位置后，就从对应虚拟桌离开，所有用户离开后，对应的虚拟桌会被删除
			CheckRemoveVTableUser(iVTableID[i], pOneTableInfo->iUserID[i], false,false);
		}
	}
	//再将剩余的人分配到空位置
	for (int i = 0; i < 10; i++)
	{
		if (pOneTableInfo->iUserID[i] > 0 && iMaxNumVTable != iVTableID[i])
		{
			for (int j = 0; j < 10; j++)
			{
				if (stNewOneTable.iUserID[j] == 0)
				{
					stNewOneTable.iUserID[j] = pOneTableInfo->iUserID[i];
					stNewOneTable.iIfFreeRoom[j] = pOneTableInfo->iIfFreeRoom[i];
					stNewOneTable.bIfInServer[j] = pOneTableInfo->bIfInServer[i];
					_log(_DEBUG, "NTL", "ResetTableNumExtra 2 stNewOneTable.iUserID[%d] = [%d]", j, stNewOneTable.iUserID[j]);
					if (iVTableID[i] > 0)
					{
						//找到需要的位置后，就从对应虚拟桌离开，所有用户离开后，对应的虚拟桌会被删除
						CheckRemoveVTableUser(iVTableID[i], pOneTableInfo->iUserID[i], true, true);
					}
					break;
				}
			}
		}
	}
	for (int i = 0; i < 10; i++)
	{
		pOneTableInfo->iUserID[i] = stNewOneTable.iUserID[i];
		pOneTableInfo->iIfFreeRoom[i] = stNewOneTable.iIfFreeRoom[i];
		pOneTableInfo->bIfInServer[i] = stNewOneTable.bIfInServer[i];
		if (pOneTableInfo->iUserID[i] > 0)
		{
			_log(_DEBUG, "NTL", "ResetTableNumExtra pOneTableInfo->iUserID[%d] = [%d],room[%d],inS[%d]", i, pOneTableInfo->iUserID[i], pOneTableInfo->iIfFreeRoom[i], pOneTableInfo->bIfInServer[i]);
		}
	}
}

VirtualTableDef* NewAssignTableLogic::FindVTableInfo(int iVTableID)
{
	map<int, VirtualTableDef*>::iterator iter = m_mapAllVTables.find(iVTableID);
	if(iter != m_mapAllVTables.end())
	{
		return iter->second;
	}
	return NULL;
}

VirtualTableDef* NewAssignTableLogic::FindAIControlVTableInfo(int iUserID, int &iVCIndex)
{
	VirtualTable* pVTable = NULL;
	for (int i = 0; i < m_vcAIControlVTables.size(); i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (m_vcAIControlVTables[i]->iUserID[j] == iUserID)
			{
				pVTable = m_vcAIControlVTables[i];
				iVCIndex = i;
				break;
			}
		}
		if (pVTable != NULL)
		{
			break;
		}
	}
	return pVTable;
}

void NewAssignTableLogic::GetUserVTableInfo(int iUserID, int &iVTableID, int &iNumExtra)
{
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (nodePlayers != NULL && nodePlayers->iVTableID > 0)
	{
		VirtualTableDef* pVTable = FindVTableInfo(nodePlayers->iVTableID);
		if (pVTable != NULL)
		{
			for (int i = 0; i < 10; i++)
			{
				if (pVTable->iUserID[i] == iUserID)
				{
					iVTableID = pVTable->iVTableID;
					iNumExtra = i;
					return;
				}
			}
		}
	}
}

VirtualTableDef* NewAssignTableLogic::GetFreeVTable(int iVTableID)
{
	_log(_DEBUG, "NTL", "GetFreeVTable iVTableID[%d]", iVTableID);
	VirtualTableDef* pVTable = NULL;
	if (m_vcFreeVTables.empty())
	{
		pVTable = new VirtualTableDef();
	}
	else
	{
		pVTable = m_vcFreeVTables[0];
		m_vcFreeVTables.erase(m_vcFreeVTables.begin());
	}
	memset(pVTable, 0, sizeof(VirtualTableDef));
	pVTable->iVTableID = iVTableID;
	return pVTable;
}

void NewAssignTableLogic::AddToFreeVTables(VirtualTableDef* pVTable)
{
	_log(_DEBUG, "NTL", "AddToFreeVTables iVTableID[%d]", pVTable->iVTableID);
	pVTable->iVTableID = -pVTable->iVTableID;
	m_vcFreeVTables.push_back(pVTable);
}

void NewAssignTableLogic::CheckRemoveVTableUser(int iVTableID, int iUserID, bool bNoticeOthers, bool bNoticeSelf, bool bEraseVTable, bool bClearVTable)
{	
	map<int, VirtualTableDef*>::iterator iter = m_mapAllVTables.find(iVTableID);
	if (iter == m_mapAllVTables.end())
	{
		return;
	}
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	VirtualTableDef* pVTable = iter->second;
	int iLeftNum = 0;
	int iLeftAINum = 0;
	int iUserNumExtra = -1;
	int iNextUserPos = -1;
	for (int i = 0; i < 10; i++)
	{
		if (pVTable->iUserID[i] == iUserID)
		{
			iUserNumExtra = i;
			pVTable->iUserID[i] = 0;
			pVTable->bNextReadyOK[i] = false;
			pVTable->pNodePlayers[i] = NULL;
			if (nodePlayers != NULL)
			{
				nodePlayers->iVTableID = 0;			
			}		
		}
		else if (pVTable->iUserID[i] > 0)
		{
			if (CheckUserIfVAI(pVTable->iUserID[i]))
			{
				iLeftAINum++;
			}
			else if(iNextUserPos == -1)
			{
				iNextUserPos = i;
			}
			iLeftNum++;
		}
	}

	bool bNeedClearVTable = false;
	if (iLeftNum == 0 || iLeftNum == iLeftAINum)
	{
		bNeedClearVTable = true;
	}
	_log(_DEBUG, "NTL", "CheckRemoveVTableUser iUserID[%d], iLeftNum[%d],leftAI[%d], iUserNumExtra[%d],bClearVT[%d],iNextUserPos[%d]", iUserID, iLeftNum, iLeftAINum, iUserNumExtra, bNeedClearVTable, iNextUserPos);
	if (bNeedClearVTable)
	{
		if (bClearVTable)
		{
			CallBackFreeVTableID(abs(pVTable->iVTableID));
			AddToFreeVTables(pVTable);
		}		
		if (bEraseVTable)
		{
			m_mapAllVTables.erase(iter);
		}	
		if (CheckUserIfVAI(iUserID) == false && bNoticeSelf)
		{		
			if (nodePlayers != NULL && iUserNumExtra >= 0)
			{
				EscapeNoticeDef msgNotice;
				memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
				msgNotice.cTableNumExtra = iUserNumExtra;
				msgNotice.iUserID = htonl(iUserID);
				msgNotice.cType = -66;
				m_pGameLogic->CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
			}
		}		
	}
	else if (bNoticeOthers && iUserNumExtra != -1)
	{
		EscapeNoticeDef msgNotice;
		memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
		msgNotice.cTableNumExtra = iUserNumExtra;
		msgNotice.iUserID = htonl(iUserID);
		for (int i = 0; i < 10; i++)
		{
			if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i] != NULL && !pVTable->pNodePlayers[i]->bIfAINode)
			{
				m_pGameLogic->CLSendSimpleNetMessage(pVTable->pNodePlayers[i]->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
			}
		}
		if (CheckUserIfVAI(iUserID) == false && nodePlayers != NULL)
		{
			msgNotice.cType = -66;
			m_pGameLogic->CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
		}			
	}
}

bool NewAssignTableLogic::CheckUserIfVAI(int iUserID)
{
	if (iUserID >= g_iMinVirtualID && iUserID <= g_iMaxVirtualID)
	{
		return true;
	}
	return false;
}

void NewAssignTableLogic::SendGetVirtualAIInfoReq(int iVirtualID, int iVTableID,int iNeedNum)
{
	RdGetVirtualAIInfoReqDef msgReq;
	memset(&msgReq, 0, sizeof(RdGetVirtualAIInfoReqDef));
	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq.msgHeadInfo.iMsgType = RD_GET_VIRTUAL_AI_INFO_MSG;
	msgReq.iVirtualID = iVirtualID;
	msgReq.iVTableID = iVTableID;
	msgReq.iNeedNum = iNeedNum;
	msgReq.iGameID = m_pGameLogic->m_pServerBaseInfo->iGameID;
	msgReq.iServerID = m_pGameLogic->m_pServerBaseInfo->iServerID;
	if (m_pGameLogic->m_pQueToCenterServer && iNeedNum > 0)
	{
		m_pGameLogic->m_pQueToCenterServer->EnQueue(&msgReq, sizeof(RdGetVirtualAIInfoReqDef));
	}
	else if (m_pGameLogic->m_pQueToRedis && iNeedNum == 0)
	{
		m_pGameLogic->m_pQueToRedis->EnQueue(&msgReq, sizeof(RdGetVirtualAIInfoReqDef));
	}
	_log(_DEBUG, "NTL", "SendGetVirtualAIInfoReq iVirtualID[%d],iVTableID[%d],iNeedNum[%d]", iVirtualID, iVTableID, iNeedNum);
}



void NewAssignTableLogic::CallBackFreeVTableID(int iVTableID)
{
	_log(_DEBUG, "NTL", "CallBackFreeNextVTableID iVTableID[%d]", iVTableID);
	NewCenterFreeVTableMsgDef msgReq;
	memset(&msgReq,0,sizeof(NewCenterFreeVTableMsgDef));
	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq.msgHeadInfo.iMsgType = NEW_CENTER_FREE_VTABLE_MSG;
	msgReq.iVTableID = iVTableID;
	if (m_pGameLogic->m_pQueToCenterServer)
	{
		m_pGameLogic->m_pQueToCenterServer->EnQueue(&msgReq, sizeof(NewCenterFreeVTableMsgDef));
	}
}

void NewAssignTableLogic::CallBackDissolveTableToVTable(FactoryTableItemDef *pTableItem)
{
	if (pTableItem == NULL)
	{
		return;
	}	
	if (pTableItem->iNextVTableID == g_iControlRoomID)//上局是控制局，继续判断下本局是否控制
	{
		FactoryPlayerNodeDef *pRealPlayer = NULL;	
		for (int i = 0; i < pTableItem->iSeatNum; i++)
		{
			if (pTableItem->pFactoryPlayers[i] != NULL && !pTableItem->pFactoryPlayers[i]->bIfAINode)
			{
				pRealPlayer = pTableItem->pFactoryPlayers[i];
				break;
			}
		}
		bool bNeedAIControl = false;
		if (pRealPlayer != NULL)
		{
			bNeedAIControl = m_pGameLogic->CheckIfPlayWithControlAI(pRealPlayer);
		}
		if (bNeedAIControl)//继续控制，直接在原桌上继续
		{
			for (int i = 0; i < pTableItem->iSeatNum; i++)
			{
				if (pTableItem->pFactoryPlayers[i] != NULL)
				{
					pTableItem->pFactoryPlayers[i]->iStatus = PS_WAIT_READY;//等待开始状态
					pTableItem->bIfReady[i] = false;
				}
			}
			//ai玩家自动准备
			for (int i = 0; i < pTableItem->iSeatNum; i++)
			{
				if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->bIfAINode)
				{
					m_pGameLogic->HandleReadyReq(pTableItem->pFactoryPlayers[i]->iUserID, NULL);
				}
			}
		}
		else
		{
			//离开桌子，等待继续参与配桌
			for (int i = 0; i < pTableItem->iSeatNum; i++)
			{
				if (pTableItem->pFactoryPlayers[i] != NULL)
				{
					pTableItem->pFactoryPlayers[i]->cTableNum = 0;
					pTableItem->pFactoryPlayers[i]->cTableNumExtra = -1;
					pTableItem->pFactoryPlayers[i]->bAIContrl = false;
					int iRoomIndex = pTableItem->pFactoryPlayers[i]->cRoomNum - 1;
					if (pTableItem->pFactoryPlayers[i]->iUserID >= g_iMinControlVID && pTableItem->pFactoryPlayers[i]->iUserID <= g_iMaxControlVID)
					{
						m_pGameLogic->ReleaseControlAINode(pTableItem->pFactoryPlayers[i]);
					}
					m_pGameLogic->RemoveTablePlayer(pTableItem, i);
				}
			}
			pTableItem->iNextVTableID = 0;
		}
		return;	
	}
	//新增控制逻辑，如果本局是正常对局，判断下局桌上玩家是否需要进入控制对局
	vector<int>vcNeedControl;
	for (int i = 0; i < pTableItem->iSeatNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i] && !pTableItem->pFactoryPlayers[i]->bIfAINode)
		{
			bool bNeedAIControl = m_pGameLogic->CheckIfPlayWithControlAI(pTableItem->pFactoryPlayers[i]);
			if (bNeedAIControl)
			{
				vcNeedControl.push_back(pTableItem->pFactoryPlayers[i]->iUserID);
				//将该玩家从原桌上离开		
				int iRoomIndex = pTableItem->pFactoryPlayers[i]->cRoomNum - 1;
				pTableItem->pFactoryPlayers[i]->cTableNum = 0;
				pTableItem->pFactoryPlayers[i]->cTableNumExtra = -1;
				m_pGameLogic->RemoveTablePlayer(pTableItem, i);
			}
		}
	}
	//vcNeedControl内的玩家从原桌上离开，并发送EscapeNotice给其余玩家+自己
	for (int i = 0; i < vcNeedControl.size(); i++)
	{
		EscapeNoticeDef msgNotice;
		memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
		msgNotice.iUserID = htonl(vcNeedControl[i]);//for see
		msgNotice.cType = 0;
		for (int j = 0; j < pTableItem->iSeatNum; j++)
		{
			if (pTableItem->pFactoryPlayers[j])
			{
				m_pGameLogic->CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[j]->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
			}
		}
		//通知中心服务器自己离开了
		CallBackUserLeave(vcNeedControl[i], 0);
		//再给自己补发一次，自己收到这个消息会重发一次入座请求在那里会进入到控制
		//FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(m_pGameLogic->hashUserIDTable.Find((void *)&vcNeedControl[i], sizeof(int)));
		//m_pGameLogic->CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
	}

	VirtualTableDef* pVTable = GetFreeVTable(pTableItem->iNextVTableID);
	pVTable->iMinPlayerNum = pTableItem->iSeatNum;
	int iPlayNum = 0;
	FactoryPlayerNodeDef *pTablePlayer = NULL;
	for (int i = 0; i < pTableItem->iSeatNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != NULL)
		{
			if (pTablePlayer == NULL)
			{
				pTablePlayer = pTableItem->pFactoryPlayers[i];
			}
			pTableItem->pFactoryPlayers[i]->iVTableID = pTableItem->iNextVTableID;			
			iPlayNum++;
			pVTable->iUserID[i] = pTableItem->pFactoryPlayers[i]->iUserID;//保证之前的位置不变
			pVTable->pNodePlayers[i] = pTableItem->pFactoryPlayers[i];
			pVTable->bNextReadyOK[i] = false;
			pVTable->pNodePlayers[i]->iStatus = PS_WAIT_READY;
			//从桌上移走			
			int iRoomIndex = pTableItem->pFactoryPlayers[i]->cRoomNum - 1;
			pTableItem->pFactoryPlayers[i]->cTableNum = 0;
			pTableItem->pFactoryPlayers[i]->cTableNumExtra = -1;
			m_pGameLogic->RemoveTablePlayer(pTableItem, i);			
		}
	}
	m_pGameLogic->ResetTableState(pTableItem);
	if (iPlayNum == 0)//桌上已经没有人了（如1个玩家和ai玩，玩家先结束退出后，剩余ai正常打完）
	{
		CallBackFreeVTableID(pTableItem->iNextVTableID);
		return;
	}
	m_mapAllVTables[pVTable->iVTableID] = pVTable;
	_log(_DEBUG, "NTL", "CallBackDissolveTableToVTable iVTableID[%d:%d]:[%d:%d,%d:%d,%d:%d,%d:%d],iPlayNum[%d],all[%d]", pTableItem->iNextVTableID, pVTable->iVTableID, pVTable->iUserID[0], pVTable->bNextReadyOK[0],
		pVTable->iUserID[1], pVTable->bNextReadyOK[1], pVTable->iUserID[2], pVTable->bNextReadyOK[2], pVTable->iUserID[3], pVTable->bNextReadyOK[3], iPlayNum, m_mapAllVTables.size());
}


void NewAssignTableLogic::CallBackNextVTableReady(int iVTableID, FactoryPlayerNodeDef *nodePlayers)
{
	VirtualTableDef* pVTableInfo = NULL;
	if (iVTableID == g_iControlRoomID)
	{
		int iVCIndex = -1;
		pVTableInfo = FindAIControlVTableInfo(nodePlayers->iUserID, iVCIndex);
	}
	else
	{
		pVTableInfo = FindVTableInfo(iVTableID);
	}
	if (pVTableInfo == NULL)
	{
		if (nodePlayers != NULL)
		{
			CallBackUserLeave(-nodePlayers->iUserID, 0);//先通知中心服务器可配桌状态
			CallBackUserNeedAssignTable(nodePlayers);
			_log(_DEBUG, "NTL", "CallBackCheckContinueNextVTable not find VT[%d],user[%d]", iVTableID, nodePlayers->iUserID);
		}
		_log(_DEBUG, "NTL", "CallBackNextVTableReady user[%d],vt[%d] not find", nodePlayers->iUserID,iVTableID);
		return;
	}
	bool bFind = false;
	for (int i = 0; i < 10; i++)
	{
		if (nodePlayers != NULL && pVTableInfo->iUserID[i] == nodePlayers->iUserID)
		{
			//_log(_DEBUG, "NTL", "CallBackNextVTableReady user[%d],vt[%d:%d]", nodePlayers->iUserID, iVTableID, pVTableInfo->iVTableID);
			pVTableInfo->bNextReadyOK[i] = true;
			//广播该玩家已准备消息
			ReadyNoticeDef msgNotice;
			memset((char*)&msgNotice, 0, sizeof(ReadyNoticeDef));
			msgNotice.cTableNumExtra = i;
			for (int j = 0; j < 10; j++)
			{
				if (pVTableInfo->pNodePlayers[j])
				{
					m_pGameLogic->CLSendSimpleNetMessage(pVTableInfo->pNodePlayers[j]->iSocketIndex, &msgNotice, READY_NOTICE_MSG, sizeof(ReadyNoticeDef));
				}
			}

			bFind = true;
			break;
		}
	}
	if (!bFind)
	{
		_log(_DEBUG, "NTL", "CallBackNextVTableReady user[%d] not find,vt[%d:%d]", nodePlayers->iUserID, iVTableID, pVTableInfo->iVTableID);
	}
	CheckVTableReadySatus(pVTableInfo,false);
}



bool NewAssignTableLogic::CheckVTableReadySatus(VirtualTableDef* pVTable, bool bTimeOut)
{
	_log(_DEBUG, "NTL", "CheckVTableReadySatus,bTimeOut[%d],VT[%d],[%d:%d,%d:%d,%d:%d,%d:%d]", bTimeOut, pVTable->iVTableID, pVTable->iUserID[0], pVTable->bNextReadyOK[0]
		, pVTable->iUserID[1], pVTable->bNextReadyOK[1], pVTable->iUserID[2], pVTable->bNextReadyOK[2], pVTable->iUserID[3], pVTable->bNextReadyOK[3]);
	//未准备好的都直接踢出服务器，已经准备好的可以继续同一桌继续配桌
	int iVTableID = pVTable->iVTableID;
	int iLeftNum = 0;
	int iLeftAINum = 0;
	int iHaveReadyNum = 0;
	FactoryPlayerNodeDef * pLeftPlayer = NULL;
	int iLeftPos = 0;

	for (int i = 0; i < 10; i++)
	{
		if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i] != NULL)
		{
			FactoryPlayerNodeDef * pTablePlayer = pVTable->pNodePlayers[i];
			//非好友房掉线或好友房掉线超时还未回来的，可以直接踢掉
			bool bNeedRemove = false;
			bool bNeedDisConnect = false;
			
			if (bTimeOut && (pTablePlayer->bIfWaitLoginTime == true || pTablePlayer->cDisconnectType == 1))
			{
				bNeedRemove = true;
				bNeedDisConnect = true;
			}
			else if (bTimeOut && !pVTable->bNextReadyOK[i])////没有准备好的玩家需要踢出
			{
				bNeedRemove = true;
				pTablePlayer->iStatus = PS_WAIT_DESK;
				if (pTablePlayer->iSocketIndex != -1)
				{
					KickOutServerDef kickout;
					memset(&kickout, 0, sizeof(KickOutServerDef));
					kickout.cKickType = g_iReadyTmOut;
					kickout.iKickUserID = pTablePlayer->iUserID;
					kickout.cClueType = 2;//提示类型（0仅提示，1退出房间，2退出游戏返回大厅）
					m_pGameLogic->CLSendSimpleNetMessage(pTablePlayer->iSocketIndex, &kickout, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
				}
				else
				{
					bNeedDisConnect = true;
				}
			}
			
			if (bNeedRemove)
			{
				_log(_DEBUG, "NTL", "CheckVTableReadySatus,bTimeOut[%d],VT[%d],needRemove[%d],bNeedDisConnect[%d]", bTimeOut, pVTable->iVTableID, pVTable->iUserID[i], bNeedDisConnect);
				if (pVTable->iVTableID != g_iControlRoomID)
				{
					CheckRemoveVTableUser(pVTable->iVTableID, pVTable->iUserID[i], true, false, bTimeOut ? false : true);
				}				
				if (bNeedDisConnect)
				{
					pTablePlayer->iStatus = PS_WAIT_DESK;
					m_pGameLogic->OnFindDeskDisconnect(pTablePlayer->iUserID, NULL);
				}
			}
			else
			{
				iLeftNum++;
				if (pTablePlayer->bIfAINode)
				{
					iLeftAINum++;
				}
				if (pLeftPlayer == NULL && !pTablePlayer->bIfAINode)
				{
					pLeftPlayer = pTablePlayer;
					iLeftPos = i;
				}
				if (pVTable->bNextReadyOK[i] || (pVTable->pNodePlayers[i] != NULL && pVTable->pNodePlayers[i]->iStatus == PS_WAIT_DESK))
				{
					iHaveReadyNum++;
				}
			}
			if (pVTable->iVTableID != g_iControlRoomID)
			{
				pVTable = FindVTableInfo(iVTableID);//防止在中间判断中移除玩家的时候桌子因没人也被释放了
				if (pVTable == NULL)
				{
					break;
				}
			}			
		}
	}
	_log(_DEBUG, "NTL", "CheckVTableReadySatus,bTimeOut[%d],VT[%d,%d],iLeft[%d],ready[%d],min[%d],pLeftPlayer[%d]", bTimeOut, iVTableID, pVTable!=NULL?pVTable->iVTableID:-1, iLeftNum, iHaveReadyNum, 
		pVTable != NULL ? pVTable->iMinPlayerNum : -1, pLeftPlayer != NULL ? pLeftPlayer->iUserID : 0);
	if (pVTable == NULL || (pVTable->iVTableID < 0 && pVTable->iVTableID != g_iControlRoomID))//桌上已经没有人了
	{
		return false;
	}
	if (iVTableID != g_iControlRoomID && iLeftNum == iHaveReadyNum && (iHaveReadyNum <= 1 || iLeftNum == iLeftAINum+1) && pLeftPlayer != NULL)//普通房没有人或仅剩一个已经准备好的人，可以直接重新配桌了
	{
		FactoryPlayerNodeDef *pAllNode[10] = { NULL };
		for (int i = 0; i < 10; i++)
		{
			if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i] != NULL)
			{
				pAllNode[i] = pVTable->pNodePlayers[i];
			}
		}
		bool bReadyOK = pLeftPlayer->iStatus == PS_WAIT_DESK ? true : false;
		if (!bReadyOK && pLeftPlayer->iStatus == PS_WAIT_READY && pVTable->bNextReadyOK[iLeftAINum])
		{
			bReadyOK = true;
		}
		//CallBackFreeVTableID(iVTableID);//回收下局的桌号,这里不需要，后面移掉用户的时候会回收
		CheckRemoveVTableUser(iVTableID, pLeftPlayer->iUserID, false, false, bTimeOut ? false : true);		
		bool bWithControlAI = m_pGameLogic->CheckIfPlayWithControlAI(pLeftPlayer);
		if (bWithControlAI)
		{
			if (pLeftPlayer->iVTableID != 0)  //玩家已经有虚拟桌号，证明中心已经有该玩家混服配桌节点，需要删掉，在这里开控制局
			{
				CallBackUserLeave(pLeftPlayer->iUserID, pLeftPlayer->iVTableID);//明确玩家退出服务器时才传groupid&iVTableID，从组队/虚拟桌信息中去掉
			}
			//这里有点问题，待优化，如a+b+虚拟ai开了4人局，a离开，b这里判断需要进入控制了，会继续用这个虚拟ai，但中心服务器在释放桌子时会释放这个虚拟ai，可能会这桌还未解散，虚拟ai又被使用了
			CallBackPlayWithContolAI(pLeftPlayer, bReadyOK, pAllNode);
		}
		else
		{
			pLeftPlayer->iStatus = PS_WAIT_DESK;
			//通知客户端玩家离开该桌
			EscapeNoticeDef msgNotice;
			memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
			msgNotice.cTableNumExtra = iLeftPos;
			msgNotice.iUserID = htonl(pLeftPlayer->iUserID);
			msgNotice.cType = -66;
			m_pGameLogic->CLSendSimpleNetMessage(pLeftPlayer->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
			//请求中心服务器可以开始配桌
			CallBackUserLeave(-pLeftPlayer->iUserID, 0);//需要先通知中心服务器重置可配桌状态
			CallBackUserNeedAssignTable(pLeftPlayer);
		}		
		return false;
	}
		
	if (iHaveReadyNum < iLeftNum)//剩下的人都还没有准备好
	{
		return false;
	}	
	if (iHaveReadyNum == pVTable->iMinPlayerNum)//都准备好了，可以开局
	{
		int iTableNum = m_pGameLogic->GetValidTableNum();
		_log(_DEBUG, "NewATable", "CheckVTableReadySatus:start VT[%d],user[%d,%d,%d,%d],iTableNum[%d]",iVTableID, pVTable->iUserID[0], pVTable->iUserID[1], pVTable->iUserID[2], pVTable->iUserID[3], iTableNum);
		if (iTableNum <= 0)//没找到可用的桌子，提示客户端吧
		{
			for (int i = 0; i < 10; i++)
			{
				if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i])
				{
					KickOutServerDef kickout;
					memset(&kickout, 0, sizeof(KickOutServerDef));
					kickout.cKickType = g_iRoomNoTable;
					kickout.iKickUserID = pVTable->iUserID[i];
					kickout.cClueType = 2;//提示类型（0仅提示，1退出房间，2退出游戏返回大厅）
					m_pGameLogic->CLSendSimpleNetMessage(pVTable->pNodePlayers[i]->iSocketIndex, &kickout, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
				}
			}
			_log(_ERROR, "NewATable", "CheckVTableReadySatus user[%d,%d,%d,%d,%d] not find table!!!", pVTable->iUserID[0], pVTable->iUserID[1], pVTable->iUserID[2], pVTable->iUserID[3], pVTable->iUserID[4]);
			return false;
		}
		FactoryTableItemDef* pTableItem = m_pGameLogic->GetTableItem(0, iTableNum - 1);
		if (pTableItem != NULL)
		{
			pTableItem->iNextVTableID = iVTableID;
		}		
		int iMinPlayerNum = pVTable->iMinPlayerNum;
		/*_log(_DEBUG, "NTL", "CheckVTableReadySatus fdRoom[%d],user[%d:%d,%d:%d,%d:%d,%d:%d]", pVTable->iVTableID,
			pVTable->iUserID[0], pVTable->pNodePlayers[0] != NULL ? pVTable->pNodePlayers[0]->iVTableID : -1,
			pVTable->iUserID[1], pVTable->pNodePlayers[1] != NULL ? pVTable->pNodePlayers[1]->iVTableID : -1,
			pVTable->iUserID[2], pVTable->pNodePlayers[2] != NULL ? pVTable->pNodePlayers[2]->iVTableID : -1,
			pVTable->iUserID[3], pVTable->pNodePlayers[3] != NULL ? pVTable->pNodePlayers[3]->iVTableID : -1);*/
		bool bSitErr = false;
		for (int i = 0; i < 10; i++)
		{
			if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i])
			{
				FactoryPlayerNodeDef * pTempNode = pVTable->pNodePlayers[i];
				if (iVTableID != g_iControlRoomID)
				{
					CheckRemoveVTableUser(iVTableID, pVTable->iUserID[i], false, false, bTimeOut ? false : true,false);				
				}
				pTempNode->iStatus = PS_WAIT_DESK;//这里状态要改下，不然入座状态判断有问题
				int iRoomIndex = pTempNode->cRoomNum - 1;
				int iFreeRoom = m_pGameLogic->m_pServerBaseInfo->stRoom[iRoomIndex].iRoomType;
				bool bSitOK = m_pGameLogic->CallBackUsersSit(pTempNode->iUserID, iFreeRoom, iTableNum, i, iMinPlayerNum);
				if (!bSitErr && !bSitOK)
				{
					bSitErr = true;
				}
			}
		}
		_log(_DEBUG, "NTL", "CheckVTableReadySatus fdRoom[%d],user[%d:%d,%d:%d,%d:%d,%d:%d],bSitErr[%d]", pVTable->iVTableID,
			pVTable->iUserID[0], pVTable->pNodePlayers[0] != NULL ? pVTable->pNodePlayers[0]->iVTableID : -1,
			pVTable->iUserID[1], pVTable->pNodePlayers[1] != NULL ? pVTable->pNodePlayers[1]->iVTableID : -1,
			pVTable->iUserID[2], pVTable->pNodePlayers[2] != NULL ? pVTable->pNodePlayers[2]->iVTableID : -1,
			pVTable->iUserID[3], pVTable->pNodePlayers[3] != NULL ? pVTable->pNodePlayers[3]->iVTableID : -1,
			bSitErr);
		if (!bSitErr)
		{
			return true;
		}		
	}
	else//普通房所有还剩余在该桌的玩家可以一起重新配桌了
	{
		for (int i = 0; i < 10; i++)
		{
			if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i] != NULL)
			{
				CallBackUserLeave(-pVTable->iUserID[i], 0);//先通知中心服务器重置状态
				//pVTable->pNodePlayers[i]->iVTableID = -iVTableID;
				//CallBackUserNeedAssignTable(pVTable->pNodePlayers[i]);//在中心服务器进入指定虚拟桌号，保证已经准备好的这批人可以在一张虚拟桌上
				//pVTable->pNodePlayers[i]->iStatus = PS_WAIT_DESK;
			}
		}
		//再通知中心服务器玩家入座
		for (int i = 0; i < 10; i++)
		{
			if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i] != NULL)
			{
				pVTable->pNodePlayers[i]->iVTableID = -iVTableID;
				CallBackUserNeedAssignTable(pVTable->pNodePlayers[i]);//在中心服务器进入指定虚拟桌号，保证已经准备好的这批人可以在一张虚拟桌上
				pVTable->pNodePlayers[i]->iStatus = PS_WAIT_DESK;
			}
		}
		pVTable->iCDSitTime = m_iWaitTableResSec;
		CallBackUserNeedAssignTable(pLeftPlayer);//可以开始发送入座请求了
		pVTable->iIfSendAssignReq = 1;
	}	
	return false;
}

void NewAssignTableLogic::CallBackTableNeedAssignTable(VirtualTableDef* pVTable, FactoryPlayerNodeDef * pLeftPlayer)
{
	for (int i = 0; i < 10; i++)
	{
		if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i] != NULL)
		{
			CallBackUserLeave(-pVTable->iUserID[i], 0);//通知中心服务器重置状态
			pVTable->pNodePlayers[i]->iVTableID = -pVTable->iVTableID;
			CallBackUserNeedAssignTable(pVTable->pNodePlayers[i]);//在中心服务器进  入指定虚拟桌号，保证已经准备好的这批人可以在一张虚拟桌上
			pVTable->pNodePlayers[i]->iStatus = PS_WAIT_DESK;
		}
	}
	pVTable->iCDSitTime = m_iWaitTableResSec;
	CallBackUserNeedAssignTable(pLeftPlayer);//可以开始发送入座请求了
}


VirtualTableDef* NewAssignTableLogic::CallBackPlayWithContolAI(FactoryPlayerNodeDef *nodePlayers, bool bReadyOK, FactoryPlayerNodeDef *pAllNode[10])
{
	if (nodePlayers == NULL)
	{
		return NULL;
	}	
	int iVCIndex = -1;
	VirtualTableDef* pVTable = FindAIControlVTableInfo(nodePlayers->iUserID, iVCIndex);
	if (pVTable != NULL)
	{
		_log(_DEBUG, "NewATable", "CallBackPlayWithContolAI:user[%d] has controlVTable", nodePlayers->iUserID);
		return pVTable;
	}
	pVTable = GetFreeVTable(g_iControlRoomID);
	_log(_DEBUG, "NewATable", "CallBackPlayWithContolAI:user[%d] pAllNode[%d,%d,%d,%d],ready[%d]", nodePlayers->iUserID, pAllNode[0]!=NULL? pAllNode[0]->iUserID:0,
		pAllNode[1] != NULL ? pAllNode[1]->iUserID : 0, pAllNode[2] != NULL ? pAllNode[2]->iUserID : 0,  pAllNode[3] != NULL ? pAllNode[3]->iUserID : 0, bReadyOK);
	for (int i = 0; i < 10; i++)
	{
		if (pAllNode[i] != NULL)
		{
			pVTable->iUserID[i] = pAllNode[i]->iUserID;
			pVTable->pNodePlayers[i] = pAllNode[i];
			pVTable->bNextReadyOK[i] = true;
			if (pAllNode[i]->iUserID == nodePlayers->iUserID)
			{
				pVTable->bNextReadyOK[i] = bReadyOK;
				if (bReadyOK)
				{
					pVTable->pNodePlayers[i]->iStatus = PS_WAIT_DESK;
				}
			}
			pVTable->pNodePlayers[i]->bAIContrl = true;
			pAllNode[i]->iVTableID = pVTable->iVTableID;
		}
	}
	pVTable->iPlayType = nodePlayers->iPlayType;
	pVTable->iMinPlayerNum = (nodePlayers->iPlayType >> 16) & 0xff;

	//pVTable->iCDSitTime = 1 + rand() % 3;//1-3s入座一个判断一次增加ai
	m_vcAIControlVTables.push_back(pVTable);
	//向中心服务器发送请求获取指定数量控制AI
	bool bISucceed = CallBackCheckAddControlAI(pVTable);
	if (!bISucceed)
	{
		//发送请求失败，当前玩家需要去中心服务器请求配桌
		CallBackUserLeave(nodePlayers->iUserID, 0);
		CallBackUserNeedAssignTable(nodePlayers);
	}
	return pVTable;
}

bool NewAssignTableLogic::CallBackCheckAddControlAI(VirtualTableDef* pVTable)
{
	int iNowAllNum = 0;
	int iFreePos = -1;
	FactoryPlayerNodeDef *nodePlayer = NULL;
	for (int i = 0; i < 10; i++)
	{
		if (pVTable->pNodePlayers[i] != NULL)
		{
			if (pVTable->pNodePlayers[i]->bIfAINode == false)
			{
				nodePlayer = pVTable->pNodePlayers[i];
			}
			iNowAllNum++;
		}
		else if(iFreePos == -1)
		{
			iFreePos = i;
		}
	}
	_log(_DEBUG, "NewATable", "CallBackCheckAddControlAI:user[%d],iNowAllNum[%d:%d],iFreePos[%d]", nodePlayer != NULL ?nodePlayer->iUserID:0, iNowAllNum, pVTable->iMinPlayerNum, iFreePos);
	if (iNowAllNum < pVTable->iMinPlayerNum && iFreePos != -1 && nodePlayer != NULL)
	{
		int iNeedNum = pVTable->iMinPlayerNum - 1;
		SendGetVirtualAIInfoReq(0, nodePlayer->iUserID, iNeedNum);
		return true;
	}
	return false;
}

bool NewAssignTableLogic::IfDynamicSeatGame()
{
	return false;
}
