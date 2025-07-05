#include "ClientEpollSockThread.h"
#include "factorymessage.h"
#include "conf.h"
#include "aeslib.h"
#include "log.h"
#include "gamefactorycomm.h"

ClientEpollSockThread::ClientEpollSockThread(int iAllNodeNum, int iWriteMode, int iReadBufferLen, int iWriteBufferLen, int iAllSend)
	: TcpSock(iAllNodeNum, iWriteMode, iReadBufferLen, iWriteBufferLen)
{
	m_pGetQueue = NULL;
	m_pSendQueue = NULL;
	m_iAllSend = iAllSend;
}

ClientEpollSockThread::~ClientEpollSockThread(void)
{

}

int ClientEpollSockThread::Ini(MsgQueue *pGetQueue, MsgQueue *pSendQueue, int iServerPort, int iHeartBeatTime)
{
	m_pSendQueue = pSendQueue;
	m_pGetQueue = pGetQueue;
	return AddTCPServerNode(iServerPort, iHeartBeatTime);
}

int ClientEpollSockThread::Run()
{
	char* szDequeueBuffer = new char[m_pSendQueue->max_msg_len];
	int iDequeueLen = 0;

	char *szEncBuffer = new char[m_pSendQueue->max_msg_len + 128];
	int iEncBufferLen;
	while (1)
	{
		ReadSocketNodeData();

		for (int i = 0; i<GetNowSocketNodeNum(); i++)//读一次消息，需要写总节点数的消息次数
		{
			int iDequeueLen = 0;
			if (m_iAllSend == 1)//群发线程
			{
				iDequeueLen = m_pSendQueue->DeQueue((void*)szDequeueBuffer, m_pSendQueue->max_msg_len, 0);
				if (iDequeueLen > 0)
				{
					MsgHeadDef* pMsgHead = (MsgHeadDef*)(szDequeueBuffer);
					//_log(_DEBUG,"ClientEpollSockThread"," AllSend  iType[%d],index[%d] m_iAllSend[%d]",msgAllSendEvent.iType,msgAllSendEvent.iIndex,m_iAllSend);
					int iIndex = 0;
					int rtn = 0;
					iEncBufferLen = 0;
					iIndex = pMsgHead->iSocketIndex;//这里注意，针对无需加密和需要加密的客户端，发送群发消息的时候，需要分开处理的，新项目暂时不会有这个问题，先无视

					memset(szEncBuffer, 0, sizeof(szEncBuffer));
					TcpSocketNodeDef* node;
					node = GetSocketNode(iIndex);
					int iNoAesFlag = 0;
					if (node && node->iNoAesFlag == 1)
					{
						memcpy(szEncBuffer, szDequeueBuffer, iDequeueLen);
						iEncBufferLen = iDequeueLen;
						iNoAesFlag = 1;
					}
					else
					{
						rtn = aes_enc_r(szDequeueBuffer, iDequeueLen,
							"empty", strlen("empty"), szEncBuffer, &iEncBufferLen);
					}
					if (rtn == 0)
					{
						//_log(_DEBUG,"ClientEpollSockThread"," AllSend  iType[%d]",msgAllSendEvent.iType);
						if (pMsgHead->cFlag1 == 0)//cFlag1用于标记群发类型消息类型，0表示群发所有人 1针对指定分组(一桌/指定编号的一组)群发 2则针对指定端口单发
						{
							WriteAllSocketNode(szEncBuffer, iEncBufferLen, -1, iNoAesFlag);
						}
						else if (pMsgHead->cFlag1 == 1)//pMsgHead->iFlagID就是群发桌ID
						{
							WriteAllSocketNode(szEncBuffer, iEncBufferLen, pMsgHead->iFlagID, iNoAesFlag);
						}
						else
						{
							WriteSocketNodeData(pMsgHead->iSocketIndex, szEncBuffer, iEncBufferLen, iNoAesFlag);
						}
					}
					else
					{
						_log(_ERROR, "ClientEpollSockThread", " AllSend  aes_enc_r error id[%d]", rtn);
					}

				}
				else
				{
					break;
				}
			}
			else
			{
				iDequeueLen = m_pSendQueue->DeQueue((void*)szDequeueBuffer, m_pSendQueue->max_msg_len, 0);
				if (iDequeueLen > 0)
				{
					//_log(_DEBUG,"ClientEpollSockThread","iType[%d],index[%d] m_iAllSend[%d]",msgEvent.iType,msgEvent.iIndex,m_iAllSend);
					MsgHeadDef* pMsgHead = (MsgHeadDef*)(szDequeueBuffer);
					//_log(_DEBUG,"ClientEpollSockThread"," AllSend  iType[%d],index[%d] m_iAllSend[%d]",msgAllSendEvent.iType,msgAllSendEvent.iIndex,m_iAllSend);
					int iIndex = 0;
					int rtn = 0;
					iEncBufferLen = 0;
					iIndex = pMsgHead->iSocketIndex;

					memset(szEncBuffer, 0, sizeof(szEncBuffer));
					TcpSocketNodeDef* node;
					node = GetSocketNode(iIndex);
					if (node && node->iNoAesFlag == 1)
					{
						memcpy(szEncBuffer, szDequeueBuffer, iDequeueLen);
						iEncBufferLen = iDequeueLen;
					}
					else
					{
						rtn = aes_enc_r(szDequeueBuffer, iDequeueLen,
							"empty", strlen("empty"), szEncBuffer, &iEncBufferLen);
					}
					//_log(_DEBUG,"ClientEpollSockThread"," iType[%d],rt[%d]",msgAllSendEvent.iType,rt);
					if(rtn == 0 && node)
					{
						if (pMsgHead->cFlag1 == 0)//cFlag1用于标记群发类型消息类型，0表示针对个单个 1针对指定分组(一桌/指定编号的一组)群发 2则针对指定端口单发
						{
							WriteSocketNodeData(pMsgHead->iSocketIndex, szEncBuffer, iEncBufferLen, node->iNoAesFlag);
						}
						else if (pMsgHead->cFlag1 == 1)
						{
							WriteAllSocketNode(szEncBuffer, iEncBufferLen, pMsgHead->iFlagID, node->iNoAesFlag);
						}
					}
					else
					{
						_log(_ERROR, "ClientEpollSockThread", " aes_enc_r error id[%d]", rtn);
					}

				}
				else
				{
					break;
				}
			}

		}
	}


	return 1;
}

void ClientEpollSockThread::CallbackAddSocketNode(int iSocketIndex)
{
	//_log(_DEBUG, "CallbackAddSocketNode", "thread[%s],iSocketIndex[%d]", m_szTcpSockName,iSocketIndex);
	if (m_iAllSend == 1)//群发线程
	{
		SetNodeTableID(iSocketIndex, 0);//为了不同客户接收不同的公告编码
	}

}

void ClientEpollSockThread::CallbackDelSocketNode(int iSocketIndex, int iDisType, int iProxy)
{
	//_log(_ERROR,"CallbackDelSocketNode"," Test index[%d] [%d][%d]",iSocketIndex,m_iAllSend,iDisType);

	TcpSocketNodeDef* pNode = GetSocketNode(iSocketIndex);

	if (pNode == NULL)
	{
		_log(_ERROR, "CEST", "CallbackDelSocketNode: %d dis type %d,MaxIndex %d,sock %d", iSocketIndex, iDisType, m_iMaxNodeIndex, m_pSokcetNode[iSocketIndex].socket);
		return;
	}

	if (pNode->iRecPackNum == 0)//最少有收到过合法的包才回调
	{
		return;
	}

	ServerComInnerMsgDef msgInner;
	memset(&msgInner, 0, sizeof(ServerComInnerMsgDef));
	msgInner.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgInner.msgHeadInfo.iMsgType = SERVER_COM_INNER_MSG;
	msgInner.msgHeadInfo.iSocketIndex = iSocketIndex;
	msgInner.msgHeadInfo.cMsgFromType = (m_iAllSend == 0 ? CLIENT_EPOLL_THREAD : CLIENT_ALL_EPOLL_THREAD);
	msgInner.iExtraMsgType = 999;//用999标记玩家客户端断开连接
	msgInner.msgHeadInfo.iFlagID = pNode->iUserID;

	m_pGetQueue->EnQueue((char*)&msgInner, sizeof(ServerComInnerMsgDef));
}

bool ClientEpollSockThread::CallbackTCPReadData(int iSocketIndex, char *szMsg, int iLen, int iAesFlag)
{
	TcpSocketNodeDef* node;
	node = GetSocketNode(iSocketIndex);
	node->iNoAesFlag = node->iFlag1; //是否不加密判断,
	//node->iNoAesFlag = node->szReadBuffer[1];//是否不加密判断,szReadBuffer用来做粘包处理，这里不能用
	//_log(_ERROR, "ClientEpollSockThread", "iSocketIdx[%d], iNoAesFlag[%d] iAesFlag[%d] iLen[%d]", iSocketIndex, node->iNoAesFlag, iAesFlag, iLen);

	int rtn = 0;
	memset(m_szDecBuffer, 0, sizeof(m_szDecBuffer));
	m_iDecBufferLen = 0;
	if (node && node->iNoAesFlag == 1)
	{
		m_iDecBufferLen = iLen;
		memcpy(m_szDecBuffer, szMsg, iLen);
	}
	else
	{
		rtn = aes_dec_r(szMsg, iLen,
			"empty", strlen("empty"), m_szDecBuffer, &m_iDecBufferLen);
	}
	if (rtn == -1)
	{
		_log(_ERROR, "ClientEpollSockThread", "CallbackTCPReadData:[%d:%d] Decryption failed!\n", iSocketIndex, node->iUserID);
		return true;
	}

	MsgHeadDef *pHead = (MsgHeadDef*)m_szDecBuffer;

	if (pHead->cVersion != MESSAGE_VERSION)
	{
		_log(_ERROR, "CEST", "index[%d:%d] msgVersion error[%d] type[%d] iheadSocketIndex[%d] cMsgFromType[%d] iFlagID[%d]",
			iSocketIndex, node->iUserID, pHead->cVersion, pHead->iMsgType, pHead->iSocketIndex, pHead->cMsgFromType, pHead->iFlagID);
		return false;
	}

	if (pHead->iMsgType == AUTHEN_REQ_MSG)
	{
		char UserAddr[20];
		int UserPort;
		struct sockaddr_in SocketAdd;
		//取得用户IP地址 IVAN 6.19
		memset(&SocketAdd, 0, sizeof(struct sockaddr_in));
		socklen_t SocketAddrLen = sizeof(SocketAdd);
		getpeername(node->socket, (struct sockaddr*)&SocketAdd, &SocketAddrLen);
		strncpy(UserAddr, inet_ntoa(SocketAdd.sin_addr), sizeof(UserAddr));
		UserPort = ntohs(SocketAdd.sin_port);

		AuthenReqDef *msg = (AuthenReqDef*)m_szDecBuffer;
		memcpy(msg->szIP, UserAddr, 20);

		msg->msgHeadInfo.iSocketIndex = iSocketIndex;
		msg->msgHeadInfo.cMsgFromType = m_iAllSend == 0 ? CLIENT_EPOLL_THREAD : CLIENT_ALL_EPOLL_THREAD;

		node->iUserID = msg->iUserID;
		m_pGetQueue->EnQueue(m_szDecBuffer, m_iDecBufferLen);
	}
	else if (pHead->iMsgType != KEEP_ALIVE_MSG)
	{
		MsgHeadDef* msgHead = (MsgHeadDef*)m_szDecBuffer;
		msgHead->iSocketIndex = iSocketIndex;
		msgHead->cMsgFromType = m_iAllSend == 0 ? CLIENT_EPOLL_THREAD : CLIENT_ALL_EPOLL_THREAD;

		m_pGetQueue->EnQueue(m_szDecBuffer, m_iDecBufferLen);
	}

	return true;
}

void ClientEpollSockThread::CallbackParseDataError(int iSocketIndex, char *szBuffer, int iLen, int iPos)
{
	//time_t tmNow = time(NULL);
	//TcpSocketNodeDef* pNode = GetSocketNode(iSocketIndex);
	//_log(_ERROR,"CES","PRB-[%s,%d,%s:%d]VE-[%d,%d,%d,%d,%d],[%d,%d,%d]",m_szTcpSockName,m_iNowSocketNodeNum,pNode->szPeerIP,pNode->iPeerPort,iSocketIndex,iPos,szBuffer[iPos],iLen,pNode->iReadPos,pNode->iUserID,tmNow-pNode->tmCreateTime,pNode->iRecPackNum);
}