#include "AssignTableHandler.h"
#include <time.h>
#include "Util.h"
#include "log.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include "conf.h"
#include <algorithm>
#include "EpollSockThread.h"
#include "RoomAndServerLogic.h"
#include <limits.h> 

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/


AssignTableHandler::AssignTableHandler(MsgQueue *pSendQueue)
{
	m_pSendQueue = pSendQueue;

	m_iReadConfCD = 600;

	memset(&m_stAssignStaInfo,0,sizeof(NCAssignStaInfoDef));
	for (int i = 0; i < 5; i++)
	{
		m_iRoomType[i] = 4 + i;
	}
	m_pQueueForRedis = NULL;
	m_iSetRecordLogCDConf = 600;//10分钟同步一次
	GetValueInt(&m_iSetRecordLogCDConf, "record_log_tm", "local_test.conf", "System Base Info", "600");//方便本地测试用
	m_iRecordLogCDTm = m_iSetRecordLogCDConf;
	InitConf();
	m_pRoomServerLogic = new RoomAndServerLogic(RoomAndServerLogic::CALL_ASSIGN, this);
	m_pRoomServerLogic->SetSendQueue(m_pSendQueue);
	//初试化一批AI
	vector<int>vcVirtualID;
	vector<VirtualAIInfoDef*>vcAllVirtualAI;
	for (int i = g_iMinVirtualID; i <= g_iMaxVirtualID; i++)
	{
		vcVirtualID.push_back(i);
	}
	while (!vcVirtualID.empty())
	{
		int iRandIndex = rand() % vcVirtualID.size();//初始虚拟AI打乱用
		VirtualAIInfoDef* pAIInfo = GetFreeVirtualAI(true);
		pAIInfo->iVirtualID = vcVirtualID[iRandIndex];
		//_log(_ERROR, "ATH", "ini_1:GetFreeVirtualAI:usedAI[%d]", pAIInfo == NULL ? 0 : pAIInfo->iVirtualID);
		vcVirtualID.erase(vcVirtualID.begin() + iRandIndex);
		m_hashVirtual.Add((void *)(&pAIInfo->iVirtualID), sizeof(int), pAIInfo);
		vcAllVirtualAI.push_back(pAIInfo);
	}
	for (int i = 0; i < vcAllVirtualAI.size(); i++)
	{
		m_vcFreeVirtulAI.push_back(vcAllVirtualAI[i]);
	}
	_log(_ERROR, "ATH", "m_vcFreeVirtulAI size[%d]", m_vcFreeVirtulAI.size());
	long long iVTableID = time(NULL);//保证每次重启后房号不会跟之前的重复导致游戏服务器已使用过该房号
	if (iVTableID > g_iMaxUseIntValue)
	{
		iVTableID = 0;
	}
	m_iVTableID = iVTableID;
	m_iATHTime = 0;
}

AssignTableHandler::~AssignTableHandler()
{
	delete m_pRoomServerLogic;
}

void AssignTableHandler::SetRedisQueue(MsgQueue* pQueueForRedis)
{
	m_pQueueForRedis = pQueueForRedis;
	GetAssignConf();
}

void AssignTableHandler::HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex)
{
	//_log(_DEBUG, "ATH", "AssignTableHandler::HandleMsg iMsgType[%x]", iMsgType);
	if (iMsgType == GAME_ROOM_INFO_REQ_RADIUS_MSG)
	{
		HandleGameRoomInfoReqMsg(szMsg, iSocketIndex);
	}
	else if (iMsgType == GAME_AUTHEN_REQ_RADIUS_MSG)
	{
		HandleGameServerAuthenMsg(szMsg, iSocketIndex);
	}
	else if (iMsgType == GAME_SYS_ONLINE_TO_CENTER_MSG)
	{
		HandleGameServerSysOnlineMsg(szMsg, iSocketIndex);
	}
	else if (iMsgType == GAME_SYS_DETAIL_TO_CENTER_MSG)
	{
		HandleGameServerDetailMsg(szMsg, iSocketIndex);
	}
	else if (iMsgType == NEW_CENTER_GET_USER_SIT_MSG)
	{
		HandleGetUserSitMsg(szMsg, iLen,iSocketIndex);
	}
	else if (iMsgType == NEW_CNNTER_USER_SIT_ERR)
	{
		HandleUserSitErrMsg(szMsg, iSocketIndex, iLen);
	}
	else if (iMsgType == NEW_CENTER_ASSIGN_TABLE_OK_MSG)
	{
		HandleAssignTableOkMsg(szMsg);
	}
	else if (iMsgType == NEW_CNNTER_USER_ASSIGN_ERR)
	{
		HandleUserAssignErrMsg(szMsg, iSocketIndex, iLen);
	}
	else if (iMsgType == NEW_CENTER_USER_LEAVE_MSG)
	{
		HandleUserLeaveMsg(szMsg, iSocketIndex);
	}
	else if (iMsgType == NEW_CENTER_FREE_VTABLE_MSG)
	{
		HandleFreeVTableMsg(szMsg, iSocketIndex);
	}
	else if (iMsgType == NEW_CENTER_TABLE_NUM_MSG)
	{
		HandleRefreshLeftTabNum(szMsg, iSocketIndex);
	}
	else if (iMsgType == REDIS_GET_PARAMS_MSG)
	{
		HandleGetParamsMsg(szMsg, iSocketIndex);
	}
	else if (iMsgType == RD_GET_VIRTUAL_AI_INFO_MSG)
	{
		HandleSendGetVirtualAIInfoReq(szMsg, iLen);
	}
	else if (iMsgType == RD_GET_VIRTUAL_AI_RES_MSG)
	{
		HandleGetVirtualAIMsg(szMsg, iLen);
	}
	else if (iMsgType == RD_USE_CTRL_AI_MSG)
	{
		HandleReleaseRdCTLAINodeReq(szMsg, iLen);
	}
}

void AssignTableHandler::HandleRefreshLeftTabNum(char* msgData, int iSocketIndex)
{
	m_pRoomServerLogic->HandleRefreshLeftTabNum(msgData, iSocketIndex);
}

void AssignTableHandler::CallBackOnTimer(int iTimeNow, int iTimeGap)
{
	m_iReadConfCD--;
	if (m_iReadConfCD < 0)
	{
		m_iReadConfCD = 600;
		GetAssignConf();
		_log(_DEBUG, "ATH", "CheckLeftVirtual AI num[%d]", m_vcFreeVirtulAI.size());
	}
	m_iRecordLogCDTm--;
	if (m_iRecordLogCDTm <= 0)
	{
		m_iRecordLogCDTm = m_iSetRecordLogCDConf;
		if (m_pQueueForRedis != NULL)
		{
			SendNCAssignStatInfo(iTimeNow);
		}
	}
	m_iATHTime += iTimeGap;
	//_log(_DEBUG, "ATH", "m_iATHTime = %d,iTimeGap=%d", m_iATHTime, iTimeGap);
}

void AssignTableHandler::CallBackNextDay(int iTimeBeforeFlag, int iTimeNowFlag)
{
	if (m_pQueueForRedis != NULL)
	{
		SendNCAssignStatInfo(iTimeBeforeFlag);
	}	
}

void AssignTableHandler::EnQueueSendMsg(char* _msg, int iLen, int iSocketIndex)
{
	//_log(_ERROR, "TaskHandler" ,"EnQueueSendMsg type = %d",((MsgHeadDef *)_msg)->cMsgType);
	MsgHeadDef* pMsgHead = (MsgHeadDef*)_msg;
	pMsgHead->cVersion = MESSAGE_VERSION;
	pMsgHead->iSocketIndex = iSocketIndex;
	pMsgHead->cFlag1 = 1;
	if (m_pSendQueue)
		m_pSendQueue->EnQueue(_msg, iLen);
}

void AssignTableHandler::HandleGameRoomInfoReqMsg(char* msgData, int iSocketIndex)
{
	m_pRoomServerLogic->HandleGameRoomInfoReqMsg(msgData, iSocketIndex);
}

void AssignTableHandler::HandleGameServerAuthenMsg(char* msgData, int iSocketIndex)
{
	int iServerID = m_pRoomServerLogic->HandleGameServerAuthenMsg(msgData, iSocketIndex);
	GameRDAuthenReqPreDef* pAuthenReq = (GameRDAuthenReqPreDef*)msgData;
	//_log(_ERROR, "ATH", "HandleGameServerAuthenMsg iReqType[%d] iServerID[%d] iHeadSocket[%d] iSocketIndex[%d]", pAuthenReq->iReqType, iServerID, pAuthenReq->msgHeadInfo.iSocketIndex, iSocketIndex);
	if (iSocketIndex < 0) //游戏服务器连接断开
	{
		map<int, WaitTableUserInfoDef*>::iterator iter = m_mapAllWaitTableUser.begin();
		while (iter != m_mapAllWaitTableUser.end())
		{
			if (iter->second->iInServerID == iServerID)//所有在这个服务器的用户也都要去掉，不能再分配了
			{
				int iUserID = iter->second->iUserID;				
				_log(_ERROR, "ATH", "ServerDisnnect iServerID[%d] clear AllWaitTableUser [%d]", iServerID, iUserID);
				//从虚拟桌中去掉该玩家
				if (iter->second->iVTableID > 0)
				{
					CheckRemoveVTableUser(iter->second);
				}
				AddToFreeWaitUsers(iter->second);
				m_mapAllWaitTableUser.erase(iter++);				
				continue;
			}
			iter++;
		}
		_log(_ERROR, "ATH", "ServerDisnnect iServerID[%d]", iServerID);
	}
}

void AssignTableHandler::HandleGameServerDetailMsg(char* msgData, int iSocketIndex)
{
	m_pRoomServerLogic->HandleGameServerDetailMsg(msgData, iSocketIndex);
}

void AssignTableHandler::HandleGameServerSysOnlineMsg(char* msgData, int iSocketIndex)
{
	m_pRoomServerLogic->HandleGameServerSysOnlineMsg(msgData, iSocketIndex);
}

void AssignTableHandler::HandleGetUserSitMsg(char *msgData, int iLen, int iSocketIndex)
{
	NCenterUserSitReqDef* pMsg = (NCenterUserSitReqDef*)msgData;
	//_log(_DEBUG, "ATH", "HandleGetUserSitMsg user[%d] serverInfo [%d],m_iATHTime[%d],iLen[%d:%d],openAI[%d]", pMsg->iUserID, pMsg->iServerID, m_iATHTime, iLen,sizeof(NCenterUserSitReqNewDef), m_iAssignAI);
	OneGameServerDetailInfoDef* pUserServerInfo = m_pRoomServerLogic->GetServerInfo(pMsg->iServerID);
	if (pUserServerInfo == NULL)
	{
		_log(_ERROR, "ATH", "HandleGetUserSitMsg user[%d] has no serverInfo [%d]", pMsg->iUserID, pMsg->iServerID);
		return;
	}
	int iUserID = pMsg->iUserID;
	if (iUserID <= g_iMaxVirtualID)
	{
		return;
	}
	int iRoomIndex = -1;
	for (int j = 0; j < 5; j++)
	{
		if (m_iRoomType[j] == pMsg->iEnterIfFreeRoom)
		{
			iRoomIndex = j;
			break;
		}
	}
	if (iRoomIndex == -1)
	{
		_log(_ERROR, "AssignHandler", "HandleGetUserSitMsg user[%d]enter room[%d] err", pMsg->iUserID, pMsg->iEnterIfFreeRoom);
		return;
	}
	
	int iServerState = m_pRoomServerLogic->JudgeIfServerCanEnter(pMsg->iServerID);//返回值0关闭，1正常，2维护
	if (iServerState != 1) //玩家所在的游戏服务器已经不处于开放状态了，先让玩家换服，再开始后续流程
	{		
		while (true)
		{
			if (iServerState == 2 && pMsg->iVipType == 96 && pMsg->iSpeMark > 0)//维护号可以继续呆在已经维护的服务器内
			{
				break;
			}
			NCenterReqUserLeaveMsgDef  msgReqLeave;
			memset(&msgReqLeave, 0, sizeof(NCenterReqUserLeaveMsgDef));
			msgReqLeave.msgHeadInfo.iMsgType = NEW_CENTER_REQ_USER_LEAVE_MSG;
			msgReqLeave.iUserID = pMsg->iUserID;
			if (iServerState == 0)
			{
				msgReqLeave.iReqType = 0;//0服务器不在开放时间内，1服务器进入维护状态且无其他可切换服务器，2服务器进入维护状态需要切换至指定服务器
			}
			else if (iServerState == 2 && (pMsg->iVipType != 96 || pMsg->iSpeMark == 0))//非维护号用户所在服务器是维护状态，判断下有没有其他服务器可以用
			{
				OneGameServerDetailInfoDef* pCommendServer = m_pRoomServerLogic->GetCommendServerInfo();			
				if (pCommendServer != NULL)
				{
					msgReqLeave.iReqType = 2;
					msgReqLeave.iNewServerIP = pCommendServer->iIP;
					msgReqLeave.iNewServerPort = (ushort)pCommendServer->sPort;
					msgReqLeave.iNewInnerIP = pCommendServer->iInnerIP;
					msgReqLeave.iNewInnerPort = (ushort)pCommendServer->sInnerPort;
				}
				else
				{
					msgReqLeave.iReqType = 1;
				}
				EnQueueSendMsg((char*)&msgReqLeave, sizeof(NCenterReqUserLeaveMsgDef), iSocketIndex);
			}
			_log(_ERROR, "ATH", "HandleGetUserSitMsg error user[%d] serverid[%d] iServerState[%d] newID[%d] newPort[%d] inner[%d][%d]", pMsg->iUserID, pMsg->iServerID, iServerState, 
				msgReqLeave.iNewServerIP, msgReqLeave.iNewServerPort, msgReqLeave.iNewInnerIP, msgReqLeave.iNewInnerPort);
			return;
		}	
	}		
	WaitTableUserInfoDef* pWaitUserInfo = FindWaitUser(iUserID);
	bool bNewAdd = false;
	if (pWaitUserInfo == NULL)
	{
		pWaitUserInfo = GetFreeWaitUsers(iUserID);
		m_mapAllWaitTableUser[iUserID] = pWaitUserInfo;
		pWaitUserInfo->iUserID = iUserID;
		pWaitUserInfo->iNowSection = -1;
		bNewAdd = true;
		/*_log(_DEBUG, "AssignHandler", "HandleGetUserSitMsg user[%d],lastIndex[%d],play[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", pMsg->iUserID, pWaitUserInfo->iLastPlayerIndex,
			pWaitUserInfo->iLatestPlayUser[0], pWaitUserInfo->iLatestPlayUser[1], pWaitUserInfo->iLatestPlayUser[2], pWaitUserInfo->iLatestPlayUser[3], pWaitUserInfo->iLatestPlayUser[4],
			pWaitUserInfo->iLatestPlayUser[5], pWaitUserInfo->iLatestPlayUser[6], pWaitUserInfo->iLatestPlayUser[7], pWaitUserInfo->iLatestPlayUser[8], pWaitUserInfo->iLatestPlayUser[9]);*/
		//_log(_DEBUG, "AssignHandler", "HandleGetUserSitMsg user[%d]status[%d] add", iUserID, pWaitUserInfo->iTableStatus);
	}
	pWaitUserInfo->iAllGameCnt = pMsg->iAllGameCnt;
	pWaitUserInfo->iAllWinCnt = pMsg->iWinCnt;
	pWaitUserInfo->iAllLoseCnt = pMsg->iLoseCnt;
	pWaitUserInfo->iInServerID = pMsg->iServerID;
	pWaitUserInfo->iNowSection = GetUserRate(pWaitUserInfo);
	pWaitUserInfo->iIfFreeRoom = pMsg->iEnterIfFreeRoom;
	pWaitUserInfo->iLastPlayerIndex = pMsg->iLastPlayerIndex;
	pWaitUserInfo->iVipType = pMsg->iVipType;
	SetWaitUserPlayType(pWaitUserInfo, pMsg->iPlayType);
	pWaitUserInfo->iSpeMark = pMsg->iSpeMark;
	for (int i = 0; i < 10; i++)
	{
		pWaitUserInfo->iLatestPlayUser[i] = pMsg->iLatestPlayUser[i];
	}
	memcpy(pWaitUserInfo->szIP, pMsg->szIP,20);
	pWaitUserInfo->iMarkLv = pMsg->iUserMarkLv;
	
	if (pMsg->iReqAssignCnt == 0)//今日首次入座，统计下概率
	{
		pWaitUserInfo->iReqAssignCnt = -1;
		JudgeUserGameRateStat(pWaitUserInfo);		
	}
	bool bNewTurnFirstSend = false;
	if (pWaitUserInfo->iReqAssignCnt == -1 || bNewAdd)
	{
		pWaitUserInfo->iFirstATHTime = m_iATHTime;
		pWaitUserInfo->iReqAssignCnt = 1;
		bNewTurnFirstSend = true;
	}
	int iLastAssignUser[10] = { 0 };
	int iLastAssignUserCnt = 0;
	int iLastAssignVTable = 0;
	int iForceAddVTable = 0;
	int iIfFromSitReq = 0;
	if (pWaitUserInfo->iVTableID > 0)
	{
		iLastAssignVTable = pWaitUserInfo->iVTableID;
		VirtualTableDef* pLastVTable = NULL;
		GetUserVTableAssignUser(pWaitUserInfo->iVTableID, iLastAssignUser, iLastAssignUserCnt, &pLastVTable);
	}
	_log(_DEBUG, "ATH", "HandleSit user[%d],serverid[%d],iSocketIdx[%d] iRoomIndex[%d] iPlayType[%d],openAI[%d]", iUserID, pMsg->iServerID, iSocketIndex, iRoomIndex, pWaitUserInfo->iPlayType, m_iAssignAI);
	if (iLen >= sizeof(NCenterUserSitReqNewDef))
	{
		NCenterUserSitReqNewDef* pMsgReqNew = (NCenterUserSitReqNewDef*)msgData;
		pWaitUserInfo->iHeadImg = pMsgReqNew->iHeadImg;
		sprintf(pWaitUserInfo->szNickName,"%s", pMsgReqNew->szNickName);
		if (strlen(pWaitUserInfo->szNickName) <= 0)
		{
			sprintf(pWaitUserInfo->szNickName, "Guest%d", iUserID);
		}
		pWaitUserInfo->iFrameID = pMsgReqNew->iFrameID;
		int iIfHaveVTable = (pMsgReqNew->iExtraInfo >> 2) & 0x1;
		_log(_ALL, "ATH", "HandleSit user[%d,%d],iIfHaveVTable[%d]", iUserID, pWaitUserInfo->iVTableID, iIfHaveVTable);
		iIfFromSitReq = (pMsgReqNew->iExtraInfo >> 3) & 0x1;
		int* pInt = (int*)(msgData + sizeof(NCenterUserSitReqNewDef));
		if (iIfHaveVTable == 1)
		{
			iForceAddVTable = *pInt;
			_log(_ALL, "ATH", "HandleSit user[%d,%d],iForceAddVTable[%d]", iUserID, pWaitUserInfo->iVTableID, iForceAddVTable);
			AddUserToVTable(pWaitUserInfo->iUserID,pWaitUserInfo, iForceAddVTable);
			pInt++;
		}		
	}
	_log(_DEBUG, "ATH", "HandleSit user[%d],server[%d],bNewAdd[%d],st[%d][%d],iReqCnt[%d,%d],enter[%d],type[%d,%d]", pMsg->iUserID, pMsg->iServerID, bNewAdd, pWaitUserInfo->iTableStatus, iIfFromSitReq,pWaitUserInfo->iReqAssignCnt, pMsg->iReqAssignCnt, pWaitUserInfo->iIfFreeRoom, pWaitUserInfo->iPlayType, pWaitUserInfo->iMinPlayer);
	/*_log(_DEBUG, "AssignHandler", "HandleSit user[%d],index[%d],[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", pMsg->iUserID, pWaitUserInfo->iLastPlayerIndex, pWaitUserInfo->iLatestPlayUser[0], pWaitUserInfo->iLatestPlayUser[1], pWaitUserInfo->iLatestPlayUser[2], pWaitUserInfo->iLatestPlayUser[3], pWaitUserInfo->iLatestPlayUser[4],
		pWaitUserInfo->iLatestPlayUser[5], pWaitUserInfo->iLatestPlayUser[6], pWaitUserInfo->iLatestPlayUser[7], pWaitUserInfo->iLatestPlayUser[8], pWaitUserInfo->iLatestPlayUser[9]);*/
	_log(_DEBUG, "ATH", "HandleSit user[%d]tm[%d],VT[%d],force[vt:%d]", pMsg->iUserID, pWaitUserInfo->iFirstATHTime, pWaitUserInfo->iVTableID, iForceAddVTable);
	//检查下是否已经被分配了（有可能这边已经分配了，客户端还没收到通知，反复发送请求）
	if (iIfFromSitReq == 1)
	{
		pWaitUserInfo->iTableStatus = 0;
	}
	if (pWaitUserInfo->iTableStatus == 1 || pWaitUserInfo->iTableStatus == 2)
	{
		m_stAssignStaInfo.iAbnormalSitReq++;
		if (bNewTurnFirstSend)
		{
			m_stAssignStaInfo.iAbnormalSitUser++;
		}
		_log(_ERROR, "ATH", "HandleGetUserSitMsg user[%d,%d]-[%d] bNewAdd[%d],has been assigned", pMsg->iUserID, iUserID, pWaitUserInfo->iTableStatus, bNewAdd);
		return;
	}
	if (iForceAddVTable > 0)//先不分配，也不参与分配，等后续玩家请求来齐才可以
	{
		return;
	}
	WaitTableUserInfoDef* pFindUser[10] = { 0 };
	pFindUser[0] = pWaitUserInfo;//自己先算找到的
	int iFindUserIndex = 1;
	//将之前已经分配好的在一个虚拟桌的直接分配进来（组队用户在组队的时候已经被加到对应的虚拟桌了）
	int iLastVirtualAI[10] = { 0 };//之前桌上已经分配的虚拟AI
	int iLastVAIIndex = 0;
	if (pWaitUserInfo->iVTableID > 0)
	{
		AddFindPlayerFromVTable(pWaitUserInfo, pFindUser, iFindUserIndex, iLastVirtualAI,iLastVAIIndex);
	}
	//重置下已同桌玩家的开始配桌时间
	for (int i = 0;i<iFindUserIndex;i++)
	{
		if (pFindUser[i]->iReqAssignCnt == -1)
		{
			pFindUser[i]->iReqAssignCnt = 1;
			pFindUser[i]->iFirstATHTime = m_iATHTime;
		}		
	}
	_log(_DEBUG, "AssignHandler", "HandleSit user[%d],find vt[%d] before[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],lastAI[%d,%d,%d,%d]",iUserID, pWaitUserInfo->iVTableID, pFindUser[0]!=NULL? pFindUser[0]->iUserID:0, pFindUser[1] != NULL ? pFindUser[1]->iUserID:0 ,pFindUser[2] != NULL ? pFindUser[2]->iUserID:0 , 
		pFindUser[3] != NULL ? pFindUser[3]->iUserID : 0, pFindUser[4] != NULL ? pFindUser[4]->iUserID : 0, pFindUser[5] != NULL ? pFindUser[5]->iUserID : 0, pFindUser[6] != NULL ? pFindUser[6]->iUserID : 0, pFindUser[7] != NULL ? pFindUser[7]->iUserID : 0, pFindUser[8] != NULL ? pFindUser[8]->iUserID : 0,
		pFindUser[9] != NULL ? pFindUser[9]->iUserID : 0, iLastVirtualAI[0], iLastVirtualAI[1], iLastVirtualAI[2], iLastVirtualAI[3]);
	bool bFindAll = false;
	int iAINum = 0;
	//寻桌流程
	//1，当前是否已经配好桌了
	//2, 遍历所有虚拟桌，寻找可以和该桌同桌的玩家（可拆其他桌）
	//3，有其他可入座但不够开局的虚拟桌，入座该虚拟桌，通知redis修改人数，否则进入4
	//4，若当前无虚拟桌,单独创建一个虚拟桌，进5
	//5,判断是否已经可以直接AI开局,若还不需要ai开局，看是否需要补虚拟AI进去模拟玩家进出
	int iAllFindUser = iFindUserIndex + iLastVAIIndex;//已经找到的ai也要加进来判断是否直接可以直接开局了
	int iOldFindUserIndex = iFindUserIndex;
	while (true)
	{
		//--1.当前是否已经配好桌了
		if (!IfDynamicSeatGame())
		{
			if (pWaitUserInfo->iPlayType == 0 && iFindUserIndex > 1)
			{
				int iMinNeedNum = 0;
				for (int i = 0; i < iFindUserIndex; i++)
				{
					if (pFindUser[i]->iMinPlayer > iMinNeedNum)
					{
						iMinNeedNum = pFindUser[i]->iMinPlayer;
					}
				}
				if (iMinNeedNum == 0 || iAllFindUser >= iMinNeedNum)//2个快速开始的用户能直接开局的
				{
					bFindAll = true;
					iAINum = iLastVAIIndex;
					break;
				}
			}
			else if (pWaitUserInfo->iMinPlayer > 0 && iAllFindUser >= pWaitUserInfo->iMinPlayer)
			{
				bFindAll = true;
				iAINum = iLastVAIIndex;
				break;
			}
		}	
		//新逻辑--2.根据请求配桌的时间，未达到时间配置前不提前匹配不同房间的人
		bool bHaveMixRoom = false;//是否已经混服了，如果允许混服，且当前已经混服，只找同房间这轮判断可以跳过了
		if (m_iIfForbidPreMixRoom == 0)
		{
			for (int i = 0; i < iFindUserIndex; i++)
			{
				for (int j = 0; j < iFindUserIndex; j++)
				{
					int iRoomGap = abs(pFindUser[i]->iIfFreeRoom - pFindUser[j]->iIfFreeRoom);
					if (iRoomGap >= 1)
					{
						bHaveMixRoom = true;
						break;
					}
				}
				if (bHaveMixRoom)
				{
					break;
				}
			}
		}
		vector<CheckAssignTableDef>vcAssignTableFirst;//只有同房间的
		vector<CheckAssignTableDef>vcAssignTableSecond;//包括同房间以及相邻房间的
		vector<CheckAssignTableDef>vcAssignTableThird;//包括同房间，相邻房间以及跨2级房间的
		if (!bHaveMixRoom)
		{
			bFindAll = FindAllCanAssignUser(pWaitUserInfo, pFindUser, iFindUserIndex, vcAssignTableFirst, 0);//只找同类型房间的
		}		
		if (!bFindAll)
		{
			int iWaitMaxTime = -1;
			for (int i = 0; i < iFindUserIndex; i++)
			{
				int iTmGap = m_iATHTime - pFindUser[i]->iFirstATHTime;
				if (iTmGap > iWaitMaxTime)
				{
					iWaitMaxTime = iTmGap;
				}
			}
			_log(_ALL, "AssignHandler", "HandleSit user[%d],iWaitMaxTime[%d],secConf[%d,%d] ForbidMacth[%d]", iUserID, iWaitMaxTime, m_iFindSameRoomSec, m_iFindNeighborRoomSec, m_iForbidMacthRoom);
			if (iWaitMaxTime > m_iFindSameRoomSec)
			{				
				bFindAll = FindAllCanAssignUser(pWaitUserInfo, pFindUser, iFindUserIndex, vcAssignTableSecond, 1);//可以找相邻的房间了
				if (iWaitMaxTime > m_iFindNeighborRoomSec)
				{
					if (!bFindAll && m_iForbidRoom7With5 == 0)
					{
						bFindAll = FindAllCanAssignUser(pWaitUserInfo, pFindUser, iFindUserIndex, vcAssignTableThird, 2);//可以找跨2级房间的了
					}
				}				
			}
		}
		if (bFindAll)
		{
			break;
		}
		//不够开桌，但找到的同房间同玩法的可以先拉到一起了（注意，确保拉过后的人数比之前的要多，否则就不用配在一起了，典型的如当前请求是a，另一桌是bc，a不能和b同桌，即使a和c可以同桌，也不用配在一起）		
		//_log(_DEBUG, "AssignHandler", "HandleSit user[%d],forbidPre[%d],find[%d,%d,%d]",iUserID, m_iIfForbidPreMixRoom, vcAssignTableThird.size(), vcAssignTableSecond.size(), vcAssignTableFirst.size());
		if (m_iIfForbidPreMixRoom == 0)//可以在开局前提前让玩家混服坐在一起
		{
			if (!vcAssignTableThird.empty())
			{
				TogetherUsersWithAssignTable(pWaitUserInfo, pFindUser, iFindUserIndex, vcAssignTableThird);
			}
			else if (!vcAssignTableSecond.empty())
			{
				TogetherUsersWithAssignTable(pWaitUserInfo, pFindUser, iFindUserIndex, vcAssignTableSecond);
			}
			else
			{
				TogetherUsersWithAssignTable(pWaitUserInfo, pFindUser, iFindUserIndex, vcAssignTableFirst);
			}
		}
		else
		{		
			TogetherUsersWithAssignTable(pWaitUserInfo, pFindUser, iFindUserIndex, vcAssignTableFirst);
		}
		//--4.若当前无虚拟桌,单独创建一个虚拟桌
		if (pWaitUserInfo->iVTableID == 0)
		{
			VirtualTableDef* pVTable = GetFreeVTable();
			//_log(_DEBUG, "AssignHandler", "HandleSit user[%d],add to new Vtable[%d]", iUserID, pVTable->iVTableID);
			m_mapAllVTables[pVTable->iVTableID] = pVTable;
			for (int i = 0; i < iFindUserIndex; i++)
			{
				AddUserToVTable(pFindUser[i]->iUserID, pFindUser[i], pVTable);
			}
			break;
		}
		//--5.判断是否已经可以直接AI开局,若还不需要ai开局，看是否需要补虚拟AI进去模拟玩家进出
		VirtualTableDef* pUserVTable = NULL;
		if (pWaitUserInfo->iVTableID > 0)
		{
			pUserVTable = FindVTable(pWaitUserInfo->iVTableID);			
		}
		if (pUserVTable != NULL)
		{		
			if (m_iAssignAI == 1)
			{
				int iPutAIRate = 0;
				int iMinWaitAiSec = 0;
				if (m_pRoomServerLogic->m_iAllPlayerNum <= m_iWaitAIOnline[0])
				{
					iMinWaitAiSec = m_iWaitAISec[0];
					iPutAIRate = m_iPutAIRate[0];
				}
				else if (m_pRoomServerLogic->m_iAllPlayerNum > m_iWaitAIOnline[1])
				{
					iMinWaitAiSec = m_iWaitAISec[2];
					iPutAIRate = m_iPutAIRate[2];
				}
				else
				{
					iMinWaitAiSec = m_iWaitAISec[1];
					iPutAIRate = m_iPutAIRate[1];
				}
				int iReqAssignTmGap = 0;
				int iVTableRealUser[10] = { 0 };
				int iVTableRealIndex = 0;
				int iMinNeedNum = 0;
				int iVTableUserNum = 0;
				for (int i = 0; i < 10; i++)
				{
					if (pUserVTable->iUserID[i] > 0)
					{
						iVTableUserNum++;
					}
					if (pUserVTable->iUserID[i] > g_iMaxVirtualID)
					{
						WaitTableUserInfoDef* pTempUser = FindWaitUser(pUserVTable->iUserID[i]);
						if (pTempUser != NULL && pTempUser->iMinPlayer > iMinNeedNum)
						{
							iMinNeedNum = pTempUser->iMinPlayer;
						}
						if (pTempUser != NULL)
						{
							int iTmGap = m_iATHTime - pTempUser->iFirstATHTime;
							if (iTmGap > iReqAssignTmGap)
							{
								iReqAssignTmGap = iTmGap;
							}							
						}
						iVTableRealUser[iVTableRealIndex] = pUserVTable->iUserID[i];
						iVTableRealIndex++;
					}
				}
				_log(_DEBUG, "AssignHandler", "HandleSit user[%d],findAI iVTableUserNum[%d:%d] iWaitAISec[%d:%d] iMinPlayer[%d] online[%d],iPutAIRate[%d]",
					iUserID, iVTableUserNum, iMinNeedNum, iReqAssignTmGap, iMinWaitAiSec, pWaitUserInfo->iMinPlayer, m_pRoomServerLogic->m_iAllPlayerNum, iPutAIRate);
				if (iReqAssignTmGap > iMinWaitAiSec && iMinWaitAiSec >= 0)
				{
					if (pWaitUserInfo->iMinPlayer == 0 && iMinNeedNum == 0 && iVTableRealIndex == 1)
					{
						//桌上只有一个快速开始的玩家,不应该存在2个以上快速开始的用户到这里还没有开局
						SetWaitUserPlayType(pWaitUserInfo, GetPlayTypeByIdx(2));
						iAINum = pWaitUserInfo->iMinPlayer - 1;
					}
					else if (iMinNeedNum > 0)
					{
						iAINum = iMinNeedNum - iVTableRealIndex;
					}
					if (iAINum > 0)
					{
						int iAddAINum = iAINum - iLastVAIIndex;
						_log(_DEBUG, "AssignHandler", "HandleSit user[%d],iAINum[%d]iAddAINum[%d],iLastVAIIndex[%d][%d,%d,%d,%d]", iUserID,iAINum, iAddAINum, iLastVAIIndex, iLastVirtualAI[0], iLastVirtualAI[1], iLastVirtualAI[2], iLastVirtualAI[3]);
						bFindAll = true;
						//将AI直接补充至虚拟桌，若ai不够用，就不分配ai
						for (int i = 0; i < iAddAINum; i++)
						{
							VirtualAIInfoDef* pGetFreeAI = GetFreeValidVirtualAI(iVTableRealUser);
							_log(_DEBUG, "AssignHandler", "HandleSit user[%d],addAI[%d]=[%d]", iUserID,i, pGetFreeAI!=NULL? pGetFreeAI->iVirtualID:0);
							if (pGetFreeAI != NULL)
							{
								AddUserToVTable(pGetFreeAI->iVirtualID, NULL, pUserVTable);
								pGetFreeAI->iInVTableID = pUserVTable->iVTableID;
								iLastVirtualAI[iLastVAIIndex] = pGetFreeAI->iVirtualID;
								iLastVAIIndex++;
							}
							else
							{
								_log(_ERROR, "AssignHandler", "HandleSit user[%d] no enough AI[%d]",iUserID, iAINum);
								bFindAll = false;
								iAINum = 0;
								break;
							}
						}					
						break;
					}
					else
					{
						_log(_ERROR, "AssignHandler", "HandleSit user[%d]can't assign ai,minPlayer[%d],table[%d],need[%d],reqTm[%d,%d]",
							iUserID, pWaitUserInfo->iMinPlayer,iVTableUserNum, iMinNeedNum, pWaitUserInfo->iFirstATHTime,m_iATHTime);
					}
				}
				if (iPutAIRate > 0)//在保证不开局的情况下，每轮只补充一个虚拟AI
				{
					int iRandRate = rand() % 10000;
					if (iRandRate < iPutAIRate && (iMinNeedNum - iVTableUserNum) >= 2)
					{
						VirtualAIInfoDef* pGetFreeAI = GetFreeValidVirtualAI(iVTableRealUser);
						if (pGetFreeAI != NULL)
						{
							AddUserToVTable(pGetFreeAI->iVirtualID, NULL, pUserVTable);
							pGetFreeAI->iInVTableID = pUserVTable->iVTableID;
						}
					}
				}
			}
		}
		else //不应该出现这时候玩家还没有对应的虚拟桌
		{
			_log(_ERROR, "AssignHandler", "HandleSit user[%d] iVTableID=0", iUserID);
			return;
		}
		break;
	}	
	_log(_DEBUG, "AssignHandler", "HandleSit user[%d],bFindAll[%d] iAINum[%d]", pWaitUserInfo->iUserID, bFindAll, iAINum);
	if (bFindAll)//配齐了，通知服务器
	{		
		_log(_DEBUG, "AssignHandler", "HandleSit findOK [%d:%d][%d:%d][%d:%d][%d:%d][%d:%d]", pFindUser[0]!= NULL?pFindUser[0]->iUserID:0, pFindUser[0]!=NULL?pFindUser[0]->iInServerID:0,
			pFindUser[1] != NULL ? pFindUser[1]->iUserID:0, pFindUser[1] != NULL ? pFindUser[1]->iInServerID:0, pFindUser[2] != NULL ? pFindUser[2]->iUserID:0, pFindUser[2] != NULL ? pFindUser[2]->iInServerID:0,
			pFindUser[3] != NULL ? pFindUser[3]->iUserID:0, pFindUser[3] != NULL ? pFindUser[3]->iInServerID:0, pFindUser[4] != NULL ? pFindUser[4]->iUserID:0, pFindUser[4] != NULL ? pFindUser[4]->iInServerID:0);
		
		CallBackStartGame(pWaitUserInfo, pFindUser, iFindUserIndex, iAINum, iLastVAIIndex, iLastVirtualAI, iSocketIndex);
	}
	else
	{
		//统计未开始人数
		for (int i = 0; i < iFindUserIndex; i++)
		{
			if (pFindUser[i]->iUserID > g_iMaxVirtualID)
			{
				int iTimeGap = m_iATHTime - pFindUser[i]->iFirstATHTime;
				if (iTimeGap > 15)
				{
					m_stAssignStaInfo.iSendReqMoreNum++;
				}
				else if (iTimeGap > 10)
				{
					m_stAssignStaInfo.iSendReq3Num++;
				}
				else
				{
					m_stAssignStaInfo.iSendReq2Num++;
				}
			}
		}
		//没配齐，若本轮虚拟桌和上轮虚拟桌玩家有变化，通知服务器
		int iNowAssignUser[10] = { 0 };
		int iNowAssignUserCnt = 0;
		int iMinNeedPlayer = 0;
		VirtualTableDef* pNowVTable = NULL;
		if (pWaitUserInfo->iVTableID > 0)
		{
			iMinNeedPlayer = GetUserVTableAssignUser(pWaitUserInfo->iVTableID, iNowAssignUser, iNowAssignUserCnt, &pNowVTable);
		}
		_log(_DEBUG, "ATH", "lastFind[%d_%d:%d,%d,%d,%d],nowFind[%d_%d:%d,%d,%d,%d],iMinNeedPlayer[%d]", iLastAssignVTable, iLastAssignUserCnt, iLastAssignUser[0], iLastAssignUser[1],
			iLastAssignUser[2], iLastAssignUser[3], pWaitUserInfo->iVTableID, iNowAssignUserCnt, iNowAssignUser[0], iNowAssignUser[1], iNowAssignUser[2], iNowAssignUser[3], iMinNeedPlayer);
		bool bChanged = false;
		if (iLastAssignUserCnt != iNowAssignUserCnt || iLastAssignVTable != pWaitUserInfo->iVTableID)
		{
			bChanged = true;
		}
		else
		{
			for (int i = 0; i < iNowAssignUserCnt; i++)
			{
				bool bFind = false;
				for (int j = 0; j < iLastAssignUserCnt; j++)
				{
					if (iNowAssignUser[i] == iLastAssignUser[j])
					{
						bFind = true;
						break;
					}
				}
				if (!bFind)
				{
					bChanged = true;
					break;
				}
			}
		}
		if (bChanged)
		{			
			if (pNowVTable == NULL)
			{
				_log(_ERROR, "AssignHandler", "HandleGetUserSitMsg, user[%d] ,asVTable[%d] is null", iUserID, pWaitUserInfo->iVTableID);
				return;
			}
			//通知游戏服务器本轮最新虚拟配桌信息
			NCenterVirtualAssignMsgDef msgVirtualAssign;
			memset(&msgVirtualAssign, 0, sizeof(NCenterVirtualAssignMsgDef));
			msgVirtualAssign.msgHeadInfo.iMsgType = NEW_CENTER_VIRTUAL_ASSIGN_MSG;
			msgVirtualAssign.msgHeadInfo.cVersion = MESSAGE_VERSION;
			msgVirtualAssign.iVTableID = pWaitUserInfo->iVTableID;		
			msgVirtualAssign.iMinPlayerNum = iMinNeedPlayer;
			for (int i = 0; i < iNowAssignUserCnt; i++)
			{
				msgVirtualAssign.iUsers[i] = iNowAssignUser[i];
			}
			msgVirtualAssign.iPlayType = pNowVTable->iPlayType;
			SetVirtualAITableUser(msgVirtualAssign.iUsers);
			_log(_DEBUG, "AssignHandler", "HandleGetUserSitMsg, user[%d],asVTable[%d:%d,%d,%d,%d,%d],playType[%d]", iUserID, msgVirtualAssign.iVTableID,
				msgVirtualAssign.iUsers[0], msgVirtualAssign.iUsers[1], msgVirtualAssign.iUsers[2], msgVirtualAssign.iUsers[3], msgVirtualAssign.iUsers[4], msgVirtualAssign.iPlayType);
			EnQueueSendMsg((char*)&msgVirtualAssign, sizeof(NCenterVirtualAssignMsgDef), iSocketIndex);
		}
	}
}

int AssignTableHandler::GetPlayTypeByIdx(int i)
{
	int iType = 0;
	int iSeatNum = 4;//暂时都按照4人游戏来设定
	int iMaxSeatNum = iSeatNum;
	iType =  iSeatNum << 16 | iMaxSeatNum << 24;
	return iType;
}

void AssignTableHandler::HandleUserSitErrMsg(char *msgData, int iSocketIndex, int iLen)
{
	//入座错误信息要直接转发要换服的那个游戏服务器
	NCenterSyncUserSitErrDef* pMsg = (NCenterSyncUserSitErrDef*)msgData;
	if (pMsg->iSocketIndex > 0)//其他服请求换桌有异常，需要通知已经配桌所在的服务器
	{
		EnQueueSendMsg(msgData, sizeof(NCenterSyncUserSitErrDef), pMsg->iSocketIndex);
	}
	int iNVTableID = -1;
	if (iLen >= sizeof(NCenterSyncUserSitErrDef))
	{
		//_log(_DEBUG, "TEST", "HandleUserSitErrMsg iNVTableID[%d]", pMsg->iNVTableID);
		iNVTableID = pMsg->iNVTableID;
	}
	_log(_ERROR, "AssignHandler", "HandleUserSitErrMsg [%d-%d,%d-%d,%d-%d,%d-%d,%d-%d] iNVTableID[%d]", pMsg->iUsers[0],
		pMsg->iSitRt[0], pMsg->iUsers[1],pMsg->iSitRt[1], pMsg->iUsers[2], pMsg->iSitRt[2],
		pMsg->iUsers[3], pMsg->iSitRt[3], pMsg->iUsers[4], pMsg->iSitRt[4], iNVTableID);
	//修改下内存值
	for (int i = 0; i < 10; i++)
	{
		if (pMsg->iUsers[i] >= g_iMinVirtualID && pMsg->iUsers[i] <= g_iMaxVirtualID)
		{
			ReleaseVirtualAI(pMsg->iUsers[i]);
			continue;
		}
		if (pMsg->iUsers[i] > 0)
		{
			int iUserID = pMsg->iUsers[i];
			if (pMsg->iSitRt[i] == 1)//服务器没有这人
			{
				RemoveUserFromWait(iUserID);
			}
			else if (pMsg->iSitRt[i] == 2)//状态不对，不能被配桌
			{
				WaitTableUserInfoDef* pUser = FindWaitUser(iUserID);
				if (pUser != NULL)
				{
					pUser->iTableStatus = 1;
				}
			}
			else
			{
				WaitTableUserInfoDef* pUser = FindWaitUser(iUserID);
				if (pUser != NULL)
				{
					if (pMsg->iSitRt[i] < 0 || pUser->iNVTableID == iNVTableID || iNVTableID < 0)
					{
						pUser->iTableStatus = 0;
					}
					else
					{
						_log(_ERROR, "AssignHandler", "HandleUserSitErrMsg user[%d],mowNVt[%d]-reqNVT[%d]", iUserID, pUser->iNVTableID, iNVTableID);
					}
				}
			}
		}
	}
}

void AssignTableHandler::HandleUserAssignErrMsg(char * msgData, int iSocketIndex, int iLen)
{
	NCenterUserAssignErrDef *pMsgErr = (NCenterUserAssignErrDef*)msgData;
	_log(_DEBUG, "AssignHandler", "HandleUserAssignErrMsg[%d] user[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", pMsgErr->iNVTableID, 
		pMsgErr->iUserID[0],pMsgErr->iUserID[1],pMsgErr->iUserID[2],pMsgErr->iUserID[3],pMsgErr->iUserID[4],
		pMsgErr->iUserID[5],pMsgErr->iUserID[6],pMsgErr->iUserID[7],pMsgErr->iUserID[8],pMsgErr->iUserID[9]);
	for (int i = 0; i < 10; i++)
	{
		if (pMsgErr->iUserID[i] > 0)
		{
			WaitTableUserInfoDef* pUser = FindWaitUser(pMsgErr->iUserID[i]);
			if (pUser != NULL)
			{
				pUser->iTableStatus = 2;
				CheckRemoveVTableUser(pUser);
			}
		}
	}
}

void AssignTableHandler::InitConf()
{
	m_iForbidSameTableNum = 0;
	m_iLowRate = 45;
	m_iHightRate = 55;
	m_iRateMethod = 0;
	m_iHighSitWithLow = 1;
	m_iForbidRoom8With5 = 1;
	m_iForbidRoom8With6 = 0;
	m_iForbidRoom7With5 = 0;
	m_iForbidRoom6With5 = 0;
	m_iForbidRoom7With6 = 0;
	m_iForbidMacthRoom = 0;
	m_iIfForbidPreMixRoom = 1;
	
	m_iSameTmLimitSec = -1;
	m_iMaxMark9Num = 1;
	m_iFindSameRoomSec = 3;
	m_iFindNeighborRoomSec = 5;

	m_iAssignAI = 0;

	char cKey[20] = { 0 };
	for (int i = 0; i < 3; i++)
	{
		m_iWaitAISec[i] = -1;
		m_iPutAIRate[i] = 0;
		if (i<2)
		{
			m_iWaitAIOnline[i] = -1;
		}
	}

	static bool bReadConf = false;
	if (!bReadConf)
	{
		_log(_ERROR, "AssignTableHandler","ReadConf:m_iForbidSameTableNum[%d],m_iRecordLogCDTm[%d],maxMark9[%d]", m_iForbidSameTableNum, m_iRecordLogCDTm, m_iMaxMark9Num);
		_log(_ERROR, "AssignTableHandler", "ReadConf:Rate[%d]--[%d],method[%d],sit[%d],forbid[8&5[%d],8&6[%d],7&5[%d]],sameTm[%d]", m_iLowRate, m_iHightRate, m_iRateMethod, m_iHighSitWithLow, m_iForbidRoom8With5, m_iForbidRoom8With6, m_iForbidRoom7With5, m_iSameTmLimitSec);
		_log(_ERROR, "AssignTableHandler", "ReadConf:m_iWaitAIOnline[%d,%d],waitSec[%d,%d,%d],putAiRate[%d,%d,%d]", m_iWaitAIOnline[0], m_iWaitAIOnline[1], m_iWaitAISec[0], m_iWaitAISec[1], m_iWaitAISec[2], m_iPutAIRate[0], m_iPutAIRate[1], m_iPutAIRate[2]);
		_log(_ERROR, "AssignTableHandler", "ReadConf:RoomSec[%d,%d]m_iForbidMacthRoom[%d],m_iIfForbidPreMixRoom[%d]", m_iFindSameRoomSec, m_iFindNeighborRoomSec,m_iForbidMacthRoom, m_iIfForbidPreMixRoom);
		bReadConf = true;
	}
}


void AssignTableHandler::HandleAssignTableOkMsg(char *msgData)
{
	NCenterAssignTable0kMsgDef* pMsg = (NCenterAssignTable0kMsgDef*)msgData;
	bool bRightSec = true;
	bool bRightRoom = true;
	int iJudgeSec = -1;
	int iJudgeRoom = -1;
	int iOldIndex[10] = { 0 };
	int iTabUserCnt = 0;
	int iMatchUser[2] = { 0 };
	_log(_DEBUG, "AssignTableHandler", "TableOkMsg:[%d,%d,%d,%d,%d]", pMsg->iUserID[0], pMsg->iUserID[1], pMsg->iUserID[2], pMsg->iUserID[3], pMsg->iUserID[4]);
	for (int i = 0; i < 10; i++)
	{
		if (pMsg->iUserID[i] == 0)
		{
			break;
		}
		WaitTableUserInfoDef* pUser = FindWaitUser(pMsg->iUserID[i]);
		if (pUser != NULL)
		{
			if (iJudgeSec == -1)
			{
				iJudgeSec = pUser->iNowSection;
			}
			else if(pUser->iNowSection != iJudgeSec)
			{
				bRightSec = false;
			}
			if (iJudgeRoom == -1)
			{
				iJudgeRoom = pUser->iIfFreeRoom;
			}
			else if (pUser->iIfFreeRoom != iJudgeRoom)
			{
				bRightRoom = false;
			}
			pUser->iReqAssignCnt = -1;

			iTabUserCnt++;

			pUser->iTableStatus = 2;
			iOldIndex[i] = pUser->iLastPlayerIndex;//内存内不用改，用户每次入座会携带最新的已同桌信息的
			int iRoomIndex = pUser->iIfFreeRoom - 4;
			if (iRoomIndex >= 0 && iRoomIndex <= 4)
			{
				m_stAssignStaInfo.iStartGameOk[iRoomIndex]++;
			}
		}
		else
		{
			//换桌离开的用户不在map内的
			//_log(_ERROR, "CSM", "HandleAssignTableOkMsguser[%d] not found[%d] in m_mapAllWaitTableUser", m_iJudgeGameID, pMsg->iUserID[i]);
		}
	}

	if (iTabUserCnt == 4)
	{
		m_stAssignStaInfo.iSucTableOkNum++;
		if (bRightSec)
		{
			m_stAssignStaInfo.iRightSecTableCnt++;
		}
		if (bRightRoom)
		{
			m_stAssignStaInfo.iRightRoomTableCnt++;
		}
	}
	//通知redis

	RDNCSetLatestUserDef msgReq;
	memset(&msgReq,0,sizeof(RDNCSetLatestUserDef));
	msgReq.msgHeadInfo.iMsgType = REDID_NCENTER_SET_LATEST_USER_MSG;
	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq.iJudgeGameID = stSysConfigBaseInfo.iGameID;
	for (int i = 0; i < 10; i++)
	{
		msgReq.iLatestIndex[i] = iOldIndex[i];
		msgReq.iUserID[i] = pMsg->iUserID[i];
	}
	if (m_pQueueForRedis != NULL)
	{
		m_pQueueForRedis->EnQueue((void*)&msgReq, sizeof(RDNCSetLatestUserDef));
	}
}

int AssignTableHandler::GetUserRate(WaitTableUserInfoDef *pUserInfo)
{
	//0高档几率 1中档几率 2低档几率
	if (pUserInfo->iAllGameCnt < 10)//近期游戏次数太低，直接放在中档
	{
		return MID_RATE;
	}
	int iRate = 0;
	if (m_iRateMethod == 0)
	{
		iRate = pUserInfo->iAllWinCnt * 100 / pUserInfo->iAllGameCnt;
	}
	else
	{
		iRate = (pUserInfo->iAllGameCnt- pUserInfo->iAllLoseCnt) * 100 / pUserInfo->iAllGameCnt;
	}
	if (iRate <= m_iLowRate)
	{
		return LOW_RATE;
	}
	else if (iRate >= m_iHightRate)
	{
		return HIGH_RATE;
	}
	return MID_RATE;
}
void AssignTableHandler::JudgeUserGameRateStat(WaitTableUserInfoDef *pUserInfo)
{
	if (pUserInfo->iAllGameCnt < 10)
	{
		m_stAssignStaInfo.iUserRateCnt[10]++;
		return;
	}
	int iRate = 0;
	if (m_iRateMethod == 0)
	{
		iRate = pUserInfo->iAllWinCnt * 100 / pUserInfo->iAllGameCnt;
	}
	else
	{
		iRate = (pUserInfo->iAllGameCnt - pUserInfo->iAllLoseCnt) * 100 / pUserInfo->iAllGameCnt;
	}
	int iRateGap[9] = { 80,70,65,60,55,50,45,40,30 };
	if (iRate < iRateGap[8])
	{
		m_stAssignStaInfo.iUserRateCnt[9]++;
	}
	else
	{
		for (int i = 0; i < 9; i++)
		{
			if (iRate >= iRateGap[i])
			{
				m_stAssignStaInfo.iUserRateCnt[i]++;
				break;
			}
		}
	}
}
void AssignTableHandler::RemoveUserFromWait(int iUserID)
{
	map<int, WaitTableUserInfoDef*>::iterator iter = m_mapAllWaitTableUser.find(iUserID);
	if (iter != m_mapAllWaitTableUser.end())
	{
		if (iter->second->iVTableID > 0)
		{
			CheckRemoveVTableUser(iter->second);
		}
		AddToFreeWaitUsers(iter->second);
		m_mapAllWaitTableUser.erase(iter);		
	}
}

void AssignTableHandler::HandleUserLeaveMsg(char *msgData, int iSocketIndex)
{	
	NCenterUserLeaveMsgDef* pMsg = (NCenterUserLeaveMsgDef*)msgData;
	map<int, WaitTableUserInfoDef*>::iterator iter = m_mapAllWaitTableUser.find(pMsg->iUserID);
	int iJustResetStatus = pMsg->iExtraInfo & 0x1;
	_log(_DEBUG, "ATH", "HandleUserLeaveMsg user[%d],ResetStatus[%d]", pMsg->iUserID, iJustResetStatus);
	int iVTableID = 0;
	if (iter != m_mapAllWaitTableUser.end())
	{
		iVTableID = iter->second->iVTableID;
		if (iJustResetStatus == 1 && iter->second->iTableStatus != 1)
		{
			iter->second->iTableStatus = 0;
			CheckRemoveVTableUser(iter->second);     //20230921 防止已配桌玩家被重复配桌，先从原虚拟桌移掉
			return;
		}
		if (iter->second->iVTableID > 0)
		{
			CheckRemoveVTableUser(iter->second);
		}
		AddToFreeWaitUsers(iter->second);
		m_mapAllWaitTableUser.erase(iter);
	}
	else
	{
		if (pMsg->iUserID >= g_iMinVirtualID && pMsg->iUserID <= g_iMaxVirtualID)
		{
			//虚拟ai回收
			ReleaseVirtualAI(pMsg->iUserID);
		}
		//_log(_ERROR, "ATH", "HandleUserLeaveMsg user[%d] no find user", pMsg->iUserID);
	}
}

void AssignTableHandler::HandleFreeVTableMsg(char *msgData, int iSocketIndex)
{
	NewCenterFreeVTableMsgDef* pMsg = (NewCenterFreeVTableMsgDef*)msgData;
	map<int, VirtualTableDef*>::iterator iter = m_mapAllVTables.find(pMsg->iVTableID);
	if (iter != m_mapAllVTables.end())
	{
		//若桌上还有用户，模拟用户离开,避免桌上还有未释放的ai
		int iLeftUser[10] = { 0 };
		int iLeftCnt = 0;
		VirtualTableDef* pVTable = iter->second;
		for (int i = 0;i<10;i++)
		{
			if (pVTable->iUserID[i] > 0)
			{
				iLeftUser[iLeftCnt] = pVTable->iUserID[i];
				iLeftCnt++;
			}
		}
		if (iLeftCnt > 0)
		{
			for (int i = 0; i < iLeftCnt; i++)
			{
				RemoveVTableUser(pVTable, iLeftUser[i], true);
				if (pVTable->iVTableID < 0)
				{
					break;
				}
			}
		}
		else
		{
			RemoveVTableUser(pVTable, 0, true);
		}
		_log(_DEBUG, "ATH", "HandleFreeVTableMsg vt[%d],iLeftCnt[%d:%d,%d,%d,%d,%d]", pMsg->iVTableID, iLeftCnt, iLeftUser[0], iLeftUser[1], iLeftUser[2], iLeftUser[3], iLeftUser[4]);
	}
	else
	{
		_log(_DEBUG, "ATH", "HandleFreeVTableMsg vt[%d] not find", pMsg->iVTableID);
	}	
}

WaitTableUserInfoDef* AssignTableHandler::GetFreeWaitUsers(int iUserID)
{
	WaitTableUserInfoDef* pWaitUsers = NULL;
	if (m_vcFreeWaitUsers.empty())
	{
		pWaitUsers = new WaitTableUserInfoDef();
		//_log(_DEBUG, "ATH", "GetFreeWaitUsers memory_user all[%d] free[%d] cur[%d]", m_mapAllWaitTableUser.size(), m_vcFreeWaitUsers.size(), iUserID);
	}
	else
	{
		pWaitUsers = m_vcFreeWaitUsers[0];
		m_vcFreeWaitUsers.erase(m_vcFreeWaitUsers.begin());
	}
	memset(pWaitUsers,0,sizeof(WaitTableUserInfoDef));
	return pWaitUsers;
}

void AssignTableHandler::AddToFreeWaitUsers(WaitTableUserInfoDef* pWaitUser)
{
	m_vcFreeWaitUsers.push_back(pWaitUser);
	//_log(_DEBUG, "ATH", "AddToFreeWaitUsers memory_user all[%d] free[%d] cur[%d]", m_mapAllWaitTableUser.size(), m_vcFreeWaitUsers.size(), pWaitUser->iUserID);
}

WaitTableUserInfoDef* AssignTableHandler::FindWaitUser(int iUserID)
{
	map<int, WaitTableUserInfoDef*>::iterator iterUser = m_mapAllWaitTableUser.find(iUserID);
	if (iterUser != m_mapAllWaitTableUser.end())
	{
		return iterUser->second;
	}
	return NULL;
}

bool SortAssignFindUser(AssignFindUserDef assign1, AssignFindUserDef assign2)
{
	if (assign1.iUserNum > assign2.iUserNum)
	{
		return true;
	}
	if (assign1.pUser[0]->iFirstATHTime < assign2.pUser[0]->iFirstATHTime)
	{
		return true;
	}
	return false;
}

bool SortAssignFindTable(CheckAssignTableDef assign1, CheckAssignTableDef assign2)
{
	if (assign1.iTableAssignUser > assign2.iTableAssignUser)
	{
		return true;
	}
	return false;
}

void AssignTableHandler::FindPointedVTableCanAssignUser(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int &iFindUserIndex, VirtualTableDef* pVTable, vector<AssignFindUserDef>&vcTempAssign, int iMaxRoomGap /*= 2*/)
{
	//_log(_ALL, "AssignHandler", "findPVT_0,reqUser[%s],jVT[%d]", GetFindUserLogInfo(pFindUser, iFindUserIndex).c_str(), pVTable->iVTableID);
	bool bCanSit = false;
	//判断该桌上可以和找桌子的这批玩家一起入座的真实玩家
	for (int i = 0; i < 10; i++)
	{
		if (pVTable->iUserID[i] > g_iMaxVirtualID)
		{
			if (pVTable->iVTableID < 0)
			{
				break;
			}
			WaitTableUserInfoDef* pVTableUser = FindWaitUser(pVTable->iUserID[i]);
			if (pVTableUser == NULL)
			{
				_log(_DEBUG, "AssignHandler", "findPVT VT[%d], [%d]:user[%d] not in server", pVTable->iVTableID, i, pVTable->iUserID[i]);
				RemoveVTableUser(pVTable, pVTable->iUserID[i], false);
				continue;
			}
			//1.判断下是否可以和找桌子的玩家一起入座
			for (int j = 0; j < iFindUserIndex; j++)
			{
				bCanSit = true;
				int iRoomGap = abs(pFindUser[j]->iIfFreeRoom - pVTableUser->iIfFreeRoom);
				if (iRoomGap > iMaxRoomGap)
				{
					bCanSit = false;
				}
				if (bCanSit)
				{
					bCanSit = CheckIfUsersCanSitTogether(pFindUser[j], pVTableUser);
				}
				if (!bCanSit)
				{
					_log(_ALL, "AssignHandler", "findPVT,VT[%d],fUser[%d:%d],vUser[%d:%d],bCanSit[%d],roomGap[%d,%d]", pVTable->iVTableID, pFindUser[j]->iUserID, pFindUser[j]->iIfFreeRoom, pVTableUser->iUserID, pVTableUser->iIfFreeRoom, bCanSit, iRoomGap, iMaxRoomGap);
				}
			}
			if (bCanSit)
			{
				AssignFindUserDef stTempAssignUser;
				memset(&stTempAssignUser, 0, sizeof(AssignFindUserDef));
				stTempAssignUser.pUser[0] = pVTableUser;
				stTempAssignUser.iUserNum = 1;
				vcTempAssign.push_back(stTempAssignUser);
			}
		}
	}
	//_log(_DEBUG, "AssignHandler", "findPVT_1,reqUser[%s],jVT[%d],Ass[%s]", GetFindUserLogInfo(pFindUser,iFindUserIndex).c_str(), pVTable->iVTableID, GetAssignUsersLogInfo(vcTempAssign).c_str());
	_log(_ALL, "AssignHandler", "findPVT_2,reqUser[%s],jVT[%d],Ass[%s]", GetFindUserLogInfo(pFindUser, iFindUserIndex).c_str(), pVTable->iVTableID, GetAssignUsersLogInfo(vcTempAssign).c_str());
}
bool AssignTableHandler::CheckIfCanSitInOtherVTable(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int &iFindUserIndex, VirtualTableDef* pVTable, vector<AssignFindUserDef>&vcTempAssign, int iMaxRoomGap)
{
	int iMinNeedPlayer = pWaituser->iMinPlayer;
	int iMaxNeedPlayer = pWaituser->iMaxPlayer;
	int iMark9Num = 0;
	for (int i = 0; i < iFindUserIndex; i++)
	{
		if (pFindUser[i]->iMarkLv == 9)
		{
			iMark9Num++;
		}
		if (iMinNeedPlayer == 0 && pFindUser[i]->iMinPlayer > 0)
		{
			iMinNeedPlayer = pFindUser[i]->iMinPlayer;
		}
		if (iMaxNeedPlayer == 0 && pFindUser[i]->iMaxPlayer > 0)
		{
			iMaxNeedPlayer = pFindUser[i]->iMaxPlayer;
		}
	}
	FindPointedVTableCanAssignUser(pWaituser, pFindUser, iFindUserIndex,pVTable,vcTempAssign,iMaxRoomGap);
	if (vcTempAssign.empty())
	{
		//_log(_DEBUG, "ATH", "CheckIfCanSitInOtherVTable user[%d],otherVT[%d],findSort = 0", pWaituser->iUserID, pVTable->iVTableID);
		return false;
	}
	/*for (int i = 0; i < vcTempAssign.size(); i++)
	{
		_log(_DEBUG, "ATH", "CheckIfCanSitInOtherVTable vcTempAssign[%d]:user[%d],group[%d],[%d,%d,%d,%d]",i, vcTempAssign[i].iUserNum, vcTempAssign[i].iGroupID,
			vcTempAssign[i].pUser[0]!=NULL? vcTempAssign[i].pUser[0]->iUserID:0, vcTempAssign[i].pUser[1] != NULL ? vcTempAssign[i].pUser[1]->iUserID : 0,
			vcTempAssign[i].pUser[2] != NULL ? vcTempAssign[i].pUser[2]->iUserID : 0, vcTempAssign[i].pUser[3] != NULL ? vcTempAssign[i].pUser[3]->iUserID : 0);
	}*/
	int iAllCanSitNum = 0;
	int iOtherMinNeed = 0;
	int iOtherMaxNeed = 0;
	int iOtherMark9Num = 0;
	for (int i = 0; i < vcTempAssign.size();)
	{
		iAllCanSitNum += vcTempAssign[i].iUserNum;
		for (int j = 0; j < vcTempAssign[i].iUserNum; j++)
		{
			if (vcTempAssign[i].pUser[j]->iMarkLv == 9)
			{
				iOtherMark9Num++;
			}
			if (iOtherMinNeed == 0 && vcTempAssign[i].pUser[j]->iMinPlayer > 0)
			{
				iOtherMinNeed = vcTempAssign[i].pUser[j]->iMinPlayer;
			}
			if (iOtherMaxNeed == 0 && vcTempAssign[i].pUser[j]->iMaxPlayer > 0)
			{
				iOtherMaxNeed = vcTempAssign[i].pUser[j]->iMaxPlayer;
			}
		}
		i++;
	}
	_log(_DEBUG, "ATH", "CheckIfCanSitInOtherVTable user[%d],otherVT[%d],mark9[%d:%d][%d],needPlay[%d-%d,o:%d-%d],find[%d,%d],vSize[%d]", pWaituser->iUserID, pVTable->iVTableID,
		iMark9Num, iOtherMark9Num, m_iMaxMark9Num, iMinNeedPlayer,iMaxNeedPlayer, iOtherMinNeed,iOtherMaxNeed, iFindUserIndex, iAllCanSitNum, vcTempAssign.size());
	
	if (iMaxNeedPlayer > 0 && 
		((iFindUserIndex + iAllCanSitNum < iMinNeedPlayer)
		||(iFindUserIndex + iAllCanSitNum < iOtherMinNeed)))//加在一起都不够人数
	{
		return false;
	}
	sort(vcTempAssign.begin(), vcTempAssign.end(), SortAssignFindUser);
	if (iMinNeedPlayer == 0)//请求配桌的是快速开始的用户(这里不应该出现2个快速开始的用户)
	{	
		if ((m_iMaxMark9Num < 0 || (m_iMaxMark9Num >= 0 && (iMark9Num + iOtherMark9Num) <= m_iMaxMark9Num))
			&& (iFindUserIndex+ iAllCanSitNum >= iOtherMinNeed)
			&& (iFindUserIndex + iAllCanSitNum <= iOtherMaxNeed))//本批人数符合另外一桌可一起入座的玩家的人数要求
		{
			for (int i = 0; i < vcTempAssign.size(); i++)
			{
				for (int j = 0; j < vcTempAssign[i].iUserNum; j++)
				{
					pFindUser[iFindUserIndex] = vcTempAssign[i].pUser[j];
					iFindUserIndex++;
				}
			}			
			return true;
		}
		if (iOtherMinNeed == 0)//说明其他桌上也是一个快速开始的用户，可以直接配在一起了（只要2个用户坐在一起就必须是有玩法的）
		{
			if (iAllCanSitNum != 1)
			{
				_log(_ERROR, "ATH", "CheckIfCanSitInOtherVTable user[%d],otherVT[%d],iOtherMinNeed[%d],iAllCanSitNum[%d],otherUser[%d,%d,%d,%d,%]", pWaituser->iUserID, pVTable->iVTableID, iOtherMinNeed, iAllCanSitNum,
					pVTable->iUserID[0], pVTable->iUserID[1], pVTable->iUserID[2], pVTable->iUserID[3], pVTable->iUserID[4]);
				return false;
			}
			pFindUser[iFindUserIndex] = vcTempAssign[0].pUser[0];
			iFindUserIndex++;
			return true;
		}		
		return false;
	}
	if ((m_iMaxMark9Num < 0 || (m_iMaxMark9Num >= 0 && (iMark9Num+ iOtherMark9Num) <= m_iMaxMark9Num))
		&& (iFindUserIndex + iAllCanSitNum == iMaxNeedPlayer)) //正好凑齐
	{
		for (int i = 0; i < vcTempAssign.size(); i++)
		{
			for (int j = 0; j < vcTempAssign[i].iUserNum; j++)
			{
				pFindUser[iFindUserIndex] = vcTempAssign[i].pUser[j];
				iFindUserIndex++;
			}
		}
		return true;
	}
	//按每次可分配人数多少逐个尝试，不考虑所有组合方式
	int iOldFindIndex = iFindUserIndex;
	int iOldMark9Num = iMark9Num;
	for (int i = 0; i < vcTempAssign.size(); i++)
	{
		iOtherMark9Num = 0;
		for (int j = 0; j < vcTempAssign[i].iUserNum; j++)
		{
			if (vcTempAssign[i].pUser[j]->iMarkLv == 9)
			{
				iOtherMark9Num++;
			}
		}
		/*_log(_DEBUG, "ATH", "CheckIfCanSitInOtherVTable user[%d],otherVT[%d],mark9[%d:%d][%d],iOldFindIndex[%d],i[%d]user[%d],iNeedPlayer[%d]", pWaituser->iUserID, pVTable->iVTableID,
			iMark9Num, iOtherMark9Num, m_iMaxMark9Num, iOldFindIndex,i, vcTempAssign[i].iUserNum, iNeedPlayer);*/
		if ((m_iMaxMark9Num < 0 || (m_iMaxMark9Num >= 0 && (iOldMark9Num + iOtherMark9Num) <= m_iMaxMark9Num)
			&& iOldFindIndex + vcTempAssign[i].iUserNum <= iMaxNeedPlayer))
		{
			for (int j = 0; j < vcTempAssign[i].iUserNum; j++)
			{
				pFindUser[iOldFindIndex] = vcTempAssign[i].pUser[j];
				iOldFindIndex++;				
			}
			iOldMark9Num += iOtherMark9Num;
		}
		_log(_ALL, "ATH", "CheckIfCanSitInOtherVTable iOldFindIndex[%d],i[%d]user[%d],iNeedPlayer[%d]", iOldFindIndex, i, vcTempAssign[i].iUserNum, iMaxNeedPlayer);
		if (iOldFindIndex == iMaxNeedPlayer)
		{
			iFindUserIndex = iOldFindIndex;
			return true;
		}
		if (iOldFindIndex > iMaxNeedPlayer)//已经超过了，后面也不用判断了
		{
			break;
		}
	}
	for (int i = iFindUserIndex; i < iOldFindIndex; i++)//不能配桌的，中途找到的要清掉
	{
		pFindUser[i] = NULL;
	}
	//_log(_DEBUG, "ATH", "CheckIfCanSitInOtherVTable user[%d],otherVT[%d] can not together", pWaituser->iUserID, pVTable->iVTableID);
	return false;
}

bool AssignTableHandler::FindAllCanAssignUser(WaitTableUserInfoDef* pWaituser,WaitTableUserInfoDef* pFindUser[], int &iFindUserIndex, vector<CheckAssignTableDef>&vcAssignTable, int iMaxRoomGap)
{
	map<int, VirtualTableDef*>::iterator iterVTable = m_mapAllVTables.begin();
	vector<AssignFindUserDef>vcCanAssign;//不够开局，但是可以入座的桌子
	VirtualTableDef* pJudgeTable = NULL;
	int iNeedUser = pWaituser->iMinPlayer - iFindUserIndex;
	//_log(_DEBUG, "ATH", "FindAllCanAssignUser user[%d],iMaxRoomGap[%d],allTable[%d],iNeedUser=[%d:%d-%d]", pWaituser->iUserID, iMaxRoomGap, m_mapAllVTables.size(), iNeedUser, pWaituser->iMinPlayer,iFindUserIndex);
	while (iterVTable != m_mapAllVTables.end())
	{
		if (iterVTable->second->iVTableID == pWaituser->iVTableID || iterVTable->second->iPlayType == -1)
		{
			//_log(_DEBUG, "ATH", "FindAllCanAssignUser user[%d],vt[%d],jVT[%d],playType[%d]", pWaituser->iUserID, pWaituser->iVTableID, iterVTable->second->iVTableID, iterVTable->second->iPlayType);
			iterVTable++;
			continue;
		}
		WaitTableUserInfoDef* pReqFindUser[10];//用临时值进行判断
		for (int i = 0; i < 10; i++)
		{
			pReqFindUser[i] = pFindUser[i];
		}
		int iReqFindUserIndex = iFindUserIndex;
		vector<AssignFindUserDef>vcTempAssign;//被判断桌上所有可以和这批找桌子玩家同桌的玩家
		FindPointedVTableCanAssignUser(pWaituser, pReqFindUser, iReqFindUserIndex, iterVTable->second, vcTempAssign, iMaxRoomGap);
		if (iterVTable->second->iVTableID > 0  && !vcTempAssign.empty())
		{			
			CheckAssignTableDef stCheckAssignTable;
			stCheckAssignTable.pVTable = iterVTable->second;
			stCheckAssignTable.iTableAllUser = GetUserVTableAssignUser(iterVTable->second);
			stCheckAssignTable.iTableAssignUser = 0;
			for (int i = 0; i < vcTempAssign.size(); i++)
			{
				stCheckAssignTable.vcAssignUser.push_back(vcTempAssign[i]);				
				stCheckAssignTable.iTableAssignUser += vcTempAssign[i].iUserNum;				
			}
			//是否是最优的桌子，是的话，后续都不用判断了（最优的桌子=桌子上所有玩家数恰好是缺的人数）
			if (stCheckAssignTable.iTableAssignUser == stCheckAssignTable.iTableAllUser)
			{
				bool bRightUser = false;
				if (pWaituser->iPlayType == 0)//桌上正好缺一个人，这个快速开始的用户可以直接加进来
				{
					if (stCheckAssignTable.pVTable->iPlayType > 0)
					{
						int iJTableMinNeed = GetPlayNum(stCheckAssignTable.pVTable->iPlayType);
						if ((stCheckAssignTable.iTableAssignUser + 1) == iJTableMinNeed)
						{
							bRightUser = true;
						}
					}
					//_log(_DEBUG, "ATH", "FindAll user[%d],iPlayType[%d:%d],bRightUser[%d],tableA[%d],tableN[%d]", pWaituser->iUserID, pWaituser->iPlayType, stCheckAssignTable.pVTable->iPlayType, bRightUser, stCheckAssignTable.iTableAssignUser, GetPlayNum(stCheckAssignTable.pVTable->iPlayType));
				}
				else if(stCheckAssignTable.iTableAssignUser == iNeedUser)
				{
					bRightUser = true;
				}
				if (bRightUser)
				{
					//直接把自己这波人放到被判断的桌上
					for (int i = 0; i < iFindUserIndex; i++)
					{
						if (pFindUser[i]->iVTableID > 0)
						{
							CheckRemoveVTableUser(pFindUser[i]);
						}
						AddUserToVTable(pFindUser[i]->iUserID, pFindUser[i], iterVTable->second);
					}
					for (int i = 0; i < vcTempAssign.size(); i++)
					{
						for (int j = 0; j < vcTempAssign[i].iUserNum; j++)
						{
							pFindUser[iFindUserIndex] = vcTempAssign[i].pUser[j];
							iFindUserIndex++;
						}
					}
					_log(_DEBUG, "ATH", "FindAll user[%d],jTable[%d],findAll", pWaituser->iUserID, iterVTable->second->iVTableID);
					return true;
				}				
			}
			vcAssignTable.push_back(stCheckAssignTable);
		}
		iterVTable++;
	}
	if (vcAssignTable.empty())
	{
		return false;
	}
	//判断本轮所有可配桌玩家凑起来，是否可以让请求配桌玩家开局(这里不找最优解了，能凑开局就可以)
	int iMinNeedPlayer = pWaituser->iMinPlayer;
	if (iMinNeedPlayer == 0)//快速开始的用户，人数用其他玩家的人数作判断
	{
		for (int i = 0; i < vcAssignTable.size(); i++)
		{
			int iTablePlayer = GetPlayNum(vcAssignTable[i].pVTable->iPlayType);
			if (iTablePlayer > 0)
			{
				iMinNeedPlayer = iTablePlayer;
				break;
			}
		}
	}
	if (iMinNeedPlayer == 0)
	{
		_log(_DEBUG, "ATH", "FindAll user[%d],iMinNeedPlayer=0", pWaituser->iUserID);
		return false;
	}
	int iIniFindUserIndex = iFindUserIndex;
	for (int i = 0; i < vcAssignTable.size(); i++)
	{
		for (int j = 0; j < vcAssignTable[i].vcAssignUser.size(); j++)
		{
			int iLastFindIndex = iFindUserIndex;
			bool bCanSit = true;
			for (int m = 0; m < vcAssignTable[i].vcAssignUser[j].iUserNum; m++)
			{
				WaitTableUserInfo* pJudgeUser = vcAssignTable[i].vcAssignUser[j].pUser[m];		
				//这里注意，还需要跟其他桌已找到的玩家做次比较，有可能有同桌限制
				for (int n = iIniFindUserIndex; n < iLastFindIndex; n++)
				{
					bCanSit = CheckIfUsersCanSitTogether(pFindUser[n], pJudgeUser);
					if (!bCanSit)
					{
						break;
					}
				}
				if (!bCanSit)
				{
					break;
				}
				pFindUser[iFindUserIndex] = pJudgeUser;
				iFindUserIndex++;				
			}
			//_log(_DEBUG, "ATH", "FindAll_2 user[%d],jTable[%d],find[%d,%d],bCanSit[%d]", pWaituser->iUserID, vcAssignTable[i].pVTable!=NULL? vcAssignTable[i].pVTable->iVTableID:0, iFindUserIndex, iMinNeedPlayer, bCanSit);
			if (iFindUserIndex >  iMinNeedPlayer || !bCanSit)//找到的这组加进来就超过了人数要求或者和已加进来的有同桌限制，新加进来的要清掉
			{
				for (int m = iLastFindIndex; m < iFindUserIndex; m++)
				{
					pFindUser[m] = NULL;
				}
				iFindUserIndex = iLastFindIndex;
				//这组人也从已找到的桌子内去掉，不然后面提前把2桌人放在一起的时候，这组人还是会被加进来，就导致人数过多或者不能坐一起的还是坐一起了
				vcAssignTable[i].vcAssignUser[j].iUserNum = -vcAssignTable[i].vcAssignUser[j].iUserNum;//先置为负值，再统一移除
			}
			if (iFindUserIndex == iMinNeedPlayer)
			{
				//走到这里能匹配成功，说明找桌子的这波玩家不适合加到其他桌子上，所有找到的人，都必须加到找桌子玩家所在桌
				VirtualTableDef* pWaitVTable = NULL;
				if (pWaituser->iVTableID > 0)
				{
					pWaitVTable = FindVTable(pWaituser->iVTableID);
				}
				if (pWaitVTable == NULL)
				{
					pWaitVTable = GetFreeVTable();
					m_mapAllVTables[pWaitVTable->iVTableID] = pWaitVTable;
					//_log(_DEBUG, "AssignHandler", "FindAllCanAssignUser user[%d],userVtable[%d],creatTable[%d]", iUserID, pWaitUserInfo->iVTableID, pWaitVTable->iVTableID);
				}
				for (int m = 0; m < iFindUserIndex; m++)//将请求配桌的玩家加入到新找到的这桌玩家上
				{
					if (pFindUser[m]->iVTableID != pWaitVTable->iVTableID)
					{
						if (pFindUser[m]->iVTableID > 0)
						{
							CheckRemoveVTableUser(pFindUser[m]);
						}
						AddUserToVTable(pFindUser[m]->iUserID, pFindUser[m], pWaitVTable);
					}
				}
				_log(_DEBUG, "ATH", "FindAll_2 user[%d],findAll", pWaituser->iUserID);
				return true;
			}
		}
		for (int j = 0; j < vcAssignTable[i].vcAssignUser.size(); )
		{
			if (vcAssignTable[i].vcAssignUser[j].iUserNum < 0)
			{
				vcAssignTable[i].vcAssignUser.erase(vcAssignTable[i].vcAssignUser.begin() + j);
				continue;
			}
			j++;
		}
	}
	//至此，说明凑不齐桌，所有中途判断加进来的都清掉
	for (int i = iIniFindUserIndex; i < iFindUserIndex; i++)
	{
		pFindUser[i] = NULL;
	}
	iFindUserIndex = iIniFindUserIndex;
	return false;
}

void AssignTableHandler::TogetherUsersWithAssignTable(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int &iFindUserIndex, vector<CheckAssignTableDef>&vcAssignTable)
{
	//_log(_DEBUG, "AssignHandler", "TogetherUsers[%s][%s]", GetFindUserLogInfo(pFindUser, iFindUserIndex).c_str(), vcAssignTable.empty()?"empty":GetAssignTableLogInfo(&vcAssignTable[0]).c_str());
	if (vcAssignTable.empty())
	{
		return;
	}
	sort(vcAssignTable.begin(), vcAssignTable.end(), SortAssignFindTable);
	vector<WaitTableUserInfo*>vcAllFindUser;
	for (int i = 0; i < iFindUserIndex; i++)
	{
		vcAllFindUser.push_back(pFindUser[i]);
	}	
	for (int i = 0; i < vcAssignTable.size(); i++)
	{
		for (int j = 0; j < vcAssignTable[i].vcAssignUser.size(); j++)
		{
			bool bCanSit = true;
			for (int m = 0; m < vcAssignTable[i].vcAssignUser[j].iUserNum; m++)
			{
				WaitTableUserInfo* pJudgeUser = vcAssignTable[i].vcAssignUser[j].pUser[m];
				//这里注意，还需要跟其他桌已找到的玩家做次比较，有可能有同桌限制
				for (int n = 0; n < vcAllFindUser.size(); n++)
				{
					bCanSit = CheckIfUsersCanSitTogether(vcAllFindUser[n], pJudgeUser);
					if (!bCanSit)
					{
						break;
					}
				}
				if (!bCanSit)
				{
					break;
				}
			}
			if (bCanSit)
			{
				for (int m = 0; m < vcAssignTable[i].vcAssignUser[j].iUserNum; m++)
				{
					vcAllFindUser.push_back(vcAssignTable[i].vcAssignUser[j].pUser[m]);//所有先加进来，再判断人数				
				}
			}
		}
	}
	if (pWaituser->iPlayType > 0 && vcAllFindUser.size() >= pWaituser->iMinPlayer)//不应该出现这种异常，在前置判断中一定是所有找到的桌子人凑齐都不够开局
	{
		_log(_ERROR, "AssignHandler", "TogetherUsers user[%d],check room user num err[%d:%d]", pWaituser->iUserID, vcAllFindUser.size(), pWaituser->iMinPlayer);
		return;
	}
	//继续判断下是否可以拉到一起，确保拉过后的人数比之前的要多
	//(典型判断场景，ab一桌，c一桌（假设c不能和a同桌），c请求的时候不能把a拉走，再来一个d请求，d可以拉ab一桌，同时不能拉c)
	int iMaxLoop = vcAllFindUser.size() - iFindUserIndex;
	_log(_ALL, "ATH", "TogetherUsers find %s,iFindUserIndex=%d,iMaxLoop=%d", GetFindUserLogInfo(vcAllFindUser).c_str(), iFindUserIndex, iMaxLoop);
	while (iMaxLoop > 0)
	{
		bool bJudgeOK = true;
		for (int i = iFindUserIndex; i < vcAllFindUser.size(); i++)
		{
			VirtualTableDef *pFindVTable = FindVTable(vcAllFindUser[i]->iVTableID);
			if (pFindVTable != NULL)
			{
				int iTableAllUser = GetUserVTableAssignUser(pFindVTable);
				if (iTableAllUser >= vcAllFindUser.size())
				{
					bJudgeOK = false;
				}
			}
			if (!bJudgeOK)//只要有一个不合适的就要从已找到的去掉，并重新判断
			{
				vcAllFindUser.erase(vcAllFindUser.begin() + i);
				break;
			}
		}
		if (bJudgeOK)
		{
			break;
		}
		iMaxLoop--;
	}
	if (vcAllFindUser.empty() || vcAllFindUser.size() <= iFindUserIndex)
	{
		_log(_DEBUG, "AssignHandler", "TogetherUsers user[%d],check room no right user[%d:%d]", pWaituser->iUserID, vcAllFindUser.size(), iFindUserIndex);
		return;
	}
	VirtualTableDef *pJudgeVTable = NULL;
	int iLastJudgeTableID = 0;
	bool bAllSameTable = true;
	for (int i = iFindUserIndex; i < vcAllFindUser.size(); i++)
	{
		if (iLastJudgeTableID == 0)
		{
			iLastJudgeTableID = vcAllFindUser[i]->iVTableID;
		}
		if (iLastJudgeTableID != 0 && vcAllFindUser[i]->iVTableID != iLastJudgeTableID)
		{
			bAllSameTable = false;
			break;
		}
	}
	bool bAddToSelfTable = false;
	VirtualTableDef* pSelfTable = NULL;
	if (pWaituser->iVTableID > 0)
	{
		pSelfTable = FindVTable(pWaituser->iVTableID);
	}
	if (bAllSameTable == false)//拉过来的不止一桌上的玩家，肯定要要拉到自己这波所在桌
	{
		bAddToSelfTable = true;
	}
	else//被判断的只有一桌，人少的加到人多的
	{
		//被判断桌若有不可一起配桌的/或所有人数加起来小于自己所在桌，加到自己这波所在桌
		int iJTableID = vcAllFindUser[iFindUserIndex]->iVTableID;
		pJudgeVTable = FindVTable(iJTableID);
		for (int i = 0; i < vcAssignTable.size(); i++)
		{
			if (vcAssignTable[i].pVTable == pJudgeVTable)
			{
				if (vcAssignTable[i].iTableAllUser > vcAssignTable[i].iTableAssignUser)
				{
					bAddToSelfTable = true;
				}
				else if(vcAssignTable[i].iTableAllUser < iFindUserIndex)
				{
					bAddToSelfTable = true;
				}
				_log(_DEBUG, "_ALL", "TogetherUsers find_2 %s,JTable[%d],user[%d:%d]", GetFindUserLogInfo(vcAllFindUser).c_str(), iJTableID, vcAssignTable[i].iTableAllUser, vcAssignTable[i].iTableAssignUser);
				break;
			}
		}		
	}
	if (bAddToSelfTable && pSelfTable == NULL)
	{
		pSelfTable = GetFreeVTable();
		m_mapAllVTables[pSelfTable->iVTableID] = pSelfTable;
	}
	if (pJudgeVTable == NULL && !bAddToSelfTable)
	{
		_log(_ERROR, "ATH", "TogetherUsers find_3 %s,pJudgeVTable == NULL && !bAddToSelfTable", GetFindUserLogInfo(vcAllFindUser).c_str());
		bAddToSelfTable = true;
	}
	if (pJudgeVTable == NULL && pSelfTable == NULL)
	{
		return;
	}	
	VirtualTableDef* pAddToTable = bAddToSelfTable?pSelfTable: pJudgeVTable;
	_log(_ALL, "ATH", "TogetherUsers find_3 %s,addToTable[%d],sameT[%d],addToS[%d]", GetFindUserLogInfo(vcAllFindUser).c_str(), pAddToTable->iVTableID, bAllSameTable, bAddToSelfTable);
	for (int i = 0; i < 10; i++)
	{
		pFindUser[i] = NULL;
	}
	iFindUserIndex = 0;

	//找到真人了，先把桌子上的ai都踢掉 add by cent 20230810
	for (int i = 0; i<10; i++)
	{
		if (pAddToTable->iUserID[i] >= g_iMinVirtualID && pAddToTable->iUserID[i] <= g_iMaxVirtualID)
		{
			RemoveVTableUser(pAddToTable, pAddToTable->iUserID[i], false);
		}
	}

	for (int i = 0; i < vcAllFindUser.size(); i++)
	{
		pFindUser[iFindUserIndex] = vcAllFindUser[i];
		iFindUserIndex++;
		if (vcAllFindUser[i]->iVTableID > 0 && vcAllFindUser[i]->iVTableID != pAddToTable->iVTableID)
		{
			CheckRemoveVTableUser(vcAllFindUser[i]);			
		}
		if (vcAllFindUser[i]->iVTableID == 0)
		{
			AddUserToVTable(vcAllFindUser[i]->iUserID, vcAllFindUser[i], pAddToTable);
		}
	}
}

string AssignTableHandler::GetAssignUserLogInfo(AssignFindUserDef* pAssignUser)
{
	char cLogInfo[512];
	sprintf(cLogInfo,"uCnt:%d", pAssignUser->iUserNum);
	char cTemp[32];
	for (int i = 0; i < pAssignUser->iUserNum; i++)
	{
		sprintf(cTemp, ",%d", pAssignUser->pUser[i]->iUserID);
		strcat(cLogInfo, cTemp);
	}
	return cLogInfo;
}

string AssignTableHandler::GetAssignUsersLogInfo(vector<AssignFindUserDef>&vcTempAssign)
{
	char cLogInfo[512];
	memset(cLogInfo,0,sizeof(cLogInfo));
	for (int i = 0; i < vcTempAssign.size(); i++)
	{
		if (i > 1)
		{
			strcat(cLogInfo, ",");
		}
		string strOne = GetAssignUserLogInfo(&vcTempAssign[i]);
		strcat(cLogInfo, strOne.c_str());
	}
	return cLogInfo;
}

string AssignTableHandler::GetAssignTableLogInfo(CheckAssignTableDef* pAssignTable)
{
	char cLogInfo[512];
	sprintf(cLogInfo, "asTable:%d,uCnt[%d,%d]", pAssignTable->pVTable!= NULL? pAssignTable->pVTable->iVTableID:0, pAssignTable->iTableAllUser, pAssignTable->iTableAssignUser);
	char cTemp[32];
	for (int i = 0; i < pAssignTable->vcAssignUser.size(); i++)
	{
		strcat(cLogInfo, "(");
		for (int j = 0; j < pAssignTable->vcAssignUser[i].iUserNum; j++)
		{
			sprintf(cTemp, ",%d", pAssignTable->vcAssignUser[i].pUser[j]->iUserID);
			strcat(cLogInfo, cTemp);
		}
		strcat(cLogInfo, ")");
	}
	return cLogInfo;
}

string AssignTableHandler::GetFindUserLogInfo(vector<WaitTableUserInfoDef*>&vcFindUser)
{
	char cLogInfo[512];
	sprintf(cLogInfo, "uCnt:%d", vcFindUser.size());
	char cTemp[32];
	for (int i = 0; i < vcFindUser.size(); i++)
	{
		sprintf(cTemp, ",%d", vcFindUser[i]->iUserID);
		strcat(cLogInfo, cTemp);
	}
	return cLogInfo;
}

string AssignTableHandler::GetFindUserLogInfo(WaitTableUserInfoDef* pFindUser[], int iFindUserIndex)
{
	char cLogInfo[512];
	sprintf(cLogInfo, "uCnt:%d", iFindUserIndex);
	char cTemp[32];
	for (int i = 0; i < iFindUserIndex; i++)
	{
		sprintf(cTemp, ",%d", pFindUser[i] != NULL? pFindUser[i]->iUserID:0);
		strcat(cLogInfo, cTemp);
	}
	return cLogInfo;
}

void AssignTableHandler::SetWaitUserPlayType(WaitTableUserInfoDef* pWaituser, int iPlayType)
{
	pWaituser->iPlayType = iPlayType;
	pWaituser->iMinPlayer = pWaituser->iPlayType >> 16 & 0xff;
	pWaituser->iMaxPlayer = pWaituser->iPlayType >> 24;
	if (pWaituser->iMaxPlayer == 0)
	{
		pWaituser->iMaxPlayer = pWaituser->iMinPlayer;
	}
}

void AssignTableHandler::CallBackStartGame(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int iFindUserIndex, int iAINum, int iLastVAIIndex, int iLastVirtualAI[], int iSocketIndex)
{
	NCenterUserSitRes msgSitRes;
	memset(&msgSitRes, 0, sizeof(NCenterUserSitRes));
	msgSitRes.msgHeadInfo.iMsgType = NEW_CENTER_GET_USER_SIT_MSG;
	msgSitRes.msgHeadInfo.cVersion = MESSAGE_VERSION;

	VirtualTableDef* pVTable = GetFreeVTable();//先提前分配一个虚拟桌号给这桌用户下次可以继续坐在一起
	pVTable->iPlayType = -1;//标记是提前分配给这桌的下桌虚拟桌号，遍历的时候跳过这种桌子
	m_mapAllVTables[pVTable->iVTableID] = pVTable;

	msgSitRes.iNextVTableID = pVTable->iVTableID;
	map<int, int>mapTempInfo;//first=serverid，second=在该服的数量
	int iUserIndex = 0;
	int iPlayType = 0;
	int iRemoveUserCnt = 0;
	VirtualTableDef* pNowVTable = NULL;
	for (int i = 0; i < 10; i++)
	{
		if (pFindUser[i] != NULL && i < iFindUserIndex )
		{
			if (iPlayType == 0 && pFindUser[i]->iPlayType > 0)
			{
				iPlayType = pFindUser[i]->iPlayType;
			}
			if (iSocketIndex == -1)
			{
				OneGameServerDetailInfoDef* pServerInfo = m_pRoomServerLogic->GetServerInfo(pFindUser[i]->iInServerID);
				iSocketIndex = pServerInfo->iSocketIndex;
			}
			msgSitRes.iUsers[iUserIndex] = pFindUser[i]->iUserID;
			pFindUser[i]->iTableStatus = 1;
			msgSitRes.iUserServerID[iUserIndex] = pFindUser[i]->iInServerID;
			msgSitRes.iIfFreeRoom[iUserIndex] = pFindUser[i]->iIfFreeRoom;
			if (pFindUser[i]->iInServerID > 0)
			{
				mapTempInfo[pFindUser[i]->iInServerID]++;
			}
			if (pFindUser[i]->iVTableID > 0)//所有玩家都从自己所在虚拟桌移除
			{
				VirtualTableDef* pTempVTable = pNowVTable;
				bool bNoticeRD = false;
				bool bEraseTable = pWaituser == NULL ? false : true;
				if (pNowVTable == NULL)
				{
					pNowVTable = FindVTable(pFindUser[i]->iVTableID);
				}
				else if (pFindUser[i]->iVTableID != pNowVTable->iVTableID)//理论上到开局的时候，所有玩家都应该在同一个虚拟桌上
				{
					bNoticeRD = true;
					pTempVTable = FindVTable(pFindUser[i]->iVTableID);
					_log(_ERROR, "ATH", "CallBackStartGame user[%d]vt[%d]!=nowVt[%d]", pFindUser[i]->iUserID, pFindUser[i]->iVTableID, pNowVTable->iVTableID);
				}
				if (iRemoveUserCnt == iFindUserIndex - 1)
				{
					bNoticeRD = true;
				}
				RemoveVTableUser(pNowVTable, pFindUser[i]->iUserID, bEraseTable);
				iRemoveUserCnt++;
			}
			iUserIndex++;
		}
	}
	if (iAINum > 0)
	{
		for (int i = 0; i < iLastVAIIndex; i++)
		{
			msgSitRes.iUsers[iUserIndex] = iLastVirtualAI[i];
			iUserIndex++;
			VirtualAIInfoDef* pTempVirtualAI = (VirtualAIInfoDef *)(m_hashVirtual.Find((void *)(&iLastVirtualAI[i]), sizeof(int)));
			if (pTempVirtualAI)
			{
				pTempVirtualAI->iInVTableID = msgSitRes.iNextVTableID;
			}
		}
		_log(_DEBUG, "AssignHandler", "CallBackStartGame findAll user[%d],iAINum[%d],iLastVAIIndex[%d][%d,%d,%d,%d]", pWaituser->iUserID, iAINum, iLastVAIIndex, iLastVirtualAI[0], iLastVirtualAI[1], iLastVirtualAI[2], iLastVirtualAI[3]);
	}
	SetVirtualAITableUser(msgSitRes.iUsers);//将玩家添加到ai近期的玩家上
	if (iPlayType == 0)
	{
		iPlayType = GetPlayTypeByIdx(0);
	}
	msgSitRes.iTablePlayType = iPlayType;
	//发到人数最多的那个服务器且不会超出本服最多人数，其他服务器通知玩家换服
	map<int, int>::iterator iterTemp = mapTempInfo.begin();
	//先赋空值
	map<int, int>::iterator iterMax = mapTempInfo.end();
	int iFinalServer = 0;
	int iSameServer = 0;
	if (mapTempInfo.size() == 1)//都在一个服务器内,并且服务器桌子数量足够,肯定就是在这个玩家所在的服务器内了，直接发出去
	{
		iSameServer = iterTemp->first;
		OneGameServerDetailInfoDef* pServerInfo = m_pRoomServerLogic->GetServerInfo(iterTemp->first);
		if (pServerInfo != NULL)
		{
			int iMaxTable = pServerInfo->iMaxTab * 95 / 100;//不要超过95%吧
			int iNewTable = pServerInfo->iUsedTabNum + 1;
			if (iNewTable <= iMaxTable)
			{
				/*_log(_DEBUG, "AssignHandler", "CallBackStartGame, user[%d]nVT[%d],all[%d,%d,%d,%d,%d],find users all in same server sk[%d]", pWaituser!=NULL?pWaituser->iUserID:0, msgSitRes.iNextVTableID,
				pFindUser[0] != NULL ? pFindUser[0]->iUserID : 0, pFindUser[1] != NULL ? pFindUser[1]->iUserID : 0,
				pFindUser[2] != NULL ? pFindUser[2]->iUserID : 0, pFindUser[3] != NULL ? pFindUser[3]->iUserID : 0,
				pFindUser[4] != NULL ? pFindUser[4]->iUserID : 0, iSocketIndex);*/
				EnQueueSendMsg((char*)&msgSitRes, sizeof(NCenterUserSitRes), iSocketIndex);
				return;
			}
		}
	}
	while (iterTemp != mapTempInfo.end())
	{
		//_log(_DEBUG, "AssignHandler", "CallBackStartGame user[%d] server[%d],userNum[%d]", pWaituser!=NULL?pWaituser->iUserID:0, iterTemp->first, iterTemp->second);
		OneGameServerDetailInfoDef* pServerInfo = m_pRoomServerLogic->GetServerInfo(iterTemp->first);
		if (pServerInfo != NULL)
		{
			int iMaxPlayer = pServerInfo->iMaxTab * 90 / 100;//不要超过95%吧
			int iNewPlayer = pServerInfo->iUsedTabNum + 1;
			//_log(_DEBUG, "AssignHandler", "CallBackStartGame user[%d] server[%d]-[%d],iMaxPalyer[%d,%d],current[%d,%d+%d]", pWaituser!=NULL?pWaituser->iUserID:0, iterTemp->first, iterTemp->second, pServerInfo->iRoomMaxPlayer, iMaxPlayer, iNewPlayer, pServerInfo->iCurrentNum,iFindUserIndex);
			if (iNewPlayer > iMaxPlayer)
			{
				iterTemp++;
				continue;
			}
			if (iterMax == mapTempInfo.end() || iterTemp->second > iterMax->second)
			{
				iterMax = iterTemp;
				iFinalServer = iterTemp->first;
			}
			else
			{
				//_log(_DEBUG, "AssignHandler", "HandleGetUserSitMsg user[%d] server[%d],iMaxPalyer[%d,%d],current[%d]", pWaituser!=NULL?pWaituser->iUserID:0, iterTemp->first, iterTempServer->second.iRoomMaxPlayer, iMaxPlayer, iterTempServer->second.iCurrentNum);
			}
		}
		iterTemp++;
	}
	if(iFinalServer == 0)
	{
		OneGameServerDetailInfoDef* pServerInfo = m_pRoomServerLogic->GetMinimumServerInfo();
		iFinalServer = pServerInfo->iServerID;
	}

	if (iSameServer == iFinalServer)    //如果需要集体换桌，并且没找到其他服务器，还是直接坐原服务器
	{
		EnQueueSendMsg((char*)&msgSitRes, sizeof(NCenterUserSitRes), iSocketIndex);
		return;
	}

	OneGameServerDetailInfoDef* pMaxServerInfo = m_pRoomServerLogic->GetServerInfo(iFinalServer);
	_log(_DEBUG, "AssignHandler", "CallBackStartGame, nVT[%d],all[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],select server[%d]", msgSitRes.iNextVTableID,
		msgSitRes.iUsers[0], msgSitRes.iUsers[1], msgSitRes.iUsers[2], msgSitRes.iUsers[3], msgSitRes.iUsers[4], msgSitRes.iUsers[5], msgSitRes.iUsers[6],
		msgSitRes.iUsers[7], msgSitRes.iUsers[8], msgSitRes.iUsers[9], iFinalServer);
	EnQueueSendMsg((char*)&msgSitRes, sizeof(NCenterUserSitRes), pMaxServerInfo->iSocketIndex);
	iterTemp = mapTempInfo.begin();
	while (iterTemp != mapTempInfo.end())
	{
		if (iterTemp != iterMax)
		{
			NCenterUserChangeMsgDef msgChange;
			memset(&msgChange, 0, sizeof(NCenterUserChangeMsgDef));
			msgChange.msgHeadInfo.iMsgType = NEW_CENTER_USER_CHANGE_MSG;
			msgChange.iNewServerIP = pMaxServerInfo->iIP;
			msgChange.iNewServerPort = (ushort)pMaxServerInfo->sPort;
			msgChange.iNewServerSocket = pMaxServerInfo->iSocketIndex;
			msgChange.iNewInnerIP = pMaxServerInfo->iInnerIP;
			msgChange.iNewInnerPort = (ushort)pMaxServerInfo->sInnerPort;
			int iTempIndex = 0;
			for (int i = 0; i < iFindUserIndex; i++)
			{
				if (pFindUser[i] && pFindUser[i]->iUserID > g_iMaxVirtualID && pFindUser[i]->iInServerID == iterTemp->first)
				{
					msgChange.iUserID[iTempIndex] = pFindUser[i]->iUserID;
					iTempIndex++;
					m_stAssignStaInfo.iNeedChangeNum++;
				}
			}
			OneGameServerDetailInfoDef* pTempServer = m_pRoomServerLogic->GetServerInfo(iterTemp->first);
			if (pTempServer != NULL)
			{
				EnQueueSendMsg((char*)&msgChange, sizeof(NCenterUserChangeMsgDef), pTempServer->iSocketIndex);
			}
			else
			{
				_log(_ERROR, "ATH", "CallBackStartGame send change msg no find server[%d] info", iterTemp->first);
			}
		}
		iterTemp++;
	}
}

void AssignTableHandler::SendNCAssignStatInfo(int iTimeFlag)
{
	RDNCRecordStatInfoDef stNCStatInfo;
	memset(&stNCStatInfo,0,sizeof(RDNCRecordStatInfoDef));
	stNCStatInfo.msgHeadInfo.iMsgType = REDIS_RECORD_NCENTER_STAT_INFO;
	stNCStatInfo.msgHeadInfo.cVersion = MESSAGE_VERSION;
	stNCStatInfo.iGameID = stSysConfigBaseInfo.iGameID;
	m_stAssignStaInfo.iTmFlag = iTimeFlag;
	//统计下当时等待配桌的相关数据
	map<int, WaitTableUserInfoDef*>::iterator iter = m_mapAllWaitTableUser.begin();
	while (iter != m_mapAllWaitTableUser.end())
	{
		
		if (iter->second->iTableStatus >= 0 && iter->second->iTableStatus <= 2)
		{
			if (iter->second->iNowSection == 0)
			{
				m_stAssignStaInfo.iHighUserCnt++;
			}
			else if (iter->second->iNowSection == 1)
			{
				m_stAssignStaInfo.iMidUserCnt++;
			}
			else
			{
				m_stAssignStaInfo.iLowUserCnt++;
			}
		}
		if (iter->second->iTableStatus == 0)
		{
			m_stAssignStaInfo.iRoomWaitCnt[iter->second->iIfFreeRoom-4]++;
		}
		iter++;
	}
	//_log(_DEBUG, "ATH", "SendNCAssignStatInfo userCnt[%d,%d,%d],wait[%d,%d,%d,%d,%d]", m_stAssignStaInfo.iHighUserCnt, m_stAssignStaInfo.iMidUserCnt, m_stAssignStaInfo.iLowUserCnt,
	//m_stAssignStaInfo.iRoomWaitCnt[0], m_stAssignStaInfo.iRoomWaitCnt[1], m_stAssignStaInfo.iRoomWaitCnt[2],
	//m_stAssignStaInfo.iRoomWaitCnt[3], m_stAssignStaInfo.iRoomWaitCnt[4]);
	//_log(_DEBUG, "ATH", "SendNCAssignStatInfo iRightRoomTableCnt[%d],rateCnt[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", m_stAssignStaInfo.iRightRoomTableCnt, m_stAssignStaInfo.iUserRateCnt[0],
	//	m_stAssignStaInfo.iUserRateCnt[1], m_stAssignStaInfo.iUserRateCnt[2], m_stAssignStaInfo.iUserRateCnt[3], m_stAssignStaInfo.iUserRateCnt[4], m_stAssignStaInfo.iUserRateCnt[5],
	//	m_stAssignStaInfo.iUserRateCnt[6], m_stAssignStaInfo.iUserRateCnt[7], m_stAssignStaInfo.iUserRateCnt[8], m_stAssignStaInfo.iUserRateCnt[9], m_stAssignStaInfo.iUserRateCnt[10]);
	int iStLen = sizeof(NCAssignStaInfoDef);
	memcpy(&stNCStatInfo.stAssignStaInfo, &m_stAssignStaInfo,iStLen);
	//_log(_DEBUG, "ATH", "SendNCAssignStatInfo iTimeFlag[%d] end", stNCStatInfo.stAssignStaInfo.iTmFlag);

	if (m_pQueueForRedis != NULL)
	{
		m_pQueueForRedis->EnQueue(&stNCStatInfo, sizeof(RDNCRecordStatInfoDef));
	}
	memset(&m_stAssignStaInfo, 0, iStLen);
}

bool AssignTableHandler::CheckIfHisSameTable(WaitTableUserInfoDef* pWaituser1, WaitTableUserInfoDef* pWaituser2)
{
	int iTempIndex = pWaituser1->iLastPlayerIndex - 1;
	if (iTempIndex < 0)
	{
		iTempIndex = 9;
	}
	if (iTempIndex > 9)
	{
		iTempIndex = 0;
	}
	bool bHisTableUser = false;
	for (int m = 0; m < m_iForbidSameTableNum; m++)
	{		
		if (pWaituser1->iLatestPlayUser[iTempIndex] == pWaituser2->iUserID)
		{
			bHisTableUser = true;
			break;
		}
		iTempIndex--;
		if (iTempIndex < 0)
		{
			iTempIndex = 9;
		}
	}	
	//先看下对方是不是自己的近期玩家，再看下自己是不是对方的近期玩家，两个不等价
	if (!bHisTableUser)
	{
		iTempIndex = pWaituser2->iLastPlayerIndex - 1;
		if (iTempIndex < 0)
		{
			iTempIndex = 9;
		}
		for (int m = 0; m < m_iForbidSameTableNum; m++)
		{			
			if (pWaituser2->iLatestPlayUser[iTempIndex] == pWaituser1->iUserID)
			{
				bHisTableUser = true;
				break;
			}
			iTempIndex--;
			if (iTempIndex < 0)
			{
				iTempIndex = 9;
			}
		}		
	}
	/*_log(_ALL, "AssignHandler", "CheckSame user1[%d],user2[%d],bSame[%d]", pWaituser1->iUserID, pWaituser2->iUserID, bHisTableUser);
	_log(_ALL, "AssignHandler", "CheckSame user1[%d],index[%d],[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", pWaituser1->iUserID, pWaituser1->iLastPlayerIndex, pWaituser1->iLatestPlayUser[0], pWaituser1->iLatestPlayUser[1], pWaituser1->iLatestPlayUser[2], pWaituser1->iLatestPlayUser[3], pWaituser1->iLatestPlayUser[4],
		pWaituser1->iLatestPlayUser[5], pWaituser1->iLatestPlayUser[6], pWaituser1->iLatestPlayUser[7], pWaituser1->iLatestPlayUser[8], pWaituser1->iLatestPlayUser[9]);
	_log(_ALL, "AssignHandler", "CheckSame user2[%d],index[%d],[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", pWaituser2->iUserID, pWaituser2->iLastPlayerIndex, pWaituser2->iLatestPlayUser[0], pWaituser2->iLatestPlayUser[1], pWaituser2->iLatestPlayUser[2], pWaituser2->iLatestPlayUser[3], pWaituser2->iLatestPlayUser[4],
		pWaituser2->iLatestPlayUser[5], pWaituser2->iLatestPlayUser[6], pWaituser2->iLatestPlayUser[7], pWaituser2->iLatestPlayUser[8], pWaituser2->iLatestPlayUser[9]);*/
	return bHisTableUser;
}
bool AssignTableHandler::CheckIfRoomTypeLimit(WaitTableUserInfoDef* pWaituser1, WaitTableUserInfoDef* pWaituser2)
{
	bool bRoomTypeLimit = false;
	if (m_iForbidRoom8With5 == 1 && ((pWaituser1->iIfFreeRoom == 5 && pWaituser2->iIfFreeRoom == 8) || (pWaituser1->iIfFreeRoom == 8 && pWaituser2->iIfFreeRoom == 5)))//<=初级房，看是否有配置禁止和vip房间同桌
	{
		bRoomTypeLimit = true;
	}		
	if (bRoomTypeLimit == false && m_iForbidRoom8With6 == 1 
		&& ((pWaituser1->iIfFreeRoom == 6 && pWaituser2->iIfFreeRoom == 8) || (pWaituser1->iIfFreeRoom == 8 && pWaituser2->iIfFreeRoom == 6)))//<=中级房，看是否有禁止中级房和vip房同桌
	{
		bRoomTypeLimit = true;
	}
	if (bRoomTypeLimit == false && m_iForbidRoom7With5 == 1 
		&& ((pWaituser1->iIfFreeRoom <= 5 && pWaituser2->iIfFreeRoom >= 7) || (pWaituser1->iIfFreeRoom >= 7 && pWaituser2->iIfFreeRoom <= 5)))//<=初级房，看是否有配置禁止和高级房间同桌
	{
		bRoomTypeLimit = true;
	}
	if (bRoomTypeLimit == false && m_iForbidRoom6With5 == 1
		&& ((pWaituser1->iIfFreeRoom <= 5 && pWaituser2->iIfFreeRoom >= 6) || (pWaituser1->iIfFreeRoom >= 6 && pWaituser2->iIfFreeRoom <= 5)))//<=初级房，看是否有配置禁止和中级房间同桌
	{
		bRoomTypeLimit = true;
	}
	if (bRoomTypeLimit == false && m_iForbidRoom7With6 == 1
		&& ((pWaituser1->iIfFreeRoom <= 6 && pWaituser2->iIfFreeRoom >= 7) || (pWaituser1->iIfFreeRoom >= 7 && pWaituser2->iIfFreeRoom <= 6)))//<=中级房，看是否有配置禁止和高级房间同桌
	{
		bRoomTypeLimit = true;
	}
	//_log(_DEBUG, "AssignHandler", "CheckIfRoomTypeLimit user1[%d],user2[%d],bRoomTypeLimit[%d]", pWaituser1->iUserID, pWaituser2->iUserID, bRoomTypeLimit);
	return bRoomTypeLimit;
}

bool AssignTableHandler::CheckIfUsersCanSitTogether(WaitTableUserInfoDef* pWaitUser, WaitTableUserInfoDef* pJudgeUser)
{
	if (pWaitUser->iTableStatus != 0 || pJudgeUser->iTableStatus != 0)
	{
		_log(_ALL, "AssignHandler", "CanSitTogether user1[%d],user2[%d],iTableStatus[%d,%d]", pWaitUser->iUserID, pJudgeUser->iUserID, pWaitUser->iTableStatus, pJudgeUser->iTableStatus);
		return false;
	}
	if (pJudgeUser->iUserID == pWaitUser->iUserID)
	{
		return false;
	}
	bool bTmLimit = false;
	bool bIPLimit = false;
	bool bHisLimit = false;
	bool bPlayTypeLimit = false;
	bool bPlayerNumLimit = false;
	bool bServerLimit = false;
	bool bRoomTypeLimit = false;
			
	if (pJudgeUser->iTableStatus == 0 && pJudgeUser->iUserID != pWaitUser->iUserID)
	{
		//检查玩家所在服务器是否还开放（未开放就不分配了，等玩家再次入座请求时，会帮助其切换服务器）
		int iServerState = m_pRoomServerLogic->JudgeIfServerCanEnter(pJudgeUser->iInServerID);
		if (iServerState == 2 && (pJudgeUser->iVipType != 96 || pJudgeUser->iSpeMark == 0))
		{
			bServerLimit = true;
			_log(_ALL, "AssignHandler", "CanSitTogether user1[%d],user2[%d],bServerLimit[%d,%d]", pWaitUser->iUserID, pJudgeUser->iUserID, pJudgeUser->iInServerID, iServerState);
			return false;
		}

		//维护号和为维护号不坐一起
		if ((pWaitUser->iVipType != 96 && pJudgeUser->iVipType == 96 && pJudgeUser->iSpeMark > 0) || (pWaitUser->iVipType == 96 && pJudgeUser->iVipType != 96 && pWaitUser->iSpeMark > 0))
		{
			bServerLimit = true;
			_log(_ALL, "AssignHandler", "CanSitTogether user1[%d],user2[%d],vipType[%d,%d] SpeMark[%d,%d]", pWaitUser->iUserID, pJudgeUser->iUserID, pWaitUser->iVipType, pJudgeUser->iVipType, pWaitUser->iSpeMark,pJudgeUser->iSpeMark);
			return false;
		}
		//人数检查
		while (true)
		{
			if (pWaitUser->iMinPlayer == 0 || pJudgeUser->iMinPlayer == 0)
			{
				break;
			}
			//检查下人数是否一致
			if (pWaitUser->iMinPlayer == pWaitUser->iMaxPlayer)//自己是固定人数玩法，就必须找同样固定玩法的人数
			{
				if (pJudgeUser->iMinPlayer == pJudgeUser->iMaxPlayer && pWaitUser->iMinPlayer == pJudgeUser->iMinPlayer)
				{
					break;
				}
			}
			else
			{
				//自己是非固定玩法的，则1，找非固定玩法，且人数范围有交集的,2找满足最低人数固定玩法的也可以（因为每轮可开桌的条件就是够最低人数就开桌）
				if (pJudgeUser->iMinPlayer == pJudgeUser->iMaxPlayer)
				{
					if (pWaitUser->iMinPlayer <= pJudgeUser->iMinPlayer)
					{
						break;
					}
				}
				else
				{
					if (pWaitUser->iMinPlayer <= pJudgeUser->iMinPlayer && pWaitUser->iMaxPlayer >= pJudgeUser->iMinPlayer)
					{
						break;
					}
					else if (pJudgeUser->iMinPlayer <= pWaitUser->iMinPlayer && pJudgeUser->iMaxPlayer >= pWaitUser->iMinPlayer)
					{
						break;
					}
				}
			}
			_log(_ALL, "AssignHandler", "CanSitTogether user1[%d],user2[%d],bPlayerNumLimit[%d-%d,%d-%d]", pWaitUser->iUserID, pJudgeUser->iUserID, pWaitUser->iMinPlayer, pWaitUser->iMaxPlayer, pJudgeUser->iMinPlayer, pJudgeUser->iMaxPlayer);
			bPlayerNumLimit = true;
			return false;
		}
		//检查下时间限制
		//_log(_DEBUG, "AssignHandler", "CanSitTogether time user[%d,%d,%d],find[%d,%d,%d],timeGap[%d]", pWaitUser->iUserID, pWaitUser->iIfFreeRoom, pWaitUser->iFirstATHTime, pJudgeUser->iUserID, pJudgeUser->iIfFreeRoom, pJudgeUser->iFirstATHTime, abs(pJudgeUser->iFirstATHTime - pWaitUser->iFirstATHTime));
		if (abs(pJudgeUser->iFirstATHTime - pWaitUser->iFirstATHTime) <= m_iSameTmLimitSec)
		{
			_log(_ALL, "AssignHandler", "CanSitTogether user1[%d],user2[%d],bTmLimit[%d,%d]", pWaitUser->iUserID, pJudgeUser->iUserID, pWaitUser->iFirstATHTime, pJudgeUser->iFirstATHTime);
			bTmLimit = true;
			return false;
		}
		//_log(_DEBUG, "AssignHandler", "CanSitTogether ip user[%d,%d,%d],find[%d,%d,%d],ipCompare[%d]", pWaitUser->iUserID, pWaitUser->iIfFreeRoom,
		//	m_pRoomServerLogic->IfRoomHaveIPLimit(pWaitUser->iIfFreeRoom), pJudgeUser->iUserID, pJudgeUser->iIfFreeRoom, 
		//	m_pRoomServerLogic->IfRoomHaveIPLimit(pJudgeUser->iIfFreeRoom), strcmp(pWaitUser->szIP, pJudgeUser->szIP));
		//检查下ip限制
		if (m_pRoomServerLogic->IfRoomHaveIPLimit(pJudgeUser->iIfFreeRoom) == 1 || m_pRoomServerLogic->IfRoomHaveIPLimit(pWaitUser->iIfFreeRoom) == 1)
		{
			if (strcmp(pWaitUser->szIP, pJudgeUser->szIP) == 0)
			{
				bIPLimit = true;
				_log(_ALL, "AssignHandler", "CanSitTogether user1[%d],user2[%d],bIPLimit", pWaitUser->iUserID, pJudgeUser->iUserID);
				return false;
			}
		}
		bHisLimit = CheckIfHisSameTable(pWaitUser, pJudgeUser);
		if (bHisLimit)
		{
			_log(_ALL, "AssignHandler", "CanSitTogether user1[%d],user2[%d],bHisLimit", pWaitUser->iUserID, pJudgeUser->iUserID);
			return false;
		}
		//检查下是不是设置了服务器类型限制
		bRoomTypeLimit = CheckIfRoomTypeLimit(pWaitUser, pJudgeUser);
		if (bRoomTypeLimit)
		{
			_log(_ALL, "AssignHandler", "CanSitTogether user1[%d],user2[%d],CheckIfRoomTypeLimit[%d,%d]", pWaitUser->iUserID, pJudgeUser->iUserID, pWaitUser->iIfFreeRoom, pJudgeUser->iIfFreeRoom);
			return false;
		}
	}
	return true;
}


VirtualAIInfoDef* AssignTableHandler::GetFreeVirtualAI(bool bCanCreate)
{
	VirtualAIInfoDef *pVirtualAI = NULL;
	if (!m_vcFreeVirtulAI.empty())
	{
		pVirtualAI = m_vcFreeVirtulAI[0];
		m_vcFreeVirtulAI.erase(m_vcFreeVirtulAI.begin());
	}
	else if(bCanCreate)
	{
		pVirtualAI = new VirtualAIInfoDef();
		//_log(_DEBUG, "ATH", "GetFreeVirtualAI memory_AI all[%d]", m_vcFreeVirtulAI.size());
	}
	if (!pVirtualAI)
	{
		_log(_ERROR, "ATH", "GetFreeVirtualAI: Cannot get free VirtualAI.");
	}
	return pVirtualAI;
}

void AssignTableHandler::ReleaseVirtualAI(VirtualAIInfoDef* pVirtualAI)
{
	//先遍历一遍保证不会重复放入
	for (int i = 0; i < m_vcFreeVirtulAI.size(); i++)
	{
		if (m_vcFreeVirtulAI[i]->iVirtualID == pVirtualAI->iVirtualID)
		{
			_log(_ERROR, "ATH", "ReleaseVirtualAI: [%d] VirtualAI has in free", pVirtualAI->iVirtualID);
			return;
		}
	}
	_log(_DEBUG, "ATH", "ReleaseVirtualAI: [%d] in[%d]", pVirtualAI->iVirtualID, pVirtualAI->iInVTableID);
	pVirtualAI->iInVTableID = 0;
	m_vcFreeVirtulAI.push_back(pVirtualAI);
}

void AssignTableHandler::ReleaseVirtualAI(int iVirtualID)
{
	VirtualAIInfoDef* pVirtualAI = (VirtualAIInfoDef *)(m_hashVirtual.Find((void *)(&iVirtualID), sizeof(int)));
	if (pVirtualAI == NULL)
	{
		return;
	}
	ReleaseVirtualAI(pVirtualAI);
}

bool AssignTableHandler::CheckIfCanSitWithVirtualAI(int iTableRealUser[], VirtualAIInfoDef* pAIInfo, int iVirtualID)
{
	if (pAIInfo == NULL)
	{
		pAIInfo = (VirtualAIInfoDef *)(m_hashVirtual.Find((void *)(&iVirtualID), sizeof(int)));
	}
	if (pAIInfo == NULL)
	{
		return false;
	}
	bool bAIOK = true;
	for (int i = 0; i < 10; i++)
	{
		if (iTableRealUser[i] > g_iMaxVirtualID)
		{
			for (int j = 0; j < 10; j++)
			{
				if (iTableRealUser[i] == pAIInfo->iLatestTableUesr[j])
				{
					bAIOK = false;
					break;
				}
			}
		}
		if (!bAIOK)
		{
			break;
		}
	}
	return bAIOK;
}

VirtualAIInfoDef* AssignTableHandler::GetFreeValidVirtualAI(int iTableRealUser[])
{		
	VirtualAIInfoDef* pFirstFindAI = NULL;
	while (true)
	{	
		VirtualAIInfoDef* pAIInfo = GetFreeVirtualAI(false);
		if (pAIInfo == NULL)
		{
			return NULL;
		}
		bool bAIOK = CheckIfCanSitWithVirtualAI(iTableRealUser, pAIInfo,0);
		if (bAIOK)
		{
			return pAIInfo;
		}
		ReleaseVirtualAI(pAIInfo);//不能用就要回收
		if (pFirstFindAI != NULL  && pFirstFindAI == pAIInfo)//所有的空闲节点都试过了
		{
			return NULL;
		}
		if (pFirstFindAI == NULL)
		{
			pFirstFindAI = pAIInfo;
		}
	}
	return NULL;
}

void AssignTableHandler::SetVirtualAITableUser(int iTableUser[])
{
	for (int i = 0; i < 10; i++)
	{
		if (iTableUser[i] >= g_iMinVirtualID && iTableUser[i] <= g_iMaxVirtualID)
		{
			VirtualAIInfoDef* pVirtualAI = (VirtualAIInfoDef *)(m_hashVirtual.Find((void *)(&iTableUser[i]), sizeof(int)));
			if (pVirtualAI == NULL)
			{
				_log(_ERROR, "ATH", "SetVirtualAITableUser: Cannot find[%d] VirtualAI", iTableUser[i]);
				continue;
			}
			for (int j = 0; j < 10; j++)
			{
				if (iTableUser[j] > g_iMaxVirtualID)
				{
					pVirtualAI->iLatestTableUesr[pVirtualAI->iLatestIndex] = iTableUser[j];
					pVirtualAI->iLatestIndex++;
					if (pVirtualAI->iLatestIndex > 9)
					{
						pVirtualAI->iLatestIndex = 0;
					}
				}
			}
		}
	}
}

int AssignTableHandler::GetPlayNum(int iPlayType)
{
	return (iPlayType >> 16 & 0xff);
}

void AssignTableHandler::AddFindPlayerFromVTable(WaitTableUserInfoDef* pWaituser, WaitTableUserInfoDef* pFindUser[], int &iFindUserIndex, int iLastVirtualAI[], int &iLastVAIIndex)
{
	VirtualTableDef* pVTableInfo = FindVTable(pWaituser->iVTableID);
	if (pVTableInfo == NULL)
	{
		_log(_ERROR, "AssignHandler", "AddFindPlayerFromVTable,not find VTable[%d]", pWaituser->iVTableID);
		pWaituser->iVTableID = 0;
	}
	else
	{
		//这里再重设一次玩法，避免中心服务器给快速开始玩家分配了玩法，但还未到游戏服务器，游戏服务器又再次发送了玩法为0的入座请求
		if (pVTableInfo->iPlayType > 0 && pWaituser->iPlayType == 0)
		{
			SetWaitUserPlayType(pWaituser, pVTableInfo->iPlayType);
		}
		for (int i = 0; i < 10; i++)
		{
			if (pVTableInfo->iUserID[i] == pWaituser->iUserID)
			{
				continue;
			}
			if (pVTableInfo->iUserID[i] == 0)
			{
				continue;
			}
			if (pVTableInfo->iUserID[i] >= g_iMinVirtualID && pVTableInfo->iUserID[i] <= g_iMaxVirtualID)
			{
				iLastVirtualAI[iLastVAIIndex] = pVTableInfo->iUserID[i];
				iLastVAIIndex++;
			}
			else
			{
				WaitTableUserInfoDef* pTempUser = FindWaitUser(pVTableInfo->iUserID[i]);
				if (pTempUser != NULL
					&& pTempUser->iTableStatus == 0
					&& pTempUser->iVTableID == pWaituser->iVTableID)
				{
					pFindUser[iFindUserIndex] = pTempUser;
					iFindUserIndex++;
				}
				else
				{
					if (pTempUser == NULL)
					{
						RemoveVTableUser(pVTableInfo, pVTableInfo->iUserID[i], true);
						_log(_ERROR, "AssignHandler", "AddFindPlayerFromVTable[%d],VTable[%d],not find[%d]", pWaituser->iUserID, pWaituser->iVTableID, pVTableInfo->iUserID[i]);
					}
					else
					{
						if (pTempUser->iTableStatus != 0 || pTempUser->iVTableID != pWaituser->iVTableID)
						{
							if (pTempUser->iTableStatus != 0)
							{
								pTempUser->iVTableID = 0;
							}
							CheckRemoveVTableUser(pTempUser);
							_log(_ERROR, "AssignHandler", "AddFindPlayerFromVTable[%d],VTable[%d],checkUser[%d],st[%d],vTable[%d:%d]", pWaituser->iUserID, pWaituser->iVTableID, pTempUser->iUserID, pTempUser->iTableStatus, pTempUser->iVTableID, pWaituser->iVTableID);
						}
					}
					if (pVTableInfo->iVTableID < 0)
					{
						break;
					}
				}
			}			
		}
	}
}

VirtualTableDef* AssignTableHandler::GetFreeVTable(int iVTableID/* = 0*/)
{
	VirtualTableDef* pVTable = NULL;
	bool bNeedNew = false;

	if (m_vcFreeVTables.empty())
	{
		bNeedNew = true;
	}
	else
	{
		int tmNow = time(NULL);
		int iIndex = 0;
		if (iVTableID > 0)
		{
			for (int i = 0; i < m_vcFreeVTables.size(); i++)
			{
				if (m_vcFreeVTables[i]->iVTableID == iVTableID && tmNow - m_vcFreeVTables[i]->iFreeTm > 600)
				{
					iIndex = i;
					break;
				}
			}
		}

		pVTable = m_vcFreeVTables[iIndex];
		//_log(_DEBUG, "ATH", "GetFreeVTable111--->VTableID[%d]:iIndex[%d]", pVTable->iVTableID, iIndex);
		if (pVTable->iVTableID < 0)
		{
			pVTable->iVTableID = -pVTable->iVTableID;
		}
		if (iVTableID > 0)
		{
			pVTable->iVTableID = iVTableID;
		}
		m_vcFreeVTables.erase(m_vcFreeVTables.begin() + iIndex);
	}

	if (bNeedNew)
	{
		pVTable = new VirtualTableDef();
		pVTable->iFreeTm = 0;
		if (iVTableID > 0)
		{
			pVTable->iVTableID = iVTableID;
		}
		else
		{
			pVTable->iVTableID = ++m_iVTableID;
			if (pVTable->iVTableID > g_iMaxUseIntValue)
			{
				pVTable->iVTableID = 1;
				m_iVTableID = 1;
			}
		}
		//_log(_DEBUG, "ATH", "GetFreeVTable memory_tab all[%d] free[%d] cur[%d]", m_mapAllVTables.size(), m_vcFreeVTables.size(), pVTable->iVTableID);
	}

	int iTempVTableID = pVTable->iVTableID;
	memset(pVTable, 0, sizeof(VirtualTableDef));
	pVTable->iVTableID = iTempVTableID;
	return pVTable;
}

void AssignTableHandler::AddToFreeVTables(VirtualTableDef* pVTable)
{
	pVTable->iVTableID = -pVTable->iVTableID;
	bool bFreeTab = true;
	for (int i = 0; i < 10; i++)
	{
		if (pVTable->iUserID[i] > 0)
		{
			bFreeTab = false;
			break;
		}
	}

	bool bExist = false;
	if (bFreeTab)
	{
		for (int i = 0; i < m_vcFreeVTables.size(); i++)
		{
			if (m_vcFreeVTables[i] == pVTable)
			{
				bExist = true;
				break;
			}
		}
	}

	if (bFreeTab && !bExist)
	{
		pVTable->iFreeTm = time(NULL);
		m_vcFreeVTables.push_back(pVTable);
		//_log(_DEBUG, "ATH", "AddToFreeVTables memory_tab all[%d] free[%d] cur[%d]", m_mapAllVTables.size(), m_vcFreeVTables.size(), pVTable->iVTableID);
	}
	else
	{
		_log(_ERROR, "ATH", "AddToFreeVTables_ERROR[%d] bFreeTab[%d] bExist[%d]", pVTable->iVTableID, bFreeTab, bExist);
	}
	//补2个判断，1检验m_vcFreeVTables是否有已经pVTable，2检验pVTable是否还有人（有人加个err日志，不push进去）
	//再补个功能VirtualTableDef加一个下次可用时间（time（null）+300），在GetFreeVTable确定过了时间才可以用这个桌子

}

VirtualTableDef* AssignTableHandler::FindVTable(int iVTableID)
{
	map<int, VirtualTableDef*>::iterator iter = m_mapAllVTables.find(iVTableID);
	if (iter != m_mapAllVTables.end())
	{
		return iter->second;
	}
	return NULL;
}
void AssignTableHandler::CheckRemoveVTableUser(WaitTableUserInfoDef* pUser)
{
	if (pUser == NULL)
	{
		return;
	}
	if (pUser->iVTableID == 0)
	{
		return;
	}
	VirtualTableDef* pVTable = FindVTable(pUser->iVTableID);
	if (pVTable != NULL)
	{
		RemoveVTableUser(pVTable, pUser->iUserID,true);
	}
}

void AssignTableHandler::RemoveVTableUser(VirtualTableDef* pVTable, int iUserID, bool bEraseFromMap)
{	
	if (pVTable == NULL)
	{
		return;
	}
	int iLeftNum = 0;
	int iLeftVAINum = 0;
	bool bFind = false;
	for (int i = 0; i < 10; i++)
	{
		if (pVTable->iUserID[i] == iUserID)
		{
			bFind = true;
			pVTable->iUserID[i] = 0;
			WaitTableUserInfo* pWaitUser = FindWaitUser(iUserID);
			if (pWaitUser != NULL)
			{
				pWaitUser->iVTableID = 0;
			}
		}
		else if (pVTable->iUserID[i] > 0)
		{
			iLeftNum++;
			if (pVTable->iUserID[i] >= g_iMinVirtualID && pVTable->iUserID[i] <= g_iMaxVirtualID)
			{
				iLeftVAINum++;
			}
		}
	}
	_log(_DEBUG, "ATH", "RemoveVTableUser[%d],VTable[%d],iLeftNum[%d] iLeftVAINum[%d]", iUserID, pVTable->iVTableID, iLeftNum, iLeftVAINum);
	if (iLeftNum == 0 || iLeftNum == iLeftVAINum)
	{
		if (iLeftVAINum > 0)//剩余全是虚拟AI，回收AI
		{
			for (int i = 0; i < 10; i++)
			{
				if (pVTable->iUserID[i] > 0)
				{
					VirtualAIInfoDef* pVirtualAI = (VirtualAIInfoDef *)(m_hashVirtual.Find((void *)(&pVTable->iUserID[i]), sizeof(int)));
					if (pVirtualAI != NULL && pVirtualAI->iInVTableID == pVTable->iVTableID)
					{
						ReleaseVirtualAI(pVTable->iUserID[i]);
					}		
				}
			}
			iLeftNum = 0;
			iLeftVAINum = 0;
		}
		if (bEraseFromMap)
		{
			map<int, VirtualTableDef*>::iterator iter = m_mapAllVTables.find(pVTable->iVTableID);
			if (iter != m_mapAllVTables.end())
			{
				m_mapAllVTables.erase(iter);
			}
		}		
		AddToFreeVTables(pVTable);
	}
}

void AssignTableHandler::AddUserToVTable(int iUserID,WaitTableUserInfoDef* pUser, int iVTableID)
{
	if (iUserID > g_iMaxVirtualID && pUser == NULL)
	{
		pUser = FindWaitUser(iUserID);
		if (pUser == NULL)
		{
			_log(_DEBUG, "ATH", "AddUserToVTable:user[%d] null,VTable[%d]", iUserID, iVTableID);
			return;
		}
	}
	if (pUser != NULL && pUser->iVTableID > 0)
	{
		_log(_DEBUG, "ATH", "AddUserToVTable:user[%d] has VT[%d],VTable[%d],", iUserID, pUser->iVTableID, iVTableID);
		return;
	}	
	VirtualTableDef* pVTable = FindVTable(iVTableID);
	if (pVTable == NULL)
	{
		pVTable = GetFreeVTable(iVTableID);
		m_mapAllVTables[pVTable->iVTableID] = pVTable;
	}	
	if (pVTable == NULL)
	{
		_log(_DEBUG, "ATH", "AddUserToVTable:user[%d] VTable[%d] null", iUserID, iVTableID);
		return;
	}
	AddUserToVTable(iUserID, pUser, pVTable);
}

void AssignTableHandler::AddUserToVTable(int iUserID, WaitTableUserInfoDef* pUser, VirtualTableDef* pVTable)
{
	if (pVTable == NULL)
	{
		return;
	}
	if (iUserID > g_iMaxVirtualID && pUser == NULL)
	{
		WaitTableUserInfoDef* pWaitUser = FindWaitUser(iUserID);
		if (pWaitUser == NULL)
		{
			return;
		}
	}
	if (pUser != NULL && pUser->iVTableID > 0)
	{
		return;
	}
	if (pVTable->iPlayType > 0 && iUserID > g_iMaxVirtualID && pUser->iPlayType > 0 && pVTable->iPlayType != pUser->iPlayType)
	{
		_log(_ERROR, "ATH", "AddUserToVTable: userp[%d],playType not same[%d,%d]", pUser->iUserID, pVTable->iPlayType, pUser->iPlayType);
		return;
	}
	int iOldTablePlayType = pVTable->iPlayType;
	int iTableUserNum = 0;
	int iLeftIndex = -1;
	bool bHaveFind = false;
	for (int j = 0; j < 10; j++)
	{
		if (pVTable->iUserID[j] == 0 && iLeftIndex == -1)
		{
			iLeftIndex = j;
		}
		if (pVTable->iUserID[j] > 0)
		{
			iTableUserNum++;
			if (pVTable->iUserID[j] == iUserID)
			{
				bHaveFind = true;
			}
		}
	}
	if (bHaveFind)
	{
		_log(_DEBUG, "ATH", "AddUserToVTable:vtable[%d] has user[%d,%d]", pVTable->iVTableID, iUserID, pUser->iVTableID);
		pUser->iVTableID = pVTable->iVTableID;
		return;
	}
	if (iLeftIndex == -1)
	{
		_log(_ERROR, "ATH", "AddUserToVTable:user[%d],vtable[%d] no pos", iUserID, pVTable->iVTableID);
		return;
	}
	pVTable->iUserID[iLeftIndex] = iUserID;
	iTableUserNum++;
	if (pUser != NULL)
	{
		pUser->iVTableID = pVTable->iVTableID;
		if (pUser->iPlayType == 0 && iOldTablePlayType > 0)
		{
			SetWaitUserPlayType(pUser, iOldTablePlayType);
		}
	}
	
	if (iOldTablePlayType <= 0 && pVTable->iPlayType > 0)
	{
		//检查下之前有未设置玩法的玩家补设置下玩法
		for (int j = 0; j < 10; j++)
		{
			if (pVTable->iUserID[j] > g_iMaxVirtualID)
			{
				WaitTableUserInfo* pTempUser = FindWaitUser(pVTable->iUserID[j]);
				if (pTempUser)
				{
					SetWaitUserPlayType(pTempUser, pVTable->iPlayType);
				}
			}
		}
	}
	_log(_DEBUG, "ATH", "AddUserToVTable:vtable[%d]type[%d],add user[%d]", pVTable->iVTableID,pVTable->iPlayType, iUserID);
}

int AssignTableHandler::GetUserVTableAssignUser(int iVTableID, int iAssignUser[], int &iAssignCnt, VirtualTableDef** pVTable)
{
	int iMinNeedPlayer = 0;
	*pVTable = FindVTable(iVTableID);
	
	VirtualTableDef* pTmpTable = *pVTable;
	if (pTmpTable == NULL)
	{
		_log(_ERROR, "ATH", "GetUserVTableAssignUser:pVTable == NULL");
		return iMinNeedPlayer;
	}
	for (int i = 0; i < 10; i++)
	{
		if (pTmpTable->iUserID[i] > 0)
		{
			WaitTableUserInfo* pUser = FindWaitUser(pTmpTable->iUserID[i]);
			if (pUser != NULL && pUser->iMinPlayer > iMinNeedPlayer)
			{
				iMinNeedPlayer = pUser->iMinPlayer;
			}
			iAssignUser[iAssignCnt] = pTmpTable->iUserID[i];
			iAssignCnt++;
		}
	}
	//return 2;
	return iMinNeedPlayer;
}

int AssignTableHandler::GetUserVTableAssignUser(VirtualTableDef* pVTable)
{
	if (pVTable == NULL)
	{
		return 0;
	}
	int iAssignCnt = 0;
	for (int i = 0; i < 10; i++)
	{
		if (pVTable->iUserID[i] > 0)
		{
			iAssignCnt++;
		}
	}
	return iAssignCnt;
}

void AssignTableHandler::GetAssignConf()
{
	vector<string> vcStrParamsKLey;
	int iGameID = stSysConfigBaseInfo.iGameID;
	char cKey[128] = "";
	sprintf(cKey, "assign_conf_%d", iGameID);
	vcStrParamsKLey.push_back(cKey);
	sprintf(cKey, "assign_limit_%d", iGameID);
	vcStrParamsKLey.push_back(cKey);
	sprintf(cKey, "assign_room_%d", iGameID);
	vcStrParamsKLey.push_back(cKey);
	sprintf(cKey, "assign_dynamic_%d", iGameID);
	vcStrParamsKLey.push_back(cKey);
	GetServerConfParams(vcStrParamsKLey);
}

void AssignTableHandler::HandleSendGetVirtualAIInfoReq(char * msgData, int iLen)
{
	//判断当前消息是否来自游戏服
	_log(_DEBUG, "AssignTableHandler", "HandleSendGetVirtualAIInfoReq ilen=%d",iLen);
	//消息来自于游戏服，直接把消息转发给redis
	m_pQueueForRedis->EnQueue(msgData, iLen);
}

void AssignTableHandler::HandleGetVirtualAIMsg(char * msgData, int iLen)
{
	//消息来自于Redis，把消息转发给对应的游戏服
	RdGetVirtualAIRtMsg *pMsg = (RdGetVirtualAIRtMsg*)msgData;
	//解析一次对应ID的socketIndex 
	OneGameServerDetailInfoDef* pServerInfo = m_pRoomServerLogic->GetServerInfo(pMsg->iServerID);
	if (pServerInfo!=NULL)
	{
		EnQueueSendMsg(msgData, iLen, pServerInfo->iSocketIndex);
	}
}

void AssignTableHandler::HandleReleaseRdCTLAINodeReq(char * msgData, int iLen)
{
	//直接转发给redis
	m_pQueueForRedis->EnQueue(msgData, iLen);
}

bool AssignTableHandler::IfDynamicSeatGame()
{
	return false;
}

void AssignTableHandler::GetServerConfParams(vector<string>& vcKey)
{
	if (vcKey.empty())
	{
		return;
	}

	char cBuff[2048];
	int iBuffLen = 0;
	GameGetParamsReqDef* pMsgReq = NULL;

	//一次最多获取12个key，超过了分开发
	int iSendNum = 0;
	for (int i = 0; i < vcKey.size(); i++)
	{
		if (iSendNum == 0)
		{
			memset(cBuff, 0, sizeof(cBuff));
			pMsgReq = (GameGetParamsReqDef*)cBuff;
			iBuffLen = sizeof(GameGetParamsReqDef);
			pMsgReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
			pMsgReq->msgHeadInfo.iMsgType = REDIS_GET_PARAMS_MSG;
		}
		memcpy(cBuff + iBuffLen, vcKey[i].c_str(), 32);
		iBuffLen += 32;
		iSendNum++;
		if (iSendNum >= 12)
		{
			if (m_pQueueForRedis)
			{
				pMsgReq->iKeyNum = iSendNum;
				m_pQueueForRedis->EnQueue(cBuff, iBuffLen);
				iSendNum = 0;
				iBuffLen = 0;
			}
		}
	}
	if (iSendNum > 0)
	{
		if (m_pQueueForRedis)
		{
			pMsgReq->iKeyNum = iSendNum;
			m_pQueueForRedis->EnQueue(cBuff, iBuffLen);
		}
	}
}

void AssignTableHandler::HandleGetParamsMsg(void* szMsg, int iSocketIndex)
{
	GameGetParamsResDef* msgRes = (GameGetParamsResDef*)szMsg;

	char cTmpKey[32] = { 0 };
	char cTmpValue[128] = { 0 };
	char cTemp[128] = { 0 };

	for (int m = 0; m < msgRes->iKeyNum; m++)
	{
		memset(cTmpKey, 0, sizeof(cTmpKey));
		memcpy(cTmpKey, (char*)szMsg + sizeof(GameGetParamsResDef) + m * (32 + 128), 32);

		memset(cTmpValue, 0, sizeof(cTmpValue));
		memcpy(cTmpValue, (char*)szMsg + sizeof(GameGetParamsResDef) + m * (32 + 128) + 32, 128);

		_log(_DEBUG, "ATH", "Params key[%s] val[%s]", cTmpKey, cTmpValue);

		int iGameID = stSysConfigBaseInfo.iGameID;
		char cKey[128] = "";
		sprintf(cKey, "assign_conf_%d", iGameID);
		if (strcmp(cTmpKey, cKey) == 0)
		{
			sscanf(cTmpValue, "%d_%d_%d_%d_%d_%d", &m_iForbidSameTableNum, &m_iIfForbidPreMixRoom, &m_iSameTmLimitSec, &m_iMaxMark9Num,
				&m_iFindSameRoomSec, &m_iFindNeighborRoomSec);
		}

		sprintf(cKey, "assign_limit_%d", iGameID);
		if (strcmp(cTmpKey, cKey) == 0)
		{
			sscanf(cTmpValue, "%d_%d_%d_%d", &m_iRateMethod, &m_iLowRate, &m_iHightRate, &m_iHighSitWithLow);
		}

		sprintf(cKey, "assign_room_%d", iGameID);
		if (strcmp(cTmpKey, cKey) == 0)
		{
			sscanf(cTmpValue, "%d_%d_%d_%d_%d", &m_iForbidRoom8With5, &m_iForbidRoom8With6, &m_iForbidRoom7With5, &m_iForbidRoom6With5, &m_iForbidRoom7With6);
		}

		sprintf(cKey, "assign_ai_%d", iGameID);
		if (strcmp(cTmpKey, cKey) == 0)
		{
			vector<string> vcStrOut;
			Util::CutString(vcStrOut, cTmpValue, "_");
			if (vcStrOut.size() == 4)
			{
				for (int i = 0; i < vcStrOut.size(); i++)
				{
					if (i == 0)
					{
						m_iAssignAI = atoi(vcStrOut[i].c_str());
					}
					else if (i == 1)
					{
						strcpy(cTemp, vcStrOut[i].c_str());
						Util::ParaseParamValue(cTemp, m_iWaitAISec, "&");
					}
					else if (i == 2)
					{
						strcpy(cTemp, vcStrOut[i].c_str());
						Util::ParaseParamValue(cTemp, m_iWaitAIOnline, "&");
					}
					else if (i == 3)
					{
						strcpy(cTemp, vcStrOut[i].c_str());
						Util::ParaseParamValue(cTemp, m_iPutAIRate, "&");
					}
				}
			}
		}
		int iLocalTestAI = 0;
		GetValueInt(&iLocalTestAI, "assign_ai", "local_test.conf", "test_conf", "-1");
		if (iLocalTestAI > -1)
		{
			m_iAssignAI = iLocalTestAI;
		}
	}
}