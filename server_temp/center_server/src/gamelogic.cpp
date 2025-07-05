//系统头文件
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>      //sleep()
#include <stdlib.h>      //rand()
#include <iconv.h>
#include <sys/time.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <iostream>

//公共模块头文件
#include "log.h"
#include "conf.h"
#include "main_ini.h"

//项目相关头文件
#include "gamefactorycomm.h"
#include "ClientEpollSockThread.h"
#include "ServerEpollSockThread.h"
#include "GlobalMethod.h"
#include <fstream>

ServerBaseInfoDef* GameLogic::m_pServerBaseInfo = NULL;

//填充消息头宏定义
#define __FILL_MSG_HEAD(_MsgTypeDef, _MsgType, _pMsg)	\
	memset(_pMsg, 0, sizeof(MsgHeadDef));				\
	_pMsg->msgHeadInfo.cVersion = MESSAGE_VERSION;		\
	_pMsg->msgHeadInfo.iMsgType = _MsgType;

GameLogic::GameLogic()
{
	_log(_ERROR, "GL", "******************* mj_game_factory  v1.0 ***************************");

	m_iUpdateCenterServerPlayerNum = 0;
	m_iSendZeroOnlineCnt = 0;
	m_bIfStartKickOutTime = false;
	m_bCheckTableRecMsg = true;
	m_tmLastCheckKick = 0;

	memset(m_PlayerInfoDesk, 0, sizeof(m_PlayerInfoDesk));
	m_iPlayerInfoDeskNum = 0;

	srand(time(NULL));

	m_iLastSendTbNum = 0;
	m_iUseringTabNum = 0;
	m_iTimerHalfHourCnt = 0;
	m_iTimerFiveMinCnt = 0;
	m_iTimerTenMinCnt = 0;
	m_iFifteenMinCnt = 0;
	m_iOneMinCnt = 0;
	m_iTimerHourCnt = 0;
	m_iTimerOneDayCDCnt = -1;//用-1初始化
	m_iTimerTenSecCnt = 0;

	m_pQueToClient = NULL;
	m_pQueToAllClient = NULL;

	m_pQueToRadius = NULL;
	m_pQueToLog = NULL;
	m_pQueToCenterServer = NULL;
	m_pQueToBull = NULL;
	m_pQueToAct = NULL;
	m_pQueToRedis = NULL;
	m_pQueToRoom = NULL;
	m_pQueToOther1 = NULL;
	m_pQueToOther2 = NULL;

	m_pClientEpoll = NULL;
	m_pClientAllEpoll = NULL;

	m_pRadiusEpoll = NULL;
	m_pLogEpoll = NULL;
	m_pCenterServerEpoll = NULL;
	m_pBullEpoll = NULL;
	m_pActEpoll = NULL;
	m_pRedisEpoll = NULL;
	m_pRoomEpoll = NULL;
	m_pOther1Epoll = NULL;
	m_pOther2Epoll = NULL;

	m_pNewAssignTableLogic = NULL;

	m_pTaskLogic = NULL;

	m_pGameEvent = new GameEvent();

	vector<int>vcControlAIID;
	for (int i = g_iMinControlVID; i <= g_iMaxControlVID; i++)
	{
		vcControlAIID.push_back(i);
	}
	while (!vcControlAIID.empty())
	{
		int iRandIndex = rand() % vcControlAIID.size();//初试虚拟AI打乱用
		m_vcControlAIID.push_back(vcControlAIID[iRandIndex]);
		vcControlAIID.erase(vcControlAIID.begin() + iRandIndex);
	}
}

GameLogic::~GameLogic()
{
	//停止线程(只停止自己创建的)
	if (m_pRadiusEpoll)
	{
		m_pRadiusEpoll->Stop();
		m_pRadiusEpoll = NULL;
	}
	if (m_pLogEpoll)
	{
		m_pLogEpoll->Stop();
		m_pLogEpoll = NULL;
	}
	if (m_pCenterServerEpoll)
	{
		m_pCenterServerEpoll->Stop();
		m_pCenterServerEpoll = NULL;
	}
	if (m_pBullEpoll)
	{
		m_pBullEpoll->Stop();
		m_pBullEpoll = NULL;
	}
	if (m_pActEpoll)
	{
		m_pActEpoll->Stop();
		m_pActEpoll = NULL;
	}
	if (m_pRedisEpoll)
	{
		m_pRedisEpoll->Stop();
		m_pRedisEpoll = NULL;
	}
	if (m_pOther1Epoll)
	{
		m_pOther1Epoll->Stop();
		m_pOther1Epoll = NULL;
	}
	if (m_pOther2Epoll)
	{
		m_pOther2Epoll->Stop();
		m_pOther2Epoll = NULL;
	}

	//释放空间
	if (m_pQueToRadius)
	{
		delete m_pQueToRadius;
		m_pQueToRadius = NULL;
	}
	if (m_pQueToLog)
	{
		delete m_pQueToLog;
		m_pQueToLog = NULL;
	}
	if (m_pQueToCenterServer)
	{
		delete m_pQueToCenterServer;
		m_pQueToCenterServer = NULL;
	}
	if (m_pQueToBull)
	{
		delete m_pQueToBull;
		m_pQueToBull = NULL;
	}
	if (m_pQueToAct)
	{
		delete m_pQueToAct;
		m_pQueToAct = NULL;
	}
	if (m_pQueToRedis)
	{
		delete m_pQueToRedis;
		m_pQueToRedis = NULL;
	}
	if (m_pQueToOther1)
	{
		delete m_pQueToOther1;
		m_pQueToOther1 = NULL;
	}
	if (m_pQueToOther2)
	{
		delete m_pQueToOther2;
		m_pQueToOther2 = NULL;
	}
	if (m_pGameEvent)
	{
		delete m_pGameEvent;
		m_pGameEvent = NULL;
	}
}

int GameLogic::Initialize(CMainIni* pMainIni, ServerBaseInfoDef* pServerBaseInfo)
{
	m_pServerBaseInfo = pServerBaseInfo;
	m_pQueToClient = pMainIni->m_pQueToClient;
	m_pQueToAllClient = pMainIni->m_pQueToAllClient;
	m_pQueToRoom = pMainIni->m_pQueToRoom;
	m_pQueLogic = pMainIni->m_pQueLogic;

	m_pClientEpoll = pMainIni->m_pClientEpoll;
	m_pClientAllEpoll = pMainIni->m_pClientAllEpoll;
	m_pRoomEpoll = pMainIni->m_pRoomEpoll;

	_log(_ERROR, "GL", "Initialize***********************************");
	return 0;
}

void GameLogic::SendLeftTabNum(int iAdd, bool bMastSend)
{
	m_iUseringTabNum += iAdd;
	if (bMastSend || abs(m_iUseringTabNum - m_iLastSendTbNum) >= 5 || (m_iUseringTabNum >= 0 && m_iUseringTabNum < 5) || m_iUseringTabNum > MAX_TABLE_NUM - 5)
	{
		NCenterTabNumMsgDef msgReq;
		memset(&msgReq, 0, sizeof(msgReq));
		msgReq.iCurNum = m_iUseringTabNum;
		msgReq.iMaxNum = MAX_TABLE_NUM - 20;
		msgReq.iServerID = m_pServerBaseInfo->iServerID;
		msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgReq.msgHeadInfo.iMsgType = NEW_CENTER_TABLE_NUM_MSG;
		if (m_pQueToCenterServer)
		{
			m_pQueToCenterServer->EnQueue(&msgReq, sizeof(NCenterTabNumMsgDef));
		}
		m_iLastSendTbNum = m_iUseringTabNum;
	}
}

int GameLogic::OnEvent(void *pMsg, int iLength)
{
	time_t timeStart;
	time(&timeStart);

	FactoryPlayerNodeDef *nodePlayers = NULL;
	int iMsgType = 0;
	int iStatusCode = 0;
	int iUserID = 0;
	char *pMessage = NULL;
	int iUserIDTemp;

	MsgHeadDef *pHead = (MsgHeadDef *)pMsg;
	pMessage = (char*)pMsg;

	if (pHead->cVersion != MESSAGE_VERSION)
	{
		if (pHead->cMsgFromType == CLIENT_EPOLL_THREAD || pHead->cMsgFromType == CLIENT_ALL_EPOLL_THREAD)
		{
			nodePlayers = (FactoryPlayerNodeDef *)(hashIndexTable.Find((void *)(&(pHead->iSocketIndex)), sizeof(int)));
			if (nodePlayers)
			{
				_log(_ERROR, "C", "ErrorCheck150[%d][%d][%s][%d][%d][%d] V[%x]T[%x]", pHead->iSocketIndex, nodePlayers->iUserID, nodePlayers->szUserName, nodePlayers->cTableNum, nodePlayers->cTableNumExtra, nodePlayers->iStatus, pHead->cVersion, pHead->iMsgType);
			}
			else
			{
				_log(_ERROR, "C", "ErrorCheck151[%d] V[%x]T[%x]", pHead->iSocketIndex, pHead->cVersion, pHead->iMsgType);
			}
		}
		else
		{
			_log(_ERROR, "GL", "ErrorCheck152[%d] V[%x]T[%x]", pHead->iSocketIndex, pHead->cVersion, pHead->iMsgType);
		}
		return -1;
	}
	static time_t tmLastLog = 0;

	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	long long tm1 = (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;

	iMsgType = pHead->iMsgType;
	//在这里判断消息来自服务器还是客户端，处理是不太一样的
	//_log(_DEBUG, "GL", "OnEvent iMsgType[0x%x] cMsgFromType[%d]", iMsgType, pHead->cMsgFromType);
	switch (pHead->cMsgFromType)
	{
		case CLIENT_EPOLL_THREAD:       //来自ClientStack
		{
			bool bHaveNode = true;
			if (pHead->iMsgType == AUTHEN_REQ_MSG)
			{
				AuthenReqDef* pMsgReq = (AuthenReqDef*)pMessage;
				iUserIDTemp = pMsgReq->iUserID;

				FactoryPlayerNodeDef *nodeTemp = NULL;
				nodeTemp = (FactoryPlayerNodeDef*)(hashIndexTable.Find((void *)(&(pHead->iSocketIndex)), sizeof(int)));//socket被重复分配
				if (nodeTemp != NULL)
				{
					_log(_ERROR, "GL", "OnEvent:ErrorCheck1 iIndex[%d]Exsit,ID[%d][%d][%s]iStatus[%d],table[%d][%d]", pHead->iSocketIndex, iUserIDTemp, nodeTemp->iUserID, nodeTemp->szUserName, nodeTemp->iStatus, nodeTemp->cTableNum, nodeTemp->cTableNumExtra);
					if (nodeTemp->iStatus == PS_WAIT_READY && nodeTemp->cTableNum == 0)
					{
						nodeTemp->iStatus = PS_WAIT_DESK;
					}
					if (nodeTemp->iUserID != iUserIDTemp)
					{						
						if (nodeTemp->iStatus == PS_WAIT_USERINFO)
						{
							OnUserInfoDisconnect(nodeTemp->iUserID, NULL);
						}
						else if (nodeTemp->cTableNum > 0)
						{
							if (nodeTemp->iStatus == PS_WAIT_READY)
							{
								OnReadyDisconnect(nodeTemp->iUserID, NULL);
							}
							else
							{
								//这里游戏状态下发这个验证请求过来的话不理他好了add by crystal del function 2009.1.23
								return 0;
							}
						}
						else
						{
							OnFindDeskDisconnect(nodeTemp->iUserID, NULL);
						}
					}
					else
					{
						//是同一个ID发来的话又是同一个Index,不理它
						return 0;
					}
				}
				nodePlayers = (FactoryPlayerNodeDef *)hashUserIDTable.Find((void *)&(iUserIDTemp), sizeof(int));
				if (nodePlayers == 0)   //节点不存在
				{
					//_log(_DEBUG,"GL","OnEvent:AUTHEN_REQ_MSG UserID[%d]  no nodelogin",iUserIDTemp);
					iStatusCode = 0;
				}
				else  //节点存在
				{
					_log(_ERROR, "GL", "OnEvent: ID[%d][%s] yes nodelogin[%d],table[%d][%d]", iUserIDTemp, nodePlayers->szUserName, nodePlayers->iStatus, nodePlayers->cTableNum, nodePlayers->cTableNumExtra);
					char cFlag = 1;
					int iOldSocketIndex = nodePlayers->iSocketIndex;
					bool bNeedKickOldSocket = false;
					bool bNeedReturn = false;
					if (nodePlayers->iStatus == PS_WAIT_READY && nodePlayers->cTableNum == 0)
					{
						nodePlayers->iStatus = PS_WAIT_DESK;
					}
					if (nodePlayers->iStatus == PS_WAIT_USERINFO)//等待登陆回应
					{
						OnUserInfoDisconnect(iUserIDTemp, NULL);//该接口内会从2个hash内去掉原用户节点，并释放掉原用户节点，相当于用户重新走完整登录流程
						bNeedKickOldSocket = true;
					}
					else if (nodePlayers->iStatus == PS_WAIT_DESK)
					{
						OnFindDeskDisconnect(iUserIDTemp, NULL);
					}	
					else if (nodePlayers->cTableNum > 0 || nodePlayers->iStatus >= PS_WAIT_READY)
					{		
						if (IfKickFormerGameAgainLogin())//需要踢掉之前的socket
						{
							hashIndexTable.Remove((void*)&(iOldSocketIndex), sizeof(int));
							//加入socketIndex哈系表
							nodePlayers->iSocketIndex = pHead->iSocketIndex;
							hashIndexTable.Add((void*)(&(nodePlayers->iSocketIndex)), sizeof(int), nodePlayers);
							bNeedKickOldSocket = true;
						}
						if (nodePlayers->cTableNum == 0  && nodePlayers->iStatus == PS_WAIT_READY)//普通房间下局等待准备的用户掉线重入，回复登录回应，并重新配桌
						{
							HandleAgainLoginReq(nodePlayers->iUserID, bNeedKickOldSocket ? -1 : iOldSocketIndex);//补发用户信息
							int iVTableID = nodePlayers->iVTableID;
							if (m_pNewAssignTableLogic)
							{
								m_pNewAssignTableLogic->CallBackUserLeave(nodePlayers->iUserID, iVTableID);
							}						
							nodePlayers->iStatus = PS_WAIT_DESK;
							bNeedReturn = true;
						}											
						else  //掉线重入
						{					
							time(&timeStart);
							nodePlayers->tmTestLastRecMsg = timeStart;
							memcpy(nodePlayers->szUserToken, pMsgReq->szUserToken, 32);
							memcpy(nodePlayers->szIP, pMsgReq->szIP, 20);
							memcpy(nodePlayers->szMac, pMsgReq->szMac, 40);
							nodePlayers->cLoginType = pMsgReq->cLoginType;
							nodePlayers->iNowVerType = pMsgReq->iNowClientVer;
							nodePlayers->iAgentID = pMsgReq->iAgentID;

							_log(_ERROR, "GL", "OnEvent: HandleAgainLoginReq ID=[%d][%s],status[%d],table[%d][%d]", iUserIDTemp, nodePlayers->szUserName,
								nodePlayers->iStatus, nodePlayers->cTableNum, nodePlayers->cTableNumExtra);

							if (nodePlayers->cRoomNum > 0 && nodePlayers->cTableNum > 0)
							{
								FactoryTableItemDef* pTableItem = GetTableItem(nodePlayers->cRoomNum - 1, nodePlayers->cTableNum - 1);
								if (pTableItem)
								{
									if (pTableItem->iFirstDisPlayer == nodePlayers->cTableNumExtra)
										pTableItem->iFirstDisPlayer = -1;
								}
							}
							HandleAgainLoginReq(iUserIDTemp, bNeedKickOldSocket ? -1 : iOldSocketIndex);
							//再找次节点，可能不需要断线重入的游戏对HandleAgainLoginReq不处理反而删节点的话，就需要重新认证了..add by crystal
							FactoryPlayerNodeDef *node2 = (FactoryPlayerNodeDef *)hashUserIDTable.Find((void *)&(iUserIDTemp), sizeof(int));
							if (node2 != NULL)
							{
								bNeedReturn = true;
							}
							else
							{
								_log(_DEBUG, "GL", "OnEvent: HandleAgainLoginReq ID=[%d] can't loginAgain", iUserIDTemp);
							}
						}						
					}
					else
					{
						_log(_ERROR, "GL", "OnEvent:ErrorCheck final ID[%d][%s] yes nodelogin[%d],table[%d][%d]", iUserIDTemp, nodePlayers->szUserName, nodePlayers->iStatus, nodePlayers->cTableNum, nodePlayers->cTableNumExtra);
					}
					if (bNeedKickOldSocket)
					{
						KickOutServerDef msgKO;
						memset(&msgKO, 0, sizeof(KickOutServerDef));
						msgKO.cKickType = g_iRepeatLoginError;//你被另一个玩家踢掉
						msgKO.iKickUserID = iUserIDTemp;
						msgKO.cClueType = 1;//提示类型（0仅提示，1退出房间，2退出游戏返回大厅）
						CLSendSimpleNetMessage(iOldSocketIndex, &msgKO, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
					}
					if (bNeedReturn)
					{
						return 0;
					}
					iStatusCode = 0;
				}
				iUserID = pHead->iSocketIndex;     //HandleAuthenReq()的第1个参数为iSocketIndex不是iUserID,特殊
				if (iStatusCode == 0)
				{
					HandleGameNetMessage(iMsgType, iStatusCode, iUserID, pMessage);
				}
				break;
			}
			else if (pHead->iMsgType == SERVER_COM_INNER_MSG)
			{
				ServerComInnerMsgDef* pMsgReq = (ServerComInnerMsgDef*)pMessage;
				if (pMsgReq->iExtraMsgType == 999)//用999标记玩家客户端断开连接)
				{
					nodePlayers = (FactoryPlayerNodeDef*)(hashIndexTable.Find((void*)(&(pHead->iSocketIndex)), sizeof(int)));
					if (!nodePlayers)
					{
						iStatusCode = 0;
						iUserID = 0;
						pMessage = NULL;
						return 0;
					}

					if (nodePlayers->iStatus > PS_WAIT_READY || nodePlayers->cDisconnectType == 1)
					{
						_log(_DEBUG, "OnEvent", "DISCONNECT_MSG TEST UserName=[%s]", nodePlayers->szUserName);
					}

					_log(_DEBUG, "GL", "OnEvent:check Disconnect,socket[%d][%d][%s]status[%d] table[%d][%d]", pMsgReq->msgHeadInfo.iSocketIndex,nodePlayers->iUserID, nodePlayers->szUserName, nodePlayers->iStatus, nodePlayers->cTableNum, nodePlayers->cTableNumExtra);
					if (nodePlayers->iStatus == PS_WAIT_READY && nodePlayers->cTableNum == 0)
					{
						nodePlayers->iStatus = PS_WAIT_DESK;
					}
					//出牌状态掉线允许等待重入续玩
					if (nodePlayers->cTableNum != 0 && nodePlayers->iStatus >= PS_WAIT_READY)
					{
						nodePlayers->iWaitLoginTime = 10;

						nodePlayers->bIfWaitLoginTime = true;

						hashIndexTable.Remove((void*)&(nodePlayers->iSocketIndex), sizeof(int));
						nodePlayers->iSocketIndex = -1;
						time(&(nodePlayers->iLeaveGameTime));//获取掉线时间
						bHaveNode = false;					
					}
					nodePlayers->iAllSocketIndex = -1;
					iStatusCode = nodePlayers->iStatus;
					iUserID = nodePlayers->iUserID;
				}
			}
			if (bHaveNode)
			{
				//其他消息
				nodePlayers = (FactoryPlayerNodeDef *)(hashIndexTable.Find((void *)(&(pHead->iSocketIndex)), sizeof(int)));
				if (!nodePlayers)
				{
					//这里就是未发过AUTHEN消息，发来了其他的消息应该都是非法的.可能是外挂
					_log(_ERROR, "GL", "OnEvent: ErrorCheck3 iIndex[%d]MsgType[%x] No HASHINDEX", pHead->iSocketIndex, iMsgType);
					m_pClientEpoll->SetKillFlag(pHead->iSocketIndex, true);
					return -4;
				}

				iStatusCode = nodePlayers->iStatus;
				iUserID = nodePlayers->iUserID;
			}
		
			//这里多加个判断
			FactoryPlayerNodeDef *nodeJudge = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
			if (nodeJudge)
			{
				if (nodeJudge->iUserID != iUserID || nodeJudge->iStatus != iStatusCode || nodeJudge->iSocketIndex != pHead->iSocketIndex)
				{
					_log(_ERROR, "GL", "OnEvent: ErrorCheck10 msg[%x]id[%d][%d]status[%d][%d]Index[%d][%d]", iMsgType, iUserID, nodeJudge->iUserID, iStatusCode, nodeJudge->iStatus, pHead->iSocketIndex, nodeJudge->iSocketIndex);
				}
				if (TEST_NET_MSG != iMsgType)
				{
					time(&timeStart);
					nodeJudge->tmTestLastRecMsg = timeStart;
					if (nodeJudge->cTableNum > 0)
					{
						FactoryTableItemDef* pTableItem = GetTableItem(nodePlayers->cRoomNum - 1, nodePlayers->cTableNum - 1);
						if (pTableItem)
						{
							pTableItem->tmLastRecUserMsg = timeStart;
						}
					}
				}
			}
			HandleGameNetMessage(iMsgType, iStatusCode, iUserID, pMessage);
			break;
		}
		case BASE_SEVER_TYPE_RADIUS:       //来自Radius
		{
			HandleRadiusServerMsg((char*)pMsg, iLength);
			break;
		}
		case CLIENT_ALL_EPOLL_THREAD:     //群发线程
		{
			if (pHead->iMsgType == AUTHEN_REQ_MSG)
			{
				int iUserIDTemp = ntohl(((AuthenReqDef *)pMessage)->iUserID);

				FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)hashUserIDTable.Find((void *)&(iUserIDTemp), sizeof(int));
				if (nodePlayers)
				{
					nodePlayers->iAllSocketIndex = pHead->iSocketIndex;
					_log(_DEBUG, "OnEvent", " UserName=[%s] iAllSocketIndex[%d]", nodePlayers->szUserName, nodePlayers->iAllSocketIndex);
				}
			}
			else if (pHead->iMsgType == SERVER_COM_INNER_MSG)
			{
				ServerComInnerMsgDef* pMsgReq = (ServerComInnerMsgDef*)pMessage;
				if (pMsgReq->iExtraMsgType == 999)//用999标记玩家客户端断开连接)
				{
					int iUserIDTemp = pMsgReq->msgHeadInfo.iFlagID;

					FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)hashUserIDTable.Find((void*)&(iUserIDTemp), sizeof(int));
					if (nodePlayers)
					{
						if (nodePlayers->iSocketIndex != -1 && nodePlayers->iAllSocketIndex == pHead->iSocketIndex)
						{
							_log(_DEBUG, "GLT", "ALL_CLIENT_THREAD Disconnect %s,iAllSocketIndex=%d,iSocketInde=%d", nodePlayers->szUserName, nodePlayers->iAllSocketIndex, nodePlayers->iSocketIndex);
							nodePlayers->iAllSocketIndex = -1;
						}
					}
				}
			}
			else
			{
				//其他消息
				nodePlayers = (FactoryPlayerNodeDef *)(hashIndexTable.Find((void *)(&(pHead->iSocketIndex)), sizeof(int)));
				if (!nodePlayers)
				{
					//这里就是未发过AUTHEN消息，发来了其他的消息应该都是非法的.可能是外挂
					_log(_ERROR, "GL", "OnEvent: ErrorCheck3 iIndex[%d]MsgType[%x] No HASHINDEX", pHead->iSocketIndex, iMsgType);
					m_pClientEpoll->SetKillFlag(pHead->iSocketIndex, true);
					return -4;
				}

				iStatusCode = nodePlayers->iStatus;
				iUserID = nodePlayers->iUserID;
				HandleGameAllEpollMessage(iMsgType, iStatusCode, iUserID, pMessage);
			}
			return 0;
		}
		case BASE_SEVER_TYPE_REDIS:
		{
			HandleRedisServerMsg((char*)pMsg, iLength);
			break;
		}
		case BASE_SEVER_TYPE_BULL:
		{
			HandleBullServerMsg((char*)pMsg, iLength);
			break;
		}
		case BASE_SEVER_TYPE_ROOM:
		{
			HandleRoomServerMsg((char*)pMsg, iLength);
			break;
		}
		case BASE_SEVER_TYPE_CENTER:
		{
			HandleCenterServerMsg((char*)pMsg, iLength);
			break;
		}
		case BASE_SEVER_TYPE_LOG:
		{
			HandleLogServerMsg((char*)pMsg, iLength);
			break;
		}
		case BASE_SEVER_TYPE_OTHER1:
		{
			if (iMsgType == GAME_SYS_APPTIME_RADIUS_MSG || iMsgType == SERVER_LOG_LEVEL_SYNC_MSG)
			{
				//直接帮底层服务器转发一下至room服务器
				if (m_pQueToRoom != NULL)
				{
					m_pQueToRoom->EnQueue(pMsg, iLength);
				}
			}
			HandleOther1ServerMsg((char*)pMsg, iLength);
			break;
		}
		case BASE_SEVER_TYPE_OTHER2:
		{
			if (iMsgType == GAME_SYS_APPTIME_RADIUS_MSG || iMsgType == SERVER_LOG_LEVEL_SYNC_MSG)
			{
				//直接帮底层服务器转发一下至room服务器
				if (m_pQueToRoom != NULL)
				{
					m_pQueToRoom->EnQueue(pMsg, iLength);
				}
			}
			HandleOther2ServerMsg((char*)pMsg, iLength);
			break;
		}
		default:
		{
			break;
		}
	}

	gettimeofday(&tv, &tz);
	long long tm2 = (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;
	if (tm2 - tm1 > 250000)	//500ms
	{
		if (pHead->cMsgFromType == CLIENT_EPOLL_THREAD)
		{
			_log(_ERROR, "GL", "Run: UserID[%d] iStatusCode[%d]time=[%lld]us, MsgType=0x[%x]", iUserID, iStatusCode, tm2 - tm1, iMsgType);
		}
		else
		{
			_log(_ERROR, "GL", "Run: server[%d]time=[%lld]us, MsgType=0x[%x]", pHead->cMsgFromType, tm2 - tm1, iMsgType);
		}
		tmLastLog = timeStart;
	}
	if (pHead->cMsgFromType == CLIENT_EPOLL_THREAD)
	{
		CallBackGameNetDelay(iMsgType, iStatusCode, iUserID, pMessage, tm2 - tm1);
	}
	return 0;
}

void GameLogic::HandleRadiusServerMsg(char* pMsgData, int iMsgLen)
{
	if (CallbackFirstHandleRadiusServerMsg(pMsgData, iMsgLen))
	{
		return;
	}
	MsgHeadDef* pMsgHead = (MsgHeadDef*)pMsgData;
	//_log(_ALL, "GL", "HandleRadiusServerMsg  ---> msgType[%x]", pMsgHead->iMsgType);
	if (pMsgHead->iMsgType == GAME_USER_AUTHEN_INFO_MSG)
	{
		GameUserAuthenResPre* pMsgPre = (GameUserAuthenResPre*)pMsgData;
		//用户登录回应
		HandleRDUserInfoRes(pMsgPre->iUserID, pMsgData);
	}
	else if (pMsgHead->iMsgType == GAME_USER_ACCOUNT_MSG)
	{
		GameUserAccountResDef* pMsgRes = (GameUserAccountResDef*)pMsgData;
		HandleRDAccountRes(pMsgRes->iUserID, pMsgData);
	}
	else if (pMsgHead->iMsgType == GAME_USER_PROP_INFO_MSG)
	{
		HandleRDUserPropsRes(pMsgData);
	}
	else if (pMsgHead->iMsgType == GAME_USER_MAIN_INFO_SYS_MSG)
	{
		HandleGameRefreshUserInfoRes(pMsgData);
	}
	else if (pMsgHead->iMsgType == GAME_GET_PARAMS_MSG)
	{
		HandleGetParamsRes(pMsgData);
	}
	else if (pMsgHead->iMsgType == GAME_AUTHEN_REQ_RADIUS_MSG)
	{
		RefreshConfInfo();//这里获取一次params配置信息
	}
	else if (pMsgHead->iMsgType == GAME_SYS_APPTIME_RADIUS_MSG || pMsgHead->iMsgType ==  SERVER_LOG_LEVEL_SYNC_MSG)
	{
		//直接帮底层服务器转发一下至room服务器
		if (m_pQueToRoom != NULL)
		{
			m_pQueToRoom->EnQueue(pMsgData, iMsgLen);
		}
	}
	else if (pMsgHead->iMsgType == GAME_GET_USER_GROWUP_EVENT_MSG)
	{
		HandleGrowupEventInfo(pMsgData);
	}
	else if (pMsgHead->iMsgType == GAME_GET_FRIEND_LIST_MSG)
	{
		HandleFriendListRes(pMsgData);
	}

	//底层处理结束后，再回调一次子类
	CallbackHandleRadiusServerMsg(pMsgData, iMsgLen);
}

void GameLogic::HandleRoomServerMsg(char* pMsgData, int iMsgLen)
{
	if (CallbackFirstHandleRoomServerMsg(pMsgData, iMsgLen))
	{
		return;
	}
	MsgHeadDef* pMsgHead = (MsgHeadDef*)pMsgData;
	if (pMsgHead->iMsgType == GAME_ROOM_INFO_REQ_RADIUS_MSG)
	{
		HandleGetAllRoomInfo(pMsgData);
	}
	else if (pMsgHead->iMsgType == GAME_AUTHEN_REQ_RADIUS_MSG)
	{
		RefreshConfInfo();//这里获取一次params配置信息
	}
	else if (pMsgHead->iMsgType == SERVER_LOG_LEVEL_SYNC_MSG)
	{
		HandleLogLevelSyncMsg(pMsgData, iMsgLen);
	}
	
	//底层处理结束后，再回调一次子类
	CallbackHandleRoomServerMsg(pMsgData, iMsgLen);
}

void GameLogic::HandleRedisServerMsg(char* pMsgData, int iMsgLen)
{
	if (CallbackFirstHandleRedisServerMsg(pMsgData, iMsgLen))
	{
		return;
	}
	MsgHeadDef* pMsgHead = (MsgHeadDef*)pMsgData;	
	if (pMsgHead->iMsgType == REDIS_RETURN_USER_ROOM_MSG)
	{
		if (m_pNewAssignTableLogic)
		{
			m_pNewAssignTableLogic->CallBackHandleUserGetRoomRes(pMsgData, iMsgLen);
		}
	}
	else if (pMsgHead->iMsgType == REDIS_RETURN_USER_TASK_INFO)
	{
		if (m_pTaskLogic)
		{
			m_pTaskLogic->HandleGetRedisTaskMsg(pMsgData);
		}
	}
	else if (pMsgHead->iMsgType == REDIS_RETURN_DECORATE_INFO)
	{
		HandleRedisGetUserDecorateInfo(pMsgData);
	}
	else if (pMsgHead->iMsgType == RD_COM_TASK_SYNC_INFO)
	{
		if (m_pTaskLogic)
		{
			m_pTaskLogic->HandleGetUserRdComTask(pMsgData);
		}
	}
	else if (pMsgHead->iMsgType == GAME_SYS_APPTIME_RADIUS_MSG || pMsgHead->iMsgType == SERVER_LOG_LEVEL_SYNC_MSG)
	{
		//直接帮底层服务器转发一下至room服务器
		if (m_pQueToRoom != NULL)
		{
			m_pQueToRoom->EnQueue(pMsgData, iMsgLen);
		}
	}
	else if (pMsgHead->iMsgType == RD_GAME_CHECK_ROOMNO_MSG)
	{
		HandleCheckRoomIDMsg(pMsgData);
	}
	else if (pMsgHead->iMsgType == RD_UPDATE_USER_ACHIEVE_INFO_MSG)
	{
		HandleAchieveTaskComp(pMsgData);
	}
	else if (pMsgHead->iMsgType == RD_GAME_ROOM_TASK_INFO_MSG)
	{
		HandleUserRoomTaskRes(pMsgData);
	}
	else if (pMsgHead->iMsgType == RD_GET_INTEGRAL_TASK_CONF)
	{
		if (m_pTaskLogic)
		{
			m_pTaskLogic->HandleIntegralTaskInfo(pMsgData);
		}
	}
	else if (pMsgHead->iMsgType == RD_INTEGRAL_TASK_HIS_RES)
	{
		HandleUserRecentIntegralTask(pMsgData);
	}
	else if (pMsgHead->iMsgType == RD_USER_DAY_INFO_MSG)
	{
		HandleUserDayInfo(pMsgData);
	}
	else
	{
		if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->HandleNetMsg(pMsgHead->iMsgType, pMsgData, iMsgLen);
		}
	}
	//底层处理结束后，再回调一次子类
	CallbackHandleRedisServerMsg(pMsgData, iMsgLen);
}

void GameLogic::HandleCenterServerMsg(char* pMsgData, int iMsgLen)
{
	if (CallbackFirstHandleCenterServerMsg(pMsgData, iMsgLen))
	{
		return;
	}

	MsgHeadDef* pMsgHead = (MsgHeadDef*)pMsgData;
	if (m_pNewAssignTableLogic != NULL)
	{
		m_pNewAssignTableLogic->HandleNetMsg(pMsgHead->iMsgType, pMsgData, iMsgLen);
	}
	if (pMsgHead->iMsgType == GAME_AUTHEN_REQ_RADIUS_MSG)
	{
		SendServerInfoToCenter();
		SendLeftTabNum(0, true);
		//连接中心服务器成功，避免可能是中心服务器重启导致的重连，需要将玩家当前的已配桌和已组队信息后续在配桌请求时重新补发过去
		CallBackAfterCenterReconnect();
	}
	//底层处理结束后，再回调一次子类
	CallbackHandleCenterServerMsg(pMsgData, iMsgLen);
}

void GameLogic::HandleLogServerMsg(char* pMsgData, int iMsgLen)
{
	if (CallbackFirstHandleLogServerMsg(pMsgData, iMsgLen))
	{
		return;
	}

	MsgHeadDef* pMsgHead = (MsgHeadDef*)pMsgData;
	if (pMsgHead->iMsgType == GAME_SYS_APPTIME_RADIUS_MSG || pMsgHead->iMsgType == SERVER_LOG_LEVEL_SYNC_MSG)
	{
		//直接帮底层服务器转发一下至room服务器
		if (m_pQueToRoom != NULL)
		{
			m_pQueToRoom->EnQueue(pMsgData, iMsgLen);
		}
	}

	//底层处理结束后，再回调一次子类
	CallbackHandleLogServerMsg(pMsgData, iMsgLen);
}

void GameLogic::HandleGameNetMessage(int iMsgType, int iStatusCode, int iUserID, void* pMsgData)
{
	switch (iMsgType)
	{
	case TEST_NET_MSG:
	{
		HandleTestNetMsg(iUserID, pMsgData);
		break;
	}
	case AUTHEN_REQ_MSG:
	{
		if (iStatusCode == 0)
		{
			HandleAuthenReq(iUserID, pMsgData);
		}
		else
		{
			FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
			if (!nodePlayers)
			{
				return;
			}
			_log(_ERROR, "GL", "ErrorCheck4:ID[%d][%s]ErrorStatue[%d]", nodePlayers->iUserID, nodePlayers->szUserName, iStatusCode);
		}
		break;
	}
	case SITDOWN_REQ_MSG:
	{
		FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
		if (iStatusCode == PS_WAIT_DESK)
		{
			HandleSitDownReq(iUserID, pMsgData);
		}
		else
		{			
			if (!nodePlayers)
			{
				return;
			}
			_log(_ERROR, "GL", "ErrorCheck5:ID[%d][%s]ErrorStatue[%d]", nodePlayers->iUserID, nodePlayers->szUserName, iStatusCode);
		}
		break;
	}
	case READY_REQ_MSG:
	{
		if (iStatusCode == PS_WAIT_READY || iStatusCode == PS_WAIT_NEXTJU)
		{
			HandleReadyReq(iUserID, pMsgData);
		}
		else
		{
			FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
			if (!nodePlayers)
			{
				return;
			}
			//_log(_ERROR, "GL", "ErrorCheck6:ID[%d][%s]ErrorStatue[%d]", nodePlayers->iUserID, nodePlayers->szUserName, iStatusCode);
		}
		break;
	}
	case ESCAPE_REQ_MSG:
	{
		if (iStatusCode == PS_WAIT_READY || iStatusCode == PS_WAIT_DESK)
		{
			HandleNormalEscapeReq(iUserID, pMsgData);
		}
		else if (iStatusCode > PS_WAIT_READY)
		{
			FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
			if (!nodePlayers)
			{
				return;
			}
			HandleErrorEscapeReq(iUserID, pMsgData);
		}
		else
		{
			FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
			if (!nodePlayers)
			{
				return;
			}
			_log(_ERROR, "GL", "OnEvent:ErrorCheck9 ESCAPE_REQ_MSG id[%d][%s]Status[%d][%d][%d][%d]", nodePlayers->iUserID, nodePlayers->szUserName, nodePlayers->iStatus, nodePlayers->cRoomNum, nodePlayers->cTableNum, nodePlayers->cTableNumExtra);
			//这里应该还有未验证通过的状态了,直接回应LEAVE_REQ好了
			LeaveGameDef msg;
			memset(&msg, 0, sizeof(LeaveGameDef));
			CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msg, LEAVE_REQ_MSG, sizeof(LeaveGameDef));
		}
		break;
	}
	case SERVER_COM_INNER_MSG:
	{
		ServerComInnerMsgDef* pMsgReq = (ServerComInnerMsgDef*)pMsgData;
		if (pMsgReq->iExtraMsgType == 999)//用999标记玩家客户端断开连接)
		{
			if (iStatusCode == PS_WAIT_USERINFO)
			{
				OnUserInfoDisconnect(iUserID, pMsgData);
			}
			else if (iStatusCode == PS_WAIT_DESK)
			{
				OnFindDeskDisconnect(iUserID, pMsgData);
			}
			else if (iStatusCode == PS_WAIT_READY)
			{
				OnReadyDisconnect(iUserID, pMsgData);
			}
			else if (iStatusCode == PS_WAIT_NEXTJU)
			{
				FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
				if (!nodePlayers)
				{
					return;
				}
				AutoSendCards(iUserID);//自动出牌
				OnFindDeskDisconnect(iUserID, pMsgData);
			}
			else if (iStatusCode > PS_WAIT_READY)
			{
				FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
				if (!nodePlayers)
				{
					return;
				}
				//这里判断玩家的状态是不是合法
				if (nodePlayers->cRoomNum > 0 && nodePlayers->cTableNum > 0)
				{
					FactoryTableItemDef* pTableItem = GetTableItem(nodePlayers->cRoomNum - 1, nodePlayers->cTableNum - 1);
					if (pTableItem)
					{
						if (pTableItem->iFirstDisPlayer == -1)
							pTableItem->iFirstDisPlayer = nodePlayers->cTableNumExtra;
					}
				}
				HandlePlayStateDisconnect(iStatusCode, iUserID, pMsgData);
				if (!nodePlayers->bIfWaitLoginTime)
				{
					_log(_DEBUG, "GL", "SERVER_COM_INNER_MSG id[%d],iStatusCode[%d] AutoSendCards", iUserID, iStatusCode);
					AutoSendCards(iUserID);
				}
			}
		}
		break;
	}
	case URGR_CARD_MSG:
	{
		HandleUrgeCard(iUserID, pMsgData);
		break;
	}
	case LEAVE_REQ_MSG:
	{
		HandleLeaveReq(iUserID, pMsgData);
		break;
	}
	case USE_INTERACT_PROP:
	{
		HandleClientUseInteractPropReq(pMsgData);
	}
	case USER_CHARGE_AND_REFRESH_REQ_MSG:
	{
		HandleUserGameChargeAndRefresh(pMsgData);
		break;
	}
	case NEW_CENTER_USER_CHANGE_MSG:
	{
		NCenterUserChangeMsgDef* pMsg = (NCenterUserChangeMsgDef*)pMsgData;
		int iUserID = pMsg->iUserID[0];
		FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
		if (!nodePlayers)
		{
			_log(_ERROR, "GL", "HandleUserChangeReq  nodePlayers[%d] null", iUserID);
			return;
		}
		nodePlayers->iStatus = PS_WAIT_DESK;
		if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->HandleNetMsg(iMsgType, pMsgData);
		}
	}
	case KICK_OUT_SERVER_MSG:
	{	
		break;
	}
	case SYS_CHANGE_SERVER_MSG:
	{
		if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->HandleNetMsg(iMsgType, pMsgData);
		}		
		break;
	}
	default:
	{
		HandleOtherGameNetMessage(iMsgType, iStatusCode, iUserID, pMsgData);
		break;
	}
	}
}

void GameLogic::HandleRDUserInfoRes(int iUserID, char* pMsgData)
{
	GameUserAuthenResPre* pMsgResPre = (GameUserAuthenResPre*)pMsgData;
	_log(_ERROR, "GL", "HandleRDUserInfoRes iUserID[%d] iAuthenRt[%d]", iUserID, pMsgResPre->iAuthenRt);
	FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "GL", "HandleRDUserInfoRes nodePlayers:null uid[%d]", iUserID);
		return;
	}

	if (pMsgResPre->iAuthenRt != 0)
	{
		GameUserAuthenInfoFailResDef* pMsgFailRes = (GameUserAuthenInfoFailResDef*)pMsgData;
		if (pMsgFailRes->iResMsgType == 1)//0登录回应获取用户信息失败，1同步用户信息时获取用户信息失败
		{
			//同步用户信息时异常的话，要将用户踢出
			KickOutServerDef msgKO;
			memset(&msgKO, 0, sizeof(KickOutServerDef));
			msgKO.cKickType = pMsgResPre->iAuthenRt;
			msgKO.iKickUserID = nodePlayers->iUserID;
			msgKO.cClueType = 1;
			_log(_ERROR, "GL", "HandleRDUserInfoRes iUserID[%d] iAuthenRt[%d] iResMsgType[%d]，bLoginOk[%d]", iUserID, pMsgResPre->iAuthenRt, pMsgFailRes->iResMsgType, nodePlayers->bGetLoginOK);
			CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgKO, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
			return;
		}
	}

	memset(m_cBuffForAccount, 0, sizeof(m_cBuffForAccount));
	AuthenResDef *pMsgCL = (AuthenResDef*)m_cBuffForAccount;
	if (pMsgResPre->iAuthenRt == 0)//成功
	{
		GameUserAuthenInfoOKResDef* pMsgOKRes = (GameUserAuthenInfoOKResDef*)pMsgData;
		int iIfGapDayRes = (pMsgOKRes->iAuthenResFlag >> 1) & 0x01;
		if (nodePlayers->bGetLoginOK && iIfGapDayRes == 0)
		{
			_log(_ERROR, "GL", "HandleRDUserInfoRes iUserID[%d] has getLoginOK st[%d]", iUserID, nodePlayers->iStatus);
			return;
		}
		int iMoveIdx = sizeof(GameUserAuthenInfoOKResDef);
		iMoveIdx += pMsgOKRes->iHeadUrlLen;
		int iAchieveLevel = *(int*)(pMsgData + iMoveIdx);
		iMoveIdx += 4;
		int iHeadFrameId = *(int*)(pMsgData + iMoveIdx);
		iMoveIdx += 4;
		int iHeadFrameTm = *(int*)(pMsgData + iMoveIdx);
		iMoveIdx += 4;
		int iHisIntegral = *(int*)(pMsgData + iMoveIdx);
		iMoveIdx += 4;
		
		if (iIfGapDayRes == 0 )
		{
			nodePlayers->iStatus = PS_WAIT_DESK;//等待坐下状态
		}		
		memcpy(nodePlayers->szNickName, pMsgOKRes->szNickName, 60);
		memcpy(nodePlayers->szUserName, pMsgOKRes->szUserName, 20);

		if (strlen(nodePlayers->szNickName) == 0)
		{
			sprintf(nodePlayers->szNickName, "Guest%d", nodePlayers->iUserID);
		}
		bool bLastLoginOK = nodePlayers->bGetLoginOK;
		nodePlayers->bGetLoginOK = true;
		nodePlayers->iMoney = pMsgOKRes->iMoney;
		nodePlayers->iDiamond = pMsgOKRes->iDiamond;
		nodePlayers->iAchieveLevel = iAchieveLevel;
		nodePlayers->iHeadFrameID = iHeadFrameId;
		nodePlayers->iHeadFrameTm = iHeadFrameTm;
		nodePlayers->iHisIntegral = iHisIntegral;
		nodePlayers->cVipType = pMsgOKRes->iVipType;
		pMsgCL->iTempChargeFirstMoney = pMsgOKRes->iTempChargeFirstMoney;
		pMsgCL->iTempChargeMoney = pMsgOKRes->iTempChargeMoney;
		pMsgCL->iTempChargeDiamond = pMsgOKRes->iTempChargeDiamond;
		pMsgCL->iDiamod = pMsgOKRes->iDiamond;
		pMsgCL->iFirstMoney = pMsgOKRes->iFirstMoney;
		pMsgCL->stPlayerInfo.iMoney = pMsgOKRes->iMoney;
		nodePlayers->iSpeMark = pMsgOKRes->iSpeMark;	
		//_log(_DEBUG, "TEST", "AuthenResDef imoney[%lld] iDiamond[%d] vip[%d] spemark[%d] ahv[%d],gapRes[%d]", pMsgOKRes->iMoney, pMsgOKRes->iDiamond, nodePlayers->cVipType, nodePlayers->iSpeMark, iAchieveLevel,iIfGapDayRes);
		if (iIfGapDayRes == 0 && m_pNewAssignTableLogic != NULL)
		{
			int iInWaitIndex = m_pNewAssignTableLogic->CheckIfInWaitTable(iUserID, false);
			if (iInWaitIndex == -1)
			{
				int iKickType = JudgeKickOutServer(nodePlayers, false, true, true);
				if (iKickType > 0)
				{
					pMsgCL->cResult = iKickType;
					_log(_ERROR, "GL", "HandleRDUserInfoRes iUserID[%d] iAuthenRt[%d] iKickType[%d],money[%lld]", iUserID, pMsgResPre->iAuthenRt, iKickType, pMsgOKRes->iMoney);
				}
			}
		}
	     //_log(_DEBUG, "TEST", "AuthenRes[%d] imoney[%ld] iDiamond[%d] vip[%d] spemark[%d] ahv[%d] iInWaitIndex[%d] cresult[%d]", iUserID, pMsgOKRes->iMoney, pMsgOKRes->iDiamond, nodePlayers->cVipType, nodePlayers->iSpeMark, iAchieveLevel, iInWaitIndex, pMsgCL->cResult);
		long long  iTempChargeMoney = pMsgOKRes->iTempChargeMoney;
		int iTempChargeDiamond = pMsgOKRes->iTempChargeDiamond;
		int iTempChargeFirstMoney = pMsgOKRes->iTempChargeFirstMoney;
		if (iTempChargeMoney > 0)//补记金币流水日志
		{
			SendMainMonetaryLog(nodePlayers->iUserID, nodePlayers->szUserName, (int)MonetaryType::Money, iTempChargeMoney, nodePlayers->iMoney - iTempChargeMoney, (int)ComGameLogType::GAME_LOGIN_GET_CHARGE);
		}
		if (iTempChargeDiamond > 0)
		{
			SendMainMonetaryLog(nodePlayers->iUserID, nodePlayers->szUserName, (int)MonetaryType::Money, iTempChargeDiamond, nodePlayers->iDiamond - iTempChargeDiamond, (int)ComGameLogType::GAME_LOGIN_GET_CHARGE);
		}
		if (iTempChargeFirstMoney > 0)
		{
			SendMainMonetaryLog(nodePlayers->iUserID, nodePlayers->szUserName, (int)MonetaryType::FIRST_MONEY, iTempChargeDiamond, nodePlayers->iFirstMoney - iTempChargeFirstMoney, (int)ComGameLogType::GAME_LOGIN_GET_CHARGE);
		}
		if (pMsgCL->cResult > 0)
		{
			if (iIfGapDayRes == 0)
			{
				CLSendSimpleNetMessage(nodePlayers->iSocketIndex, pMsgCL, AUTHEN_RES_MSG, sizeof(AuthenResDef));
			}
			else
			{
				_log(_ERROR, "GL", "HandleRDUserInfoRes iUserID[%d] iAuthenRt[%d] iIfGapDayRes[%d]", iUserID, pMsgResPre->iAuthenRt, iIfGapDayRes);
			}
			return;
		}
		nodePlayers->iGender = pMsgOKRes->iGender;
		nodePlayers->iExp = pMsgOKRes->iExp;
		nodePlayers->iHeadImg = pMsgOKRes->iHeadImg;
		nodePlayers->iFirstMoney = pMsgOKRes->iFirstMoney;
		nodePlayers->iIntegral = pMsgOKRes->iIntegral;
		nodePlayers->iTotalCharge = pMsgOKRes->iTotalCharge;
		nodePlayers->iSpreadID = pMsgOKRes->iSpreadID;
		nodePlayers->tmRegisterTime = pMsgOKRes->iRegTime;
		nodePlayers->iRegType = pMsgOKRes->iRegType;
		nodePlayers->iAllGameTime = pMsgOKRes->iAllGameTime;
		//游戏信息
		nodePlayers->iGameTime = pMsgOKRes->iGameTime;
		nodePlayers->iWinNum = pMsgOKRes->iWinNum;
		nodePlayers->iLoseNum = pMsgOKRes->iLoseNum;
		nodePlayers->iAllNum = pMsgOKRes->iAllNum;
		nodePlayers->iWinMoney = pMsgOKRes->iWinMoney * 1000;
		nodePlayers->iLoseMoney = pMsgOKRes->iLoseMoney * 1000;
		nodePlayers->iGetIntegral = pMsgOKRes->iGetIntegral;
		nodePlayers->iFirstGameTime = pMsgOKRes->iFirstGameTime;
		nodePlayers->iLastGameTime = pMsgOKRes->iLastGameTime;
		nodePlayers->iGameExp = pMsgOKRes->iGameExp;
		nodePlayers->iBuffA0 = pMsgOKRes->iBuffA0;
		nodePlayers->iBuffA1 = pMsgOKRes->iBuffA1;
		nodePlayers->iBuffA2 = pMsgOKRes->iBuffA2;
		nodePlayers->iBuffA3 = pMsgOKRes->iBuffA3;
		nodePlayers->iBuffA4 = pMsgOKRes->iBuffA4;
		nodePlayers->iBuffB0 = pMsgOKRes->iBuffB0;
		nodePlayers->iBuffB1 = pMsgOKRes->iBuffB1;
		nodePlayers->iBuffB2 = pMsgOKRes->iBuffB2;
		nodePlayers->iBuffB3 = pMsgOKRes->iBuffB3;
		nodePlayers->iBuffB4 = pMsgOKRes->iBuffB4;
		nodePlayers->iContinueWin = pMsgOKRes->iContinueWin;
		nodePlayers->iContinueLose = pMsgOKRes->iContinueLose;
		nodePlayers->iDayWinNum = pMsgOKRes->iDayWinNum;
		nodePlayers->iDayLoseNum = pMsgOKRes->iDayLoseNum;
		nodePlayers->iDayAllNum = pMsgOKRes->iDayAllNum;
		nodePlayers->iDayWinMoney = pMsgOKRes->iDayWinMoney * 1000;
		nodePlayers->iDayLoseMoney = pMsgOKRes->iDayLoseMoney * 1000;
		nodePlayers->iDayIntegral = pMsgOKRes->iDayIntegral;
		nodePlayers->iRecentPlayCnt = pMsgOKRes->iRecentPlayCnt;
		nodePlayers->iRecentWinCnt = pMsgOKRes->iRecentWinCnt;
		nodePlayers->iRecentLoseCnt = pMsgOKRes->iRecentLoseCnt;
		nodePlayers->iRecentWinMoney = pMsgOKRes->iRecentWinMoney;
		nodePlayers->iRecentLoseMoney = pMsgOKRes->iRecentLoseMoney;
		if (strlen(pMsgOKRes->szNickNameThird) > 0)
		{
			sprintf(nodePlayers->szNickName,"%s", pMsgOKRes->szNickNameThird);
		}
		int iLen = pMsgOKRes->iHeadUrlLen;
		if (iLen > 0 && iLen <= 512)
		{
			memset(nodePlayers->szHeadUrlThird,0,sizeof(nodePlayers->szHeadUrlThird));
			strncpy(nodePlayers->szHeadUrlThird, (char*)pMsgOKRes+sizeof(GameUserAuthenInfoOKResDef), iLen);
		}

		int iIfGapDayLogin = pMsgOKRes->iAuthenResFlag & 0x1;
		if (iIfGapDayLogin == 1 || iIfGapDayRes == 1)
		{
			nodePlayers->bGapDaySitReq = true;
		}
		if (!bLastLoginOK)//首次登录验证
		{
			time_t tmNow = time(NULL);
			if (!iIfGapDayLogin)//radius验证时没有跨天，到服务器时已经跨天
			{
				bool bGapDay = GlobalMethod::JudgeIfGapDay(nodePlayers->iLastGameTime, tmNow);
				if (bGapDay)
				{
					nodePlayers->iLastCheckGameTm = nodePlayers->iLastGameTime;//保证下次计费时还可以判断一次跨天
				}
			}
			if (nodePlayers->iLastCheckGameTm == 0)
			{
				nodePlayers->iLastCheckGameTm = tmNow;
			}			
		}
		//游戏信息_end
		if (iIfGapDayRes == 0)
		{
			//登录回应填充
			pMsgCL->iRoomType = nodePlayers->iEnterRoomType;
			int iRoomTypeIdx = -1;
			for (int i = 0; i < 10; i++)
			{
				if (m_pServerBaseInfo->stRoom[i].iRoomType == nodePlayers->iEnterRoomType)
				{
					pMsgCL->iBasePoint = m_pServerBaseInfo->stRoom[i].iBasePoint;
					break;
				}
			}

			pMsgCL->iServerTime = time(NULL);
			pMsgCL->iDiamod = nodePlayers->iDiamond;
			pMsgCL->iTotalCharge = nodePlayers->iTotalCharge;
			pMsgCL->iPlayType = nodePlayers->iPlayType;
			pMsgCL->stPlayerInfo.iUserID = nodePlayers->iUserID;
			memcpy(pMsgCL->stPlayerInfo.szNickName, nodePlayers->szNickName, 64);
			pMsgCL->stPlayerInfo.iMoney = nodePlayers->iMoney;
			pMsgCL->stPlayerInfo.cLoginType = nodePlayers->cLoginType;
			pMsgCL->stPlayerInfo.cGender = nodePlayers->iGender;
			pMsgCL->stPlayerInfo.cVipType = nodePlayers->cVipType;
			pMsgCL->stPlayerInfo.cLoginFlag1 = nodePlayers->cLoginFlag1;
			pMsgCL->stPlayerInfo.cLoginFlag2 = nodePlayers->cLoginFlag2;
			pMsgCL->stPlayerInfo.iHeadImg = nodePlayers->iHeadImg;
			pMsgCL->stPlayerInfo.iExp = nodePlayers->iExp;
			pMsgCL->stPlayerInfo.iGameExp = nodePlayers->iGameExp;
			pMsgCL->stPlayerInfo.iWinNum = nodePlayers->iWinNum;
			pMsgCL->stPlayerInfo.iLoseNum = nodePlayers->iLoseNum;
			pMsgCL->stPlayerInfo.iAllNum = nodePlayers->iAllNum;
			pMsgCL->stPlayerInfo.iBuffA0 = nodePlayers->iBuffA0;
			pMsgCL->stPlayerInfo.iBuffA1 = nodePlayers->iBuffA1;
			pMsgCL->stPlayerInfo.iBuffA2 = nodePlayers->iBuffA2;
			pMsgCL->stPlayerInfo.iBuffA3 = nodePlayers->iBuffA3;
			pMsgCL->stPlayerInfo.iBuffA4 = nodePlayers->iBuffA4;
			pMsgCL->stPlayerInfo.iBuffB0 = nodePlayers->iBuffB0;
			pMsgCL->stPlayerInfo.iBuffB1 = nodePlayers->iBuffB1;
			pMsgCL->stPlayerInfo.iBuffB2 = nodePlayers->iBuffB2;
			pMsgCL->stPlayerInfo.iBuffB3 = nodePlayers->iBuffB3;
			pMsgCL->stPlayerInfo.iBuffB4 = nodePlayers->iBuffB4;
			pMsgCL->stPlayerInfo.iContinueWin = nodePlayers->iContinueWin;
			pMsgCL->stPlayerInfo.iContinueLose = nodePlayers->iContinueLose;
			pMsgCL->stPlayerInfo.iHeadFrameID = nodePlayers->iHeadFrameID;
			pMsgCL->stPlayerInfo.iClockPropID = nodePlayers->iClockPropID;
			pMsgCL->stPlayerInfo.iChatBubbleID = nodePlayers->iChatBubbleID;
			CallBackAuthenSuccExtra(pMsgCL);

			int iAuthResLen = sizeof(AuthenResDef);
			memcpy(pMsgCL + 1, &(nodePlayers->iIntegral), sizeof(int));
			iAuthResLen += sizeof(int);
			CLSendSimpleNetMessage(nodePlayers->iSocketIndex, pMsgCL, AUTHEN_RES_MSG, iAuthResLen);

			CallBackRDUserInfo(nodePlayers);//部分游戏要重新获取一些相关用户信息
			
			//最终登录成功后判断是否是等待其换服进来的玩家，是的话让其入座
			if (pMsgCL->cResult == 0)
			{
				if (m_pNewAssignTableLogic != NULL)
				{
					m_pNewAssignTableLogic->CheckIfNeedSitDirectly(nodePlayers->iUserID);
				}				
			}
		}		
	}
	else
	{
		GameUserAuthenInfoFailResDef* pMsgFailRes = (GameUserAuthenInfoFailResDef*)pMsgData;
		pMsgCL->cResult = pMsgResPre->iAuthenRt;
		if (pMsgCL->cResult == g_iLockInOtherServer)//卡在其他服务器了
		{
			pMsgCL->iLockServerID = pMsgFailRes->iLastServerID;
			pMsgCL->iLockGameAndRoom = pMsgFailRes->iLastGameAndRoom;
		}
		_log(_ERROR, "GL", "HandleRDUserInfoRes 1 iUserID[%d] iAuthenRt[%d] iResMsgType[%d]", iUserID, pMsgResPre->iAuthenRt, pMsgFailRes->iResMsgType);
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, pMsgCL, AUTHEN_RES_MSG, sizeof(AuthenResDef));

		int iVTableID = nodePlayers->iVTableID;
		//两个哈希表删除节点，再释放节点
		hashIndexTable.Remove((void*)&(nodePlayers->iSocketIndex), sizeof(int));
		hashUserIDTable.Remove((void*)&(nodePlayers->iUserID), sizeof(int));
		UpdateRoomNum(-1, nodePlayers->cRoomNum - 1, nodePlayers->cLoginType);
		ReleaseNode((void*)nodePlayers);
		//发送消息到中心服务更新人数
		UpdateCServerPlayerNum(-1);		
		if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->CallBackUserLeave(iUserID, iVTableID);
		}		
		return;
	}
	//防沉迷(暂无)

	//记录一些基础统计信息
	int iPlatformIndex = nodePlayers->cLoginType;

	if (iPlatformIndex < 0 || iPlatformIndex > 1)
	{
		iPlatformIndex = 1;
	}
	time_t tmNow = time(NULL);
	int iNowDateFlag = GetDayTimeFlag(tmNow);
	if (nodePlayers->iFirstGameTime == 0)
	{
		nodePlayers->iFirstGameTime = tmNow;
	}
	int iRegisterFlag = GetDayTimeFlag(nodePlayers->iFirstGameTime);
	if (nodePlayers->bGapDaySitReq == 1)//跨天
	{
		m_stGameBaseStatInfo.stPlatStatInfo[iPlatformIndex].iActiveCnt++;
		if (iNowDateFlag == iRegisterFlag)
		{
			m_stGameBaseStatInfo.stPlatStatInfo[iPlatformIndex].iNewAddCnt++;
		}
		int iGapDay = abs(iRegisterFlag - iNowDateFlag);   //新手留存数据统计
		if (iGapDay == 1)
		{
			m_stGameBaseStatInfo.stPlatStatInfo[iPlatformIndex].iNewAddContinue2++;
		}
		else if (iGapDay == 2)
		{
			m_stGameBaseStatInfo.stPlatStatInfo[iPlatformIndex].iNewAddContinue3++;
		}
		else if (iGapDay == 6)
		{
			m_stGameBaseStatInfo.stPlatStatInfo[iPlatformIndex].iNewAddContinue7++;
		}
		else if (iGapDay == 14)
		{
			m_stGameBaseStatInfo.stPlatStatInfo[iPlatformIndex].iNewAddContinue15++;
		}
		else if (iGapDay == 29)
		{
			m_stGameBaseStatInfo.stPlatStatInfo[iPlatformIndex].iNewAddContinue30++;
		}
		//_log(_DEBUG, "loginOk", "user[%d],gapDay[%d]--[%d,%d]", nodePlayers->iUserID, iGapDay, iGapDay, iRegisterFlag);
	}
	//_log(_DEBUG, "loginOk", "user[%d],loginType[%d],nextDay[%d],create[%d]", nodePlayers->iUserID, nodePlayers->cLoginType, nodePlayers->bGapDaySitReq, iRegisterFlag);
}

int GameLogic::GetDayTimeFlag(const time_t& theTime)
{
	struct tm* timenow;
	timenow = localtime(&theTime);
	int year = 1900 + timenow->tm_year;
	int month = 1 + timenow->tm_mon;
	int day = timenow->tm_mday;
	return year * 10000 + month * 100 + day;
}

int GameLogic::GetDayGap(int iDateFlag1, int iDateFlag2)
{
	time_t tm1 = GetPointDayStartTimestamp(iDateFlag1);
	time_t tm2 = GetPointDayStartTimestamp(iDateFlag2);
	int iDayGap = (tm1 - tm2) / (24 * 3600);
	//_log(_DEBUG, "gamelogic", "GetDayGap iDateFlag1 = %d,iDateFlag2=%d,tm1=%lld,tm2=%lld,iDayGap=%d", iDateFlag1, iDateFlag2, tm1, tm2, iDayGap);
	return abs(iDayGap);
}

time_t GameLogic::GetPointDayStartTimestamp(int iDateFlag)
{
	char szTime[16];
	sprintf(szTime, "%d", iDateFlag);
	char szTemp[8];
	memset(szTemp, 0, sizeof(szTemp));
	memcpy(szTemp, szTime, 4);
	int iYear = atoi(szTemp);
	memset(szTemp, 0, sizeof(szTemp));
	if (szTime[4] == '0')
	{
		memcpy(szTemp, szTime + 5, 1);
	}
	else
	{
		memcpy(szTemp, szTime + 4, 2);
	}
	int iMonth = atoi(szTemp);
	memset(szTemp, 0, sizeof(szTemp));
	if (szTime[6] == '0')
	{
		memcpy(szTemp, szTime + 7, 1);
	}
	else
	{
		memcpy(szTemp, szTime + 6, 2);
	}
	int iDay = atoi(szTemp);
	//_log(_DEBUG, "gamelogic", "GetPointDayStartTimestamp iDateFlag = %d,iYear=%d,iMonth=%d,iDay=%d", iDateFlag, iYear, iMonth, iDay);
	return GetPointDayStartTimestamp(iYear, iMonth, iDay);
}


time_t GameLogic::GetPointDayStartTimestamp(int iYear, int iMonth, int iDay)
{
	struct tm tm_t;
	tm_t.tm_mon = iMonth - 1;
	tm_t.tm_year = iYear - 1900;
	tm_t.tm_mday = iDay;
	tm_t.tm_hour = 0;
	tm_t.tm_min = 0;
	tm_t.tm_sec = 0;
	time_t iTm = mktime(&tm_t);
	return iTm;
}

void GameLogic::RefreshConfInfo()
{
	bool bFirst = m_pServerBaseInfo->iAccountTimeGap == 0 ? true : false;

	GetValueInt(&m_pServerBaseInfo->iAccountTimeGap, "account_time_gap", m_pServerBaseInfo->szConfFile, "System Base Info", "300");
	GetValueInt(&m_pServerBaseInfo->iAccountMoneyGap, "account_money_gap", m_pServerBaseInfo->szConfFile, "System Base Info", "100000000");
	GetValueInt(&m_pServerBaseInfo->iAccountDiamondGap, "account_diamond_gap", m_pServerBaseInfo->szConfFile, "System Base Info", "20");
	GetValueInt(&m_pServerBaseInfo->iAccountIntegralGap, "account_integral_gap", m_pServerBaseInfo->szConfFile, "System Base Info", "100");
	GetValueInt(&(m_pServerBaseInfo->iTestNet), "limit_test_net", m_pServerBaseInfo->szConfFile, "System Base Info", "999999999");

	if (bFirst)
	{
		GetValueInt(&(m_pServerBaseInfo->iLocalUpdateOnlineTm), "update_online_tm", "local_test.conf", "System Base Info", "0");
		GetValueInt(&(m_pServerBaseInfo->iLocalSysRoomInfoTm), "sys_room_info_tm", "local_test.conf", "System Base Info", "0");
		GetValueInt(&(m_pServerBaseInfo->iLocalSysServerStatTm), "sys_server_stat_tm", "local_test.conf", "System Base Info", "0");

		GetValueStr(m_pServerBaseInfo->szLocalTestIP, "ip", "local_test.conf", "server", "");
		GetValueInt(&(m_pServerBaseInfo->iLocalTestPort), "port", "local_test.conf", "server", "0");

		GetValueStr(m_pServerBaseInfo->szLocalTestRoomIP, "ip", "local_test.conf", "room", "");
		GetValueInt(&(m_pServerBaseInfo->iLocalTestRoomPort), "port", "local_test.conf", "room", "0");

		GetValueStr(m_pServerBaseInfo->szLocalTestCenterIP, "ip", "local_test.conf", "center", "");
		GetValueInt(&(m_pServerBaseInfo->iLocalTestCenterPort), "port", "local_test.conf", "center", "0");

		GetValueStr(m_pServerBaseInfo->szLocalTestRadiusIP, "ip", "local_test.conf", "radius", "");
		GetValueInt(&(m_pServerBaseInfo->iLocalTestRadiusPort), "port", "local_test.conf", "radius", "0");

		GetValueStr(m_pServerBaseInfo->szLocalTestLogIP, "ip", "local_test.conf", "log", "");
		GetValueInt(&(m_pServerBaseInfo->iLocalTesttLogPort), "port", "local_test.conf", "log", "0");

		GetValueStr(m_pServerBaseInfo->szLocalTestRedisIP, "ip", "local_test.conf", "redis", "");
		GetValueInt(&(m_pServerBaseInfo->iLocalTestRedisPort), "port", "local_test.conf", "redis", "0");

		_log(_DEBUG, "GL", "RefreshConfInfo center[%s:%d]", m_pServerBaseInfo->szLocalTestCenterIP, m_pServerBaseInfo->iLocalTestCenterPort);
		return;
	}

	if (m_pQueToRadius != NULL)
	{
		_log(_ERROR, "GL", "RefreshConfInfo GetPropDiamond Conf!!!!!");
		vector<string> vcGetParams;
		vcGetParams.push_back("prop_diamond_value");
		vcGetParams.push_back("prop_diamond_value1");
		SendGetServerConfParams(vcGetParams);
	}

	SendGetLogLevelSyncMsg(0, m_pServerBaseInfo->iServerID); //10分钟请求刷新一次log日志等级
}

void GameLogic::SkipToDay()
{
	CallBackSkipToDay();
}

void GameLogic::SendGetLogLevelSyncMsg(int iSeverType, int iServerId)
{
	//请求同步log日志的配置
	ServerLogLevelConfMsgDef msgReq;
	memset(&msgReq, 0, sizeof(ServerLogLevelConfMsgDef));

	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq.msgHeadInfo.iMsgType = SERVER_LOG_LEVEL_SYNC_MSG;

	msgReq.iServerType = iSeverType;
	msgReq.iServerId = iServerId;

	if (m_pQueToRoom)
	{
		m_pQueToRoom->EnQueue(&msgReq, sizeof(ServerLogLevelConfMsgDef));
	}
}

void GameLogic::CheckTableRecMsgOutTime(time_t tmNow)
{
	for (int j = 0; j < MAX_TABLE_NUM; j++)
	{
		FactoryTableItemDef* pTableItem = GetTableItem(0, j);
		if (pTableItem->cPlayerNum > 0 && (tmNow - pTableItem->tmLastRecUserMsg) >= 600)
		{
			int iTimeOutPlayer = 0;
			int iPlayerNum = 0;
			for (int m = 0; m < 10; m++)
			{
				if (pTableItem->pFactoryPlayers[m])
				{
					iPlayerNum++;
					if (pTableItem->pFactoryPlayers[m]->tmTestLastRecMsg > 0 && (tmNow - pTableItem->pFactoryPlayers[m]->tmTestLastRecMsg) > 600)
					{
						_log(_ERROR, "GL", "CheckTableRecMsgOutTime[%s] tbTm[%d] userTm[%d] table[%d][%d][%d] nstatus[%d]",
							pTableItem->cReplayGameNum, pTableItem->tmLastRecUserMsg, pTableItem->pFactoryPlayers[m]->tmTestLastRecMsg,
							j, m, pTableItem->pFactoryPlayers[m]->iUserID, pTableItem->pFactoryPlayers[m]->iStatus);
						iTimeOutPlayer++;
					}
					else if (pTableItem->pFactoryPlayers[m]->tmTestLastRecMsg == 0)
					{
						_log(_ERROR, "GL", "CheckTableRecMsgOutTime_2[%s] tbTm[%d] userTm[%d] table[%d][%d][%d] nstatus[%d]",
							pTableItem->cReplayGameNum, pTableItem->tmLastRecUserMsg, pTableItem->pFactoryPlayers[m]->tmTestLastRecMsg,
							j, m, pTableItem->pFactoryPlayers[m]->iUserID, pTableItem->pFactoryPlayers[m]->iStatus);
						pTableItem->pFactoryPlayers[m]->tmTestLastRecMsg = tmNow;
					}
				}
			}
			if (iTimeOutPlayer == iPlayerNum && iTimeOutPlayer > 0)
			{
				for (int m = 0; m < 10; m++)
				{
					if (pTableItem->pFactoryPlayers[m])
					{
						pTableItem->pFactoryPlayers[m]->iStatus = PS_WAIT_DESK;
					}
				}
				CallBackGameOver(pTableItem, NULL);

			}
			else
			{
				_log(_ERROR, "GL", "CheckTableRecMsgOutTime,table[%d],player[%d,%d],iTimeOutPlayer[%d]", j, pTableItem->cPlayerNum, iPlayerNum, iTimeOutPlayer);
			}
		}
	}
}

void GameLogic::OnTimer(int iSecGap, time_t now)
{
	if (m_iTimerOneDayCDCnt == -1)
	{
		m_iTimerOneDayCDCnt = GlobalMethod::GetTodayLeftSeconds();
		m_iTodayDayFlag = GlobalMethod::GetDayTimeFlag(now);
	}
	bool bNeedBaseStatInfo = false;
	for (int i = 0; i < iSecGap; i++)
	{
		OneSecTime();//1秒 的定时器
		if (m_pNewAssignTableLogic)
		{
			m_pNewAssignTableLogic->CallBackOneSec(now);
		}
		if (m_pTaskLogic)
		{
			m_pTaskLogic->CallBackOneSec(now);
		}
		if (m_iTimerOneDayCDCnt - i == 20)   //距离跨天还剩20s，先上报一次
		{
			bNeedBaseStatInfo = true;
		}
	}
	m_iTimerTenMinCnt += iSecGap;
	bool bNeedGetRoomInfo = false;
	bool bNeedUpdateOnline = false;
	if (m_iTimerTenMinCnt >= 600)//10分钟刷新一次配置信息
	{		
		m_iTimerTenMinCnt = 0;
		RefreshConfInfo();
		//10分钟拿次房间信息
		bNeedGetRoomInfo = true;
		CallBackTenMinutes();
		SendLeftTabNum(0, true);
	}
	m_iTimerTenSecCnt += iSecGap;
	if (m_iTimerTenSecCnt >= 10)//十秒的定时器
	{
		m_iTimerTenSecCnt = 0;
		DoTenSecTime();
		if (m_bIfStartKickOutTime)
		{
			JudgePlayerKickOutTime(10);//判断玩家长时间没有操作剔除
		}
	}
	m_iTimerHourCnt += iSecGap;
	if (m_iTimerHourCnt >= 3600)//1小时定时器
	{
		m_iTimerHourCnt = 0;
		DoOneHour();
	}

	m_iTimerHalfHourCnt += iSecGap;
	if (m_iTimerHalfHourCnt >= 1800)//半小时定时器
	{
		m_iTimerHalfHourCnt = 0;
		bNeedBaseStatInfo = true;
		CallBackHalfHour();
	}

	m_iTimerFiveMinCnt += iSecGap;
	if (m_iTimerFiveMinCnt >= 300)//5分钟定时器
	{
		m_iTimerFiveMinCnt = 0;
		//5分钟统计一次在线人数
		FreshRoomOnline();
		bNeedUpdateOnline = true;
		CallBackFiveMinutes();
		if (m_bCheckTableRecMsg)
		{
			CheckTableRecMsgOutTime(now);
		}
	}
	if (m_pServerBaseInfo->iLocalUpdateOnlineTm > 0)
	{
		m_pServerBaseInfo->iLocalUpdateOnlineCnt += iSecGap;
		if (m_pServerBaseInfo->iLocalUpdateOnlineCnt >= m_pServerBaseInfo->iLocalUpdateOnlineTm)
		{
			m_pServerBaseInfo->iLocalUpdateOnlineCnt = 0;
			bNeedUpdateOnline = true;
		}
	}
	if (m_pServerBaseInfo->iLocalSysRoomInfoTm > 0)
	{
		m_pServerBaseInfo->iLocalSysRoomInfoCnt += iSecGap;
		if (m_pServerBaseInfo->iLocalSysRoomInfoCnt >= m_pServerBaseInfo->iLocalSysRoomInfoTm)
		{
			m_pServerBaseInfo->iLocalSysRoomInfoCnt = 0;
			bNeedGetRoomInfo = true;
		}
	}

	if (m_pServerBaseInfo->iLocalSysServerStatTm > 0)
	{
		m_pServerBaseInfo->iLocalSysServerStatCnt += iSecGap;
		if (m_pServerBaseInfo->iLocalSysServerStatCnt >= m_pServerBaseInfo->iLocalSysServerStatTm)
		{
			m_pServerBaseInfo->iLocalSysServerStatCnt = 0;
			bNeedBaseStatInfo = true;
		}
	}
	//判断是否跨天
	m_iTimerOneDayCDCnt -= iSecGap;
	if (m_iTimerOneDayCDCnt <= 0)
	{
		m_iTimerOneDayCDCnt = GlobalMethod::GetTodayLeftSeconds();
		m_iTodayDayFlag = GlobalMethod::GetDayTimeFlag(now);
		SkipToDay();
	}
	m_iOneMinCnt += iSecGap;
	if (m_iOneMinCnt >= 60)//1分钟定时器
	{
		m_iOneMinCnt = 0;
		CallBackOneMinute();
	}
	m_iFifteenMinCnt += iSecGap;
	if (m_iFifteenMinCnt >= 900)//15分钟定时器
	{
		m_iFifteenMinCnt = 0;
		CallBackFifteenMinutes();
	}
	
	if (bNeedGetRoomInfo)
	{
		SendGetRoomInfo(true);
		UpdateCServerPlayerNum(0, true);
	}
	else if (bNeedUpdateOnline)
	{
		SendGetRoomInfo(false);
		UpdateCServerPlayerNum(0, true);
	}

	//跨天之前20s上报一次
	if (bNeedBaseStatInfo)
	{
		SendGameBaseStatInfo(now);
		m_pGameEvent->SendComStatRedis(this);
	}

	time_t timeEnd;
	time(&timeEnd);

	if (timeEnd - now > 1)
	{
		_log(_ERROR, "GL", "OnTime:Long[%d][%d]", now, timeEnd);
	}
}

bool GameLogic::RDSendAccountReq(GameUserAccountReqDef* msgAccountReq, GameUserAccountGameDef* pAccountGameInfo, bool bForceSend/* = false*/)
{
	int iReqUserID = msgAccountReq->iUserID;
	if (iReqUserID < 0)//AI不计费
	{
		_log(_ERROR, "GL", "RDSendAccountReq uid[%d]", iReqUserID);
		return false;
	}
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iReqUserID, sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "GL", "RDSendAccountReq nodePlayers null uid:%d", iReqUserID);
		return false;
	}
	if (nodePlayers->bIfAINode)//AI不计费
	{
		return false;
	}
	else
	{
		if (msgAccountReq->llRDMoney < 0)
		{
			AddComEventData(1, EVENT_1::RECYCLE_MONEY, -msgAccountReq->llRDMoney);
		}
		else
		{
			AddComEventData(1, EVENT_1::SEND_MONEY, msgAccountReq->llRDMoney);
		}
	}

	time_t tmNow;
	time(&tmNow);
	int iGapTime = 0;
	if (nodePlayers->iBeginGameTime > 0)
	{
		iGapTime = tmNow - nodePlayers->iBeginGameTime;
		if (iGapTime < 0)
		{
			iGapTime = 0;
		}
		msgAccountReq->iRDGameTime = iGapTime;//记录在线时间为多少秒	
		msgAccountReq->iRDExp = iGapTime;
	}
	nodePlayers->iBeginGameTime = 0;

	msgAccountReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgAccountReq->msgHeadInfo.iMsgType = GAME_USER_ACCOUNT_MSG;
	
	nodePlayers->iMoney += msgAccountReq->llRDMoney;

	msgAccountReq->llNowMoney = nodePlayers->iMoney;
	msgAccountReq->iNowDiamond = nodePlayers->iDiamond;
	msgAccountReq->iNowSpeMark = nodePlayers->iSpeMark;
	msgAccountReq->iGameID = m_pServerBaseInfo->iGameID;
	msgAccountReq->iServerID = m_pServerBaseInfo->iServerID;
	memcpy(msgAccountReq->szUserToken, nodePlayers->szUserToken, 32);

	nodePlayers->iAllGameTime += iGapTime;
	nodePlayers->iGameTime += iGapTime;
	nodePlayers->iExp += iGapTime;

	CallBackRDSendAccountReq(nodePlayers, msgAccountReq, pAccountGameInfo);

	//内存累积值start
	nodePlayers->llMemMoney += msgAccountReq->llRDMoney;
	nodePlayers->iMemDiamond += msgAccountReq->iRDDiamond;
	nodePlayers->iMemIntegral += msgAccountReq->iRDIntegral;
	nodePlayers->iMemGameTime += msgAccountReq->iRDGameTime;
	nodePlayers->iMemExp += msgAccountReq->iRDExp;

	int iIfHaveGameInfo = msgAccountReq->iReqFlag & 0x1;
	if (iIfHaveGameInfo == 1 && pAccountGameInfo != NULL)
	{
		nodePlayers->iMemGameAccountCnt++;
		nodePlayers->stMemGameAccount.iWinNum += pAccountGameInfo->iWinNum;
		nodePlayers->stMemGameAccount.iLoseNum += pAccountGameInfo->iLoseNum;
		nodePlayers->stMemGameAccount.iAllNum += pAccountGameInfo->iAllNum;
		nodePlayers->stMemGameAccount.llWinMoney += pAccountGameInfo->llWinMoney;
		nodePlayers->stMemGameAccount.llLoseMoney += pAccountGameInfo->llLoseMoney;
		nodePlayers->stMemGameAccount.llTableMoney += pAccountGameInfo->llTableMoney;
		nodePlayers->stMemGameAccount.llGameAmount += pAccountGameInfo->llGameAmount;
		nodePlayers->stMemGameAccount.iGameExp += pAccountGameInfo->iGameExp;
		nodePlayers->stMemGameAccount.iBuffA0 = pAccountGameInfo->iBuffA0;
		nodePlayers->stMemGameAccount.iBuffA1 = pAccountGameInfo->iBuffA1;
		nodePlayers->stMemGameAccount.iBuffA2 = pAccountGameInfo->iBuffA2;
		nodePlayers->stMemGameAccount.iBuffA3 = pAccountGameInfo->iBuffA3;
		nodePlayers->stMemGameAccount.iBuffA4 = pAccountGameInfo->iBuffA4;
		nodePlayers->stMemGameAccount.iAddBuffB0 += pAccountGameInfo->iAddBuffB0;
		nodePlayers->stMemGameAccount.iAddBuffB1 += pAccountGameInfo->iAddBuffB1;
		nodePlayers->stMemGameAccount.iAddBuffB2 += pAccountGameInfo->iAddBuffB2;
		nodePlayers->stMemGameAccount.iAddBuffB3 += pAccountGameInfo->iAddBuffB3;
		nodePlayers->stMemGameAccount.iAddBuffB4 += pAccountGameInfo->iAddBuffB4;
		nodePlayers->stMemGameAccount.iContinueWin = pAccountGameInfo->iContinueWin;
		nodePlayers->stMemGameAccount.iContinueLose = pAccountGameInfo->iContinueLose;
		nodePlayers->stMemGameAccount.iDisonlineCnt = pAccountGameInfo->iDisonlineCnt;
	}
	//内存累积值end

	//清除玩家节点中当局计费值
	nodePlayers->iRDMoney = 0;
	nodePlayers->iRDDiamond = 0;
	nodePlayers->iRDIntegral = 0;
	nodePlayers->iRDGameTime = 0;
	nodePlayers->iRDExp = 0;
	nodePlayers->stRDGameAccount.iWinNum = 0;
	nodePlayers->stRDGameAccount.iLoseNum = 0;
	nodePlayers->stRDGameAccount.iAllNum = 0;
	nodePlayers->stRDGameAccount.llWinMoney = 0;
	nodePlayers->stRDGameAccount.llLoseMoney = 0;
	nodePlayers->stRDGameAccount.llTableMoney = 0;
	nodePlayers->stRDGameAccount.llGameAmount = 0;
	nodePlayers->stRDGameAccount.iGameExp = 0;
	nodePlayers->stRDGameAccount.iAddBuffB0 = 0;
	nodePlayers->stRDGameAccount.iAddBuffB1 = 0;
	nodePlayers->stRDGameAccount.iAddBuffB2 = 0;
	nodePlayers->stRDGameAccount.iAddBuffB3 = 0;
	nodePlayers->stRDGameAccount.iAddBuffB4 = 0;

	int iRoomIndex = nodePlayers->cRoomNum - 1;
	bool bGapDay = GlobalMethod::JudgeIfGapDay(nodePlayers->iLastCheckGameTm, tmNow);
	//断线当然一定发,或者满足条件也一定发了,每10分钟也肯定要发下了
	if (msgAccountReq->iIfQuit == 1 || bForceSend == true
		|| abs(nodePlayers->llMemMoney) > m_pServerBaseInfo->iAccountMoneyGap
		|| abs(nodePlayers->iMemIntegral) > m_pServerBaseInfo->iAccountIntegralGap
		|| abs(nodePlayers->iMemDiamond) > m_pServerBaseInfo->iAccountDiamondGap
		|| tmNow - nodePlayers->tmLastAccount > m_pServerBaseInfo->iAccountTimeGap
		|| (iRoomIndex >= 0 && iRoomIndex <= 9 && nodePlayers->iMoney < m_pServerBaseInfo->stRoom[iRoomIndex].iKickMoney)
		|| bGapDay)
	{
		memset(m_cBuffForAccount, 0, sizeof(m_cBuffForAccount));
		memcpy(m_cBuffForAccount, msgAccountReq, sizeof(GameUserAccountReqDef));
		m_iBuffAccountLen = sizeof(GameUserAccountReqDef);
		//发累积内存值
		msgAccountReq->llRDMoney = nodePlayers->llMemMoney;
		msgAccountReq->iRDDiamond = nodePlayers->iMemDiamond;
		msgAccountReq->iRDIntegral = nodePlayers->iMemIntegral;
		msgAccountReq->iRDGameTime = nodePlayers->iMemGameTime;
		msgAccountReq->iRDExp = nodePlayers->iMemExp;

		if (nodePlayers->iMemGameAccountCnt > 0)
		{
			msgAccountReq->iReqFlag |= 0x01;
			memcpy(m_cBuffForAccount + m_iBuffAccountLen, &nodePlayers->stMemGameAccount, sizeof(GameUserAccountGameDef));
			m_iBuffAccountLen += sizeof(GameUserAccountGameDef);
		}

		//这里之前漏了以下部分，实际发出去的是m_cBuffForAccount，上面几句将mem赋值的不会生效(added by sff 2023/9/6)
		//added by sff 2023/9/6 start
		GameUserAccountReqDef* pTempAccountReq = (GameUserAccountReqDef*)m_cBuffForAccount;
		pTempAccountReq->llRDMoney = msgAccountReq->llRDMoney;
		pTempAccountReq->iRDDiamond = msgAccountReq->iRDDiamond;
		pTempAccountReq->iRDIntegral = msgAccountReq->iRDIntegral;
		pTempAccountReq->iRDGameTime = msgAccountReq->iRDGameTime;
		pTempAccountReq->iRDExp = msgAccountReq->iRDExp;
		pTempAccountReq->iReqFlag = msgAccountReq->iReqFlag; 
		//added by sff 2023/9/6 end
		//记录金币流水日志
		if (msgAccountReq->llRDMoney != 0)
		{
			SendMainMonetaryLog(nodePlayers->iUserID, nodePlayers->szUserName, (int)MonetaryType::Money, msgAccountReq->llRDMoney, nodePlayers->iMoney - msgAccountReq->llRDMoney, (int)ComGameLogType::GAME_NORMAL_ACCOUNT);
		}
		if (msgAccountReq->iRDDiamond != 0)
		{
			SendMainMonetaryLog(nodePlayers->iUserID, nodePlayers->szUserName, (int)MonetaryType::Diamond, msgAccountReq->iRDDiamond, nodePlayers->iDiamond - msgAccountReq->iRDDiamond, (int)ComGameLogType::GAME_NORMAL_ACCOUNT);
		}
		if (msgAccountReq->iRDIntegral != 0)
		{
			SendMainMonetaryLog(nodePlayers->iUserID, nodePlayers->szUserName, (int)MonetaryType::Integral, msgAccountReq->iRDIntegral, nodePlayers->iIntegral - msgAccountReq->iRDIntegral, (int)ComGameLogType::GAME_NORMAL_ACCOUNT);
		}
		//清掉
		nodePlayers->llMemMoney = 0;
		nodePlayers->iMemDiamond = 0;
		nodePlayers->iMemIntegral = 0;
		nodePlayers->iMemGameTime = 0;
		nodePlayers->iMemExp = 0;
		memset(&nodePlayers->stMemGameAccount, 0, sizeof(GameUserAccountGameDef));
	}
	else
	{
		_log(_DEBUG, "GL", "RDSendAccountReq error:[%d][%s]delay account,money[%lld,%lld],diamond[%d,%d],integral[%d,%d]", nodePlayers->iUserID,
			nodePlayers->szUserName, msgAccountReq->llRDMoney, nodePlayers->llMemMoney, msgAccountReq->iRDDiamond, nodePlayers->iMemDiamond,
			msgAccountReq->iRDIntegral, nodePlayers->iMemIntegral);
		return false;
	}

	_log(_DEBUG, "GL", "RDSendAccountReq:[%d][%s]account,nowmoney[%d] money[%lld,%lld],diamond[%d,%d],integral[%d,%d]，bGapDay[%d]", nodePlayers->iUserID,
		nodePlayers->szUserName, nodePlayers->iMoney, msgAccountReq->llRDMoney, nodePlayers->llMemMoney, msgAccountReq->iRDDiamond, nodePlayers->iMemDiamond,
		msgAccountReq->iRDIntegral, nodePlayers->iMemIntegral, bGapDay);

	nodePlayers->tmLastAccount = time(NULL);

	if (m_pQueToRadius)
	{
		m_pQueToRadius->EnQueue(m_cBuffForAccount, m_iBuffAccountLen);
	}

	if (msgAccountReq->iIfQuit == 0)	//add by zwr 20231201 本次计费后不退出游戏再发登录请求，否则玩家退出后，自动登录会卡房间
	{
		if (bGapDay)//跨天直接再发一次登录请求，通过radius同步下用户跨天的信息
		{
			//向Radius发送用户信息请求
			GameUserAuthenInfoReqDef msgRD;
			memset(&msgRD, 0, sizeof(GameUserAuthenInfoReqDef));
			msgRD.msgHeadInfo.cVersion = MESSAGE_VERSION;
			msgRD.msgHeadInfo.iMsgType = GAME_USER_AUTHEN_INFO_MSG;
			msgRD.iUserID = nodePlayers->iUserID;
			memcpy(msgRD.szUserToken, nodePlayers->szUserToken, 32);
			msgRD.iGameID = m_pServerBaseInfo->iGameID;
			msgRD.iServerID = m_pServerBaseInfo->iServerID;
			msgRD.iRoomType = nodePlayers->iEnterRoomType;
			//第5位是否仅是游戏跨天重新同步用户信息
			msgRD.iAuthenFlag = (1 << 4);
			_log(_DEBUG, "GL", "RDSendAccountReq user[%d] iAuthenFlag[%d]", nodePlayers->iUserID, msgRD.iAuthenFlag);
			if (m_pQueToRadius != NULL)
			{
				m_pQueToRadius->EnQueue(&msgRD, sizeof(GameUserAuthenInfoReqDef));
			}
			CallBackDayGapAfterRdAccount(nodePlayers);
			nodePlayers->iLastCheckGameTm = tmNow;
		}
	}
	return true;
}


void GameLogic::CLSendSimpleNetMessage(int iIndex, void *msg, int iMsgType, int iMsgLen)
{
	//_log(_DEBUG, "GL", "CLSendSimpleNetMessage[%d],iMsgType[%x]", iIndex, iMsgType);

	if (iIndex < 0)
		return;

	MsgHeadDef *msgHead = (MsgHeadDef*)msg;
	msgHead->cVersion = MESSAGE_VERSION;
	msgHead->iMsgType = iMsgType;
	msgHead->iSocketIndex = iIndex;

	if (iMsgType == KICK_OUT_SERVER_MSG)
	{
		KickOutServerDef * pMsg = (KickOutServerDef*)msg;
		_log(_DEBUG, "GL", "KickOutServer user[%d] cKickType[%d] cClueType[%d] cKickSubType[%d]", pMsg->iKickUserID, pMsg->cKickType, pMsg->cClueType, pMsg->cKickSubType);
	}

	if (iMsgType == GAME_BULL_NOTICE_MSG)
	{
		m_pQueToClient->EnQueue(msg, iMsgLen, 0, false);
	}
	else
	{
		m_pQueToClient->EnQueue(msg, iMsgLen);
	}
	return;
}

void GameLogic::CLSendPlayerInfoNotice(int iIndex, int iTablePlayerNum)
{
	//正常游戏
	if (iIndex < 0)
	{
		_log(_ERROR, "GL", "CLSendPlayerInfoNotice index [%d]", iIndex);
		return;
	}
	CallBackBeforeSendTableUsersInfo(iIndex);

	memset(m_cPlayerInfoBuff, 0, sizeof(m_cPlayerInfoBuff));
	PlayerInfoNoticeDef* pMsgNotice = (PlayerInfoNoticeDef*)m_cPlayerInfoBuff;
	int iSendNum = 0;
	m_iPlayerInfoBuffLen = 0;
	for (int i = 0; i < m_iPlayerInfoDeskNum; i++)
	{
		if (iSendNum == 0)
		{
			memset(m_cPlayerInfoBuff, 0, sizeof(m_cPlayerInfoBuff));
			pMsgNotice->msgHeadInfo.iMsgType = PLAYER_INFO_NOTICE_MSG;
			pMsgNotice->msgHeadInfo.cVersion = MESSAGE_VERSION;
			pMsgNotice->msgHeadInfo.iSocketIndex = iIndex;
			pMsgNotice->cFlag1 = iTablePlayerNum;
			m_iPlayerInfoBuffLen = sizeof(PlayerInfoNoticeDef);
		}
		memcpy(m_cPlayerInfoBuff + m_iPlayerInfoBuffLen, &m_PlayerInfoDesk[i], sizeof(PlayerInfoResNorDef)); 
		m_iPlayerInfoBuffLen += sizeof(PlayerInfoResNorDef);
		iSendNum++;
	}

	//桌上所有玩家的额外信息（暂无）
	/*int iExtraInfoLen = 0;
	char cExtraInfo[256] = { 0 };
	bool bNeedAchv = IfNoticeNeedAchieve();
	if (bNeedAchv)
	{
		for (int i = 0; i < m_iPlayerInfoDeskNum; i++)
		{
			int iAchieveLv = m_PlayerInfoDesk[i].iAchieveLv;
			memcpy(cExtraInfo + iExtraInfoLen, &iAchieveLv, sizeof(int));
			iExtraInfoLen += sizeof(int);
		}
	}*/

	if (iSendNum > 0)
	{
		pMsgNotice->cPlayerNum = iSendNum;
		//memcpy(m_cPlayerInfoBuff + m_iPlayerInfoBuffLen, cExtraInfo, iExtraInfoLen);
		//m_iPlayerInfoBuffLen += iExtraInfoLen;
		if (m_pQueToClient && m_pQueToClient->EnQueue(m_cPlayerInfoBuff, m_iPlayerInfoBuffLen) != 0)
		{
			_log(_ERROR, "GL", "CLSendPlayerInfoNotice: Enqueue fail.");
		}
	}

	return;
}

int GameLogic::SetAgainPlayerInfo(char *pDate)//设置掉线重入时玩家信息
{
	memcpy(pDate, m_PlayerInfoDesk, sizeof(PlayerInfoResNorDef)*m_iPlayerInfoDeskNum);
	return sizeof(PlayerInfoResNorDef)*m_iPlayerInfoDeskNum;
}

void GameLogic::HandleRDAccountRes(int iUserID, void *pMsgData)
{
	GameUserAccountResDef*pMsgRes = (GameUserAccountResDef*)pMsgData;
	_log(_DEBUG, "GL", "HandleRDAccountRes:[%d],iAccountType =%d,rt=%d\n", pMsgRes->iUserID, pMsgRes->iAccountType, pMsgRes->iRt);
	if (pMsgRes->iRt == g_iNeedChangeServer)//需要换服
	{
		if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->CallBackUserChangeServer(iUserID);
		}	
		return;
	}

	FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)(&iUserID), sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "GL", "HandleRDAccountRes:uid[%d] nodePlayers is null, iAccountType =%d,rt=%d", pMsgRes->iUserID, pMsgRes->iAccountType, pMsgRes->iRt);
		return;
	}
	if (pMsgRes->iRt != 0)//异常，踢人
	{
		FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
		if (nodePlayers)
		{
			if (pMsgRes->iRt == g_iMoneySysError)   //计费不同步，主动去同步一次
			{
				SendGameRefreshUserInfoReq(nodePlayers, true, false);
			}
			else  //否则踢人
			{
				nodePlayers->cErrorType = pMsgRes->iRt;
			}
			_log(_DEBUG, "GL", "HandleRDAccountRes:msgRes->cResult =%d,UserName[%s]\n", pMsgRes->iRt, nodePlayers->szUserName);
		}
	}
	else //仅同步下iSpeMark
	{
		nodePlayers->iSpeMark = pMsgRes->iSpeMark;
	}
	return;
}

void GameLogic::HandleRDUserPropsRes(void* pMsgData)
{
	GameUserProResMsg* pMsgRes = (GameUserProResMsg*)pMsgData;
	FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)(&pMsgRes->iUserID), sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_DEBUG, "GL", "HandleRDUserPropsRes nodePlayers is null iUserID[%d]", pMsgRes->iUserID);
		return;
	}

	int iPlatformProp = pMsgRes->iFlag & 0x1;
	int iGameProp = (pMsgRes->iFlag >> 1) & 0x1;
	int iPointProp = (pMsgRes->iFlag >> 2) & 0x1;
	if (iPlatformProp == 1 || iGameProp == 1)
	{
		vector<GameUserOnePropDef>().swap(nodePlayers->vcUserProp);
		nodePlayers->vcUserProp.clear();
	}
	GameUserOnePropDef* pOneProp = (GameUserOnePropDef*)((char*)pMsgData + sizeof(GameUserProResMsg));
	for (int i = 0; i < pMsgRes->iPropNum; i++)
	{
		SetUserProp(nodePlayers, *pOneProp);
		pOneProp++;
	}
	CallBackGetPropsSuccess(nodePlayers);

	//道具信息发送给客户端
	SendPropInfoToClient(nodePlayers);
}

void GameLogic::SendPropInfoToClient(FactoryPlayerNodeDef *nodePlayers)
{
	if (!nodePlayers)
	{
		_log(_ERROR, "GL", "SendPropInfoToClient nodePlayers No *****");
		return;
	}

	char cMsg[2048] = {0};
	int iMsgLen = 0;

	UserPropInfoResDef *msgPropRes = (UserPropInfoResDef*)cMsg;
	memset(msgPropRes, 0, sizeof(UserPropInfoResDef));
	msgPropRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgPropRes->msgHeadInfo.iMsgType = USER_PROPINFO_RES_MSG;
	msgPropRes->msgHeadInfo.iSocketIndex = nodePlayers->iSocketIndex;
	msgPropRes->iUserID = nodePlayers->iUserID;
	msgPropRes->iNum = nodePlayers->vcUserProp.size();
	iMsgLen += sizeof(UserPropInfoResDef);
	for (int i = 0; i < msgPropRes->iNum; i++)
	{
		GameUserOnePropDef info = nodePlayers->vcUserProp[i];
		memcpy(cMsg + iMsgLen, &info, sizeof(GameUserOnePropDef));
		iMsgLen += sizeof(GameUserOnePropDef);
	}
	int iPriceNum = m_mapPropDiamodValue.size();
	memcpy(cMsg + iMsgLen, &iPriceNum, 4);
	iMsgLen += 4;
	map<int, int>::iterator pos = m_mapPropDiamodValue.begin();
	while (pos != m_mapPropDiamodValue.end())
	{
		int iPropID = pos->first;
		int iPrice = pos->second;
		int info[2] = { iPropID , iPrice };
		memcpy(cMsg + iMsgLen, &info, 8);
		iMsgLen += 8;
		pos++;
	}

	if (m_pQueToClient && m_pQueToClient->EnQueue(cMsg, iMsgLen) != 0)
	{
		_log(_ERROR, "GL", "SendPropInfoToClient: Enqueue fail.");
	}
}

void GameLogic::SetUserProp(FactoryPlayerNodeDef* nodePlayers, GameUserOnePropDef& stOneProp)
{
	if (nodePlayers == NULL)
	{
		return;
	}
	bool bFind = false;
	for (int i = 0; i < nodePlayers->vcUserProp.size(); i++)
	{
		if (nodePlayers->vcUserProp[i].iPropID == stOneProp.iPropID)
		{
			memcpy(&nodePlayers->vcUserProp[i], &stOneProp, sizeof(GameUserOnePropDef));
			bFind = true;
			break;
		}
	}
	if (!bFind)
	{
		nodePlayers->vcUserProp.push_back(stOneProp);
	}
}
void GameLogic::AddUserProp(FactoryPlayerNodeDef* nodePlayers, int iPropID, int iPropNum, int iLastTime, int iGameID)
{
	if (nodePlayers == NULL)
	{
		return;
	}
	for (int i = 0; i < nodePlayers->vcUserProp.size(); i++)
	{
		if (nodePlayers->vcUserProp[i].iPropID == iPropID)
		{
			nodePlayers->vcUserProp[i].iPropNum += iPropNum;
			nodePlayers->vcUserProp[i].iLastTime = iLastTime;
			break;
		}
	}

	UpdateUserPropReqDef req;
	memset(&req, 0, sizeof(UpdateUserPropReqDef));
	req.msgHeadInfo.cVersion = MESSAGE_VERSION;
	req.msgHeadInfo.iMsgType = GAME_UPDATE_USER_PROP_MSQ;
	req.iGameID = iGameID;
	req.iPropID = iPropID;
	req.iPropNum = iPropNum;
	req.iUserID = nodePlayers->iUserID;
	req.iUpdateType = iPropNum > 0 ? 1 : 2;
	if (iLastTime > 0)
	{
		req.iUpdateType = 3;
		req.iPropNum = iLastTime;
	}
	if (m_pQueToRadius)
	{
		m_pQueToRadius->EnQueue(&req, sizeof(UpdateUserPropReqDef));
	}
}

GameUserOnePropDef* GameLogic::GetUserPropInfo(FactoryPlayerNodeDef* nodePlayers, int iPropID)
{
	if (nodePlayers == NULL)
	{
		return NULL;
	}
	for (int i = 0; i < nodePlayers->vcUserProp.size(); i++)
	{
		if (nodePlayers->vcUserProp[i].iPropID == iPropID)
		{
			return &nodePlayers->vcUserProp[i];
			break;
		}
	}
	return NULL;
}

int GameLogic::GetPropDiamondValue(int iPropID)
{
	int iValue = -1;
	map<int, int>::iterator pos = m_mapPropDiamodValue.find(iPropID);
	if (pos != m_mapPropDiamodValue.end())
	{
		iValue = pos->second;
	}
	return iValue;
}
bool GameLogic::CheckGameServerVersionNum(AuthenReqDef* pAuthenInfo)//校验客户端放过来服务器版本号
{
	int iRoomIndex = -1;
	int iRoomVer = -1;
	for (int i = 0; i < 10; i++)
	{
		_log(_DEBUG, "GL", "HandleAuthenReq: i[%d] roomType[%d] cEnterRoomType[%d] iMinGameVer[%d] iNowClientVer[%d]", i, m_pServerBaseInfo->stRoom[i].iRoomType, pAuthenInfo->cEnterRoomType, m_pServerBaseInfo->stRoom[i].iMinGameVer, pAuthenInfo->iNowClientVer);
		if (m_pServerBaseInfo->stRoom[i].iRoomType == pAuthenInfo->cEnterRoomType)
		{
			iRoomVer = m_pServerBaseInfo->stRoom[i].iMinGameVer;
			if (pAuthenInfo->iNowClientVer >= m_pServerBaseInfo->stRoom[i].iMinGameVer)
			{
				return true;
			}
		}
	}
	_log(_ERROR, "GL", "HandleAuthenReq: user[%d]clientVer[%d] err,iRoomVer[%d] cEnterRoomType[%d]", pAuthenInfo->iUserID, pAuthenInfo->iNowClientVer, iRoomVer, pAuthenInfo->cEnterRoomType);
	return false;
}

void GameLogic::HandleAuthenReq(int iSocketIndex, void *pMsgData)
{
	AuthenReqDef *msgReq = (AuthenReqDef*)pMsgData;
	int iReqUserID = msgReq->iUserID;

	_log(_DEBUG, "GL", "HandleAuthenReq:user[%d],IP:%s iLoginType[%d]cNowVerType[%d]roomNo[%d]entertype[%d]", 
		msgReq->iUserID, msgReq->szIP, msgReq->iVipType, msgReq->iNowClientVer, msgReq->iRoomID, msgReq->cEnterRoomType);
	//版本验证
	//if(strcmp(msgReq->szVersionNum,m_pServerBaseInfo->cVersionNum)!=0)
	int iAuthenFailType = 0;
	int iRoomNum = -1;
	while (true)
	{
		if (msgReq->iRoomID != 0)
		{
			//暂时屏蔽好友房相关
			iAuthenFailType = g_iRadiusAuthenErr;
			break;
		}
		if (m_pRadiusEpoll == NULL || m_pRadiusEpoll->m_bServerAuthenOK == false)
		{
			iAuthenFailType = g_iRadiusAuthenErr;
			break;
		}
		if (m_pRoomEpoll == NULL || m_pRoomEpoll->m_bServerAuthenOK == false || m_pServerBaseInfo->stRoom[0].iRoomType == 0)
		{
			iAuthenFailType = g_iRoomAuthenErr;
			break;
		}
		if (!CheckGameServerVersionNum(msgReq))
		{
			iAuthenFailType = g_iLowClientVer;
			break;
		}
		int iExtraError = CheckAuthExtraFlag((const char*)pMsgData);
		if (iExtraError != 0)
		{
			iAuthenFailType = iExtraError;
			break;
		}
		if (msgReq->cEnterRoomType == 0)
		{
			iAuthenFailType = g_iAuthenParamErr;
			break;
		}
		if (iRoomNum == -1)
		{
			for (int i = 0; i < 10; i++)
			{
				//_log(_DEBUG, "GL", "HandleAuthenReq i[%d] iRoomType[%d] cEnterRoomType[%d]", i, m_pServerBaseInfo->stRoom[i].iRoomType, msgReq->cEnterRoomType);
				if (m_pServerBaseInfo->stRoom[i].iRoomType == msgReq->cEnterRoomType)
				{
					iRoomNum = i + 1;
					break;
				}
			}
		}
		if (iRoomNum == -1)
		{
			iAuthenFailType = g_iRoomClosed;
			break;
		}
		if (strlen(msgReq->szUserToken) < 4)
		{
			iAuthenFailType = g_iTokenError;
			_log(_DEBUG, "GL", "status clear1 ID[%d]HandleAuthenReq iAuthenFailType[%d] szUserToken[%s]", ((AuthenReqDef *)(pMsgData))->iUserID, iAuthenFailType, msgReq->szUserToken);
			break;
		}
		if (!CallBackJudgePlayType(msgReq->iPlayType))
		{
			iAuthenFailType = g_iGameLogicKick;
			_log(_DEBUG, "GL", "status clear1 ID[%d]HandleAuthenReq iAuthenFailType[%d] playType[%d]", ((AuthenReqDef *)(pMsgData))->iUserID, iAuthenFailType, msgReq->iPlayType);
			break;
		}
		break;
	}
	if (iAuthenFailType > 0)
	{
		AuthenResDef msgCL;
		memset((char*)&msgCL, 0x00, sizeof(AuthenResDef));
		msgCL.cResult = iAuthenFailType;
		CLSendSimpleNetMessage(iSocketIndex, &msgCL, AUTHEN_RES_MSG, sizeof(AuthenResDef));  //反馈
		if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->CheckIfInWaitTable(msgReq->iUserID, true);
			m_pNewAssignTableLogic->CallBackUserLeave(msgReq->iUserID, 0);
		}
		return;
	}
	//建立节点
	FactoryPlayerNodeDef *nodeTmpPlayer = (FactoryPlayerNodeDef *)(GetFreeNode());

	nodeTmpPlayer->iStatus = 0;
	nodeTmpPlayer->iSocketIndex = iSocketIndex;
	nodeTmpPlayer->iUserID = iReqUserID;
	nodeTmpPlayer->cRoomNum = iRoomNum;
	nodeTmpPlayer->cLoginType = msgReq->cLoginType;//平台类型0Android，1iPhone，2ipad，3微小，4抖小
	nodeTmpPlayer->cVipType = msgReq->iVipType;//暂时信任客户端的
	nodeTmpPlayer->iSpeMark = msgReq->iSpeMark;//暂时信任客户端的
	nodeTmpPlayer->cLoginFlag1 = msgReq->cExtraFlag1;
	nodeTmpPlayer->cLoginFlag2 = msgReq->cExtraFlag2;
	nodeTmpPlayer->iAgentID = msgReq->iAgentID;
	nodeTmpPlayer->iPlayType = msgReq->iPlayType;
	nodeTmpPlayer->iEnterRoomType = msgReq->cEnterRoomType;
	strcpy(nodeTmpPlayer->szIP, msgReq->szIP);
	strcpy(nodeTmpPlayer->szMac, msgReq->szMac);
	strcpy(nodeTmpPlayer->szUserToken, msgReq->szUserToken);
	_log(_DEBUG, "GL", "HandleAuthenReq user[%d] iRoomNum[%d] iPlayType[%d]] cVer[%d]", iReqUserID, msgReq->iRoomID, msgReq->iPlayType, msgReq->iNowClientVer);

	//加入到Hash表
	hashIndexTable.Add((void *)(&(iSocketIndex)), sizeof(int), nodeTmpPlayer);
	hashUserIDTable.Add((void *)(&(iReqUserID)), sizeof(int), nodeTmpPlayer);
	UpdateRoomNum(1, iRoomNum - 1, nodeTmpPlayer->cLoginType);
	//发送消息到中心服务更新人数
	UpdateCServerPlayerNum(1);

	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&(iReqUserID), sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "GL", "HandleNewAuthenReq: check nodePlayers is no NULL ID[%d]", iReqUserID);
		return;
	}

	nodePlayers->iStatus = PS_WAIT_USERINFO; //等待用户信息状态	
	nodePlayers->cTableNum = 0;
	nodePlayers->cTableNumExtra = -1;          //所在桌位置初始化
	nodePlayers->cLoginType = msgReq->cLoginType;
	nodePlayers->iIntoGameTime = time(NULL);//进入游戏时间
	nodePlayers->iNowVerType = msgReq->iNowClientVer;
	nodePlayers->iAgentID = msgReq->iAgentID;
	/*if (msgReq->iRoomID > 0)  //如果是创建房间玩家，通过reids判断房间号是否合法(暂时屏蔽)
	{
		nodeTmpPlayer->iFriendRoomID = msgReq->iRoomID;
		nodeTmpPlayer->bCreateRoom = true;
		//创建的房间id，=0表示正常普通入座,>0自己是创建者，<0房主/邀请者id
		RdGameCheckRoomNoDef pMsgCheckRoom;
		pMsgCheckRoom.msgHeadInfo.iMsgType = RD_GAME_CHECK_ROOMNO_MSG;
		pMsgCheckRoom.msgHeadInfo.cVersion = MESSAGE_VERSION;
		pMsgCheckRoom.iRoomNum = msgReq->iRoomID;
		pMsgCheckRoom.iRoomUserID = iReqUserID;
		pMsgCheckRoom.iGameID = m_pServerBaseInfo->iGameID;
		pMsgCheckRoom.iServerID = m_pServerBaseInfo->iServerID;
		if (m_pQueToRedis)
		{
			m_pQueToRedis->EnQueue(&pMsgCheckRoom, sizeof(RdGameCheckRoomNoDef));
		}
	}
	else   *///非房主非比赛玩家，直接走radius认证，房主要等房间号检查结束后；比赛玩家也等redis拿到比赛信息后再去认证
	{
		AuthByRadiusMsg(nodePlayers);
	}

	//同时向redis获取任务以及同桌等信息
	GameRedisGetUserInfoReqDef msgRedisReq;
	memset(&msgRedisReq, 0, sizeof(GameRedisGetUserInfoReqDef));
	msgRedisReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRedisReq.msgHeadInfo.iMsgType = GAME_REDIS_GET_USER_INFO_MSG;
	msgRedisReq.iUserID = iReqUserID;
	msgRedisReq.iGameID = m_pServerBaseInfo->iGameID;
	msgRedisReq.iServerID = m_pServerBaseInfo->iServerID;
	msgRedisReq.iRoomType = msgReq->cEnterRoomType;
	msgRedisReq.iPlayType = msgReq->iPlayType;
	msgRedisReq.iPlatform = msgReq->cLoginType;
	strcpy(msgRedisReq.szIP, msgReq->szIP);
	int iNeedLastestPlay = IfNeedLatestPlayer() ? 1 : 0;
	int iNeedTaskInfo = IfNeedTaskInfo() ? 1 : 0;
	int iNeedDecorate = IfNeedDecorateInfo() ? 1 : 0;
	int iNeedComTask = IfNeedComTaskInfo() ? 1 : 0;
	int iNeedRecentTask = IfNeedRecentTask() ? 1 : 0;
	int iIfMaintainUser = msgReq->iVipType == 96 ? 1 : 0;
	msgRedisReq.iReqFlag = iNeedLastestPlay | (iNeedTaskInfo << 1) | (iNeedDecorate << 2) | (iNeedComTask << 3) | (iIfMaintainUser << 4) | iNeedRecentTask << 5 | 1<<6;
	CallBackSerUserRedisLoginFlag(nodePlayers, msgRedisReq.iReqFlag);
	nodePlayers->iGetRdTaskInfoRt = 2;
	nodePlayers->iGetRdLatestPlayerRt = 2;
	nodePlayers->iGetRdDecorateRt = 2;
	nodePlayers->iGetRdComTaskRt = 2;
	if (m_pQueToRedis)
	{
		m_pQueToRedis->EnQueue(&msgRedisReq, sizeof(GameRedisGetUserInfoReqDef));
	}
}

void GameLogic::HandleSitDownReq(int iUserID, void *pMsgData)
{
	int i, j;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));//从hash表中把玩家找到
	if (!nodePlayers)
	{
		return;
	}

	if (nodePlayers->iStatus != PS_WAIT_DESK)
	{
		_log(_ERROR, "GL", "ErrorCheck6 HandleSitDownReq uid[%d] ErrorStatue[%d]", nodePlayers->iUserID, nodePlayers->iStatus);
		return;
	}

	SitDownReqDef *msgReq = (SitDownReqDef*)pMsgData;
	if (msgReq->iBindUserID > 0)
	{
		_log(_DEBUG, "GL", "HandleSitDownReq uid[%d] BindUserID[%d]", nodePlayers->iUserID, msgReq->iBindUserID);
		//待处理
		return;
	}
	//这里不检测服务器是否处于维护状态，直接发入座请求至中心服务器，中心服务器内会判断该下线服务器若处于维护状态，会再次返回合适的其他下线服务器，这样方便游戏过程中切换维护
	int iKickType = JudgeKickOutServer(nodePlayers, true, false, false);
	if (iKickType > 0)
	{
		_log(_DEBUG, "GL", "HandleSitDownReq uid[%d] iKickType[%d]", nodePlayers->iUserID, iKickType);
		return;
	}
	int cTableNum = msgReq->iTableNum;
	int cTableNumExtra = msgReq->cTableNumExtra;
	int iReqTableNum = cTableNum;

	int l, m;

	if (!SpeJudgPlayerBringMoney(nodePlayers->iUserID))
	{
		KickOutServerDef msgKO;
		memset(&msgKO, 0, sizeof(KickOutServerDef));
		msgKO.cKickType = g_iNoEnoughMoney;
		msgKO.cClueType = 1;//提示类型（0仅提示，1退出房间，2退出游戏返回大厅）
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgKO, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
		nodePlayers->cErrorType = g_iNoEnoughMoney;
		return;
	}

	if (cTableNum <= 0)//由底层分配座位
	{
		cTableNumExtra = -1;
		bool bWithControlAI = CheckIfPlayWithControlAI(nodePlayers);
		_log(_DEBUG, "GL", "HandleSitDownReq uid[%d] bWithControlAI[%d]", nodePlayers->iUserID, bWithControlAI);
		if (bWithControlAI == true)
		{
			if (nodePlayers->iVTableID != 0 && m_pNewAssignTableLogic != NULL)  //玩家已经有虚拟桌号，证明中心已经有该玩家混服配桌节点，需要删掉，在这里开控制局
			{
				m_pNewAssignTableLogic->CallBackUserLeave(nodePlayers->iUserID, nodePlayers->iVTableID);//明确玩家退出服务器时才传groupid&iVTableID，从组队/虚拟桌信息中去掉
			}
			FactoryPlayerNodeDef *pAllNode[10] = { NULL };
			pAllNode[0] = nodePlayers;
			if (m_pNewAssignTableLogic != NULL)
			{
				m_pNewAssignTableLogic->CallBackPlayWithContolAI(nodePlayers, true, pAllNode);
			}			
			return;
		}

		bool bSpecialSit = CallBackSpecialSitDown(nodePlayers);
		if (bSpecialSit)   //如果游戏有自己的入座逻辑，这里先走一次
		{
			return;
		}

		if (cTableNum <= 0 || cTableNumExtra == -1)
		{			
			if (m_pNewAssignTableLogic != NULL)//通过新的配桌去新的中心服务器统一进行配桌
			{		
				_log(_DEBUG, "GL", "HandleSitDownReq  user[%d],playerType[%d] roomType[%d],st[%d],vt[%d]", nodePlayers->iUserID, nodePlayers->iPlayType, nodePlayers->iEnterRoomType, nodePlayers->iStatus, nodePlayers->iVTableID);
				int iWaitTableIndex = m_pNewAssignTableLogic->CheckIfInWaitTable(nodePlayers->iUserID, false);
				_log(_DEBUG, "GL", "HandleSitDownReq  user[%d] iWaitTableIndex[%d]", nodePlayers->iUserID, iWaitTableIndex);
				if (iWaitTableIndex != -1)//已经配好桌了，无视发过来的入座请求
				{
					return;
				}
				m_pNewAssignTableLogic->CallBackUserNeedAssignTable(nodePlayers,true);
				return;
			}
			else
			{
				//暂不支持底层分配座位，统一由中心服务器分配
				_log(_ERROR, "GL", "HandleSitDownReq  SitDown[%d] Eorr", cTableNum);
				SitDownResDef msg;
				memset((char*)&msg, 0x00, sizeof(SitDownResDef));
				msg.cResult = 0;
				msg.iTableNum = cTableNum;
				msg.cTableNumExtra = cTableNumExtra;

				CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msg, SITDOWN_RES_MSG, sizeof(SitDownResDef));
			}
		}
	}
	else
	{
		//暂不支持指定桌子
		_log(_ERROR, "GL", "ErrorCheck6 HandleSitDownReq uid[%d]cTableNum[%d]", nodePlayers->iUserID, cTableNum);
	}
}

void GameLogic::GetTablePlayerInfo(FactoryTableItemDef *pTableItem, FactoryPlayerNodeDef *nodePlayers)
{
	int i, j;
	memset(m_PlayerInfoDesk, 0, sizeof(PlayerInfoResNorDef) * 10);
	m_iPlayerInfoDeskNum = 0;
	for (i = 0, j = 0; i<10; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != NULL)
		{
			CheckUserDecorateTm(pTableItem->pFactoryPlayers[i]);
			PlayerInfoResNorDef* playerInfoNor = &m_PlayerInfoDesk[j];
			playerInfoNor->iUserID = pTableItem->pFactoryPlayers[i]->iUserID;
			memcpy(playerInfoNor->szNickName, pTableItem->pFactoryPlayers[i]->szNickName, 64);
			playerInfoNor->iMoney = pTableItem->pFactoryPlayers[i]->iMoney;
			playerInfoNor->iAchieveLv = pTableItem->pFactoryPlayers[i]->iAchieveLevel;
			SetUserShowMoney(&m_PlayerInfoDesk[j], nodePlayers, j);
			playerInfoNor->cTableNum = pTableItem->pFactoryPlayers[i]->cTableNum;
			playerInfoNor->cTableNumExtra = pTableItem->pFactoryPlayers[i]->cTableNumExtra;
			playerInfoNor->cIfReady = pTableItem->bIfReady[i];
			playerInfoNor->cLoginType = pTableItem->pFactoryPlayers[i]->cLoginType;
			playerInfoNor->cGender = pTableItem->pFactoryPlayers[i]->iGender;
			playerInfoNor->cVipType = pTableItem->pFactoryPlayers[i]->cVipType;
			playerInfoNor->cLoginFlag1 = pTableItem->pFactoryPlayers[i]->cLoginFlag1;
			playerInfoNor->cLoginFlag2 = pTableItem->pFactoryPlayers[i]->cLoginFlag2;
			playerInfoNor->iAchieveLv = pTableItem->pFactoryPlayers[i]->iAchieveLevel;
			playerInfoNor->iHeadImg = pTableItem->pFactoryPlayers[i]->iHeadImg;
			playerInfoNor->iExp = pTableItem->pFactoryPlayers[i]->iExp;
			playerInfoNor->iGameExp = pTableItem->pFactoryPlayers[i]->iGameExp;
			playerInfoNor->iWinNum = pTableItem->pFactoryPlayers[i]->iWinNum;
			playerInfoNor->iLoseNum = pTableItem->pFactoryPlayers[i]->iLoseNum;
			playerInfoNor->iAllNum = pTableItem->pFactoryPlayers[i]->iAllNum;
			playerInfoNor->iBuffA0 = pTableItem->pFactoryPlayers[i]->iBuffA0;
			playerInfoNor->iBuffA1 = pTableItem->pFactoryPlayers[i]->iBuffA1;
			playerInfoNor->iBuffA2 = pTableItem->pFactoryPlayers[i]->iBuffA2;
			playerInfoNor->iBuffA3 = pTableItem->pFactoryPlayers[i]->iBuffA3;
			playerInfoNor->iBuffA4 = pTableItem->pFactoryPlayers[i]->iBuffA4;
			playerInfoNor->iBuffB0 = pTableItem->pFactoryPlayers[i]->iBuffB0;
			playerInfoNor->iBuffB1 = pTableItem->pFactoryPlayers[i]->iBuffB1;
			playerInfoNor->iBuffB2 = pTableItem->pFactoryPlayers[i]->iBuffB2;
			playerInfoNor->iBuffB3 = pTableItem->pFactoryPlayers[i]->iBuffB3;
			playerInfoNor->iBuffB4 = pTableItem->pFactoryPlayers[i]->iBuffB4;
			playerInfoNor->iContinueWin = pTableItem->pFactoryPlayers[i]->iContinueWin;
			playerInfoNor->iContinueLose = pTableItem->pFactoryPlayers[i]->iContinueLose;
			playerInfoNor->iHeadFrameID = pTableItem->pFactoryPlayers[i]->iHeadFrameID;
			playerInfoNor->iClockPropID = pTableItem->pFactoryPlayers[i]->iClockPropID;
			playerInfoNor->iChatBubbleID = pTableItem->pFactoryPlayers[i]->iChatBubbleID;
			//strcpy(m_PlayerInfoDesk[j].playerInfoExtra.szHeadUrlThird, pTableItem->pFactoryPlayers[i]->szHeadUrlThird);//这个待改为后续动态发送
			j++;
		}
	}
	m_iPlayerInfoDeskNum = j;
}

void GameLogic::SetUserShowMoney(PlayerInfoResNorDef* playerInfoNor, FactoryPlayerNodeDef *nodePlayers, int idx)
{
	if (m_pNewAssignTableLogic && nodePlayers)
	{
		//使用了新的配桌机制，从每个玩家角度看，其他玩家当前身上money超过了房间限制或低于房间限制，做下显示调整
		int iRoomID = nodePlayers->cRoomNum - 1;

		if (playerInfoNor->iUserID == nodePlayers->iUserID)
		{
			playerInfoNor->iMoney = nodePlayers->iMoney;
			return;
		}

		//先判断玩家身上是否有之前存储的其他玩家金币值
		long long iShowMoney = 0;
		for (int i = 0; i < 10; i++)
		{
			if (nodePlayers->otherMoney[i].iUserID == playerInfoNor->iUserID)
			{
				playerInfoNor->iMoney = nodePlayers->otherMoney[i].llMoney;
				iShowMoney = playerInfoNor->iMoney;
				break;
			}
		}
		
		if (iShowMoney == 0)
		{
			FactoryPlayerNodeDef* nodePlayersOther = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&playerInfoNor->iUserID, sizeof(int)));
			iShowMoney = GetSimulateMoney(iRoomID, nodePlayersOther->cRoomNum-1, playerInfoNor->iMoney);
		}

		//如果没有存储或者转化完不符合上下限，重新计算
		if (iShowMoney == 0 || iShowMoney < m_pServerBaseInfo->stRoom[iRoomID].iMinMoney
			|| (iShowMoney > m_pServerBaseInfo->stRoom[iRoomID].iMaxMoney && m_pServerBaseInfo->stRoom[iRoomID].iMaxMoney > 0))
		{
			long long  iMaxMoney = m_pServerBaseInfo->stRoom[iRoomID].iMaxMoney;
			if (iMaxMoney == 0)//没有上限，还是加个
			{
				iMaxMoney = m_pServerBaseInfo->stRoom[iRoomID].iMinMoney * 100;
			}
			int iMoneyGap = iMaxMoney - m_pServerBaseInfo->stRoom[iRoomID].iMinMoney;
			int iRandValue = rand() % 100;
			int iTempMoney = iMoneyGap / 4;
			int iLeftMoney = iMoneyGap - iTempMoney;
			iShowMoney = m_pServerBaseInfo->stRoom[iRoomID].iMinMoney + iTempMoney + (iRandValue * iLeftMoney) / 100;
		}

		playerInfoNor->iMoney = iShowMoney;
		nodePlayers->otherMoney[idx].iUserID = playerInfoNor->iUserID;
		nodePlayers->otherMoney[idx].llMoney = playerInfoNor->iMoney;
		//_log(_DEBUG, "GL", "SetUserShowMoney user[%d] other[%d] money[%d]", nodePlayers->iUserID, playerInfoNor->iUserID, playerInfoNor->iMoney);
		//_log(_ALL, "GL", "SetUserShowMoney user[%d] other[%d] money[%d]", nodePlayers->iUserID, playerInfoNor->iUserID, playerInfoNor->iMoney);
	}
}

bool GameLogic::CheckUserDecorateTm(FactoryPlayerNodeDef* nodePlayers)
{
	bool bUpdateData = false;
	if (nodePlayers)
	{
		int iTime = time(NULL);
		if (nodePlayers->iHeadFrameID > 0 && nodePlayers->iHeadFrameTm > 0 && iTime > nodePlayers->iHeadFrameTm)
		{
			nodePlayers->iHeadFrameID = 0;
			nodePlayers->iHeadFrameTm = 0;
			bUpdateData = true;
		}
		if (nodePlayers->iClockPropID > 0 && nodePlayers->iClockPropTm > 0 && iTime > nodePlayers->iClockPropTm)
		{
			nodePlayers->iClockPropID = 0;
			nodePlayers->iClockPropTm = 0;
			bUpdateData = true;
		}
		if (nodePlayers->iChatBubbleID > 0 && nodePlayers->iChatBubbleTm > 0 && iTime > nodePlayers->iChatBubbleTm)
		{
			nodePlayers->iChatBubbleID = 0;
			nodePlayers->iChatBubbleTm = 0;
			bUpdateData = true;
		}
	}
	return bUpdateData;
}

void GameLogic::HandleNormalEscapeReq(FactoryPlayerNodeDef *nodePlayers, void *pMsgData)
{
	int i, j;
	int iUserID = nodePlayers->iUserID;	
	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomIndex = nodePlayers->cRoomNum - 1;
	int iEscapeType = 0;
	if (pMsgData != NULL)
	{
		EscapeReqDef *msgReq = (EscapeReqDef*)pMsgData;
		iEscapeType = msgReq->cType;
	}
	_log(_ERROR, "GL", "HandleNormalEsc user[%d]Name[%s] status[%d],iEscapeType[%d],vt[%d],iTableNum[%d][%d]", nodePlayers->iUserID, nodePlayers->szUserName, nodePlayers->iStatus,iEscapeType, nodePlayers->iVTableID, iTableNum, iRoomIndex);

	if ((nodePlayers->iVTableID != 0 || iTableNum > 0)  && nodePlayers->iStatus == PS_WAIT_READY)//等待开始时发了换桌离开，直接默认是继续（现在客户端游戏结束点继续都是发送的换桌离开）
	{	
		//此处逻辑待进一步确认
		//10退出游戏，1换桌继续
		if (iEscapeType == 10 || iEscapeType == 1)
		{
			if (iTableNum == 0)//非0情况应该是上局是控制局，本局还需要继续控制
			{
				nodePlayers->iStatus = PS_WAIT_DESK;
				int iVTableID = nodePlayers->iVTableID;
				bool bWithControlAI = false;
				if (iEscapeType == 1)
				{
					bWithControlAI = CheckIfPlayWithControlAI(nodePlayers);
				}
				int iJudgeUserID = (iEscapeType == 1&& !bWithControlAI)? -iUserID : iUserID;
				if (m_pNewAssignTableLogic != NULL)
				{
					m_pNewAssignTableLogic->CallBackUserLeave(iJudgeUserID, iVTableID);//明确玩家退出服务器时才传iVTableID，从虚拟桌信息中去掉
				}				
				if (iEscapeType == 1)
				{
					int iKickType = JudgeKickOutServer(nodePlayers, true, true, true); 
					if (iKickType > 0)
					{
						_log(_DEBUG, "GL", "HandleNormalEsc 0 simulate ready uid[%d] iKickType[%d]", nodePlayers->iUserID, iKickType);
						//从桌上移掉该玩家
						OnReadyDisconnect(nodePlayers->iUserID);
						return;
					}

					if (!bWithControlAI)
					{
						if (iEscapeType == 1)
						{
							if (JudgPlayerBringInMoney(nodePlayers->iUserID))
							{
								if (m_pNewAssignTableLogic != NULL)
								{
									m_pNewAssignTableLogic->CallBackUserNeedAssignTable(nodePlayers);
								}								
							}
							else  //钱不够踢掉
							{
								KickOutServerDef msgKO;
								memset(&msgKO, 0, sizeof(KickOutServerDef));
								msgKO.cKickType = g_iNoEnoughMoney;
								msgKO.cClueType = 1;//提示类型（0仅提示，1退出房间，2退出游戏返回大厅）
								CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgKO, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
								nodePlayers->cErrorType = g_iNoEnoughMoney;
							}
						}
						else
						{
							if (m_pNewAssignTableLogic != NULL)
							{
								m_pNewAssignTableLogic->CallBackUserNeedAssignTable(nodePlayers);
							}						
						}
						return;
					}					
				}
			}							
		}	
	}
	if (m_pNewAssignTableLogic != NULL && m_pNewAssignTableLogic->CheckUserIfVAI(iUserID))
	{
		m_pNewAssignTableLogic->CallBackUserLeave(iUserID,0);//AI离开，需要直接通知中心服务器
	}
	if (nodePlayers->iStatus == PS_WAIT_DESK)
	{
		EscapeNoticeDef msgNotice;
		memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
		msgNotice.iUserID = htonl(iUserID);//for see
		msgNotice.cType = iEscapeType;
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
		
		_log(_DEBUG, "GL", "HandleNormalEsc uid[%d] waitDesk eacspe[%d]", nodePlayers->iUserID, iEscapeType);
		//若已经在中心服务器请求配桌过，明确退出服务器时,先通知中心服务器其离开
		if (iEscapeType == 10)
		{
			int iVTableID = nodePlayers->iVTableID;
			if (m_pNewAssignTableLogic != NULL)
			{
				m_pNewAssignTableLogic->CheckIfInWaitTable(iUserID, true);
				m_pNewAssignTableLogic->CallBackUserLeave(iUserID, iVTableID);
			}
		}
		FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
		if (pTableItem != NULL)
		{
			if (iRoomIndex != -1)
			{
				if (IfSinglePlayerGame() == false)	//单人游戏，玩家离开不需要通知桌上的其他的玩家
				{
					for (i = 0; i < 10; i++)
					{
						if (pTableItem->pFactoryPlayers[i] != NULL && iTableNumExtra != i)
						{
							CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
						}
					}
				}
				RemoveTablePlayer(pTableItem, iTableNumExtra);
				nodePlayers->cTableNum = 0;
				nodePlayers->cTableNumExtra = -1;
			}
		}		
		if (nodePlayers->bIfWaitLoginTime == true || nodePlayers->cDisconnectType == 1)
		{
			//已经掉线的用户这种情况下，直接踢出服务器
			OnFindDeskDisconnect(nodePlayers->iUserID,NULL);
		}
		else if (iEscapeType == 10)
		{
			nodePlayers->bWaitExit = true;
		}
		if (nodePlayers->iUserID <= g_iMaxControlVID && nodePlayers->iUserID >= g_iMinControlVID)
		{
			ReleaseControlAINode(nodePlayers);
		}
		return;
	}

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomIndex < 0 || iRoomIndex>9)
	{
		_log(_ERROR, "GL", "HandleNormalEsc:Error user[%d]Name[%s],iRoomIndex[%d]Table[%d]", nodePlayers->iUserID, nodePlayers->szUserName, iRoomIndex, iTableNum);
		return;
	}
	if (nodePlayers->iStatus > PS_WAIT_READY && nodePlayers->iStatus != PS_WAIT_NEXTJU)
	{
		_log(_DEBUG, "GL", "HandleNormalEsc:OK user[%d]Name[%s],iRoomIndex[%d]Table[%d]Extra[%d]Status[%d]", nodePlayers->iUserID, nodePlayers->szUserName, iRoomIndex, iTableNum, iTableNumExtra, nodePlayers->iStatus);
		return;
	}

	EscapeNoticeDef msgNotice;
	memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
	msgNotice.cTableNumExtra = nodePlayers->cTableNumExtra;
	msgNotice.iUserID = htonl(iUserID);//for see
	msgNotice.cType = iEscapeType;

	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (!pTableItem)
		return;
	int  iInTableIndex = -1;
	for (i = 0; i < 10; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != NULL)
		{
			if (pTableItem->pFactoryPlayers[i]->iUserID == iUserID)
			{
				iInTableIndex = i;
				break;
			}
		}
	}
	if (iInTableIndex == -1)
	{
		_log(_ERROR, "GL", "HandleNormalEsc uid[%d],RoomIndex[%d]Table[%d] bTableJudgeOK == false", iUserID, iRoomIndex, iTableNum);
	}
	if (iTableNumExtra != -1)
	{
		if (IfSinglePlayerGame())	//单人游戏，玩家离开只通知自己，不需要通知桌上的其他玩家
		{
			CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
		}
		else
		{
			for (i = 0; i < 10; i++)
			{
				if (pTableItem->pFactoryPlayers[i] != NULL /*&& pTableItem->pFactoryPlayers[i]->cDisconnectType == 0*/)//modify by crystal 11/20
				{
					CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
				}
			}
		}
	}
	else//GM旁观的话只给GM自己发就好了吧.......还是根本就不发算了?
	{
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
	}
	if (iRoomIndex != -1)
	{
		RemoveTablePlayer(pTableItem, iTableNumExtra);
	}
	_log(_ERROR, "GL", "HandleNormalEsc_2 user[%d]Name[%s] status[%d],iEscapeType[%d],tableNextVT[%d]", nodePlayers->iUserID, nodePlayers->szUserName, nodePlayers->iStatus, iEscapeType, pTableItem->iNextVTableID);
	nodePlayers->iStatus = PS_WAIT_DESK;//逃跑玩家的状态置回找座位
	nodePlayers->cTableNum = 0;//没有座位
	nodePlayers->cTableNumExtra = -1;
	//等待开始时候离开，桌下剩余的人要判断解散
	if (pTableItem && (pTableItem->iNextVTableID > 0 || pTableItem->iNextVTableID == g_iControlRoomID))
	{
		CallBackGameOver(pTableItem, nodePlayers);
	}	
	/*if (iTableNumExtra != -1)//这段不用
	{
		pTableItem->bIfReady[iTableNumExtra] = false;

		for (i = 0; i<m_iMaxPlayerNum; i++)
		{
			if (pTableItem->pFactoryPlayers[i] != NULL)
			{
				pTableItem->pFactoryPlayers[i]->iStatus = PS_WAIT_READY;//桌上其他玩家状态置为等待开始
			}
		}
		ResetTableState(pTableItem);
	}*/

	CallBackHandleTruss(pTableItem, nodePlayers, iTableNum);//处理捆绑函数
	CallBackUserLeaveTable(pTableItem, nodePlayers, iTableNum, iTableNumExtra, false, iEscapeType);
	return;
}

void GameLogic::HandleNormalEscapeReq(int iUserID, void *pMsgData)
{
	int i, j;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}
	HandleNormalEscapeReq(nodePlayers, pMsgData);
}

void GameLogic::FillRDAccountReq(GameUserAccountReqDef* pAccountMsg, GameUserAccountGameDef* pGameAccountMsg, FactoryPlayerNodeDef* nodePlayers, int iIfQuit)
{
	pAccountMsg->iUserID = nodePlayers->iUserID;
	pAccountMsg->llRDMoney = nodePlayers->iRDMoney;
	pAccountMsg->iRDDiamond = nodePlayers->iRDDiamond;
	pAccountMsg->iRDIntegral = nodePlayers->iRDIntegral;
	pAccountMsg->iRDGameTime = nodePlayers->iRDGameTime;
	pAccountMsg->iRDExp = nodePlayers->iRDExp;
	pAccountMsg->iIfQuit = iIfQuit;
	if (nodePlayers->iIfHaveGamAccount == 1)
	{	
		memcpy(pGameAccountMsg, &nodePlayers->stRDGameAccount, sizeof(GameUserAccountGameDef));
	}
	CallBackFillRDAccountReq(pAccountMsg, pGameAccountMsg, nodePlayers, iIfQuit);
}

void GameLogic::OnUserInfoDisconnect(int iUserID, void *pMsgData)
{
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}

	_log(_ERROR, "GL", "OnUserInfoDisconnect:hashUserIDTable.Remove[%d]", nodePlayers->iUserID);

	GameUserAccountReqDef msgRadius;
	memset(&msgRadius, 0, sizeof(GameUserAccountReqDef));

	GameUserAccountGameDef msgGameAccount;
	memset(&msgGameAccount, 0, sizeof(GameUserAccountGameDef));

	FillRDAccountReq(&msgRadius, &msgGameAccount, nodePlayers, 1);
	RDSendAccountReq(&msgRadius, &msgGameAccount, true);

	//两个哈希表删除节点，再释放节点
	int rtIndex = hashIndexTable.Remove((void *)&(nodePlayers->iSocketIndex), sizeof(int));
	int rtUserID = hashUserIDTable.Remove((void *)&(nodePlayers->iUserID), sizeof(int));
	if (rtIndex < 0)
	{
		_log(_ERROR, "GL", "OnUserInfoDisconnect:hashIndexTable.Remove[%d]<0", nodePlayers->iSocketIndex);
	}
	if (rtUserID < 0)
	{
		_log(_ERROR, "GL", "OnUserInfoDisconnect:hashUserIDTable.Remove[%d]<0", nodePlayers->iUserID);
	}
	int iVTableID = nodePlayers->iVTableID;
	UpdateRoomNum(-1, nodePlayers->cRoomNum - 1, nodePlayers->cLoginType);
	ReleaseNode(nodePlayers);
	//发送消息到中心服务更新人数
	UpdateCServerPlayerNum(-1);
	if (m_pNewAssignTableLogic != NULL)
	{
		m_pNewAssignTableLogic->CheckIfInWaitTable(iUserID, true);
		m_pNewAssignTableLogic->CallBackUserLeave(iUserID, iVTableID);
	}
}

void GameLogic::OnFindDeskDisconnect(int iUserID, void *pMsgData, bool bNeedChangeServer /*= false*/)
{
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}
	int iRoomIndex = nodePlayers->cRoomNum;
	if (iRoomIndex < 1 || iRoomIndex >9)
	{
		_log(_ERROR, "GL", "OnFindDeskDisconnect:Error user[%d][%s],iRoomIndex[%d]", iUserID, nodePlayers->szUserName, iRoomIndex);
		return;
	}

	_log(_DEBUG, "GL", "OnFindDeskDisconnect:user[%d][%s] bNeedChangeServer[%d]", nodePlayers->iUserID, nodePlayers->szUserName, bNeedChangeServer);

	GameUserAccountReqDef msgRadius;
	memset(&msgRadius, 0, sizeof(GameUserAccountReqDef));
	GameUserAccountGameDef msgGameRadius;
	memset(&msgGameRadius, 0, sizeof(GameUserAccountGameDef));
	if (bNeedChangeServer)
		FillRDAccountReq(&msgRadius, &msgGameRadius, nodePlayers, 99);
	else
		FillRDAccountReq(&msgRadius, &msgGameRadius, nodePlayers, 1);
	RDSendAccountReq(&msgRadius, &msgGameRadius, true);

	nodePlayers->cDisconnectType = 1;

	CallBackDisconnect(nodePlayers);//部分游戏在离开游戏前需要用到清楚玩家节点的信息

	//如果此时桌上上还有遗留未清掉的指针，这里清理掉
	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (pTableItem)
	{
		int iTableIndex = -1;
		int iLeftNum = 0;
		for (int i = 0; i < 10; i++)
		{
			if (pTableItem->pFactoryPlayers[i] != NULL)
			{
				if (pTableItem->pFactoryPlayers[i]->iUserID == iUserID)
				{
					iTableIndex = i;
				}
				else
				{
					iLeftNum++;					
				}
			}
		}
		if (iTableIndex != -1)
		{
			RemoveTablePlayer(pTableItem, iTableNumExtra);
		}
		if (iLeftNum > 0 && iTableIndex != -1)//桌上剩余还有人，可能是一局内先结束的用户，通知桌上其他人
		{
			EscapeNoticeDef msgNotice;
			memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
			msgNotice.cTableNumExtra = iTableIndex;
			msgNotice.iUserID = htonl(iUserID);

			if (IfSinglePlayerGame())	//add by zwr 20230925 单人游戏，玩家离开只通知自己，不需要通知桌上的其他玩家
			{
				CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
			}
			else
			{
				for (int i = 0; i < 10; i++)
				{
					if (pTableItem->pFactoryPlayers[i] != NULL)
					{
						CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
					}
				}
			}
		}
	}
	int iVTableID = nodePlayers->iVTableID;
	//两个哈希表删除节点，再释放节点
	hashIndexTable.Remove((void *)&(nodePlayers->iSocketIndex), sizeof(int));
	hashUserIDTable.Remove((void *)&(nodePlayers->iUserID), sizeof(int));
	UpdateRoomNum(-1, iRoomIndex, nodePlayers->cLoginType);
	ReleaseNode(nodePlayers);

	nodePlayers->iAllSocketIndex = -1;

	//发送消息到中心服务更新人数
	UpdateCServerPlayerNum(-1);

	if (m_pNewAssignTableLogic != NULL)
	{
		m_pNewAssignTableLogic->CheckIfInWaitTable(iUserID, true);
		m_pNewAssignTableLogic->CallBackUserLeave(iUserID, iVTableID);
	}
}

void GameLogic::OnReadyDisconnect(int iUserID, void *pMsgData)
{
	int i, j;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}
	int iRoomIndex = nodePlayers->cRoomNum - 1;
	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	if (iTableNum == 0 && nodePlayers->iVTableID > 0)
	{
		//_log(_DEBUG, "GL", "OnReadyDis:user[%d][%s],VTable[%d]", iUserID, nodePlayers->szUserName, nodePlayers->iVTableID);
		int iVTableID = nodePlayers->iVTableID;
		if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->CallBackUserLeave(iUserID, iVTableID);
		}		
		return;
	}
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomIndex < 0 || iRoomIndex >9)
	{
		_log(_ERROR, "GL", "OnReadyDis:Error user[%d][%s],iRoomIndex[%d],Table[%d]", iUserID, nodePlayers->szUserName, iRoomIndex, iTableNum);
		return;
	}

	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (!pTableItem)
		return;

	_log(_DEBUG, "GL", "OnReadyDisconnect:user[%d][%s],iRoomIndex[%d:%d],Table[%d]Extra[%d]", iUserID, nodePlayers->szUserName, iRoomIndex,
		m_pServerBaseInfo->stRoom[iRoomIndex].iRoomType, iTableNum, iTableNumExtra);

	GameUserAccountReqDef msgRadius;
	memset(&msgRadius, 0, sizeof(GameUserAccountReqDef));

	GameUserAccountGameDef msgGameRadius;
	memset(&msgGameRadius, 0, sizeof(GameUserAccountGameDef));
	FillRDAccountReq(&msgRadius, &msgGameRadius, nodePlayers, 1);
	RDSendAccountReq(&msgRadius, &msgGameRadius, true);
	int iTableIndex = -1;//再次检查下桌子是不是都是对的，避免移错桌子上的人
	for (i = 0; i < 10; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != NULL)
		{
			if (pTableItem->pFactoryPlayers[i]->iUserID == iUserID)
			{
				iTableIndex = i;
				break;
			}
		}
	}
	if (iTableIndex == -1)
	{
		_log(_ERROR, "GL", "OnReadyDisconnect:table err,user[%d],room[%d],table[%d]", iUserID, iRoomIndex, iTableNum);
		return;
	}

	RemoveTablePlayer(pTableItem, iTableNumExtra);

	CallBackDisconnect(nodePlayers);//部分类游戏在离开游戏前需要用到清楚玩家节点的信息

	CallBackHandleTruss(pTableItem, nodePlayers, iTableNum);//处理捆绑函数
	CallBackUserLeaveTable(pTableItem, nodePlayers, iTableNum, iTableNumExtra, true);

	nodePlayers->cDisconnectType = 1;

	int iVTableID = nodePlayers->iVTableID;
	//两个哈希表删除节点，再释放节点
	hashIndexTable.Remove((void *)&(nodePlayers->iSocketIndex), sizeof(int));
	hashUserIDTable.Remove((void *)&(nodePlayers->iUserID), sizeof(int));
	UpdateRoomNum(-1, iRoomIndex, nodePlayers->cLoginType);
	ReleaseNode(nodePlayers);

	nodePlayers->iAllSocketIndex = -1;

	//发送消息到中心服务更新人数
	UpdateCServerPlayerNum(-1);

	if (iTableNumExtra != -1)
	{
		//通知玩家逃跑
		EscapeNoticeDef msgNotice;
		memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
		msgNotice.cTableNumExtra = iTableNumExtra;
		msgNotice.iUserID = htonl(iUserID);

		if (IfSinglePlayerGame())	//单人游戏，玩家离开只通知自己，不需要通知桌上的其他玩家
		{
			CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
		}
		else
		{
			for (i = 0; i < 10; i++)
			{
				if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->cDisconnectType == 0)//modify by crystal 11/20
				{
					CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
				}
			}
		}

		ResetTableState(pTableItem);
	}
	//配好桌的玩家有人不开始就退出，这桌都要重新配桌，通知剩下的人换桌离开
	if (m_pNewAssignTableLogic != NULL)
	{
		m_pNewAssignTableLogic->CheckIfInWaitTable(iUserID, true);
		m_pNewAssignTableLogic->CallBackUserLeave(iUserID, iVTableID);
		if (pTableItem && pTableItem->iNextVTableID > 0)//一局都没有打，在等待开始的过程中有人退出
		{
			bool bAllWaitReady = true;
			for (i = 0; i < 10; i++)
			{
				if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->iStatus > PS_WAIT_READY)
				{
					bAllWaitReady = false;
					break;
				}
			}
			if (bAllWaitReady)
			{
				CallBackGameOver(pTableItem, NULL);
			}		
			return;
		}

		if (IfSinglePlayerGame() == false)	//单人游戏，玩家离开只通知自己，不需要通知桌上的其他玩家
		{
			for (i = 0; i < 10; i++)
			{
				if (pTableItem->pFactoryPlayers[i] != NULL)
				{
					FactoryPlayerNodeDef* pTempNode = pTableItem->pFactoryPlayers[i];
					int iTempUserID = pTableItem->pFactoryPlayers[i]->iUserID;
					int iSocketIndex = pTableItem->pFactoryPlayers[i]->iSocketIndex;
					int iRoomIndex = pTableItem->pFactoryPlayers[i]->cRoomNum - 1;
					iTableNumExtra = pTableItem->pFactoryPlayers[i]->cTableNumExtra;
					RemoveTablePlayer(pTableItem, iTableNumExtra);
					pTempNode->iStatus = PS_WAIT_DESK;
					pTempNode->cTableNum = 0;//没有座位
					pTempNode->cTableNumExtra = -1;
					pTableItem->bIfReady[iTableNumExtra] = false;

					CallBackHandleTruss(pTableItem, pTempNode, iTableNum);//处理捆绑函数
					CallBackUserLeaveTable(pTableItem, pTempNode, iTableNum, iTableNumExtra, true);

					if (pTempNode->bIfAINode)   //AI节点
					{
						UpdateRoomNum(-1, pTempNode->cRoomNum - 1, -1);
					}
					if (m_pNewAssignTableLogic != NULL)
					{
						m_pNewAssignTableLogic->CallBackUserLeave(-iTempUserID, 0);//userid传负值表示仅通知中心服务器重可配桌状态
					}
					EscapeNoticeDef msgNotice;
					memset(&msgNotice, 0, sizeof(EscapeNoticeDef));
					msgNotice.iUserID = htonl(iTempUserID);
					msgNotice.cType = 1;//请求换桌的离开
					CLSendSimpleNetMessage(iSocketIndex, &msgNotice, ESCAPE_NOTICE_MSG, sizeof(EscapeNoticeDef));
				}
			}
		}
	}
	return;
}

void GameLogic::OnWaitLoginAgin(int iUserID, void *pMsgData)
{
	int i, j;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}

	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomIndex = nodePlayers->cRoomNum - 1;
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iTableNumExtra < 0 || iTableNumExtra > m_iMaxPlayerNum || iRoomIndex <0 || iRoomIndex > 9)
	{
		_log(_ERROR, "GL", "OnWaitLogin:user[%d]Name[%s][%d][%d][%d]", nodePlayers->iUserID, nodePlayers->szUserName, iTableNum, iTableNumExtra);
		return;
	}

	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (!pTableItem)
		return;

	WaitLoginAgianNoticeDef msgNotice;
	memset(&msgNotice, 0, sizeof(WaitLoginAgianNoticeDef));
	msgNotice.cTableNumExtra = iTableNumExtra;
	if (nodePlayers->cDisconnectType == 0)
	{
		nodePlayers->cDisconnectType = (char)UserDisconnectType::NET_DIS;
	}
	if (nodePlayers->cDisconnectType == (char)UserDisconnectType::NET_DIS)//超时或者中断
	{
		//累加掉线次数
		nodePlayers->stMemGameAccount.iDisonlineCnt++;
		nodePlayers->iDisNum++;
		nodePlayers->bIfDis = true;
	}
	msgNotice.cDisconnectType = nodePlayers->cDisconnectType;//掉线类型 20080716

	for (i = 0; i < 10; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			if (pTableItem->pFactoryPlayers[i]->bIfWaitLoginTime == false && pTableItem->pFactoryPlayers[i]->cDisconnectType == 0)//modify by crystal 11/20
			{
				CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgNotice, WAIT_LOGIN_AGAIN_NOTICE_MSG, sizeof(WaitLoginAgianNoticeDef));
			}
		}
	}
}

void GameLogic::HandleErrorEscapeReq(int iUserID, void *pMsgData)
{
	if (JudgeMidwayLeave(iUserID, pMsgData) == true)
	{
		return;
	}
	else
	{
		EscapeReqDef *pMsg = (EscapeReqDef*)pMsgData;
		_log(_DEBUG, "GL", "HandleErrorEscapeReq iUserID[%d] iType[%d]", iUserID, pMsg->cType);
		HandleLeaveReq(iUserID, pMsgData);
	}
}

int GameLogic::JudgeKickOutServer(FactoryPlayerNodeDef *nodePlayers, bool bSendKickMsg, bool bCheckServerSate, bool bCheckMaxLmt)//判断服务踢人函数
{
	int iKickType = 0;
	int iKickClueType = 1;//（0仅提示，1退出房间，2退出游戏返回大厅）
	if (nodePlayers == NULL)
	{
		return iKickType;
	}
	if (nodePlayers->cErrorType > 0)
	{
		iKickType = nodePlayers->cErrorType;
	}
	if (iKickType == 0 && nodePlayers->cRoomNum > 0)
	{
		int iRoomIndex = nodePlayers->cRoomNum - 1;

		if (IfNeedCheckLimit(nodePlayers))
		{
			if (m_pServerBaseInfo->stRoom[iRoomIndex].iMaxMoney > 0 && nodePlayers->iMoney > m_pServerBaseInfo->stRoom[iRoomIndex].iMaxMoney)
			{
				iKickType = g_iTooManyMoney;
			}
			if (nodePlayers->iMoney <m_pServerBaseInfo->stRoom[iRoomIndex].iKickMoney)  //下限
			{
				iKickType = g_iNoEnoughMoney;
			}
		}

		if (iKickType == 0)
		{
			iKickType = CheckSpecialLimit(nodePlayers);
		}


		//判断房间状态
		if (iKickType == 0)
		{
			if (m_pServerBaseInfo->stRoom[iRoomIndex].iRoomState == 0)
			{
				iKickType = g_iRoomClosed;
			}
			else if (m_pServerBaseInfo->stRoom[iRoomIndex].iRoomState == 2 && (nodePlayers->cVipType != 96 || nodePlayers->iSpeMark == 0))
			{
				iKickType = g_iRoomMaintian;
			}
			if (iKickType == 0)
			{
				//判断下房间开放时间
				bool bifLimit = GlobalMethod::JudgeIfOpenTimeLimit(m_pServerBaseInfo->stRoom[iRoomIndex].iBeginTime, m_pServerBaseInfo->stRoom[iRoomIndex].iEndTime);
				if (bifLimit)
				{
					iKickType = g_iRoomClosed;
				}
			}
		}
	}
	//判断服务器状态
	if (iKickType == 0 && bCheckServerSate)
	{
		if (m_pServerBaseInfo->iServerState == 0)
		{
			iKickType = g_iServerClosed;
		}
		else if (m_pServerBaseInfo->iServerState == 2 && (nodePlayers->cVipType != 96 || nodePlayers->iSpeMark == 0))
		{
			iKickType = g_iServerMaintain;
		}
		if (iKickType == 0)
		{
			//判断下服务器开放时间
			bool bifLimit = GlobalMethod::JudgeIfOpenTimeLimit(m_pServerBaseInfo->iBeginTime, m_pServerBaseInfo->iLongTime);
			if (bifLimit)
			{
				iKickType = g_iServerClosed;
			}
		}
	}
	if (iKickType == 0)
	{
		KickOutServerDef msgKO;
		memset(&msgKO, 0, sizeof(KickOutServerDef));
		CallBackJudgeKickOut(nodePlayers, &msgKO);
		if (msgKO.cKickType > 0)
		{
			iKickType = msgKO.cKickType;
			if (bSendKickMsg)
			{
				msgKO.iKickUserID = nodePlayers->iUserID;
				CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgKO, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
				bSendKickMsg = false;
			}
		}
	}
	if (iKickType > 0 && bSendKickMsg)
	{
		KickOutServerDef msgKO;
		memset(&msgKO, 0, sizeof(KickOutServerDef));
		msgKO.cKickType = iKickType;//金币不够强制T除
		msgKO.iKickUserID = nodePlayers->iUserID;
		msgKO.cClueType = iKickClueType;
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgKO, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
	}
	//其他审核防沉迷判断暂无
	return iKickType;
}

void GameLogic::SendKickOutMsg(FactoryPlayerNodeDef* nodePlayers, char cKickType, char cClueType, char cKickSubType)
{
	KickOutServerDef msgKO;
	memset(&msgKO, 0, sizeof(KickOutServerDef));
	msgKO.iKickUserID = nodePlayers->iUserID;
	msgKO.cKickType = cKickType;
	msgKO.cKickSubType = cKickSubType;
	msgKO.cClueType = cClueType;
	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgKO, KICK_OUT_SERVER_MSG, sizeof(KickOutServerDef));
}
void GameLogic::SendKickOutWithMsg(FactoryPlayerNodeDef* nodePlayers, char* cKickClueMsg, char cKickSubType, char cClueType)
{
	KickOutServerDef msgKO;
	memset(&msgKO, 0, sizeof(KickOutServerDef));
	msgKO.iKickUserID = nodePlayers->iUserID;
	msgKO.cKickType = g_iGameLogicKick;
	msgKO.cKickSubType = cKickSubType;
	msgKO.cClueType = cClueType;
	char cBuffer[512];
	int iBufferLen = sizeof(KickOutServerDef);
	memset(cBuffer,0,sizeof(cBuffer));
	memcpy(cBuffer,&msgKO, iBufferLen);
	char szOutTemp[250] = { 0 };
	GlobalMethod::code_convert("GBK", "utf-8", cKickClueMsg, strlen(cKickClueMsg), szOutTemp, 250);
	msgKO.cFlag = strlen(szOutTemp);
	memcpy(cBuffer+ iBufferLen, szOutTemp, msgKO.cFlag);
	iBufferLen += msgKO.cFlag;
	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, (void*)cBuffer, KICK_OUT_SERVER_MSG, iBufferLen);
}

bool GameLogic::HandleReadyReq(int iUserID, void *pMsgData)
{
	int i;
	ReadyNoticeDef msgNotice;
	memset((char*)&msgNotice, 0, sizeof(ReadyNoticeDef));
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		_log(_DEBUG, "GL", "HandleReadyReq:ID[%d] user node is null !!!", iUserID);
		return false;
	}

	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomIndex = nodePlayers->cRoomNum - 1;

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iTableNumExtra < 0 || iTableNumExtra > m_iMaxPlayerNum || iRoomIndex < 0 || iRoomIndex>9)
	{
		_log(_ERROR, "GL", "HandleReadyReq:ID[%d][%s],iRoomIndex[%d]table[%d][%d]", nodePlayers->iUserID, nodePlayers->szUserName, iRoomIndex, iTableNum, iTableNumExtra);
		return false;
	}
	msgNotice.cTableNumExtra = nodePlayers->cTableNumExtra;

	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (!pTableItem)
	{
		_log(_DEBUG, "GL", "HandleReadyReq:ID[%d] room[%d] table[%d] is null !!!", iUserID, iRoomIndex, iTableNum);
		return false;
	}
	//已经发送开始请求的，就不在判断是否维护，反正每局都要重新入座，发送入座请求的时候会判断是否要踢出服务器
	//准备时间	
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);

	if (pTableItem->bIfReady[iTableNumExtra])
	{
		return true;
	}
	if (pTableItem->bIfReady[iTableNumExtra] == false)
	{
		for (i = 0; i<10; i++)
		{
			if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->cDisconnectType == 0)//modify by crystal 11/20
			{
				CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgNotice, READY_NOTICE_MSG, sizeof(ReadyNoticeDef));
			}
		}
	}

	pTableItem->bIfReady[iTableNumExtra] = true;
	pTableItem->iReadyNum = 0;
	int iTablePlayerNum = 0;
	int iAiNum = 0;
	for (i = 0; i<m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			if (pTableItem->bIfReady[i] == true)
			{
				pTableItem->iReadyNum++;
			}
			if (pTableItem->pFactoryPlayers[i]->bIfAINode)
			{
				iAiNum++;
			}
			iTablePlayerNum++;
		}
	}
	pTableItem->cPlayerNum = iTablePlayerNum;
	_log(_DEBUG, "GL", "HandleReadyReq:ID[%d] iReadyNum[%d] iTablePlayerNum[%d] m_iMaxPlayerNum[%d],iRealNum[%d]", iUserID, pTableItem->iReadyNum, iTablePlayerNum, m_iMaxPlayerNum,pTableItem->iRealNum);
	if (pTableItem->iReadyNum == pTableItem->iRealNum)//所有玩家都准备OK。发牌,并且随机选择一个玩家叫牌并且可以先出牌
	{
		map<int, FactoryTableItemDef*>::iterator iter = m_mapTableWaitNext.find(iTableNum);
		if (iter != m_mapTableWaitNext.end())
		{
			m_mapTableWaitNext.erase(iter);
		}
		pTableItem->iFirstDisPlayer = -1;
		time(&(pTableItem->iBeginGameTime)); 
		//备份玩家开局钱
		for (i = 0; i<m_iMaxPlayerNum; i++)
		{
			if (pTableItem->pFactoryPlayers[i])
			{
				pTableItem->pFactoryPlayers[i]->iBeginMoney = pTableItem->pFactoryPlayers[i]->iMoney;
				pTableItem->pFactoryPlayers[i]->iBeginIntegral = pTableItem->pFactoryPlayers[i]->iIntegral;
				pTableItem->pFactoryPlayers[i]->iBeginDiamond = pTableItem->pFactoryPlayers[i]->iDiamond;
				time(&(pTableItem->pFactoryPlayers[i]->iBeginGameTime));

				pTableItem->bIfReady[i] = false;
				pTableItem->pFactoryPlayers[i]->ClearGameStart();
			}
		}
		if (m_pNewAssignTableLogic)
		{
			int iUserID[5];
			memset(iUserID, 0, sizeof(iUserID));
			for (i = 0; i < m_iMaxPlayerNum; i++)
			{
				if (pTableItem->pFactoryPlayers[i])
				{
					iUserID[i] = pTableItem->pFactoryPlayers[i]->iUserID;
				}
			}
			if (!nodePlayers->bAIContrl)
			{
				m_pNewAssignTableLogic->CallBackTableOK(iUserID);
			}
		}

		CallBackReadyReq(nodePlayers, pTableItem, 0);
		SendRedisTogetherUser(pTableItem);
	}
	return true;
}

void GameLogic::SendRedisTogetherUser(FactoryTableItemDef *pTableItem)
{
	if (pTableItem == NULL)
	{
		return;
	}

	//游戏日志用，同桌好友用户名
	memset(pTableItem->cPlayerName, 0, sizeof(pTableItem->cPlayerName));
	char cName[20];
	for (int i = 0; i < pTableItem->iSeatNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != nullptr)
		{
			if (!pTableItem->pFactoryPlayers[i]->bIfAINode)
			{
				//游戏日志用，记录同局玩家用户名
				memset(cName, 0, sizeof(cName));
				sprintf(cName, "%s,", pTableItem->pFactoryPlayers[i]->szUserName);
				strcat(pTableItem->cPlayerName, cName);
			}
			else
			{
				memset(cName, 0, sizeof(cName));
				sprintf(cName, "#,");
				strcat(pTableItem->cPlayerName, cName);
			}
		}
	}

	//通知redis同桌玩家信息
	char cBuffer[1280];
	for (int i = 0; i < pTableItem->iSeatNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != nullptr && !pTableItem->pFactoryPlayers[i]->bIfAINode)
		{
			FactoryPlayerNodeDef* nodePlayers = pTableItem->pFactoryPlayers[i];
			memset(cBuffer, 0, sizeof(cBuffer));
			int iLen = sizeof(RdGameTogetherUserReqDef);
			RdGameTogetherUserReqDef * pMsgReq = (RdGameTogetherUserReqDef*)cBuffer;
			pMsgReq->msgHeadInfo.iMsgType = RD_GAME_TOGETHER_USER_MSG;
			pMsgReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
			pMsgReq->iUserID = nodePlayers->iUserID;
			pMsgReq->iGameID = m_pServerBaseInfo->iGameID;
			pMsgReq->iExtraFlag = CallBackRecentExtra();
			for (int j = 0; j < 10; j++)
			{
				if (m_pServerBaseInfo->stRoom[j].iRoomType == nodePlayers->iEnterRoomType)
				{
					pMsgReq->iBasePoint = m_pServerBaseInfo->stRoom[j].iBasePoint;
					break;
				}
			}

			bool bWithFriend = false;
			TogetherUserDef* pUserInfo = (TogetherUserDef*)(pMsgReq + 1);
			for (int m = 0; m < pTableItem->iSeatNum; m++)
			{
				FactoryPlayerNodeDef* otherPlayers = pTableItem->pFactoryPlayers[m];
				if (otherPlayers == NULL || m == nodePlayers->cTableNumExtra || otherPlayers->bIfAINode)
				{
					continue;
				}

				bool bFriend = false;
				/*for (int n = 0; n < MAX_FRIENDS_NUM; n++)
				{
					if (nodePlayers->iFriends[n] == otherPlayers->iUserID)
					{
						nodePlayers->bPlayWithFriend = true;
						bWithFriend = true;
						bFriend = true;
						break;
					}
				}*///好友相关暂时屏蔽
				if (!bFriend)
				{
					pUserInfo->iExp = otherPlayers->iExp;
					pUserInfo->iHeadFrameId = otherPlayers->iHeadFrameID;
					pUserInfo->iHeadImg = otherPlayers->iHeadImg;
					pUserInfo->iUserID = otherPlayers->iUserID;
					memcpy(pUserInfo->szNickName, otherPlayers->szNickName, 64);
					pUserInfo++;
					iLen += sizeof(TogetherUserDef);
					pMsgReq->iUserNum++;
				}
			}
			//需要实时检查是否与好友同桌
			/*if (!bWithFriend && pTableItem->pFactoryPlayers[i]->bCheckFriendship)
			{
				pTableItem->pFactoryPlayers[i]->bPlayWithFriend = false;
				if (m_pTaskLogic)
				{
					m_pTaskLogic->CheckFriendship(pTableItem, pTableItem->pFactoryPlayers[i]);
				}
			}*///好友相关暂时屏蔽
			//记录近期同桌玩家信息
			if (m_pQueToRedis && pMsgReq->iUserNum > 0)
			{
				m_pQueToRedis->EnQueue(cBuffer, iLen);
			}
		}
	}
}

bool GameLogic::JudgeIsPlayWithAI(FactoryPlayerNodeDef* nodePlayers, bool bAllCheck)
{
	if (!nodePlayers)
	{
		return false;
	}

	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomIndex = nodePlayers->cRoomNum - 1;

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iTableNumExtra < 0 || iTableNumExtra > m_iMaxPlayerNum || iRoomIndex < 0 || iRoomIndex>9)
	{
		_log(_ERROR, "GL", "JudgeIsPlayWithAI:ID[%d][%s],iRoomIndex[%d]table[%d][%d]", nodePlayers->iUserID, nodePlayers->szUserName, iRoomIndex, iTableNum, iTableNumExtra);
		return false;
	}
	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (!pTableItem)
	{
		_log(_DEBUG, "GL", "HandleReadyReq:ID[%d] room[%d] table[%d] is null !!!", nodePlayers->iUserID, iRoomIndex, iTableNum);
		return false;
	}

	int iAllAINum = 0;
	int iMaxAINum = pTableItem->iSeatNum - 1;
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		FactoryPlayerNodeDef *pPlayerNode = pTableItem->pFactoryPlayers[i];
		if (pPlayerNode && pPlayerNode->bIfAINode)
		{
			if (!bAllCheck)
			{
				return true;
			}
			iAllAINum++;
		}
	}
	return iAllAINum == iMaxAINum;
}

void GameLogic::CallBackGameOver(FactoryTableItemDef *pTableItem, FactoryPlayerNodeDef *nodePlayers, bool bNeedAssign)
{
	if (pTableItem == NULL || pTableItem->iNextVTableID == 0)
	{
		return;
	}
	//每局结束后，将桌上剩余的人都挪到虚拟桌上等下一局继续
	time_t tmNow;
	time(&tmNow);
	bool bGameOver = true;
	for (int i = 0; i < pTableItem->iSeatNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->iStatus > PS_WAIT_READY)   //桌上还有有效玩家
		{
			bGameOver = false;
			break;
		}
	}
	if (!bGameOver)
	{
		return;
	}
	bool bInWaitTable = false;
	_log(_DEBUG, "GL", "CallBackGameOver:nVT[%d]", pTableItem->iNextVTableID);
	FactoryPlayerNodeDef *pLeftUser = NULL;
	for (int i = 0; i < pTableItem->iSeatNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			pLeftUser = pTableItem->pFactoryPlayers[i];
		}
	}
	if (nodePlayers && m_pNewAssignTableLogic != NULL)
	{
		int iInWaitIndex = m_pNewAssignTableLogic->CheckIfInWaitTable(nodePlayers->iUserID, true);
		if (iInWaitIndex != -1)
		{
			bInWaitTable = true;
		}
	}
	if (!bInWaitTable && m_pNewAssignTableLogic != NULL)
	{
		m_pNewAssignTableLogic->CallBackDissolveTableToVTable(pTableItem);
	}
	if (m_pNewAssignTableLogic != NULL)
	{
		VirtualTableDef* pVTable = m_pNewAssignTableLogic->FindVTableInfo(pTableItem->iNextVTableID);
		if (bNeedAssign && pVTable && pLeftUser)
		{
			m_pNewAssignTableLogic->CallBackTableNeedAssignTable(pVTable, pLeftUser);
		}
	}	
	if (pTableItem->iNextVTableID != g_iControlRoomID)
	{
		pTableItem->iNextVTableID = 0;
	}
	ResetTableState(pTableItem);

}

void GameLogic::SendGameLogInfo(FactoryTableItemDef *pTableItem, GameLogOnePlayerLogDef* pGameLogInfo/* = NULL*/, int iNum  /*= 0*/)
{
	if (pTableItem == NULL)
	{
		printf("pTableItem == NULL\n");
		return;
	}

	AddComEventData(1, EVENT_1::GAME_CNT);

	int i = 0;
	memset(m_cGameLogBuff, 0, sizeof(m_cGameLogBuff));
	m_iGameLogBuffLen = sizeof(GameRecordGameLogMsgDef);

	GameRecordGameLogMsgDef* pMsgReq = (GameRecordGameLogMsgDef*)m_cGameLogBuff;
	pMsgReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgReq->msgHeadInfo.iMsgType = GMAE_RECORD_GAME_LOG_MSG;

	sprintf(pMsgReq->szGameNum, "%d-%s", m_pServerBaseInfo->iServerID, pTableItem->cReplayGameNum);

	time_t tmNow;
	time(&tmNow);
	if (pTableItem->iBeginGameTime > 0)
	{
		pMsgReq->iGameTime = tmNow - pTableItem->iBeginGameTime;//记录在线时间为多少秒
	}

	int iRoomID = 0;
	int iPlayerNum = 0;
	int iRateMoney = 0;
	char cPlayerName[150];
	char cName[20];

	memset(cPlayerName, 0, sizeof(cPlayerName));
	for (i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			memset(cName, 0, sizeof(cName));
			sprintf(cName, "%s,", (pTableItem->pFactoryPlayers[i])->szUserName);
			strcat(cPlayerName, cName);

			iPlayerNum++;
		}
		else
		{
			memset(cName, 0, sizeof(cName));
			sprintf(cName, "#,");
			strcat(cPlayerName, cName);
		}
	}
	if (pGameLogInfo != NULL && iPlayerNum != iNum)
	{
		_log(_DEBUG, "FL", "SendGameLogInfo B Game iPlayerNum = %d ,iNum %d", iPlayerNum, iNum);
		return;
	}

	pMsgReq->iGameID = m_pServerBaseInfo->iGameID;
	pMsgReq->iServerID = m_pServerBaseInfo->iServerID;
	pMsgReq->iPlayerNum = iPlayerNum;
	if (strlen(cPlayerName) <= 150 && strlen(cPlayerName) > 0)
	{
		memcpy(pMsgReq->cPlayName, cPlayerName, strlen(cPlayerName) - 1);//顺便把最后一个逗号去掉
	}
	GameLogOnePlayerLogDef* pOneLog = (GameLogOnePlayerLogDef*)(m_cGameLogBuff + m_iGameLogBuffLen);
	for (i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i] && pTableItem->pFactoryPlayers[i]->bIfAINode == false)
		{
			FactoryPlayerNodeDef* pPlayerNode = pTableItem->pFactoryPlayers[i];
			pOneLog->iUserID = pPlayerNode->iUserID;
			strcpy(pOneLog->szUserName, pPlayerNode->szUserName);
			pOneLog->iAgentID = pPlayerNode->iAgentID;
			pOneLog->iPlatform = pPlayerNode->cLoginType;
			pOneLog->iGameVer = pPlayerNode->iNowVerType;
			if (pPlayerNode->cRoomNum > 0)
			{
				pOneLog->iRoomType = m_pServerBaseInfo->stRoom[pPlayerNode->cRoomNum - 1].iRoomType;
				pOneLog->iTableMoney = m_pServerBaseInfo->stRoom[pPlayerNode->cRoomNum - 1].iTableMoney;
				pOneLog->iNumInfo1 = m_pServerBaseInfo->stRoom[pPlayerNode->cRoomNum - 1].iBasePoint;
			}
			pOneLog->iTableID = pTableItem->iTableID * 1000000 + pPlayerNode->cTableNumExtra;//桌号*1000000+座位号
			pOneLog->iBeginMoney = pPlayerNode->iBeginMoney;
			pOneLog->iEndMoney = pPlayerNode->iMoney;
			pOneLog->iBeginIntegral = pPlayerNode->iBeginIntegral;
			pOneLog->iEndIntegral = pPlayerNode->iIntegral;
			pOneLog->iBeginDiamond = pPlayerNode->iBeginDiamond;
			pOneLog->iExtraAddMoney = pPlayerNode->iExtraAddMoney;
			pOneLog->iExtraLoseMoney = pPlayerNode->iExtraLoseMoney;
			strcpy(pOneLog->cExtraMoneyDetail, pPlayerNode->cExtraMoneyDetail);
			pOneLog->iEndDiamond = pPlayerNode->iDiamond;
			if (pTableItem->pFactoryPlayers[i]->bIfDis == true)
			{
				pOneLog->iIfDisConnect = 1;
			}
			if (pGameLogInfo != NULL)
			{
				for (int j = 0; j < iNum; j++)
				{
					if (pGameLogInfo[j].iUserID == pPlayerNode->iUserID)
					{
						pOneLog->iWinMoney = pGameLogInfo[j].iWinMoney;
						pOneLog->iLoseMoeny = pGameLogInfo[j].iLoseMoeny;
						pOneLog->iExtraAddMoney += pGameLogInfo[j].iExtraAddMoney;
						pOneLog->iExtraLoseMoney += pGameLogInfo[j].iExtraLoseMoney;
						//strcat(pOneLog->cExtraMoneyDetail, pGameLogInfo[j].cExtraMoneyDetail);
						pOneLog->iGameRank = pGameLogInfo[j].iGameRank;
						strcpy(pOneLog->cStrInfo1, pGameLogInfo[j].cStrInfo1);
						strcpy(pOneLog->cStrInfo2, pGameLogInfo[j].cStrInfo2);
						strcpy(pOneLog->cStrInfo3, pGameLogInfo[j].cStrInfo3);
						strcpy(pOneLog->cStrInfo4, pGameLogInfo[j].cStrInfo4);
						strcpy(pOneLog->cStrInfo5, pGameLogInfo[j].cStrInfo5);
						strcpy(pOneLog->cStrInfo6, pGameLogInfo[j].cStrInfo6);
						pOneLog->iNumInfo2 = pGameLogInfo[j].iNumInfo2;
						pOneLog->iNumInfo3 = pGameLogInfo[j].iNumInfo3;
						pOneLog->iNumInfo4 = pGameLogInfo[j].iNumInfo4;
						pOneLog->iNumInfo5 = pGameLogInfo[j].iNumInfo5;
						pOneLog->iNumInfo6 = pGameLogInfo[j].iNumInfo6;
						break;
					}
				}
			}
			m_iGameLogBuffLen += sizeof(GameLogOnePlayerLogDef);
			pOneLog++;
		}
	}
	if (m_pQueToLog)
	{
		m_pQueToLog->EnQueue(m_cGameLogBuff, m_iGameLogBuffLen);
	}
	//这里判断下任务相关
	if (m_pTaskLogic)
	{
		m_pTaskLogic->JudgeUserRedisTask(pTableItem);
	}
	for (i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			CheckUserDecorateTm(pTableItem->pFactoryPlayers[i]);
		}
	}
	if (m_pTaskLogic)
	{
		m_pTaskLogic->JudgeRdComTask(pTableItem);
	}

	//公用统计数据
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			if (!pTableItem->pFactoryPlayers[i]->bIfAINode)
			{
				SetUserBaseStat(pTableItem->pFactoryPlayers[i]);
			}
			else
			{
				SetAIBaseStat(pTableItem->pFactoryPlayers[i]);
			}
		}
	}
}

void GameLogic::SetAIBaseStat(FactoryPlayerNodeDef *nodePlayers)
{
	if (nodePlayers == nullptr)
	{
		_log(_ERROR, "GL", "SetAIBaseStat nodePlayers is NULL");
		return;
	}
	if (!nodePlayers->bIfAINode)
	{
		_log(_ERROR, "GL", "SetAIBaseStat nodePlayers[%d] not ai", nodePlayers->iUserID);
		return;
	}
	int iRoomTypeIdx = -1;
	int iRoomID = nodePlayers->cRoomNum - 1;
	if (m_pServerBaseInfo->stRoom[iRoomID].iRoomType >= 5 && m_pServerBaseInfo->stRoom[iRoomID].iRoomType <= 8)//初级房5,中级房6，高级房7，vip房8
	{
		iRoomTypeIdx = m_pServerBaseInfo->stRoom[iRoomID].iRoomType - 5;
	}
	int iRoomType = m_pServerBaseInfo->stRoom[iRoomID].iRoomType;
	int iBasePoint = m_pServerBaseInfo->stRoom[iRoomTypeIdx].iBasePoint;
	float iTableMoneyRate = m_pServerBaseInfo->stRoom[iRoomTypeIdx].iTableMoney / 100;
	int iTableMoney = iBasePoint * iTableMoneyRate;

	if (nodePlayers->llGameRtAmount > 0)
	{
		m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llAIPlayWinMoney += nodePlayers->llGameRtAmount;
		if (nodePlayers->bAIContrl)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llAIControlWin += nodePlayers->llGameRtAmount;
		}
	}
	else if (nodePlayers->llGameRtAmount < 0)
	{
		m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llAIPlayLoseMoney -= nodePlayers->llGameRtAmount;
		if (nodePlayers->bAIContrl)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llAIControlLose -= nodePlayers->llGameRtAmount;
		}
	}

	//游戏输赢分统计
	int iGameResult = nodePlayers->iGameResultForTask & 0x000f;
	if (iGameResult == 1)
	{
		m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iAIPlayWin++;
		if (nodePlayers->bAIContrl)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iAIControlWinCnt++;
		}
	}
	else if (iGameResult == 2)
	{
		m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iAIPlayLose++;
		if (nodePlayers->bAIContrl)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iAIControlLoseCnt++;
		}
	}
	else if (iGameResult == 3)  //平局
	{
		m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iAIPlayPing++;
	}
}

void GameLogic::SetUserBaseStat(FactoryPlayerNodeDef *nodePlayers)
{
	if (nodePlayers && !nodePlayers->bIfAINode)
	{
		int iPlatformIndex = nodePlayers->cLoginType;
		if (iPlatformIndex < 0 || iPlatformIndex> 2)
		{
			iPlatformIndex = 2;
		}
		m_stGameBaseStatInfo.stPlatStatInfo[iPlatformIndex].iAllGameCnt++;
		time_t tmNow = time(NULL);
		int iNowDateFlag = GetDayTimeFlag(tmNow);
		if (nodePlayers->iFirstGameTime == 0)
		{
			nodePlayers->iFirstGameTime = tmNow;
		}
		int iFirstDateFlag = GetDayTimeFlag(nodePlayers->iFirstGameTime);
		if (iNowDateFlag == iFirstDateFlag)//今日新增用户输赢情况
		{
			int iResulTask = nodePlayers->iGameResultForTask & 0x000f;
			int iIndex = 0;
			if (nodePlayers->iAllNum >= 1 && nodePlayers->iAllNum <= 9)
			{
				iIndex = nodePlayers->iAllNum - 1;
			}
			else
			{
				iIndex = 9;
			}
			if (iResulTask == 1)
			{
				m_stGameBaseStatInfo.iNewResultWin[iIndex]++;
			}
			else
			{
				m_stGameBaseStatInfo.iNewResultLose[iIndex]++;
			}
		}
		//统计下破产或低于进入限制信息
		int iRoomTypeIdx = -1;
		int iRoomID = nodePlayers->cRoomNum - 1;
		if (m_pServerBaseInfo->stRoom[iRoomID].iRoomType >= 5 && m_pServerBaseInfo->stRoom[iRoomID].iRoomType <= 8)//初级房5,中级房6，高级房7，vip房8
		{
			iRoomTypeIdx = m_pServerBaseInfo->stRoom[iRoomID].iRoomType - 5;
		}
		if (iRoomTypeIdx > -1 && nodePlayers->iMoney < 500)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iBrokenCnt++;
		}
		if (iRoomTypeIdx > -1 && nodePlayers->iMoney < m_pServerBaseInfo->stRoom[iRoomID].iMinMoney)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iKickCnt++;
		}

		int iRoomType = m_pServerBaseInfo->stRoom[iRoomID].iRoomType;
		int iBasePoint = m_pServerBaseInfo->stRoom[iRoomTypeIdx].iBasePoint;
		float iTableMoneyRate = m_pServerBaseInfo->stRoom[iRoomTypeIdx].iTableMoney / 100;
		int iTableMoney = iBasePoint * iTableMoneyRate;
		if (iRoomType >= 5 && iRoomType <= 8)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iTableMoney += iTableMoney;

			bool bPlayWithAI = JudgeIsPlayWithAI(nodePlayers);
			if (bPlayWithAI)
			{
				m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iAIPlayCnt++;
			}
			//游戏输赢分统计
			int iGameResult = nodePlayers->iGameResultForTask & 0x000f;
			if (iGameResult == 1)
			{
				m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iWinCnt++;
			}
			else if (iGameResult == 2)
			{
				m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iLoseCnt++;
			}
			else if (iGameResult == 3)
			{
				m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iPingCnt++;
			}
			// 以当前玩家的输赢分
			if (nodePlayers->llGameRtAmount > 0)
			{
				m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llAllWin += nodePlayers->llGameRtAmount;
			}
			else if (nodePlayers->llGameRtAmount < 0)
			{
				m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llAllLose -= nodePlayers->llGameRtAmount;
			}
			long long llGameRtMoney = nodePlayers->iMoney - nodePlayers->iBeginMoney - nodePlayers->iExtraAddMoney + nodePlayers->iExtraLoseMoney;
			if (llGameRtMoney > 0)
			{
				m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llSendMoney += llGameRtMoney;
			}
			else
			{
				m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llUseMoney -= llGameRtMoney;
			}

			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llExtraSendMoney += nodePlayers->iExtraAddMoney;
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llExtraUseMoney += nodePlayers->iExtraLoseMoney;


			_log(_DEBUG, "gamelogic", "SetUserBaseStat uid[%d] iGameResult[%d] llGameRtAmount[%lld] bPlayWithAI[%d] send[%lld] use[%lld]", 
				nodePlayers->iUserID, iGameResult, nodePlayers->llGameRtAmount, bPlayWithAI, m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llSendMoney, m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].llUseMoney);
		}
	}
}

void GameLogic::SendGameOneLogInfo(FactoryTableItemDef *pTableItem, FactoryPlayerNodeDef *nodePlayers, GameLogOnePlayerLogDef* pGameLogInfo/* = NULL*/)
{
	if (pTableItem == NULL)
	{
		printf("SendGameOneLogInfo pTableItem == NULL\n");
		return;
	}

	if (nodePlayers == NULL)
	{
		printf("SendGameOneLogInfo nodePlayers == NULL\n");
		return;
	}

	if (nodePlayers->bIfAINode)
	{
		SetAIBaseStat(nodePlayers);
		return;
	}

	int i = 0;
	memset(m_cGameLogBuff, 0, sizeof(m_cGameLogBuff));
	m_iGameLogBuffLen = sizeof(GameRecordGameLogMsgDef);

	GameRecordGameLogMsgDef* pMsgReq = (GameRecordGameLogMsgDef*)m_cGameLogBuff;
	pMsgReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgReq->msgHeadInfo.iMsgType = GMAE_RECORD_GAME_LOG_MSG;

	sprintf(pMsgReq->szGameNum, "%d-%s", m_pServerBaseInfo->iServerID, pTableItem->cReplayGameNum);

	time_t tmNow;
	time(&tmNow);
	if (pTableItem->iBeginGameTime > 0)
	{
		pMsgReq->iGameTime = tmNow - pTableItem->iBeginGameTime;//记录在线时间为多少秒
	}

	int iRoomID = 0;
	int iPlayerNum = 0;
	int iRateMoney = 0;

	pMsgReq->iGameID = m_pServerBaseInfo->iGameID;
	pMsgReq->iServerID = m_pServerBaseInfo->iServerID;
	pMsgReq->iPlayerNum = 1;
	if (strlen(pTableItem->cPlayerName) <= 150 && strlen(pTableItem->cPlayerName) > 0)
	{
		memcpy(pMsgReq->cPlayName, pTableItem->cPlayerName, strlen(pTableItem->cPlayerName) - 1);//顺便把最后一个逗号去掉
	}
	GameLogOnePlayerLogDef* pOneLog = (GameLogOnePlayerLogDef*)(m_cGameLogBuff + m_iGameLogBuffLen);
	
	pOneLog->iUserID = nodePlayers->iUserID;
	strcpy(pOneLog->szUserName, nodePlayers->szUserName);
	pOneLog->iAgentID = nodePlayers->iAgentID;
	pOneLog->iPlatform = nodePlayers->cLoginType;
	pOneLog->iGameVer = nodePlayers->iNowVerType;
	if (nodePlayers->cRoomNum > 0)
	{
		pOneLog->iRoomType = m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iRoomType;
		pOneLog->iTableMoney = m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iTableMoney;
		pOneLog->iNumInfo1 = m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iBasePoint;
	}
	pOneLog->iTableID = pTableItem->iTableID * 1000000 + nodePlayers->cTableNumExtra;//桌号*1000000+座位号
	pOneLog->iBeginMoney = nodePlayers->iBeginMoney;
	pOneLog->iEndMoney = nodePlayers->iMoney;
	pOneLog->iBeginIntegral = nodePlayers->iBeginIntegral;
	pOneLog->iEndIntegral = nodePlayers->iIntegral;
	pOneLog->iBeginDiamond = nodePlayers->iBeginDiamond;
	pOneLog->iEndDiamond = nodePlayers->iDiamond;
	pOneLog->iExtraAddMoney = nodePlayers->iExtraAddMoney;
	pOneLog->iExtraLoseMoney = nodePlayers->iExtraLoseMoney;
	strcpy(pOneLog->cExtraMoneyDetail, nodePlayers->cExtraMoneyDetail);
	if (nodePlayers->bIfDis == true)
	{
		pOneLog->iIfDisConnect = 1;
	}
	if (pGameLogInfo != NULL)
	{
		if (pGameLogInfo->iUserID == nodePlayers->iUserID)
		{
			pOneLog->iWinMoney = pGameLogInfo->iWinMoney;
			pOneLog->iLoseMoeny = pGameLogInfo->iLoseMoeny;
			pOneLog->iExtraAddMoney += pGameLogInfo->iExtraAddMoney;
			pOneLog->iExtraLoseMoney += pGameLogInfo->iExtraLoseMoney;
			//strcat(pOneLog->cExtraMoneyDetail, pGameLogInfo->cExtraMoneyDetail);
			pOneLog->iGameRank = pGameLogInfo->iGameRank;
			strcpy(pOneLog->cStrInfo1, pGameLogInfo->cStrInfo1);
			strcpy(pOneLog->cStrInfo2, pGameLogInfo->cStrInfo2);
			strcpy(pOneLog->cStrInfo3, pGameLogInfo->cStrInfo3);
			strcpy(pOneLog->cStrInfo4, pGameLogInfo->cStrInfo4);
			strcpy(pOneLog->cStrInfo5, pGameLogInfo->cStrInfo5);
			strcpy(pOneLog->cStrInfo6, pGameLogInfo->cStrInfo6);
			pOneLog->iNumInfo2 = pGameLogInfo->iNumInfo2;
			pOneLog->iNumInfo3 = pGameLogInfo->iNumInfo3;
			pOneLog->iNumInfo4 = pGameLogInfo->iNumInfo4;
			pOneLog->iNumInfo5 = pGameLogInfo->iNumInfo5;
			pOneLog->iNumInfo6 = pGameLogInfo->iNumInfo6;
		}
	}
	_log(_DEBUG, "GL", "SendGameOneLogInfo user[%d] iTableMoney[%d] winMoney[%lld] iLoseMoeny[%lld]", nodePlayers->iUserID, pOneLog->iTableMoney, pOneLog->iWinMoney, pOneLog->iLoseMoeny);
	m_iGameLogBuffLen += sizeof(GameLogOnePlayerLogDef);

	if (m_pQueToLog)
	{
		m_pQueToLog->EnQueue(m_cGameLogBuff, m_iGameLogBuffLen);
	}

	//这里判断下任务相关
	if (m_pTaskLogic)
	{
		m_pTaskLogic->JudgeUserRedisTask(pTableItem, nodePlayers);
	}
	
	CheckUserDecorateTm(nodePlayers);
	
	if (m_pTaskLogic)
	{
		m_pTaskLogic->JudgeRdComTask(pTableItem, nodePlayers);
	}

	//公用统计数据
	SetUserBaseStat(nodePlayers);
}

void GameLogic::HandleTestNetMsg(int iUserID, void *pMsgData)
{
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}

	TestNetMsgDef *msg = (TestNetMsgDef*)pMsgData;

	//第一测试包直接返回
	if (msg->cType == 0)
	{
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, msg, TEST_NET_MSG, sizeof(TestNetMsgDef));
		return;
	}
	//第二包处理
	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomIndex = nodePlayers->cRoomNum - 1;
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iTableNumExtra < 0 || iTableNumExtra > m_iMaxPlayerNum || iRoomIndex < 0 || iRoomIndex > 9)
	{
		return;
	}

	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (!pTableItem)
		return;

	int iTestNetTime = msg->iDelayTime;
	if (iTestNetTime > m_pServerBaseInfo->iTestNet)
	{
		//nodePlayers->iTallNetTime++;//记录高延时的次数
		_log(_ERROR, "GL", "HandleTestNetMsg: TestNetTime=[%d],iUserID=[%d],UserName=[%s]", iTestNetTime, nodePlayers->iUserID, nodePlayers->szUserName);
	}

	msg->cTableNumExtra = nodePlayers->cTableNumExtra;
	for (int i = 0; i<10; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->cDisconnectType == 0)//modify by crystal 11/20
		{
			CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, msg, TEST_NET_MSG, sizeof(TestNetMsgDef));
		}
	}
}

void GameLogic::SendGameReplayData(FactoryTableItemDef *pTableItem)
{
	//首先发送GAME_NUM更新消息
	GameUpdateLastGameNumDef msgSet;
	memset(&msgSet, 0x00, sizeof(GameUpdateLastGameNumDef));
	msgSet.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgSet.msgHeadInfo.iMsgType = GAME_UPDATE_LAST_GAMENUM;
	msgSet.iGameID = m_pServerBaseInfo->iGameID;
	msgSet.iServerID = m_pServerBaseInfo->iServerID;
	strcpy(msgSet.szGameName, pTableItem->cReplayGameNum);
	if (m_iUpdateGameNum % 50 == 0 || m_pServerBaseInfo->iServerState != 1)//为了减轻数据的压力，每50局或服务器进入关闭/维护状态后发送一次
	{
		m_iUpdateGameNum = 0;
		if (m_pQueToRoom)
		{
			m_pQueToRoom->EnQueue(&msgSet, sizeof(GameUpdateLastGameNumDef));
		}
	}
	m_iUpdateGameNum++;
}

void GameLogic::HandleUrgeCard(int iUserID, void *pMsgData)
{
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}
	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomIndex = nodePlayers->cRoomNum - 1;

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomIndex < 0 || iRoomIndex > 9)
	{
		_log(_ERROR, "GL", "HandleUrgeCard:Error user[%d]Name[%s],iRoomIndex[%d]Table[%d]", nodePlayers->iUserID, nodePlayers->szUserName, iRoomIndex, iTableNum);
		return;
	}
	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (!pTableItem)
		return;
	UrgeCardDef *msg = (UrgeCardDef *)pMsgData;

	for (int i = 0; i<10; i++)
	{
		if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->cDisconnectType == 0)//modify by crystal 11/20
		{
			CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, msg, URGR_CARD_MSG, sizeof(UrgeCardDef));
		}
	}
}

void GameLogic::C36STRAddOne(char* pStr, int iLength)
{
	if (iLength <= 0)
		return;
	if (0 == pStr[iLength - 1])
	{
		pStr[iLength - 1] = 0x30;
	}
	pStr[iLength - 1] += 1;
	if (0x3A == pStr[iLength - 1])
	{
		pStr[iLength - 1] = 0x61;
	}
	if ((0x5B == pStr[iLength - 1]) || (0x7B == pStr[iLength - 1]))
	{
		pStr[iLength - 1] = 0x30;
		if (0 == iLength - 1)
		{
			//数组溢出了
			pStr[iLength - 1] = '-';//如果字符串有“-”时表示数字溢出了
			return;
		}
		C36STRAddOne(pStr, iLength - 1);
	}
}

void GameLogic::HandleLeaveReq(int iUserID, void *pMsgData)
{
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}
	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomIndex = nodePlayers->cRoomNum;
	int iSocketIndex = nodePlayers->iSocketIndex;

	int iLeaveType = 0;
	if (pMsgData != NULL)
	{
		LeaveGameDef* pMsgReq = (LeaveGameDef*)pMsgData;
		iLeaveType = pMsgReq->cType;
	}
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomIndex < 0 || iRoomIndex >9)
	{
		_log(_ERROR, "GL", "HandleLeaveReq:Error user[%d][%s],iRoomIndex[%d],Table[%d]", iUserID, nodePlayers->szUserName, iRoomIndex, iTableNum);
		return;
	}
	FactoryTableItemDef *pTableItem = GetTableItem(iRoomIndex, iTableNum - 1);
	if (!pTableItem)
		return;

	_log(_DEBUG, "GL", "HandleLeaveReq Game UserID=[%d],UserName=[%s]", nodePlayers->iUserID, nodePlayers->szUserName);

	nodePlayers->cDisconnectType = 1;

	LeaveGameDef msg;
	memset(&msg, 0, sizeof(LeaveGameDef));
	msg.cType = iLeaveType;
	CLSendSimpleNetMessage(iSocketIndex, &msg, LEAVE_REQ_MSG, sizeof(LeaveGameDef));
}

//iType消息类型，0表示群发所有人 1针对指定分组(一桌/指定编号的一组)群发 2则针对指定端口单发
void GameLogic::CLSendAllClient(void* msg, int iMsgType, int iMsgLen, int iRoomID, int iUserID, int iType, int iAllSocketIndex)
{
	//pMsgHead->cFlag1用于标记群发类型消息类型，0表示群发所有人 1针对指定分组(一桌/指定编号的一组)群发 2则针对指定端口单发
	//pMsgHead->iFlagID就是群发桌ID
	if (iType == 2 && iAllSocketIndex == -1)
	{
		return;
	}
	MsgHeadDef* msgHead = (MsgHeadDef*)msg;
	msgHead->cVersion = MESSAGE_VERSION;
	msgHead->iMsgType = iMsgType;
	msgHead->cFlag1 = iType;
	msgHead->iSocketIndex = iAllSocketIndex;
	msgHead->iFlagID = iRoomID;
	if (iType == 2)
	{
		msgHead->iFlagID = iUserID;
	}
	if (m_pQueToAllClient)
	{
		m_pQueToAllClient->EnQueue(msg, iMsgLen);
	}
	return;
}

bool GameLogic::JudageRoomCloseKickPlayer(FactoryPlayerNodeDef *nodePlayers)
{
	int iKickType = 0;
	if (m_pServerBaseInfo->iServerState == 0 || (m_pServerBaseInfo->iServerState == 2 && (nodePlayers->cVipType != 96 || nodePlayers->iSpeMark == 0)))
	{
		iKickType = m_pServerBaseInfo->iServerState == 0 ? g_iServerClosed : g_iServerMaintain;
	}
	if (iKickType == 0 && nodePlayers->cRoomNum > 0)//再看看房间是不是在维护
	{
		int iRoomIndex = nodePlayers->cRoomNum - 1;
		if (m_pServerBaseInfo->stRoom[iRoomIndex].iRoomState == 0 || m_pServerBaseInfo->stRoom[iRoomIndex].iRoomState == 2 && (nodePlayers->cVipType != 96 || nodePlayers->iSpeMark == 0))
		{
			iKickType = m_pServerBaseInfo->stRoom[iRoomIndex].iRoomState == 0 ? g_iRoomClosed : g_iRoomMaintian;
		}
	}
	if (iKickType == 0 && nodePlayers->cErrorType > 0)
	{
		iKickType = nodePlayers->cErrorType;
	}
	if (iKickType > 0)
	{
		if (m_tmLastCheckKick == 0)
		{
			m_tmLastCheckKick = time(NULL);
		}
		else
		{
			if (time(NULL) - m_tmLastCheckKick > 600)
			{
				if (nodePlayers->iStatus > PS_WAIT_READY)
				{
					_log(_ERROR, "GL", "JudageRoomCloseKickPlayer,iKickType[%d]:[%d][%s][%d] NO Kick[%d]", iKickType, nodePlayers->iUserID, nodePlayers->szUserName, nodePlayers->iStatus, nodePlayers->cRoomNum);
				}
				else if (!nodePlayers->bIfAINode)
				{
					_log(_ERROR, "GL", "JudageRoomCloseKickPlayer,iKickType[%d]:[%d][%s]Kick[%d] status[%d]", iKickType, nodePlayers->iUserID, nodePlayers->szUserName, nodePlayers->cRoomNum, nodePlayers->iStatus);
					SendKickOutMsg(nodePlayers, iKickType, 2,0);
					if (!nodePlayers->bGetLoginOK || nodePlayers->cDisconnectType > 0)  //如果此时还没有收到radius认证信息或者已经掉线了，直接把node节点清掉
					{
						EscapeReqDef pEscape;
						memset(&pEscape, 0, sizeof(pEscape));
						pEscape.cType = 10;
						HandleNormalEscapeReq(nodePlayers, &pEscape);
					}
					return true;
				}
			}
		}
	}
	return false;
}

void GameLogic::UpdateCServerPlayerNum(int iNum, bool bForceSend)//iNum = 1表示增加一个人，iNum = -1表示减少一个人
{
	if (m_pQueToCenterServer != NULL)//是否连接中心服务器
	{
		m_iUpdateCenterServerPlayerNum += iNum;
		int iAllRoomOnline = 0;
		for (int i = 0; i < 10; i++)
		{
			iAllRoomOnline += m_pServerBaseInfo->stRoomOnline[i].iOnlineNum;
		}
		bool bSend = bForceSend;
		if (!bSend)
		{
			if (abs(m_iUpdateCenterServerPlayerNum) > 5)//当服务器人数改变量超过5的时候就更新一下中心服务器的人数
			{
				bSend = true;
			}
			else if (m_pServerBaseInfo->iServerState != 1)//服务进入维护状态后，人数低于10人时，每次更新人数
			{
				if (iAllRoomOnline <= 10)
				{
					bSend = true;
				}
			}
		}
		if (m_pServerBaseInfo->iServerState == 1)
		{
			m_iSendZeroOnlineCnt = 0;
		}
		if (m_pServerBaseInfo->iServerState != 1 && iAllRoomOnline == 0)
		{
			m_iSendZeroOnlineCnt++;
			if (m_iSendZeroOnlineCnt >= 5)
			{
				return;
			}
		}
		if (bSend)
		{
			GameSysOnlineToCenterMsgDef msg;
			memset(&msg, 0, sizeof(GameSysOnlineToCenterMsgDef));
			msg.msgHeadInfo.cVersion = MESSAGE_VERSION;
			msg.msgHeadInfo.iMsgType = GAME_SYS_ONLINE_TO_CENTER_MSG;
			msg.iMaxNum = m_pServerBaseInfo->iMaxPlayerNum;
			msg.iNowPlayerNum = iAllRoomOnline;
			msg.iServerID = m_pServerBaseInfo->iServerID;

			m_pQueToCenterServer->EnQueue(&msg, sizeof(GameSysOnlineToCenterMsgDef));

			_log(_ERROR, "GL", "UpdateCServerPlayerNum,iAllRoomOnline[%d] m_iUpdateCenterServerPlayerNum[%d]", iAllRoomOnline, m_iUpdateCenterServerPlayerNum);

			m_iUpdateCenterServerPlayerNum = 0;
		}
	}
}

void GameLogic::UpdateRoomNum(int iNum, int iRoomIndex, char cLoginType)
{
	if (iRoomIndex >= 0 && iRoomIndex <= 9)
	{
		if (m_pServerBaseInfo->stRoomOnline[iRoomIndex].iRoomType == 0)
		{
			int iRoomType = m_pServerBaseInfo->stRoom[iRoomIndex].iRoomType;
			m_pServerBaseInfo->stRoomOnline[iRoomIndex].iRoomType = iRoomType;
		}
		if (cLoginType == -1)
		{
			//-1--->AI节点
			m_pServerBaseInfo->stRoomOnline[iRoomIndex].iOnlineAI += iNum;
			return;
		}
		m_pServerBaseInfo->stRoomOnline[iRoomIndex].iOnlineNum += iNum;

		//平台类型0Android，1iPhone，2ipad，3微小，4抖小
		if (cLoginType == 0)
		{
			m_pServerBaseInfo->stRoomOnline[iRoomIndex].iOnlineAndroid += iNum;
		}
		else if (cLoginType == 1 || cLoginType == 2)
		{
			m_pServerBaseInfo->stRoomOnline[iRoomIndex].iOnlineIos += iNum;
		}
		else if ( cLoginType == 3)
		{
			m_pServerBaseInfo->stRoomOnline[iRoomIndex].iOnlinePC += iNum;
		}
		else
		{
			m_pServerBaseInfo->stRoomOnline[iRoomIndex].iOnlineOther += iNum;
		}
	}
}

void GameLogic::SetCallBackReadyBaseInfo(FactoryTableItemDef *pTableItem, FactoryPlayerNodeDef *nodePlayers)
{
	int i;
	memset(pTableItem->cReplayData, 0, sizeof(pTableItem->cReplayData));
	pTableItem->iDataLength = 0;

	if (strlen(m_pServerBaseInfo->cReplayGameNum) == 0)
	{
		strcpy(m_pServerBaseInfo->cReplayGameNum, "0000000000");
	}
	char cReplayGNumtemp[20];
	memset(cReplayGNumtemp, 0, sizeof(cReplayGNumtemp));
	memcpy(cReplayGNumtemp, m_pServerBaseInfo->cReplayGameNum, sizeof(m_pServerBaseInfo->cReplayGameNum));
	C36STRAddOne(cReplayGNumtemp, strlen(cReplayGNumtemp));
	strcpy(pTableItem->cReplayGameNum, cReplayGNumtemp);
	strcpy(m_pServerBaseInfo->cReplayGameNum, cReplayGNumtemp);
	_log(_DEBUG, "GL", "SetCallBackReadyBaseInfo:cReplayGameNum[%s]", m_pServerBaseInfo->cReplayGameNum);
	SendGameReplayData(pTableItem);
	pTableItem->iTableID = nodePlayers->cTableNum;
	for (i = 0; i < 10; i++)
	{
		pTableItem->bIfReady[i] = 0;
		if (pTableItem->pFactoryPlayers[i])
		{
			sprintf(pTableItem->pFactoryPlayers[i]->cLastGameNum, "%d-%s", m_pServerBaseInfo->iServerID, m_pServerBaseInfo->cReplayGameNum);
		}
	}
	//桌信息在这里初始化
	pTableItem->iReadyNum = 0;
}

void GameLogic::SetAgainLoginBaseNew(AgainLoginResBaseDef *pMsg, FactoryTableItemDef *pTableItem, FactoryPlayerNodeDef *nodePlayers, int iSocketIndex)
{
	int i;
	int iTableNum = nodePlayers->cTableNum;   //找出所在的桌
	int iTableNumExtra = nodePlayers->cTableNumExtra;

	//判断iSocketIndex为-1的时候其实不是重入，仅仅只是再次请求入座而已
	if (iSocketIndex != -1)
	{
		if (nodePlayers->iSocketIndex != -1)//掉线重入还没检测到断线就进来，或者可能是重复登陆,先把前一个的移出HASH
		{
			SendKickOutMsg(nodePlayers, g_iRepeatLoginError, 1, 0);
			hashIndexTable.Remove((void *)&(nodePlayers->iSocketIndex), sizeof(int));
		}
		//新的加入socketIndex哈系表
		hashIndexTable.Add((void *)(&(iSocketIndex)), sizeof(int), nodePlayers);
		nodePlayers->iSocketIndex = iSocketIndex;
	}
	nodePlayers->iWaitLoginTime = 0;//add at 3/31
	nodePlayers->bIfWaitLoginTime = false;
	nodePlayers->cDisconnectType = 0;//掉线重入清0
	if (nodePlayers->iLeaveGameTime > 0)
	{
		nodePlayers->iGameLeaveSec += (time(NULL) - nodePlayers->iLeaveGameTime);
	}
	nodePlayers->iLeaveGameTime = 0;
	if (pMsg == NULL)
	{
		return;
	}
	if (pTableItem)
	{
		GetTablePlayerInfo(pTableItem, nodePlayers);	//获取桌玩家信息	
	}

	pMsg->iServerTime = time(NULL);
	pMsg->cTableNum = iTableNum;
	pMsg->cTableNumExtra = iTableNumExtra;
	pMsg->iFirstMoney = nodePlayers->iFirstMoney;
	pMsg->iDiamod = nodePlayers->iDiamond;
	pMsg->iRoomType = nodePlayers->iEnterRoomType;
	pMsg->iTotalCharge = nodePlayers->iTotalCharge;
	if (pTableItem)
	{
		pMsg->cPlayerNum = pTableItem->cPlayerNum;
		for (int i = 0; i < 10; i++)
		{
			if (pTableItem->pFactoryPlayers[i] && pTableItem->pFactoryPlayers[i]->bIfWaitLoginTime)
			{
				pMsg->bOffLine[i] = true;
			}
		}
		sprintf(pMsg->cGameNum, "%d%s", m_pServerBaseInfo->iServerID, pTableItem->cReplayGameNum);//把游戏编号发送给玩家 add skyhawk 

		for (i = 0; i < 10; i++)
		{
			if (pTableItem->pFactoryPlayers[i] && (pTableItem->pFactoryPlayers[i]->bIfWaitLoginTime == true || pTableItem->pFactoryPlayers[i]->cDisconnectType == (char)UserDisconnectType::LEAVE_BY_SELF))
			{
				pMsg->bOffLine[i] = 1;
			}
		}
		LoginAgainNoticeDef msgLoginAgain;
		memset(&msgLoginAgain, 0, sizeof(LoginAgainNoticeDef));
		msgLoginAgain.cTableNumExtra = iTableNumExtra;
		for (i = 0; i<m_iMaxPlayerNum; i++)
		{
			if (pTableItem->pFactoryPlayers[i] && (pTableItem->pFactoryPlayers[i]->cTableNumExtra != iTableNumExtra))//自己不发送了
			{
				CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgLoginAgain, LOGIN_AGAIN_NOTICE_MSG, sizeof(LoginAgainNoticeDef));
			}
		}
	}	
}

int GameLogic::SetAccountReqRadiusDef(GameUserAccountReqDef* pMsg, GameUserAccountGameDef* pGameAccount, FactoryTableItemDef* pTableItem, FactoryPlayerNodeDef* nodePlayers,long long  llGameAmount,long long  llExtraAddAmount,long long  llExtraDecAmount, int iAddDayCnt/* = 1*/)
{
	pMsg->iUserID = nodePlayers->iUserID;
	pMsg->iGameID = m_pServerBaseInfo->iGameID;
	pMsg->iServerID = m_pServerBaseInfo->iServerID;

	nodePlayers->iAllNum++;
	pGameAccount->iAllNum = 1;

	//底层统一赋值总净分llGameAmount
	pGameAccount->llGameAmount = pMsg->llRDMoney;

	if (llGameAmount < 0)//本局输
	{
		long long llAbsAmount = abs(llGameAmount);
		pGameAccount->iLoseNum = 1;
		nodePlayers->iLoseNum++;
		pGameAccount->llLoseMoney = llAbsAmount;
		nodePlayers->iLoseMoney += llAbsAmount;
		nodePlayers->iDayLoseNum++;
		nodePlayers->iDayLoseMoney += llAbsAmount;
		nodePlayers->iRecentLoseCnt++;
		nodePlayers->iRecentLoseMoney += llAbsAmount;
	}
	else//赢分
	{
		pGameAccount->iWinNum = 1;
		nodePlayers->iWinNum++;
		pGameAccount->llWinMoney = llGameAmount;
		nodePlayers->iWinMoney += llGameAmount;
		nodePlayers->iDayWinNum++;
		nodePlayers->iDayWinMoney += llGameAmount;
		nodePlayers->iRecentWinCnt++;
		nodePlayers->iRecentWinMoney += llGameAmount;
	}
	if (llExtraAddAmount > 0)
	{
		nodePlayers->iWinMoney += llExtraAddAmount;
		nodePlayers->iDayWinMoney += llExtraAddAmount;
		nodePlayers->iRecentWinMoney += llExtraAddAmount;
	}
	if (llExtraDecAmount > 0)
	{
		nodePlayers->iLoseMoney += llExtraDecAmount;
		nodePlayers->iDayLoseMoney += llExtraDecAmount;
		nodePlayers->iRecentLoseMoney += llExtraDecAmount;
	}
	nodePlayers->iDayAllNum++;
	nodePlayers->iRecentPlayCnt++;
	nodePlayers->llGameRtAmount = llGameAmount;

	nodePlayers->iIntegral += pMsg->iRDIntegral;
	nodePlayers->iHisIntegral += pMsg->iRDIntegral;
	nodePlayers->iDayIntegral += pMsg->iRDIntegral;
	nodePlayers->iDiamond += pMsg->iRDDiamond;

	//刷新当日游戏局数
	if (iAddDayCnt > 0)
	{
		RdUserDayInfoMsgReqDef msgReq;
		memset(&msgReq, 0, sizeof(msgReq));
		msgReq.msgHeadInfo.iMsgType = RD_USER_DAY_INFO_MSG;
		msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgReq.iAddGameCnt += iAddDayCnt;
		msgReq.iUserID = nodePlayers->iUserID;
		msgReq.iGameID = m_pServerBaseInfo->iGameID;
		if (m_pQueToRedis)
		{
			m_pQueToRedis->EnQueue(&msgReq, sizeof(RdUserDayInfoMsgReqDef));
		}
	}	
}

void GameLogic::SendSpecialAcount(FactoryPlayerNodeDef* nodePlayers, int iMoneyType, int iAddNum, int iLogType, int	iLogExtraInfo)
{
	if (nodePlayers == NULL)
	{
		return;
	}
	GameSpecialAccontMsgDef msgReq;
	memset(&msgReq, 0, sizeof(GameSpecialAccontMsgDef));
	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq.msgHeadInfo.iMsgType = GAME_SPECIAL_ACCOUNT_MSG;
	msgReq.iUserID = nodePlayers->iUserID;
	msgReq.iGameID = m_pServerBaseInfo->iGameID;
	msgReq.iServerID = m_pServerBaseInfo->iServerID;
	memcpy(msgReq.szUserToken, nodePlayers->szUserToken, 32);
	msgReq.iMoneyType = iMoneyType;
	msgReq.iAddNum = iAddNum;

	int iRoomIndex = nodePlayers->cRoomNum - 1;
	int iRoomTypeIdx = m_pServerBaseInfo->stRoom[iRoomIndex].iRoomType - 5;

	if (iMoneyType == (int)MonetaryType::FIRST_MONEY) //货币相关的事件统计待统一调整
	{
		nodePlayers->iFirstMoney += iAddNum;
		msgReq.iNowNum = nodePlayers->iFirstMoney;
		if (iAddNum > 0)
		{
			//AddComEventData(1, EVENT_1::SEND_MONEY, iAddNum);
		}
		else
		{
			//AddComEventData(1, EVENT_1::RECYCLE_MONEY, -iAddNum);
		}
	}
	else if (iMoneyType == (int)MonetaryType::Money)
	{
		nodePlayers->iMoney += iAddNum;
		msgReq.iNowNum = nodePlayers->iMoney;
		if (iAddNum > 0)
		{
			nodePlayers->iExtraAddMoney += iAddNum;
			AddComEventData(1, EVENT_1::SEND_MONEY, iAddNum);
		}
		else
		{
			nodePlayers->iExtraLoseMoney -= iAddNum;
			AddComEventData(1, EVENT_1::RECYCLE_MONEY, -iAddNum);
		}
		AddExtraMoneyDetail(nodePlayers, iLogType, iAddNum);
	}
	else if (iMoneyType == (int)MonetaryType::Diamond)
	{
		nodePlayers->iDiamond += iAddNum;
		msgReq.iNowNum = nodePlayers->iDiamond;
		if (iAddNum > 0)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iSendDiamond += iAddNum;
			AddComEventData(1, EVENT_1::SEND_DIAMOND, iAddNum);
			AddComEventData(10, 6, iAddNum);
		}
		else
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iUseDiamond -= iAddNum;
			AddComEventData(1, EVENT_1::RECYCLE_DIAMOND, -iAddNum);
		}
	}
	else if (iMoneyType == (int)MonetaryType::Integral)
	{
		nodePlayers->iIntegral += iAddNum;
		nodePlayers->iHisIntegral += iAddNum;
		nodePlayers->iDayIntegral += iAddNum;
		msgReq.iNowNum = nodePlayers->iIntegral;
		if (iLogType == (int)ComGameLogType::GAME_TASK_SEND_INTEGRAL)
		{
			nodePlayers->iDayTaskIntegral += iAddNum;
		}
		if (iAddNum > 0)
		{
			nodePlayers->iGameGetIntegral += iAddNum;
		}
		if (iAddNum > 0)
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iSendIntegral += iAddNum;
			AddComEventData(1, EVENT_1::SEND_INTEGRAL, iAddNum);
		}
		else
		{
			m_stGameBaseStatInfo.stRoomStat[iRoomTypeIdx].iUseIntegral -= iAddNum;
			AddComEventData(1, EVENT_1::RECYCLE_INTEGRAL, -iAddNum);
		}
	}

	if (m_pQueToRadius)
	{
		int iRt = m_pQueToRadius->EnQueue(&msgReq, sizeof(GameSpecialAccontMsgDef));
		if (iRt < 0)
		{
			_log(_ERROR, "GL", "SendSpecialAcount user[%d] iMoneyType[%d] iAddNum[%d]", nodePlayers->iUserID, iMoneyType, iAddNum);
		}
	}

	//发送日志
	SendMainMonetaryLog(nodePlayers->iUserID, nodePlayers->szUserName, iMoneyType, iAddNum, msgReq.iNowNum - iAddNum, iLogType, iLogExtraInfo);
}

void GameLogic::AddExtraMoneyDetail(FactoryPlayerNodeDef *nodePlayers, int iLogType, int iAddNum)
{
	if (iAddNum == 0)
	{
		return;
	}
	//char cExtra[16] = { 0 };
	//sprintf(cExtra, "%d_%d&", iLogType, iAddNum);
	//strcat(nodePlayers->cExtraMoneyDetail, cExtra);
}

void GameLogic::SetUserNameHide(FactoryPlayerNodeDef *nodePlayers, char *szUserName)//隐藏玩家名称
{
	if (nodePlayers == NULL)
		return;

	char szTemp[4];
	memset(szTemp, 0, sizeof(szTemp));
	sprintf(szTemp, "\"");
	char szBullTemp[128];  //这个定义长度要长点
	char szNickTemp[64];
	memset(szNickTemp, 0, sizeof(szNickTemp));
	memset(szBullTemp, 0, sizeof(szBullTemp));
	strcpy(szNickTemp, nodePlayers->szNickName);

	int iCode = GlobalMethod::code_convert("GBK", "utf-8", szNickTemp, 64, szBullTemp, sizeof(szBullTemp));

	if (strcmp(nodePlayers->szNickName, "N") != 0 && iCode == 0)
	{
		strcat(szUserName, szTemp);
		strcat(szUserName, nodePlayers->szNickName);
		strcat(szUserName, szTemp);
	}
	else
	{
		char cUserNameTemp[20];
		memset(cUserNameTemp, 0, sizeof(cUserNameTemp));
		if (strlen(nodePlayers->szUserName) >0)//隐藏玩家的部分姓名
		{
			if (strlen(nodePlayers->szUserName) >= 4)
			{
				memcpy(cUserNameTemp, nodePlayers->szUserName, 2);
				memcpy(cUserNameTemp + 2, "**", 2);
				memcpy(cUserNameTemp + 4, nodePlayers->szUserName + strlen(nodePlayers->szUserName) - 2, 2);
			}
			else
			{
				memcpy(cUserNameTemp, nodePlayers->szUserName, strlen(nodePlayers->szUserName) - 1);
				strcat(cUserNameTemp, "**");
			}
		}
		strcat(szUserName, szTemp);
		strcat(szUserName, cUserNameTemp);
		strcat(szUserName, szTemp);
	}
}

void GameLogic::AgainLoginOtherHandle(FactoryPlayerNodeDef *nodePlayers)//掉线重入后的其他处理函数
{
	if (nodePlayers == NULL)
		return;
}

void GameLogic::HandleUserGameChargeAndRefresh(void *pMsgData)
{
	UserChargeAndRefreshReqDef* msgReq = (UserChargeAndRefreshReqDef*)pMsgData;

	int iUserID = msgReq->iUserID;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "GameLogic", "HandleUserGameChargeAndRefresh  nodePlayers null");
		return;
	}

	int iTableNum = nodePlayers->cTableNum;
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomNum = nodePlayers->cRoomNum;
	int iTableItemIndex = nodePlayers->cTableNumExtra;

	_log(_DEBUG, "GL", " HandleUserGameChargeAndRefresh userid = %d iFlag = %d", msgReq->iUserID, msgReq->iFlag);
	if (msgReq->iFlag == 0)
	{
		SendGameRefreshUserInfoReq(nodePlayers, false);
	}

	//这里再刷新一下道具信息
	GameUserProReqDef propReq;
	memset(&propReq, 0, sizeof(GameUserProReqDef));
	propReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	propReq.msgHeadInfo.iMsgType = GAME_USER_PROP_INFO_MSG;
	propReq.iUserID = nodePlayers->iUserID;
	propReq.iFlag = 1;
	if (m_pQueToRadius)
	{
		m_pQueToRadius->EnQueue(&propReq, sizeof(GameUserInfoSysReqDef));
	}
}

void GameLogic::SendGameRefreshUserInfoReq(FactoryPlayerNodeDef *nodePlayers, bool bNeedConvertCharge, bool bNeedAccount /*= true*/)//发送游戏中刷新用户信息请求
{
	if (bNeedAccount)
	{
		GameUserAccountReqDef msgRadius;
		memset(&msgRadius, 0, sizeof(GameUserAccountReqDef));
		msgRadius.iReqFlag |= 1 << 1;   //跳过计费同步校验

		GameUserAccountGameDef msgGameAccount;
		memset(&msgGameAccount, 0, sizeof(GameUserAccountGameDef));

		FillRDAccountReq(&msgRadius, &msgGameAccount, nodePlayers, 0);
		RDSendAccountReq(&msgRadius, &msgGameAccount, true);//先强制计费掉
	}

	GameUserInfoSysReqDef msgReq;
	memset(&msgReq, 0, sizeof(GameUserInfoSysReqDef));

	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq.msgHeadInfo.iMsgType = GAME_USER_MAIN_INFO_SYS_MSG;
	msgReq.iUserID = nodePlayers->iUserID;
	msgReq.iIfConvertCharge = bNeedConvertCharge ? 1 : 0;

	if (m_pQueToRadius)
	{
		m_pQueToRadius->EnQueue(&msgReq, sizeof(GameUserInfoSysReqDef));
	}
}

void GameLogic::HandleGameRefreshUserInfoRes(void *pMsgData)//游戏中刷新用户信息
{
	GameUserInfoSysResDef* pMsgRes = (GameUserInfoSysResDef*)pMsgData;
	int iUserID = pMsgRes->iUserID;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "GL", "HandleGameRefreshUserInfoRes  nodePlayers null");
		return;
	}
	long long iExtraMoney = pMsgRes->iMoney - nodePlayers->iMoney;
	if (iExtraMoney > 0)
		nodePlayers->iExtraAddMoney = iExtraMoney;
	else
		nodePlayers->iExtraLoseMoney = iExtraMoney;

	AddExtraMoneyDetail(nodePlayers, 0, iExtraMoney);
	nodePlayers->iFirstMoney = pMsgRes->iFirstMoney;
	nodePlayers->iMoney = pMsgRes->iMoney;
	nodePlayers->iIntegral = pMsgRes->iIntegral;
	nodePlayers->iDiamond = pMsgRes->iDiamond;
	nodePlayers->iTotalCharge = pMsgRes->iTotalCharge;
	nodePlayers->iHeadImg = pMsgRes->iHeadImg;

	_log(_DEBUG, "GL", "HandleGameRefreshUserInfoRes,user[%d],money[%lld],intrgral[%d],diamond[%d],charge[%d],temp[%d][%d,%d,%d]",
		iUserID, nodePlayers->iMoney, nodePlayers->iIntegral, nodePlayers->iDiamond, nodePlayers->iTotalCharge, pMsgRes->iIfConvertCharge,
		pMsgRes->iTempChargeMoney, pMsgRes->iTempChargeDiamond, pMsgRes->iTempChargeFirstMoney);

	CallBackGameRefreshUserInfo(nodePlayers, pMsgRes->iIfConvertCharge);

	char cBuff[512];
	memset(cBuff, 0, sizeof(cBuff));
	int iBuffLen1 = sizeof(SysClientUserMainInfoDef);
	int iBuffLen2 = sizeof(SysClientUserSelfMainInfoDef);
	SysClientUserMainInfoDef* pMsgMainInfo = (SysClientUserMainInfoDef*)cBuff;
	SysClientUserSelfMainInfoDef* pMsgSelfInfo = (SysClientUserSelfMainInfoDef*)(cBuff + iBuffLen1);
	pMsgMainInfo->iUserID = iUserID;
	pMsgMainInfo->iHeadImg = nodePlayers->iHeadImg;
	pMsgMainInfo->iMoney = nodePlayers->iMoney;

	pMsgSelfInfo->iDiamond = nodePlayers->iDiamond;
	pMsgSelfInfo->iIntegral = nodePlayers->iIntegral;
	pMsgSelfInfo->iTotalCharge = nodePlayers->iTotalCharge;
	pMsgSelfInfo->iIfConvertCharge = pMsgRes->iIfConvertCharge;
	pMsgSelfInfo->iTempChargeMoney = pMsgRes->iTempChargeMoney;
	pMsgSelfInfo->iTempChargeDiamond = pMsgRes->iTempChargeDiamond;
	pMsgSelfInfo->iTempChargeFirstMoney = pMsgRes->iTempChargeFirstMoney;

	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, cBuff, SYS_CLIENT_USER_MAIN_INFO, iBuffLen1 + iBuffLen2);
	if (nodePlayers->cTableNum > 0)
	{
		FactoryTableItemDef* pTableItem = GetTableItem(nodePlayers->cRoomNum - 1, nodePlayers->cTableNum - 1);
		if (pTableItem)
		{
			for (int i = 0; i < 10; i++)
			{
				if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->iUserID != iUserID)
				{
					CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, cBuff, SYS_CLIENT_USER_MAIN_INFO, iBuffLen1);
				}
			}
		}
	}
}

void GameLogic::SetClientSockTableID(int iSocketIndex, int iTableID)//设置主线程Socket的桌(组)节点编号（iTableID:0-255）
{
	if (iTableID < 0)
	{
		_log(_ERROR, "GLT", "SetClientSockTableID  error:iTableID < 0  iTableID=%d", iTableID);
		return;
	}
	if (m_pClientEpoll)
	{
		//_log(_DEBUG,"SetClientALLSockTableID","iSocketIndex=%d,iTableID=%d",iSocketIndex,iTableID);
		m_pClientEpoll->SetNodeTableID(iSocketIndex, iTableID);
	}
}

void GameLogic::SetClientALLSockTableID(int iSocketIndex, int iTableID)//设置群发线程Socket的桌(组)节点编号（iTableID:0-255）
{
	if (iTableID < 0)
	{
		_log(_ERROR, "GLT", "SetClientALLSockTableID  error:iTableID < 0 || iTableID > 255  iTableID=%d", iTableID);
		return;
	}
	if (m_pClientAllEpoll)
	{
		//_log(_DEBUG,"SetClientALLSockTableID","iSocketIndex=%d,iTableID=%d",iSocketIndex,iTableID);
		m_pClientAllEpoll->SetNodeTableID(iSocketIndex, iTableID);
	}
}

void GameLogic::SendMsgToClientTable(void *pMsg, int iLen, unsigned short iMsgType, int iTableID)//为桌或者组编号
{
	if (iTableID < 0)
	{
		_log(_ERROR, "GLT", "SendMsgToClientTable  error:iTableID < 0 iTableID=%d", iTableID);
		return;
	}

	if (pMsg == NULL)
		return;
	//_log(_DEBUG,"GLT","SendMsgToClientTable iTableID=%d,iMsgType[%x]",iTableID,iMsgType);

	MsgHeadDef *msgHead = (MsgHeadDef*)pMsg;
	msgHead->cVersion = MESSAGE_VERSION;
	msgHead->iMsgType = iMsgType;
	msgHead->cFlag1 = 1;//cFlag1=0正常给指定玩家端口发送消息，=1给指定桌号的群发
	msgHead->iFlagID = iTableID;

	if (iMsgType == GAME_BULL_NOTICE_MSG)
	{
		m_pQueToClient->EnQueue(pMsg, iLen, 0, false);
	}
	else
	{
		m_pQueToClient->EnQueue(pMsg, iLen);
	}
}

void GameLogic::SendMsgToAllClientTable(void *pMsg, int iLen, unsigned short iMsgType, int iTableID)
{
	if (iTableID < 0)
	{
		_log(_ERROR, "GLT", "SendMsgToAllClientTable  error:iTableID < 0 iTableID=%d", iTableID);
		return;
	}

	if (pMsg == NULL)
		return;

	//_log(_DEBUG,"GLT","SendMsgToAllClientTable iTableID=%d,type[%x]",iTableID,iMsgType);
	//cFlag1=0表示群发所有人 1针对指定分组(一桌/指定编号的一组)群发 2则针对指定端口单发
	MsgHeadDef *msgHead = (MsgHeadDef*)pMsg;
	msgHead->cVersion = MESSAGE_VERSION;
	msgHead->iMsgType = iMsgType;
	msgHead->cFlag1 = 1;
	msgHead->iFlagID = iTableID;

	if (m_pQueToAllClient)
	{
		m_pQueToAllClient->EnQueue(pMsg, iLen);
	}
}

void GameLogic::SendGetServerConfParams(vector<string>& vcKey)
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
			pMsgReq->msgHeadInfo.iMsgType = GAME_GET_PARAMS_MSG;
		}
		memcpy(cBuff + iBuffLen, vcKey[i].c_str(), 32);
		iBuffLen += 32;
		iSendNum++;
		if (iSendNum >= 12)
		{
			if (m_pQueToRadius)
			{
				pMsgReq->iKeyNum = iSendNum;
				int iRt = m_pQueToRadius->EnQueue(cBuff, iBuffLen);
				iSendNum = 0;
				iBuffLen = 0;
			}
		}
	}
	if (iSendNum > 0)
	{
		if (m_pQueToRadius)
		{
			pMsgReq->iKeyNum = iSendNum;
			int iRt = m_pQueToRadius->EnQueue(cBuff, iBuffLen);
		}
	}
}

void GameLogic::HandleGetParamsRes(void *pMsgData)
{
	GameGetParamsResDef* msgRes = (GameGetParamsResDef*)pMsgData;
	_log(_DEBUG, "GL", "HandleGetParamsRes GetPropDiamond Conf END keyNum[%d] !!!!!", msgRes->iKeyNum);

	char cTmpKey[32] = { 0 };
	char cTmpValue[128] = { 0 };
	for (int i = 0; i < msgRes->iKeyNum; i++)
	{
		memset(cTmpKey, 0, sizeof(cTmpKey));
		memcpy(cTmpKey, (char*)pMsgData + sizeof(GameGetParamsResDef) + i * (32 + 128), 32);

		memset(cTmpValue, 0, sizeof(cTmpValue));
		memcpy(cTmpValue, (char*)pMsgData + sizeof(GameGetParamsResDef) + i * (32 + 128) + 32, 128);

		if (strcmp(cTmpKey, "prop_diamond_value") == 0 || strcmp(cTmpKey, "prop_diamond_value1") == 0)
		{
			vector<string>vcStrOut;
			GlobalMethod::CutString(vcStrOut, cTmpValue, "&");

			for (int i = 0; i < vcStrOut.size(); i++)
			{
				int iPropID = 0;
				int iPropValue = 0;
				int iRt = sscanf(vcStrOut[i].c_str(), "%d_%d", &iPropID, &iPropValue);
				if (iRt == 2)
				{
					m_mapPropDiamodValue[iPropID] = iPropValue;
				}
			}
		}
		_log(_DEBUG, "GL", "HandleGetParamsRes cTmpKey[%s] cTmpValue[%s]", cTmpKey, cTmpValue);
	}
}

void GameLogic::HandleClientUseInteractPropReq(void *pMsgData)
{
	UseInteractPropReqDef* msgReq = (UseInteractPropReqDef*)pMsgData;

	int iUserID = msgReq->iUserID;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "GL", "HandleClientUseInteractPropReq  nodePlayers null");
		return;
	}

	bool bFindToUser = false;
	FactoryTableItemDef *pTableItem = NULL;

	FactoryTableItemDef tmpTable;
	memset(&tmpTable, 0, sizeof(FactoryTableItemDef));
	if (nodePlayers->cTableNum == 0 && m_pNewAssignTableLogic != NULL)  //还没有桌子时
	{
		VirtualTableDef* pVTable = m_pNewAssignTableLogic->FindVTableInfo(abs(nodePlayers->iVTableID));
		if (pVTable == NULL)
		{
			int iIndex = 0;
			pVTable = m_pNewAssignTableLogic->FindAIControlVTableInfo(nodePlayers->iUserID, iIndex);
		}
		if (pVTable)
		{
			for (int i = 0; i < 10; i++)
			{
				if (pVTable->iUserID[i] > 0 && pVTable->pNodePlayers[i] != NULL)
				{
					tmpTable.pFactoryPlayers[i] = pVTable->pNodePlayers[i];
				}
				if (pVTable->iUserID[i] == msgReq->iUseToUserID)
				{
					bFindToUser = true;
				}
			}
		}
		if (bFindToUser)
		{
			pTableItem = &tmpTable;
		}
	}
	if (pTableItem == NULL)  //没找到虚拟桌子，找一下实体桌
	{
		if (nodePlayers->cRoomNum < 1)
		{
			_log(_ERROR, "GL", "HandleClientUseInteractPropReq:Error [%d]Name[%s]cRoomNum[%d]", nodePlayers->iUserID, nodePlayers->szUserName, nodePlayers->cRoomNum);
			return;
		}
		int iTableNum = nodePlayers->cTableNum;
		int iTableNumExtra = nodePlayers->cTableNumExtra;

		if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM)
		{
			_log(_ERROR, "GL", "HandleClientUseInteractPropReq:Error [%d]Name[%s]Table[%d]", nodePlayers->iUserID, nodePlayers->szUserName, iTableNum);
			return;
		}
		pTableItem = GetTableItem(nodePlayers->cRoomNum - 1, iTableNum - 1);
		if (!pTableItem)
			return;

		//向各个客户端通知聊天信息
		for (int i = 0; i < 10; i++)
		{
			if (pTableItem->pFactoryPlayers[i] != NULL && pTableItem->pFactoryPlayers[i]->iUserID == msgReq->iUseToUserID)
			{
				bFindToUser = true;
				break;
			}
		}
	}

	if (!bFindToUser)
	{
		_log(_ERROR, "GL", "HandleClientUseInteractPropReq bFindToUser[%d]=false", msgReq->iUseToUserID);
		return;
	}
	int iRoomIndex = nodePlayers->cRoomNum - 1;
	int iRoomTypeIdx = m_pServerBaseInfo->stRoom[iRoomIndex].iRoomType - 5;

	UseInteractPropResDef msgRes;
	memset(&msgRes, 0, sizeof(UseInteractPropResDef));
	msgRes.iUserID = msgReq->iUserID;
	msgRes.iPropID = msgReq->iPropID;
	msgRes.iUseToUserID = msgReq->iUseToUserID;
	msgRes.iUseType = msgReq->iUseType;
	_log(_DEBUG, "GL", "HandleClientUseInteractPropReq iUseProp[%d],iUseType[%d]", msgReq->iPropID, msgReq->iUseType);

	if (msgReq->iUseType == 1)//1使用现有数量 2使用钻石
	{
		GameUserOnePropDef* pPropInfo = GetUserPropInfo(nodePlayers, msgReq->iPropID);
		if (pPropInfo == NULL || pPropInfo->iPropNum < 1)
		{
			msgRes.iErrRt = 1;
			CLSendSimpleNetMessage(nodePlayers->iSocketIndex, (void*)&msgRes, USE_INTERACT_PROP, sizeof(UseInteractPropResDef));
			return;
		}
		msgRes.iDecNum = 1;
		AddUserProp(nodePlayers, msgReq->iPropID, -1, pPropInfo->iLastTime);
		nodePlayers->iGameMagicCnt++;
		SendPropLog(nodePlayers->iUserID, nodePlayers->szUserName, msgReq->iPropID, -1, pPropInfo->iPropNum + 1, (int)ComGameLogType::GAME_USE_PROP, 0);
	}
	if (msgReq->iUseType == 2)
	{
		int iPropMoneyValue = GetPropDiamondValue(msgReq->iPropID);//没有找到配置内有，默认就用2个
		if (iPropMoneyValue == -1)
		{
			iPropMoneyValue = 2;
		}
		if (nodePlayers->iDiamond < iPropMoneyValue)
		{
			msgRes.iErrRt = 2;
			CLSendSimpleNetMessage(nodePlayers->iSocketIndex, (void*)&msgRes, USE_INTERACT_PROP, sizeof(UseInteractPropResDef));
			return;
		}
		msgRes.iDecNum = iPropMoneyValue;
		//钻石特殊计费
		nodePlayers->iGameMagicCnt++;
		SendSpecialAcount(nodePlayers, (int)MonetaryType::Diamond, -iPropMoneyValue, (int)ComGameLogType::GAME_SPECIAL_ACCOUNT, msgReq->iPropID);
		AddComEventData(10, 1, iPropMoneyValue);
		GameUserOnePropDef* pPropInfo = GetUserPropInfo(nodePlayers, msgReq->iPropID);
		int iBeginNum = 0;
		if (pPropInfo != NULL)
		{
			iBeginNum = pPropInfo->iPropNum;
		}
		SendPropLog(nodePlayers->iUserID, nodePlayers->szUserName, msgReq->iPropID, 0, iBeginNum, (int)ComGameLogType::GAME_USE_PROP, iPropMoneyValue);
		m_pGameEvent->AddUesPropData(iRoomTypeIdx, -msgReq->iPropID);
	}

	CallBackInteractPropSucc(msgRes);
	m_pGameEvent->AddUesPropData(iRoomTypeIdx, msgReq->iPropID);

	for (int i = 0; i<10; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgRes, USE_INTERACT_PROP, sizeof(UseInteractPropResDef));
		}
	}
}


void GameLogic::SendPropLog(int iUserID, char* szUserName, int iPropID, int iAddNum, int iBeginNum, int iLogType, int iLogExtraInfo /*= 0*/)
{
	GamePropLogDef msgPropLog;
	memset(&msgPropLog, 0, sizeof(GamePropLogDef));

	msgPropLog.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgPropLog.msgHeadInfo.iMsgType = GAME_PROP_LOG_MSG;
	msgPropLog.iUserID = iUserID;
	strcpy(msgPropLog.szUserName, szUserName);
	msgPropLog.iGameID = m_pServerBaseInfo->iGameID;
	msgPropLog.iServerID = m_pServerBaseInfo->iServerID;
	msgPropLog.iPropID = iPropID;
	msgPropLog.iAddNum = iAddNum;
	msgPropLog.iLogType = iLogType;
	msgPropLog.iBeginNum = iBeginNum;
	msgPropLog.iExtraInfo = iLogExtraInfo;
	if (m_pQueToLog)
	{
		m_pQueToLog->EnQueue(&msgPropLog, sizeof(GamePropLogDef));
	}
}

void GameLogic::SendMainMonetaryLog(int iUserID, char* szUserName, int iMoneyType, long long llAddNum, long long llBeginNum, int iLogType, int iLogExtraInfo/* = 0*/)
{
	GameMainMonetaryLogMsgDef msgMoneyLog;
	memset(&msgMoneyLog, 0, sizeof(GameMainMonetaryLogMsgDef));
	msgMoneyLog.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgMoneyLog.msgHeadInfo.iMsgType = GAME_MAIN_MONETARY_LOG_MSG;
	msgMoneyLog.iUserID = iUserID;
	strcpy(msgMoneyLog.szUserName, szUserName);
	msgMoneyLog.iGameID = m_pServerBaseInfo->iGameID;
	msgMoneyLog.iServerID = m_pServerBaseInfo->iServerID;
	msgMoneyLog.iMoneyType = iMoneyType;
	//msgMoneyLog.iAddNum = llAddNum;	//不用赋值了，用新字段llAddNum代替
	msgMoneyLog.iAddNum = llAddNum;
	msgMoneyLog.iLogType = iLogType;
	msgMoneyLog.llBeginNum = llBeginNum;
	msgMoneyLog.iExtraInfo = iLogExtraInfo;
	if (m_pQueToLog)
	{
		m_pQueToLog->EnQueue(&msgMoneyLog, sizeof(GameMainMonetaryLogMsgDef));
	}
}

void GameLogic::SendGetRoomInfo(bool bOnlyUpdateOnline)
{
	char szBuffer[1024];
	memset(szBuffer, 0, sizeof(szBuffer));
	GameRoomInfoReqRadiusDef* pMsgReq = (GameRoomInfoReqRadiusDef*)szBuffer;
	pMsgReq->msgHeadInfo.iMsgType = GAME_ROOM_INFO_REQ_RADIUS_MSG;
	pMsgReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgReq->iServerType = 0;//0游戏服务，1中心服务器
	pMsgReq->iServerID = m_pServerBaseInfo->iServerID;
	pMsgReq->iNeedRoomInfo = bOnlyUpdateOnline ? 0 : 1;

	int iBuffLen = sizeof(GameRoomInfoReqRadiusDef);
	GameOneRoomOnlineDef* pRoomOnline = (GameOneRoomOnlineDef*)(szBuffer + iBuffLen);
	int iOneStSize = sizeof(GameOneRoomOnlineDef);
	int iRoomNum = 0;
	pRoomOnline->iRoomType = m_pServerBaseInfo->iServerRoomType;
	for (int i = 0; i < 10; i++)
	{
		if (m_pServerBaseInfo->stRoom[i].iRoomType > 0)
		{
			pMsgReq->iRoomNum++;
			iBuffLen += iOneStSize;
			m_pServerBaseInfo->stRoomOnline[i].iRoomType = m_pServerBaseInfo->stRoom[i].iRoomType;
			memcpy(pRoomOnline, &m_pServerBaseInfo->stRoomOnline[i], iOneStSize);
			pRoomOnline++;
		}
	}
	if (m_pQueToRoom)
	{
		m_pQueToRoom->EnQueue(szBuffer, iBuffLen);
	}
}

void GameLogic::GetUserNickName(char* szNickName, FactoryPlayerNodeDef *nodePlayers)
{
	if (strcmp(nodePlayers->szNickName, "N") != 0)
	{
		strcpy(szNickName, nodePlayers->szNickName);
	}
	else
	{
		strcpy(szNickName, nodePlayers->szUserName);
	}
}

void GameLogic::SendGameBaseStatInfo(int iTmFlag)
{
	if (m_pQueToRedis == NULL)
	{
		return;
	}
	m_stGameBaseStatInfo.msgHeadInfo.cVersion = MESSAGE_VERSION;
	m_stGameBaseStatInfo.msgHeadInfo.iMsgType = REDIS_GAME_BASE_STAT_INFO_MSG;
	m_stGameBaseStatInfo.iJudgeGameID = m_pServerBaseInfo->iGameID;
	m_stGameBaseStatInfo.iTmFlag = iTmFlag;
	m_pQueToRedis->EnQueue(&m_stGameBaseStatInfo, sizeof(RdGameBaseStatInfoDef));
	memset(&m_stGameBaseStatInfo, 0, sizeof(RdGameBaseStatInfoDef));
}

void GameLogic::AddComEventData(int iEventID, int iSubID, long long iAddNum)
{
	m_pGameEvent->AddComEventData(iEventID, iSubID, iAddNum);
}

int GameLogic::GetValidTableNum()
{
	int iTableNum = 0;
	FactoryTableItemDef* pTableItem = NULL;
	//找到空座位，都入座，若异常没找到，则都再重新配桌
	for (int i = 0; i < MAX_TABLE_NUM; i++)
	{
		pTableItem = GetTableItem(0, i);//都只用子类二维数组m_tbItem中第0个即可
		if (!pTableItem)
			continue;
		pTableItem->cPlayerNum = 0;
		for (int l = 0; l < m_iMaxPlayerNum; l++)
		{
			if (pTableItem->pFactoryPlayers[l])
			{
				pTableItem->cPlayerNum++;
				//_log(_DEBUG, "GL", "GetValidTableNum table[%d][%d],user[%d]-st[%d],playNum[%d--%d]",i,l, pTableItem->pFactoryPlayers[l]->iUserID, pTableItem->pFactoryPlayers[l]->iStatus,pTableItem->cPlayerNum, m_iMaxPlayerNum);
			}
		}
		if (pTableItem->cPlayerNum == 0)
		{
			iTableNum = i + 1;
			break;
		}
	}
	return iTableNum;
}

bool GameLogic::CallBackUsersSit(int iUserID, int iIfFreeRoom, int iTableNum, char cTableNumExtra, int iTablePlayerNum, int iRealNum)
{
	_log(_DEBUG, "GL", "CallBackUsersSit user[%d] iIfFreeRoom[%d],iTableNum[%d],cTableNumExtra[%d],iTablePlayerNum[%d]iRealNum[%d]", iUserID, iIfFreeRoom, iTableNum, cTableNumExtra, iTablePlayerNum, iRealNum);
	FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)(hashUserIDTable.Find((void*)&iUserID, sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "GL", "CallBackUsersSit ERR user[%d] nodePlayers == NULL", iUserID);
		return false;
	}
	bool bCanSit = false;
	if (nodePlayers->iStatus == PS_WAIT_DESK)
	{
		bCanSit = true;
	}
	if (!bCanSit)
	{
		_log(_ERROR, "GL", "CallBackUsersSit ERR user[%d] nodePlayers status[%d]", iUserID, nodePlayers->iStatus);
		return false;
	}
	FactoryTableItemDef* pTableItem = GetTableItem(0, iTableNum - 1);//都只用子类二维数组m_tbItem中第0个即可
	if (pTableItem == NULL)
	{
		_log(_ERROR, "GL", "CallBackUsersSit ERR user[%d] pTableItem ==NULL,iTableNum=%d", iUserID, iTableNum);
		return false;
	}
	if (iIfFreeRoom > 0)
	{
		for (int m = 0; m < MAX_ROOM_NUM; m++)
		{
			if (m_pServerBaseInfo->stRoom[m].iRoomType == iIfFreeRoom)
			{
				nodePlayers->cRoomNum = m + 1;
				break;
			}
		}
	}
	
	
	int iCurNum = 0;
	for (int i = 0; i < 10; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			iCurNum++;
		}
	}
	if (iCurNum == 0)
	{
		_log(_DEBUG, "GL", "CallBackUsersSit tab[%s][%d] used sit[%d]", pTableItem->cReplayGameNum, iTableNum, iCurNum);
		SendLeftTabNum(1);
	}
	SitDownResDef msgRes;
	memset((char*)&msgRes, 0x00, sizeof(SitDownResDef));

	msgRes.cResult = 1;
	msgRes.iTableNum = iTableNum;
	msgRes.cTableNumExtra = cTableNumExtra;

	//加入到桌结构体
	nodePlayers->cTableNum = iTableNum;
	nodePlayers->cTableNumExtra = cTableNumExtra;
	nodePlayers->iStatus = PS_WAIT_READY;//等待开始状态
	pTableItem->pFactoryPlayers[cTableNumExtra] = nodePlayers;//加入到桌结构体

	_log(_DEBUG, "GL", "CallBackUsersSit:OK id[%d][%s]Room[%d]Table[%d]Extra[%d]", iUserID, nodePlayers->szUserName, nodePlayers->cRoomNum, iTableNum, cTableNumExtra);

	pTableItem->iSeatNum = iTablePlayerNum;   //玩法座位数
	pTableItem->cPlayerNum = pTableItem->cPlayerNum + 1;//桌上玩家个数+1
	pTableItem->bIfReady[cTableNumExtra] = false;
	pTableItem->iRealNum = iTablePlayerNum;
	if (iRealNum > 0)
	{
		pTableItem->iRealNum = iRealNum;  //实际开局人数
	}
	if (iTablePlayerNum == 0)  //玩法异常，直接用实际人数
	{
		pTableItem->iSeatNum = iRealNum;
	}
	nodePlayers->iVTableID = 0;
	CallBackSitSuccedBeforePlayerInfoNotice(pTableItem, nodePlayers);
	//调用CLSendPlayerInfoNotice()发送通知
	int iPlayerNum = 0;
	for (int m = 0; m < 10; m++)
	{
		if (pTableItem->pFactoryPlayers[m])
		{
			iPlayerNum++;
			if (!pTableItem->pFactoryPlayers[m]->bIfAINode)//给非AI节点发送桌上所有玩家
			{
				GetTablePlayerInfo(pTableItem, pTableItem->pFactoryPlayers[m]);
				CLSendPlayerInfoNotice(pTableItem->pFactoryPlayers[m]->iSocketIndex, iTablePlayerNum);
			}	
		}
	}
	HandleReadyReq(nodePlayers->iUserID, NULL);//分配到桌上的玩家默认都直接准备好了，无需等客户端再开始
	if (iPlayerNum == pTableItem->iRealNum)
	{
		 if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->CallBackTableSitOk(iUserID);
		}
	}
	if (!nodePlayers->bIfAINode)
	{
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &msgRes, SITDOWN_RES_MSG, sizeof(SitDownResDef));//在上面要先发CLSendPlayerInfoNotice.add at 2/20 by crystal
	}	
	CallBackSitSuccedDownRes(pTableItem, nodePlayers);
	return true;
}

void GameLogic::HandleRedisGetUserDecorateInfo(void* pMsgData)
{
	SRdGetDecoratePropInfoDef* msgRes = (SRdGetDecoratePropInfoDef*)pMsgData;
	int iUserIDTemp = msgRes->iUserID;
	FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)hashUserIDTable.Find((void*)&(iUserIDTemp), sizeof(int));
	if (nodePlayers)
	{
		nodePlayers->iHeadFrameID = msgRes->iHeadFrameID;
		nodePlayers->iHeadFrameTm = msgRes->iHeadFrameTm;

		nodePlayers->iClockPropID = msgRes->iClockPropID;
		nodePlayers->iClockPropTm = msgRes->iClockPropTm;

		nodePlayers->iChatBubbleID = msgRes->iChatBubbleID;
		nodePlayers->iChatBubbleTm = msgRes->iChatBubbleTm;

		CheckUserDecorateTm(nodePlayers);
		_log(_DEBUG, "GL", "HandleRedisGetUserDecorateInfo: [%d] ,head[%d,%d],clock[%d,%d],bubble[%d,%d]", iUserIDTemp, msgRes->iHeadFrameID, msgRes->iHeadFrameTm, msgRes->iClockPropID, msgRes->iClockPropTm, msgRes->iChatBubbleID, msgRes->iChatBubbleTm);
	}
	else
	{
		_log(_DEBUG, "GL", "HandleRedisGetUserDecorateInfo: [%d] nodePlayer null", iUserIDTemp);
	}
}

void GameLogic::HandleGetAllRoomInfo(void* pMsgData)
{
	bool bFirst = false;
	int iBeginTime = m_pServerBaseInfo->iBeginTime;
	int iLongTime = m_pServerBaseInfo->iLongTime;
	int iMaxPlayerNum = m_pServerBaseInfo->iMaxPlayerNum;
	int iServerState = m_pServerBaseInfo->iServerState;

	GameRoomInfoResRadiusDef* pMsgInfo = (GameRoomInfoResRadiusDef*)pMsgData;
	m_pServerBaseInfo->iGameID = pMsgInfo->iGameID;
	memcpy(m_pServerBaseInfo->szGameName, pMsgInfo->szGameName, 32);
	memcpy(m_pServerBaseInfo->szServerName, pMsgInfo->szServerName, 100);
	m_pServerBaseInfo->iBeginTime = pMsgInfo->iBeginTime;
	m_pServerBaseInfo->iLongTime = pMsgInfo->iOpenTime;
	m_pServerBaseInfo->iServerState = pMsgInfo->iServerState;
	m_pServerBaseInfo->iServerRoomType = pMsgInfo->iServerRoomType;
	m_pServerBaseInfo->iMaxPlayerNum = pMsgInfo->iMaxPlayerNum;
	m_pServerBaseInfo->iServerIP = pMsgInfo->iServerIP;
	m_pServerBaseInfo->iInnerIP = pMsgInfo->iInnerIP;
	m_pServerBaseInfo->sInnerPort = pMsgInfo->sInnerPort;
	_log(_ERROR, "GL", "HandleGetAllRoomInfo roomtype[%d] status[%d] iGameID[%d] [%s] ip[%d] inner[%d][%d],iBaseServerNum=[%d]", 
		pMsgInfo->iServerRoomType, pMsgInfo->iServerState, m_pServerBaseInfo->iGameID, m_pServerBaseInfo->cReplayGameNum, pMsgInfo->iServerIP, pMsgInfo->iInnerIP, pMsgInfo->sInnerPort, pMsgInfo->iBaseServerNum);
	if (strlen(m_pServerBaseInfo->cReplayGameNum) == 0)
	{
		strcpy(m_pServerBaseInfo->cReplayGameNum, pMsgInfo->cGameNum);
		bFirst = true;
	}

	GameServerIPInfoDef* pServerIPInfo = (GameServerIPInfoDef*)((char*)pMsgData + sizeof(GameRoomInfoResRadiusDef));
	GameOneRoomDetailDef* pRoomInfo = (GameOneRoomDetailDef*)((char*)pMsgData + sizeof(GameRoomInfoResRadiusDef) + pMsgInfo->iBaseServerNum * sizeof(GameServerIPInfoDef));
	for (int i = 0; i < pMsgInfo->iBaseServerNum; i++)
	{
		//需要连接的底层服务器
		GameServerEpollResetIPMsgDef msgResetIP;
		memset(&msgResetIP, 0, sizeof(GameServerEpollResetIPMsgDef));
		msgResetIP.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgResetIP.msgHeadInfo.iMsgType = GAME_SERVER_EPOLL_RESET_IP_MSG;
		sockaddr_in InternetAddr;
		InternetAddr.sin_addr.s_addr = (pServerIPInfo->iServerIP);
		char* ip = inet_ntoa(InternetAddr.sin_addr);
		strcpy(msgResetIP.szIP, ip);
		msgResetIP.sServerPort = pServerIPInfo->sServerPort;

		InternetAddr.sin_addr.s_addr = (pServerIPInfo->iServerIPBak);
		ip = inet_ntoa(InternetAddr.sin_addr);
		strcpy(msgResetIP.szIPBak, ip);
		msgResetIP.sServerPortBak = pServerIPInfo->sServerPortBak;

		_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d],iServerType[%d] main[%s][%d] bak[%s][%d]", i, pServerIPInfo->iServerType, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);

		if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_ROOM)//room服务器，启动时已连接，重新传一次ip信息
		{
			if (m_pQueToRoom)
			{
				if (m_pServerBaseInfo->iLocalTestRoomPort > 0 && strlen(m_pServerBaseInfo->szLocalTestIP) > 0)
				{
					strcpy(msgResetIP.szIP, m_pServerBaseInfo->szLocalTestIP);
					msgResetIP.sServerPort = m_pServerBaseInfo->iLocalTestRoomPort;
					strcpy(msgResetIP.szIPBak, m_pServerBaseInfo->szLocalTestIP);
					msgResetIP.sServerPortBak = m_pServerBaseInfo->iLocalTestRoomPort;
				}
				m_pQueToRoom->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d] room epollstart main[%s][%d] bak[%s][%d]", i, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				}
			}
		}
		else if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_CENTER)//中心服务器
		{
			if (m_pServerBaseInfo->iLocalTestCenterPort > 0 && strlen(m_pServerBaseInfo->szLocalTestCenterIP) > 0)
			{
				strcpy(msgResetIP.szIP, m_pServerBaseInfo->szLocalTestCenterIP);
				msgResetIP.sServerPort = m_pServerBaseInfo->iLocalTestCenterPort;
				strcpy(msgResetIP.szIPBak, m_pServerBaseInfo->szLocalTestCenterIP);
				msgResetIP.sServerPortBak = m_pServerBaseInfo->iLocalTestCenterPort;
			}
			if (m_pQueToCenterServer == NULL)
			{
				m_pQueToCenterServer = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到中心服务器的消息队列
				m_pQueToCenterServer->SetQueName("pToCenterQueue");
				m_pCenterServerEpoll = new ServerEpollSockThread(100, BASE_SEVER_TYPE_CENTER, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
				m_pCenterServerEpoll->Ini(m_pQueLogic, m_pQueToCenterServer, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				m_pCenterServerEpoll->Start();
				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d] center epollstart main[%s][%d] bak[%s][%d]", i, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				}
				if (m_pNewAssignTableLogic == NULL)
				{
					m_pNewAssignTableLogic = new NewAssignTableLogic();
					m_pNewAssignTableLogic->Ini(this);
				}
				if (m_pTaskLogic == NULL)
				{
					m_pTaskLogic = new TaskLogic(this);
					m_pTaskLogic->SetNeedRateTable(CallBackNeedRateTable());
					m_pTaskLogic->SetNeedIntegralTask(CallBackNeedTaskInfo());
				}
			}
			else
			{
				m_pQueToCenterServer->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
			}
		}
		else if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_RADIUS)//radius服务器
		{
			if (m_pServerBaseInfo->iLocalTestRadiusPort > 0 && strlen(m_pServerBaseInfo->szLocalTestRadiusIP) > 0)
			{
				strcpy(msgResetIP.szIP, m_pServerBaseInfo->szLocalTestRadiusIP);
				msgResetIP.sServerPort = m_pServerBaseInfo->iLocalTestRadiusPort;
				strcpy(msgResetIP.szIPBak, m_pServerBaseInfo->szLocalTestRadiusIP);
				msgResetIP.sServerPortBak = m_pServerBaseInfo->iLocalTestRadiusPort;
			}
			if (m_pQueToRadius == NULL)
			{
				m_pQueToRadius = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到radius服务器的消息队列
				m_pQueToRadius->SetQueName("pToRadiusQueue");
				m_pRadiusEpoll = new ServerEpollSockThread(100, BASE_SEVER_TYPE_RADIUS, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
				m_pRadiusEpoll->Ini(m_pQueLogic, m_pQueToRadius, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				m_pRadiusEpoll->Start();
				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d] radius epollstart main[%s][%d] bak[%s][%d]", i, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				}
			}
			else
			{
				m_pQueToRadius->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
			}
		}
		else if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_REDIS)//redis服务器
		{
			if (m_pServerBaseInfo->iLocalTestRedisPort > 0 && strlen(m_pServerBaseInfo->szLocalTestRedisIP) > 0)
			{
				strcpy(msgResetIP.szIP, m_pServerBaseInfo->szLocalTestRedisIP);
				msgResetIP.sServerPort = m_pServerBaseInfo->iLocalTestRedisPort;
				strcpy(msgResetIP.szIPBak, m_pServerBaseInfo->szLocalTestRedisIP);
				msgResetIP.sServerPortBak = m_pServerBaseInfo->iLocalTestRedisPort;
			}
			if (m_pQueToRedis == NULL)
			{
				m_pQueToRedis = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到redis服务器的消息队列
				m_pQueToRedis->SetQueName("pToRedisQueue");
				m_pRedisEpoll = new ServerEpollSockThread(100, BASE_SEVER_TYPE_REDIS, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
				m_pRedisEpoll->Ini(m_pQueLogic, m_pQueToRedis, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				m_pRedisEpoll->Start();

				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d],Redis EpollStart,ip[%s],port[%d]", i, msgResetIP.szIP, msgResetIP.sServerPort);
				}

				//if (m_iThreadType == BASE_SEVER_TYPE_REDIS)
				{
					GameRedisGetUserInfoReqDef msgRedisReq;
					memset(&msgRedisReq, 0, sizeof(GameRedisGetUserInfoReqDef));
					msgRedisReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
					msgRedisReq.msgHeadInfo.iMsgType = GAME_REDIS_GET_USER_INFO_MSG;
					msgRedisReq.iUserID = 100001;
					m_pQueToRedis->EnQueue(&msgRedisReq, sizeof(GameRedisGetUserInfoReqDef));
				}
			}
			else
			{
				m_pQueToRedis->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
			}
		}
		else if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_BULL)//bull服务器
		{
			if (m_pQueToBull == NULL)
			{
				m_pQueToBull = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到bull服务器的消息队列
				m_pQueToBull->SetQueName("pToBullQueue");
				m_pBullEpoll = new ServerEpollSockThread(100, BASE_SEVER_TYPE_BULL, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
				m_pBullEpoll->Ini(m_pQueLogic, m_pQueToBull, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				m_pBullEpoll->Start();
				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d],Bull EpollStart,ip[%s],port[%d]", i, msgResetIP.szIP, msgResetIP.sServerPort);
				}
			}
			else
			{
				m_pQueToBull->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
			}
		}
		else if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_LOG)//log服务器
		{
			if (m_pServerBaseInfo->iLocalTesttLogPort > 0 && strlen(m_pServerBaseInfo->szLocalTestLogIP) > 0)
			{
				strcpy(msgResetIP.szIP, m_pServerBaseInfo->szLocalTestLogIP);
				msgResetIP.sServerPort = m_pServerBaseInfo->iLocalTesttLogPort;
				strcpy(msgResetIP.szIPBak, m_pServerBaseInfo->szLocalTestLogIP);
				msgResetIP.sServerPortBak = m_pServerBaseInfo->iLocalTesttLogPort;
			}
			if (m_pQueToLog == NULL)
			{
				m_pQueToLog = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到log服务器的消息队列
				m_pQueToLog->SetQueName("pToLogQueue");
				m_pLogEpoll = new ServerEpollSockThread(100, BASE_SEVER_TYPE_LOG, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
				m_pLogEpoll->Ini(m_pQueLogic, m_pQueToLog, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				m_pLogEpoll->Start();
				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d],Log EpollStart,ip[%s],port[%d]", i, msgResetIP.szIP, msgResetIP.sServerPort);
				}
			}
			else
			{
				m_pQueToLog->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
			}
		}
		else if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_ACT)//act服务器
		{
			if (m_pQueToAct == NULL)
			{
				m_pQueToAct = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到act服务器的消息队列
				m_pQueToAct->SetQueName("pToActQueue");
				m_pActEpoll = new ServerEpollSockThread(100, BASE_SEVER_TYPE_ACT, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
				m_pActEpoll->Ini(m_pQueLogic, m_pQueToAct, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				m_pActEpoll->Start();
				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d],Act EpollStart,ip[%s],port[%d]", i, msgResetIP.szIP, msgResetIP.sServerPort);
				}
			}
			else
			{
				m_pQueToAct->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
			}
		}
		else if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_OTHER1)//Other1服务器
		{
			if (m_pQueToOther1 == NULL)
			{
				m_pQueToOther1 = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到Other1服务器的消息队列
				m_pQueToOther1->SetQueName("pToOther1Queue");
				m_pOther1Epoll = new ServerEpollSockThread(100, BASE_SEVER_TYPE_OTHER1, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
				m_pOther1Epoll->Ini(m_pQueLogic, m_pQueToOther1, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				m_pOther1Epoll->Start();
				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d],Other1 EpollStart,ip[%s],port[%d]", i, msgResetIP.szIP, msgResetIP.sServerPort);
				}
			}
			else
			{
				m_pQueToOther1->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
			}
		}
		else if (pServerIPInfo->iServerType == BASE_SEVER_TYPE_OTHER2)//Other2服务器
		{
			if (m_pQueToOther2 == NULL)
			{
				m_pQueToOther2 = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到Other2服务器的消息队列
				m_pQueToOther2->SetQueName("pToOther2Queue");
				m_pOther2Epoll = new ServerEpollSockThread(100, BASE_SEVER_TYPE_OTHER2, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
				m_pOther2Epoll->Ini(m_pQueLogic, m_pQueToOther2, msgResetIP.szIP, msgResetIP.sServerPort, msgResetIP.szIPBak, msgResetIP.sServerPortBak);
				m_pOther2Epoll->Start();
				if (bFirst)
				{
					_log(_ERROR, "SLT", "HandleGetAllRoomInfo[%d],Other2 EpollStart,ip[%s],port[%d]", i, msgResetIP.szIP, msgResetIP.sServerPort);
				}
			}
			else
			{
				m_pQueToOther2->EnQueue(&msgResetIP, sizeof(GameServerEpollResetIPMsgDef));
			}
		}
		pServerIPInfo++;
	}

	for (int i = 0; i < pMsgInfo->iRoomNum; i++)
	{
		int iRoomIndex = 0;
		for (int j = 0; j < 10; j++)
		{
			_log(_DEBUG, "GL", "HandleGetAllRoomInfo i[%d] iRoomType[%d] name[%s]", j, m_pServerBaseInfo->stRoom[j].iRoomType, m_pServerBaseInfo->stRoom[j].szRoomName);
			if (m_pServerBaseInfo->stRoom[j].iRoomType == pRoomInfo->iRoomType || m_pServerBaseInfo->stRoom[j].iRoomType == 0)
			{
				iRoomIndex = j;
				break;
			}
		}

		_log(_DEBUG, "GL", "HandleGetAllRoomInfo iRoomIndex[%d] iRoomType[%d] name[%s] iRoomState[%d]", iRoomIndex, pRoomInfo->iRoomType, pRoomInfo->szRoomName, pRoomInfo->iRoomState);
		memcpy(&m_pServerBaseInfo->stRoom[iRoomIndex], pRoomInfo, sizeof(GameOneRoomDetailDef));
		pRoomInfo++;
	}

	CallBackGetRoomInfo();

	if (bFirst || iBeginTime != m_pServerBaseInfo->iBeginTime || iLongTime != m_pServerBaseInfo->iLongTime
		|| iMaxPlayerNum != m_pServerBaseInfo->iMaxPlayerNum || iServerState != m_pServerBaseInfo->iServerState)
	{
		//信息发生变化时，给中心服务器同步一次
		SendServerInfoToCenter();
	}
}

void GameLogic::HandleLogLevelSyncMsg(void* pMsgData, int iMsgLen)
{
	ServerLogLevelConfResDef* msgRes = (ServerLogLevelConfResDef*)pMsgData;

	_log(_ERROR, "GL", "HandleLogLevelSyncMsg iServerType[%d] iLogLevel[%d]", msgRes->iServerType, msgRes->iLogLevel);
	if (msgRes->iServerType == 0)
	{
		int iLogLevel = msgRes->iLogLevel;

		int iLocalTestLogLv = -1;
		GetValueInt(&iLocalTestLogLv, "log_level", "local_test.conf", "test_conf", "-1");
		if (iLocalTestLogLv != -1)
		{
			iLogLevel = iLocalTestLogLv;
		}

		if (iLogLevel == _ALL || iLogLevel == _DEBUG || iLogLevel == _NOTE || iLogLevel == _WARN || iLogLevel == _ERROR || iLogLevel == _SPECIAL)
		{
			set_log_level(iLogLevel);

			_log(_ERROR, "GL", "HandleLogLevelSyncMsg iLogLevel[%d],msg[%d],local[%d]", iLogLevel, msgRes->iLogLevel, iLocalTestLogLv);
		}
	}
	else
	{
		//转发给其他服务器
		MsgQueue* pMsgQueue = NULL;
		if (msgRes->iServerType == BASE_SEVER_TYPE_CENTER)
		{
			pMsgQueue = m_pQueToCenterServer;
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_RADIUS)
		{
			pMsgQueue = m_pQueToRadius;
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_REDIS)
		{
			pMsgQueue = m_pQueToRedis;
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_BULL)
		{
			pMsgQueue = m_pQueToBull;
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_ROOM)
		{
			//pMsgQueue = m_pQueToRoom;  //room不需要转发
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_LOG)
		{
			pMsgQueue = m_pQueToLog;
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_CHAT)
		{			
			//pMsgQueue = NULL;        //chat暂时无用
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_OTHER1)
		{
			pMsgQueue = m_pQueToOther1;
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_OTHER2)
		{
			pMsgQueue = m_pQueToOther2;
		}
		else if (msgRes->iServerType == BASE_SEVER_TYPE_ACT)
		{
			pMsgQueue = m_pQueToAct;
		}

		if (pMsgQueue != NULL)
		{
			pMsgQueue->EnQueue(pMsgData, iMsgLen);
		}
	}
}

void GameLogic::SendServerInfoToCenter()
{
	GameSysDetailToCenterMsgDef msgReq;
	memset(&msgReq, 0, sizeof(GameSysDetailToCenterMsgDef));
	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq.msgHeadInfo.iMsgType = GAME_SYS_DETAIL_TO_CENTER_MSG;
	msgReq.iServerID = m_pServerBaseInfo->iServerID;
	if (m_pServerBaseInfo->iLocalTestPort > 0 && strlen(m_pServerBaseInfo->szLocalTestIP) > 0)
	{
		msgReq.iIP = inet_addr(m_pServerBaseInfo->szLocalTestIP);
		msgReq.sPort = m_pServerBaseInfo->iLocalTestPort;
	}
	else
	{
		msgReq.iIP = m_pServerBaseInfo->iServerIP;
		msgReq.sPort = m_pServerBaseInfo->iServerPort;
	}
	msgReq.iBeginTime = m_pServerBaseInfo->iBeginTime;
	msgReq.iOpenTime = m_pServerBaseInfo->iLongTime;
	msgReq.iMaxNum = m_pServerBaseInfo->iMaxPlayerNum;
	msgReq.iServerState = m_pServerBaseInfo->iServerState;
	msgReq.iInnerIP = m_pServerBaseInfo->iInnerIP;
	msgReq.sInnerPort = m_pServerBaseInfo->sInnerPort;

	_log(_ERROR, "GL", "HandleGetAllRoomInfo irefresh center info id[%d] [%d][%d] state[%d]", msgReq.iServerID, msgReq.iIP, msgReq.sPort, msgReq.iServerState);

	if (m_pQueToCenterServer != NULL)
	{
		m_pQueToCenterServer->EnQueue(&msgReq, sizeof(GameSysDetailToCenterMsgDef));
	}
}

void GameLogic::HandleBullServerMsg(char* pMsgData, int iMsgLen)
{
	if (CallbackFirstHandleBullServerMsg(pMsgData, iMsgLen))
	{
		return;
	}
	MsgHeadDef* pMsgHead = (MsgHeadDef*)pMsgData;
	_log(_ALL, "GL", " HandleBullServerMsg [%x]", pMsgHead->iMsgType);
	/*if (pMsgHead->iMsgType == RD_BULL_ADD_FRIEND_REQ_MSG)
	{
		HandleUserAddFriendReq(pMsgData);
	}*/
	//底层处理结束后，再回调一次子类
	CallbackHandleBullServerMsg(pMsgData, iMsgLen);
}

void GameLogic::HandleCheckRoomIDMsg(void * pMsgData)
{
	//暂时屏蔽好友房相关
	/*RdGameCheckRoomNoResDef* pMsgRes = (RdGameCheckRoomNoResDef*)pMsgData;
	int iUserID = pMsgRes->iRoomUserID;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		return;
	}

	//房号检测通过，加入好友房数据缓存
	if (pMsgRes->iRt && m_pNewAssignTableLogic)
	{
		bool bCreateOK = m_pNewAssignTableLogic->CallBackCreateFriendVTable(nodePlayers);
		if (!bCreateOK)
		{
			pMsgRes->iRt = 0;
		}
	}

	if (pMsgRes->iRt == 0)   //房间号失效，返回认证失败
	{
		_log(_ERROR, "GL", "HandleCheckRoomIDMsg ERROR 26 !!!");
		AuthenResDef pMsgFailRes;
		memset(&pMsgFailRes, 0, sizeof(pMsgFailRes));
		pMsgFailRes.cResult = g_iRoomIdInvalid;
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &pMsgFailRes, AUTHEN_RES_MSG, sizeof(AuthenResDef));
		return;
	}
	//房间号检测成功、连接radius走认证
	AuthByRadiusMsg(nodePlayers);*/
}

void GameLogic::AuthByRadiusMsg(FactoryPlayerNodeDef *nodePlayers)
{
	_log(_DEBUG, "GL", "AuthByRadiusMsg user[%d] bGetLoginOK[%d] cLoginFlag2=[%d],cErrorType=[%d]", nodePlayers->iUserID, nodePlayers->bGetLoginOK, nodePlayers->cLoginFlag2, nodePlayers->cErrorType);
	if (nodePlayers->bGetLoginOK)
	{
		return;
	}
	if (nodePlayers->cLoginFlag2 & 1 == 1)  //客户端需要二次确认
	{
		nodePlayers->cLoginFlag2 &= ~(1);
		char cBuffer[1280] = { 0 };
		FaceGroundAuthenDef* pMsgRes = (FaceGroundAuthenDef*)cBuffer;
		memset(cBuffer, 0, sizeof(cBuffer));
		int iMsgLen = CallBackFaceGroundAuthen(nodePlayers, pMsgRes);   //每个游戏填充自己的二次确认消息，如果不作填充，直接走认证
		if (iMsgLen > 0)
		{
			CLSendSimpleNetMessage(nodePlayers->iSocketIndex, cBuffer, FACE_GROUND_AUTHEN_MSG, iMsgLen);
			OnUserInfoDisconnect(nodePlayers->iUserID);  //先把玩家节点删掉，下次进来重新走认证流程
			return;
		}
	}
	if (nodePlayers->cErrorType > 0)
	{
		AuthenResDef pMsgFailRes;
		memset(&pMsgFailRes, 0, sizeof(pMsgFailRes));
		pMsgFailRes.cResult = nodePlayers->cErrorType;
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &pMsgFailRes, AUTHEN_RES_MSG, sizeof(AuthenResDef));
		return;
	}

	CallBackBeforeAuthByRadius(nodePlayers);

	//向Radius发送用户信息请求
	GameUserAuthenInfoReqDef msgRD;
	memset(&msgRD, 0, sizeof(GameUserAuthenInfoReqDef));
	msgRD.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRD.msgHeadInfo.iMsgType = GAME_USER_AUTHEN_INFO_MSG;
	msgRD.iUserID = nodePlayers->iUserID;
	memcpy(msgRD.szUserToken, nodePlayers->szUserToken, 32);
	msgRD.iGameID = m_pServerBaseInfo->iGameID;
	msgRD.iServerID = m_pServerBaseInfo->iServerID;
	msgRD.iRoomType = nodePlayers->iEnterRoomType;
	int iIfNeedPlatformProp = IfNeedPlatformProp() ? 1 : 0;
	int iIfNeedGameProp = 0;
	int iIfNeedPointedProp = 0;
	int iIfNeedFriend = IfNeedFriends() ? 1 : 0;
	IfNeedGameProp(msgRD.iExtraInfo);
	for (int i = 0; i < 5; i++)
	{
		if (msgRD.iExtraInfo[i] > 0)
		{
			iIfNeedGameProp = 1;
			break;
		}
	}
	if (iIfNeedGameProp == 0)
	{
		IfNeedPointedProp(msgRD.iExtraInfo);
		for (int i = 0; i < 5; i++)
		{
			if (msgRD.iExtraInfo[i] > 0)
			{
				iIfNeedPointedProp = 1;
				break;
			}
		}
	}
	//第1位是否需要平台道具，第2位是否需要指定对应gameid道具（见extrainfo，最多5个），第3位是否需要获取指定道具，（见extrainfo，最多5个）,2和3位不能同时指定，第4位
	msgRD.iAuthenFlag = iIfNeedPlatformProp | (iIfNeedGameProp << 1) | (iIfNeedPointedProp << 2) | (iIfNeedFriend << 3);
	_log(_DEBUG, "GL", "AuthByRadiusMsg user[%d] iAuthenFlag[%d]", nodePlayers->iUserID, msgRD.iAuthenFlag);
	if (m_pQueToRadius != NULL)
	{
		m_pQueToRadius->EnQueue(&msgRD, sizeof(GameUserAuthenInfoReqDef));
	}
}

void GameLogic::SetAIBaseUserInfo(FactoryPlayerNodeDef *nodePlayers)
{
	vector<int>vcRoomNum;
	for (int m = 0; m < MAX_ROOM_NUM; m++)
	{
		if (m_pServerBaseInfo->stRoom[m].iRoomType > 0)
		{
			vcRoomNum.push_back(m+1);
		}
	}
	if (!vcRoomNum.empty())
	{
		nodePlayers->cRoomNum = vcRoomNum[rand() % vcRoomNum.size()];
	}	
	nodePlayers->iHeadImg = nodePlayers->iGender;
	int iMinMoney = m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iMinMoney;
	int iMaxMoney = m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iMaxMoney;
	int iLimit = iMaxMoney - iMinMoney <= 0 ? 1 : iMaxMoney - iMinMoney;
	if (iMaxMoney == 0 && iMinMoney > 0)
	{
		iLimit = iMinMoney * 2;
	}
	int iRandMoney = rand() % iLimit + iMinMoney;
	nodePlayers->iMoney = iRandMoney;
}

void GameLogic::CallBackSetRobotNick(char * szNick, int & iGender, int iUserID)
{
	//随机一个性别 奇数女偶数男
	iGender = iUserID % 2 + 1;	
	//读取配置，给ai节点昵称赋值
	if (m_mapVNick.size() == 0)
	{
		GetPointedTxtContent("virtual_nick.conf", m_mapVNick);
	}
	if (iUserID <= g_iMaxControlVID)//控制ai的昵称随机一个吧
	{
		if (!m_vcAllNick.empty())
		{
			int iRand = rand() % m_vcAllNick.size();
			sprintf(szNick, "%s", m_vcAllNick[iRand].c_str());
			m_vcAllNick.erase(m_vcAllNick.begin()+ iRand);
		}
		else
		{
			sprintf(szNick, "Guest%d", iUserID);
		}
		return;
	}
	map<int, string>::iterator iter = m_mapVNick.find(iUserID);
	if (iter != m_mapVNick.end())
	{
		strcpy(szNick, iter->second.c_str());
	}
	else
	{
		sprintf(szNick, "Guest%d", iUserID);
	}
}

void GameLogic::GetPointedTxtContent(char* szFileName, map<int, string>& mapTxtContent)
{
	map<int, string>().swap(mapTxtContent);
	mapTxtContent.clear();
	ifstream _file(szFileName);
	if (!_file)
	{
		_log(_ERROR, "GetPointedTxtContent", "not exist file %s", szFileName);
		return;
	}

	int iIndex = 0;
	char szContent[64] = { 0 };
	while (!_file.eof())
	{
		memset(szContent, 0, sizeof(szContent));

		_file.getline(szContent, sizeof(szContent));

		int iUserID = atoi(szContent);
		for (int i = strlen(szContent) - 1; i > 0; i--)
		{
			if (szContent[i] == ' ' || szContent[i] == 0x0d)
			{
				szContent[i] = 0;
			}
		}

		if (iUserID > 0)
		{
			char * szShowTm = strchr(szContent, '=');
			mapTxtContent[iUserID] = szShowTm + 1;
			m_vcAllNick.push_back(szShowTm + 1);
		}
	}
	_file.close();
}

FactoryPlayerNodeDef* GameLogic::GetControlAINode(FactoryPlayerNodeDef *nodePlayer)
{
	FactoryPlayerNodeDef* pNode = NULL;
	if (m_vcControlAIID.empty())
	{
		_log(_ERROR, "GL", "GetControlAINode no id");
		return pNode;
	}
	int iCnt = 0;
	while (true)
	{
		if (iCnt >= m_vcControlAIID.size())
		{
			break;
		}
		int iUserID = m_vcControlAIID[iCnt];
		FactoryPlayerNodeDef *nodeTemp = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
		if (nodeTemp != NULL)
		{
			_log(_ERROR, "GL", "GetControlAINode nodePlayers[%d] has existed,status[%d],table[%d,%d]", iUserID, nodeTemp->iStatus, nodeTemp->cTableNum, nodeTemp->cTableNumExtra);
		}
		else
		{
			pNode = (FactoryPlayerNodeDef *)GetFreeNode();
			if (pNode != NULL)
			{
				pNode->iUserID = iUserID;
				pNode->iSocketIndex = -1;
				pNode->iPlayType = nodePlayer->iPlayType;
				pNode->iAllNum = 50+rand()%300;
				int iRandRate = 30+rand() % 41;//胜率30-70
				pNode->iWinNum = pNode->iAllNum*iRandRate/100;
				CallBackSetRobotNick(pNode->szNickName, pNode->iGender, pNode->iUserID);
				iRandRate = rand() % 100;//30的概率有头像框
				pNode->iAchieveLevel = GetVirtualAchivel(pNode->iAllNum);
				if (iRandRate < 30 && pNode->iAchieveLevel / 100 > 1)
				{
					int iRandAchv = rand() % ((pNode->iAchieveLevel - 100) / 100);
					if (pNode->iAchieveLevel == 602)
					{
						pNode->iHeadFrameID = 231 + iRandAchv;
					}
					else if (iRandAchv > 0)
					{
						pNode->iHeadFrameID = 231 + iRandAchv - 1;
					}
					else
					{
						pNode->iHeadFrameID = 231;
					}
				}
				pNode->iExp = 0;
				pNode->bIfAINode = true;
				hashUserIDTable.Add((void *)(&(iUserID)), sizeof(int), pNode);
				SetAIBaseUserInfo(pNode);
				break;
			}
			else
			{
				_log(_ERROR, "GL", "GetControlAINode GetFreeNode err");
			}
		}
		iCnt++;
		if (iCnt > 5)
		{
			break;
		}
	}
	if (pNode)
	{
		m_vcControlAIID.erase(m_vcControlAIID.begin());
	}
	return pNode;
}

int GameLogic::GetVirtualAchivel(int iAllCnt)
{
	int iCnt[18] = { 0, 80, 200, 500, 800, 1000, 1500, 2000, 2700, 3500, 4000, 5000, 6000, 7500, 9000, 12000, 15000, 20000 };
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

void GameLogic::ReleaseControlAINode(FactoryPlayerNodeDef *nodePlayer)
{
	_log(_DEBUG, "GL", "ReleaseControlAINode [%d]", nodePlayer!= NULL? nodePlayer->iUserID:0);
	if (m_pNewAssignTableLogic != NULL)
	{
		m_pNewAssignTableLogic->ReleaseRdCtrlAI(nodePlayer->iUserID);
	}	
	char szNickPre[16];
	memset(szNickPre,0, sizeof(szNickPre));
	memcpy(szNickPre, nodePlayer->szNickName,5);
	if (strcmp("Guest", szNickPre) != 0)
	{
		m_vcAllNick.push_back(nodePlayer->szNickName);
	}
	m_vcControlAIID.push_back(nodePlayer->iUserID);
	hashUserIDTable.Remove((void *)&(nodePlayer->iUserID), sizeof(int));
	UpdateRoomNum(-1, nodePlayer->cRoomNum - 1, -1);
	ReleaseNode(nodePlayer);	

}

void GameLogic::HandleAchieveTaskComp(void * pMsgData)
{
	RdUpdateUserAchieveInfoResDef* pMsgRes = (RdUpdateUserAchieveInfoResDef*)pMsgData;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&pMsgRes->iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "DGL", "●●●● HandleAchieveTaskComp:nodePlayers is NULL ●●●●");
		return;
	}
	//有成功领取成就点，通知客户端，并通知log服务器
	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, pMsgRes, RD_UPDATE_USER_ACHIEVE_INFO_MSG, sizeof(RdUpdateUserAchieveInfoResDef));

	memset(m_cGameLogBuff, 0, sizeof(m_cGameLogBuff));
	GameAchieveTaskLogDef* pMsgLog = (GameAchieveTaskLogDef*)m_cGameLogBuff;
	m_iGameLogBuffLen = sizeof(GameAchieveTaskLogDef);
	pMsgLog->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgLog->msgHeadInfo.iMsgType = GAME_ACHIEVE_TASK_LOG_MSG;
	pMsgLog->iUserID = pMsgRes->iUserID;
	pMsgLog->iTaskType = pMsgRes->iTaskType;
	pMsgLog->iCompleteNum = pMsgRes->iCompleteNum;
	pMsgLog->iAchievePoint = pMsgRes->iAchievePoint;
	if (m_pQueToLog)
	{
		m_pQueToLog->EnQueue(m_cGameLogBuff, m_iGameLogBuffLen);
	}
}


void GameLogic::SendGrowupEventInfoReq(int iUserID, int iGrowEventId)
{
	GameGetUserGrowupEventReqDef msgReq;
	memset(&msgReq, 0, sizeof(msgReq));
	msgReq.iUserID = iUserID;
	msgReq.iGrowEventId = iGrowEventId;
	msgReq.msgHeadInfo.iMsgType = GAME_GET_USER_GROWUP_EVENT_MSG;
	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	if (m_pQueToRadius)
	{
		m_pQueToRadius->EnQueue(&msgReq, sizeof(msgReq));
	}
}

void GameLogic::HandleGrowupEventInfo(void *pMsgData)
{
	GameGetUserGrowupEventResDef *pMsgRes = (GameGetUserGrowupEventResDef*)pMsgData;
	char cValue[128] = { 0 };
	if (pMsgRes->iGrowStrInfoLen > 0)
	{
		memcpy(cValue, pMsgRes + 1, pMsgRes->iGrowStrInfoLen);
	}
	CallBackGrowupEventRes(pMsgRes->iUserID, pMsgRes->iGrowEventId, pMsgRes->iGrowNumInfo, cValue);
}

void GameLogic::UpdateGrowupEventInfo(int iUserID, int iEventID, int iGrowNumInfo, char* szGrowStrInfo, int iLen)
{
	char cBuffer[256] = { 0 };
	GameUpdateUserGrowupEventReqDef *msgReq = (GameUpdateUserGrowupEventReqDef*)cBuffer;
	msgReq->iUserID = iUserID;
	msgReq->iGrowEventId = iEventID;
	msgReq->msgHeadInfo.iMsgType = GAME_UPDATE_USER_GROWUP_EVENT_MSG;
	msgReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq->iGrowNumInfo = iGrowNumInfo;
	msgReq->iGrowStrInfoLen = iLen;
	memcpy((char*)msgReq+1, szGrowStrInfo, iLen);
	if (m_pQueToRadius)
	{
		m_pQueToRadius->EnQueue(&msgReq, sizeof(msgReq) + iLen);
	}
}

void GameLogic::RefreshUserGrowUpEvent(FactoryPlayerNodeDef *pPlayer, int iGrowEvent, int iGrowNum, char *szGrowStr, int iLen)
{
	char cBuffer[1024] = { 0 };
	GameUpdateUserGrowupEventReqDef *pMsgReq = (GameUpdateUserGrowupEventReqDef*)cBuffer;
	pMsgReq->msgHeadInfo.iMsgType = GAME_UPDATE_USER_GROWUP_EVENT_MSG;
	pMsgReq->msgHeadInfo.cVersion = MESSAGE_VERSION;

	pMsgReq->iUserID = pPlayer->iUserID;
	pMsgReq->iGrowNumInfo = iGrowNum;
	pMsgReq->iGrowEventId = iGrowEvent;
	pMsgReq->iGrowStrInfoLen = -1;
	if (iLen >= 0)
	{
		memcpy(pMsgReq + 1, szGrowStr, iLen);
		pMsgReq->iGrowStrInfoLen = iLen;
	}
	else
	{
		iLen = 0;
	}
	_log(_DEBUG, "GL", "RefreshUserGrowUpEvent user[%d] event[%d] num[%d] str[%s]len[%d]", pPlayer->iUserID, iGrowEvent, iGrowNum, szGrowStr, pMsgReq->iGrowStrInfoLen);
	if (m_pQueToRadius)
	{
		m_pQueToRadius->EnQueue(cBuffer, sizeof(GameUpdateUserGrowupEventReqDef) + iLen);
	}
}

void GameLogic::HandleFriendListRes(void *pMsgData)
{
	//好友相关暂时屏蔽
	/*	GameGetFriendListResDef* pMsgRes = (GameGetFriendListResDef*)pMsgData;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&pMsgRes->iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "DGL", "HandleFriendListRes:nodePlayers is NULL");
		return;
	}
	int iUserID = pMsgRes->iUserID;
	//memcpy(nodePlayers->iFriends, pMsgRes+1, pMsgRes->iFriendsNum * sizeof(int));
	int* pFriendID = (int*)(pMsgRes + 1);
	for (int i = 0; i < pMsgRes->iFriendsNum; i++)
	{
		if (i >= MAX_FRIENDS_NUM)
		{
			break;
		}
		nodePlayers->iFriends[i] = *pFriendID;
		pFriendID++;
	}*/
}

void GameLogic::GetUserRoomTask(FactoryPlayerNodeDef *nodePlayers, int iAddNum)
{
	int iRoomType = m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iRoomType;
	RdGetRoomTaskInfoReqDef msgReq;
	memset(&msgReq, 0, sizeof(msgReq));
	msgReq.iGameID = m_pServerBaseInfo->iGameID;;
	msgReq.iReqType = iAddNum;
	msgReq.iRoomType = iRoomType;
	msgReq.iUserID = nodePlayers->iUserID;
	msgReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgReq.msgHeadInfo.iMsgType = RD_GAME_ROOM_TASK_INFO_MSG;
	if (m_pQueToRedis)
	{
		m_pQueToRedis->EnQueue(&msgReq, sizeof(RdGetRoomTaskInfoReqDef));
	}
}

void GameLogic::HandleUserRoomTaskRes(void *pMsgData)
{
	RdGameRoomTaskInfoResDef* pMsgRes = (RdGameRoomTaskInfoResDef*)pMsgData;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&pMsgRes->iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "DGL", "HandleUserRoomTaskRes:nodePlayers is NULL");
		return;
	}

	int iRoomType = m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iRoomType;

	//玩家房间任务回应
	if (pMsgRes->iRoomType == iRoomType)
	{
		nodePlayers->roomTask.iRoomType = pMsgRes->iRoomType;
		nodePlayers->roomTask.iAwardID = pMsgRes->iAwardID;
		nodePlayers->roomTask.iAwardNum = pMsgRes->iAwardNum;
		nodePlayers->roomTask.iCompNum = pMsgRes->iCompNum;
		nodePlayers->roomTask.iNeedNum = pMsgRes->iNeedNum;
		CallBackUserRoomTaskInfo(nodePlayers);
	}
}

void GameLogic::HandleUserRecentIntegralTask(void *pMsgData)
{
	RdGetIntegralTaskMsgDef* pMsgRes = (RdGetIntegralTaskMsgDef*)pMsgData;
	int iUserID = pMsgRes->iUserID;
	if (pMsgRes->iGameID != m_pServerBaseInfo->iGameID)
	{
		return;
	}

	//更新玩家历史奖券任务记录
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&pMsgRes->iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "DGL", "HandleUserRecentIntegralTask:nodePlayers is NULL");
		return;
	}

	vector<int> vcRecent;
	int *pCur = (int*)(pMsgRes + 1);
	for (int i = 0; i < pMsgRes->iRecentNum; i++)
	{
		vcRecent.push_back(*(pCur+i));
	}

	for (int i = 0; i < vcRecent.size(); i++)
	{
		if (i >= 3)
		{
			break;
		}
		nodePlayers->iRecentTask[i] = vcRecent[i];
	}
}

void GameLogic::HandleUserDayInfo(void * pMsgData)
{
	RdUserDayInfoResMsgDef* pMsgRes = (RdUserDayInfoResMsgDef*)pMsgData;
	FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)(&pMsgRes->iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "DGL", "HandleUserDayInfo:nodePlayers is NULL");
		return;
	}

	nodePlayers->iDayTaskIntegral = pMsgRes->iDayTaskIntegral;
	//_log(_DEBUG, "GL", "HandleUserDayInfo user[%d] daytaskintegral[%d]", nodePlayers->iUserID, nodePlayers->iDayTaskIntegral);
}