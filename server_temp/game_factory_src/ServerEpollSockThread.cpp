#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ServerEpollSockThread.h"
#include "conf.h"
#include "time.h"
#include "log.h"
#include "aeslib.h"
#include "main_ini.h"
#include <string>
#include "gamelogic.h"
using namespace std;

ServerEpollSockThread::ServerEpollSockThread(int iAllNodeNum, int iThreadType, int iWriteMode, int iReadBufferLen, int iWriteBufferLen)
	:TcpSock(iAllNodeNum, iWriteMode, iReadBufferLen, iWriteBufferLen)
{
	m_iSocketIndex = -1;
	m_iOldSocketIndex = -1;
	memset(m_szConfFileName, 0, sizeof(m_szConfFileName));

	memset(m_szPasswd, 0, sizeof(m_szPasswd));
	strcpy(m_szPasswd, "k348jf93nr");

	m_iThreadType = iThreadType;

	m_iConnectIndex = 0;
	m_bServerAuthenOK = false;

	m_pGL = NULL;
}

ServerEpollSockThread::~ServerEpollSockThread()
{

}

int ServerEpollSockThread::Ini(MsgQueue *pGetQueue, MsgQueue *pSendQueue, char *strFileName)
{
	m_pGetQueue = pGetQueue;
	m_pSendQueue = pSendQueue;

	strcpy(m_szConfFileName, strFileName);

	GetValueStr(m_szServerIP, "HOST_IP", strFileName, "GENERAL", "");
	GetValueInt(&m_iServerPort, "HOST_PORT", strFileName, "GENERAL", "0");

	GetValueStr(m_szServerIP2, "HOST_IP2", strFileName, "GENERAL", "");
	GetValueInt(&m_iServerPort2, "HOST_PORT2", strFileName, "GENERAL", "0");

	if (m_iServerPort == 0 || m_iServerPort2 == 0)
	{
		_log(_ERROR, "SEST", "IniErr:iGameID[%d] file[%s],m_szServerIP =%s m_iServerPort =%d m_szServerIP2 =%s m_iServerPort2 =%d", GameLogic::m_pServerBaseInfo->iGameID, strFileName, m_szServerIP, m_iServerPort, m_szServerIP2, m_iServerPort2);
		return -1;
	}

	m_iSocketIndex = ConnectBaseServer();
	if(m_iSocketIndex == -1)
	{
		exit(0);
	}

	string strTemp = strFileName;
	int iPos = strTemp.find(".");
	if (iPos != string::npos)
	{
		strTemp = strTemp.substr(0, iPos);
	}

	SetTcpSockName((char*)strTemp.c_str());

	SendServerAuthenMsg();

	return m_iSocketIndex;
}

int ServerEpollSockThread::Ini(MsgQueue *pGetQueue, MsgQueue *pSendQueue, char *szServerIP, int iServerPort, char *szServerIP2, int iServerPort2)
{
	m_pGetQueue = pGetQueue;
	m_pSendQueue = pSendQueue;

	strcpy(m_szServerIP, szServerIP);
	m_iServerPort = iServerPort;

	strcpy(m_szServerIP2, szServerIP2);
	m_iServerPort2 = iServerPort2;

	m_iSocketIndex = ConnectBaseServer();
	if (m_iSocketIndex == -1)
	{
		exit(0);
	}

	char szTemp[32];
	sprintf(szTemp, "serverEpoll_%d", m_iThreadType);
	SetTcpSockName(szTemp);

	SendServerAuthenMsg();

	return m_iSocketIndex;
}

int ServerEpollSockThread::ConnectBaseServer()
{
	//连接主服务器
	int iSocketIndex = AddTCPClientNode(m_szServerIP, m_iServerPort, 0);
	if (iSocketIndex == -1)
	{
		_log(_ERROR, "SEST", "--Server[%s:%d] connect1 error", m_szServerIP, m_iServerPort);
	}
	else
	{
		_log(_ERROR, "SEST", "--Server[%s:%d] connect1 ok", m_szServerIP, m_iServerPort);
		m_iConnectIndex = 0;
	}

	//连接备用服务器
	if (iSocketIndex == -1)
	{
		iSocketIndex = AddTCPClientNode(m_szServerIP2, m_iServerPort2, 0);
		if (iSocketIndex == -1)
		{
			_log(_ERROR, "SEST", "--Server[%s:%d] connect2 error", m_szServerIP2, m_iServerPort2);
		}
		else
		{
			_log(_ERROR, "SEST", "--Server[%s:%d] connect2 ok", m_szServerIP2, m_iServerPort2);
			m_iConnectIndex = 1;
		}
	}

	return iSocketIndex;
}

void ServerEpollSockThread::ResetSeverIPInfo(char* szServerIP, int iServerPort, char* szServerIP2, int iServerPort2)
{
	bool bNeedReconnection = false;
	if (strcmp(szServerIP, m_szServerIP) != 0 || m_iServerPort != iServerPort)
	{
		bNeedReconnection = true;
		_log(_ERROR, "SEST", "ResetSeverIPInfo1 ip[%s][%s] port[%d][%d]", m_szServerIP, szServerIP, m_iServerPort, iServerPort);
	}

	if (m_iConnectIndex == 1 && (strcmp(szServerIP2, m_szServerIP2) != 0 || m_iServerPort2 != iServerPort2))
	{
		bNeedReconnection = true;
		_log(_ERROR, "SEST", "ResetSeverIPInfo2 ip[%s][%s] port[%d][%d]", m_szServerIP2, szServerIP2, m_iServerPort2, iServerPort2);
	}

	strcpy(m_szServerIP, szServerIP);
	m_iServerPort = iServerPort;

	strcpy(m_szServerIP2, szServerIP2);
	m_iServerPort2 = iServerPort2;

	if (bNeedReconnection)
	{
		int iSocketIndex = AddTCPClientNode(m_szServerIP, m_iServerPort, 0);
		if (iSocketIndex != -1)
		{
			m_iConnectIndex = 0;
			SetKillFlag(m_iSocketIndex, true);
			m_iOldSocketIndex = m_iSocketIndex;
			m_iSocketIndex = iSocketIndex;
			SendServerAuthenMsg();
			_log(_ERROR, "SEST", "%s thread[%d],convert conncet OK--Server1[%s:%d]", m_szConfFileName, m_iThreadType, m_szServerIP, m_iServerPort);
		}
		else
		{
			iSocketIndex = AddTCPClientNode(m_szServerIP2, m_iServerPort2, 0);
			if (iSocketIndex != -1)
			{
				m_iConnectIndex = 1;
				SetKillFlag(m_iSocketIndex, true);
				m_iOldSocketIndex = m_iSocketIndex;
				m_iSocketIndex = iSocketIndex;
				SendServerAuthenMsg();
				_log(_ERROR, "SEST", "%s thread[%d],convert conncet OK--Server2[%s:%d]", m_szConfFileName, m_iThreadType, m_szServerIP2, m_iServerPort2);
			}
		}
	}
}

void ServerEpollSockThread::SetGameLogic(GameLogic* pGL)
{
	m_pGL = pGL;
}

void ServerEpollSockThread::CallbackAddSocketNode(int iSocketIndex)
{

}

void ServerEpollSockThread::CallbackDelSocketNode(int iSocketIndex, int iDisType, int iProxy)
{
	_log(_ERROR, "SEST", "filename[%s],threadType[%d],disconnet----Server[%s:%d],iDisType[%d],disSocket[%d],old[%d]now[%d]", m_szConfFileName, m_iThreadType, m_szServerIP, m_iServerPort, iDisType, iSocketIndex, m_iOldSocketIndex, m_iSocketIndex);
	if (iSocketIndex == m_iOldSocketIndex)
	{
		m_iOldSocketIndex = -1;
		return;
	}
	m_iSocketIndex = -1;
}

bool ServerEpollSockThread::CallbackTCPReadData(int iSocketIndex, char *szMsg, int iLen, int iAesFlag)
{
	//MsgHeadDef* pHeadTemp = (MsgHeadDef*)szMsg;
	//_log(_DEBUG, "SEST", "CallbackTCPReadData_s:%x,%d,iLen[%d],iSocketIndex[%d],aesFlag[%d]\n", pHeadTemp->iMsgType, pHeadTemp->cVersion, iLen, iSocketIndex, iAesFlag);
	//和room服务器等交互无需加密
	memset(m_szDecBuffer, 0, sizeof(m_szDecBuffer));
	m_iDecBufferLen = 0;
	if (iAesFlag == NO_AES_PACKET)
	{
		memcpy(m_szDecBuffer, szMsg, iLen);
		m_iDecBufferLen = iLen;
	}
	else
	{
		int rtn = aes_dec_r(szMsg, iLen, m_szPasswd, strlen(m_szPasswd), m_szDecBuffer, &m_iDecBufferLen);
		if (rtn == -1)
		{
			_log(_ERROR, "SEST", "CallbackTCPReadData: Decryption failed!\n");
			return true;
		}
	}
	MsgHeadDef *pHead = (MsgHeadDef*)m_szDecBuffer;
	//_log(_DEBUG, "SEST", "CallbackTCPReadData:%x,%d,iLen[%d],iSocketIndex[%d],aesFlag[%d]\n", pHead->iMsgType,pHead->cVersion,iLen,iSocketIndex, iAesFlag);
	if (pHead->iMsgType == KEEP_ALIVE_MSG)
	{
		return true;
	}
	if (pHead->iMsgType == GAME_AUTHEN_REQ_RADIUS_MSG)
	{
		m_bServerAuthenOK = true;
	}
	if (m_pGetQueue != NULL)
	{
		pHead->cMsgFromType = m_iThreadType;
		pHead->iSocketIndex = iSocketIndex;
		m_pGetQueue->EnQueue(&m_szDecBuffer, m_iDecBufferLen);
	}
	return true;
}

void ServerEpollSockThread::SendServerAuthenMsg()
{
	if (m_iThreadType == BASE_SEVER_TYPE_ROOM)//连接room服务器
	{
		//发送给room服务器
		GameRDAuthenSimpleReqDef msgAuthenReq;
		memset(&msgAuthenReq, 0, sizeof(GameRDAuthenSimpleReqDef));
		msgAuthenReq.msgPre.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgAuthenReq.msgPre.msgHeadInfo.iMsgType = GAME_AUTHEN_REQ_RADIUS_MSG;
		msgAuthenReq.msgPre.iReqType = 0;
		msgAuthenReq.msgPre.iServerID = GameLogic::m_pServerBaseInfo->iServerID;
		msgAuthenReq.iAppTimeStamp = GameLogic::m_pServerBaseInfo->iAppMTime;
		msgAuthenReq.iStartTimeStamp = GameLogic::m_pServerBaseInfo->iStartTimeStamp;
		SendRadiusMsg(&msgAuthenReq, sizeof(GameRDAuthenSimpleReqDef));
	}
	else
	{
		//发送给其他服务器
		GameRDAuthenReqDef msgAuthenReq;
		memset(&msgAuthenReq, 0, sizeof(GameRDAuthenReqDef));
		msgAuthenReq.msgPre.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgAuthenReq.msgPre.msgHeadInfo.iMsgType = GAME_AUTHEN_REQ_RADIUS_MSG;
		msgAuthenReq.msgPre.iReqType = 2;
		msgAuthenReq.msgPre.iServerID = GameLogic::m_pServerBaseInfo->iServerID;
		msgAuthenReq.iGameID = GameLogic::m_pServerBaseInfo->iGameID;
		strcpy(msgAuthenReq.szGameName, GameLogic::m_pServerBaseInfo->szGameName);
		strcpy(msgAuthenReq.szServerName, GameLogic::m_pServerBaseInfo->szServerName);
		SendRadiusMsg(&msgAuthenReq, sizeof(GameRDAuthenReqDef));

		_log(_ERROR, "SEST", "SendServerAuthenMsg baseserver threadtype[%d]", m_iThreadType);
	}
}

int ServerEpollSockThread::Run()
{
	char *szDequeueBuffer = new char[m_pSendQueue->max_msg_len];
	int iDequeueLen = 0;

	time_t tmLast = time(NULL);
	time_t tmNow = tmLast;
	static int iCount = 0;
	while (1)
	{
		if (m_iSocketIndex != -1)
		{
			ReadSocketNodeData();
			tmNow = time(NULL);
			for (int i = 0; i < 10; i++)
			{
				iDequeueLen = m_pSendQueue->DeQueue((void*)szDequeueBuffer, m_pSendQueue->max_msg_len, 0);
				if (iDequeueLen > 0)
				{
					MsgHeadDef* pMsgHead = (MsgHeadDef*)(szDequeueBuffer);
					if (pMsgHead->iMsgType == GAME_SERVER_EPOLL_RESET_IP_MSG)
					{
						GameServerEpollResetIPMsgDef* pMsg = (GameServerEpollResetIPMsgDef*)szDequeueBuffer;
						ResetSeverIPInfo(pMsg->szIP, pMsg->sServerPort, pMsg->szIPBak, pMsg->sServerPortBak);
					}
					else
					{
						SendRadiusMsg(szDequeueBuffer, iDequeueLen);
					}
				}
				else
				{
					break;
				}
			}

			if (tmNow - tmLast > 15)
			{
				tmLast = tmNow;
				KeepAliveDef msgKeepAlive;
				memset(&msgKeepAlive, 0, sizeof(KeepAliveDef));
				msgKeepAlive.msgHeadInfo.cVersion = MESSAGE_VERSION;
				msgKeepAlive.msgHeadInfo.iMsgType = KEEP_ALIVE_MSG;
				msgKeepAlive.szEmpty[0] = 1;//标记是服务器发过来的心跳信息
				SendRadiusMsg(&msgKeepAlive, sizeof(KeepAliveDef));
			}
		}
		else
		{
			if (iCount < 5) //前5次连主服务器
			{
				m_iSocketIndex = AddTCPClientNode(m_szServerIP, m_iServerPort, 0);
				if (m_iSocketIndex != -1)
				{
					m_iConnectIndex = 0;
					_log(_ERROR, "SEST", "filenaem[%s] reconncet OK--Server1[%s:%d]", m_szConfFileName, m_szServerIP, m_iServerPort);
				}
			}
			else
			{
				m_iSocketIndex = AddTCPClientNode(m_szServerIP2, m_iServerPort2, 0);
				if (m_iSocketIndex != -1)
				{
					m_iConnectIndex = 1;
					_log(_ERROR, "SEST", "filenaem[%s] reconncet OK--Server2[%s:%d]", m_szConfFileName, m_szServerIP2, m_iServerPort2);
				}
			}

			if (m_iSocketIndex != -1)
			{
				iCount = 0;
				/*if (m_bNeedConvertConnect)
				{
					m_bNeedConvertConnect = false;
				}*/

				SendServerAuthenMsg();
			}
			else
			{
#ifdef _WIN32
				Sleep(2000);
#else
				sleep(10);
#endif

				iCount++;
				if (iCount == 10)
				{
					_log(_ERROR, "SEST", "iCount = %d  reconncet [%s][%d] exit", iCount, m_szServerIP, m_iServerPort);

					exit(0);
				}
			}
		}
	}

	return 1;
}

int ServerEpollSockThread::GetSocketIndex()
{
	return m_iSocketIndex;
}

int ServerEpollSockThread::SendRadiusMsg(void *msgData, int iDataLen)
{
	if (m_iSocketIndex < 0)
		return 0;

	MsgHeadDef* msgHead = (MsgHeadDef*)msgData;

	int iRt = WriteSocketNodeData(m_iSocketIndex, (char*)msgData, iDataLen);
	if (iRt < 0)
	{
		_log(_ERROR, "SEST", "SendRadiusMsg iRt[%d], m_iThreadType[%d] iMsgType[0x%x] m_iSocketIndex[%d]", iRt, m_iThreadType, msgHead->iMsgType, m_iSocketIndex);
	}
	return iRt;
}