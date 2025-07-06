#include "ClientAuthenHandler.h"
#include <time.h>
#include "Util.h"
#include "log.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include <arpa/inet.h>  
#include "AssignTableHandler.h"

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/

ClientAuthenHandler::ClientAuthenHandler(MsgQueue *pSendQueue)
{
	m_pSendQueue = pSendQueue;
	memset(m_szBuffer,0,sizeof(m_szBuffer));
	m_pRoomServerLogic = new RoomAndServerLogic(RoomAndServerLogic::CALL_CLIENT, this);
	m_pRoomServerLogic->SetSendQueue(m_pSendQueue);
}
ClientAuthenHandler::~ClientAuthenHandler()
{
	delete m_pRoomServerLogic;
}
void ClientAuthenHandler::HandleMsg(int iMsgType, char* szMsg, int iLen, int iSocketIndex)
{
	//_log(_DEBUG, "ClientAuthenHandler::HandleMsg", "iMsgType[%x]", iMsgType);
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
	else if (iMsgType == AUTHEN_REQ_MSG)
	{
		HandleAuthenReqRadius(szMsg, iSocketIndex);
	}
}

void ClientAuthenHandler::CallBackOnTimer(int iTimeNow)
{
}

void ClientAuthenHandler::HandleGameRoomInfoReqMsg(char* msgData, int iSocketIndex)
{
	//_log(_DEBUG, "ClientAuthenHandler", "---HandleGameRoomInfoReqMsg---");
	m_pRoomServerLogic->HandleGameRoomInfoReqMsg(msgData, iSocketIndex);
}

void ClientAuthenHandler::HandleGameServerAuthenMsg(char* msgData, int iSocketIndex)
{
	m_pRoomServerLogic->HandleGameServerAuthenMsg(msgData, iSocketIndex);
}

void ClientAuthenHandler::HandleGameServerDetailMsg(char* msgData, int iSocketIndex)
{
	m_pRoomServerLogic->HandleGameServerDetailMsg(msgData, iSocketIndex);
}

void ClientAuthenHandler::HandleGameServerSysOnlineMsg(char* msgData, int iSocketIndex)
{
	m_pRoomServerLogic->HandleGameServerSysOnlineMsg(msgData, iSocketIndex);
}

void ClientAuthenHandler::HandleAuthenReqRadius(char *msgData, int iSocketIndex)
{
	AuthenReqDef* pMsgReq = (AuthenReqDef*)msgData;

	_log(_DEBUG, "RSL", "HandleAuthenReqRadius uid[%d] serverId[%d] roomType[%d] vip[%d] spMark[%d] roomif[%d],CenterState[%d]",
		pMsgReq->iUserID, pMsgReq->iServerID, pMsgReq->cEnterRoomType, pMsgReq->iVipType, pMsgReq->iSpeMark, pMsgReq->iRoomID, m_pRoomServerLogic->m_iCenterServerState);

	//发送
	char m_szBuffer[1024];
	memset(m_szBuffer, 0, sizeof(m_szBuffer));
	int iBufferLen = sizeof(CenterServerResDef);
	CenterServerResDef *pMsgRes = (CenterServerResDef*)m_szBuffer;
	pMsgRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgRes->msgHeadInfo.iMsgType = CLIENT_GET_CSERVER_LIST_MSG;
	pMsgRes->msgHeadInfo.iSocketIndex = iSocketIndex;

	ServerInfoDef* pServerInfo = (ServerInfoDef*)(m_szBuffer + iBufferLen);
	int iServerID = pMsgReq->iServerID;
	if (iServerID > 0)//有卡房间，返回所卡房间信息(这里在客户端会改成通过web服务器获取所卡房间的ip和端口信息，避免所卡游戏服务器并没有连接在该中心服务器下)
	{
		OneGameServerDetailInfoDef* pDetailServerInfo = m_pRoomServerLogic->GetServerInfo(iServerID);
		if (pDetailServerInfo != NULL)
		{
			pMsgRes->iNum = 1;
			pServerInfo->iIP = pDetailServerInfo->iIP;
			pServerInfo->sPort = pDetailServerInfo->sPort;
			pServerInfo->iInnerIP = pDetailServerInfo->iInnerIP;
			pServerInfo->sInnerPort = pDetailServerInfo->sInnerPort;
		}
	}
	else
	{
		//先检测下对应的房间是不是已经关闭了
		while (true)
		{
			AssignRoomConfDef* pRoomConf = m_pRoomServerLogic->GetAssignRoomConf(pMsgReq->cEnterRoomType);
			if (pRoomConf == NULL || pRoomConf->iRoomState == 0)
			{
				pMsgRes->iNum = -1;//客户端提示房间已关闭
				break;
			}
			if (pRoomConf->iRoomState == 2)//维护中，只让维护号且iSpeMark=1的维护号进
			{
				if (pMsgReq->iVipType == 96 && pMsgReq->iSpeMark > 0)
				{
					//维护号可以进
				}
				else
				{
					pMsgRes->iNum = -2;//客户端提示房间已关闭
					break;
				}
			}
			if (m_pRoomServerLogic->m_iCenterServerState != 1)//0关闭，1正常，2维护,100隐藏
			{
				if (m_pRoomServerLogic->m_iCenterServerState != 0 && pMsgReq->iVipType == 96 && pMsgReq->iSpeMark > 0)
				{
					//维护号可以继续进维护/隐藏的中心服务器
				}
				else
				{
					pMsgRes->iNum = -3;//-3该中心服务器已关闭/维护/隐藏需要重新获取信息
					break;
				}
			}

			OneGameServerDetailInfoDef* pCommendServer = NULL;
			if (pMsgReq->iRoomID > 0)
			{
				//带了房号的房主优先给人数少的服务器
				pCommendServer = m_pRoomServerLogic->GetMinimumServerInfo();
			}
			if (pCommendServer == NULL)
			{
				pMsgRes->iNum = 0;
				if (pMsgReq->iVipType == 96 && pMsgReq->iSpeMark > 0) //维护号且要求优先进维护房间，看有没有维护房间给进去的
				{
					pCommendServer = m_pRoomServerLogic->GetMaintainServerInfo();
				}
				if (pCommendServer == NULL)
				{
					pCommendServer = m_pRoomServerLogic->GetCommendServerInfo();
				}
			}
			//房间关闭不要或者满员就不用发下线的房间信息了 iNum 为0
			if (pCommendServer == NULL)
			{
				pMsgRes->iNum = 0;
				break;
			}
			else
			{
				pMsgRes->iNum = 1;
				pServerInfo->iIP = pCommendServer->iIP;
				pServerInfo->sPort = pCommendServer->sPort;
				pServerInfo->iInnerIP = pCommendServer->iInnerIP;
				pServerInfo->sInnerPort = pCommendServer->sInnerPort;
			}
			break;
		}
	}
	if (pMsgRes->iNum > 0)
	{
		iBufferLen += pMsgRes->iNum * sizeof(ServerInfoDef);
	}
	if (m_pSendQueue)
	{
		m_pSendQueue->EnQueue(m_szBuffer, iBufferLen);
	}

	_log(_DEBUG, "RSL", "HandleAuthenReqRadius iNum[%d] iIP[%d] sPort[%d] state[%d]", pMsgRes->iNum, pServerInfo->iIP, pServerInfo->sPort, m_pRoomServerLogic->m_iCenterServerState);

}