//系统头文件
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>      //sleep()
#include <stdlib.h>      //rand()

//公共模块头文件
#include "log.h"
#include "conf.h"

//项目相关头文件
#include "hzxlmj_gamelogic.h"
#include "judgecard.h"
#include "msg.h"
#include "main.h"
#include <vector>

#include "RandKiss.h"
#include "GameConf.h"
#include <algorithm>
#include <math.h>
#include "hzxlmj_AIControl.h"
#include "GlobalMethod.h"
#include "ClientEpollSockThread.h"

using namespace std;

extern ServerBaseInfoDef stSysConfBaseInfo;

#define KICK_OUT_TIME 22		// 玩家超时时间
#define HZXL_MAX_CARD_NUM  112	// 红中血流麻将总共用到的牌数

//填充消息头宏定义
#define __FILL_MSG_HEAD(_MsgTypeDef, _MsgType, _pMsg)	\
	memset(_pMsg, 0, sizeof(MsgHeadDef));				\
	_pMsg->msgHeadInfo.cVersion = MESSAGE_VERSION;		\
	_pMsg->msgHeadInfo.cMsgType = _MsgType;				\
	_pMsg->msgHeadInfo.iMsgBodyLen = 	htonl(sizeof(_MsgTypeDef) - sizeof(MsgHeadDef))

HZXLGameLogic* g_pHZXLGameLogic = NULL;

HZXLGameLogic::HZXLGameLogic()
{
	g_pHZXLGameLogic = this;

	m_iMaxPlayerNum = PLAYER_NUM;
	m_pNodeFree = NULL;
	m_pNodePlayer = NULL;
	//m_pTmpNodeFree = NULL;
	//m_pTmpNodePlayer = NULL;

	vector<MJGPlayerNodeDef*>().swap(m_vTmpPlayerNode);

	m_bIfStartKickOutTime = true;        //超时踢人开启
	CRandKiss::g_comRandKiss.SrandKiss(time(NULL));
	srandom(time(NULL));

	GameConf::GetInstance()->ReadGameConf("hzxlmj_game.conf");

	m_pGameAlgorithm = new hzxlmj_GameAlgorithm();
	m_pHzxlAICTL = new HZXLMJ_AIControl();

	m_pHzxlAICTL->ReadRoomConf();

	//初始化“桌”信息	
	ResetTableInfo();

	m_bOpenRecharge = false;
}

HZXLGameLogic::~HZXLGameLogic()
{
	m_pGameAlgorithm = NULL;
	m_pHzxlAICTL = NULL;
}

HZXLGameLogic * HZXLGameLogic::shareHZXLGameLogic()
{
	return g_pHZXLGameLogic;
}

void HZXLGameLogic::AllUsersSendNoitce(void *pDate, int iType)//向游戏中所有用户发送公告通知
{
	MJGPlayerNodeDef *pNode;
	pNode = m_pNodePlayer;
	int iJudgeCnt = 0;
	char cBuffer[1024];
	BullNoticeDef msg;
	memset(&msg, 0, sizeof(BullNoticeDef));
	msg.cType = iType;
	msg.iBullLen = strlen((char*)pDate);
	memset(cBuffer, 0, sizeof(cBuffer));
	int iBufferLen = sizeof(BullNoticeDef);
	memcpy(cBuffer, &msg, iBufferLen);
	memcpy(cBuffer + iBufferLen, (char*)pDate, msg.iBullLen);
	iBufferLen += msg.iBullLen;
	while (1)
	{
		if (pNode)
		{
			//发送公告
			CLSendSimpleNetMessage(pNode->iSocketIndex, cBuffer, GAME_BULL_NOTICE_MSG, iBufferLen);
			pNode = pNode->pNext;
			iJudgeCnt++;
			if (iJudgeCnt >= KICK_MAX_CNT)
			{
				_log(_ERROR, "HZXLGameLogic", "AllUsersSendNoitce_:iJudgeCnt[%d]", iJudgeCnt);
				break;
			}
		}
		else
		{
			break;
		}
	}
}


// 玩家掉线的处理
void HZXLGameLogic::HandlePlayStateDisconnect(int iStatusCode, int iUserID, void *pMsgData)
{

	if (iStatusCode > PS_WAIT_READY)
	{
		if (iStatusCode == PS_WAIT_RESERVE_TWO)
		{
			FactoryPlayerNodeDef *nodePlayers = (FactoryPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
			if (!nodePlayers)
			{
				return;
			}

			HandleNormalEscapeReq(nodePlayers, pMsgData);
			((MJGPlayerNodeDef *)nodePlayers)->Reset();
			return;
		}

		OnWaitLoginAgin(iUserID, pMsgData);
	}
}

//直接复制
void HZXLGameLogic::JudgeAgainLoginTime(int iTime)
{
	MJGPlayerNodeDef *nodeTemp = m_pNodePlayer;
	int iJudgeCnt = 0;
	while (nodeTemp)
	{
		MJGPlayerNodeDef *tempNode2;
		tempNode2 = nodeTemp->pNext;
		bool bFlag = JudageRoomCloseKickPlayer(nodeTemp);
		if (bFlag)
		{
			nodeTemp = tempNode2;
			continue;
		}
		//遍例查看掉线等待重入玩家是否等待时间结束,add at 3/31
		bool bNeedRelease = false;
		MJGPlayerNodeDef *tempNode;
		if (nodeTemp->bIfWaitLoginTime == true)
		{
			nodeTemp->iWaitLoginTime -= iTime;
			if (nodeTemp->iWaitLoginTime <= 0)//时间到了
			{
				//需要调用Disconnect来释放结点,释放前必须先保存当前判断的结点，不然循环会马上跳出
				tempNode = nodeTemp->pNext;
				if (nodeTemp->iStatus == PS_WAIT_READY)
				{
					OnReadyDisconnect(nodeTemp->iUserID, NULL);
				}
				else if (nodeTemp->iStatus == PS_WAIT_DESK)
				{
					OnFindDeskDisconnect(nodeTemp->iUserID, NULL);
				}
				/*else
				{
				OnPlayStateDisconnect(nodeTemp->iUserID,NULL);
				}*/

				bNeedRelease = true;
			}
		}
		if (bNeedRelease == false)
			nodeTemp = nodeTemp->pNext;
		else
			nodeTemp = tempNode;

		iJudgeCnt++;
		if (iJudgeCnt >= KICK_MAX_CNT)
		{
			_log(_ERROR, "HZXLGameLogic", "JudgeAgainLoginTime_:iJudgeCnt[%d]", iJudgeCnt);
			break;
		}
	}
}

//可以直接复制
void HZXLGameLogic::FreshRoomOnline()
{
	memset(m_pServerBaseInfo->stRoomOnline, 0, sizeof(m_pServerBaseInfo->stRoomOnline));
	//遍历所有玩家
	MJGPlayerNodeDef *node = m_pNodePlayer;
	int iJudgeCnt = 0;
	while (node)
	{
		if (node->cRoomNum > 0 && node->cRoomNum < MAX_ROOM_NUM + 1)
		{
			char cLoginType;
			if (node->bIfAINode || node->iPlayerType != NORMAL_PLAYER)
			{
				cLoginType = -1;
			}
			else
			{
				cLoginType = node->cLoginType;
			}
			if (!node->bIfAINode || (node->bIfAINode && node->iStatus > PS_WAIT_READY))
			{
				UpdateRoomNum(1, node->cRoomNum - 1, cLoginType);
			}
		}
		node = node->pNext;
		iJudgeCnt++;
		if (iJudgeCnt >= KICK_MAX_CNT)
		{
			_log(_ERROR, "HZXLGameLogic", "FreshRoomOnline_:iJudgeCnt[%d]", iJudgeCnt);
			break;
		}
	}
}

//取得具体桌信息指针.可以直接复制
FactoryTableItemDef* HZXLGameLogic::GetTableItem(int iRoomID, int iTableIndex)
{
	if (iRoomID >= MAX_ROOM_NUM || iTableIndex >= MAX_TABLE_NUM || iTableIndex < 0)
	{
		return NULL;
	}
	else
	{
		return &(m_tbItem[iRoomID][iTableIndex]);
	}
}

//移除玩家节点,直接复制
void HZXLGameLogic::RemoveTablePlayer(FactoryTableItemDef *pTableItem, int iTableNumExtra)
{
	MJGTableItemDef *pTable = (MJGTableItemDef*)pTableItem;
	if (iTableNumExtra != -1)
	{
		pTable->pFactoryPlayers[iTableNumExtra] = NULL;//座位置为空
		pTable->pPlayers[iTableNumExtra] = NULL;

	}
}

//取得自由节点.游戏直接复制
void* HZXLGameLogic::GetFreeNode()
{
	_log(_DEBUG, "HZXLGameLogic", "GetFreeNode_ Begin");
	MJGPlayerNodeDef *nodePlayer;
	if (m_pNodeFree)
	{
		nodePlayer = m_pNodeFree;
		m_pNodeFree = m_pNodeFree->pNext;
		if (m_pNodeFree)
		{
			m_pNodeFree->pPrev = NULL;
		}
	}
	else
	{
		nodePlayer = new MJGPlayerNodeDef;
	}
	_log(_DEBUG, "HZXLGameLogic", "GetFreeNode_ Mid");

	if (nodePlayer)
	{
		//插入到“使用中的节点”
		nodePlayer->Reset();
		if (m_pNodePlayer) m_pNodePlayer->pPrev = nodePlayer;
		nodePlayer->pNext = m_pNodePlayer;
		nodePlayer->pPrev = 0;
		m_pNodePlayer = nodePlayer;
	}
	else
	{
		_log(_ERROR, "HZXLGameLogic", "GetFreeNode: Cannot get free nodePlayer.");
		return NULL;
	}

	_log(_DEBUG, "HZXLGameLogic", "GetFreeNode_ End");
	return (void*)nodePlayer;
}

//释放节点,游戏直接复制
void HZXLGameLogic::ReleaseNode(void* pNode)
{
	MJGPlayerNodeDef *pCurNode = (MJGPlayerNodeDef*)pNode;
	_log(_DEBUG, "HZXLGameLogic", "ReleaseNode_ Begin UID[%d]", pCurNode->iUserID);

	if (pCurNode->pNext)  //若当前节点后有节点，让后节点的前指针指向前节点
	{
		pCurNode->pNext->pPrev = pCurNode->pPrev;
	}
	if (pCurNode->pPrev)  //若当前节点前有节点，让前节点的后指针指向后节点
	{
		pCurNode->pPrev->pNext = pCurNode->pNext;
	}
	if (pCurNode == m_pNodePlayer) //如果是首节点，首节点后移
	{
		m_pNodePlayer = m_pNodePlayer->pNext;
		if (m_pNodePlayer)
		{
			m_pNodePlayer->pPrev = 0;
		}
	}

	_log(_DEBUG, "HZXLGameLogic", "ReleaseNode_ Mid");

	//将释放的节点加入到“空闲节点”中去
	if (m_pNodeFree) //已经存在“空闲节点”
	{
		m_pNodeFree->pPrev = pCurNode;
	}
	pCurNode->pNext = m_pNodeFree;
	pCurNode->pPrev = 0;
	m_pNodeFree = pCurNode;

	_log(_DEBUG, "HZXLGameLogic", "ReleaseNode_ End m_pNodeFree->UID[%d]", m_pNodeFree->iUserID);
}

//************************下面所有代码都是定式的，所有游戏都一模一样********************************end*/

//处理客户端发来的消息
void HZXLGameLogic::HandleOtherGameNetMessage(int iMsgType, int iStatusCode, int iUserID, void *pMsgData)
{

	switch (iMsgType)
	{

	case MJG_SEND_CARD_REQ:
		HandleSendCard(iUserID, pMsgData);
		break;
	case MJG_SPECIAL_CARD_REQ:
		HandleSpecialCard(iUserID, pMsgData);
		break;
	case XLMJ_CONFIRM_QUE_REQ:
		HandleConfirmQueReq(iUserID, pMsgData);
		break;
	case XLMJ_CHANGE_CARD_REQ:
		HandleChangeCardReq(iUserID, pMsgData);
		break;
	case GET_HZXL_CONTROL_CARD_MSG:
		HandleGetControlCardMsg(pMsgData);
		break;
	case HZXLMJ_RECHARGE_REQ:
		HandleRechargeReqMsg(iUserID, pMsgData);
		break;
	case HZXLMJ_RECHARGE_RESULT_MSG:
		HandleRechargeResultMsg(iUserID, pMsgData);
		break;
	case HZXLMJ_RECHARGE_RECORD_MSG:
		HandleRechargeRecordMsg(iUserID, pMsgData);
		break;
	default:
		break;
	}
}

//redis消息回调
void HZXLGameLogic::CallbackHandleRedisServerMsg(char* pMsgData, int iMsgLen)
{
	
}

//清空桌信息		自动出牌
void HZXLGameLogic::AutoSendCards(int iUserID)
{
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "MJG_GameLogic", "●●●● AutoSendCards: nodePlayers is NULL ●●●●");
		return;
	}

	int iTableNum = nodePlayers->cTableNum;
	char iTableNumExtra = nodePlayers->cTableNumExtra;
	char iRoomNum = nodePlayers->cRoomNum;

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "AutoSendCards:[%d][%d][%d]", iRoomNum, iTableNum, iTableNumExtra);
		return;
	}

	// 玩家在等待充值的状态下离线，可能是直接关闭客户端，或者跳转充值，导致离线，等待定时器踢出玩家
	if (nodePlayers->bWaitRecharge && nodePlayers->iAllowChargeTime > 0)
	{
		_log(_DEBUG, "HZXLGameLogic", "AutoSendCards: Player Wait Recharge UID[%d],iACTime", nodePlayers->iUserID, nodePlayers->iAllowChargeTime);
		return;
	}

	MJGTableItemDef* pTableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);

	for (int i = 0; i<4; i++)
	{
		if (pTableItem->pPlayers[i] == NULL)
		{
			_log(_ERROR, "HZXLGameLogic", "AutoSendCards:[%d][%d][%d] tableItem.pPlayers[%d]=NULL", iRoomNum, iTableNum, iTableNumExtra, i);
		}
	}
	_log(_DEBUG, "HZXLGameLogic", "AutoSendCards R[%d]T[%d]UID[%d] iStatus[%d]", iRoomNum, iTableNum, nodePlayers->iUserID, nodePlayers->iStatus);

	//如果玩家是在打牌阶段 就自动帮玩家打牌
	if (nodePlayers->iStatus == PS_WAIT_SEND)
	{
		if (pTableItem->iCurMoPlayer == nodePlayers->cTableNumExtra)
		{
			if ((nodePlayers->iSpecialFlag >> 6) == 1)
			{
				if (nodePlayers->bIsHu)
				{
					_log(_DEBUG, "HZXLGameLogic", "AutoSendCards PS_WAIT_SEND  bAutoHu R[%d]T[%d]UID[%d]", iRoomNum, iTableNum, nodePlayers->iUserID);
					SpecialCardsReqDef pMsg;
					memset(&pMsg, 0, sizeof(SpecialCardsReqDef));
					pMsg.iSpecialFlag = (1 << 6);
					pMsg.cFirstCard = pTableItem->cTableCard;
					HandleSpecialCard(nodePlayers->iUserID, (void *)&pMsg);
					return;
				}
			}
			SendCardsReqDef pSendMsg;
			memset(&pSendMsg, 0, sizeof(SendCardsReqDef));
			if (nodePlayers->bIsHu == true)
			{
				pSendMsg.cCard = nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1];
			}
			else
			{
				if (nodePlayers->vcSendTingCard.size() > 0)
				{
					pSendMsg.cIfTing = 1;
					pSendMsg.cCard = nodePlayers->vcSendTingCard[rand() % nodePlayers->vcSendTingCard.size()];
				}
				else
				{
					for (int i = 0;i < nodePlayers->iHandCardsNum; i++)
					{
						if (nodePlayers->cHandCards[i] >> 4 == nodePlayers->iQueType)
						{
							pSendMsg.cCard = nodePlayers->cHandCards[i];
							break;
						}
					}
					if (pSendMsg.cCard == 0)
					{
						pSendMsg.cCard = nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1];
					}
				}
			}

			HandleSendCard(nodePlayers->iUserID, (void *)&pSendMsg);
		}
	}
	else if(nodePlayers->iStatus == PS_WAIT_CHANGE_CARD)
	{
		//如果玩家已经请求过了 就帮他清掉
		if(nodePlayers->bChangeCardReq)
		{
			return;
		}

		//掉线玩家请求换牌3张
		ChangeCardsReqDef msgChangeCardReq;
		memset(&msgChangeCardReq,0,sizeof(ChangeCardsReqDef));


		int iTempArr[5][10] = { 0 };
		int iMinType = 0;
		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
		{
			int iType = nodePlayers->cHandCards[i] >> 4;
			int iValue = nodePlayers->cHandCards[i] & 0x000f;

			iTempArr[iType][0]++;
			iTempArr[iType][iValue]++;
		}
		_log(_DEBUG, "XLMJGameLogic", "AutoSendCards, iTempArr[0][0] = [%d], iTempArr[1][0] = [%d], iTempArr[2][0] = [%d], iTempArr[4][0]=%d", iTempArr[0][0], iTempArr[1][0], iTempArr[2][0], iTempArr[4][0]);
		for (int i = 0; i < 3; i++)
		{
			if (iTempArr[iMinType][0] < 3)
				iMinType++;
			if (iTempArr[i][0] < 3 || i == iMinType)
			{
				continue;
			}
			else
			{
				if (iTempArr[i][0] < iTempArr[iMinType][0])
				{
					iMinType = i;
				}
				else if (iTempArr[i][0] == iTempArr[iMinType][0])
				{
					int iTempA = 0;
					int iTempB = 0;
					for (int j = 0; j < 10; j++)
					{
						if (iTempArr[i][j] > 0)
							iTempA++;
						if (iTempArr[iMinType][j] > 0)
							iTempB++;
					}
					if (iTempA > iTempB)
						iMinType = i;
				}
			}
		}

		int iChoice = 0;
		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
		{
			int iValue = nodePlayers->cHandCards[i] & 0x000f;
			if (nodePlayers->cHandCards[i] >> 4 == iMinType && iTempArr[iMinType][iValue] == 1)
			{
				msgChangeCardReq.cCards[iChoice] = nodePlayers->cHandCards[i];
				iChoice++;
			}
			if (iChoice >= 3)
				break;
		}
		if (iChoice < 3)
		{
			for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
			{
				int iValue = nodePlayers->cHandCards[i] & 0x000f;
				if ((nodePlayers->cHandCards[i] >> 4) == iMinType && iTempArr[iMinType][iValue] > 1)
				{
					msgChangeCardReq.cCards[iChoice] = nodePlayers->cHandCards[i];
					iChoice++;
				}
				if (iChoice >= 3)
					break;
			}
		}
		HandleChangeCardReq(iUserID,(void*)&msgChangeCardReq);
	}
	else if(nodePlayers->iStatus == PS_WAIT_CONFIRM_QUE)
	{
		//_log(_DEBUG, "XLMJGameLogic", "AutoSendCards PS_WAIT_CONFIRMQUE");
		//if(nodePlayers->iQueType == -1)
		{

			int i;
			ConfirmQueReqDef msgReq;
			memset(&msgReq, 0, sizeof(ConfirmQueReqDef));



			int iTempArr[3] = { 0 };
			int iMinType = 0;
			for (int i = 0; i < sizeof(nodePlayers->cHandCards); i++)
			{
				int iType = nodePlayers->cHandCards[i] >> 4;

				iTempArr[iType]++;
			}

			for (int i = 0; i < 3; i++)
			{
				if (iTempArr[i] < iTempArr[iMinType])
				{
					iMinType = i;
				}
			}
			msgReq.iQueType = iMinType;
			HandleConfirmQueReq(iUserID, (void*)&msgReq);
		}
	}
	else if (nodePlayers->iStatus == PS_WAIT_SPECIAL)
	{
		SpecialCardsReqDef pSpecialMsg;
		memset(&pSpecialMsg, 0, sizeof(SpecialCardsReqDef));
		if ((nodePlayers->iSpecialFlag >> 5) == 1)
		{
			_log(_DEBUG, "HZXLGameLogic", "AutoSendCards PS_WAIT_SPECIAL  bAutoHu R[%d]T[%d]UID[%d] bAutoHu[%d]", iRoomNum, iTableNum, nodePlayers->iUserID, nodePlayers->bAutoHu);
			if (nodePlayers->bAutoHu || nodePlayers->bIsHu)
			{
				pSpecialMsg.iSpecialFlag = (1 << 5);
				pSpecialMsg.cFirstCard = pTableItem->cTableCard;
			}
		}
		HandleSpecialCard(nodePlayers->iUserID, (void *)&pSpecialMsg);
	}
}

//换牌请求
void HZXLGameLogic::HandleChangeCardReq(int iUserID, void *pMsgData)
{
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if(nodePlayers == NULL)
	{
		_log(_ERROR, "XLMJGameLogic", "HandleChangeCardReq: nodePlayers is NULL");
		return;
	}

	int iTableNum = nodePlayers->cTableNum;   //找出所在的桌
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomNum = nodePlayers->cRoomNum;

	if(iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR,"XLMJGameLogic","HandleChangeCardReq:[%d][%d][%d]",iRoomNum,iTableNum,iTableNumExtra);
		return;
	}

	//得到当前玩家桌子信息
	MJGTableItemDef *tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum-1,iTableNum-1);

	if(tableItem == NULL)
	{
		_log(_ERROR, "XLMJGameLogic", "HandleChangeCardReq tableItem is null szNickName = [%s]",nodePlayers->szNickName);
		return;
	}

	//非换牌状态请求肯定不对
	if(nodePlayers->iStatus != PS_WAIT_CHANGE_CARD)
	{
		_log(_ERROR, "XLMJGameLogic", "HandleChangeCardReq  roomID= %d T[%d] userID = [%d] szNickName = [%s],iTableNumExtra[%d],iStatus[%d]",
			nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, nodePlayers->szNickName,iTableNumExtra,nodePlayers->iStatus);

		SendKickOutWithMsg(nodePlayers, "不是换牌状态", 1301, 1);
		return;
	}

	//如果玩家已经请求过换牌了 直接返回得了
	if(nodePlayers->bChangeCardReq)
	{
		_log(_ERROR, "XLMJGameLogic", "HandleChangeCardReq : roomID= %d T[%d] userID = [%d] szNickName = [%s]  玩家已经请求过换牌了", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, nodePlayers->szNickName);
		return;
	}


	ChangeCardsReqDef *msg = (ChangeCardsReqDef*)pMsgData;

	//下面效验玩家请求的牌手上是否均有

	int iFindCardNum = 0;
	char cCardIndex[3];
	//将玩家手中的牌去掉
	for (int i = 0; i < 3; i++)
	{
		cCardIndex[i] = -1;
	}
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
		{
			if (cCardIndex[0] == j || cCardIndex[1] == j || cCardIndex[2] == j)
				continue;
			if (nodePlayers->cHandCards[j] == msg->cCards[i])
			{
				cCardIndex[iFindCardNum] = j;
				iFindCardNum++;
				break;
			}
		}
	}

	if(iFindCardNum != 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleChangeCardReq roomID= %d T[%d] userID = [%d] szNickName = [%s]  玩家请求所换的牌不匹配 iFindCardNum = %d",
			nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID , nodePlayers->szNickName, iFindCardNum);
		//for (int i = 0; i < 3; i++)
		//{
		//	//_log(_ERROR, "HZXLGameLogic", "HandleChangeCardReq roomID= %d T[%d] userID = [%d] szNickName = [%s] msg->cCards[%d] = [%d]",
		//		//nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, nodePlayers->szNickName, i, msg->cCards[i]);
		//}
		//for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
		//{
		//	_log(_ERROR, "XLGameLogic", "HandleChangeCardReq R[%d] T[%d] userID[%d] nodePlayers->cHandCards[%d] = [%d]",
		//		nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, j, nodePlayers->cHandCards[j]);
		//}
		SendKickOutWithMsg(nodePlayers, "客户端发来的牌不符合要求", 1301, 1);
		return;
	}

	nodePlayers->bChangeCardReq = true;

	_log(_DEBUG, "HZXLGameLogic", "HandleChangeCardReq R[%d]T[%d]UID[%d] bChangeCardReq == true", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID);

	//保存换牌
	for(int i=0; i<3; i++)
	{
		nodePlayers->cChangeReqCard[i] = msg->cCards[i];
		nodePlayers->cChangeIndex[i] = cCardIndex[i];
		_log(_DEBUG, "HZXLGameLogic", "HandleChangeCardReq R[%d]T[%d]UID[%d] szNickName = [%s] msg->cCards[%d] = [%d]",
			nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, nodePlayers->szNickName, i, msg->cCards[i]);
	}

	nodePlayers->iKickOutTime = 0;
	//通知桌上所有人  玩家换牌
	SureChangeCardNoticeDef msgNotice;
	memset(&msgNotice,0,sizeof(SureChangeCardNoticeDef));
	msgNotice.iTableNumExtra = iTableNumExtra;
	memcpy(msgNotice.cSourceCards,nodePlayers->cChangeReqCard,sizeof(msgNotice.cSourceCards));


	for(int i=0; i<PLAYER_NUM; i++)
	{
		if(tableItem->pPlayers[i] != NULL)
		{
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex,&msgNotice,SURE_CHANGE_CARD_NOTICE_MSG,sizeof(SureChangeCardNoticeDef));
		}
	}

	//下面判断是否所有人都发了换牌请求了 如果是 就开始换牌了
	int iAlReqChangeCardNum = 0;

	for(int i=0; i<PLAYER_NUM; i++)
	{
		if(tableItem->pPlayers[i] != NULL)
		{
			if(tableItem->pPlayers[i]->bChangeCardReq)
			{
				iAlReqChangeCardNum++;
			}
		}
	}
	//如果是和AI的对局，玩家请求换牌后即可开始换牌 
	if (tableItem->bIfExistAIPlayer)
	{
		//AI创建换牌消息
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->iPlayerType != NORMAL_PLAYER)
			{
				//_log(_DEBUG, "HZXLGameLogic", "HandleChangeCardReq Create AI[%d] Change CardsMsg", tableItem->pPlayers[i]->iPlayerType);
				m_pHzxlAICTL->CreateAIChangeCardReq(tableItem,tableItem->pPlayers[i]);
			}
		}
	}

	_log(_DEBUG,"HZXLGameLogic","HandleChangeCardReq R[%d]T[%d]UID[%d] iAlReqChangeCardNum[%d]", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID , iAlReqChangeCardNum);
	//所有人都请求换牌了
	if(iAlReqChangeCardNum == PLAYER_NUM)
	{
		//换牌
		CallBackChangeCard(tableItem);
	}
}

void HZXLGameLogic::CallBackChangeCard(MJGTableItemDef *tableItem)
{
	if(tableItem == NULL)
	{
		return;
	}
	if (!tableItem->bIfExistAIPlayer)
	{
		char cCardType[4];
		memset(cCardType, 0, sizeof(cCardType));
		//得到桌上玩家请求所换的牌
		for(int i=0; i<PLAYER_NUM; i++)
		{
			if(tableItem->pPlayers[i] != NULL)
			{
				cCardType[i] = tableItem->pPlayers[i]->cChangeReqCard[0] >> 4;
			}
		}

		//下面对玩家换牌及通知
		ChangeCardsNoticeDef msgNotice[4];
		memset(msgNotice,0,sizeof(msgNotice));


		//先确定怎么换 0顺时针  1逆时针  2对家
		int iChangeType = rand() % 3;

		//GameConf::GetInstance()->ReadGameConf("xlmj_game.conf");
		if (GameConf::GetInstance()->m_iChangeCard != -1)
		{
			iChangeType = GameConf::GetInstance()->m_iChangeCard;
		}

		int iSame = 0;
		if(iChangeType == 0)
		{
			if (cCardType[0] == cCardType[3])
				iSame++;
			for (int i = 3; i > 0;i--)
			{
				if (cCardType[i] == cCardType[i - 1])
					iSame++;
			}
		}
		else if(iChangeType == 1)
		{
			if (cCardType[3] == cCardType[0])
				iSame++;
			for (int i = 0; i < 3;i++)
			{
				if (cCardType[i] == cCardType[i + 1])
					iSame++;
			}
		}
		else
		{
			if (cCardType[0] == cCardType[2] && cCardType[1] == cCardType[3])
			{
				iSame == 2;
				if (cCardType[3] == cCardType[0])
					iSame++;
				for (int i = 0; i < 3;i++)
				{
					if (cCardType[i] == cCardType[i + 1])
						iSame++;
				}
			}
		}

		int iChangeOptimize = GameConf::GetInstance()->m_iChangeOptimize;

		if (iChangeOptimize > 0 && iSame == 2)
		{
			int iTempOptimize = random() % 100;
			if (iChangeOptimize == 100 || iTempOptimize < iChangeOptimize)
			{
				if (iChangeType == 2)
				{
					iChangeType = random() % 2;
				}
				else
				{
					iChangeType = 2;
				}

			}
		}
		_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard T[%d], iChangeType = [%d]", tableItem->iTableID, iChangeType);
		if (iChangeType == 0)
		{
			for (int i = 0; i < 4;i++)
			{
				int iTag = (i + 1) % 4;
				if (tableItem->pPlayers[i] && tableItem->pPlayers[iTag])
				{
					for (int j = 0; j < 3; j++)
					{
						int iIndex = tableItem->pPlayers[i]->cChangeIndex[j];
						bool bChangeMoCard = false;
						if (tableItem->pPlayers[i]->cTableNumExtra == tableItem->iZhuangPos && tableItem->pPlayers[i]->cHandCards[iIndex] == tableItem->cCurMoCard && iIndex == 13)
						{
							bChangeMoCard = true;
						}
						tableItem->pPlayers[i]->cHandCards[iIndex] = tableItem->pPlayers[iTag]->cChangeReqCard[j];
						msgNotice[i].cCards[j] = tableItem->pPlayers[i]->cHandCards[iIndex];
						if (bChangeMoCard)
						{
							tableItem->cCurMoCard = tableItem->pPlayers[i]->cHandCards[iIndex];
							tableItem->pPlayers[i]->cMoCardsNum = tableItem->cCurMoCard;
						}
					}
				}
			}
		}
		else if (iChangeType == 1)
		{
			for (int i = 3; i >= 0;i--)
			{
				int iTag = i - 1;
				if (i == 0)
					iTag = 3;
				if (tableItem->pPlayers[i] && tableItem->pPlayers[iTag])
				{
					for (int j = 0; j < 3; j++)
					{
						int iIndex = tableItem->pPlayers[i]->cChangeIndex[j];
						bool bChangeMoCard = false;
						if (tableItem->pPlayers[i]->cTableNumExtra == tableItem->iZhuangPos && tableItem->pPlayers[i]->cHandCards[iIndex] == tableItem->cCurMoCard && iIndex == 13)
						{
							bChangeMoCard = true;
						}
						tableItem->pPlayers[i]->cHandCards[iIndex] = tableItem->pPlayers[iTag]->cChangeReqCard[j];
						msgNotice[i].cCards[j] = tableItem->pPlayers[i]->cHandCards[iIndex];
						if (bChangeMoCard)
						{
							tableItem->cCurMoCard = tableItem->pPlayers[i]->cHandCards[iIndex];
							tableItem->pPlayers[i]->cMoCardsNum = tableItem->cCurMoCard;
						}
					}
				}
			}
		}
		//玩家换牌后如果庄家摸到的牌被换走了 那么table上的 CurMoCard 变更
		else
		{
			for (int i = 0; i < 4;i++)
			{
				int iTag = (i + 2) % 4;
				if (tableItem->pPlayers[i] && tableItem->pPlayers[iTag])
				{
					for (int j = 0; j < 3; j++)
					{
						bool bChangeMoCard = false;
						int iIndex = tableItem->pPlayers[i]->cChangeIndex[j];
						_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard i[%d] j[%d], iIndex[%d], cTableNumExtra[%d], iZhuangPos[%d], cHandCards[iIndex][%d], cCurMoCard[%d]",
							i, j, iIndex, tableItem->pPlayers[i]->cTableNumExtra, tableItem->iZhuangPos, tableItem->pPlayers[i]->cHandCards[iIndex], tableItem->cCurMoCard);
						if (tableItem->pPlayers[i]->cTableNumExtra == tableItem->iZhuangPos && tableItem->pPlayers[i]->cHandCards[iIndex] == tableItem->cCurMoCard && iIndex == 13)
						{
							bChangeMoCard = true;
							//tableItem->cCurMoCard = tableItem->pPlayers[i]->cHandCards[iIndex];
							//tableItem->pPlayers[i]->cMoCardsNum = tableItem->cCurMoCard;
						}
						tableItem->pPlayers[i]->cHandCards[iIndex] = tableItem->pPlayers[iTag]->cChangeReqCard[j];
						msgNotice[i].cCards[j] = tableItem->pPlayers[i]->cHandCards[iIndex];
						_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard Type == 2 cCurMoCard[%d], bChangeMoCard[%d]", tableItem->cCurMoCard, bChangeMoCard);

						if (bChangeMoCard)
						{
							tableItem->cCurMoCard = tableItem->pPlayers[i]->cHandCards[iIndex];
							tableItem->pPlayers[i]->cMoCardsNum = tableItem->cCurMoCard;
						}
					}
				}
			}
		}

		for(int i=0; i<PLAYER_NUM; i++)
		{
			if(tableItem->pPlayers[i] != NULL)
			{
				msgNotice[i].cTableNumExtra = i;
				msgNotice[i].iChangeType = iChangeType;
				for(int j=0; j<3; j++)
				{
					msgNotice[i].cSourceCards[j] = tableItem->pPlayers[i]->cChangeReqCard[j];
				}
			}
		}
		for(int i=0; i<PLAYER_NUM; i++)
		{
			if(tableItem->pPlayers[i] != NULL)
			{
				//通知换牌
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotice[i], CHANGE_CARDS_NOTICE_MSG, sizeof(ChangeCardsNoticeDef));
			}
		}
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] != NULL)
			{
				//开始定缺
				ConfirmQueServerDef pMsg;
				memset(&pMsg, 0, sizeof(ConfirmQueServerDef));

				int iMinType = 0;
				iMinType = CalCulateQueType(tableItem->pPlayers[i]);
				pMsg.iQueType = iMinType;

				_log(_DEBUG, "HZXLGameLogic", " CallBackChangeCard R[%d]T[%d]UID[%d] DingQue pMsg.iQueType = %d", tableItem->pPlayers[i]->cRoomNum, tableItem->iTableID, tableItem->pPlayers[i]->iUserID, pMsg.iQueType);

				tableItem->pPlayers[i]->iQueType = -1;
				tableItem->pPlayers[i]->iStatus = PS_WAIT_CONFIRM_QUE;
				pMsg.cTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;

				tableItem->pPlayers[i]->iKickOutTime = KICK_OUT_TIME;
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsg, XLMJ_CONFIRM_QUE_SERVER, sizeof(ConfirmQueServerDef));
			}
		}
	}
	//AI对局中处理换牌，AI不做换牌，从牌墙中随机取三张作为普通玩家的换牌
	else
	{
		int iMainAIPos = -1;
		int iAssistAIPos = -1;
		int iNormalPalyerPos = -1; 
		int iDeputyAIPos = -1;
		vector<char>vecChangeCards;
		vector<char>vecChangeCardsValue;
		vector<char>vecSourceCards;
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i]->iPlayerType ==  MIAN_AI_PLAYER)
			{
				iMainAIPos = tableItem->pPlayers[i]->cTableNumExtra;
			}
			else if (tableItem->pPlayers[i]->iPlayerType ==  ASSIST_AI_PLAYER)
			{
				iAssistAIPos = tableItem->pPlayers[i]->cTableNumExtra;
			}
			else if (tableItem->pPlayers[i]->iPlayerType ==  NORMAL_PLAYER)
			{
				iNormalPalyerPos = tableItem->pPlayers[i]->cTableNumExtra;
			}
			else if (tableItem->pPlayers[i]->iPlayerType == DEPUTY_AI_PLAYER)
			{
				iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
			}
		}
		for (int i = 0; i < 3; i++)
		{
			vecSourceCards.push_back(tableItem->pPlayers[iNormalPalyerPos]->cChangeReqCard[i]);
			_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard vecSourceCards = [%d]", vecSourceCards[i]);
		}
		int iChangeType;
		if (iMainAIPos == 1)
		{
			//主AI在下家，不可是逆时针换牌
			int iRandType = rand() % 2;
			if (iRandType == 0)
			{
				iChangeType = 0;
			}
			else
			{
				iChangeType = 2;
			}
		}
		else if (iMainAIPos == 2)
		{
			//主AI在对家，不可对家换牌
			iChangeType = rand() % 2;
		}
		_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard iChangeType = [%d]", iChangeType);
		int iExchangePos = -1;
		if (iChangeType == 0)
		{
			//顺时针换牌，从3号位取三张
			iExchangePos = 3;
		}
		else if (iChangeType == 1)
		{
			iExchangePos = 1;
		}
		else if (iChangeType == 2)
		{
			iExchangePos = 2;
		}
		_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard iExchangePos = [%d]", iExchangePos);
		for (int i = 0; i < 3; i++)
		{
			_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard i = [%d]", i);
			for (int j = 0; j < tableItem->pPlayers[iExchangePos]->iHandCardsNum; j++)
			{
				bool bIfFind = (find(vecSourceCards.begin(), vecSourceCards.end(), tableItem->pPlayers[iExchangePos]->cHandCards[j]) == vecSourceCards.end());
				bool bIfFindBefore = (find(vecChangeCardsValue.begin(), vecChangeCardsValue.end(), tableItem->pPlayers[iExchangePos]->cHandCards[j]) == vecChangeCardsValue.end());
				_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard bIfFind[%d] bIfFindBefore[%d]", bIfFind, bIfFindBefore);
				if (tableItem->pPlayers[iExchangePos]->cHandCards[j] != 65 && bIfFind && bIfFindBefore)
				{
					vecChangeCards.push_back(j);
					vecChangeCardsValue.push_back(tableItem->pPlayers[iExchangePos]->cHandCards[j]);
					_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard vecChangeCards = [%d]", tableItem->pPlayers[iExchangePos]->cHandCards[j]);
					break;
				}
			}
		}
		/*for (int i = tableItem->iCardIndex; i < tableItem->iMaxMoCardNum; i++)
		{
			if (find(tableItem->pPlayers[iMainAIPos]->vecAIHuCard.begin(), tableItem->pPlayers[iMainAIPos]->vecAIHuCard.end(), tableItem->cAllCards[i]) == tableItem->pPlayers[iMainAIPos]->vecAIHuCard.end())
			{
				if (tableItem->cAllCards[i] != 65)
				{
					vecChangeCards.push_back(i);
				}
			}
			if (vecChangeCards.size() == 3)
			{
				break;
			}
		}*/
		//对普通玩家发送换牌通知
		ChangeCardsNoticeDef msgNotice[4];
		memset(&msgNotice, 0, sizeof(msgNotice));
		
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (i == iNormalPalyerPos)
			{
				if (tableItem->pPlayers[iNormalPalyerPos])
				{
					_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard iNormalPalyerPos[%d]", iNormalPalyerPos);
					msgNotice[iNormalPalyerPos].iChangeType = iChangeType;
					msgNotice[iNormalPalyerPos].cTableNumExtra = iNormalPalyerPos;
					for (int j = 0; j < 3; j++)
					{
						msgNotice[iNormalPalyerPos].cSourceCards[j] = tableItem->pPlayers[iNormalPalyerPos]->cChangeReqCard[j];
					}
				}
				else
				{
					_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard node null", iNormalPalyerPos);
				}
			}
			else
			{
				msgNotice[i].cTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;
				msgNotice[i].iChangeType = iChangeType;
				for (int j = 0; j < 3; j++)
				{
					msgNotice[i].cSourceCards[j] = tableItem->pPlayers[i]->cChangeReqCard[j];
					msgNotice[i].cCards[j] = 0;
				}
			}
		}
		//处理换牌
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] != NULL)
			{
				if (i == iNormalPalyerPos)
				{
					for (int j = 0; j < 3; j++)
					{
						int iIndex = tableItem->pPlayers[iNormalPalyerPos]->cChangeIndex[j];
						bool bChangeMoCard = false;
						if (iNormalPalyerPos == tableItem->iZhuangPos && tableItem->pPlayers[iNormalPalyerPos]->cHandCards[iIndex] == tableItem->cCurMoCard && iIndex == 13)
						{
							bChangeMoCard = true;
							_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard  ChangeMoCard True");
						}
						char cTmpCard = tableItem->pPlayers[iExchangePos]->cHandCards[vecChangeCards[j]];
						tableItem->pPlayers[iExchangePos]->cHandCards[vecChangeCards[j]] = tableItem->pPlayers[iNormalPalyerPos]->cHandCards[iIndex];
						tableItem->pPlayers[iNormalPalyerPos]->cHandCards[iIndex] = cTmpCard;
						msgNotice[iNormalPalyerPos].cCards[j] = tableItem->pPlayers[iNormalPalyerPos]->cHandCards[iIndex];
						//vecChangeCards.erase(vecChangeCards.begin());
						if (bChangeMoCard)
						{
							tableItem->cCurMoCard = tableItem->pPlayers[iNormalPalyerPos]->cHandCards[iIndex];
							tableItem->pPlayers[iNormalPalyerPos]->cMoCardsNum = tableItem->cCurMoCard;
						}
						_log(_DEBUG, "XLMJGameLogic", "CallBackChangeCard i[%d]iIndex[%d], cSourceCards[%d], cNowHandCards[iIndex][%d],iZhuangPos[%d], cCurMoCard[%d]", i, iIndex, tableItem->pPlayers[iNormalPalyerPos]->cChangeReqCard[j], tableItem->pPlayers[iNormalPalyerPos]->cHandCards[iIndex], tableItem->iZhuangPos, tableItem->cCurMoCard);
					}
					CLSendSimpleNetMessage(tableItem->pPlayers[iNormalPalyerPos]->iSocketIndex, &msgNotice, CHANGE_CARDS_NOTICE_MSG, sizeof(ChangeCardsNoticeDef));
				}
			}
		}
		//处理定缺
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] != NULL && tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
			{
				//开始定缺
				ConfirmQueServerDef pMsg;
				memset(&pMsg, 0, sizeof(ConfirmQueServerDef));

				int iMinType = 0;
				iMinType = CalCulateQueType(tableItem->pPlayers[i]);
				pMsg.iQueType = iMinType;

				_log(_DEBUG, "HZXLGameLogic", " CallBackChangeCard R[%d]T[%d]UID[%d] DingQue pMsg.iQueType = %d", tableItem->pPlayers[i]->cRoomNum, tableItem->iTableID, tableItem->pPlayers[i]->iUserID, pMsg.iQueType);

				tableItem->pPlayers[i]->iQueType = -1;
				tableItem->pPlayers[i]->iStatus = PS_WAIT_CONFIRM_QUE;
				pMsg.cTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;

				tableItem->pPlayers[i]->iKickOutTime = KICK_OUT_TIME;
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsg, XLMJ_CONFIRM_QUE_SERVER, sizeof(ConfirmQueServerDef));
			}
			//AI相关字段处理
			tableItem->pPlayers[i]->iQueType = -1;
			tableItem->pPlayers[i]->iStatus = PS_WAIT_CONFIRM_QUE;
			tableItem->pPlayers[i]->iKickOutTime = KICK_OUT_TIME;
		}

	}
	
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] != NULL)
		{
			if (tableItem->pPlayers[i]->bIfWaitLoginTime == true || tableItem->pPlayers[i]->cDisconnectType == 1)
			{
				AutoSendCards(tableItem->pPlayers[i]->iUserID);
			}
		}
	}
}

// 计算玩家的定缺类型
int HZXLGameLogic::CalCulateQueType(MJGPlayerNodeDef *nodePlayers)
{
	if (nodePlayers == NULL)
		return 0;
	int iTempArr[3] = { 0 };
	int iMinType = 0;
	for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
	{
		if (nodePlayers->cHandCards[j] == 0)
			continue;
		int iType = nodePlayers->cHandCards[j] >> 4;

		if(iType > -1 && iType < 3)
			iTempArr[iType]++;

	}

	for (int j = 0; j < 3; j++)
	{
		if (iTempArr[j] < iTempArr[iMinType])
		{
			iMinType = j;
		}
	}

	if (iMinType < 0 || iMinType > 2)
		iMinType = 0;

	return iMinType;
}

void HZXLGameLogic::DoOneDay()
{
}

//梭哈类游戏要重新获取一些相关用户信息
void HZXLGameLogic::CallBackRDUserInfo(FactoryPlayerNodeDef *nodePlayers)
{
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "XLMJGameLogic", "CallBackRDUserInfo Error nodePlayers = NULL");
		return;
	}
	MJGPlayerNodeDef *nodePlayer = (MJGPlayerNodeDef*)nodePlayers;
	char cIfLoginAgain = 0;
	SendRoomInfoToClient(nodePlayer, cIfLoginAgain);

	//发送活动币：测试用
	//if (m_pAnniversaryChristmas)
	//{
	//	m_pAnniversaryChristmas->SendActiNum(nodePlayer, 5, 100, 5);
	//}
}


void HZXLGameLogic::SendRoomInfoToClient(MJGPlayerNodeDef *nodePlayers, char cIfLoginAgain)
{
	if (nodePlayers == NULL)
		return;

	RoomInfoServerDef pMsg;
	memset(&pMsg, 0, sizeof(RoomInfoServerDef));

	int iRoomIndex = nodePlayers->cRoomNum - 1;
	int iRoomType = m_pServerBaseInfo->stRoom[iRoomIndex].iRoomType;			// 房间类型
	long long iBaseScore = stSysConfBaseInfo.stRoom[iRoomIndex].iBasePoint;				// 基础底分
	long long iCurTableMoney = stSysConfBaseInfo.stRoom[iRoomIndex].iTableMoney;			// 桌费
	int iMinPoint = (int)stSysConfBaseInfo.stRoom[iRoomIndex].iMinPoint;
	long long iMaxPoint = stSysConfBaseInfo.stRoom[iRoomIndex].iMaxPoint;

	pMsg.iEnterRoomType = iRoomType;
	pMsg.iBaseScore = iBaseScore;
	pMsg.iTableMoney = iCurTableMoney;
	pMsg.cIfLoginAgain = cIfLoginAgain;

	_log(_DEBUG, "HZXLGameLogic", "SendRoomInfoToClient:UID[%d] iIfFreeRoom[%d] iBaseScore[%lld] iCurTableMoney[%lld]", nodePlayers->iUserID, iRoomType, iBaseScore, iCurTableMoney);
	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &pMsg, HZXLMJ_ROOM_INFO_MSG, sizeof(RoomInfoServerDef));

	GameInfoServerDef pGameInfoMsg;
	memset(&pGameInfoMsg, 0, sizeof(GameInfoServerDef));

	pGameInfoMsg.iEnterRoomType = iRoomType;
	pGameInfoMsg.iFengDingFan = iMinPoint;
	pGameInfoMsg.iMaxPoint = iMaxPoint;
	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &pGameInfoMsg, HZXLMJ_GAME_INFO_MSG, sizeof(GameInfoServerDef));
}

void HZXLGameLogic::HandleAgainLoginReq(int iUserID, int iSocketIndex)
{

	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLGameLogic", "●●●● HandleSendCardsReq:nodePlayers is NULL ●●●●");
		return;
	}

	int iTableNum = nodePlayers->cTableNum;
	char iTableNumExtra = nodePlayers->cTableNumExtra;
	char iRoomNum = nodePlayers->cRoomNum;
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleAgainLoginReq:R[%d] T[%d] I[%d]", iRoomNum, iTableNum, iTableNumExtra);
		return;
	}

	MJGTableItemDef* pTableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);

	//基本恢复牌局结构体
	GameAgainLoginRes pMsg;
	memset(&pMsg, 0, sizeof(GameAgainLoginRes));
	SetAgainLoginBaseNew((AgainLoginResBaseDef*)&pMsg, pTableItem, nodePlayers, iSocketIndex);//调用底层方法填充公共信息

	GameAgainLoginTingRes pTingMsg;
	memset(&pTingMsg, 0, sizeof(GameAgainLoginTingRes));

	GameAgainLoginLiuShuiRes pLiuShuiMsg;
	memset(&pLiuShuiMsg, 0, sizeof(GameAgainLoginLiuShuiRes));

	pMsg.stAgainLoginBase.iStructSize = htonl(sizeof(GameAgainLoginRes));//指定实际结构体大小
	pMsg.iZhuangPos = pTableItem->iZhuangPos;
	pMsg.iCurMoPlayer = pTableItem->iCurMoPlayer;
	pMsg.iCurSendPlayer = pTableItem->iCurSendPlayer;
	pMsg.iNormalPlayerNum = GetNormalPlayerNum(pTableItem);
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (pTableItem->pPlayers[i])
		{
			if (pTableItem->pPlayers[i]->cPoChan == 0)
				pMsg.vcPlayerState[i] = PLAYER_STATE_NORMAL;
			else if (pTableItem->pPlayers[i]->cPoChan == 1)
				pMsg.vcPlayerState[i] = PLAYER_STATE_BANKRUPT;
		}
		else
			pMsg.vcPlayerState[i] = PLAYER_STATE_LEAVE;
	}

	pTingMsg.iCurMoPlayer = pTableItem->iCurMoPlayer;
	pTingMsg.cTableNumExtra = nodePlayers->cTableNumExtra;
	for (int i = 0; i < 14; i++)
	{
		pMsg.cHandCards[i] = nodePlayers->cHandCards[i];
	}

	// 发送玩家当前剩余金币，以及玩家视角下其他玩家的剩余金币
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (i == nodePlayers->cTableNumExtra)
			pMsg.iCurMoney[i] = nodePlayers->iMoney + nodePlayers->iCurWinMoney;
		else
			pMsg.iCurMoney[i] = nodePlayers->otherMoney[i].llMoney + nodePlayers->iAllUserCurWinMoney[i];

		_log(_DEBUG, "HZXLGameLogic", "HandleAgainLoginReq:R[%d]T[%d]UID[%d], pMsg.iCurMoney[%lld] = %d, iCurWinMoney[%lld]",
			iRoomNum, iTableNum, nodePlayers->iUserID, i, pMsg.iCurMoney[i], nodePlayers->iCurWinMoney);
	}

	if (nodePlayers->bTing || nodePlayers->bIsHu)
	{
		if (nodePlayers->bIsHu && !nodePlayers->bTing)
		{
			char cCard = pTableItem->cTableCard;
			if (cCard == 0)
			{
				cCard = pTableItem->cCurMoCard;
			}
			vector<char> vcCards;
			for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
			{
				vcCards.push_back(nodePlayers->cHandCards[i]);
			}
			vcCards.push_back(cCard);
			map<char, vector<char> > mpTing;
			map<char, map<char, int> > mpTingFan;
			int iFlag = JudgeSpecialTing(nodePlayers, pTableItem, nodePlayers->resultType, vcCards, mpTing, mpTingFan);// 断线重连时，计算玩家能听的牌

			if (iFlag > 0 && mpTing.find(cCard) != mpTing.end() && !mpTing[cCard].empty())
			{
				vector<char>().swap(nodePlayers->vcSendTingCard);
				//给玩家听的牌 赋值
				nodePlayers->bTing = true;
				vector<char>().swap(nodePlayers->vcTingCard);
				nodePlayers->vcTingCard.insert(nodePlayers->vcTingCard.begin(), mpTing[cCard].begin(), mpTing[cCard].end());
				map<char, int>().swap(nodePlayers->mpTingFan);
				nodePlayers->mpTingFan = mpTingFan[cCard];
				int iTempMaxFan = 0;
				map<char, int>::iterator pos = mpTingFan[cCard].begin();
				while (pos != mpTingFan[cCard].end())
				{
					if (pos->second > iTempMaxFan)
						iTempMaxFan = pos->second;
					pos++;
				}
				nodePlayers->iTingHuFan = iTempMaxFan;
			}
		}
		pMsg.cAutoHu = nodePlayers->bAutoHu ? 1 : 0;
		for (int i = 0; i < nodePlayers->vcTingCard.size(); i++)
		{
			pMsg.cTingCard[i] = nodePlayers->vcTingCard[i];
			pMsg.iTingFan[i] = nodePlayers->mpTingFan[nodePlayers->vcTingCard[i]];
		}
	}
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (pTableItem->pPlayers[i])
		{
			if (pTableItem->pPlayers[i]->bTing)
			{
				pMsg.iTingPlyerPos[i] = 1;
			}
			pMsg.iHandCardNum[i] = pTableItem->pPlayers[i]->iHandCardsNum;
			//pMsg.cpgInfo[i] = tableItem->pPlayers[i]->CPGInfo;
			for (int j = 0; j < 34; j++)
			{
				pMsg.cSendCards[i][j] = pTableItem->pPlayers[i]->cSendCards[j];
			}
		}
	}
	pMsg.cTableCard = pTableItem->cTableCard;

	pMsg.iLeftCardNum = pTableItem->iMaxMoCardNum - pTableItem->iCardIndex;

	_log(_DEBUG, "HZXLGameLogic", "HandleAgainLoginReq:R[%d]T[%d]UID[%d], iStatus = %d", iRoomNum, iTableNum, nodePlayers->iUserID, nodePlayers->iStatus);

	map<char, vector<char> > mpTing;
	map<char, map<char, int> > mpTingFan;
	if (nodePlayers->iStatus == PS_WAIT_SEND)
	{
		if (pTableItem->iCurMoPlayer == nodePlayers->cTableNumExtra)
		{
			pMsg.cCurMoCard = pTableItem->cCurMoCard;
			pMsg.iSpecialFlag = nodePlayers->iSpecialFlag;

			int iFlag = JudgeSpecialGang(nodePlayers, pTableItem, pMsg.iGangNum, pMsg.iGangCard, pMsg.cGangType);
			_log(_DEBUG, "hzxlmj_gamelogic", "HandleAgainLoginReq:R[%d]T[%d] UID[%d] 杠牌flag  座位[%d], 杠牌flag[%d] 摸到的牌[%d]", iRoomNum, iTableNum, nodePlayers->iUserID, nodePlayers->cTableNumExtra, iFlag, pTableItem->cCurMoCard);
			vector<char> vcCards;
			for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
			{
				vcCards.push_back(nodePlayers->cHandCards[j]);
			}

			iFlag = JudgeSpecialTing(nodePlayers, pTableItem, nodePlayers->resultType, vcCards, mpTing, mpTingFan);		// 断线重连，某个玩家出牌阶段，判断玩家是否能听牌
			if (iFlag > 0)
			{
				vector<char>().swap(nodePlayers->vcSendTingCard);
				map<char, vector<char> >::iterator pos = mpTing.begin();
				map<char, map<char, int> >::iterator fanPos = mpTingFan.begin();
				int iIndex = 0;
				while (fanPos != mpTingFan.end())
				{
					pTingMsg.iTingSend[iIndex] = fanPos->first;
					int m = 0;
					for (map<char, int>::iterator fan = fanPos->second.begin(); fan != fanPos->second.end(); fan++)
					{
						pTingMsg.iTingHuCard[iIndex][m] = fan->first;
						m++;
					}
					pTingMsg.iTingHuNum[iIndex] = m;
					fanPos++;
					iIndex++;
				}
				pTingMsg.iTingNum = iIndex;
				nodePlayers->iSpecialFlag |= 0x0010;
			}

		}
	}
	else if (nodePlayers->iStatus == PS_WAIT_SPECIAL)
	{
		pMsg.iSpecialFlag = nodePlayers->iSpecialFlag;
	}
	else if (nodePlayers->iStatus == PS_WAIT_CHANGE_CARD)
	{
		pMsg.bIsChange = true;
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			pMsg.iChangeExtra[i] = -1;
			if(pTableItem->pPlayers[i] && pTableItem->pPlayers[i]->bChangeCardReq)
				pMsg.iChangeExtra[i] = i;
		}
		if (nodePlayers->bChangeCardReq)
		{
			memcpy(pMsg.cChangeCards, nodePlayers->cChangeReqCard, sizeof(pMsg.cChangeCards));
		}
	}
	else if (nodePlayers->iStatus == PS_WAIT_CONFIRM_QUE)
	{
		pMsg.cIsQue = 1;
	}
	else if (nodePlayers->iStatus == PS_WAIT_RESERVE_TWO)
	{
	}
	int iDuoXiangNum = 0;

	// 玩家胡过的牌
	vector <HuCardInfoDef>::iterator HuCardPos = pTableItem->vcAllHuCard.begin();
	int iNum = 0;
	while (HuCardPos != pTableItem->vcAllHuCard.end())
	{
		pMsg.cHuCardInfo[iNum] = *HuCardPos;
		iNum++;
		HuCardPos++;
		if (iNum >= 56)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleAgainLoginReq  R[%d]T[%d]   vcAllHuCard.size ===%d iNum = %d", iRoomNum, iTableNum, pTableItem->vcAllHuCard.size(), iNum);
			break;
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		pMsg.iQueType[i] = -1;
		if (pTableItem->pPlayers[i])
			_log(_DEBUG, "HZXLGameLogic", "HandleAgainLoginReq  UID[%d], tableItem->pPlayers[%d]->bConfirmQueReq = %d", pTableItem->pPlayers[i]->iUserID, i, pTableItem->pPlayers[i]->bConfirmQueReq);
		if (pTableItem->pPlayers[i]->bConfirmQueReq)
		{
			pMsg.iQueType[i] = pTableItem->pPlayers[i]->iQueType;
		}
		else
		{
			if (i == nodePlayers->cTableNumExtra)
			{
				pMsg.iQueType[i] = CalCulateQueType(pTableItem->pPlayers[i]);
			}
		}

		if (pTableItem->pPlayers[i]->bIsHu)
		{
			pMsg.cHuExtra[i] = 1;
		}
		if (pTableItem->pPlayers[i]->cPoChan > 0)
		{
			pMsg.cPoChan[i] = 1;
		}
	}

	int iLiuShuiCnt = 0;
	vector <GameLiuShuiInfoDef>::iterator pos = pTableItem->vLiuShuiList[nodePlayers->cTableNumExtra].begin();
	GameLiuShuiInfoDef lsInfo[56];
	while (pos != pTableItem->vLiuShuiList[nodePlayers->cTableNumExtra].end())
	{
		if ((pos->cWinner & (1 << nodePlayers->cTableNumExtra)) || (pos->cLoser & (1 << nodePlayers->cTableNumExtra)))
		{
			lsInfo[iLiuShuiCnt] = *pos;
			_log(_DEBUG, "HZXLGameLogic", "HandleAgainLoginReq_log() iLiuShuiCnt[%d], cWinner[%d], cLoser[%d], iSpecialFlag[%d]", iLiuShuiCnt, pos->cWinner, pos->cLoser, pos->iSpecialFlag);
			iLiuShuiCnt++;
		}
		pos++;
	}

	pLiuShuiMsg.iLiuShui = iLiuShuiCnt;
	char cTempBuffer[1024 * 5];
	memset(cTempBuffer,0,sizeof(cTempBuffer));
	int iTempBufferLen = sizeof(GameAgainLoginRes);
	memcpy(cTempBuffer, &pMsg, iTempBufferLen);
	memcpy(cTempBuffer + iTempBufferLen, m_PlayerInfoDesk, sizeof(PlayerInfoResNorDef)* m_iPlayerInfoDeskNum);
	iTempBufferLen += sizeof(PlayerInfoResNorDef) * m_iPlayerInfoDeskNum;
	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, cTempBuffer, AGAIN_LOGIN_RES_MSG, iTempBufferLen);
	SendExtraAgainLoginToClient(nodePlayers, pTableItem);

	char cIfLoginAgain = 1;
	SendRoomInfoToClient(nodePlayers, cIfLoginAgain);

	if (pTingMsg.iTingNum > 0 && nodePlayers->iStatus != PS_WAIT_RESERVE_TWO && nodePlayers->iStatus > PS_WAIT_READY)
	{
		pTingMsg.iSpecialFlag = nodePlayers->iSpecialFlag;

		char **iTingHuFan = new char *[pTingMsg.iTingNum];
		int iTingSize = 0;
		for (int i = 0; i < pTingMsg.iTingNum; i++)
		{
			if(pTingMsg.iTingHuNum[i] > 0)
				iTingHuFan[i] = new char [pTingMsg.iTingHuNum[i]+1];
			iTingSize += pTingMsg.iTingHuNum[i]+1;
		}

		char *szBuff = new char[sizeof(GameAginLoginTingResDef) + iTingSize];
		memset(szBuff, 0, sizeof(GameAginLoginTingResDef) + iTingSize);
		memcpy(szBuff, &pTingMsg, sizeof(GameAginLoginTingResDef));

		map<char, vector<char> >::iterator pos = mpTing.begin();
		map<char, map<char, int> >::iterator fanPos = mpTingFan.begin();
		int iIndex = 0;
		int iTempSize = 0;
		while (fanPos != mpTingFan.end())
		{
			pTingMsg.iTingSend[iIndex] = fanPos->first;
			int m = 0;
			for (map<char, int>::iterator fan = fanPos->second.begin(); fan != fanPos->second.end(); fan++)
			{
				iTingHuFan[iIndex][m] = fan->second;
				m++;
			}
			memcpy(szBuff + sizeof(GameAginLoginTingResDef)+ iTempSize, iTingHuFan[iIndex], m);
			iTempSize += m;
			fanPos++;
			iIndex++;
		}
		//_log(_DEBUG, "HZXLGameLogic", "HandleAgainLoginReq:R=[%d] T[%d] UserID = %d pTingMsg  iIndex = %d,  iTempSize = %d  iTingSize = %d", iRoomNum, iTableNum, nodePlayers->iUserID, iTempSize, iTingSize);
		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, szBuff, XLMJ_AGAIN_LOGIN_TING, sizeof(GameAginLoginTingResDef)+iTingSize);

		memset(szBuff, 0, sizeof(GameAginLoginTingResDef) + iTingSize);
		delete[] szBuff;
		szBuff = NULL;
		for (int i = 0; i < pTingMsg.iTingNum; i++)
		{
			memset(iTingHuFan[i], 0, pTingMsg.iTingHuNum[i] + 1);
			delete[] iTingHuFan[i];
			iTingHuFan[i] = NULL;
		}
		memset(iTingHuFan, 0, pTingMsg.iTingNum);
		delete[] iTingHuFan;
		iTingHuFan = NULL;
	}

	if (pLiuShuiMsg.iLiuShui > 0 && nodePlayers->iStatus != PS_WAIT_RESERVE_TWO && nodePlayers->iStatus > PS_WAIT_READY)
	{
		int iLiuShuiSize = sizeof(GameLiuShuiInfoDef)*pLiuShuiMsg.iLiuShui;
		GameLiuShuiInfoDef * pTempLs = new GameLiuShuiInfoDef[pLiuShuiMsg.iLiuShui];
		memset(pTempLs, 0, iLiuShuiSize);
		memcpy(pTempLs, lsInfo, iLiuShuiSize);
		char *szBuff = new char[sizeof(GameAgainLoginLiuShuiResDef) + iLiuShuiSize];
		memset(szBuff, 0, sizeof(GameAgainLoginLiuShuiResDef) + iLiuShuiSize);
		memcpy(szBuff, &pLiuShuiMsg, sizeof(GameAgainLoginLiuShuiResDef));
		memcpy(szBuff + sizeof(GameAgainLoginLiuShuiResDef), pTempLs, iLiuShuiSize);

		CLSendSimpleNetMessage(nodePlayers->iSocketIndex, szBuff, XLMJ_AGAIN_LOGIN_LIUSHUI, sizeof(GameAgainLoginLiuShuiResDef)+ iLiuShuiSize);

		memset(szBuff, 0, sizeof(GameAgainLoginLiuShuiResDef) + iLiuShuiSize);
		delete[] szBuff;
		szBuff = NULL;
		memset(pTempLs, 0, iLiuShuiSize);
		delete[] pTempLs;
		pTempLs = NULL;
	}

	// 发送玩家胡牌次数
	GameAgainLoginHuInfoResDef pHuInfoRes;
	memset(&pHuInfoRes, 0, sizeof(GameAgainLoginHuInfoResDef));
	for (int i = 0; i < MAX_PLAYER_NUM; i++)
	{
		if (pTableItem->pPlayers[i])
		{
			pHuInfoRes.vcCurHuCount[i] = pTableItem->pPlayers[i]->vcHuCard.size();
			_log(_DEBUG, "HZXLGameLogic", "HandleAgainLoginRes UserID[%d], vcCurHuCount[%d] = %d", pTableItem->pPlayers[i]->iUserID, i, pHuInfoRes.vcCurHuCount[i]);
		}
	}
	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &pHuInfoRes, XLMJ_AGAIN_LOGIN_HU_INFO, sizeof(GameAgainLoginHuInfoResDef));

	//底层相关
	AgainLoginOtherHandle(nodePlayers);
}

bool HZXLGameLogic::CheckIfPlayWithControlAI(FactoryPlayerNodeDef* nodePlayers)
{
	
	if (nodePlayers)
	{
		((MJGPlayerNodeDef*)nodePlayers)->bNeedMatchAI = false;
		((MJGPlayerNodeDef*)nodePlayers)->bOnlyMatchAI = false;
	}

	//如果开启AI配置
	if (m_pHzxlAICTL->m_iIfOpenAI == 1)
	{
		int iPlayerWaitAiTime = 0;
		bool bBrushScorePlayer = false;
		bool bCanWaitPlayer = false;				// 是否需要寻找真实玩家
		if (nodePlayers->iSpeMark == AI_MARK_BRUSH_PLAYER)
		{
			bBrushScorePlayer = true;
		}
		if (bBrushScorePlayer)
		{
			iPlayerWaitAiTime = m_pHzxlAICTL->m_iDoubtfulWaitAiTimeLow + rand() % (m_pHzxlAICTL->m_iDoubtfulWaitAiTimeHigh - m_pHzxlAICTL->m_iDoubtfulWaitAiTimeLow);
			((MJGPlayerNodeDef*)nodePlayers)->bNeedMatchAI = true;
			_log(_DEBUG, "HZXLGameLogic", "HandleSitDownReq_Log()Brush UserID[%d] iPlayerWaitAiTime[%d]", nodePlayers->iUserID, iPlayerWaitAiTime);
		}
		else
		{
			if (m_pHzxlAICTL->m_iIfNormalCanMatchNormal == 1)
			{
				bool bIfControl = false;
				//判断当前是否需要进入控制流程
				bIfControl = m_pHzxlAICTL->JudgePlayerBeforeSit(((MJGPlayerNodeDef*)nodePlayers));
				if (!bIfControl)
				{
					//当前玩家不需要进入控制
					_log(_ERROR, "HZXLGameLogic", "HandleSitDownReq_Log() UserID[%d] Don't Need To Be Controlled", nodePlayers->iUserID);
					bCanWaitPlayer = true;
				}
				else
				{
					//当前玩家需要进入控制
					_log(_ERROR, "HZXLGameLogic", "HandleSitDownReq_Log() UserID[%d] Need To Be Controlled", nodePlayers->iUserID);
					((MJGPlayerNodeDef*)nodePlayers)->iPlayerType =  NORMAL_PLAYER;
					((MJGPlayerNodeDef*)nodePlayers)->bIfControl = true;
					((MJGPlayerNodeDef*)nodePlayers)->bNeedMatchAI = true;
					//((MJGPlayerNodeDef*)nodePlayers)->bOnlyMatchAI = true;
					bCanWaitPlayer = false;
					iPlayerWaitAiTime = m_pHzxlAICTL->m_iNormalWaitAiTimeLow + rand() % (m_pHzxlAICTL->m_iNormalWaitAiTimeHigh - m_pHzxlAICTL->m_iNormalWaitAiTimeLow);
					_log(_DEBUG, "HZXLGameLogic", "HandleSitDownReq_Log() UserID[%d] m_iNormalWaitAiTimeLow[%d]", nodePlayers->iUserID, iPlayerWaitAiTime);
				}
			}
			else
			{
				((MJGPlayerNodeDef*)nodePlayers)->iPlayerType =  NORMAL_PLAYER;
				((MJGPlayerNodeDef*)nodePlayers)->bIfControl = true;
				((MJGPlayerNodeDef*)nodePlayers)->bNeedMatchAI = true;
				bCanWaitPlayer = false;
				iPlayerWaitAiTime = m_pHzxlAICTL->m_iNormalWaitAiTimeLow + rand() % (m_pHzxlAICTL->m_iNormalWaitAiTimeHigh - m_pHzxlAICTL->m_iNormalWaitAiTimeLow);
			}
		}
		if (nodePlayers)
		{
			((MJGPlayerNodeDef*)nodePlayers)->iWaitAiTime = iPlayerWaitAiTime;
			_log(_DEBUG, "HZXLGameLogic", "HandleSitDownReq_Log() UserID[%d] bBrushScorePlayer[%d], iPlayerWaitAiTime[%d], bCanWaitPlayer[%d]", nodePlayers->iUserID, bBrushScorePlayer, iPlayerWaitAiTime, bCanWaitPlayer);
		}
		if (!bCanWaitPlayer)
		{
			_log(_DEBUG, "HZXLGameLogic", "UserID[%d] HandleSitDownReq_Log() Find AI", nodePlayers->iUserID);
			FindEmptyTableWithAI(nodePlayers);
		}
	}
	if (((MJGPlayerNodeDef*)nodePlayers)->bNeedMatchAI)
	{
		return true;
	}
	return false;
}


void HZXLGameLogic::CallBackReadyReq(void *pPlayerNode, void *pTableItem, int iRoomID)
{

	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef*)pPlayerNode;
	MJGTableItemDef *tableItem = (MJGTableItemDef*)pTableItem;

	SetCallBackReadyBaseInfo(tableItem, nodePlayers);
	ResetTableState(tableItem);

	//备份玩家节点,一定要做
	for (int i = 0; i<10; i++)
	{
		tableItem->pPlayers[i] = (MJGPlayerNodeDef*)tableItem->pFactoryPlayers[i];
	}

	//初始化本桌玩家节点信息
	for (int i = 0; i<m_iMaxPlayerNum; i++)
	{
		if (tableItem->pPlayers[i] != NULL)
		{
			tableItem->pPlayers[i]->bTempMidway = false;
			tableItem->pPlayers[i]->iKickOutTime = 0;
		}
	}
	//检测当前桌是否有AI
	int iNormalPlayerPos = -1;
	int iMainAIPos = -1;
	int iAssistAIPos = -1;
	int iDeputyAIPos = -1;
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (tableItem->pPlayers[i]->iPlayerType > 0)
		{
			//当前桌上存在AI节点
			tableItem->bIfExistAIPlayer = true;
		}
		if (tableItem->pPlayers[i]->iPlayerType ==  NORMAL_PLAYER)
		{
			
			iNormalPlayerPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		else if (tableItem->pPlayers[i]->iPlayerType ==  MIAN_AI_PLAYER)
		{
			iMainAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		else if (tableItem->pPlayers[i]->iPlayerType ==  ASSIST_AI_PLAYER)
		{
			iAssistAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		else if (tableItem->pPlayers[i]->iPlayerType ==  DEPUTY_AI_PLAYER)
		{
			iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
	}
	// 重置玩家信息
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		tableItem->iTingPlyerPos[i] = -1;
		tableItem->pPlayers[i]->Reset();

		if (tableItem->bIfExistAIPlayer)
		{
			if (i == iMainAIPos)
			{
				//_log(_ERROR, "HZXLGameLogic", "HandleSitDownReq_Log() 1111" );
				tableItem->pPlayers[i]->iPlayerType =  MIAN_AI_PLAYER;
				tableItem->pPlayers[i]->bIfAINode = true;
			}
			else if (i == iAssistAIPos)
			{
				//_log(_ERROR, "HZXLGameLogic", "HandleSitDownReq_Log() 2222");
				tableItem->pPlayers[i]->iPlayerType =  ASSIST_AI_PLAYER;
				tableItem->pPlayers[i]->bIfAINode = true;
			}
			else if (i == iDeputyAIPos)
			{
				//_log(_ERROR, "HZXLGameLogic", "HandleSitDownReq_Log() 3333");
				tableItem->pPlayers[i]->iPlayerType =  DEPUTY_AI_PLAYER;
				tableItem->pPlayers[i]->bIfAINode = true;
			}
			else
			{
				//_log(_ERROR, "HZXLGameLogic", "HandleSitDownReq_Log() 0000");
				tableItem->pPlayers[i]->iPlayerType =  NORMAL_PLAYER;
			}
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++){
		_log(_DEBUG,"HZXLGameLogic","CallBackReadyReq R[%d]T[%d]UID[%d],szNickName[%s]->iMoney[%d], iCurWinMoney[%d] iPlayerType[%d]",
			nodePlayers->cRoomNum, tableItem->iTableID, tableItem->pPlayers[i]->iUserID, tableItem->pPlayers[i]->szNickName, tableItem->pPlayers[i]->iMoney, tableItem->pPlayers[i]->iCurWinMoney, tableItem->pPlayers[i]->iPlayerType);
	}
	CallBackGameStart(nodePlayers, tableItem);

}

void HZXLGameLogic::ResetTableInfo()
{
	int i, j, k;
	for (k = 0; k<MAX_ROOM_NUM; k++)
	{
		for (i = 0; i<MAX_TABLE_NUM; i++)
		{
			m_tbItem[k][i].Reset();
			m_tbItem[k][i].iAllCardNum = HZXL_MAX_CARD_NUM;
		}
	}
}

// 所有玩家准备好了，游戏开始
void HZXLGameLogic::CallBackGameStart(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem)
{
	_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() Start");

	if (GameConf::GetInstance()->m_iIfPeiPai == 0)
	{
		_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() m_iIfPeiPai == 0");
		CreateCard(tableItem);
		_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart CreateCard_Log() End");
	}
	else
	{
		_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() m_iIfPeiPai == 1");
		//测试代码 自己配牌		
		int iAllCard[5][10] = { 0 };
		for (int i = 0; i < GameConf::GetInstance()->m_vcPeiPai.size(); i++)
		{
			tableItem->cAllCards[i] = GameConf::GetInstance()->m_vcPeiPai[i];
			if (GameConf::GetInstance()->m_vcPeiPai[i] > 0)
			{
				int iType = GameConf::GetInstance()->m_vcPeiPai[i] >> 4;
				int iValue = GameConf::GetInstance()->m_vcPeiPai[i] & 0x0f;

				if (iAllCard[iType][iValue] >= 4)
				{
					tableItem->cAllCards[i] = 0;
				}
				else
				{
					iAllCard[iType][0] ++;
					iAllCard[iType][iValue] ++;
				}

				//_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() tableItem->cAllCards[%d] = %d", i, tableItem->cAllCards[i]);
			}
		}

		for (int i = 0; i < HZXL_MAX_CARD_NUM; i++)
		{
			if (tableItem->cAllCards[i] == 0)
			{
				int iRand = rand() % 37;
				if (iRand >= 27 && iRand < 36)
				{
					iRand = 36;							// 红中
				}

				int iType = iRand / 9;					// 万条筒
				int iValue = iRand % 9 + 1;				// 1-9

				while (iAllCard[iType][iValue] == 4)
				{
					iRand = rand() % 37;
					if (iRand >= 27 && iRand < 36)
					{
						iRand = 36;
					}

					iType = iRand / 9;
					iValue = iRand % 9 + 1;
				}

				iAllCard[iType][0] ++;
				iAllCard[iType][iValue] ++;
				tableItem->cAllCards[i] = ((iType << 4) | iValue);
			}
		}
	}

	_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart Save iZhuangPos[%d]", tableItem->iZhuangPos);

	//将刚刚创建的牌 保存到玩家节点
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] != NULL)
		{
			//CleanPlayerNode函数中已经把玩家手中的牌清0过啦
			for (int j = 0; j < 13; j++)
			{
				tableItem->pPlayers[i]->cHandCards[j] = tableItem->cAllCards[i * 13 + j];//每个玩家先发13张
				//_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log player[%d] type[%d],cHandCards[%d]",i, tableItem->pPlayers[i]->iPlayerType, tableItem->pPlayers[i]->cHandCards[j]);
			}

			tableItem->pPlayers[i]->iHandCardsNum = 13;
			tableItem->pPlayers[i]->iStatus = PS_WAIT_SEND;             //等待发牌状态		
		}
	}
	tableItem->iCardIndex = 0;
	tableItem->iCardIndex += 13 * 4;                                                //cAllCards数组中玩家摸牌的下标 

	tableItem->iMaxMoCardNum = HZXL_MAX_CARD_NUM;

	int iPoint1 = (int)((double)rand() / RAND_MAX * 6) + 1;
	int iPoint2 = (int)((double)rand() / RAND_MAX * 6) + 1;

	tableItem->iZhuangPos = (iPoint1 + iPoint2) % PLAYER_NUM;

	_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart iZhuangPos[%d]", tableItem->iZhuangPos);

	if (GameConf::GetInstance()->m_iZhuangPos != -1)
	{
		tableItem->iZhuangPos = GameConf::GetInstance()->m_iZhuangPos;
	}

	_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() iZhuangPos[%d], iCardIndex[%d], iMaxMoCardNum[%d]", tableItem->iZhuangPos, tableItem->iCardIndex, tableItem->iMaxMoCardNum);

	//发送发牌消息
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		DealCardsServerDef pMsg;
		memset(&pMsg,0,sizeof(DealCardsServerDef));
		pMsg.cPoint[0] = iPoint1;
		pMsg.cPoint[1] = iPoint2;
		pMsg.cZhuangPos = tableItem->iZhuangPos;
		memset(pMsg.viPlayerMoney, 0, sizeof(pMsg.viPlayerMoney));

		if (tableItem->pPlayers[i] != NULL)
		{
			memcpy(pMsg.cCards, tableItem->pPlayers[i]->cHandCards, sizeof(pMsg.cCards));

			long long viShowPlayerMoney[PLAYER_NUM] = { 0, 0, 0, 0 };		// 显示模拟玩家的金币
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (i == j)
					viShowPlayerMoney[j] = tableItem->pPlayers[i]->iMoney;
				else
					viShowPlayerMoney[j] = tableItem->pPlayers[i]->otherMoney[j].llMoney;
			}
			memcpy(pMsg.viPlayerMoney, viShowPlayerMoney, sizeof(viShowPlayerMoney));

			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsg, HEBMJ_DEAL_CARD_SERVER, sizeof(DealCardsServerDef));
			_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() sendDealCard  pMsg.iPlayerMoney[%d] = %d", i, pMsg.viPlayerMoney[i]);
		}
		_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() sendDealCard  R=%d, T[%d], userid[%d]", tableItem->pPlayers[i]->cRoomNum, tableItem->iTableID, tableItem->pPlayers[i]->iUserID);
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() tableItem->pPlayers[%d]->iMoney = %d, UID[%d]", i, tableItem->pPlayers[i]->iMoney, tableItem->pPlayers[i]->iUserID);
		//for (int j = 0; j < PLAYER_NUM; j++)
		//{
		//	_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() tableItem->pPlayers[%d]->otherMoney[%d] = %d", i, j, tableItem->pPlayers[i]->otherMoney[j].llMoney);
		//	_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() tableItem->pPlayers[%d]->otherMoney[%d] = %d", i, j, tableItem->pPlayers[i]->otherMoney[j].llMoney);
		//}
	}

	for (int i = 0; i<PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			ChangeCardServerDef pMsg;
			memset(&pMsg, 0, sizeof(ChangeCardServerDef));

			if (i == tableItem->iZhuangPos)
			{
				m_pHzxlAICTL->CTLGetCardControl(tableItem, tableItem->pPlayers[i]);
				
				tableItem->cCurMoCard = tableItem->cAllCards[tableItem->iCardIndex];
				tableItem->pPlayers[i]->cMoCardsNum = tableItem->cCurMoCard;
				tableItem->pPlayers[i]->iHandCardsNum += 1;
				tableItem->pPlayers[i]->cHandCards[tableItem->pPlayers[i]->iHandCardsNum - 1] = tableItem->pPlayers[i]->cMoCardsNum;  //把摸到的牌给玩家
				tableItem->iCardIndex++; //
				pMsg.cZhuangMoCard = tableItem->cCurMoCard;
				_log(_DEBUG, "HZXLGameLogic", "CallBackGameStart_Log() 庄第 14 张 R[%d] T[%d], userid =%d  cCurMoCard = %d", tableItem->pPlayers[i]->cRoomNum, tableItem->iTableID, tableItem->pPlayers[i]->iUserID, pMsg.cZhuangMoCard);
			}
			pMsg.cZhuangPos = tableItem->iZhuangPos;
			tableItem->pPlayers[i]->iStatus = PS_WAIT_CHANGE_CARD;
			pMsg.cTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;

			tableItem->pPlayers[i]->iKickOutTime = KICK_OUT_TIME;
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsg, XLMJ_CHANGE_CARD_SERVER, sizeof(ChangeCardServerDef));
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			if (tableItem->pPlayers[i]->bIfWaitLoginTime == true || tableItem->pPlayers[i]->cDisconnectType == 1)
			{
				AutoSendCards(tableItem->pPlayers[i]->iUserID);
			}
		}
	}
}

// 创建所有的手牌
void HZXLGameLogic::CreateCard(MJGTableItemDef *tableItem)
{
	//控制流程下配置手牌
	_log(_DEBUG, "HZXLGameLogic", "CreateCard_Log() bIfExistAIPlayer[%d]", tableItem->bIfExistAIPlayer);
	if (tableItem->bIfExistAIPlayer)
	{
		HZXLControlCards CTLControlCard;
		//如果存在AI就控制配牌
		for (int i = 0; i < m_iMaxPlayerNum; i++)
		{
			_log(_DEBUG, "HZXLGameLogic", "CreateCard_Log() iPlayerType[%d]", tableItem->pPlayers[i]->iPlayerType);
			if (tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
			{
				m_pHzxlAICTL->SetPlayerIfControl(tableItem, tableItem->pPlayers[i], CTLControlCard);//设置相关概率，控制条件
				break;
			}
		}
		int iCTLCardsSize = m_pHzxlAICTL->GetCtlCardSize();
		_log(_DEBUG, "HZXLGameLogic", "CreateCard_Log() iCTLCardsSize[%d]", iCTLCardsSize);
		if (iCTLCardsSize != 0)
		{
			int iRandCards = rand() % iCTLCardsSize;
			m_pHzxlAICTL->CTLSetControlCard(tableItem, iRandCards);//控制流程下AI配牌
		}
		else
		{
			_log(_ERROR, "HZXLGameLogic", "CreateCard_Log() iCTLCardsSize=0");
		}
	}
	else
	{
		int  i, j;
		//初始化牌,不包括花牌
		memset(tableItem->cAllCards, 0, HZXL_MAX_CARD_NUM);
		int iCardType, iCardValue;
		for (j = 0; j < 4; j++)
		{
			for (i = 0; i < 27; i++)     //0-26
			{
				iCardType = i / 9;			//牌型0-2,0为万，1为条，2为饼
				iCardValue = i % 9 + 1;		//牌值1-9
				tableItem->cAllCards[i + 27 * j] = iCardValue | (iCardType << 4);//高四位牌型，低四位牌值
			}
		}

		for (j = 0; j < 4; j++)
		{
			iCardType = 4;
			iCardValue = 1;
			tableItem->cAllCards[108 + j] = iCardValue | (iCardType << 4);  //红中 65
		}

		//洗牌
		for (i = 0; i<HZXL_MAX_CARD_NUM * 10; i++)
		{
			int iRandNum = CRandKiss::g_comRandKiss.RandKiss() % HZXL_MAX_CARD_NUM;
			char temp = tableItem->cAllCards[i%HZXL_MAX_CARD_NUM];
			tableItem->cAllCards[i%HZXL_MAX_CARD_NUM] = tableItem->cAllCards[iRandNum];
			tableItem->cAllCards[iRandNum] = temp;
		}

		for (i = 0; i<HZXL_MAX_CARD_NUM; i++)
		{
			if (tableItem->cAllCards[i] == 0)
			{
				_log(_ERROR, "xlmjGameLogic", "血流麻将CreateCard:ALL CARD ERROR INDEX[%d]", i);
			}
		}
	}
}

void HZXLGameLogic:: SendCardToPlayer(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, bool bCP /*= false*/, bool bFirst /*= false*/)
{

	if (nodePlayers == NULL || tableItem == NULL)
	{
		_log(_ERROR, "HZXLGameLogic", "SendCardToPlayer_Log() nodePlayers = NULL or tableItem = NULL");
		return;
	}

	bool bLastGang = false;
	_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log() UID[%d], iSpecialFlag[%d]", nodePlayers->iUserID, nodePlayers->iSpecialFlag);
	if (nodePlayers->iSpecialFlag & 0x000C)
	{
		nodePlayers->bLastGang = true;
		bLastGang = true;
	}
	else
	{
		nodePlayers->bLastGang = false;
	}

	// 其他玩家的杠牌状态置0，防止在自家杠上开花后，下一轮自家摸牌之前，如果胡牌了，会误计算成杠上开花
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			if(i != nodePlayers->cTableNumExtra)
				tableItem->pPlayers[i]->bLastGang = false;
			tableItem->pPlayers[i]->bGuoHu = false;				// 重置玩家过胡状态
		}
	}

	if (tableItem->cLeastPengExtra > -1 && tableItem->cLeastPengExtra != nodePlayers->cTableNumExtra)	// 上家碰牌，不是下家摸牌
		tableItem->cLeastPengExtra = -1;

	nodePlayers->iStatus = PS_WAIT_SEND;
	nodePlayers->bGuoHu = false;
	nodePlayers->bCanQiangGangHu = false;

	//将玩家的吃碰听杠胡状态清零
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->cPoChan == 0)
		{
			tableItem->pPlayers[i]->iSpecialFlag = 0;
			memset(&tableItem->pPlayers[i]->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
		}
	}

	//当前出牌的玩家的位置置为 -1
	tableItem->iCurSendPlayer = -1;
	if(bFirst == false)
		tableItem->cCurMoCard = 0;

	//如果是吃碰之后的摸牌 //碰了之后判断能不能杠  听
	if (bCP)
	{
		_log(_DEBUG, "HZXLGameLogic", " SendCardToPlayer_Log() R[%d]T[%d]UID[%d] After Chi Peng: TableExtra = %d", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, nodePlayers->cTableNumExtra);
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			SendCardsServerDef pSendMsg;
			memset(&pSendMsg, 0, sizeof(SendCardsServerDef));
			pSendMsg.cTableNumExtra = nodePlayers->cTableNumExtra;

			pSendMsg.cCard = -1;
			if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER))
			{
				//普通玩家吃碰后需要发送出牌请求
				if (i == nodePlayers->cTableNumExtra)
				{
					nodePlayers->iKickOutTime = KICK_OUT_TIME;
					int iSpecialFlag = 0;

					_log(_DEBUG, "HZXLGameLogic", " SendCardToPlayer_Log() nodePlayers->iHandCardsNum[%d]", nodePlayers->iHandCardsNum);
					vector<char> vcCards;
					int iNum = 0;
					for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
						vcCards.push_back(nodePlayers->cHandCards[j]);

					map<char, vector<char> > mpTing;
					map<char, map<char, int> > mpTingFan;

					int iFlag = JudgeSpecialTing(nodePlayers, tableItem, nodePlayers->resultType,vcCards,mpTing, mpTingFan); // 吃碰后的出牌，判断能否听牌
					if (iFlag > 0)
					{
						vector<char>().swap(nodePlayers->vcSendTingCard);				// 清空向量
						map<char, map<char, int> >::iterator fanPos = mpTingFan.begin();
						int iIndex = 0;

						while (fanPos != mpTingFan.end())
						{
							pSendMsg.iTingSend[iIndex] = fanPos->first;
							int m = 0;
							for (map<char, int>::iterator fan = fanPos->second.begin(); fan != fanPos->second.end(); fan++)
							{
								pSendMsg.iTingHuCard[iIndex][m] = fan->first;
								pSendMsg.iTingHuFan[iIndex][m] = fan->second;
								m++;
							}
							fanPos++;
							iIndex++;
						}
						pSendMsg.iTingNum = iIndex;
					}

					iSpecialFlag |= iFlag << 4;					// 代表听牌
					pSendMsg.iSpecialFlag = iSpecialFlag;
				}
			}
			else
			{
				//AI直接调用一次创建出牌消息
				if (i == nodePlayers->cTableNumExtra)
				{
					m_pHzxlAICTL->CreateAISendCardReq(tableItem, nodePlayers);
				}
			}
			if (tableItem->pPlayers[i])
			{
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pSendMsg, MJG_MO_CARD_SERVER, sizeof(SendCardsServerDef));
			}

			//如果玩家掉线 自动帮玩家打牌
			if (nodePlayers->bIfWaitLoginTime == true || nodePlayers->cDisconnectType == 1)
			{
				AutoSendCards(nodePlayers->iUserID);
			}
		}
		return;
	}

	//判断 如果当前摸牌 到最大的摸牌数量的时候 流局
	if (tableItem->iCardIndex == tableItem->iMaxMoCardNum)
	{
		_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log() iCardIndex[%d], iMaxMoCardNum[%d], iHasHuNum[%d]", tableItem->iCardIndex, tableItem->iMaxMoCardNum, tableItem->iHasHuNum);
		if (tableItem->iHasHuNum < 3)
		{
			//流局
			GameEndHu(nodePlayers, tableItem, true, 0);		// 当前摸牌达到最大的摸牌数量
			return;
		}
		GameEndHu(nodePlayers, tableItem, false, 0);		// 当前摸牌达到最大的摸牌数量
		return;
	}

	if (bFirst == false)
	{
		//当前是摸牌，将摸到的牌 加到玩家的牌的数组里面
		if (tableItem->bIfExistAIPlayer)
		{
			//如果是AI对局，那就需要进入摸牌控制
			m_pHzxlAICTL->CTLGetCardControl(tableItem, nodePlayers);
		}
		tableItem->cCurMoCard = tableItem->cAllCards[tableItem->iCardIndex];
		nodePlayers->cMoCardsNum = tableItem->cCurMoCard;
		nodePlayers->iHandCardsNum += 1;
		nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1] = nodePlayers->cMoCardsNum;  //把摸到的牌给玩家
		tableItem->iCardIndex++;
	}
	_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log():UID[%d], cExtra[%d] cCurMoCard[%d] iCardIndex[%d]", nodePlayers->iUserID, nodePlayers->cTableNumExtra, tableItem->cCurMoCard, tableItem->iCardIndex);

	bool bHuPai = false;
	int iTempFan = 0;

	char cFanResult[MJ_FAN_TYPE_CNT];
	memset(cFanResult, 0, sizeof(cFanResult));
	iTempFan = JudgeSpecialHu(nodePlayers, tableItem, cFanResult);   // 发牌给某个玩家，判断能不能胡牌    （玩家节点和桌子节点）  直接返回胡的牌型（比如碰碰胡）									  
	if (iTempFan > 0)									  // 如果番数大于0 
	{
		bHuPai = true;
		nodePlayers->iHuFan = iTempFan;
		memcpy(nodePlayers->cFanResult, cFanResult, sizeof(cFanResult));
		_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log(): UID[%d], iTempFan[%d]", nodePlayers->iUserID, iTempFan);
	}
	else
		memset(nodePlayers->cFanResult, 0, sizeof(nodePlayers->cFanResult));

	//先发送摸牌消息
	//非AI对局 or AI对局中普通玩家的摸牌消息处理
	if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && nodePlayers->iPlayerType ==  NORMAL_PLAYER))
	{
		for(int i = 0; i < PLAYER_NUM; i++)
		{
			SendCardsServerDef pSendMsg;
			memset(&pSendMsg,0,sizeof(SendCardsServerDef));
			pSendMsg.cTableNumExtra = nodePlayers->cTableNumExtra;
			if (bFirst)
			{ 
				pSendMsg.cFirst = 1;
			}
			if(i == nodePlayers->cTableNumExtra)
			{
				nodePlayers->iKickOutTime = KICK_OUT_TIME;
				pSendMsg.cCard = tableItem->cCurMoCard;
				int iSpecialFlag = 0;
				int iFlag = JudgeSpecialGang(nodePlayers, tableItem, pSendMsg.iGangNum, pSendMsg.iGangCard, pSendMsg.cGangType);
				if (iFlag>0)
				{
					_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log() Gang iFlag = %d, nodePlayers->iHandCardsNum[%d]", iFlag, nodePlayers->iHandCardsNum);
				}
				iSpecialFlag |= iFlag << 2;     //位运算不同的位代表不同的flag  有没有杠

				vector<char> vcCards;
				int iNum = 0;
				for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
				{
					vcCards.push_back(nodePlayers->cHandCards[j]);
				}

				map<char, vector<char> > mpTing;  //经过JudgeSpecialTing后mpTing保存的是ting的牌
				map<char, map<char, int> > mpTingFan;
				iFlag = JudgeSpecialTing(nodePlayers, tableItem, nodePlayers->resultType, vcCards, mpTing, mpTingFan); // 给玩家发一张牌，判断能否听牌
				if (iFlag > 0)
				{
					vector<char>().swap(nodePlayers->vcSendTingCard);
					map<char, map<char, int> >::iterator fanPos = mpTingFan.begin();
					int iIndex = 0;

					while (fanPos != mpTingFan.end())
					{
						pSendMsg.iTingSend[iIndex] = fanPos->first;
						int m = 0;
						for (map<char, int>::iterator fan = fanPos->second.begin(); fan != fanPos->second.end(); fan++)
						{
							pSendMsg.iTingHuCard[iIndex][m] = fan->first;
							pSendMsg.iTingHuFan[iIndex][m] = fan->second;
							m++;
						}
						fanPos++;
						iIndex++;
						if (iIndex == 14)
							break;
					}
					pSendMsg.iTingNum = iIndex;
					_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log() Ting iFlag = %d, iIndex[%d]", iFlag, iIndex);
				}
				iSpecialFlag |= iFlag << 4;
				pSendMsg.iSpecialFlag = iSpecialFlag;  //有没有听
				nodePlayers->iSpecialFlag = iSpecialFlag;
				//胡牌
				if(bHuPai)
				{
					pSendMsg.cIfHu = 1;
					iSpecialFlag |= 1 << 6;		//40 zimo 20 hu
					pSendMsg.iSpecialFlag = iSpecialFlag;  //有没有胡
					nodePlayers->iSpecialFlag = iSpecialFlag;
					tableItem->iWillHuNum = 1;

					int iTotalMulti = CalculateTotalMulti(nodePlayers);
					pSendMsg.iHuMulti = iTotalMulti;
				}
				else
				{
					if (nodePlayers->iContinousZiMoCnt >= 0)	// 还未连续自摸，可以判断连续自摸，并重新开始计次
					{
						nodePlayers->iContinousZiMoCnt = 0;
					}
					tableItem->cLeastPengExtra = -1;			// 玩家不能自摸胡牌，上家碰牌的玩家重置
				}
				_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log() iUserID[%d], iFlag[%d], iSpecialFlag[%d], bIsHu[%d]", nodePlayers->iUserID, iFlag, iSpecialFlag, nodePlayers->bIsHu);
			}

			if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->bTempMidway == false)
			{
				_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log() i[%d], iUserID[%d]", i, tableItem->pPlayers[i]->iUserID);
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pSendMsg, MJG_MO_CARD_SERVER, sizeof(SendCardsServerDef));
			}
		}

		if (nodePlayers->bIfWaitLoginTime == true || nodePlayers->cDisconnectType == 1)
		{
			AutoSendCards(nodePlayers->iUserID);
		}
	}
	else
	{   //发送摸牌通知
		SendCardsServerDef pSendMsg;
		memset(&pSendMsg, 0, sizeof(SendCardsServerDef));
		pSendMsg.cTableNumExtra = nodePlayers->cTableNumExtra;
		if (bFirst)
		{
			pSendMsg.cFirst = 1;
		}
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->bTempMidway == false)
			{
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pSendMsg, MJG_MO_CARD_SERVER, sizeof(SendCardsServerDef));
			}
		}

		//AI对局中，AI自摸胡or摸牌后的出牌的处理
		if (nodePlayers->iPlayerType !=  NORMAL_PLAYER)
		{
			if (iTempFan > 0)
			{
				_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log(): AI HU 处理 iTempFan[%d]", iTempFan);
				//当前AI存在自摸的胡的可能
				nodePlayers->iSpecialFlag = 1 << 6;
				m_pHzxlAICTL->CreateAISpcialCardReq(tableItem, nodePlayers);
			}
			else
			{
				//不能自摸判断是否能杠
				int iSpecialFlag = 0;
				int iFlag = JudgeSpecialGang(nodePlayers, tableItem, pSendMsg.iGangNum, pSendMsg.iGangCard, pSendMsg.cGangType);
				_log(_DEBUG, "HZXLGameLogic", "SendCardToPlayer_Log(): iFlag[%d]", iFlag);
				if (iFlag > 0)
				{
					iSpecialFlag |= iFlag << 2;     //位运算不同的位代表不同的flag  有没有杠
					nodePlayers->iSpecialFlag = iSpecialFlag;
					m_pHzxlAICTL->CreateAISpcialCardReq(tableItem, nodePlayers);
				}
				//创建AI的出牌消息
				else
				{
					m_pHzxlAICTL->CreateAISendCardReq(tableItem, nodePlayers);
				}
			}
		}
	}
}

int HZXLGameLogic::JudgeSpecialPeng(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem)
{
	if (nodePlayers == NULL)
		return 0;

	char cCard = tableItem->cTableCard;
	//_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialPeng  Start UID[%d], bIsHu[%d], cCard[%d]", nodePlayers->iUserID, nodePlayers->bIsHu, cCard);
	//玩家状态要是等待发牌状态
	if (nodePlayers->iStatus != PS_WAIT_SEND)
	{
		_log(_ERROR, "HZXLGameLogic", "JudgeSpecialPeng  R[%d] T[%d] UserID = %d   ++,状态不对", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID);
		return 0;
	}
	if (nodePlayers->bIsHu == true)
	{
		return 0;
	}

	//判断当前玩家手中是否有可以碰的牌
	if (tableItem->iCurSendPlayer == nodePlayers->cTableNumExtra)
	{
		_log(_ERROR, "HZXLGameLogic", "JudgeSpecialPeng R[%d] T[%d] UserID = %d ++,是出牌玩家", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID);
		return 0;
	}

	int iAllNum = 0;

	if (nodePlayers->iQueType == (cCard >> 4))
	{
		return 0;
	}

	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		if (nodePlayers->cHandCards[i] == cCard && (cCard >> 4) != nodePlayers->iQueType)
		{
			iAllNum++;
		}
	}
	//_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialPengb         ++, iAllNum = %d",iAllNum);
	if (iAllNum >= 2)
	{
		_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialPeng  R[%d]T[%d]UID[%d] 可以碰", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID);
		return 1;
	}
	return 0;
}

int HZXLGameLogic::JudgeSpecialGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, int &iGameNum, char iGangCard[], char iGangType[])//, bool bGuaDaFeng /*= false*/
{
	//有两种情况 一种是玩家是摸牌的玩家  一种是别的玩家出牌
	//玩家状态要是等待发牌状态
	iGameNum = 0;
	if (nodePlayers->iStatus != PS_WAIT_SEND)
	{
		_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialGang_log() ERR Status UID[%d] iStatus[%d]", nodePlayers->iUserID, nodePlayers->iStatus);
		return 0;
	}

	//如果没有可以摸的牌 那就不能杠了
	if (tableItem->iCardIndex == tableItem->iMaxMoCardNum)
	{
		return 0;
	}

	// 如果玩家已经胡牌，并且胡七对，则不能杠牌(明杠，暗杠，补杠都不行)
	if (nodePlayers->bIsHu)
	{
		if (nodePlayers->cFanResult[LIAN_QI_DUI] > 0 || nodePlayers->cFanResult[LONG_QI_DUI] > 0 || nodePlayers->cFanResult[QI_DUI] > 0)
		{
			_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialGang_log() UID[%d], LIAN_QI_DUI[%d], LONG_QI_DUI[%d], QI_DUI[%d]", nodePlayers->iUserID,
				nodePlayers->cFanResult[LIAN_QI_DUI], nodePlayers->cFanResult[LONG_QI_DUI], nodePlayers->cFanResult[QI_DUI]);
			return 0;
		}
	}

	//判断当前玩家处于摸牌阶段
	if (tableItem->iCurMoPlayer >= 0 && nodePlayers->cTableNumExtra == tableItem->iCurMoPlayer)
	{
		//判断暗杠
		int iAllCard[5][10] = {0};
		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
		{
			int iType = nodePlayers->cHandCards[i] >> 4;
			int iValue = nodePlayers->cHandCards[i] & 0x000f;

			iAllCard[iType][0] ++;                  //手中万条饼类型的牌各有多少张
			iAllCard[iType][iValue] ++;				//手中相同的牌有多少张
		}

		//如果有四张一样的牌 那么暗杠
		bool bAnGang = false;
		for (int i = 0; i < 5; i++)
		{
			if (i == nodePlayers->iQueType)
				continue;
			for (int j = 1; j < 10; j++)
			{
				if (iAllCard[i][j] == 4 && nodePlayers->iHandCardsNum >= 5)
				{
					if (nodePlayers->bIsHu == false)
					{
						bAnGang = true;
						iGangCard[iGameNum] = i * 16 + j;
						iGangType[iGameNum] = 12;
						iGameNum++;
					}
					else
					{
						if ((i * 16 + j) != tableItem->cCurMoCard)
							continue;
						//首先从玩家手中移除这三张牌
						vector<char> vcCards;
						char cCard = tableItem->cCurMoCard;
						int iTempAllCards[5][10] = { 0 };
						for (int k = 0; k < nodePlayers->iHandCardsNum; k++)
						{
							if (nodePlayers->cHandCards[k] != cCard)
							{
								vcCards.push_back(nodePlayers->cHandCards[k]);
								int iType = nodePlayers->cHandCards[k] >> 4;
								int iValue = nodePlayers->cHandCards[k] & 0x000f;
								iTempAllCards[iType][0]++;
								iTempAllCards[iType][iValue]++;
							}
						}
						//判断有没有换听
						vector<char> vcTemp;
						for (int k = 0; k < 3; k++)
						{
							if (k == nodePlayers->iQueType)
								continue;
							for (int l = 1; l < 10; l++)
							{
								vcTemp.push_back((k << 4) | l);
							}
						}
						vcTemp.push_back((4 << 4) | 1);
						_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialGang Judge if Change Ting cCard=== %d", cCard);
						vector<char> vcTingCard;

						for (int k = 0; k < vcTemp.size(); k++)
						{
							ResultTypeDef tempResult = nodePlayers->resultType;
							tempResult.pengType[tempResult.iPengNum].bOther = true;
							tempResult.pengType[tempResult.iPengNum].iGangType = 1;
							tempResult.pengType[tempResult.iPengNum].iType = cCard >> 4;
							tempResult.pengType[tempResult.iPengNum].iValue = cCard & 0x000f;
							tempResult.iPengNum++;

							int iType = vcTemp[k] >> 4;
							int iValue = vcTemp[k] & 0x000f;

							int iTempCards[5][10] = { 0 };
							memcpy(iTempCards, iTempAllCards, sizeof(iTempCards));

							iTempCards[iType][0] ++;
							iTempCards[iType][iValue] ++;

							tempResult.lastCard.iType = iType;
							tempResult.lastCard.iValue = iValue;
							tempResult.lastCard.bOther = false;

							int iSpecialFlag = 0;
							char cFanResult[MJ_FAN_TYPE_CNT] = { 0 };
							memset(cFanResult, 0, sizeof(cFanResult));
							int iTotalFan = 0;
							iTotalFan = JudgeResult::JudgeHZXLMJHu(iTempCards, tempResult, nodePlayers->iQueType, cFanResult);	// 暗杠后，判断是否换听

							if (iTotalFan > 0)
							{
								vcTingCard.push_back(vcTemp[k]);
							}
						}

						//判断 胡牌的牌没有换的话 那就可以杠
						_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialGang_ AnGang vcTingCardSize[%d], NodeSize[%d]", vcTingCard.size(), nodePlayers->vcTingCard.size());
						if (vcTingCard.size() == nodePlayers->vcTingCard.size())
						{
							int iNum = 0;
							for (int k = 0; k < vcTingCard.size(); k++)
							{
								if (vcTingCard[k] == nodePlayers->vcTingCard[k])
								{
									iNum++;
								}
							}
							if (iNum == vcTingCard.size())
							{
								bAnGang = true;
								iGangCard[iGameNum] = i * 16 + j;
								iGangType[iGameNum] = 12;
								iGameNum++;
							}
						}
					}
				}
			}
		}

		//判断玩家碰的牌里面有没有手里的牌 有的话补杠 
		bool bBuGang = false;
		for (int i = 0; i < nodePlayers->CPGInfo.iAllNum; i++)
		{
			if (nodePlayers->CPGInfo.Info[i].iCPGType == 1 && nodePlayers->CPGInfo.Info[i].cGangType != 1)
			{
				//如果 没有听牌 那么手里的牌都可以补杠
				//if (nodePlayers->resultType.pengType == false)
				{
					char cCard = nodePlayers->CPGInfo.Info[i].cChiValue;  //cChiValue  吃碰杠的牌

					for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
					{
						if (nodePlayers->cHandCards[j] == cCard)
						{
							if (nodePlayers->bIsHu == true)
							{
								if (cCard != tableItem->cCurMoCard)
									continue;
							}
							bBuGang = true;

							iGangCard[iGameNum] = cCard;
							iGangType[iGameNum] = 8;
							iGameNum++;
						}
					}
				}

			}
		}

		if (bAnGang)
		{
			return 3;
		}
		if (bBuGang)
		{
			return 2;
		}
		return 0;
	}

	//判断当前别人处于出牌阶段	
	if (tableItem->iCurSendPlayer >= 0 && (tableItem->iCurSendPlayer != nodePlayers->cTableNumExtra)) //|| bGuaDaFeng
	{
		//听牌之后 要判断杠完牌之后能不能换听

		char cCard = tableItem->cTableCard;

		if (nodePlayers->iQueType == (cCard >> 4))
		{
			return 0;
		}

		int iNum = 0;
		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
		{
			if (nodePlayers->cHandCards[i] == cCard)
			{
				iNum++;
			}
		}

		if (iNum >= 3)
		{

			if (nodePlayers->bIsHu)
			{
				//首先从玩家手中移除这三张牌
				vector<char> vcCards;
				int iAllCards[5][10] = { 0 };

				for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
				{
					if (nodePlayers->cHandCards[i] != cCard)
					{
						vcCards.push_back(nodePlayers->cHandCards[i]);
						int iType = nodePlayers->cHandCards[i] >> 4;
						int iValue = nodePlayers->cHandCards[i] & 0x000f;
						iAllCards[iType][0]++;
						iAllCards[iType][iValue]++;
					}
				}
				//判断有没有换听
				vector<char > vcTemp;
				for (int i = 0; i < 3; i++)
				{
					if (i == nodePlayers->iQueType)
						continue;
					for (int j = 1; j < 10; j++)
					{
						vcTemp.push_back((i << 4) | j);
					}
				}
				vcTemp.push_back((4 << 4) | 1);
				vector<char> vcTingCard;

				_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialGang_ MingGang JudgeHZXLMJHu_Begin UID[%d]", nodePlayers->iUserID);
				for (int i = 0; i < vcTemp.size(); i++)
				{
					ResultTypeDef tempResult = nodePlayers->resultType;
					tempResult.pengType[tempResult.iPengNum].bOther = true;
					tempResult.pengType[tempResult.iPengNum].iGangType = 1;
					tempResult.pengType[tempResult.iPengNum].iType = cCard >> 4;
					tempResult.pengType[tempResult.iPengNum].iValue = cCard & 0x000f;
					tempResult.iPengNum++;

					int iType = vcTemp[i] >> 4;
					int iValue = vcTemp[i] & 0x000f;

					int iTempCards[5][10] = { 0 };
					memcpy(iTempCards, iAllCards, sizeof(iAllCards));

					iTempCards[iType][0]++;
					iTempCards[iType][iValue]++;

					tempResult.lastCard.iType = iType;
					tempResult.lastCard.iValue = iValue;
					tempResult.lastCard.bOther = true;

					int iSpecialFlag = 0;
					char cFanResult[MJ_FAN_TYPE_CNT] = { 0 };
					memset(cFanResult, 0, sizeof(cFanResult));
					int iTotalFan = 0;
					iTotalFan = JudgeResult::JudgeHZXLMJHu(iTempCards, tempResult, nodePlayers->iQueType, cFanResult);	// 明杠后，判断是否换听
					_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialGang_ MingGang JudgeHZXLMJHu_End vcTemp[%d] = %d, iTotalFan[%d]", i, vcTemp[i], iTotalFan);
					if (iTotalFan > 0)
					{
						vcTingCard.push_back(vcTemp[i]);
					}
				}

				if (vcTingCard.size() == nodePlayers->vcTingCard.size())
				{
					int iNum = 0;
					for (int i = 0; i < vcTingCard.size(); i++)
					{
						if (vcTingCard[i] == nodePlayers->vcTingCard[i])
						{
							iNum++;
						}
					}

					if (iNum == vcTingCard.size())
					{
						iGangCard[iGameNum] = cCard;  //杠的什么牌
						iGangType[iGameNum] = 4;      //什么杠  明杠 暗杠 补杠
						iGameNum++;                   //杠了几个
						//_log(_DEBUG, "HZXLGameLogic", " R[%d] T[%d] JudgeSpecialGang  Gang", nodePlayers->iRoomID, tableItem->iTableID);
						return 1;
					}
				}
				return 0;
			}
			else
			{
				iGangCard[iGameNum] = cCard;  //杠的什么牌
				iGangType[iGameNum] = 4;      //什么杠  明杠 暗杠 补杠
				iGameNum++;                   //杠了几个
				//_log(_DEBUG, "HZXLGameLogic", " R[%d] T[%d] JudgeSpecialGang  Gang", nodePlayers->cRoomNum, tableItem->iTableID);
				return 1;
			}
		}
	}
	return 0;
}

// vcCards:玩家手牌
int HZXLGameLogic::JudgeSpecialTing(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, ResultTypeDef resultType, vector<char> vcCards, map<char, vector<char> > &mpTingCard, map<char, map<char, int> > &mpTingFan)//, map<char, vector<char> > &mpKaDangCard
{
	int iQueNum = 0;
	for (int i = 0; i < vcCards.size(); i++)
	{
		int iType = vcCards[i]>>4;
		if (iType == nodePlayers->iQueType)
		{
			iQueNum++;
			if(iQueNum > 1)
				return 0;
		}
	}
	int iAllCard[5][10] = { 0 };
	ResultTypeDef tempResultOne;

	_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialTing_Log() iQueNum[%d], UserID[%d], vcCards.size()[%d]", iQueNum, nodePlayers->iUserID, vcCards.size());

	bool bTing = false;
	vector<char > vcTemp;          //麻将中所有的牌1-9万条饼
	for (int i = 0; i < 3; i++)
	{
		if (i == nodePlayers->iQueType)
			continue;
		for (int j = 1; j < 10; j++)
		{
			vcTemp.push_back((i << 4) | j);
		}
	}
	vcTemp.push_back(4 << 4 | 1);		// 添加红中牌

	//从当前手牌中去掉任何一张牌 然后补出来牌可以胡牌的情况下 算听牌
	for (int i = 0; i < vcCards.size(); i++)
	{
		map<char, vector<char> >::iterator pos = mpTingCard.begin();
		bool bExist = false;
		while (pos != mpTingCard.end())
		{
			if (pos->first == vcCards[i])				// 判断这张牌是否计算过听牌，如果计算过，则跳过
			{
				bExist = true;
				break;
			}
			pos++;
		}
		_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialTing_Log() vcCards[%d] = %d, bExist[%d]", i, vcCards[i], bExist);
		if (bExist)
			continue;

		int iTempAllCard[5][10] = { 0 };
		int iHandCardNum = 0;
		bool bExistHZCard = false;
		bool bExistOneHZCard = false;
		for (int j = 0; j < vcCards.size(); j++)			// 遍历所有手牌
		{
			if (j == i || vcCards[j] <= 0)										// 跳过当前要计算听的牌
			{
				continue;
			}

			int iType = vcCards[j]>>4;
			int iValue = vcCards[j] & 0x000f;

			iTempAllCard[iType][0]++;               //单类牌的数量
			iTempAllCard[iType][iValue]++;          //单张牌的数量
			iHandCardNum++;
			if (vcCards[j] == 65)
				bExistHZCard = true;
		}
		if (bExistHZCard && iHandCardNum == 1)			// 除去要听的牌之后，只剩一张红中牌
			bExistOneHZCard = true;

		for (int j = 0 ; j < vcTemp.size() ; j++)
		{
			memset(iAllCard, 0, sizeof(iAllCard));

			for (int m = 0; m < 5; m++)
			{
				for (int n = 0; n < 10; n++)
				{
					iAllCard[m][n] = iTempAllCard[m][n];
				}
			}

			int iType = vcTemp[j] >> 4;
			int iValue = vcTemp[j] & 0x000f;

			iAllCard[iType][0]++;
			iAllCard[iType][iValue]++;

			memset(&tempResultOne, 0, sizeof(ResultTypeDef));
			memcpy(&tempResultOne, &resultType, sizeof(ResultTypeDef));

			tempResultOne.lastCard.iType = iType;
			tempResultOne.lastCard.iValue = iValue;
			tempResultOne.lastCard.bOther = true;

			char cFanResult[MJ_FAN_TYPE_CNT] = { 0 };
			memset(cFanResult, 0, sizeof(cFanResult));
			int iTotalFan = 0;

			//_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialTing_Log() vcTemp[%d] = %d, bExistOneHZCard[%d]", j, vcTemp[j], bExistOneHZCard);
			if (bExistOneHZCard && vcTemp[j] == 65)
			{
				iTotalFan = JudgeResult::JudgeSZBYHu(iAllCard, tempResultOne, nodePlayers->iQueType, cFanResult);
			}
			else
			{
				iTotalFan = JudgeResult::JudgeHZXLMJHu(iAllCard, tempResultOne, nodePlayers->iQueType, cFanResult);		// 听牌时，判断能否胡某张牌
			}
			if (iTotalFan > 0)
			{
				//mpTingCard 表示去掉手中某张牌后听什么牌
				if (find(mpTingCard[vcCards[i]].begin(), mpTingCard[vcCards[i]].end(), vcTemp[j]) == mpTingCard[vcCards[i]].end())
					mpTingCard[vcCards[i]].push_back(vcTemp[j]);

				map<char, int>::iterator iter = mpTingFan[vcCards[i]].find(vcTemp[j]);
				if (iter == mpTingFan[vcCards[i]].end())
				{
					mpTingFan[vcCards[i]][vcTemp[j]] = iTotalFan;
				}

				if (nodePlayers->iTingHuFan < iTotalFan)
				{
					nodePlayers->iTingHuFan = iTotalFan;
				}
				bTing = true;
			}
		}
	}

	_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialTing_ End iUserID[%d], bTing[%d]", nodePlayers->iUserID, bTing);

	if (bTing)
	{
		return 1;
	}

	return 0;
}

// 胡牌判断
/***
	@return cFanResult:胡的番数
***/
int HZXLGameLogic::JudgeSpecialHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char *cFanResult, char cQiangGangCard /*= 0*/)
{
	char cFanFinal[MJ_FAN_TYPE_CNT];
	memset(cFanFinal, 0, sizeof(cFanFinal));
	int iResultHuFan = 0;

	bool bLastOther = false;		// 是否是胡其他玩家的牌
	_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialHu_Log() UID[%d],cExtra[%d],iCurMoPlayer[%d],cQiangGangCard[%d]",
		nodePlayers->iUserID, nodePlayers->cTableNumExtra, tableItem->iCurMoPlayer, cQiangGangCard);

	// 获取胡的那张牌
	char cLastCard = 0;
	if (tableItem->iCurMoPlayer == nodePlayers->cTableNumExtra)
		cLastCard = tableItem->cCurMoCard;
	else
	{
		if (cQiangGangCard > 0)
			cLastCard = cQiangGangCard;
		else
			cLastCard = tableItem->cTableCard;

		bLastOther = true;
	}
	int iLastType = cLastCard >> 4;
	int iLastValue = cLastCard & 0x000f;

	int iAllCard[5][10] = { 0 };
	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		int iType = nodePlayers->cHandCards[i] >> 4;
		int iValue = nodePlayers->cHandCards[i] & 0x000f;
		//手上还有定缺的牌 直接返回 不能胡
		if (iType == nodePlayers->iQueType)
		{
			_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialHu_Log() UID[%d],iQueType[%d],iType[%d]",
				nodePlayers->iUserID, nodePlayers->iQueType, iType);
			return 0;
		}
		iAllCard[iType][0]++;
		iAllCard[iType][iValue]++;
	}

	// 如果是点炮胡，加上点炮的那张牌
	if (tableItem->iCurMoPlayer != nodePlayers->cTableNumExtra)
	{
		iAllCard[iLastType][0]++;
		iAllCard[iLastType][iLastValue]++;
	}

	ResultTypeDef tempResultOne = nodePlayers->resultType;
	tempResultOne.lastCard.iType = iLastType;
	tempResultOne.lastCard.iValue = iLastValue;
	tempResultOne.lastCard.bOther = bLastOther;

	_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialHu_Log() cLastCard[%d], bLastGang[%d], iHandCardsNum[%d]", cLastCard, nodePlayers->bLastGang, nodePlayers->iHandCardsNum);
	if ((nodePlayers->iHandCardsNum == 1 || nodePlayers->iHandCardsNum == 2 && bLastOther == false) && nodePlayers->cHandCards[0] == 65)		// 手中只剩一张红中，只能胡红中
	{
		if (cLastCard == 65)
		{
			tempResultOne.jiangType.iType = 4;
			tempResultOne.jiangType.iValue = 1;

			iResultHuFan = JudgeResult::JudgeSZBYHu(iAllCard, tempResultOne, nodePlayers->iQueType, cFanFinal);
		}
	}
	else
	{
		iResultHuFan = JudgeResult::JudgeHZXLMJHu(iAllCard, tempResultOne, nodePlayers->iQueType, cFanFinal);			// 胡牌时，判断能否胡
	}
	_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialHu_Log()iResultHuFan[%d]", iResultHuFan);

	// 再计算特殊的胡番
	if (iResultHuFan > 0)
	{
		if (nodePlayers->bLastGang)			// 判断杠上开花
		{
			if (cFanFinal[MJ_GANG_SHANG_KAI_HUA] != -1)
				cFanFinal[MJ_GANG_SHANG_KAI_HUA] = 4;
		}

		// 自摸
		if (tableItem->iCurMoPlayer == nodePlayers->cTableNumExtra)
		{
			_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialHu_Log() iCardIndex[%d] cTableNumExtra[%d] iZhuangPos=%d ", tableItem->iCardIndex, nodePlayers->cTableNumExtra, tableItem->iZhuangPos);
			if (tableItem->iCardIndex == 53 && nodePlayers->cTableNumExtra == tableItem->iZhuangPos)//自摸并且摸的牌是第53张牌 天胡
			{
				if (cFanFinal[MJ_TIAN_HU] != -1)
					cFanFinal[MJ_TIAN_HU] = 48;
			}
			else if (tableItem->iCardIndex == HZXL_MAX_CARD_NUM)	// 海底捞月
			{
				if (cFanFinal[HAI_DI_LAO_YUE] != -1)
					cFanFinal[HAI_DI_LAO_YUE] = 2;
			}

			if (nodePlayers->cTableNumExtra != tableItem->iZhuangPos)		// 判断地胡
			{
				int j;
				for (j = 0; j < PLAYER_NUM; j++)
				{
					if (tableItem->pPlayers[j] == NULL)
					{
						continue;
					}
					if (tableItem->pPlayers[j]->resultType.iPengNum > 0)
						break;
				}
				if (j == PLAYER_NUM && tableItem->iCardIndex >= 53 && tableItem->iCardIndex <= 56 && tableItem->pPlayers[tableItem->iZhuangPos]->iAllSendCardsNum <= 1 && tableItem->iHasHuNum == 0)
				{
					if (cFanFinal[MJ_DI_HU] != -1)
						cFanFinal[MJ_DI_HU] = 24;
				}
			}
		}
		// 点炮
		else
		{
			_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialHu_Log() DianPao cQiangGangCard[%d], iWinFlag[%d]", cQiangGangCard, nodePlayers->iWinFlag);
			if (tableItem->iCardIndex == HZXL_MAX_CARD_NUM)		// 妙手回春
			{
				if (cFanFinal[MIAO_SHOU_HUI_CHUN] != -1)
					cFanFinal[MIAO_SHOU_HUI_CHUN] = 2;
			}

			// 抢杠胡
			if (cQiangGangCard > 0)
			{
				if (cFanFinal[MJ_QIANG_GANG_HU] != -1)
					cFanFinal[MJ_QIANG_GANG_HU] = 2;
			}

			// 杠上炮
			if (tableItem->iCurSendPlayer >= 0)
			{
				if (tableItem->pPlayers[tableItem->iCurSendPlayer]->bLastGang == true)
				{
					nodePlayers->iWinFlag |= 1 << WIN_HU_JIAO_ZHUAN_YI;
					if (cFanFinal[MJ_GANG_SHANG_PAO] != -1)
						cFanFinal[MJ_GANG_SHANG_PAO] = 2;
				}
			}
		}
	}

	if (cFanFinal[MJ_TIAN_HU] == 48)
	{
		memset(cFanFinal, 0, sizeof(cFanFinal));
		cFanFinal[MJ_TIAN_HU] = 48;
	}
	else if (cFanFinal[MJ_DI_HU] == 24)
	{
		memset(cFanFinal, 0, sizeof(cFanFinal));
		cFanFinal[MJ_DI_HU] = 24;
	}

	// 统计所有番型
	int iTotalFan = 1;
	bool bExistFan = false;
	for (int i = 0; i < MJ_FAN_TYPE_CNT; i++)
	{
		if (cFanFinal[i] > 0)
		{
			iTotalFan *= cFanFinal[i];
			bExistFan = true;
			//_log(_DEBUG, "HZXLGameLogic", "JudgeSpecialHu_Log() cFanFinal[%d] = %d", i, cFanFinal[i]);
		}
	}
	if (!bExistFan)
		iTotalFan = 0;
	else
	{
		if (iTotalFan > MAX_WIN_FAN || iTotalFan < 0)
			iTotalFan = MAX_WIN_FAN;
	}

	memcpy(cFanResult, cFanFinal, sizeof(cFanFinal));

	return iTotalFan;
}

bool cmp(vector<int> a, vector<int> b)
{
	if(a[1] != b[1]) return a[1] < b[1];
}

bool cmpChangeCard(vector<char> a, vector<char> b)
{
	if (a[1] != b[1]) return a[1] < b[1];
}

// 游戏结束
void HZXLGameLogic::GameEndHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, bool bLiuJu, int iSpecialFlag)
{
	if (tableItem == NULL)
	{
		_log(_ERROR, "XLMJGameLogic", "GameEndHu_Log() Error TableItem = NULL");
		return;
	}

	GameResultServerDef pMsgResult;
	memset(&pMsgResult, 0, sizeof(GameResultServerDef));

	_log(_DEBUG, "HZXLMJGameLogic", "GameEndHu_log() iUserID[%d]", nodePlayers->iUserID);
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		pMsgResult.cHuNumExtra[i] = -1;
		pMsgResult.cPoChan[i] = 0;
	}
	pMsgResult.iZhuangPos = tableItem->iZhuangPos;
	int iIndex = tableItem->vcOrderHu.size();
	pMsgResult.iHuNum = tableItem->vcOrderHu.size();
	_log(_DEBUG, "HZXLMJGameLogic", "GameEndHu_log() 本局胡牌人数iHuNum[%d]", pMsgResult.iHuNum);
	for (int i = 0; i < tableItem->vcOrderHu.size(); i++)
	{
		pMsgResult.cHuOrder[i] = tableItem->vcOrderHu.at(i);
	}
	for (int i = 0; i< 4; i++)
	{
		int j = 0;
		for (; j < tableItem->vcOrderHu.size(); j++)
		{
			if (i == tableItem->vcOrderHu.at(j))
				break;
		}
		if (j == tableItem->vcOrderHu.size())
		{
			pMsgResult.cHuOrder[iIndex] = i;
			iIndex++;
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			memcpy(pMsgResult.cHandCards[i], tableItem->pPlayers[i]->cHandCards, sizeof(tableItem->pPlayers[i]->cHandCards));
			memcpy(&pMsgResult.cpgInfo[i], &tableItem->pPlayers[i]->CPGInfo, sizeof(tableItem->pPlayers[i]->CPGInfo));
			if (tableItem->pPlayers[i]->bBankruptRecharge)
				pMsgResult.cIfTing[i] = 1;

			if (tableItem->pPlayers[i]->iSpecialFlag >> 5)
				pMsgResult.cHuNumExtra[i] = i;
			pMsgResult.cHuCount[i] = tableItem->pPlayers[i]->vcHuCard.size();
		}
	}

	// 游戏结束，计算本局总输赢分数
	int viResultMoney[4][4] = { 0 };
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			for (int j = 0; j < PLAYER_NUM; j++)									// i视角下，j玩家本局总输赢分
			{
				int iTempMoney = 0;
				if (j == i)
					iTempMoney = tableItem->pPlayers[i]->iCurWinMoney;
				else
					iTempMoney = tableItem->pPlayers[i]->iAllUserCurWinMoney[j];
				viResultMoney[i][j] = iTempMoney;
			}
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		for (int j = 0; j < PLAYER_NUM; j++)									// i视角下，j玩家本局总输赢分
		{
			_log(_DEBUG, "HZXLMJGameLogic", "GameEndHu_log() viResultMoney[%d][%d] = %d", i, j, viResultMoney[i][j]);
		}
	}

	int iGameAmount[PLAYER_NUM] = { 0 };//净分，不包含抽水
	int iRateMoney[PLAYER_NUM] = { 0 };//抽水和桌费
	int iPunishMoney[PLAYER_NUM] = { 0 };//掉线惩罚，注意必须是正值

	GameUserAccountReqDef msgRD[PLAYER_NUM];						//ACCOUNT_REQ_RADIUS_MSG	向Radius发送计分更改请求
	memset(msgRD, 0, sizeof(msgRD));

	char cHuaZhu[4] = { 0 };
	memset(cHuaZhu, 0, sizeof(cHuaZhu));
	LiuJuSpecialInfoDef liujuSpecialInfoDef;
	memset(&liujuSpecialInfoDef, 0, sizeof(liujuSpecialInfoDef));
	if (bLiuJu)
	{
		if (tableItem->iHasHuNum >= 3)
		{
			_log(_ERROR, "HZXLMJGameLogic", "GameEndHu_log() Error tableItem->iHasHuNum ");
			return;
		}

		pMsgResult.cLiuJu = 1;

		SpecialCardsNoticeDef pSpecialNotice;
		memset(&pSpecialNotice, 0, sizeof(SpecialCardsNoticeDef));
		pSpecialNotice.iSpecialFlag = 1 << 5;
		pSpecialNotice.cTableNumExtra = -1;
		pSpecialNotice.cTagNumExtra = -1;
		pSpecialNotice.cLiuJuHu = 1;
		if(tableItem->iHasHuNum > 0)
			pSpecialNotice.cLiuJuHu = 0;

		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i])
			{
				tableItem->pPlayers[i]->iKickOutTime = 0;
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pSpecialNotice, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef));		// 流局胡
			}
		}

		HandleGameEndHuLiuJu(tableItem, pMsgResult, cHuaZhu, liujuSpecialInfoDef);
	}

	int viResultFinalMoney[4][4] = { 0 };			// 玩家本局输赢分，不包括桌费
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			for (int j = 0; j < PLAYER_NUM; j++)									// i视角下，j玩家本局总输赢分
			{
				int iTempMoney = 0;
				if (j == i)
					iTempMoney = tableItem->pPlayers[i]->iCurWinMoney;
				else
					iTempMoney = tableItem->pPlayers[i]->iAllUserCurWinMoney[j];
				viResultFinalMoney[i][j] = iTempMoney;
			}
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		for (int j = 0; j < PLAYER_NUM; j++)									// i视角下，j玩家本局总输赢分
		{
			_log(_DEBUG, "HZXLMJGameLogic", "GameEndHu_log() viResultFinalMoney[%d][%d] = %d", i, j, viResultFinalMoney[i][j]);
		}
	}

	// 输赢统计
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			_log(_DEBUG, "HZXLMJGameLogic", "GameEndHu_log  tableItem->pPlayers[%d]->iCurWinMoney = %d", i, tableItem->pPlayers[i]->iCurWinMoney);
			if (tableItem->pPlayers[i]->iCurWinMoney > 0)
				tableItem->pPlayers[i]->iGameResultForTask = 1;
			else if (tableItem->pPlayers[i]->iCurWinMoney == 0)
				tableItem->pPlayers[i]->iGameResultForTask = 3;
			else if (tableItem->pPlayers[i]->iCurWinMoney < 0)
				tableItem->pPlayers[i]->iGameResultForTask = 2;
		}
	}

	// 计费要放在流局之后，加入退税花猪查大叫的分
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->cPoChan == 0)
		{
			//AI计费不用记录到数据库
			if (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType !=  NORMAL_PLAYER)
			{
				continue;
			}
			msgRD[i].iUserID = htonl(tableItem->pPlayers[i]->iUserID);
			msgRD[i].iGameID = htonl(m_pServerBaseInfo->iGameID);
			GameUserAccountGameDef msgGameRD;
			memset(&msgRD, 0, sizeof(GameUserAccountGameDef));
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			msgGameRD.llTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;
			SetAccountReqRadiusDef(&msgRD[i], &msgGameRD,tableItem, tableItem->pPlayers[i], tableItem->pPlayers[i]->iCurWinMoney, iPunishMoney[i], iRateMoney[i]);	// 游戏结束，计费
			memcpy(&tableItem->pPlayers[i]->msgRDTemp, &msgRD[i], sizeof(GameUserAccountReqDef));
		}
	}
	//将当前对局结果存到gcgameinfo中
	int iAmountMoney[PLAYER_NUM];
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType != NORMAL_PLAYER)
		{
			continue;
		}
		iAmountMoney[i] = tableItem->pPlayers[i]->iCurWinMoney - iPunishMoney[i] - iRateMoney[i];
	}

	int viTransScore[4][4] = { 0 };
	for (int i = 0; i<PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			if(tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType!=  NORMAL_PLAYER)
			{ 
				continue;
			}
			memcpy(viTransScore[i], viResultFinalMoney[i], sizeof(viTransScore[i]));
			bool bTransSuccess = TransResultScore(i, viResultFinalMoney[i], viTransScore[i]);

			if (!tableItem->pPlayers[i]->bTempMidway)
			{
				RDSendAccountReq(&tableItem->pPlayers[i]->msgRDTemp,NULL);
			}
			_log(_DEBUG, "HZXLMJGameLogic", "GameEndHu_log  tableItem->pPlayers[%d]->iUSerID = %d, bTransSuccess[%d]", i, tableItem->pPlayers[i]->iUserID, bTransSuccess);
		}
	}

	pMsgResult.cEnd = 1;
	// 结算消息发送至客户端
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		tableItem->pPlayers[i]->iKickOutTime = 0;
		int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
		long long iBaseScore = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;				// 基础底分
		long long  iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费
		int iFengDingFan = (int)stSysConfBaseInfo.stRoom[iPlayerRoomID].iMinPoint;			// 封顶番
		memset(pMsgResult.iMoneyResult, 0, sizeof(pMsgResult.iMoneyResult));
		memset(pMsgResult.iTuiShui, 0, sizeof(pMsgResult.iTuiShui));
		memset(pMsgResult.cTuiShui, 0, sizeof(pMsgResult.cTuiShui));
		memset(pMsgResult.iHuaZhu, 0, sizeof(pMsgResult.iHuaZhu));
		memset(pMsgResult.cHuaZhu, 0, sizeof(pMsgResult.cHuaZhu));
		memset(pMsgResult.iDaJiao, 0, sizeof(pMsgResult.iDaJiao));
		memset(pMsgResult.iDaJiaoFan, 0, sizeof(pMsgResult.iDaJiaoFan));
		memcpy(pMsgResult.iMoneyResult, viResultFinalMoney[i], sizeof(viResultFinalMoney[i]));

		if (liujuSpecialInfoDef.cTuiShui == 1)
		{
			memcpy(pMsgResult.iTuiShui, liujuSpecialInfoDef.viTuiShuiMoney[i], sizeof(liujuSpecialInfoDef.viTuiShuiMoney[i]));
			memcpy(pMsgResult.cTuiShui, liujuSpecialInfoDef.vcTuiShuiFan[i], sizeof(liujuSpecialInfoDef.vcTuiShuiFan[i]));
		}
		if (liujuSpecialInfoDef.cHuaZhu == 1)
		{
			memcpy(pMsgResult.iHuaZhu, liujuSpecialInfoDef.viHuaZhuMoney[i], sizeof(liujuSpecialInfoDef.viHuaZhuMoney[i]));
			memcpy(pMsgResult.cHuaZhu, liujuSpecialInfoDef.vcHuaZhuFan[i], sizeof(liujuSpecialInfoDef.vcHuaZhuFan[i]));
		}
		if (liujuSpecialInfoDef.cChaDaJiao == 1)
		{
			memcpy(pMsgResult.iDaJiao, liujuSpecialInfoDef.viChaDaJiaoMoney[i], sizeof(liujuSpecialInfoDef.viChaDaJiaoMoney[i]));
			memcpy(pMsgResult.iDaJiaoFan, liujuSpecialInfoDef.viChaDaJiaoFan[i], sizeof(liujuSpecialInfoDef.viChaDaJiaoFan[i]));
		}

		pMsgResult.iTableMoney = iCurTableMoney;
		pMsgResult.iMaxFan = iFengDingFan;

		_log(_DEBUG, "HZXLMJGameLogic", "GameEndHu_log HZXLMJ_GAME_RESULT_SERVER i = %d", i);
		CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsgResult, HZXLMJ_GAME_RESULT_SERVER, sizeof(GameResultServerDef));
	}

	int iAwardNum[PLAYER_NUM] = { 0 };

	// 日志服务器添加参数（待补充）
	
	//重置为等待开始状态
	int iNormalPlayerPos = -1;
	for (int i = 0; i<PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER))
			{
				iNormalPlayerPos = i;
				tableItem->pPlayers[i]->iStatus = PS_WAIT_READY;
				tableItem->pPlayers[i]->Reset();
				_log(_DEBUG, "XLMJGameLogic", "GameEndHu_log(): i[%d]:-------cHandCards[%d].", i, sizeof(tableItem->pPlayers[i]->cHandCards));
			}
		}
	}
	for (int i = 0;i<m_iMaxPlayerNum;i++)
	{
		if (tableItem->pFactoryPlayers[i] != NULL)
		{
			tableItem->pFactoryPlayers[i]->iStatus = PS_WAIT_READY;//桌上其他玩家状态置为等待开始
		}
	}
	//删除AI节点
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i]->iPlayerType != -1)
		{
			_log(_DEBUG, "XLMJGameLogic", "GameEndHu_log(): tableItem->pPlayers[%d]->iPlayerType[%d]", i, tableItem->pPlayers[i]->iPlayerType);
			ReleaseAIPlayerNode(tableItem, tableItem->pPlayers[i]);
		}
	}
	GameLogOnePlayerLogDef gameExtraLog[PLAYER_NUM];
	memset(gameExtraLog,0,sizeof(gameExtraLog));//这里待赋值
	if (tableItem->bIfExistAIPlayer && iNormalPlayerPos > -1)
	{
		SendGameLogInfo(tableItem, gameExtraLog, 1);
		_log(_DEBUG, "XLMJGameLogic", "GameEndHu_log(): UserID[%d] AI 对局游戏记录上传",tableItem->pPlayers[iNormalPlayerPos]->iUserID);
	}
	else
	{
		SendGameLogInfo(tableItem, gameExtraLog, PLAYER_NUM);
		_log(_DEBUG, "XLMJGameLogic", "GameEndHu_log(): 非AI 对局游戏记录上传");
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		_log(_DEBUG, "XLMJGameLogic", "GameEndHu_Log() cHuOrder[%d] = %d, cHuNumExtra[%d] = %d", i, pMsgResult.cHuOrder[i], i, pMsgResult.cHuNumExtra[i]);
		_log(_DEBUG, "XLMJGameLogic", "GameEndHu_Log() pMsgResult.cpgInfo[%d].iAllNum = %d", i, pMsgResult.cpgInfo[i].iAllNum);
	}
	
	//把掉线的人强制逃跑
	EscapeReqDef msgReq;
	memset(&msgReq, 0, sizeof(EscapeReqDef));
	for (int i = 0; i<PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			if (tableItem->pPlayers[i]->bIfWaitLoginTime == true || tableItem->pPlayers[i]->cDisconnectType == 1)
			{
				if (tableItem->pPlayers[i]->bTempMidway)  //断定是临时节点
				{
					_log(_DEBUG, "HZXLMJGameLogic", "GameEnd room[%d] T[%d] 玩家 userID = %d szNickName[%s] 座位[%d] 游戏中途离开 删除临时节点", tableItem->pPlayers[i]->cRoomNum, tableItem->iTableID, tableItem->pPlayers[i]->iUserID, tableItem->pPlayers[i]->szNickName, i);

					// 注意由于调用HandleNormalEscapeReq后底层代码已经将玩家的桌节点移除了，因此需要在调用HandleNormalEscapeReq之前先备份一下临时节点，之后再删除临时节点
					MJGPlayerNodeDef *pNode = tableItem->pPlayers[i];

					HandleNormalEscapeReq(tableItem->pPlayers[i], &msgReq);
					//删除临时节点
					//delete pNode;
					//pNode = NULL;
					ReleaseTmpNode(pNode);
				}
				else
				{
					HandleNormalEscapeReq(tableItem->pPlayers[i], &msgReq);
				}
			}
			else
			{
				_log(_DEBUG, "XLMJGameLogic", "GameEndHu_log():  bIfExistAIPlayer[%d]", tableItem->bIfExistAIPlayer);
				if (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
				{
					//模拟离桌
					msgReq.cType = 50;  //为了游戏结束让客户端离开，不占座位，客户端不处理此类型
					HandleNormalEscapeReq(tableItem->pPlayers[i], &msgReq);
				}
				else
				{
					_log(_DEBUG, "XLMJGameLogic", "GameEndHu_log():  玩家 userID = %d szNickName[%s] 座位[%d]:----------玩家离开.", tableItem->pPlayers[i]->iUserID, tableItem->pPlayers[i]->szNickName, i);
					memset(&msgReq, 0, sizeof(EscapeReqDef));
					msgReq.cType = 99;
					HandleNormalEscapeReq(tableItem->pPlayers[i], &msgReq);
				}
			}
		}
	}

	ResetTableState(tableItem);
}

// 游戏结束，处理流局相关
void HZXLGameLogic::HandleGameEndHuLiuJu(MJGTableItemDef *tableItem, GameResultServerDef &pMsgResult, char *cHuaZhu, LiuJuSpecialInfoDef &liujuSpecialInfoDef)
{
	//退税:流局时若玩家未听牌，则需返还刮风下雨所得的分数
	char vcPoChan[4][4];
	long long viTuiShuiMoney[4][4][4] = { 0 };			// 四个玩家视角下玩家的退税输赢分
	char vcTuiShuiPlayerSeat[4][4][4];				// 四个玩家视角下，玩家i视角下,玩家j要给玩家k退税的倍数
	memset(vcPoChan, 0, sizeof(vcPoChan));
	memset(vcTuiShuiPlayerSeat, 0, sizeof(vcTuiShuiPlayerSeat));
	bool bTuiShui = false;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iBaseGangFen = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint; // 基础底分
			for (int t = 0; t < PLAYER_NUM; t++)										// 玩家i视角下，看看有没有玩家要退税的
			{
				if (tableItem->pPlayers[t]->bTing == false && tableItem->pPlayers[t]->bIsHu == false && tableItem->pPlayers[t]->iMoney > iBaseGangFen)		// 玩家t未听牌，需要退税
				{
					long long iTempMoney = 0;
					if (t == i)
						iTempMoney = tableItem->pPlayers[t]->iMoney + tableItem->pPlayers[i]->iCurWinMoney;
					else
						iTempMoney = tableItem->pPlayers[i]->otherMoney[t].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[t];							// 玩家i视角下，玩家t所携带的金币

					_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu TuiShui R[%d]T[%d]UID[%d], iTempMoney[%lld]", iPlayerRoomID, tableItem->iTableID, tableItem->pPlayers[i]->iUserID, iTempMoney);
					for (int j = 0; j < tableItem->pPlayers[t]->CPGInfo.iAllNum; j++)						// 玩家i视角下，遍历玩家t所有杠的牌，计算玩家t退税
					{
						if (tableItem->pPlayers[t]->CPGInfo.Info[j].iCPGType == 2 && iTempMoney > 0)		// 如果是杠牌，需要退税
						{
							_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu TuiShui R[%d]T[%d]UID[%d]i[%d]t[%d], cGangType[%d]",
								iPlayerRoomID, tableItem->iTableID, tableItem->pPlayers[i]->iUserID, i, t, tableItem->pPlayers[t]->CPGInfo.Info[j].cGangType);
							bTuiShui = true;
							//明杠 扣除2倍底分 
							if (tableItem->pPlayers[t]->CPGInfo.Info[j].cGangType == 0)
							{
								int iGangTag = tableItem->pPlayers[t]->CPGInfo.Info[j].cGangTag;			// 放杠玩家
								if (tableItem->pPlayers[iGangTag] == NULL || tableItem->pPlayers[iGangTag]->cPoChan == 1)
									continue;
								long long iMoneyResult = (iBaseGangFen * 2);
								if (iMoneyResult > iTempMoney)
								{
									iMoneyResult = iTempMoney;
									vcPoChan[i][t] = 1;								// 玩家i视角下，玩家t因退税破产
								}

								viTuiShuiMoney[i][t][t] -= iMoneyResult;			// 玩家i视角下，玩家t需要退税给iGangTag，即玩家i的视角下，玩家iGangTag赢的分
								viTuiShuiMoney[i][t][iGangTag] += iMoneyResult;		// 玩家i视角下，玩家iGangTag需要补税
								vcTuiShuiPlayerSeat[i][t][iGangTag] += 2;
							}
							//补杠 扣除乘每个未胡玩家1倍底分  暗杠两倍
							else if (tableItem->pPlayers[t]->CPGInfo.Info[j].cGangType > 0)
							{
								int iGangTag[4] = { 0 };
								int iGangNum = 0;								// 需要退补杠的玩家数量
								for (int k = 0; k < PLAYER_NUM; k++)			// 获得退补杠玩家座位
								{
									if (tableItem->pPlayers[t]->CPGInfo.Info[j].cGangTag & (1 << k))
									{
										if (tableItem->pPlayers[k] == NULL || tableItem->pPlayers[k]->cPoChan == 1)
											continue;
										iGangTag[k] = 1;
										iGangNum++;
									}
								}
								if (iGangNum == 0)
									continue;

								long long iMoneyResult = iBaseGangFen * iGangNum;
								if (tableItem->pPlayers[t]->CPGInfo.Info[j].cGangType == 2)		// 暗杠
									iMoneyResult = iMoneyResult * 2;

								if (iMoneyResult > iTempMoney)
								{
									iMoneyResult = iTempMoney;
									vcPoChan[i][t] = 1;
								}

								int iTempGangFen = iMoneyResult / iGangNum;
								for (int k = 0; k < PLAYER_NUM; k++)				// 被扣补杠的玩家，需要补税
								{
									if (iGangTag[k] == 1)
									{
										viTuiShuiMoney[i][t][t] -= iTempGangFen;
										viTuiShuiMoney[i][t][k] += iTempGangFen;
										vcTuiShuiPlayerSeat[i][t][k] += 1;
										if (tableItem->pPlayers[t]->CPGInfo.Info[j].cGangType == 2)
											vcTuiShuiPlayerSeat[i][t][k] += 1;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//查花猪 手上还有三种花色的玩家要陪给其他有缺一门玩家8倍		 
	int iHuaZhuNum = 0;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->bTing == false && tableItem->pPlayers[i]->cPoChan == 0 && tableItem->pPlayers[i]->bIsHu == false)
		{
			for (int j = 0; j< tableItem->pPlayers[i]->iHandCardsNum; j++)
			{
				if (tableItem->pPlayers[i]->cHandCards[j] >> 4 == tableItem->pPlayers[i]->iQueType)
				{
					_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu HuaZhu UID[%d], i[%d], t[%d]", tableItem->pPlayers[i]->iUserID, i, j);
					cHuaZhu[i] = 1;
					iHuaZhuNum++;
					break;
				}
			}
		}
	}
	_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu_log() iHuaZhuNum = %d", iHuaZhuNum);

	char vcHuaZhuPoChan[4][4];
	char vcHuaZhuFan[4][4][4];
	long long viHuaZhuMoney[4][4][4] = { 0 };
	memset(vcHuaZhuPoChan, 0, sizeof(vcHuaZhuPoChan));
	memset(vcHuaZhuFan, 0, sizeof(vcHuaZhuFan));
	bool bHuaZhu = false;
	if (iHuaZhuNum > 0 && iHuaZhuNum < 4)
	{
		bHuaZhu = HandleLiuJuHuaZhu(tableItem, cHuaZhu, iHuaZhuNum, vcHuaZhuPoChan, viHuaZhuMoney, vcHuaZhuFan);
	}

	// 查大叫 未听牌玩家要赔给听牌未胡牌玩家最大可能的番数（不包含自摸番数）
	char vcChaDaJiaoPoChan[4][4];
	long long  viChaDaJiaoMoney[4][4][4] = { 0 };
	int viChaDaJiaoFan[4][4][4] = { 0 };
	bool bChaDaJiao = false;
	memset(vcChaDaJiaoPoChan, 0, sizeof(vcChaDaJiaoPoChan));

	bChaDaJiao = HandleLiuJuChaDaJiao(tableItem, cHuaZhu, iHuaZhuNum, vcChaDaJiaoPoChan, viChaDaJiaoMoney, viChaDaJiaoFan);

	// 记录破产信息
	int viFinalPoChan[4][4] = { 0 };
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (vcPoChan[i][j] > 0 || vcHuaZhuPoChan[i][j] > 0 || vcChaDaJiaoPoChan[i][j] > 0)
				liujuSpecialInfoDef.vcPoChan[i][j] = 1;
		}
	}

	_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu HuaZhu bTuiShui[%d], bHuaZhu[%d], bChaDaJiao[%d]", bTuiShui, bHuaZhu, bChaDaJiao);

	// 记录退税
	if (bTuiShui)
	{
		liujuSpecialInfoDef.cTuiShui = 1;
		memcpy(liujuSpecialInfoDef.viTuiShuiMoney, viTuiShuiMoney, sizeof(viTuiShuiMoney));
		memcpy(&liujuSpecialInfoDef.vcTuiShuiFan, vcTuiShuiPlayerSeat, sizeof(vcTuiShuiPlayerSeat));
	}

	// 记录花猪
	if (bHuaZhu)
	{
		liujuSpecialInfoDef.cHuaZhu = 1;
		memcpy(liujuSpecialInfoDef.viHuaZhuMoney, viHuaZhuMoney, sizeof(viHuaZhuMoney));
		memcpy(&liujuSpecialInfoDef.vcHuaZhuFan, vcHuaZhuFan, sizeof(vcHuaZhuFan));
	}

	// 记录查大叫
	if (bChaDaJiao)
	{
		liujuSpecialInfoDef.cChaDaJiao = 1;
		memcpy(liujuSpecialInfoDef.viChaDaJiaoMoney, viChaDaJiaoMoney, sizeof(viChaDaJiaoMoney));
		memcpy(&liujuSpecialInfoDef.viChaDaJiaoFan, viChaDaJiaoFan, sizeof(viChaDaJiaoFan));
	}

	//for (int i = 0; i < PLAYER_NUM; i++)			// 玩家i视角下
	//{
	//	for (int j = 0; j < PLAYER_NUM; j++)		// 玩家i视角下，玩家j的输赢分
	//	{
	//		for (int k = 0; k < PLAYER_NUM; k++)
	//		{
	//			if (viTuiShuiMoney[i][j][k] != 0 || viHuaZhuMoney[i][j][k] != 0 || viChaDaJiaoMoney[i][j][k] != 0)
	//			{
	//				_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu viTuiShuiMoney[%d][%d][%d]=%d,viHuaZhuMoney[%d][%d][%d]=%d,viChaDaJiaoMoney[%d][%d][%d]=%d",
	//					i, j, k, viTuiShuiMoney[i][j][k], i, j, k, viHuaZhuMoney[i][j][k], i, j, k, viChaDaJiaoMoney[i][j][k]);
	//			}
	//		}
	//	}
	//}

	//for (int i = 0; i < PLAYER_NUM; i++)			// 玩家i视角下
	//{
	//	for (int j = 0; j < PLAYER_NUM; j++)		// 玩家i视角下，玩家j的输赢分
	//	{
	//		for (int k = 0; k < PLAYER_NUM; k++)
	//		{
	//			if (liujuSpecialInfoDef.vcHuaZhuFan[i][j][k] != 0)
	//			{
	//				_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu liujuSpecialInfoDef.vcHuaZhuFan[%d][%d][%d]=%d",
	//					i, j, k, liujuSpecialInfoDef.vcHuaZhuFan[i][j][k]);
	//			}
	//		}
	//	}
	//}

	//for (int i = 0; i < PLAYER_NUM; i++)			// 玩家i视角下
	//{
	//	for (int j = 0; j < PLAYER_NUM; j++)		// 玩家i视角下，玩家j的输赢分
	//	{
	//		for (int k = 0; k < PLAYER_NUM; k++)
	//		{
	//			if (vcTuiShuiPlayerSeat[i][j][k] != 0 || vcHuaZhuFan[i][j][k] != 0 || viChaDaJiaoFan[i][j][k] != 0)
	//			{
	//				_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu vcTuiShuiPlayerSeat[%d][%d][%d]=%d,vcHuaZhuFan[%d][%d][%d]=%d,viChaDaJiaoFan[%d][%d][%d]=%d",
	//					i, j, k, vcTuiShuiPlayerSeat[i][j][k], i, j, k, vcHuaZhuFan[i][j][k], i, j, k, viChaDaJiaoFan[i][j][k]);
	//			}
	//		}
	//	}
	//}

	//for (int i = 0; i < PLAYER_NUM; i++)			// 玩家i视角下
	//{
	//	for (int j = 0; j < PLAYER_NUM; j++)		// 玩家i视角下，玩家j的输赢分
	//	{
	//		for (int k = 0; k < PLAYER_NUM; k++)
	//		{
	//			if (liujuSpecialInfoDef.viTuiShuiMoney[i][j][k] != 0)
	//			{
	//				_log(_DEBUG, "HZXLMJGameLogic", "HandleGameEndHuLiuJu_log  viTuiShuiMoney[%d][%d][%d]=%d", i, j, k, viTuiShuiMoney[i][j][k]);
	//			}
	//		}
	//	}
	//}

	// 计费
	for (int i = 0; i < PLAYER_NUM; i++)			// 玩家i视角下
	{
		for (int j = 0; j < PLAYER_NUM; j++)		// 玩家i视角下，玩家j的输赢分
		{
			for (int k = 0; k < PLAYER_NUM; k++)	// 玩家i视角下，玩家j要给玩家k的分
			{
				if (i == k)
				{
					tableItem->pPlayers[i]->iCurWinMoney += viTuiShuiMoney[i][j][k];
					tableItem->pPlayers[i]->iCurWinMoney += viHuaZhuMoney[i][j][k];
					tableItem->pPlayers[i]->iCurWinMoney += viChaDaJiaoMoney[i][j][k];
				}
				else
				{
					tableItem->pPlayers[i]->iAllUserCurWinMoney[k] += viTuiShuiMoney[i][j][k];
					tableItem->pPlayers[i]->iAllUserCurWinMoney[k] += viHuaZhuMoney[i][j][k];
					tableItem->pPlayers[i]->iAllUserCurWinMoney[k] += viChaDaJiaoMoney[i][j][k];
				}
			}

			for (int k = 0; k < PLAYER_NUM; k++)	// 玩家i视角下，玩家j要给玩家k的分
			{
				liujuSpecialInfoDef.viTotalScore[i][k] += viTuiShuiMoney[i][j][k];
				liujuSpecialInfoDef.viTotalScore[i][k] += viHuaZhuMoney[i][j][k];
				liujuSpecialInfoDef.viTotalScore[i][k] += viChaDaJiaoMoney[i][j][k];
			}
		}
	}
	//// 计费
	//for (int i = 0; i < PLAYER_NUM; i++)			// 玩家i视角下
	//{
	//	for (int j = 0; j < PLAYER_NUM; j++)		// 玩家i视角下，玩家j的输赢分
	//	{
	//		if (i == j)
	//		{
	//			for (int k = 0; k < PLAYER_NUM; k++)
	//			{
	//				tableItem->pPlayers[i]->iCurWinMoney += viTuiShuiMoney[i][j][k];
	//				tableItem->pPlayers[i]->iCurWinMoney += viHuaZhuMoney[i][j][k];
	//				tableItem->pPlayers[i]->iCurWinMoney += viChaDaJiaoMoney[i][j][k];
	//			}
	//		}
	//		else
	//		{
	//			for (int k = 0; k < PLAYER_NUM; k++)	// 玩家i视角下，玩家j要给玩家k的分
	//			{
	//				tableItem->pPlayers[i]->iAllUserCurWinMoney[k] += viTuiShuiMoney[i][j][k];
	//				tableItem->pPlayers[i]->iAllUserCurWinMoney[k] += viHuaZhuMoney[i][j][k];
	//				tableItem->pPlayers[i]->iAllUserCurWinMoney[k] += viChaDaJiaoMoney[i][j][k];
	//			}
	//		}
	//		for (int k = 0; k < PLAYER_NUM; k++)	// 玩家i视角下，玩家j要给玩家k的分
	//		{
	//			liujuSpecialInfoDef.viTotalScore[i][k] += viTuiShuiMoney[i][j][k];
	//			liujuSpecialInfoDef.viTotalScore[i][k] += viHuaZhuMoney[i][j][k];
	//			liujuSpecialInfoDef.viTotalScore[i][k] += viChaDaJiaoMoney[i][j][k];
	//		}
	//	}
	//}
}

// 处理流局中的花猪计算
bool HZXLGameLogic::HandleLiuJuHuaZhu(MJGTableItemDef *tableItem, char *cHuaZhu, const int iHuaZhuNum, char vcHuaZhuPoChan[4][4], long long viHuaZhuMoney[4][4][4], char vcHuaZhuFan[4][4][4])
{
	long long  viResultMoney[4][4][4] = { 0 };			// 玩家i视角下，玩家j要给玩家k的花猪分
	char vcResultHuaZhuFan[4][4][4];			// 玩家i视角下，玩家j赢得玩家k的花猪番
	char vcPoChan[4][4];					// 玩家i视角下，玩家j是否破产
	bool bHuaZhu = false;
	memset(vcResultHuaZhuFan, 0, sizeof(vcResultHuaZhuFan));
	memset(vcPoChan, 0, sizeof(vcPoChan));

	int iHuaZhu[PLAYER_NUM] = { 0 };
	int iOnlineCount = 0;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->cPoChan == 0)	//  && tableItem->pPlayers[i]->bIsHu == false
			iOnlineCount++;
	}
	_log(_DEBUG, "HZXLMJGameLogic", "HandleLiuJuHuaZhu Begin iOnlineCount = %d, iHuaZhuNum = %d", iOnlineCount, iHuaZhuNum);

	// 计算每个玩家的花猪分数
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
		long long iBaseDiFen = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;				// 基础底分
		for (int j = 0; j < PLAYER_NUM; j++)										// 玩家i视角下，寻找花猪玩家
		{
			if (tableItem->pPlayers[j] && cHuaZhu[j] == 1)							// 玩家j是花猪
			{
				bHuaZhu = true;
				long long  iTempMoney = 0;
				if (j == i)
					iTempMoney = tableItem->pPlayers[j]->iMoney + tableItem->pPlayers[i]->iCurWinMoney;
				else
					iTempMoney = tableItem->pPlayers[i]->otherMoney[j].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[j];				// 玩家i视角下，玩家j所携带的金币

				_log(_DEBUG, "HZXLMJGameLogic", "HandleLiuJuHuaZhu i[%d], j[%d], iTempMoney = %lld", i, j, iTempMoney);
				// 如果花猪玩家钱不够当前查花猪玩家倍数乘8 直接平分
				if (iTempMoney < (iBaseDiFen * (iOnlineCount - iHuaZhuNum) * 8))
				{
					vcPoChan[i][j] = 1;
					long long iMoney = iTempMoney / (iOnlineCount - iHuaZhuNum);
					for (int k = 0; k < PLAYER_NUM; k++)
					{
						if (tableItem->pPlayers[k] == NULL || cHuaZhu[k] == 1 || tableItem->pPlayers[k]->cPoChan == 1)	//  || tableItem->pPlayers[k]->bIsHu == true
							continue;

						viResultMoney[i][j][k] += iMoney;		// 玩家i视角下，玩家j要给玩家k的花猪分
						vcResultHuaZhuFan[i][j][k] = 8;
						viResultMoney[i][j][j] -= iMoney;
						_log(_DEBUG, "HZXLMJGameLogic", "HandleLiuJuHuaZhu PingFen vcResultHuaZhuFan[%d][%d][%d] = %lld", i, j, k, vcResultHuaZhuFan[i][j][k]);
					}
				}
				// 花猪玩家的钱够8倍
				else
				{
					int iTempFan[4] = { 0 };	// 每个玩家番数
					int iTempAllFan = 0;		// 所有查花猪玩家总番
					for (int k = 0; k < PLAYER_NUM; k++)
					{
						if (cHuaZhu[k] == 1 || tableItem->pPlayers[k] == NULL || tableItem->pPlayers[k]->cPoChan == 1)	//  || tableItem->pPlayers[k]->bIsHu == true
							continue;

						iTempFan[k] = 8;
						iTempAllFan += 8;
						if (tableItem->pPlayers[k]->bTing)
						{
							int iTempTingFan = tableItem->pPlayers[k]->iTingHuFan;
							if (iTempTingFan > 8)		// 有玩家听牌大于8倍，则按16倍赔付
							{
								iTempFan[k] = 16;
								iTempAllFan += 8;
							}
						}
						_log(_DEBUG, "HZXLMJGameLogic", "HandleLiuJuHuaZhu Enough i[%d]j[%d], iTempFan[%d]  = %d", i, j, k, iTempFan[k]);
					}

					//如果 花猪玩家不够查花猪玩家乘16倍 并且玩家都是16倍 平分
					if ((iOnlineCount - iHuaZhuNum > 0 && iTempMoney < (iBaseDiFen * (iOnlineCount - iHuaZhuNum) * 16) && iTempAllFan == 16 * (iOnlineCount - iHuaZhuNum)))
					{
						long long iMoney = iTempMoney / (iOnlineCount - iHuaZhuNum);
						for (int k = 0; k < PLAYER_NUM; k++)
						{
							if (tableItem->pPlayers[k] == NULL || cHuaZhu[k] == 1 || tableItem->pPlayers[k]->cPoChan == 1)	//  || tableItem->pPlayers[k]->bIsHu == true
								continue;
							viResultMoney[i][j][k] += iMoney;	// 玩家i视角下，玩家j要给玩家k的花猪分
							vcResultHuaZhuFan[i][j][k] = 16;
							viResultMoney[i][j][j] -= iMoney;
						}
						vcPoChan[i][j] = 1;
					}
					//如果 花猪玩家不够查花猪玩家倍数 并且玩家有一个16倍 先把8倍给完 剩余给16
					else if ((iBaseDiFen * iTempAllFan) > iTempMoney && iTempAllFan == ((iOnlineCount - iHuaZhuNum) * 8 + 8))
					{
						int iTempMax = -1;
						for (int k = 0; k < PLAYER_NUM; k++)
						{
							if (tableItem->pPlayers[k] == NULL || cHuaZhu[k] == 1 || tableItem->pPlayers[k]->cPoChan == 1)	//  || tableItem->pPlayers[k]->bIsHu == true
								continue;
							if (iTempFan[k] == 16)
							{
								iTempMax = k;
								continue;
							}
							long long  iMoney = iTempFan[k] * iBaseDiFen;
							viResultMoney[i][j][k] += iMoney;	// 玩家i视角下，玩家j要给玩家k的花猪分
							vcResultHuaZhuFan[i][j][k] = 8;
							viResultMoney[i][j][j] -= iMoney;
						}
						if (iTempMax != -1)
						{
							long long iMoney = iTempMoney - ((iTempAllFan - 16)* iBaseDiFen);
							vcPoChan[i][j] = 1;
							viResultMoney[i][j][iTempMax] += iMoney;	// 玩家i视角下，玩家j要给玩家iTempMax的花猪分
							vcResultHuaZhuFan[i][j][iTempMax] = 16;
							viResultMoney[i][j][j] -= iMoney;
						}
					}
					//如果 花猪玩家不够查花猪玩家倍数 查花猪玩家有3个 并且玩家有2个16倍 先把8倍给完 剩余给16平分
					else if ((iBaseDiFen * iTempAllFan) > iTempMoney && iTempAllFan == ((iOnlineCount - iHuaZhuNum) * 8 + 16) && iOnlineCount - iHuaZhuNum == 3)
					{
						int k = 0;
						for (k = 0; k < PLAYER_NUM; k++)
						{
							if (cHuaZhu[k] == 1 || tableItem->pPlayers[k] == NULL || tableItem->pPlayers[k]->cPoChan == 1)	//  || tableItem->pPlayers[k]->bIsHu == true
								continue;
							if (iTempFan[k] == 8)
								break;
						}
						long long iMinMoney = 0;
						if (k < 4 && tableItem->pPlayers[k])
						{
							iMinMoney = iTempFan[k] * iBaseDiFen;
							viResultMoney[i][j][k] += iMinMoney;
							vcResultHuaZhuFan[i][j][k] = 8;
							viResultMoney[i][j][j] -= iMinMoney;
						}

						for (k = 0; k < PLAYER_NUM; k++)
						{
							if (cHuaZhu[k] == 1 || tableItem->pPlayers[k] == NULL || tableItem->pPlayers[k]->cPoChan == 1)	//  || tableItem->pPlayers[k]->bIsHu == true
								continue;
							if (iTempFan[k] == 8)
								continue;
							if (tableItem->pPlayers[k])
							{
								long long iMoney = (iTempMoney - iMinMoney) / 2;
								vcPoChan[i][j] = 1;
								viResultMoney[i][j][k] += iMinMoney;
								vcResultHuaZhuFan[i][j][k] = 8;
								viResultMoney[i][j][j] -= iMinMoney;
							}
						}
					}
					//如果上述条件都不达成 就都够 一家家给
					else
					{
						for (int k = 0; k < PLAYER_NUM; k++)
						{
							if (tableItem->pPlayers[k] == NULL || cHuaZhu[k] == 1)	//  || tableItem->pPlayers[k]->bIsHu == true
								continue;
							long long iMoney = iTempFan[k] * iBaseDiFen;
							viResultMoney[i][j][k] += iMoney;
							vcResultHuaZhuFan[i][j][k] = iTempFan[k];
							viResultMoney[i][j][j] -= iMoney;
						}
					}
				}
			}
		}
	}

	//for (int i = 0; i < PLAYER_NUM; i++)			// 玩家i视角下
	//{
	//	for (int j = 0; j < PLAYER_NUM; j++)		// 玩家i视角下，玩家j的输赢分
	//	{
	//		for (int k = 0; k < PLAYER_NUM; k++)
	//		{
	//			if (vcResultHuaZhuFan[i][j][k] != 0)
	//			{
	//				_log(_DEBUG, "HZXLMJGameLogic", "HandleLiuJuHuaZhu vcResultHuaZhuFan[%d][%d][%d] = %d", i, j, k, vcResultHuaZhuFan[i][j][k]);
	//			}
	//		}
	//	}
	//}

	memcpy(vcHuaZhuPoChan, vcPoChan, sizeof(vcPoChan));
	memcpy(viHuaZhuMoney, viResultMoney, sizeof(viResultMoney));
	memcpy(vcHuaZhuFan, vcResultHuaZhuFan, sizeof(vcResultHuaZhuFan));

	return bHuaZhu;
}

// 查大叫 未听牌玩家要赔给听牌未胡牌玩家最大可能的番数（不包含自摸番数）
bool HZXLGameLogic::HandleLiuJuChaDaJiao(MJGTableItemDef *tableItem, char *cHuaZhu, const int iHuaZhuNum, char vcChaDaJiaoPoChan[4][4], long long viChaDaJiaoMoney[4][4][4], int viChaDaJiaoFan[4][4][4])
{
	long long viChaDaJiao[4][4][4] = { 0 };			// 玩家i视角下,玩家j要给玩家k的大叫分
	int viChaDaJiaoFanResult[4][4][4] = {0};	// 玩家i视角下，玩家j要赢玩家k的大叫番
	char vcPoChan[4][4];					// 玩家i视角下，玩家j是否因为查大叫破产
	bool bChaDaJiao = false;
	memset(vcPoChan, 0, sizeof(vcPoChan));

	int iDaJiaoNum = 0;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			if (tableItem->pPlayers[i]->bIsHu == false && tableItem->pPlayers[i]->bTing && tableItem->pPlayers[i]->cPoChan == 0)		// 听牌未胡玩家
			{
				iDaJiaoNum++;
			}
		}
	}

	if (iDaJiaoNum <= 0 || iDaJiaoNum >= 4)
		return false;
	else
		bChaDaJiao = true;

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])		// 玩家i视角下的查大叫
		{
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iBaseScore = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;				// 基础底分
			int iFengDingFan = (int)stSysConfBaseInfo.stRoom[iPlayerRoomID].iMinPoint;			// 封顶番
			long long iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费
			long long iAllMoney = 0;			// 总计要查大叫赔付的分数

			int viTempFan[4] = { 0 };
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				viTempFan[j] = tableItem->pPlayers[j]->iTingHuFan;
				if (tableItem->pPlayers[j]->bIsHu == false && tableItem->pPlayers[j]->bTing && tableItem->pPlayers[j]->cPoChan == 0)		// 听牌未胡玩家
				{
					if (viTempFan[j] >= iFengDingFan && iFengDingFan > 0)
						viTempFan[j] = iFengDingFan;
					long long iTempMoney = viTempFan[j] * iBaseScore;
					iAllMoney += iTempMoney;
				}
			}

			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (tableItem->pPlayers[j]->bTing == false && cHuaZhu[j] == 0 && tableItem->pPlayers[j]->cPoChan == 0 && tableItem->pPlayers[j]->bIsHu == false)		// 未听牌玩家，需要给听牌未胡玩家查大叫
				{
					long long iTempMoney = 0;			// 被查大叫玩家的金币
					if (j == i)
						iTempMoney = tableItem->pPlayers[i]->iMoney + tableItem->pPlayers[i]->iCurWinMoney;
					else
						iTempMoney = tableItem->pPlayers[i]->otherMoney[j].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[j];

					_log(_DEBUG, "HZXLMJGameLogic", "HandleLiuJuChaDaJiao() i[%d], j[%d], iTempMoney[%lld], iAllMoney[%lld], iDaJiaoNum[%d]", i, j, iTempMoney, iAllMoney, iDaJiaoNum);
					_log(_DEBUG, "HZXLMJGameLogic", "HandleLiuJuChaDaJiao() iMoney[%lld], iCurWinMoney[%lld]", tableItem->pPlayers[i]->iMoney, tableItem->pPlayers[i]->iCurWinMoney);

					iTempMoney = iTempMoney - iCurTableMoney;

					// 金币够赔付
					if (iTempMoney >= iAllMoney)
					{
						for (int k = 0; k < PLAYER_NUM; k++)
						{
							if (viTempFan[k] == 0)
								continue;

							if (tableItem->pPlayers[k]->bIsHu == false && tableItem->pPlayers[k]->bTing && tableItem->pPlayers[k]->cPoChan == 0)		// 听牌未胡玩家
							{
								viChaDaJiao[i][j][k] += viTempFan[k] * iBaseScore;			// 赢的查大叫分数
								viChaDaJiaoFanResult[i][j][k] += viTempFan[k];
								viChaDaJiao[i][j][j] -= viTempFan[k] * iBaseScore;			// 输的查大叫分数
							}
						}
					}
					else
					{
						//如果只有一个人听牌未胡， 被查大叫玩家不够金额， 将身上的钱全部输给查大叫玩家(不包括桌费)
						if (iDaJiaoNum == 1)
						{
							for (int k = 0; k < PLAYER_NUM; k++)
							{
								if (viTempFan[k] == 0)
									continue;

								if (tableItem->pPlayers[k]->bIsHu == false && tableItem->pPlayers[k]->bTing && tableItem->pPlayers[k]->cPoChan == 0)		// 听牌未胡玩家
								{
									viChaDaJiao[i][j][k] += iTempMoney;			// 赢的查大叫分数
									viChaDaJiaoFanResult[i][j][k] += viTempFan[k];
									viChaDaJiao[i][j][j] -= iTempMoney;			// 输的查大叫分数
								}
							}
						}
						else
						{
							int iMinFan = 999999;
							for (int k = 0; k < PLAYER_NUM; k++)
							{
								if (viTempFan[k] != 0 && viTempFan[k] < iMinFan)
									iMinFan = viTempFan[k];
							}

							if (iMinFan >= iFengDingFan && iFengDingFan > 0)
								iMinFan = iFengDingFan;
							long long iTempMinMoney = iMinFan * iBaseScore;
							long long iTempAllMinMoney = iTempMinMoney * iDaJiaoNum;

							// 如果被查大叫玩家的金币，小于最小金额的查大叫人数倍数的金额 平分
							if (iTempMoney <= iTempAllMinMoney)
							{
								for (int k = 0; k < PLAYER_NUM; k++)
								{
									if (tableItem->pPlayers[k] == NULL || viTempFan[k] == 0)
										continue;
									long long iMoney = iTempMoney / iDaJiaoNum;
									viChaDaJiao[i][j][k] += iMoney;			// 赢的查大叫分数
									viChaDaJiaoFanResult[i][j][k] += viTempFan[k];
									viChaDaJiao[i][j][j] -= iMoney;			// 输的查大叫分数
								}
							}
							else
							{
								int iTempMinNum = 0;
								long long iPayedMoney = 0;
								//大于最小金额 先把最小的给了
								for (int k = 0; k < PLAYER_NUM; k++)
								{
									if (tableItem->pPlayers[k] && viTempFan[k] == iMinFan)
									{
										long long iMoney = iMinFan * iBaseScore;
										viChaDaJiao[i][j][k] += iMoney;			// 赢的查大叫分数
										viChaDaJiaoFanResult[i][j][k] += viTempFan[k];
										viChaDaJiao[i][j][j] -= iMoney;			// 输的查大叫分数
										iTempMinNum++;
										iPayedMoney += iMoney;
									}
								}
								//剩余一个大番直接给了
								if (iDaJiaoNum - iTempMinNum == 1)
								{
									for (int k = 0; k < PLAYER_NUM; k++)
									{
										if (tableItem->pPlayers[k] && iMinFan < viTempFan[k])
										{
											long long iMoney = iTempMoney - iPayedMoney;
											viChaDaJiao[i][j][k] += iMoney;			// 赢的查大叫分数
											viChaDaJiaoFanResult[i][j][k] += viTempFan[k];
											viChaDaJiao[i][j][j] -= iMoney;			// 输的查大叫分数
											break;
										}
									}
								}
								//剩余两个判断 其中最小的两倍够不够 不够就平分 否则就 先给小的剩下的给大的
								else if (iDaJiaoNum - iTempMinNum == 2)
								{
									int iTempMinFan = 999999;
									//判断找到第二个小的番
									for (int k = 0; k < PLAYER_NUM; k++)
									{
										if (tableItem->pPlayers[j] && viTempFan[k] > iMinFan)
										{
											if (iTempMinFan > viTempFan[k])
												iTempMinFan = viTempFan[k];
										}
									}
									if (iTempMinFan >= iFengDingFan && iFengDingFan > 0)
										iTempMinFan = iFengDingFan;
									iPayedMoney = iTempMinFan * iBaseScore;
									iPayedMoney = iPayedMoney * 2;
									//小的都不够就平分
									if (iTempMoney <= iPayedMoney)
									{
										for (int k = 0; k < PLAYER_NUM; k++)
										{
											if (tableItem->pPlayers[k] == NULL || viTempFan[k] == 0)
												continue;
											long long iMoney = iTempMoney / iDaJiaoNum;
											viChaDaJiao[i][j][k] += iMoney;			// 赢的查大叫分数
											viChaDaJiaoFanResult[i][j][k] += viTempFan[k];
											viChaDaJiao[i][j][j] -= iMoney;			// 输的查大叫分数
										}
									}
									//够两份小的 先给小的剩下的给大的
									else
									{
										for (int k = 0; k < PLAYER_NUM; k++)
										{
											if (tableItem->pPlayers[k] && viTempFan[k] == iTempMinFan)
											{
												if (iTempMinFan >= iFengDingFan && iFengDingFan > 0)
													iTempMinFan = iFengDingFan;
												long long iMoney = viTempFan[k] * iBaseScore;
												iPayedMoney += iMoney;
												viChaDaJiao[i][j][k] += iMoney;			// 赢的查大叫分数
												viChaDaJiaoFanResult[i][j][k] += viTempFan[k];
												viChaDaJiao[i][j][j] -= iMoney;			// 输的查大叫分数
												break;
											}
										}
										// 剩下的金币都给大的
										for (int k = 0; k < PLAYER_NUM; k++)
										{
											if (tableItem->pPlayers[k] && iTempMinFan < viTempFan[k])
											{
												long long iMoney = iTempMoney - iPayedMoney;
												viChaDaJiao[i][j][k] += iMoney;			// 赢的查大叫分数
												viChaDaJiaoFanResult[i][j][k] += viTempFan[k];
												viChaDaJiao[i][j][j] -= iMoney;			// 输的查大叫分数

												break;
											}
										}
									}
								}
							}
						}
						vcPoChan[i][j] = 1;		// 被查大叫的玩家破产
					}
				}
			}
		}
	}

	memcpy(vcChaDaJiaoPoChan, vcPoChan, sizeof(vcPoChan));
	memcpy(viChaDaJiaoMoney, viChaDaJiao, sizeof(viChaDaJiao));
	memcpy(viChaDaJiaoFan, viChaDaJiaoFanResult, sizeof(viChaDaJiaoFanResult));

	return bChaDaJiao;
}

// 处理玩家定缺
void HZXLGameLogic::HandleConfirmQueReq(int iUserID, void *pMsgData)
{
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLGameLogic", "●●●● HandleConfirmQueReq: nodePlayers is NULL ●●●●");
		return;
	}

	if(nodePlayers->iStatus != PS_WAIT_CONFIRM_QUE)
	{
		_log(_ERROR, "HZXLGameLogic", "●●●● HandleConfirmQueReq: nodePlayers 不是定缺状态 ●●●●");
		SendKickOutWithMsg(nodePlayers, "不是定缺状态", 1301, 1);
		return;
	}

	int  iTableNum = nodePlayers->cTableNum;                   //找出所在的桌
	char iTableNumExtra = nodePlayers->cTableNumExtra;
	char iRoomNum = nodePlayers->cRoomNum;

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleConfirmQueReq:[%d][%d]", iTableNum, iTableNumExtra);
		return;
	}

	MJGTableItemDef* tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	if(nodePlayers->bConfirmQueReq)
	{
		_log(_ERROR, "XLMJGameLogic", "HandleConfirmQueReq szNickName = [%s]  玩家已经请求过定缺了",nodePlayers->szNickName);
		return;
	}

	ConfirmQueReqDef *pMsg = (ConfirmQueReqDef*)pMsgData;
	int iQueType = pMsg->iQueType;
	if(iQueType == -1)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleConfirmQueReq R[%d] T[%d], userID = %d  iQueType == -1", iRoomNum, iTableNum, nodePlayers->iUserID);
		AutoSendCards(nodePlayers->iUserID);
		return;
	}
	//nodePlayers->iQueType = iQueType;
	if (iQueType >= 0 && iQueType < 3)
	{
		nodePlayers->iQueType = iQueType;
		_log(_DEBUG, "HZXLGameLogic", "HandleConfirmQueReq R[%d]T[%d]UID[%d],QueType[%d] \n", iRoomNum, iTableNum, nodePlayers->iUserID, iQueType);

		int iHasConfirmQueCount=0;
		for(int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->iQueType >= 0)
			{
				iHasConfirmQueCount++;
			}
		}

		ConfirmQueNoticeDef msgNotice;
		memset(&msgNotice, 0, sizeof(ConfirmQueNoticeDef));
		msgNotice.iQueType = iQueType;
		msgNotice.iCurCompleteQueCnt = iHasConfirmQueCount;
		msgNotice.cTableNumExtra = iTableNumExtra;
		for (int i = 0; i<PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i])
			{
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotice, CONFIRM_QUE_NOTICE_MSG, sizeof(ConfirmQueNoticeDef));
			}
		}
		nodePlayers->bConfirmQueReq = true;
		nodePlayers->iKickOutTime = 0;

		//AI对局时，此时发送AI开始定缺消息
		if (tableItem->bIfExistAIPlayer)
		{
			for (int i = 0; i < PLAYER_NUM; i++)
			{
				if (tableItem->pPlayers[i]->iPlayerType !=  NORMAL_PLAYER)
				{
					m_pHzxlAICTL->CreateAIConfirmQueReq(tableItem, tableItem->pPlayers[i]);
				}
			}
		}
		_log(_DEBUG, "HZXLGameLogic", " HandleConfirmQueReq R[%d], T[%d],  iHasConfirmQueCount = %d", iRoomNum, iTableNum, iHasConfirmQueCount);
		if (iHasConfirmQueCount == 4)
		{
			for (int i = 0; i < PLAYER_NUM; i++)
			{
				if (tableItem->pPlayers[i])
				{
					tableItem->pPlayers[i]->iStatus = PS_WAIT_SEND;
				}
			}

			//给玩家发牌
			if (tableItem->pPlayers[tableItem->iZhuangPos])
			{
				tableItem->iCurMoPlayer = tableItem->iZhuangPos;
				SendCardToPlayer(tableItem->pPlayers[tableItem->iZhuangPos], tableItem, false, true);//1.这里要判断能不能天胡
			}
		}
	}
}

//处理玩家出牌
void HZXLGameLogic::HandleSendCard(int iUserID, void *pMsgData)
{
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));

	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLGameLogic", "●●●● HandleSendCard: nodePlayers is NULL ●●●●");
		return;
	}

	int  iTableNum = nodePlayers->cTableNum;                   //找出所在的桌
	char iTableNumExtra = nodePlayers->cTableNumExtra;
	char iRoomNum = nodePlayers->cRoomNum;
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleSendCardsReq:R[%d]T[%d] I[%d]  iUserID = %d", iRoomNum, iTableNum, iTableNumExtra, nodePlayers->iUserID);
		return;
	}

	MJGTableItemDef* tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	SendCardsReqDef * pSendCard = (SendCardsReqDef *)pMsgData;
	char cSendCard = pSendCard->cCard;
	//如果当前摸牌的玩家 不是打牌的这个玩家  踢出玩家
	if (tableItem->iCurMoPlayer != nodePlayers->cTableNumExtra)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleSendCard:R[%d]T[%d] [%s][%d], illegal Player[%d], Real Player[%d], CardType[%d]Value[%d]",
			iRoomNum, iTableNum, nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->cTableNumExtra, tableItem->iCurMoPlayer, cSendCard >> 4, cSendCard & 0xf);
		SendKickOutWithMsg(nodePlayers, "不轮到当前玩家出牌", 1301,1);
		return;
	}

	//检查玩家手中有没有这张牌
	bool bFind = false;
	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		if (nodePlayers->cHandCards[i] == pSendCard->cCard)
		{
			bFind = true;
			break;
		}
	}

	if (bFind == false)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleSendCard:R[%d]T[%d] [%s][%d], Player Hand Not Exist Card: CardType[%d]Value[%d]", iRoomNum, iTableNum, nodePlayers->szNickName, nodePlayers->iUserID, cSendCard >> 4, cSendCard & 0xf);
		SendKickOutWithMsg(nodePlayers, "您手中没有这张牌", 1301, 1);
		return;
	}

	if (nodePlayers->iSpecialFlag >> 6)			// 玩家可以胡牌
	{
		nodePlayers->iSpecialFlag = 0;
		nodePlayers->iWinFlag = 0;
		nodePlayers->iHuFlag = 0;
		tableItem->iWillHuNum = 0;
	}

	vector<char> vcCards;
	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		vcCards.push_back(nodePlayers->cHandCards[i]);
		//_log(_DEBUG, "HZXLGameLogic", "HandleSendCard_log() nodePlayers->cHandCards[%d] = %d", i, nodePlayers->cHandCards[i]);
	}
	_log(_DEBUG, "HZXLGameLogic", "HandleSendCard: R[%d] T[%d], iExtra[%d], szNickName[%s], UserID[%d], CardType[%d]Value[%d], iHandCardsNum[%d]",
		iRoomNum, iTableNum, iTableNumExtra, nodePlayers->szNickName, nodePlayers->iUserID, cSendCard >> 4, cSendCard & 0xf, nodePlayers->iHandCardsNum);

	map<char, vector<char> > mpTing;
	map<char, map<char, int> > mpTingFan;

	// 判断玩家出牌后能否听牌，如果可以听牌，保存听牌相关的信息(此时玩家手中要打出的牌还没有删除，可以判断听牌)
	nodePlayers->bTing = false;
	nodePlayers->iTingHuFan = 0;
	int iFlag = JudgeSpecialTing(nodePlayers, tableItem, nodePlayers->resultType, vcCards, mpTing, mpTingFan); // 玩家出牌后，判断能否听牌
	_log(_DEBUG, "HZXLGameLogic", "HandleSendCard_log() iFlag = %d, cSendCard = %d, mpTing[cSendCard].size() = %d", iFlag, cSendCard, mpTing[cSendCard].size());
	if (iFlag > 0 && !mpTing[cSendCard].empty())
	{
		vector<char>().swap(nodePlayers->vcSendTingCard);
		//给玩家听的牌 赋值
		nodePlayers->bTing = true;
		vector<char>().swap(nodePlayers->vcTingCard);
		nodePlayers->vcTingCard.insert(nodePlayers->vcTingCard.begin(), mpTing[cSendCard].begin(), mpTing[cSendCard].end());
		map<char, int>().swap(nodePlayers->mpTingFan);
		nodePlayers->mpTingFan = mpTingFan[cSendCard];

		int iTempMaxFan = 0;
		map<char, int>::iterator pos = mpTingFan[cSendCard].begin();
		while (pos != mpTingFan[cSendCard].end())
		{
			if (pos->second > iTempMaxFan)
				iTempMaxFan = pos->second;
			pos++;
		}
		nodePlayers->iTingHuFan = iTempMaxFan;
		//桌子节点上听牌玩家保留
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->iTingPlyerPos[i] == -1)
			{
				tableItem->iTingPlyerPos[i] = nodePlayers->cTableNumExtra;
				break;
			}
		}
	}

	//给出牌的位置赋值
	tableItem->iWillHuNum = 0;
	tableItem->cCurMoCard = 0;
	tableItem->iCurMoPlayer = -1;
	tableItem->iCurSendPlayer = nodePlayers->cTableNumExtra;
	tableItem->iLastSendPlayer = nodePlayers->cTableNumExtra;
	//给当前出牌赋值
	tableItem->cTableCard = cSendCard;
	nodePlayers->cSendCards[nodePlayers->iAllSendCardsNum] = tableItem->cTableCard;
	nodePlayers->iAllSendCardsNum++;

	//将玩家手中的牌去掉
	for (int i = 0; i<nodePlayers->iHandCardsNum; i++)
	{
		if (nodePlayers->cHandCards[i] == cSendCard)
		{
			for (int j = i; j<nodePlayers->iHandCardsNum - 1; j++)
			{
				nodePlayers->cHandCards[j] = nodePlayers->cHandCards[j + 1];
			}
			nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1] = 0;
			nodePlayers->iHandCardsNum--;
			break;
		}
	}
	if (cSendCard >> 4 != nodePlayers->iQueType)
	{
		int iAllCard[5][10] = {0};
		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
		{
			int iType = nodePlayers->cHandCards[i] >> 4;
			int iValue = nodePlayers->cHandCards[i] & 0x000f;
			//手上还有定缺的牌 直接返回 不能胡
			if (iType == nodePlayers->iQueType)
			{
				continue;
			}
			iAllCard[iType][0] ++;
			iAllCard[iType][iValue] ++;
		}
	}

	nodePlayers->iSpecialFlag = 0;
	nodePlayers->iKickOutTime = 0;

	//给玩家发送出牌通知
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			SendCardsNoticeDef pMsgNotice;
			memset(&pMsgNotice,0,sizeof(SendCardsNoticeDef));
			pMsgNotice.cTableNumExtra = nodePlayers->cTableNumExtra;
			pMsgNotice.cCard = tableItem->cTableCard;
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsgNotice, MJG_SEND_CARD_RES, sizeof(SendCardsNoticeDef));
		}
	}
	_log(_DEBUG, "HZXLGameLogic", "HandleSendCard: userid[%d], szNickName[%s], cTableNumExtra[%d], cTableCard[%d]", nodePlayers->iUserID, nodePlayers->szNickName, nodePlayers->cTableNumExtra, tableItem->cTableCard);

	//先判断其他玩家有没有点炮的 有点炮	
	for (int i = 1; i < PLAYER_NUM; i++)
	{
		//按照当前玩家的顺序 判断玩家的是否可以吃碰听杠胡
		int iTableExtra = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
		_log(_DEBUG, "HZXLGameLogic", "HandleSendCard: iTableExtra[%d], bIsHu[%d]", iTableExtra, nodePlayers->bIsHu);
		if (tableItem->pPlayers[iTableExtra] != NULL && tableItem->pPlayers[iTableExtra]->cPoChan == 0)
		{
			char cFanResult[MJ_FAN_TYPE_CNT];
			memset(cFanResult, 0, sizeof(cFanResult));
			int iHuFan = JudgeSpecialHu(tableItem->pPlayers[iTableExtra], tableItem, cFanResult);	//点炮胡判断
			if (iHuFan > 0)
			{
				_log(_DEBUG, "HZXLGameLogic", "HandleSendCard: userid[%d], szNickName[%s], cTableNumExtra[%d], iHuFan[%d], Player->iHuFan[%d], bGuoHu[%d]",
					tableItem->pPlayers[iTableExtra]->iUserID, tableItem->pPlayers[iTableExtra]->szNickName, tableItem->pPlayers[iTableExtra]->cTableNumExtra, iHuFan,
					tableItem->pPlayers[iTableExtra]->iHuFan, tableItem->pPlayers[iTableExtra]->bGuoHu);

				if (tableItem->pPlayers[iTableExtra]->bGuoHu)
				{
					if (iHuFan > tableItem->pPlayers[iTableExtra]->iHuFan)
					{
						tableItem->pPlayers[iTableExtra]->iHuFan = iHuFan;
						tableItem->pPlayers[iTableExtra]->bGuoHu = false;
						memcpy(tableItem->pPlayers[iTableExtra]->cFanResult, cFanResult, sizeof(cFanResult));
						//_log(_DEBUG, "HZXLGameLogic", "HandleSendCard_Log(): bGuoHu   00000");
					}
					else
					{
						//_log(_DEBUG, "HZXLGameLogic", "HandleSendCard_Log(): bGuoHu   11111");
						tableItem->pPlayers[iTableExtra]->iHuFan = 0;
						memset(tableItem->pPlayers[iTableExtra]->cFanResult, 0, sizeof(tableItem->pPlayers[iTableExtra]->cFanResult));
						continue;
					}
				}
				else
				{
					tableItem->pPlayers[iTableExtra]->iHuFan = iHuFan;
					memcpy(tableItem->pPlayers[iTableExtra]->cFanResult, cFanResult, sizeof(cFanResult));
					//_log(_DEBUG, "HZXLGameLogic", "HandleSendCard_Log(): bGuoHu   22222");
				}
				tableItem->pPlayers[iTableExtra]->iSpecialFlag = 1 << 5;

				//tableItem->iYiPaoDuoXiangPlayer[iTableExtra] = 1;//0初值  1可以胡牌  2胡牌了 3放弃胡牌
				tableItem->iWillHuNum++;
			}
			else
			{
				tableItem->pPlayers[iTableExtra]->iHuFan = 0;
				memset(&tableItem->pPlayers[iTableExtra]->cFanResult, 0, sizeof(tableItem->pPlayers[iTableExtra]->cFanResult));
			}
		}
	}

	for (int i = 1; i < PLAYER_NUM; i++)
	{
		//按照当前玩家的顺序 判断其他玩家的是否可以吃碰听杠胡
		int iTableExtra = (tableItem->iCurSendPlayer + i);
		if (iTableExtra >= PLAYER_NUM)
			iTableExtra = iTableExtra - PLAYER_NUM;

		if (tableItem->pPlayers[iTableExtra] && tableItem->pPlayers[iTableExtra]->cPoChan == 0)
		{
			int iSpecialFlag = 0;

			//判断其他玩家的碰杠状态
			int iFlag = JudgeSpecialPeng(tableItem->pPlayers[iTableExtra], tableItem);   // 0 1
			iSpecialFlag |= iFlag << 1;

			int iGangNum = 0;
			char cGangCard[14] = {0};
			char cGangType[14] = { 0 };
			iFlag = JudgeSpecialGang(tableItem->pPlayers[iTableExtra], tableItem, iGangNum, cGangCard, cGangType);
			iSpecialFlag |= iFlag << 2;
			_log(_DEBUG, "xlGameLogic", "HandleSendCard: Gang：------iSpecialFlag[%d]-----------cTableCard[%d] --------iTableExtra[%d]", iSpecialFlag, tableItem->cTableCard, iTableExtra);
			tableItem->pPlayers[iTableExtra]->iSpecialFlag |= iSpecialFlag;
		}
	}
	SendSpecialServerToPlayer(tableItem);
}

void HZXLGameLogic::SendSpecialServerToPlayer(MJGTableItemDef *tableItem)
{
	_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() tableItem->iWillHuNum[%d]", tableItem->iWillHuNum);
	if (tableItem->iWillHuNum > 0)
	{
		int viSpecialSeatFlag[4] = { 0, 0, 0, 0 };
		for (int i = 1; i< 4; i++)
		{
			int iTableExtra = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
			if (tableItem->pPlayers[iTableExtra] && tableItem->pPlayers[iTableExtra]->cPoChan == 0)
			{
				if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[iTableExtra]->iPlayerType ==  NORMAL_PLAYER))
				{
					int iFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 5;
					if (iFlag > 0)
					{
						tableItem->pPlayers[iTableExtra]->iStatus = PS_WAIT_SPECIAL;

						int iTotalMulti = CalculateTotalMulti(tableItem->pPlayers[iTableExtra]);
						SpecialCardsServerDef pMsg;
						memset(&pMsg, 0, sizeof(SpecialCardsServerDef));
						pMsg.cTableNumExtra = iTableExtra;
						pMsg.iSpecialFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag;
						pMsg.cHuCard = tableItem->cTableCard;
						pMsg.iHuMulti = iTotalMulti;

						pMsg.cWillHuNum = tableItem->iWillHuNum;
						pMsg.iHuType = tableItem->pPlayers[iTableExtra]->iHuFlag;
						pMsg.iHuFlag = tableItem->pPlayers[iTableExtra]->iWinFlag;
						pMsg.cTagExtra = tableItem->iCurSendPlayer;

						tableItem->pPlayers[iTableExtra]->iKickOutTime = KICK_OUT_TIME;
						CLSendSimpleNetMessage(tableItem->pPlayers[iTableExtra]->iSocketIndex, &pMsg, MJG_SPECIAL_CARD_SERVER, sizeof(SpecialCardsServerDef));

						viSpecialSeatFlag[iTableExtra] = 1;
					}
				}
				else
				{
					_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() 判断AI胡牌 ", tableItem->iWillHuNum);
					if (tableItem->pPlayers[iTableExtra]->iPlayerType !=  NORMAL_PLAYER)
					{
						//处理AI玩家的胡牌
						int iFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 5;
						if (iFlag > 0)
						{
							_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() 当前AI[%d] Can HU", tableItem->pPlayers[iTableExtra]->iPlayerType);
							tableItem->pPlayers[iTableExtra]->iStatus = PS_WAIT_SPECIAL;
							m_pHzxlAICTL->CreateAISpcialCardReq(tableItem, tableItem->pPlayers[iTableExtra]);
							viSpecialSeatFlag[iTableExtra] = 1;
						}
					}
				}
			}
		}
		SendSpecialInfoToClient(tableItem, viSpecialSeatFlag);			// 有玩家可以胡牌

		//玩家掉线的自动操作
		int iWill = tableItem->iWillHuNum;
		for (int i = 1; i < 4; i++)
		{
			int iTableExtra = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
			if (tableItem->pPlayers[iTableExtra] && tableItem->pPlayers[iTableExtra]->cPoChan == 0)
			{
				int iFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 5;
				if (iFlag > 0)
				{
					iWill--;
					if (tableItem->pPlayers[iTableExtra]->bIfWaitLoginTime == true || tableItem->pPlayers[iTableExtra]->cDisconnectType == 1)
					{
						AutoSendCards(tableItem->pPlayers[iTableExtra]->iUserID);
					}
					if (iWill == 0)
						return;
				}
			}
		}

		return;
	}

	int viSpecialSeatFlag[4] = { 0, 0, 0, 0 };

	//如果没有胡牌的玩家 那就看下有没有可以 碰 杠的玩家
	for (int i = 1; i < PLAYER_NUM; i++)
	{
		//按照当前玩家的顺序 判断玩家的是否可以碰杠
		int iTableExtra = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
		//_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() tableItem->iCurSendPlayer[%d]iTableExtra[%d]", tableItem->iCurSendPlayer, iTableExtra);
		
		if (tableItem->pPlayers[iTableExtra] && tableItem->pPlayers[iTableExtra]->cPoChan == 0)// && tableItem->pPlayers[iTableExtra]->bIfHuPai == false
		{
			if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[iTableExtra]->iPlayerType ==  NORMAL_PLAYER))
			{
				int iFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 1;

				//可以碰
				if (iFlag > 0)
				{
					//_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() NormalPlayer 碰杠 iFlag[%d]", iFlag);
					tableItem->pPlayers[iTableExtra]->iStatus = PS_WAIT_SPECIAL;
					SpecialCardsServerDef pMsg;
					memset(&pMsg, 0, sizeof(SpecialCardsServerDef));
					pMsg.cTableNumExtra = iTableExtra;
					pMsg.iSpecialFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag;   //碰 杠 的标记
					pMsg.cHuCard = tableItem->cTableCard;

					tableItem->pPlayers[iTableExtra]->iKickOutTime = KICK_OUT_TIME;
					CLSendSimpleNetMessage(tableItem->pPlayers[iTableExtra]->iSocketIndex, &pMsg, MJG_SPECIAL_CARD_SERVER, sizeof(SpecialCardsServerDef));

					viSpecialSeatFlag[iTableExtra] = 1;
					//如果玩家掉线 自动帮玩家打牌
					if (tableItem->pPlayers[iTableExtra]->bIfWaitLoginTime == true || tableItem->pPlayers[iTableExtra]->cDisconnectType == 1)
					{
						AutoSendCards(tableItem->pPlayers[iTableExtra]->iUserID);
					}

					SendSpecialInfoToClient(tableItem, viSpecialSeatFlag);			// 有玩家可以碰杠
					return;
				}
			}
			else
			{
				//处理AI玩家的碰杠请求
				if (tableItem->pPlayers[iTableExtra]->iPlayerType !=  NORMAL_PLAYER)
				{
					int iFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 1;
					if (iFlag > 0)
					{
						//_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() AI 碰杠 iFlag[%d]", iFlag);

						tableItem->pPlayers[iTableExtra]->iStatus = PS_WAIT_SPECIAL;
						m_pHzxlAICTL->CreateAISpcialCardReq(tableItem, tableItem->pPlayers[iTableExtra]);

						viSpecialSeatFlag[iTableExtra] = 1;
						SendSpecialInfoToClient(tableItem, viSpecialSeatFlag);			// 有玩家可以碰杠
						return;
					}
				}
				
			}
		}
	}
	//如果出牌了没有碰杠胡   下一个未胡牌摸牌
	int i;
	for (i = 1; i < PLAYER_NUM;i++)
	{
		tableItem->iCurMoPlayer = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
		_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() [%d] get Card cPoChan[%d]", tableItem->iCurMoPlayer,tableItem->pPlayers[tableItem->iCurMoPlayer]->cPoChan );
		if (tableItem->pPlayers[tableItem->iCurMoPlayer] && tableItem->pPlayers[tableItem->iCurMoPlayer]->cPoChan == 0)
		{
			SendCardToPlayer(tableItem->pPlayers[tableItem->iCurMoPlayer], tableItem);
			break;
		}
	}
	if (i == PLAYER_NUM)
	{
		_log(_ERROR, "HZXLGameLogic", "SendSpecialServer: T[%d] Player tableItem->iCurMoPlayer[%d] Abnormal 没有找到对应玩家", tableItem->iTableID, tableItem->iCurMoPlayer);
		GameEndHu(tableItem->pPlayers[tableItem->iCurSendPlayer], tableItem, false, tableItem->pPlayers[tableItem->iCurSendPlayer]->iSpecialFlag);		// 吃碰杠后，摸牌玩家异常，游戏结束
	}

}

//处理玩家 吃碰杠听胡
void HZXLGameLogic::HandleSpecialCard(int iUserID, void *pMsgData)
{
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));

	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLGameLogic", "●●●● HandleSendCard: nodePlayers is NULL ●●●●");
		return;
	}

	int  iTableNum = nodePlayers->cTableNum;                   //找出所在的桌
	char iTableNumExtra = nodePlayers->cTableNumExtra;
	char iRoomNum = nodePlayers->cRoomNum;

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleSendCardsReq:[%d][%d]", iTableNum, iTableNumExtra);
		return;
	}

	MJGTableItemDef* tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);

	SpecialCardsReqDef * pReq = (SpecialCardsReqDef *)pMsgData;
	int iSpecialFlag = pReq->iSpecialFlag;

	_log(_DEBUG, "HZXLGameLogic", "HandleSpecialCard_log() iSpecialFlag[%d]", iSpecialFlag);
	if (iSpecialFlag & 0x0002)
	{
		_log(_DEBUG, "HZXLGameLogic", "碰牌 Peng Pai R[%d]T[%d]  nodePlayer->userID = %d  ,iTableNumExtra = %d", iRoomNum, iTableNum, nodePlayers->iUserID, iTableNumExtra);
		HandlePeng(nodePlayers, tableItem, pMsgData);
		return;
	}

	if (iSpecialFlag & 0x000C)
	{
		_log(_DEBUG, "HZXLGameLogic", "杠牌 Gang Pai R[%d]T[%d]  nodePlayer->userID = %d  ,iTableNumExtra = %d", iRoomNum, iTableNum, nodePlayers->iUserID, iTableNumExtra);
		HandleGang(nodePlayers, tableItem, pMsgData);
		return;
	}

	if (iSpecialFlag >> 5)
	{
		_log(_DEBUG, "HZXLGameLogic", "胡牌  Hu Pai R[%d]T[%d]  nodePlayer->userID = %d  ,iTableNumExtra = %d", iRoomNum, iTableNum, nodePlayers->iUserID, iTableNumExtra);
		HandleHu(nodePlayers, tableItem, pMsgData);		// 客户端请求胡牌
		return;
	}

	if (iSpecialFlag == 0)
	{
		_log(_DEBUG, "HZXLGameLogic", "弃牌 Qi Pai R[%d]T[%d]  nodePlayer->userID = %d  ,iTableNumExtra = %d", iRoomNum, iTableNum, nodePlayers->iUserID, iTableNumExtra);
		HandleQi(nodePlayers, tableItem, pMsgData);
		return;
	}
}

void HZXLGameLogic::HandlePeng(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, void * pMsgData)
{
	if (nodePlayers->iStatus != PS_WAIT_SPECIAL)
	{
		_log(_ERROR, "HZXLGameLogic", "HandlePeng: R[%d] T[%d] name[%s] userid[%d][%d]当前玩家状态异常", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iStatus);
		SendKickOutWithMsg(nodePlayers, "当前玩家状态异常", 1301, 1);
		return;
	}


	//先判定是否可以碰牌 
	SpecialCardsReqDef * pReq = (SpecialCardsReqDef *)pMsgData;
	//判断当前吃的牌是不是桌面上的牌

	if (pReq->cFirstCard != tableItem->cTableCard)
	{
		_log(_ERROR, "HZXLGameLogic", "HandlePeng: R[%d] T[%d] name[%s] userid[%d] 要碰的牌不是刚打出的牌 [%d][%d]", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, pReq->cFirstCard, tableItem->cTableCard);
		SendKickOutWithMsg(nodePlayers, "要碰的牌不是刚打出的牌", 1301, 1);
		return;
	}


	//判断玩家手中有没有碰的牌

	int iTemp = 0;
	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		_log(_DEBUG, "HZXLGameLogic", "HandlePeng: playerpos[%d],handcards[%d]", nodePlayers->cTableNumExtra, nodePlayers->cHandCards[i]);
		if (nodePlayers->cHandCards[i] == pReq->cFirstCard)
		{
			iTemp++;
		}
	}

	if (iTemp < 2)
	{
		_log(_ERROR, "HZXLGameLogic", "HandlePeng: R[%d] T[%d] name[%s] userid[%d] 玩家手中碰牌数量不足 [%d][%d]", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, pReq->cFirstCard, iTemp);
		SendKickOutWithMsg(nodePlayers, "玩家手中碰牌数量不足", 1301, 1);
		return;
	}



	//nodePlayers->bIsHu = false;
	nodePlayers->iKickOutTime = 0;

	//判断玩家是否在一炮多响里面点的杠
	if (tableItem->iWillHuNum + tableItem->iAllHuNum > 1 && nodePlayers->iSpecialFlag >> 5)
	{
		//如果是第一个点的 记录玩家当前杠牌状态数据 
		if (tableItem->iWillHuNum > 1)
		{

			nodePlayers->iSpecialFlag = 2;					//把之前的杠存下来 防止弃胡 
			memset(&nodePlayers->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
			nodePlayers->pTempCPG.cFirstCard = pReq->cFirstCard;
			memcpy(nodePlayers->pTempCPG.cCards, pReq->cCards, sizeof(pReq->cCards));
			nodePlayers->pTempCPG.iSpecialFlag = nodePlayers->iSpecialFlag;
			tableItem->iWillHuNum--;
			nodePlayers->iWinFlag = 0;
			nodePlayers->iHuFlag = 0;
			return;
		}
		else
		{
			//否则视为弃胡
			nodePlayers->iSpecialFlag = 0;
			nodePlayers->iWinFlag = 0;
			nodePlayers->iHuFlag = 0;
			if (tableItem->iWillHuNum != 0)
				tableItem->iWillHuNum--;

			nodePlayers->iSpecialFlag = 0;
			nodePlayers->iStatus = PS_WAIT_SEND;
			nodePlayers->iKickOutTime = 0;

			if (tableItem->iWillHuNum)
				return;
			if (tableItem->iAllHuNum > 0)
			{
				for (int i = 0; i< PLAYER_NUM; i++)
				{
					int iTableExtra = (nodePlayers->cTableNumExtra + i) % PLAYER_NUM;
					if (tableItem->pPlayers[iTableExtra])
					{
						if (tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 5)
						{
							SpecialCardsReqDef pMsg;
							memset(&pMsg, 0, sizeof(SpecialCardsReqDef));
							pMsg.iSpecialFlag = 0x0020;
							pMsg.cFirstCard = tableItem->cTableCard;
							HandleHu(tableItem->pPlayers[iTableExtra], tableItem, &pMsg);
							return;
						}
					}
				}
			}
		}
	}

	//如果都没有 那么就将玩家的状态清空 走碰牌逻辑

	MJGCPGDef cpgInfo;
	memset(&cpgInfo, 0, sizeof(MJGCPGDef));
	cpgInfo.cCard[0] = pReq->cCards[0];
	cpgInfo.cCard[1] = pReq->cCards[1];
	cpgInfo.cChiValue = pReq->cFirstCard;
	cpgInfo.cGangType = 0;
	cpgInfo.cTagNum = tableItem->iCurSendPlayer;
	cpgInfo.iCPGType = 1;
	if (nodePlayers->iSpecialFlag & (0x000c))
	{
		//表示玩家放弃了杠选择碰 不能再杠
		cpgInfo.cGangType = 1;
	}
	//碰的信息记录
	nodePlayers->CPGInfo.Info[nodePlayers->CPGInfo.iAllNum] = cpgInfo;
	nodePlayers->CPGInfo.iAllNum++;

	//记录到玩家手中
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iType = tableItem->cTableCard >> 4;
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iValue = tableItem->cTableCard & 0xf;
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].bOther = true;   //碰别人的牌
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iGangType = 0;
	nodePlayers->resultType.iPengNum++;

	//将玩家的状态清空
	nodePlayers->iSpecialFlag = 0;
	memset(&nodePlayers->pTempCPG, 0, sizeof(MJGTempCPGReqDef));

	//找到吃牌倒下的那两张牌从玩家竖立的牌的数组中删除了 现在不要了！
	for (int k = 0; k<2; k++)
	{
		for (int i = 0; i<nodePlayers->iHandCardsNum; i++)
		{
			if (nodePlayers->cHandCards[i] == tableItem->cTableCard)
			{
				for (int j = i; j<nodePlayers->iHandCardsNum - 1; j++)
				{
					nodePlayers->cHandCards[j] = nodePlayers->cHandCards[j + 1];
				}
				nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1] = 0;
				nodePlayers->iHandCardsNum--;
				break;
			}
		}
	}

	//将吃的那张牌在出牌的数组中清除了  
	tableItem->pPlayers[tableItem->iCurSendPlayer]->cSendCards[tableItem->pPlayers[tableItem->iCurSendPlayer]->iAllSendCardsNum - 1] = 0;
	tableItem->pPlayers[tableItem->iCurSendPlayer]->iAllSendCardsNum--;


	//向玩家发送碰牌的通知
	SpecialCardsNoticeDef msgNotic;
	memset(&msgNotic, 0, sizeof(SpecialCardsNoticeDef));
	msgNotic.iSpecialFlag = 2;  //2  碰
	msgNotic.cTableNumExtra = nodePlayers->cTableNumExtra;
	msgNotic.cTagNumExtra = tableItem->iCurSendPlayer;
	msgNotic.cCards[0] = tableItem->cTableCard;
	msgNotic.cCards[1] = tableItem->cTableCard;
	msgNotic.cCards[2] = tableItem->cTableCard;
	
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotic, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef));		// 碰牌通知
		}
	}

	// 标记上家碰牌后自摸
	if (tableItem->cLeastPengExtra < 4)
	{
		int iGapExtra = tableItem->iCurSendPlayer - nodePlayers->cTableNumExtra + 4;
		if (iGapExtra % 4 == 1)
		{
			tableItem->cLeastPengExtra = tableItem->iCurSendPlayer;
		}
	}

	//清掉桌面上出的牌
	tableItem->cTableCard = 0;

	tableItem->iCurSendPlayer = -1;


	//流程结束掉 那就清除所有玩家的状态

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->cPoChan == 0)
		{
			tableItem->pPlayers[i]->iSpecialFlag = 0;
			memset(&tableItem->pPlayers[i]->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
		}
	}

	//如果换宝过程中有人对宝了  那就结束
	// if (CheckIfHuanBao(tableItem))
	// {
	// 	return ;
	// }
	tableItem->iCurMoPlayer = nodePlayers->cTableNumExtra;

	SendCardToPlayer(nodePlayers, tableItem, true);
}

void HZXLGameLogic::HandleGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, void * pMsgData)
{
	SpecialCardsReqDef * pReq = (SpecialCardsReqDef *)pMsgData;

	//自己摸牌 暗杠或补杠
	if (nodePlayers->cTableNumExtra == tableItem->iCurMoPlayer)
	{
		char cCard = pReq->cFirstCard;
		int iType = pReq->iSpecialFlag & 0x000C;
		if ((cCard >> 4) == nodePlayers->iQueType)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleGang: R[%d] T[%d]  [%s][%d] 当前不可以杠 [%d]", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, cCard);
			SendKickOutWithMsg(nodePlayers, "当前不可以杠", 1301, 1);
			return;
		}
		if (iType == 0x0004)	// iType 1为明杠 2为补杠  3为暗杠	
		{
			_log(_ERROR, "HZXLGameLogic", "HandleGang: R[%d] T[%d]  [%s][%d] 当前不可以明杠 [%d]", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, cCard);
			SendKickOutWithMsg(nodePlayers, "当前不可以明杠", 1301, 1);
			return;
		}

		//如果是补杠  那就从碰牌中找到和这张牌相同的牌，补杠注意抢杠胡
		if (iType == 0x0008)
		{
			bool bTemp = false;
			for (int i = 0; i < nodePlayers->CPGInfo.iAllNum; i++)
			{
				_log(_DEBUG, "HZXLGameLogic", "BUGANG: playerpos[%d],value[%d]", nodePlayers->cTableNumExtra,nodePlayers->CPGInfo.Info[i].cChiValue);
				if (nodePlayers->CPGInfo.Info[i].iCPGType == 1 && nodePlayers->CPGInfo.Info[i].cChiValue == cCard)
				{
					bTemp = true;
					break;
				}
			}

			if (bTemp == false)
			{
				_log(_ERROR, "HZXLGameLogic", "HandleGang: BuGang room =%d table =%d  [%s][%d] 当前不可以补杠 [%d]", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, cCard);
				SendKickOutWithMsg(nodePlayers, "当前不可以补杠", 1301, 1);
				return;
			}

			HandlePengGang(nodePlayers, tableItem, pReq);
		}
		// 暗杠
		else if (iType == 0x000C)
		{
			//判断玩家手里有没有这张牌
			int iNum = 0;
			for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
			{
				_log(_DEBUG, "HZXLGameLogic", "ANGANG: playerpos[%d],handcards[%d]", nodePlayers->cTableNumExtra, nodePlayers->cHandCards[i]);
				if (nodePlayers->cHandCards[i] == cCard)
				{
					iNum++;
				}
			}

			if (iNum < 4)
			{
				_log(_ERROR, "HZXLGameLogic", "HandleGang:Room[%d],  T[%d] [%s][%d] 手中牌数量不足，不可以暗杠 [%d][%d]",
					nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, cCard, iNum);
				SendKickOutWithMsg(nodePlayers, "手中牌数量不足，不可以暗杠", 1301, 1);
				return;
			}

			HandleAnGang(nodePlayers, tableItem, pReq);
		}
	}
	//别人打牌 明杠
	else
	{
		if (nodePlayers->iStatus != PS_WAIT_SPECIAL)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleGang: iStatus: R[%d]T[%d] [%s][%d][%d]当前玩家状态异常", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iStatus);
			SendKickOutWithMsg(nodePlayers, "当前玩家状态异常", 1301, 1);
			return;
		}
		if (nodePlayers->iSpecialFlag & 0x0004 == 0)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleGang: MingGang iSpecialFlag: R[%d]T[%d] [%s][%d][%d]当前玩家操作异常", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iSpecialFlag);
			SendKickOutWithMsg(nodePlayers, "当前玩家操作异常", 1301, 1);
			return;
		}

		if ((tableItem->cTableCard >> 4) == nodePlayers->iQueType)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleGang: MingGang cTableCard: R[%d]T[%d] [%s][%d] 当前不可以杠 [%d]", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, tableItem->cTableCard);
			SendKickOutWithMsg(nodePlayers, "当前不可以杠", 1301, 1);
			return;
		}

		//判断当前吃的牌是不是桌面上的牌
		if (pReq->cFirstCard != tableItem->cTableCard)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleGang: cFirstCard: R[%d]T[%d] [%s][%d] 要杠的牌不是刚打出的牌 [%d][%d]", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, pReq->cFirstCard, tableItem->cTableCard);
			SendKickOutWithMsg(nodePlayers, "要杠的牌不是刚打出的牌", 1301, 1);
			return;
		}

		//判断玩家手中有没有碰的牌
		int iTemp = 0;
		for (int i = 0; i <nodePlayers->iHandCardsNum; i++)
		{
			_log(_DEBUG, "HZXLGameLogic", "MINGGANG: playerpos[%d],handcards[%d]", nodePlayers->cTableNumExtra, nodePlayers->cHandCards[i]);
			if (nodePlayers->cHandCards[i] == pReq->cFirstCard)
			{
				iTemp++;
			}
		}

		if (iTemp < 3)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleGang:R[%d]T[%d][%s][%d] 玩家手中杠牌数量不足 [%d][%d]",
				nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, pReq->cFirstCard, iTemp);
			SendKickOutWithMsg(nodePlayers, "玩家手中杠牌数量不足", 1301, 1);
			return;
		}

		HandleMingGang(nodePlayers, tableItem, pReq);
	}
}

// 处理碰杠/补杠
void HZXLGameLogic::HandlePengGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, SpecialCardsReqDef *pReq)
{
	long long iRateMoney[PLAYER_NUM] = { 0 };		// 抽水和桌费
	long long iPunishMoney[PLAYER_NUM] = { 0 };	// 掉线惩罚，注意必须是正值
	char cCard = pReq->cFirstCard;
	int iTagPos = -1;

	nodePlayers->iKickOutTime = 0;

	//判断其他玩家是否有抢杠胡的 如果有的话 抢杠胡
	bool szBQiangGangHu[4] = { false };
	bool bQiangGangHu = false;
	int iFan[4] = { 0 };
	tableItem->iWillHuNum = 0;
	// 将其他玩家抢杠胡的胡牌信息保存到玩家节点中
	for (int i = 1; i < PLAYER_NUM; i++)
	{
		int iTablePos = (nodePlayers->cTableNumExtra + i) % PLAYER_NUM;
		if (tableItem->pPlayers[iTablePos] && tableItem->pPlayers[iTablePos]->cPoChan == 0)
		{
			char cFanResult[MJ_FAN_TYPE_CNT];
			memset(cFanResult, 0, sizeof(cFanResult));
			iFan[iTablePos] = JudgeSpecialHu(tableItem->pPlayers[iTablePos], tableItem, cFanResult, cCard);			// 抢杠胡判断
			_log(_DEBUG, "HZXLMJGameLogic", "HandleGang  i[%d], iTablePos[%d], iFan[%d], iHuFan[%d]", i, iTablePos, iFan[iTablePos], tableItem->pPlayers[iTablePos]->iHuFan);
			if (iFan[iTablePos] > 0)
			{
				if (tableItem->pPlayers[iTablePos]->bGuoHu)
				{
					if (iFan[iTablePos] > tableItem->pPlayers[iTablePos]->iHuFan)
					{
						_log(_DEBUG, "HZXLMJGameLogic", "HandleGang R[%d]T[%d]  补杠 抢杠胡  JudgeSpecialHuFan iFan[iTablePos] = %d   tableItem->pPlayers[iTablePos]->iHuFan =%d",
							tableItem->pPlayers[iTablePos]->cRoomNum, tableItem->iTableID, iFan[iTablePos], tableItem->pPlayers[iTablePos]->iHuFan);
						tableItem->pPlayers[iTablePos]->iSpecialFlag = 0x0020;
						tableItem->pPlayers[iTablePos]->iHuFan = iFan[iTablePos];
						tableItem->pPlayers[iTablePos]->iHuFlag = iFan[iTablePos];
						tableItem->pPlayers[iTablePos]->bGuoHu = false;
						memcpy(tableItem->pPlayers[iTablePos]->cFanResult, cFanResult, sizeof(cFanResult));
					}
					else
					{
						iFan[iTablePos] = 0;
						tableItem->pPlayers[iTablePos]->iHuFlag = 0;
						tableItem->pPlayers[iTablePos]->iWinFlag = 0;
						memset(tableItem->pPlayers[iTablePos]->cFanResult, 0, sizeof(nodePlayers->cFanResult));
						continue;
					}
				}
				else
				{
					tableItem->pPlayers[iTablePos]->iSpecialFlag = 0x0020;
					tableItem->pPlayers[iTablePos]->iHuFan = iFan[iTablePos];;
					tableItem->pPlayers[iTablePos]->iHuFlag = iFan[iTablePos];
					memcpy(tableItem->pPlayers[iTablePos]->cFanResult, cFanResult, sizeof(cFanResult));
				}
				szBQiangGangHu[iTablePos] = true;
				bQiangGangHu = true;
				tableItem->iWillHuNum++;
			}
			else
			{
				memset(&tableItem->pPlayers[iTablePos]->cFanResult, 0, sizeof(tableItem->pPlayers[iTablePos]->cFanResult));
			}
		}
	}

	SpecialCardsNoticeDef msgNotic;
	memset(&msgNotic, 0, sizeof(SpecialCardsNoticeDef));
	msgNotic.iSpecialFlag = 8;
	msgNotic.cTableNumExtra = nodePlayers->cTableNumExtra;
	msgNotic.cTagNumExtra = iTagPos;
	msgNotic.cCards[0] = cCard;
	msgNotic.cCards[1] = cCard;
	msgNotic.cCards[2] = cCard;
	msgNotic.cQiangGangHu = bQiangGangHu;
	if(nodePlayers->pTempCPG.cBeQiangGangHu == 1)
		msgNotic.iHuFlag = 1000;
	nodePlayers->pTempCPG.cBeQiangGangHu = 0;
	
	

	SpecialHuNoticeDef pMsgHu;
	memset(&pMsgHu, 0, sizeof(SpecialHuNoticeDef));
	pMsgHu.iZhuangPos = tableItem->iZhuangPos;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		pMsgHu.cHuNumExtra[i] = -1;
		pMsgHu.cPoChan[i] = 0;
	}

	PoChanCardNoticeMsgDef pMsgPoChan;
	memset(&pMsgPoChan, 0, sizeof(PoChanCardNoticeMsgDef));

	int viReslutGangFen[4][4] = { { 0 },{ 0 },{ 0 },{ 0 } };			// 四个玩家根据模拟分得到的杠分
	memset(viReslutGangFen, 0, sizeof(viReslutGangFen));
	char vcPoChan[4][4];											// 四个玩家的破产状态
	memset(vcPoChan, 0, sizeof(vcPoChan));

	// 计算其他玩家应扣的补杠分，保存到SpecialNotice消息中
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iBaseGangFen = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;			// 基础杠分
			long long iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费

			for (int j = 0; j < PLAYER_NUM; j++)			// 对于玩家i，计算每个玩家的杠分
			{
				int iResultGangFen = iBaseGangFen;

				if (j == nodePlayers->cTableNumExtra || tableItem->pPlayers[j]->cPoChan == 1)		// 赢杠玩家跳过，玩家破产跳过
					continue;

				int iTempMoney = 0;
				if (j == i)
				{
					iTempMoney = tableItem->pPlayers[i]->iMoney + tableItem->pPlayers[i]->iCurWinMoney - iCurTableMoney;
					//_log(_DEBUG, "HZXLGameLogic", "HandlePengGang 1111 [%d][%d][%d][%d]", iTempMoney, tableItem->pPlayers[i]->iMoney, tableItem->pPlayers[i]->iCurWinMoney, iCurTableMoney);
				}
				else
				{
					iTempMoney = tableItem->pPlayers[i]->otherMoney[j].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[j] - iCurTableMoney;
					//_log(_DEBUG, "HZXLGameLogic", "  2222 [%lld][%lld][%lld][%lld]", iTempMoney, tableItem->pPlayers[i]->otherMoney[j].llMoney, tableItem->pPlayers[i]->iAllUserCurWinMoney[j], iCurTableMoney);
				}
				if (iTempMoney < iResultGangFen)		// 金币不够扣杠分
				{
					//_log(_DEBUG, "HZXLGameLogic", "HandlePengGang 3333 ");
					iResultGangFen = iTempMoney;
					vcPoChan[i][j] = 1;
				}
				viReslutGangFen[i][j] = 0 - iResultGangFen;
				viReslutGangFen[i][nodePlayers->cTableNumExtra] += iResultGangFen;
			}
		}
	}

	// 保存玩家杠分
	if (bQiangGangHu == false)
	{
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i])
			{
				for (int j = 0; j < PLAYER_NUM; j++)
				{
					if (i == j)
					{
						tableItem->pPlayers[i]->iCurWinMoney += viReslutGangFen[i][j];
						tableItem->pPlayers[i]->iGangFen += viReslutGangFen[i][j];
					}
					else
					{
						tableItem->pPlayers[i]->iAllUserCurWinMoney[j] += viReslutGangFen[i][j];
					}
				}
			}
		}
	}

	// 如果没有抢杠胡，执行补杠
	if (bQiangGangHu == false)
	{
		memset(&nodePlayers->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
		for (int i = 0; i < nodePlayers->CPGInfo.iAllNum; i++)
		{
			if (nodePlayers->CPGInfo.Info[i].iCPGType == 1 && nodePlayers->CPGInfo.Info[i].cChiValue == cCard)		// 玩家有一组碰牌
			{
				nodePlayers->CPGInfo.Info[i].iCPGType = 2;
				nodePlayers->CPGInfo.Info[i].cGangType = 1;
				iTagPos = nodePlayers->CPGInfo.Info[i].cTagNum;
				for (int j = 0; j < PLAYER_NUM; j++)
				{
					if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->cPoChan == 0)
					{
						if (tableItem->pPlayers[j] != nodePlayers)
							nodePlayers->CPGInfo.Info[i].cGangTag |= 1 << j;
					}
				}
				break;
			}
		}
		//沒有抢杠胡
		
		// 手牌信息中，碰牌改为明杠牌
		for (int i = 0; i < nodePlayers->resultType.iPengNum; i++)
		{
			int iType = cCard / 16;
			int iValue = cCard % 16;

			if (nodePlayers->resultType.pengType[i].iType == iType && nodePlayers->resultType.pengType[i].iValue == iValue)
			{
				nodePlayers->resultType.pengType[i].iGangType = 1;
				break;
			}
		}

		// 补杠时，将玩家手中的那张牌去掉
		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
		{
			if (nodePlayers->cHandCards[i] == cCard)
			{
				for (int j = i; j < nodePlayers->iHandCardsNum - 1; j++)
				{
					nodePlayers->cHandCards[j] = nodePlayers->cHandCards[j + 1];
				}
				nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1] = 0;
				nodePlayers->iHandCardsNum--;
				break;
			}
		}

		// 根据玩家杠分和破产信息，发送至客户端
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			memset(msgNotic.iGangFen, 0, sizeof(msgNotic.iGangFen));
			memset(pMsgHu.iMoneyResult, 0, sizeof(pMsgHu.iMoneyResult));
			memset(pMsgHu.iMoney, 0, sizeof(pMsgHu.iMoney));
			memset(pMsgHu.cPoChan, 0, sizeof(pMsgHu.cPoChan));
			memset(pMsgHu.viCurWinMoney, 0, sizeof(pMsgHu.viCurWinMoney));
			memset(pMsgPoChan.cHandCards, 0, sizeof(pMsgPoChan.cHandCards));
			memset(pMsgPoChan.cpgInfo, 0, sizeof(pMsgPoChan.cpgInfo));
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费
			pMsgHu.iTableMoney = iCurTableMoney;

			bool bExistPoChan = false;
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				msgNotic.iGangFen[j] = viReslutGangFen[i][j];
				pMsgHu.iMoneyResult[i] = viReslutGangFen[i][j];
				if (i == j)
				{
					pMsgHu.iMoney[i] = tableItem->pPlayers[i]->iMoney;
					pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iCurWinMoney;
				}
				else
				{
					pMsgHu.iMoney[i] = tableItem->pPlayers[i]->otherMoney[j].llMoney;
					pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iAllUserCurWinMoney[j];
				}

				if (vcPoChan[i][j] == 1)		// 玩家破产
				{
					pMsgHu.cPoChan[j] = 1;
					memcpy(pMsgPoChan.cHandCards[j], tableItem->pPlayers[j]->cHandCards, sizeof(tableItem->pPlayers[j]->cHandCards));
					memcpy(&pMsgPoChan.cpgInfo[j], &tableItem->pPlayers[j]->CPGInfo, sizeof(tableItem->pPlayers[j]->CPGInfo));
					bExistPoChan = true;
				}
			}

			if (bExistPoChan)		// 玩家破产
			{
				pMsgPoChan.iTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsgPoChan, MJG_PLAYER_POCHAN_MSG, sizeof(PoChanCardNoticeMsgDef));	// 补杠后破产

				msgNotic.cHaveHuNotice = 1;
				char *cMsgBuffer = new char[sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef)];
				memset(cMsgBuffer, 0, sizeof(cMsgBuffer));
				memcpy(cMsgBuffer, &msgNotic, sizeof(SpecialCardsNoticeDef));
				memcpy(cMsgBuffer + sizeof(SpecialCardsNoticeDef), &pMsgHu, sizeof(SpecialHuNoticeDef));

				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, (void *)cMsgBuffer, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef));	// 补杠后破产
				delete[] cMsgBuffer;
			}
			else
			{
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotic, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef));		// 补杠
			}
		}

		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i])
			{
				_log(_DEBUG, "HZXLMJGameLogic", "HandleBuGang_Log()UID[%d], pPlayers[%d]->iCurWinMoney = %lld", tableItem->pPlayers[i]->iUserID, i, tableItem->pPlayers[i]->iCurWinMoney);
			}
		}

		// 如果有玩家杠分破产，该玩家计费并结束游戏
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (vcPoChan[i][i] == 1)
			{
				tableItem->iPoChanNum++;
				tableItem->pPlayers[i]->cPoChan = 1;
				tableItem->pPlayers[i]->iStatus = PS_WAIT_RESERVE_TWO;		// 补杠后破产
				GameUserAccountReqDef msgRD;									// ACCOUNT_REQ_RADIUS_MSG	向Radius发送计分更改请求
				memset(&msgRD, 0, sizeof(GameUserAccountReqDef));
				msgRD.iUserID = htonl(tableItem->pPlayers[i]->iUserID);
				msgRD.iGameID = htonl(m_pServerBaseInfo->iGameID);
				GameUserAccountGameDef msgGameRD;
				memset(&msgRD, 0, sizeof(GameUserAccountGameDef));
				int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
				msgGameRD.llTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;
				SetAccountReqRadiusDef(&msgRD, &msgGameRD,tableItem, tableItem->pPlayers[i], tableItem->pPlayers[i]->iCurWinMoney, iPunishMoney[i], iRateMoney[i]);	// 补杠后破产
				memcpy(&tableItem->pPlayers[i]->msgRDTemp, &msgRD, sizeof(GameUserAccountReqDef));
			}
		}
	}
	else
	{
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i])
			{
				memset(msgNotic.iGangFen, 0, sizeof(msgNotic.iGangFen));
				for (int j = 0; j < PLAYER_NUM; j++)
				{
					msgNotic.iGangFen[j] = viReslutGangFen[i][j];
				}
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotic, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef));		// 抢杠胡
			}
		}
	}
	if (!bQiangGangHu)
	{
		// 记录补杠流水
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			GameLiuShuiInfoDef TempLs;
			memset(&TempLs, 0, sizeof(GameLiuShuiInfoDef));
			TempLs.cWinner |= (1 << nodePlayers->cTableNumExtra);
			TempLs.iSpecialFlag = WIN_GUA_FENG;			// 补杠 0x0008
			TempLs.iWinMoney = viReslutGangFen[i][i];
			memset(TempLs.iMoney, 0, sizeof(TempLs.iMoney));
			TempLs.cLoser = 0;
			TempLs.iFan = 1;
			TempLs.cPoChan = 0;

			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->cPoChan == 0 && j != nodePlayers->cTableNumExtra)
				{
					TempLs.cLoser |= (1 << j);
					if (tableItem->pPlayers[j]->cPoChan == 1)
						TempLs.cPoChan |= (1 << j);
				}
			}

			memcpy(TempLs.iMoney, viReslutGangFen[i], sizeof(viReslutGangFen[i]));
			tableItem->vLiuShuiList[i].push_back(TempLs);
		}
	}

	//向玩家发送抢杠胡的通知
	if (bQiangGangHu)
	{
		tableItem->iCurSendPlayer = nodePlayers->cTableNumExtra;
		tableItem->cTableCard = cCard;
		_log(_DEBUG, "HZXLMJGameLogic", "HandlePengGang_Log QiangGangHu iCurSendPlayer[%d], cTableCard[%d], cFirstCard[%d]", tableItem->iCurSendPlayer, tableItem->cTableCard, pReq->cFirstCard);

		//抢杠胡没有杠分
		msgNotic.cQiangGangHu = 1;
		nodePlayers->iSpecialFlag = 8;					//抢杠胡要把之前的杠存下来 防止弃胡 
		memset(&nodePlayers->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
		nodePlayers->pTempCPG.cFirstCard = pReq->cFirstCard;
		memcpy(nodePlayers->pTempCPG.cCards, pReq->cCards, sizeof(pReq->cCards));
		nodePlayers->pTempCPG.iSpecialFlag = nodePlayers->iSpecialFlag;
		nodePlayers->pTempCPG.cBeQiangGangHu = 1;

		//如果抢杠胡 就把当前出牌的玩家赋值为补杠的那个玩家   当前杠的牌就赋值给桌面出的牌吧
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (szBQiangGangHu[i])			// 玩家可以胡抢杠
			{
				//如果抢杠胡的是普通玩家，要发送胡牌消息
				if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER))
				{
					if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->bGuoHu == false && tableItem->pPlayers[i]->cPoChan == 0)				// 明杠 4 补杠 8  暗杠c
					{
						tableItem->pPlayers[i]->iSpecialFlag |= 1 << 5;		//   左移5位 为胡		20平胡 40自摸
						tableItem->pPlayers[i]->bCanQiangGangHu = true;

						SpecialCardsServerDef pMsg;
						memset(&pMsg, 0, sizeof(SpecialCardsServerDef));
						int iTotalMulti = CalculateTotalMulti(tableItem->pPlayers[i]);
						pMsg.cTableNumExtra = i;
						pMsg.iSpecialFlag = tableItem->pPlayers[i]->iSpecialFlag;
						pMsg.cHuCard = tableItem->cTableCard;
						pMsg.cWillHuNum = tableItem->iWillHuNum;
						pMsg.iHuType = tableItem->pPlayers[i]->iHuFlag;
						pMsg.iHuFlag = tableItem->pPlayers[i]->iWinFlag;
						pMsg.cTagExtra = tableItem->iCurSendPlayer;
						pMsg.iHuMulti = iTotalMulti;
						_log(_DEBUG, "HZXLMJGameLogic", "HandlePengGang_Log() i[%d], iTotalMulti[%d]", i, iTotalMulti);

						tableItem->pPlayers[i]->iStatus = PS_WAIT_SPECIAL;
						tableItem->pPlayers[i]->iKickOutTime = KICK_OUT_TIME;
						CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsg, MJG_SPECIAL_CARD_SERVER, sizeof(SpecialCardsServerDef));		// 请求玩家胡牌
					}
				}
				else
				{
					if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->bGuoHu == false && tableItem->pPlayers[i]->cPoChan == 0)				// 明杠 4 补杠 8  暗杠c
					{
						//如果是AI的抢杠胡，需要去调用AI的胡牌判断
						tableItem->pPlayers[i]->iSpecialFlag |= 1 << 5;		//   左移5位 为胡		
						tableItem->pPlayers[i]->bCanQiangGangHu = true;
						m_pHzxlAICTL->CreateAISpcialCardReq(tableItem, tableItem->pPlayers[i]);
					}
				}
			}
		}
		int iWill = tableItem->iWillHuNum;
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (szBQiangGangHu[i])
			{
				if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->bGuoHu == false && tableItem->pPlayers[i]->cPoChan == 0)				// 明杠 4 补杠 8  暗杠c
				{
					iWill--;
					//如果玩家掉线 自动帮玩家打牌
					if (tableItem->pPlayers[i]->bIfWaitLoginTime == true || tableItem->pPlayers[i]->cDisconnectType == 1)
					{
						AutoSendCards(tableItem->pPlayers[i]->iUserID);
					}
					if (iWill == 0)
						return;
				}
			}
		}
		return;
	}

	if (tableItem->iPoChanNum >= 3)
	{
		_log(_DEBUG, "XLMJGameLogic", "HandleGang_Log() Bu Gang: tableItem->iPoChanNum = %d", tableItem->iPoChanNum);
		GameEndHu(nodePlayers, tableItem, false, nodePlayers->iSpecialFlag);		// 补杠阶段，导致破产玩家数量大于等于3
		return;
	}
	SendCardToPlayer(nodePlayers, tableItem);
}

// 处理暗杠HZXLMJ_GAME_INFO_MSG
void HZXLGameLogic::HandleAnGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, SpecialCardsReqDef *pReq)
{
	int iRateMoney[PLAYER_NUM] = { 0 };		// 抽水和桌费HZXLMJ_GAME_INFO_MSG
	int iPunishMoney[PLAYER_NUM] = { 0 };	// 掉线惩罚，注意必须是正值
	char cCard = pReq->cFirstCard;

	nodePlayers->iKickOutTime = 0;

	MJGCPGDef cpgInfo;
	memset(&cpgInfo, 0, sizeof(MJGCPGDef));
	cpgInfo.cCard[0] = cCard;
	cpgInfo.cCard[1] = cCard;
	cpgInfo.cChiValue = cCard;
	cpgInfo.cGangType = 2;
	cpgInfo.cTagNum = -1;
	cpgInfo.iCPGType = 2;

	// 记录杠牌信息
	for (int j = 0; j < PLAYER_NUM; j++)
	{
		if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->cPoChan == 0)
		{
			if (tableItem->pPlayers[j] != nodePlayers)
			{
				cpgInfo.cGangTag |= 1 << j;
			}
		}
	}
	nodePlayers->CPGInfo.Info[nodePlayers->CPGInfo.iAllNum] = cpgInfo;
	nodePlayers->CPGInfo.iAllNum++;

	//记录到玩家手中
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iType = cCard >> 4;
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iValue = cCard & 0xf;
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].bOther = false;
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iGangType = 2;
	nodePlayers->resultType.iPengNum++;

	//从玩家手中移除四张牌
	for (int k = 0; k<4; k++)
	{
		for (int i = 0; i<nodePlayers->iHandCardsNum; i++)
		{
			if (nodePlayers->cHandCards[i] == cCard)
			{
				for (int j = i; j<nodePlayers->iHandCardsNum - 1; j++)
				{
					nodePlayers->cHandCards[j] = nodePlayers->cHandCards[j + 1];
				}
				nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1] = 0;
				nodePlayers->iHandCardsNum--;
				break;
			}
		}
	}
	_log(_DEBUG, "HZXLGameLogic", "HandleAnGang_log() AnGang R[%d] T[%d]  UserID = %d  nickName = %s , iTablePos =%d , iTag = %d  cCard = %d",
		nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, nodePlayers->szNickName, nodePlayers->cTableNumExtra, tableItem->iCurSendPlayer, tableItem->cTableCard);

	// 暗杠消息
	SpecialCardsNoticeDef msgNotic;
	memset(&msgNotic, 0, sizeof(SpecialCardsNoticeDef));
	msgNotic.iSpecialFlag = 12;
	msgNotic.cTableNumExtra = nodePlayers->cTableNumExtra;
	msgNotic.cTagNumExtra = -1;
	msgNotic.cCards[0] = cCard;
	msgNotic.cCards[1] = cCard;
	msgNotic.cCards[2] = cCard;
	
	
	SpecialHuNoticeDef pMsgHu;
	memset(&pMsgHu, 0, sizeof(SpecialHuNoticeDef));
	pMsgHu.iZhuangPos = tableItem->iZhuangPos;

	PoChanCardNoticeMsgDef pMsgPoChan;
	memset(&pMsgPoChan, 0, sizeof(PoChanCardNoticeMsgDef));
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		pMsgHu.cHuNumExtra[i] = -1;
		pMsgHu.cPoChan[i] = -1;
	}

	int viReslutGangFen[4][4] = { { 0 },{ 0 },{ 0 },{ 0 } };			// 四个玩家根据模拟分得到的杠分
	memset(viReslutGangFen, 0, sizeof(viReslutGangFen));
	char vcPoChan[4][4];													// 四个玩家的破产状态
	memset(vcPoChan, 0, sizeof(vcPoChan));

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iBaseGangFen = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;			// 基础杠分
			long long  iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费

			for (int j = 0; j < PLAYER_NUM; j++)			// 玩家i视角下，每个玩家的输赢分
			{
				int iResultGangFen = iBaseGangFen * 2;

				if (j == nodePlayers->cTableNumExtra || tableItem->pPlayers[j]->cPoChan == 1)		// 赢杠玩家跳过,破产玩家也不用计算
					continue;

				int iTempMoney = 0;
				if (j == i)
				{
					iTempMoney = tableItem->pPlayers[i]->iMoney + tableItem->pPlayers[i]->iCurWinMoney - iCurTableMoney;
					//_log(_DEBUG, "HZXLGameLogic", "HandleAnGang 1111 [%d][%d][%d][%d]", iTempMoney, tableItem->pPlayers[i]->iMoney, tableItem->pPlayers[i]->iCurWinMoney, iCurTableMoney);
				}
					
				else
				{
					iTempMoney = tableItem->pPlayers[i]->otherMoney[j].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[j] - iCurTableMoney;
					//_log(_DEBUG, "HZXLGameLogic", "HandleAnGang 2222 [%lld][%lld][%lld][%lld]", iTempMoney, tableItem->pPlayers[i]->otherMoney[j].llMoney, tableItem->pPlayers[i]->iAllUserCurWinMoney[j], iCurTableMoney);
				}
				if (iTempMoney < iResultGangFen)		// 金币不够扣杠分，破产
				{
					//_log(_DEBUG, "HZXLGameLogic", "HandleAnGang 3333");
					iResultGangFen = iTempMoney;
					vcPoChan[i][j] = 1;
				}
				viReslutGangFen[i][j] = 0 - iResultGangFen;
				viReslutGangFen[i][nodePlayers->cTableNumExtra] += iResultGangFen;
			}
		}
	}

	// 保存玩家杠分
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			for (int j = 0; j < PLAYER_NUM; j++)	// 玩家i视角下，每个玩家的杠分
			{
				if (i == j)
				{
					tableItem->pPlayers[i]->iCurWinMoney += viReslutGangFen[i][j];
					tableItem->pPlayers[i]->iGangFen += viReslutGangFen[i][j];
				}
				else
				{
					tableItem->pPlayers[i]->iAllUserCurWinMoney[j] += viReslutGangFen[i][j];
				}
			}
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			_log(_DEBUG, "HZXLMJGameLogic", "HandleAnGang_Log() UID[%d], pPlayers[%d]->iCurWinMoney = %d", tableItem->pPlayers[i]->iUserID, i, tableItem->pPlayers[i]->iCurWinMoney);
		}
	}

	// 根据玩家杠分和破产信息，发送至客户端
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		memset(msgNotic.iGangFen, 0, sizeof(msgNotic.iGangFen));
		memset(pMsgHu.iMoneyResult, 0, sizeof(pMsgHu.iMoneyResult));
		memset(pMsgHu.iMoney, 0, sizeof(pMsgHu.iMoney));
		memset(pMsgHu.viCurWinMoney, 0, sizeof(pMsgHu.viCurWinMoney));
		memset(pMsgPoChan.cHandCards, 0, sizeof(pMsgPoChan.cHandCards));
		memset(pMsgPoChan.cpgInfo, 0, sizeof(pMsgPoChan.cpgInfo));
		msgNotic.cHaveHuNotice = 0;

		int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
		long long iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费
		pMsgHu.iTableMoney = iCurTableMoney;

		bool bExistPoChan = false;
		for (int j = 0; j < PLAYER_NUM; j++)
		{
			msgNotic.iGangFen[j] = viReslutGangFen[i][j];

			if(i == j)
				pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iCurWinMoney;
			else
				pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iAllUserCurWinMoney[j];

			if (vcPoChan[i][j] == 1)		// 玩家i视角下，玩家j破产
			{
				pMsgHu.cPoChan[j] = 1;
				memcpy(pMsgPoChan.cHandCards[j], tableItem->pPlayers[j]->cHandCards, sizeof(tableItem->pPlayers[j]->cHandCards));
				memcpy(&pMsgPoChan.cpgInfo[j], &tableItem->pPlayers[j]->CPGInfo, sizeof(tableItem->pPlayers[j]->CPGInfo));
				bExistPoChan = true;
			}
		}

		if (bExistPoChan)
		{
			pMsgPoChan.iTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsgPoChan, MJG_PLAYER_POCHAN_MSG, sizeof(PoChanCardNoticeMsgDef));	// 暗杠后破产

			msgNotic.cHaveHuNotice = 1;
			char *cMsgBuffer = new char[sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef)];
			memset(cMsgBuffer, 0, sizeof(cMsgBuffer));
			memcpy(cMsgBuffer, &msgNotic, sizeof(SpecialCardsNoticeDef));
			memcpy(cMsgBuffer + sizeof(SpecialCardsNoticeDef), &pMsgHu, sizeof(SpecialHuNoticeDef));

			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, (void *)cMsgBuffer, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef));	// 补杠后破产
			delete[] cMsgBuffer;
		}
		else
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotic, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef));		// 暗杠
	}

	// 如果有玩家破产，该玩家计费并结束游戏
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (vcPoChan[i][i] == 1)				// 玩家i视角下，自己破产
		{
			tableItem->iPoChanNum++;
			tableItem->pPlayers[i]->cPoChan = 1;
			GameUserAccountReqDef msgRD;						//ACCOUNT_REQ_RADIUS_MSG	向Radius发送计分更改请求
			memset(&msgRD, 0, sizeof(GameUserAccountReqDef));
			msgRD.iUserID = htonl(tableItem->pPlayers[i]->iUserID);
			msgRD.iGameID = htonl(m_pServerBaseInfo->iGameID);
			GameUserAccountGameDef msgGameRD;
			memset(&msgRD, 0, sizeof(GameUserAccountGameDef));
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			msgGameRD.llTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;
			SetAccountReqRadiusDef(&msgRD, &msgGameRD,tableItem, tableItem->pPlayers[i], tableItem->pPlayers[i]->iCurWinMoney, iPunishMoney[i], iRateMoney[i]);	// 暗杠后破产
			memcpy(&tableItem->pPlayers[i]->msgRDTemp, &msgRD, sizeof(GameUserAccountReqDef));
			tableItem->pPlayers[i]->iStatus = PS_WAIT_RESERVE_TWO;		// 暗杠后破产
		}
	}

	// 记录暗杠流水
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		GameLiuShuiInfoDef TempLs;
		memset(&TempLs, 0, sizeof(GameLiuShuiInfoDef));
		TempLs.cWinner |= (1 << nodePlayers->cTableNumExtra);
		TempLs.iSpecialFlag = WIN_XIA_YU;
		TempLs.cLoser = 0;
		TempLs.iFan = 2;
		TempLs.cPoChan = 0;
		TempLs.iWinMoney = 0;
		memset(TempLs.iMoney, 0, sizeof(TempLs.iMoney));

		for (int j = 0; j < PLAYER_NUM; j++)
		{
			if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->cPoChan == 0 && j != nodePlayers->cTableNumExtra)
			{
				TempLs.cLoser |= (1 << j);
				if (tableItem->pPlayers[i]->cPoChan == 1)
					TempLs.cPoChan |= (1 << j);
			}
		}
		TempLs.iWinMoney = viReslutGangFen[i][i];
		tableItem->vLiuShuiList[i].push_back(TempLs);
	}

	if (tableItem->iPoChanNum >= 3)
	{
		_log(_DEBUG, "HZXLMJGameLogic", "HandleGang_Log() An Gang: tableItem->iPoChanNum = %d", tableItem->iPoChanNum);
		GameEndHu(nodePlayers, tableItem, false, nodePlayers->iSpecialFlag);	// 暗杠阶段，导致破产玩家数量大于等于3
		return;
	}
	SendCardToPlayer(nodePlayers, tableItem);
}

// 处理明杠
void HZXLGameLogic::HandleMingGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, SpecialCardsReqDef *pReq)
{
	long long iRateMoney[PLAYER_NUM] = { 0 };//抽水和桌费
	long long iPunishMoney[PLAYER_NUM] = { 0 };//掉线惩罚，注意必须是正值

	nodePlayers->iKickOutTime = 0;

	//判断玩家是否在一炮多响里面点的杠
	if (tableItem->iWillHuNum + tableItem->iAllHuNum > 1 && nodePlayers->iSpecialFlag >> 5)
	{
		//如果是第一个点的 记录玩家当前杠牌状态数据 
		if (tableItem->iWillHuNum > 1)
		{
			nodePlayers->iSpecialFlag = 4;					//把之前的杠存下来 防止弃胡 
			memset(&nodePlayers->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
			nodePlayers->pTempCPG.cFirstCard = pReq->cFirstCard;
			memcpy(nodePlayers->pTempCPG.cCards, pReq->cCards, sizeof(pReq->cCards));
			nodePlayers->pTempCPG.iSpecialFlag = nodePlayers->iSpecialFlag;
			tableItem->iWillHuNum--;
			nodePlayers->iWinFlag = 0;
			nodePlayers->iHuFlag = 0;
			return;
		}
		else
		{
			// 能杠和胡，选择了杠，视为弃胡
			nodePlayers->iSpecialFlag = 4;
			nodePlayers->iWinFlag = 0;
			nodePlayers->iHuFlag = 0;
			if (tableItem->iWillHuNum != 0)
				tableItem->iWillHuNum--;

			nodePlayers->iStatus = PS_WAIT_SEND;

			if (tableItem->iWillHuNum)
				return;
			if (tableItem->iAllHuNum > 0)		// 处理能胡的玩家
			{
				for (int i = 0; i< PLAYER_NUM; i++)
				{
					int iTableExtra = (nodePlayers->cTableNumExtra + i) % PLAYER_NUM;
					if (tableItem->pPlayers[iTableExtra])
					{
						if (tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 5)
						{
							SpecialCardsReqDef pMsg;
							memset(&pMsg, 0, sizeof(SpecialCardsReqDef));
							pMsg.iSpecialFlag = 0x0020;
							pMsg.cFirstCard = tableItem->cTableCard;
							HandleHu(tableItem->pPlayers[iTableExtra], tableItem, &pMsg);
							return;
						}
					}
				}
			}
		}
	}

	//如果都没有 那么就将玩家的状态清空 走杠牌逻辑
	memset(&nodePlayers->pTempCPG, 0, sizeof(MJGTempCPGReqDef));

	MJGCPGDef cpgInfo;
	memset(&cpgInfo, 0, sizeof(MJGCPGDef));
	cpgInfo.cCard[0] = pReq->cCards[0];
	cpgInfo.cCard[1] = pReq->cCards[1];
	cpgInfo.cChiValue = pReq->cFirstCard;
	cpgInfo.cGangType = 0;							// 明杠
	cpgInfo.cTagNum = tableItem->iCurSendPlayer;
	cpgInfo.iCPGType = 2;
	cpgInfo.cGangTag = tableItem->iCurSendPlayer;
	nodePlayers->CPGInfo.Info[nodePlayers->CPGInfo.iAllNum] = cpgInfo;
	nodePlayers->CPGInfo.iAllNum++;

	//记录到玩家手中
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iType = tableItem->cTableCard >> 4;
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iValue = tableItem->cTableCard & 0xf;
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].bOther = true;
	nodePlayers->resultType.pengType[nodePlayers->resultType.iPengNum].iGangType = 1;
	nodePlayers->resultType.iPengNum++;

	//找到吃牌倒下的那两张牌从玩家竖立的牌的数组中删除
	for (int k = 0; k<3; k++)
	{
		for (int i = 0; i<nodePlayers->iHandCardsNum; i++)
		{
			if (nodePlayers->cHandCards[i] == tableItem->cTableCard)
			{
				for (int j = i; j<nodePlayers->iHandCardsNum - 1; j++)
				{
					nodePlayers->cHandCards[j] = nodePlayers->cHandCards[j + 1];
				}
				nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1] = 0;
				nodePlayers->iHandCardsNum--;
				break;
			}
		}
	}
	//将吃的那张牌在出牌的数组中清除了  
	tableItem->pPlayers[tableItem->iCurSendPlayer]->cSendCards[tableItem->pPlayers[tableItem->iCurSendPlayer]->iAllSendCardsNum - 1] = 0;
	tableItem->pPlayers[tableItem->iCurSendPlayer]->iAllSendCardsNum--;

	//向玩家发送明杠牌的通知
	SpecialCardsNoticeDef msgNotic;
	memset(&msgNotic, 0, sizeof(SpecialCardsNoticeDef));
	msgNotic.iSpecialFlag = 4;
	msgNotic.cTableNumExtra = nodePlayers->cTableNumExtra;
	msgNotic.cTagNumExtra = tableItem->iCurSendPlayer;
	msgNotic.cCards[0] = tableItem->cTableCard;
	msgNotic.cCards[1] = tableItem->cTableCard;
	msgNotic.cCards[2] = tableItem->cTableCard;
	

	SpecialHuNoticeDef pMsgHu;
	memset(&pMsgHu, 0, sizeof(SpecialHuNoticeDef));
	pMsgHu.iZhuangPos = tableItem->iZhuangPos;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		pMsgHu.cHuNumExtra[i] = -1;
		pMsgHu.cPoChan[i] = -1;
	}
	PoChanCardNoticeMsgDef pMsgPoChan;
	memset(&pMsgPoChan, 0, sizeof(PoChanCardNoticeMsgDef));

	long long viReslutGangFen[4][4] = { { 0 },{ 0 },{ 0 },{ 0 } };			// 四个玩家根据模拟分得到的杠分
	memset(viReslutGangFen, 0, sizeof(viReslutGangFen));
	char vcPoChan[4];													// 四个玩家的破产状态
	memset(vcPoChan, 0, sizeof(vcPoChan));

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long  iBaseGangFen = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;		// 基础杠分
			long long iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费

			long long iResultGangFen = iBaseGangFen * 2;
			if (tableItem->pPlayers[i]->cTableNumExtra == tableItem->iCurSendPlayer)	// 输分玩家，使用自己的真实金币
			{
				long long iTempMoney = tableItem->pPlayers[i]->iMoney + tableItem->pPlayers[i]->iCurWinMoney - iCurTableMoney;
				//_log(_DEBUG, "HZXLGameLogic", "HandleMingGang 1111 [%lld][%lld][%lld][%lld]", iTempMoney, tableItem->pPlayers[i]->iMoney, tableItem->pPlayers[i]->iCurWinMoney, iCurTableMoney);
				if (iTempMoney < iResultGangFen)		// 金币不够扣杠分
				{
					//_log(_DEBUG, "HZXLGameLogic", "HandleMingGang 2222");
					iResultGangFen = iTempMoney;
					vcPoChan[i] = 1;
				}

				viReslutGangFen[i][tableItem->iCurSendPlayer] = -iResultGangFen;
				viReslutGangFen[i][nodePlayers->cTableNumExtra] = iResultGangFen;
			}
			else     // 非放杠玩家，使用自己视角下放杠玩家的金币
			{
				long long iTempMoney = tableItem->pPlayers[i]->otherMoney[tableItem->iCurSendPlayer].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[tableItem->iCurSendPlayer] - iCurTableMoney;
				//_log(_DEBUG, "HZXLGameLogic", "HandleMingGang 3333 [%lld][%d][%d][%d]", iTempMoney, tableItem->pPlayers[i]->otherMoney[tableItem->iCurSendPlayer], tableItem->pPlayers[i]->iAllUserCurWinMoney[tableItem->iCurSendPlayer], iCurTableMoney);
				if (iTempMoney < iResultGangFen)		// 金币不够扣杠分
				{
					//_log(_DEBUG, "HZXLGameLogic", "HandleMingGang 4444");
					iResultGangFen = iTempMoney;
					vcPoChan[tableItem->iCurSendPlayer] = 1;
				}

				viReslutGangFen[i][tableItem->iCurSendPlayer] = -iResultGangFen;		// 玩家i视角下放杠玩家扣的金币
				viReslutGangFen[i][nodePlayers->cTableNumExtra] = iResultGangFen;		// 玩家i视角下赢杠玩家赢的金币
			}
		}
	}

	// 更新玩家金币
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (j == i)
				{
					tableItem->pPlayers[i]->iCurWinMoney += viReslutGangFen[i][j];
					tableItem->pPlayers[i]->iGangFen += viReslutGangFen[i][j];
				}
				else
				{
					tableItem->pPlayers[i]->iAllUserCurWinMoney[j] += viReslutGangFen[i][j];
				}
			}
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			_log(_DEBUG, "HZXLMJGameLogic", "HandleMingGang_Log()UID[%d] pPlayers[%d]->iCurWinMoney = %d", tableItem->pPlayers[i]->iUserID, i, tableItem->pPlayers[i]->iCurWinMoney);
		}
	}

	// 根据玩家杠分和破产信息，发送至客户端
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			memset(msgNotic.iGangFen, 0, sizeof(msgNotic.iGangFen));
			memset(pMsgHu.iMoneyResult, 0, sizeof(pMsgHu.iMoneyResult));
			memset(pMsgHu.iMoney, 0, sizeof(pMsgHu.iMoney));
			memset(pMsgHu.cPoChan, 0, sizeof(pMsgHu.cPoChan));
			memset(pMsgHu.viCurWinMoney, 0, sizeof(pMsgHu.viCurWinMoney));
			memset(pMsgPoChan.cHandCards, 0, sizeof(pMsgPoChan.cHandCards));
			memset(pMsgPoChan.cpgInfo, 0, sizeof(pMsgPoChan.cpgInfo));

			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;		// 桌费
			pMsgHu.iTableMoney = iCurTableMoney;

			for (int j = 0; j < PLAYER_NUM; j++)
			{
				msgNotic.iGangFen[j] = viReslutGangFen[i][j];
				pMsgHu.iMoneyResult[i] = viReslutGangFen[i][j];
				if (i == j)
				{
					pMsgHu.iMoney[i] = tableItem->pPlayers[i]->iMoney;
					pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iCurWinMoney;
				}
				else
				{
					pMsgHu.iMoney[i] = tableItem->pPlayers[i]->otherMoney[j].llMoney;
					pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iAllUserCurWinMoney[j];
				}
			}

			if (vcPoChan[i] == 1)		// 玩家破产
			{
				tableItem->iPoChanNum++;
				tableItem->pPlayers[i]->cPoChan = 1;
				tableItem->pPlayers[i]->iStatus = PS_WAIT_RESERVE_TWO;		// 补杠后破产

				memcpy(pMsgPoChan.cHandCards[i], tableItem->pPlayers[i]->cHandCards, sizeof(tableItem->pPlayers[i]->cHandCards));
				memcpy(&pMsgPoChan.cpgInfo[i], &tableItem->pPlayers[i]->CPGInfo, sizeof(tableItem->pPlayers[i]->CPGInfo));

				pMsgPoChan.iTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsgPoChan, MJG_PLAYER_POCHAN_MSG, sizeof(PoChanCardNoticeMsgDef));	// 明杠后破产

				GameUserAccountReqDef msgRD;						//ACCOUNT_REQ_RADIUS_MSG	向Radius发送计分更改请求
				memset(&msgRD, 0, sizeof(GameUserAccountReqDef));
				msgRD.iUserID = htonl(tableItem->pPlayers[i]->iUserID);
				msgRD.iGameID = htonl(m_pServerBaseInfo->iGameID);
				GameUserAccountGameDef msgGameRD;
				memset(&msgGameRD,0,sizeof(GameUserAccountReqDef));
				msgGameRD.llTableMoney = iCurTableMoney;
				SetAccountReqRadiusDef(&msgRD,&msgGameRD, tableItem, tableItem->pPlayers[i], tableItem->pPlayers[i]->iCurWinMoney, iPunishMoney[i], iRateMoney[i]);	// 补杠后破产
				memcpy(&tableItem->pPlayers[i]->msgRDTemp, &msgRD, sizeof(GameUserAccountReqDef));

				msgNotic.cHaveHuNotice = 1;
				char *cMsgBuffer = new char[sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef)];
				memset(cMsgBuffer, 0, sizeof(cMsgBuffer));
				memcpy(cMsgBuffer, &msgNotic, sizeof(SpecialCardsNoticeDef));
				memcpy(cMsgBuffer + sizeof(SpecialCardsNoticeDef), &pMsgHu, sizeof(SpecialHuNoticeDef));

				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, (void *)cMsgBuffer, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef));	// 补杠后破产
				delete[] cMsgBuffer;
			}
			else
			{
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotic, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef));		// 补杠
			}
		}
	}

	// 记录明杠流水
	GameLiuShuiInfoDef TempLs;
	memset(&TempLs, 0, sizeof(GameLiuShuiInfoDef));
	TempLs.cWinner |= (1 << nodePlayers->cTableNumExtra);
	TempLs.cLoser |= (1 << tableItem->iCurSendPlayer);
	TempLs.iFan = 2;
	TempLs.iSpecialFlag = WIN_GUA_FENG;			// 补杠 0x0008
	TempLs.iWinMoney = 0;
	if (tableItem->pPlayers[tableItem->iCurSendPlayer]->cPoChan == 1)
		TempLs.cPoChan |= (1 << tableItem->iCurSendPlayer);

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		//TempLs.iWinMoney = viReslutGangFen[i][nodePlayers->cTableNumExtra];
		TempLs.iWinMoney = viReslutGangFen[i][i];
		tableItem->vLiuShuiList[i].push_back(TempLs);
	}

	// 清掉桌面上出的牌
	tableItem->cTableCard = 0;
	tableItem->iCurSendPlayer = -1;
	tableItem->iCurMoPlayer = nodePlayers->cTableNumExtra;

	//流程结束掉 那就清除所有玩家的状态
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->cPoChan == 0)
		{
			memset(&tableItem->pPlayers[i]->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
		}
	}
	if (tableItem->iPoChanNum >= 3)
	{
		_log(_DEBUG, "XLMJGameLogic", "HandleGang_Log() Ming Gang: tableItem->iPoChanNum = %d", tableItem->iPoChanNum);
		GameEndHu(nodePlayers, tableItem, false, nodePlayers->iSpecialFlag);	// 明杠阶段，导致破产玩家数量大于等于3
		return;
	}
	SendCardToPlayer(nodePlayers, tableItem);
}

void HZXLGameLogic::HandleQi(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, void * pMsgData)
{
	//如果当前是摸牌的玩家点的弃  那么直接返回
	if (tableItem->iCurMoPlayer == nodePlayers->cTableNumExtra)
	{
		if (tableItem->bIfExistAIPlayer && nodePlayers->iPlayerType != NORMAL_PLAYER)
		{
			//如果是AI玩家选择弃，调一次出牌
			nodePlayers->iStatus = PS_WAIT_SEND;
			m_pHzxlAICTL->CreateAISendCardReq(tableItem, nodePlayers);
		}
		if (nodePlayers->iSpecialFlag >> 6 == 1)
		{
			_log(_DEBUG, "HZXLGameLogic", "HandleQi R[%d] T[%d] 自摸胡牌放弃  userid =%d , tableExtra =%d, nickName = %s",
				nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, nodePlayers->cTableNumExtra, nodePlayers->szNickName);
			//nodePlayers->bIsHu = false;
			nodePlayers->iSpecialFlag = 0;
			nodePlayers->iWinFlag = 0;
			nodePlayers->iHuFlag = 0;
			tableItem->iAllHuNum = 0;
			tableItem->iWillHuNum = 0;
			//SendCardToPlayer(nodePlayers, tableItem, true);
		}
		return;
	}

	if (nodePlayers->iStatus != PS_WAIT_SPECIAL)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleQi R[%d] T[%d] :[%s][%d][%d]当期玩家不可以放弃", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iStatus);
		SendKickOutWithMsg(nodePlayers, "当期玩家不可以放弃", 1301, 1);
		return;
	}

	//如果玩家胡牌为true 但是点了弃按钮
	if (nodePlayers->iSpecialFlag >> 5)
	{
		//nodePlayers->bIsHu = false;
		if (nodePlayers->bCanQiangGangHu)		// 玩家可以抢杠胡，但是没胡，不允许再次抢杠胡
		{
			nodePlayers->bGuoHu = true;			// 自摸胡牌弃后，过胡标志
			nodePlayers->bCanQiangGangHu = false;
		}
		else
		{
			nodePlayers->bGuoHu = false;
		}
		nodePlayers->iSpecialFlag = 0;
		nodePlayers->iWinFlag = 0;
		nodePlayers->iHuFlag = 0;
		if (tableItem->iWillHuNum != 0)
			tableItem->iWillHuNum--;
	}

	nodePlayers->iSpecialFlag = 0;
	nodePlayers->iStatus = PS_WAIT_SEND;
	nodePlayers->iKickOutTime = 0;

	if (tableItem->iWillHuNum)
		return;
	if (tableItem->iAllHuNum > 0)
	{
		for (int i = 0; i< PLAYER_NUM; i++)
		{
			int iTableExtra = (nodePlayers->cTableNumExtra + i) % PLAYER_NUM;
			if (tableItem->pPlayers[iTableExtra])
			{
				if (tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 5 && tableItem->pPlayers[iTableExtra]->cPoChan == false)
				{
					SpecialCardsReqDef pMsg;
					memset(&pMsg, 0, sizeof(SpecialCardsReqDef));
					pMsg.iSpecialFlag = 0x0020;
					pMsg.cFirstCard = tableItem->cTableCard;
					HandleHu(tableItem->pPlayers[iTableExtra], tableItem, &pMsg);			// 一炮多响，其他玩家点了弃胡，给点胡的玩家判断胡牌
					return;
				}
			}
		}
	}
	//有没有可以 碰 杠的玩家

	for (int i = 1; i < PLAYER_NUM; i++)
	{
		int iTableExtra = (nodePlayers->cTableNumExtra + i);

		if (iTableExtra >= PLAYER_NUM)
			iTableExtra = iTableExtra - PLAYER_NUM;
		if (tableItem->pPlayers[iTableExtra] && tableItem->pPlayers[iTableExtra]->cPoChan == 0)
		{
			if (!tableItem->bIfExistAIPlayer | (tableItem->bIfExistAIPlayer && tableItem->pPlayers[iTableExtra]->iPlayerType == NORMAL_PLAYER))
			{
				int iFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 1;
				if (iFlag > 0)
				{
					_log(_DEBUG, "HZXLGameLogic", "HandleQi: R[%d] T[%d] :[%s][%d][%d]放弃还有碰", nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iStatus);
					//如果玩家之前就碰杠过 那就直接发送
					if (tableItem->pPlayers[iTableExtra]->pTempCPG.iSpecialFlag > 0)
					{
						SpecialCardsReqDef pMsg;
						memset(&pMsg,0,sizeof(SpecialCardsReqDef));
						pMsg.iSpecialFlag = tableItem->pPlayers[iTableExtra]->pTempCPG.iSpecialFlag;
						pMsg.cFirstCard = tableItem->pPlayers[iTableExtra]->pTempCPG.cFirstCard;
						memcpy(pMsg.cCards, tableItem->pPlayers[iTableExtra]->pTempCPG.cCards,sizeof(tableItem->pPlayers[iTableExtra]->pTempCPG.cCards));
						HandleSpecialCard(tableItem->pPlayers[iTableExtra]->iUserID,(void *)&pMsg);
						tableItem->pPlayers[iTableExtra]->pTempCPG.cBeQiangGangHu = 1;
					}
					else
					{
						tableItem->pPlayers[iTableExtra]->iStatus = PS_WAIT_SPECIAL;
						SpecialCardsServerDef pMsg;
						memset(&pMsg, 0, sizeof(SpecialCardsServerDef));
						pMsg.cTableNumExtra = iTableExtra;
						pMsg.iSpecialFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag;
						tableItem->pPlayers[iTableExtra]->iKickOutTime = KICK_OUT_TIME;
						CLSendSimpleNetMessage(tableItem->pPlayers[iTableExtra]->iSocketIndex, &pMsg, MJG_SPECIAL_CARD_SERVER, sizeof(SpecialCardsServerDef));


						//如果玩家掉线 自动帮玩家打牌
						if (tableItem->pPlayers[iTableExtra]->bIfWaitLoginTime == true || tableItem->pPlayers[iTableExtra]->cDisconnectType == 1)
						{
							AutoSendCards(tableItem->pPlayers[iTableExtra]->iUserID);
						}
					}
					return;
				}
			}
			else
			{
				if (tableItem->pPlayers[iTableExtra]->iPlayerType != NORMAL_PLAYER)
				{
					_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() 222");
					int iFlag = tableItem->pPlayers[iTableExtra]->iSpecialFlag >> 1;
					_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() 222 iFlag[%d]", iFlag);
					if (iFlag > 0)
					{
						_log(_DEBUG, "xlGameLogic", "SendSpecialServerToPlayer_Log() AI 碰杠 iFlag[%d]", iFlag);
						m_pHzxlAICTL->CreateAISpcialCardReq(tableItem, tableItem->pPlayers[iTableExtra]);
						return;
					}
				}
			}
		}
	}

	_log(_DEBUG, "HZXLGameLogic", "tableItem->iCurSendPlayer = %d",tableItem->iCurSendPlayer);
	//如果没有碰杠的玩家 那就请求下一个玩家摸牌了

	int i;
	for (i = 1; i < PLAYER_NUM;i++)
	{
		tableItem->iCurMoPlayer = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
		if (tableItem->pPlayers[tableItem->iCurMoPlayer] && tableItem->pPlayers[tableItem->iCurMoPlayer]->cPoChan == 0)
		{
			SendCardToPlayer(tableItem->pPlayers[tableItem->iCurMoPlayer], tableItem);
			return;
		}
	}
	if (i == PLAYER_NUM)
	{
_log(_ERROR, "HZXLGameLogic", "HandleQi:T[%d] 玩家tableItem->iCurMoPlayer[%d] 异常 没有找到对应玩家",  tableItem->iTableID, tableItem->iCurMoPlayer);
	}
	//if (tableItem->pPlayers[tableItem->iCurMoPlayer])
	//{
	//	SendCardToPlayer(tableItem->pPlayers[tableItem->iCurMoPlayer], tableItem);
	//}

}

void HZXLGameLogic::HandleHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, void * pMsgData)
{
	_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log(): Room[%d] T[%d]  Player [%s][%d][%d][%d][%d]",
		nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->cTableNumExtra, nodePlayers->iStatus, nodePlayers->cTableNumExtra);

	_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log(): cPoChan[%d]", nodePlayers->cPoChan);
	if (nodePlayers->cPoChan)
		return;

	SpecialCardsReqDef * pReq = (SpecialCardsReqDef *)pMsgData;
	int iGameAmount[PLAYER_NUM] = { 0 };	// 净分，不包含抽水
	int iRateMoney[PLAYER_NUM] = { 0 };		// 抽水和桌费
	int iPunishMoney[PLAYER_NUM] = { 0 };	// 掉线惩罚，注意必须是正值

	GameUserAccountReqDef msgRD[PLAYER_NUM];						//ACCOUNT_REQ_RADIUS_MSG	向Radius发`送计分更改请求
	memset(msgRD, 0, sizeof(msgRD));

	//int viRealResultMoney[4] = { 0 };		// 每个玩家真实输赢的金币
	char vcRealPoChan[4];					// 每个玩家真实破产状态
	char vcSendPoChan[4][4];				// 四个玩家视角下，出牌玩家是否破产
	memset(vcRealPoChan, 0, sizeof(vcRealPoChan));
	memset(vcSendPoChan, 0, sizeof(vcSendPoChan));

	_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log(): cTNExtra[%d],iCurMP[%d]", nodePlayers->cTableNumExtra, tableItem->iCurMoPlayer);
	// 自摸
	if (nodePlayers->cTableNumExtra == tableItem->iCurMoPlayer)
	{
		_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log(): Begin ZiMo");
		if (pReq->iSpecialFlag != nodePlayers->iSpecialFlag && pReq->iSpecialFlag >> 6 == 0)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleHu_Log(): szNickName[%s],iUserID[%d] Not ZiMo Current Player Is Not ZiMo iSpecialFlag[%d]", nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iSpecialFlag);
			SendKickOutWithMsg(nodePlayers, "当前玩家状态不是自摸", 1301, 1);
			return;
		}

		if (nodePlayers->iHuFan == 0)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleHu_Log():szNickName[%s],iUserID[%d] Current Player Cannot Hu iWinFlag[%d]", nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iWinFlag);
			SendKickOutWithMsg(nodePlayers, "当前玩家不能胡牌", 1301, 1);
			return;
		}
		_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log() nodePlayers->iHuFan[%d]", nodePlayers->iHuFan);
		for (int i = 0; i < MJ_FAN_TYPE_CNT; i++)
			if(nodePlayers->cFanResult[i] > 0)
				_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log() nodePlayers->cFanResult[%d] = %d", i, nodePlayers->cFanResult[i]);

		tableItem->iAllHuNum = tableItem->iWillHuNum;
		tableItem->iWillHuNum = 0;

		nodePlayers->cHuCard = tableItem->cCurMoCard;
		nodePlayers->vcHuCard.push_back(tableItem->cCurMoCard);
		nodePlayers->cHandCards[nodePlayers->iHandCardsNum - 1] = 0;
		nodePlayers->iHandCardsNum--;

		// 记录胡牌信息
		HuCardInfoDef info;
		memset(&info, 0, sizeof(HuCardInfoDef));
		info.cHuCard = tableItem->cCurMoCard;
		info.cHuExtra |= (1 << nodePlayers->cTableNumExtra);
		tableItem->vcAllHuCard.push_back(info);
		if (nodePlayers->bIsHu == false)			// 玩家未胡牌，保存玩家的胡牌信息
			tableItem->vcOrderHu.push_back(nodePlayers->cTableNumExtra);

		// 处理自摸胡算分相关
		HandleZiMoHu(nodePlayers, tableItem, vcRealPoChan);

		if (m_bOpenRecharge)
		{
			if (tableItem->cCurMoCard == 65)		// 考虑多次自摸红中胡
			{
				nodePlayers->iZiMoHuHongZhongCnt++;
			}
			if (nodePlayers->iContinousZiMoCnt >= 0)		// 考虑连续3次自摸
			{
				nodePlayers->iContinousZiMoCnt++;
			}
			AutoSendMagicExpress(nodePlayers, tableItem, -1);
		}

		//for (int i = 1; i < PLAYER_NUM; i++)
		//{
		//	char cToExtra = (nodePlayers->cTableNumExtra + i) % PLAYER_NUM;
		//	if (tableItem->pPlayers[cToExtra])
		//	{
		//		int iToUserID = tableItem->pPlayers[cToExtra]->iUserID;
		//		int iPropIdCheers = 3008;
		//		SendMagicExpress(tableItem, nodePlayers->iUserID, iToUserID, iPropIdCheers);
		//	}
		//}
	}
	// 点炮
	else
	{
		_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log(): Begin DianPao");
		if (nodePlayers->iStatus != PS_WAIT_SPECIAL)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleHu_Log():DianPao szNickName[%s], iUserID[%d], iStatus[%d] Player Abnormal 当前玩家状态异常", nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iStatus);
			SendKickOutWithMsg(nodePlayers, "玩家不可以胡", 1301, 1);
			return;
		}
		if (pReq->iSpecialFlag != nodePlayers->iSpecialFlag && pReq->iSpecialFlag >> 5 == 0)
		{
			_log(_ERROR, "HZXLGameLogic", "HandleHu_Log():DianPao [%s][%d] Not Hu 当前玩家状态不是胡牌 iSpecialFlag[%d]", nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iSpecialFlag);
			SendKickOutWithMsg(nodePlayers, "当前玩家状态不是胡牌", 1301, 1);
			return;
		}
		if (nodePlayers->iHuFan == 0)
		{
			_log(_ERROR, "HBGameLogic", "HandleHu_Log():DianPao [%s][%d] Cannot Hu 当前玩家不能胡牌 iWinFlag[%d]", nodePlayers->szNickName, nodePlayers->iUserID, nodePlayers->iWinFlag);
			SendKickOutWithMsg(nodePlayers, "当前玩家不能胡牌", 1301, 1);
			return;
		}

		_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log(): DianPao iWillHuNum[%d]", tableItem->iWillHuNum);
		if (tableItem->iWillHuNum > 0)
		{
			tableItem->iWillHuNum--;
			tableItem->iAllHuNum++;
		}
		if (tableItem->iWillHuNum > 0)
			return;

		_log(_DEBUG, "HZXLGameLogic", "HandlHu  QiangGangHu nodePlayers->cFanResult[MJ_QIANG_GANG_HU] = %d", nodePlayers->cFanResult[MJ_QIANG_GANG_HU]);
		// 抢杠胡
		bool bNeedPlayQGHAni = false;
		if (nodePlayers->cFanResult[MJ_QIANG_GANG_HU] > 0)
		{
			_log(_DEBUG, "HZXLGameLogic", "HandlHu  QiangGangHu cTableNumExtra[%d] iCurSendPlayer[%d], cTableCard[%d], iHandCardsNum[%d]",
				nodePlayers->cTableNumExtra, tableItem->iCurSendPlayer, tableItem->cTableCard, tableItem->pPlayers[tableItem->iCurSendPlayer]->iHandCardsNum);
			for (int i = 0; i < tableItem->pPlayers[tableItem->iCurSendPlayer]->iHandCardsNum; i++)
			{
				if (tableItem->pPlayers[tableItem->iCurSendPlayer]->cHandCards[i] == tableItem->cTableCard)			// 打出被杠/胡那张牌的玩家，手牌要删除打出的牌
				{
					for (int j = i; j < tableItem->pPlayers[tableItem->iCurSendPlayer]->iHandCardsNum - 1; j++)
					{
						tableItem->pPlayers[tableItem->iCurSendPlayer]->cHandCards[j] = tableItem->pPlayers[tableItem->iCurSendPlayer]->cHandCards[j + 1];
					}
					tableItem->pPlayers[tableItem->iCurSendPlayer]->cHandCards[tableItem->pPlayers[tableItem->iCurSendPlayer]->iHandCardsNum - 1] = 0;
					tableItem->pPlayers[tableItem->iCurSendPlayer]->iHandCardsNum--;
					break;
				}
			}
			//将被抢杠玩家的状态清空
			tableItem->pPlayers[tableItem->iCurSendPlayer]->iSpecialFlag = 0;
			memset(&tableItem->pPlayers[tableItem->iCurSendPlayer]->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
			tableItem->pPlayers[tableItem->iCurSendPlayer]->iHuFan = 0;
			memset(tableItem->pPlayers[tableItem->iCurSendPlayer]->cFanResult, 0, sizeof(tableItem->pPlayers[tableItem->iCurSendPlayer]->cFanResult));
			bNeedPlayQGHAni = true;
		}
		else
		{
			tableItem->pPlayers[tableItem->iCurSendPlayer]->cSendCards[tableItem->pPlayers[tableItem->iCurSendPlayer]->iAllSendCardsNum - 1] = 0;
			tableItem->pPlayers[tableItem->iCurSendPlayer]->iAllSendCardsNum--;
		}

		// 统计连续点炮胡
		if (m_bOpenRecharge && tableItem->pPlayers[tableItem->iCurSendPlayer])
		{
			int iUserID = tableItem->pPlayers[tableItem->iCurSendPlayer]->iUserID;
			int iToUserID = nodePlayers->iUserID;
			int cToExtra = nodePlayers->cTableNumExtra;
			bool bPlaySerialAni = false;
			if (tableItem->pPlayers[tableItem->iCurSendPlayer]->viContinousHuPosCnt[cToExtra] >= 0)
			{
				tableItem->pPlayers[tableItem->iCurSendPlayer]->viContinousHuPosCnt[cToExtra]++;
				if (tableItem->pPlayers[tableItem->iCurSendPlayer]->viContinousHuPosCnt[cToExtra] == 3)		// 连续三次打出的牌被同一个玩家胡
				{
					tableItem->pPlayers[tableItem->iCurSendPlayer]->viContinousHuPosCnt[cToExtra] = -1;

					_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log Egg iUserID[%d]iToUserID[%d]", iUserID, iToUserID);
					int iPropIdEgg = 3001;
					SendMagicExpress(tableItem, iUserID, iToUserID, iPropIdEgg);
					bPlaySerialAni = true;
				}
			}

			if (m_bOpenRecharge && !bPlaySerialAni && bNeedPlayQGHAni)
			{
				int iPropIdBomb = 3002;
				_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log Bomb iUserID[%d]iToUserID[%d]", iUserID, iToUserID);
				SendMagicExpress(tableItem, iUserID, iToUserID, iPropIdBomb);
			}

			//int iPropIdBomb = 3002;
			//SendMagicExpress(tableItem, iUserID, iToUserID, iPropIdBomb);
		}

		HuCardInfoDef info;
		memset(&info, 0, sizeof(HuCardInfoDef));
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->iSpecialFlag >> 5 && tableItem->pPlayers[i]->cPoChan == 0)
			{
				if (tableItem->pPlayers[i]->bIsHu == false)
				{
					tableItem->vcOrderHu.push_back(tableItem->pPlayers[i]->cTableNumExtra);
				}
				tableItem->pPlayers[i]->vcHuCard.push_back(tableItem->cTableCard);

				info.cHuCard = tableItem->cTableCard;
				info.cHuExtra |= (1 << tableItem->pPlayers[i]->cTableNumExtra);
			}
		}
		_log(_DEBUG, "HZXLGameLogic", "HandlHu  DianPao info.cHuCard ===%d info.cHuExtra == %d", info.cHuCard, info.cHuExtra);
		tableItem->vcAllHuCard.push_back(info);

		char vcTempPoChan[4];							// 四个玩家视角下，点炮玩家是否破产
		memset(vcTempPoChan, 0, sizeof(vcTempPoChan));
		HandleDianPaoHu(nodePlayers, tableItem, vcTempPoChan);
		if (vcTempPoChan[tableItem->iCurSendPlayer] > 0)
			vcRealPoChan[tableItem->iCurSendPlayer] = 1;
		for (int k = 0; k < 4; k++)
		{
			_log(_DEBUG, "HZXLGameLogic", "HandlHu_log() DianPao  vcTempPoChan[%d] = %d, vcRealPoChan[%d] = %d", k, vcTempPoChan[k], k, vcRealPoChan[k]);
			vcSendPoChan[k][tableItem->iCurSendPlayer] = vcTempPoChan[k];
		}
	}

	//AI相关处理
	//if (tableItem->bIfExistAIPlayer && nodePlayers->iPlayerType == NORMAL_PLAYER)
	//{
	//	int iDeputyAIPos = -1;
	//	int iAssistAIPos = -1;
	//	for (int i = 0; i < PLAYER_NUM; i++)
	//	{
	//		if (tableItem->pPlayers[i]->iPlayerType == ASSIST_AI_PLAYER)
	//		{
	//			iAssistAIPos = tableItem->pPlayers[i]->cTableNumExtra;
	//		}
	//		else if (tableItem->pPlayers[i]->iPlayerType == DEPUTY_AI_PLAYER)
	//		{
	//			iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
	//		}
	//	}
	//	
	//	bool bIfNeedReset = m_pHzxlAICTL->CTLJudegIfNeedResetHandCard(tableItem);
	//	_log(_DEBUG, "HZXLGameLogic", "HandleHu_Log() ResetAIHandCards 2222 [%d][%d]", 112 - tableItem->iCardIndex, bIfNeedReset);
	//	if (bIfNeedReset)
	//	{
	//		if (112 - tableItem->iCardIndex < 20)
	//		{
	//			m_pHzxlAICTL->ResetAssistAIHandCardFromWall(tableItem, tableItem->pPlayers[iAssistAIPos]);
	//		}
	//		else
	//		{
	//			m_pHzxlAICTL->ReSetAssistAIHandCard(tableItem, tableItem->pPlayers[iAssistAIPos]);
	//		}
	//		//m_pHzxlAICTL->ResetDeputyAIHandCard(tableItem, tableItem->pPlayers[iDeputyAIPos]);
	//	}
	//}

	// 保存玩家胡牌状态
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->iSpecialFlag >> 5 && tableItem->pPlayers[i]->bIsHu == false && tableItem->pPlayers[i]->cPoChan == 0)
			{
				tableItem->iHasHuNum++;
				tableItem->pPlayers[i]->bIsHu = true;
			}
		}
	}

	// 玩家计费
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER))
		{
			if (tableItem->pPlayers[i])
			{
				if (tableItem->pPlayers[i]->cPoChan == 0 && vcRealPoChan[i] == 1)
				{
					msgRD[i].iUserID = htonl(tableItem->pPlayers[i]->iUserID);
					msgRD[i].iGameID = htonl(m_pServerBaseInfo->iGameID);
					GameUserAccountGameDef msgGameRD;
					memset(&msgRD, 0, sizeof(GameUserAccountGameDef));
					int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
					msgGameRD.llTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;
					SetAccountReqRadiusDef(&msgRD[i], &msgGameRD,tableItem, tableItem->pPlayers[i], tableItem->pPlayers[i]->iCurWinMoney, iPunishMoney[i], iRateMoney[i]);		// 胡牌破产

					memcpy(&tableItem->pPlayers[i]->msgRDTemp, &msgRD[i], sizeof(GameUserAccountGameDef));
				}
			}
		}
	}
	// 判断任务
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER))
		{
			if (tableItem->pPlayers[i])
			{
				tableItem->pPlayers[i]->iKickOutTime = 0;
			}
		}
	}
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		_log(_DEBUG, "HZXLGameLogic", "HandlHu_log() vcRealPoChan[%d] = %d", i, vcRealPoChan[i]);
		if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER))
		{
			if (vcRealPoChan[i] == 1)
			{
				if (tableItem->pPlayers[i])
				{
					_log(_DEBUG, "HZXLGameLogic", "HandlHu_log() tableItem->pPlayers[%d]->cPoChan = %d", i, tableItem->pPlayers[i]->cPoChan);
					if (m_bOpenRecharge && !(tableItem->pPlayers[i]->bIfWaitLoginTime == true || tableItem->pPlayers[i]->cDisconnectType == 1))	// 玩家未掉线，等待充值
					{
						tableItem->pPlayers[i]->bWaitRecharge = true;
						tableItem->bExistWaitRecharge = true;
						tableItem->iWaitRechargeExtra |= 1 << i;
					}
					else
					{
						tableItem->pPlayers[i]->cPoChan = 1;
						tableItem->pPlayers[i]->iStatus = PS_WAIT_RESERVE_TWO;		// 玩家胡牌后破产，在Escape时清除掉玩家的节点
						tableItem->iPoChanNum++;
					}
				}

			}
		}
	}
	// 处理玩家胡牌后的操作
	OperateAfterHuState(nodePlayers, tableItem);
}

// 处理自摸胡牌
/*
	@param nodePlayers:自摸胡玩家
	@pram tableItem
	@return cRealPoChan:四个玩家真实破产状态
*/
void HZXLGameLogic::HandleZiMoHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char cRealPoChan[4])
{
	SpecialHuNoticeDef pMsgHu;
	memset(&pMsgHu, 0, sizeof(SpecialHuNoticeDef));
	pMsgHu.iZhuangPos = tableItem->iZhuangPos;
	int iPlayerMaxFanType = CalculateMaxFanType(nodePlayers);
	for (int t = 0; t < PLAYER_NUM; t++)
	{
		pMsgHu.cHuNumExtra[t] = -1;
	}

	
	_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() ZiMo bIfExistAIPlayer[%d]  cIfExistAI[%d]", tableItem->bIfExistAIPlayer, pMsgHu.cEmpty[0]);
	pMsgHu.cHuNumExtra[nodePlayers->cTableNumExtra] = 1; // nodePlayers->cTableNumExtra;
	pMsgHu.iMaxFanType[nodePlayers->cTableNumExtra] = iPlayerMaxFanType;
	pMsgHu.iSpecialFlag[nodePlayers->cTableNumExtra] = nodePlayers->iSpecialFlag;
	pMsgHu.iHuFan[nodePlayers->cTableNumExtra] = nodePlayers->iHuFan;
	pMsgHu.iHuFlag[nodePlayers->cTableNumExtra] = nodePlayers->iWinFlag;
	pMsgHu.iHuType[nodePlayers->cTableNumExtra] = nodePlayers->iHuFlag;
	memcpy(pMsgHu.cHuFanResult[nodePlayers->cTableNumExtra], nodePlayers->cFanResult, sizeof(nodePlayers->cFanResult));
	pMsgHu.cLastWinCard = tableItem->cCurMoCard;
	if (tableItem->iHasHuNum == 0)
		pMsgHu.iFirstHu = 1;		//第一个胡
	if (nodePlayers->bTing)
		pMsgHu.cIfTing[nodePlayers->cTableNumExtra] = 1;
	char cTingHuCard[4][28];
	int iTingHuFan[4][28];
	char cNeedUpdateTing[4];
	memset(cTingHuCard, 0, sizeof(cTingHuCard));
	memset(iTingHuFan, 0, sizeof(iTingHuFan));
	memset(cNeedUpdateTing, 0, sizeof(cNeedUpdateTing));
	CalculateHuTingInfo(tableItem, cTingHuCard, iTingHuFan, cNeedUpdateTing);
	//for(int i = 0; i < 4; i++)
	//	_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() ZiMo cNeedUpdateTing[%d] = %d", i, cNeedUpdateTing[i]);

	SpecialCardsNoticeDef pMsgSpecial;
	memset(&pMsgSpecial, 0, sizeof(SpecialCardsNoticeDef));
	pMsgSpecial.iHuFlag = nodePlayers->iWinFlag;
	pMsgSpecial.iHuType = nodePlayers->iHuFlag;
	pMsgSpecial.cTableNumExtra = nodePlayers->cTableNumExtra;
	pMsgSpecial.iSpecialFlag = 1 << 6;
	pMsgSpecial.cTagNumExtra = -1;

	StatisticAllHuFan(tableItem,nodePlayers);

	int iWinMoney[PLAYER_NUM] = { 0 };
	_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() ZiMo iUserID[%d] iHuFan[%d], cLastWinCard[%d], iPlayerMaxFanType[%d]", nodePlayers->iUserID, nodePlayers->iHuFan, pMsgHu.cLastWinCard, iPlayerMaxFanType);

	// 计算分数
	for (int i = 0; i < PLAYER_NUM; i++)				// 玩家i视角下，每个玩家的输赢分
	{
		bool bExistPoChan = false;						// 是否存在破产玩家
		char vcPoChan[4];
		int viResultMoney[4] = { 0 };					// 玩家i视角下，四个玩家的输赢分
		memset(vcPoChan, 0, sizeof(vcPoChan));
		PoChanCardNoticeMsgDef pPCInfo;
		if (tableItem->pPlayers[i])						// 玩家i视角下，计算金币
		{
			int iResultFan = nodePlayers->iHuFan;									// 赢家赢的番数(倍数)
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iBaseScore = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;				// 基础底分
			int iFengDingFan = (int)stSysConfBaseInfo.stRoom[iPlayerRoomID].iMinPoint;			// 封顶番
			long long iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费
			memset(pMsgHu.cFengDing, 0, sizeof(pMsgHu.cFengDing));
			memset(pMsgHu.iMoneyResult, 0, sizeof(pMsgHu.iMoneyResult));
			memset(pMsgHu.cTingHuCard, 0, sizeof(pMsgHu.cTingHuCard));
			memset(pMsgHu.iTingHuFan, 0, sizeof(pMsgHu.iTingHuFan));
			memset(pMsgHu.cPoChan, 0, sizeof(pMsgHu.cPoChan));
			memset(pMsgHu.viCurWinMoney, 0, sizeof(pMsgHu.viCurWinMoney));
			pMsgSpecial.cHaveHuNotice = 0;
			memset(&pPCInfo, 0, sizeof(PoChanCardNoticeMsgDef));

			pMsgHu.iTableMoney = iCurTableMoney;

			int iWinMoney = iResultFan * iBaseScore;
			_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() ZiMo i[%d], iResultFan[%d] iFengDingFan[%d], iBaseScore[%lld]", i, iResultFan, iFengDingFan, iBaseScore);
			if (iResultFan > iFengDingFan && iFengDingFan > 0)
			{
				iResultFan = iFengDingFan;
				iWinMoney = iResultFan * iBaseScore;
				for (int k = 0; k < PLAYER_NUM; k++)			// 输赢玩家都要显示封顶j
				{
					pMsgHu.cFengDing[k] = 1;
				}
			}

			for (int j = 0; j < PLAYER_NUM; j++)			// 玩家i视角下，计算每个玩家的输赢分
			{
				int iTempMoney = 0;
				if (j != nodePlayers->cTableNumExtra && tableItem->pPlayers[j]->cPoChan == 0)		// 非自摸玩家计算输分,并且非自摸玩家未破产
				{
					viResultMoney[j] = iWinMoney;
					if (j == i)
					{
						iTempMoney = tableItem->pPlayers[i]->iMoney + tableItem->pPlayers[i]->iCurWinMoney - iCurTableMoney;		// 如果是玩家i，计算携带的金币与当前赢的金币总和
						//_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() 1111 [%d][%d] [%d][%d]", iTempMoney, tableItem->pPlayers[i], tableItem->pPlayers[i]->iCurWinMoney, iCurTableMoney);
					}
					else
					{
						iTempMoney = tableItem->pPlayers[i]->otherMoney[j].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[j] - iCurTableMoney;	// 如果是其他玩家，使用虚拟的金币
						//_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() 2222 [%lld][%lld] [%lld][%lld]", iTempMoney, tableItem->pPlayers[i]->otherMoney[j].llMoney, tableItem->pPlayers[i]->iAllUserCurWinMoney[j], iCurTableMoney);
					}
					if (iWinMoney > iTempMoney)
					{
						//_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() 3333");
						viResultMoney[j] = iTempMoney;
						vcPoChan[j] = 1;
					}
					viResultMoney[nodePlayers->cTableNumExtra] += viResultMoney[j];			// 玩家i视角下，自摸玩家的赢分
				}
			}
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (j == nodePlayers->cTableNumExtra)
					pMsgHu.iMoneyResult[j] = viResultMoney[j];
				else
					pMsgHu.iMoneyResult[j] = -viResultMoney[j];
				pMsgHu.cPoChan[j] = vcPoChan[j];

				if (j == i)		// 保存玩家的真实输赢信息，用于计费
				{
					//iRealResultMoney[i] = pMsgHu.iMoneyResult[j];
					cRealPoChan[i] = pMsgHu.cPoChan[j];
				}

				if (pMsgHu.cPoChan[j] == 1)
				{
					memcpy(pPCInfo.cHandCards[j], tableItem->pPlayers[j]->cHandCards, sizeof(tableItem->pPlayers[j]->cHandCards));
					memcpy(&pPCInfo.cpgInfo[j], &tableItem->pPlayers[j]->CPGInfo, sizeof(tableItem->pPlayers[j]->CPGInfo));
					bExistPoChan = true;
				}
			}

			// 保存玩家输赢信息
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (tableItem->pPlayers[i])
				{
					if (i == j)
					{
						tableItem->pPlayers[i]->iCurWinMoney += pMsgHu.iMoneyResult[j];
						pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iCurWinMoney;
					}
					else
					{
						tableItem->pPlayers[i]->iAllUserCurWinMoney[j] += pMsgHu.iMoneyResult[j];
						pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iAllUserCurWinMoney[j];
					}
				}
			}

			if (cNeedUpdateTing[i] == 1)		// 需要更新该玩家的听牌信息
			{
				//_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() ZiMo size(cTingHuCard[%d]) = %d", i, sizeof(cTingHuCard[i]));
				for (int j = 0; j < sizeof(cTingHuCard[i]); j++)
				{
					pMsgHu.cTingHuCard[j] = cTingHuCard[i][j];
					pMsgHu.iTingHuFan[j] = iTingHuFan[i][j];
				}
				pMsgHu.vcNeedUpdateTingInfo[i] = 1;
			}
			else
			{
				pMsgHu.vcNeedUpdateTingInfo[i] = 0;
			}

			_log(_DEBUG, "HZXLGameLogic", "HandleZiMoHu_Log() ZiMo bExistPoChan[%d] i[%d]", bExistPoChan, i);

			// 给四个玩家发送胡牌消息
			if (bExistPoChan)
			{
				pPCInfo.iTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pPCInfo, MJG_PLAYER_POCHAN_MSG, sizeof(PoChanCardNoticeMsgDef));	// 自摸胡后破产

				pMsgSpecial.cHaveHuNotice = 1;
				char *cMsgBuffer = new char[sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef)];
				memset(cMsgBuffer, 0, sizeof(cMsgBuffer));
				memcpy(cMsgBuffer, &pMsgSpecial, sizeof(SpecialCardsNoticeDef));
				memcpy(cMsgBuffer + sizeof(SpecialCardsNoticeDef), &pMsgHu, sizeof(SpecialHuNoticeDef));
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, (void *)cMsgBuffer, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef));
				delete[] cMsgBuffer;
				cMsgBuffer = NULL;
			}
			else
			{
				char *cMsgBuffer = new char[sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef)];
				memset(cMsgBuffer, 0, sizeof(cMsgBuffer));
				memcpy(cMsgBuffer, &pMsgSpecial, sizeof(SpecialCardsNoticeDef));
				memcpy(cMsgBuffer + sizeof(SpecialCardsNoticeDef), &pMsgHu, sizeof(SpecialHuNoticeDef));

				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, (void *)cMsgBuffer, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef));
				delete[] cMsgBuffer;
				cMsgBuffer = NULL;
			}

			HandleZiMoHuLiuShui(nodePlayers, tableItem, pMsgHu, i);
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			_log(_DEBUG, "HZXLMJGameLogic", "HandleZiMoHu_Log()UID[%d], pPlayers[%d]->iCurWinMoney = %d", tableItem->pPlayers[i]->iUserID, i, tableItem->pPlayers[i]->iCurWinMoney);
		}
	}
}

// 记录自摸胡的流水
void HZXLGameLogic::HandleZiMoHuLiuShui(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, SpecialHuNoticeDef pMsgHu, int iPos)
{
	// 记录流水
	GameLiuShuiInfoDef TempLs;
	memset(&TempLs, 0, sizeof(GameLiuShuiInfoDef));
	TempLs.cWinner |= (1 << nodePlayers->cTableNumExtra);
	for (int j = 0; j < PLAYER_NUM; j++)
	{
		if (j != nodePlayers->cTableNumExtra && tableItem->pPlayers[j] && tableItem->pPlayers[j]->cPoChan == 0)
			TempLs.cLoser |= (1 << j);
		if (pMsgHu.cPoChan[j] == 1)
			TempLs.cPoChan |= (1 << j);
	}
	//TempLs.iWinMoney = pMsgHu.iMoneyResult[nodePlayers->cTableNumExtra];
	TempLs.iWinMoney = pMsgHu.iMoneyResult[iPos];
	TempLs.iFan = nodePlayers->iHuFan;

	TempLs.cFengDing = pMsgHu.cFengDing[nodePlayers->cTableNumExtra];
	memcpy(TempLs.cFanResult, pMsgHu.cHuFanResult[nodePlayers->cTableNumExtra], sizeof(pMsgHu.cHuFanResult[nodePlayers->cTableNumExtra]));

	tableItem->vLiuShuiList[iPos].push_back(TempLs);
}

//处理点炮胡
void HZXLGameLogic::HandleDianPaoHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char cRealPoChan[4])
{
	// 呼叫转移(杠上炮)
	long long viHJZYScore[4] = { 0 };			// 四个玩家视角下呼叫转移分数	
	int iHJZYFan = 0;					// 呼叫转移番数
	bool bHJZY = false;
	_log(_DEBUG, "HZXLGameLogic", "HandleDianPaoHu_Log() DianPao UID[%d], iWinFlag[%d], iAllHuNum[%d]", nodePlayers->iUserID, nodePlayers->iWinFlag, tableItem->iAllHuNum);
	if (tableItem->iAllHuNum == 1 && nodePlayers->iWinFlag &(1 << WIN_HU_JIAO_ZHUAN_YI))		// 只有一个玩家胡牌，并且是呼叫转移
	{
		bHJZY = true;
		HandleHJZYHu(nodePlayers, tableItem, viHJZYScore, iHJZYFan);
	}

	SpecialHuNoticeDef pMsgHu;
	memset(&pMsgHu, 0, sizeof(SpecialHuNoticeDef));
	pMsgHu.iZhuangPos = tableItem->iZhuangPos;
	pMsgHu.cLastWinCard = tableItem->cTableCard;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		pMsgHu.cHuNumExtra[i] = -1;
		pMsgHu.cPoChan[i] = 0;
	}
	
	_log(_DEBUG, "HZXLGameLogic", "HandleDianPaoHu_Log() DianPao bIfExistAIPlayer[%d]  cIfExistAI[%d]", tableItem->bIfExistAIPlayer, pMsgHu.cEmpty[0]);
	SpecialCardsNoticeDef pMsgSpecial;
	memset(&pMsgSpecial, 0, sizeof(SpecialCardsNoticeDef));
	pMsgSpecial.iHuFlag = nodePlayers->iWinFlag;
	pMsgSpecial.iHuType = nodePlayers->iHuFlag;
	pMsgSpecial.cTableNumExtra = nodePlayers->cTableNumExtra;
	pMsgSpecial.iSpecialFlag = 1 << 5;
	pMsgSpecial.cTagNumExtra = tableItem->iCurSendPlayer;
	pMsgSpecial.iWillHuNum = tableItem->iAllHuNum;

	char cTingHuCard[4][28];
	int iTingHuFan[4][28];
	char cNeedUpdateTing[4];
	memset(cTingHuCard, 0, sizeof(cTingHuCard));
	memset(iTingHuFan, 0, sizeof(iTingHuFan));
	memset(cNeedUpdateTing, 0, sizeof(cNeedUpdateTing));
	CalculateHuTingInfo(tableItem, cTingHuCard, iTingHuFan, cNeedUpdateTing);

	// 与算分无关的统一赋值
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->iSpecialFlag >> 5 && tableItem->pPlayers[i]->cPoChan == 0)			// 该玩家胡点炮
		{
			tableItem->pPlayers[i]->cHuCard = tableItem->cTableCard;
			pMsgSpecial.iHuPos[i] = 1;
			pMsgHu.cHuNumExtra[i] = 1;
			int iPlayerMaxFanType = CalculateMaxFanType(tableItem->pPlayers[i]);
			pMsgHu.iMaxFanType[i] = iPlayerMaxFanType;
			memcpy(pMsgHu.cHuFanResult[i], tableItem->pPlayers[i]->cFanResult, sizeof(tableItem->pPlayers[i]->cFanResult));

			pMsgHu.iSpecialFlag[i] = tableItem->pPlayers[i]->iSpecialFlag;
			pMsgHu.iHuFan[i] = tableItem->pPlayers[i]->iHuFan;
			pMsgHu.iHuFlag[i] = tableItem->pPlayers[i]->iWinFlag;
			pMsgHu.iHuType[i] = tableItem->pPlayers[i]->iHuFlag;
			if (tableItem->pPlayers[i]->bTing)
				pMsgHu.cIfTing[i] = 1;

			StatisticAllHuFan(tableItem,tableItem->pPlayers[i]);
		}
	}

	// 算分
	long long viWinMoney[4][4] = { 0 };							// 四个玩家视角下赢家赢的金币数量
	char vcFengDingFlag[4][4];								// 四个玩家视角下玩家是否封顶
	char vcPoChanFlag[4][4];								// 如果只有一个玩家胡牌，四个玩家视角下点炮玩家是否破产
	memset(vcFengDingFlag, 0, sizeof(vcFengDingFlag));
	memset(vcPoChanFlag, 0, sizeof(vcPoChanFlag));
	for (int i = 0; i < PLAYER_NUM; i++)					// 玩家i视角下每个玩家的输赢分
	{
		if (tableItem->pPlayers[i])
		{
			int iRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iBaseScore = stSysConfBaseInfo.stRoom[iRoomID].iBasePoint;				// 基础底分
			int iFengDingFan = stSysConfBaseInfo.stRoom[iRoomID].iMinPoint;			// 封顶番
			long long iCurTableMoney = stSysConfBaseInfo.stRoom[iRoomID].iTableMoney;		// 桌费
			_log(_DEBUG, "HZXLGameLogic", "HandleDianPaoHu_log() iRoomID[%d],iBaseScore[%lld],iFengDingFan[%d],iCurTableMoney[%lld]", iRoomID, iBaseScore, iFengDingFan, iCurTableMoney);
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5 && tableItem->pPlayers[j]->cPoChan == 0)		// 玩家j胡点炮
				{
					int iResultFan = tableItem->pPlayers[j]->iHuFan;
					if (iResultFan > iFengDingFan && iFengDingFan > 0)
					{
						iResultFan = iFengDingFan;
						vcFengDingFlag[i][j] = 1;
						vcFengDingFlag[i][tableItem->iCurSendPlayer] = 1;
					}
					long long iResultWinMoney = iBaseScore * iResultFan;

					viWinMoney[i][j] = iResultWinMoney;
					viWinMoney[i][tableItem->iCurSendPlayer] = iResultWinMoney;		// 先使用正数，最后再改为负数

																					// 如果只有玩家j胡牌，在这里判断点炮玩家是否破产
					if (tableItem->iAllHuNum == 1)
					{
						long long iTempMoney = 0;

						if (i == tableItem->iCurSendPlayer)		// 点炮玩家视角
						{
							iTempMoney = tableItem->pPlayers[i]->iMoney + tableItem->pPlayers[i]->iCurWinMoney - iCurTableMoney;
						}
							
						else                                    // 其他玩家视角
						{
							iTempMoney = tableItem->pPlayers[i]->otherMoney[tableItem->iCurSendPlayer].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[tableItem->iCurSendPlayer] - iCurTableMoney;
						}
						if (viWinMoney[i][j] > iTempMoney)		// i玩家视角下，j玩家赢的分数大于点炮玩家携带的分数,则i玩家视角下,点炮玩家破产
						{
							viWinMoney[i][j] = iTempMoney;
							viWinMoney[i][tableItem->iCurSendPlayer] = iTempMoney;
							vcPoChanFlag[i][tableItem->iCurSendPlayer] = 1;
						}
						break;	// 只有一个玩家胡牌，后面不用再判断其他玩家是否胡牌
					}
				}
			}
		}
	}

	// 判断破产
	bool bExistPoChan = false;
	long long iAllWinMoney[4] = { 0 };				// 四个玩家视角下，所有胡牌玩家总共应得赢分
	if (tableItem->iAllHuNum > 1)
	{
		for (int i = 0; i < PLAYER_NUM; i++)	// 玩家i视角下
		{
			if (tableItem->pPlayers[i])
			{
				long long iMinHuMoney = -1;		// 所有胡牌玩家中，最少玩家的赢分
				int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
				long long iCurTableMoney = stSysConfBaseInfo.stRoom[iPlayerRoomID].iTableMoney;		// 桌费

				// 计算玩家i视角下，多个胡牌玩家是否会造成点炮玩家破产
				for (int j = 0; j < PLAYER_NUM; j++)
				{
					if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5)
					{
						iAllWinMoney[i] += viWinMoney[i][j];
						if (viWinMoney[i][j] < iMinHuMoney || iMinHuMoney == -1)
							iMinHuMoney = viWinMoney[i][j];
					}
				}

				long long iSendPlayerMoney = 0;			// 点炮玩家能够输的最大金币数量
				if (i == tableItem->iCurSendPlayer)
					iSendPlayerMoney = tableItem->pPlayers[i]->iMoney + tableItem->pPlayers[i]->iCurWinMoney - iCurTableMoney;
				else
					iSendPlayerMoney = tableItem->pPlayers[i]->otherMoney[tableItem->iCurSendPlayer].llMoney + tableItem->pPlayers[i]->iAllUserCurWinMoney[tableItem->iCurSendPlayer] - -iCurTableMoney;

				// 玩家i视角下，所有胡牌玩家赢分大于点炮玩家携带的金币，需要根据情况分配金币（此时点炮玩家破产）
				if (iAllWinMoney[i] > iSendPlayerMoney)
				{
					long long iSendPlayerCurMoney = iSendPlayerMoney;
					//如果最小胡番赢钱数乘胡牌人数 大于玩家金币 平分
					if (iMinHuMoney * tableItem->iAllHuNum >= iSendPlayerCurMoney)
					{
						for (int j = 0; j < PLAYER_NUM; j++)
						{
							if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5)
								viWinMoney[i][j] = (iSendPlayerCurMoney) / tableItem->iAllHuNum;
						}
					}
					// 如果最小胡番乘胡牌人数 小于 放炮玩家金钱 先记录最小胡番金币 剩下的金币判断是否大于第二小的胡番
					else
					{
						int iMinHuNum = 0;
						long long iTempAllMoney = iSendPlayerCurMoney;
						long long iTempMinHuMoney = -1;
						for (int j = 0; j < PLAYER_NUM; j++)
						{
							if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5)
							{
								if (viWinMoney[i][j] == iMinHuMoney)
								{
									iTempAllMoney -= iMinHuMoney;	// 减掉最小胡牌玩家的金额 剩下的金额是 其余玩家可以拿到的
									iMinHuNum++;
								}
							}
						}

						//如果还剩一个玩家 剩余的金额直接给他
						if ((iMinHuNum == 2 && tableItem->iAllHuNum == 3) || (iMinHuNum == 1 && tableItem->iAllHuNum == 2))		//如果最小有两个玩家 那么一炮三响还剩一个玩家直接赋值
						{
							for (int j = 0; j < PLAYER_NUM; j++)
							{
								if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5)
								{
									if (viWinMoney[i][j] >= iTempAllMoney)
									{
										viWinMoney[i][j] = iTempAllMoney;
										break;
									}
								}
							}
						}
						//否则 判断剩余金币 是否大于第二小的胡番
						else
						{
							for (int j = 0; j < PLAYER_NUM; j++)
							{
								if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5)
								{
									if (viWinMoney[i][j] > iMinHuMoney)
									{
										if (iTempMinHuMoney > viWinMoney[i][j] || iTempMinHuMoney == -1)
										{
											iTempMinHuMoney = viWinMoney[i][j];
										}
									}
								}
							}
							//如果剩余金币 不大于 第二小的胡的两倍 那么直接平分了
							if ((iTempMinHuMoney * 2) >= iTempAllMoney)
							{
								for (int j = 0; j < PLAYER_NUM; j++)
								{
									if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5 && viWinMoney[i][j] >= iTempMinHuMoney)
									{
										viWinMoney[i][j] = iTempAllMoney / 2;
									}
								}
							}
							else
							{
								for (int j = 0; j < PLAYER_NUM; j++)
								{
									if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5 && viWinMoney[i][j] == iTempMinHuMoney)
									{
										iTempAllMoney -= iTempMinHuMoney;
									}
								}
								for (int j = 0; j < PLAYER_NUM; j++)
								{
									if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5 && viWinMoney[i][j] > iTempMinHuMoney)
									{
										viWinMoney[i][j] = iTempAllMoney;
									}
								}
							}
						}
					}
					vcPoChanFlag[i][tableItem->iCurSendPlayer] = 1;					// i玩家视角下，点炮玩家破产
					viWinMoney[i][tableItem->iCurSendPlayer] = iSendPlayerMoney;

					_log(_DEBUG, "HZXLGameLogic", "HandleDianPaoHu_log() i[%d], iCurSendPlayer[%d], iSendPlayerMoney[%lld]", i, tableItem->iCurSendPlayer, iSendPlayerMoney);
				}
				// 点炮玩家分数够支付，点炮玩家支付所有赢牌玩家赢的总分数
				else
				{
					viWinMoney[i][tableItem->iCurSendPlayer] = iAllWinMoney[i];
				}
			}
		}
	}

	// 点炮玩家的分数置为负数
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if(tableItem->iCurSendPlayer >= 0 && tableItem->iCurSendPlayer < PLAYER_NUM)
			viWinMoney[i][tableItem->iCurSendPlayer] = -viWinMoney[i][tableItem->iCurSendPlayer];
	}

	// 保存玩家输赢分数，发送胡牌消息到客户端
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		cRealPoChan[i] = vcPoChanFlag[i][tableItem->iCurSendPlayer];
		if (tableItem->pPlayers[i])
		{
			memset(pMsgHu.cFengDing, 0, sizeof(pMsgHu.cFengDing));
			memset(pMsgHu.iMoneyResult, 0, sizeof(pMsgHu.iMoneyResult));
			memset(pMsgHu.cPoChan, 0, sizeof(pMsgHu.cPoChan));
			memset(pMsgHu.cTingHuCard, 0, sizeof(pMsgHu.cTingHuCard));
			memset(pMsgHu.iTingHuFan, 0, sizeof(pMsgHu.iTingHuFan));
			memset(pMsgHu.viCurWinMoney, 0, sizeof(pMsgHu.viCurWinMoney));
			pMsgSpecial.cHaveHuNotice = 0;
			pMsgHu.ihjzyFan = 0;
			pMsgHu.iHuJiaoZhuanYi = 0;

			int iRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iCurTableMoney = stSysConfBaseInfo.stRoom[iRoomID].iTableMoney;		// 桌费
			pMsgHu.iTableMoney = iCurTableMoney;

			if (bHJZY)
			{
				pMsgHu.ihjzyFan = iHJZYFan;
				pMsgHu.iHuJiaoZhuanYi = viHJZYScore[i];
			}

			memcpy(pMsgHu.cFengDing, vcFengDingFlag[i], sizeof(vcFengDingFlag[i]));
			memcpy(pMsgHu.cPoChan, vcPoChanFlag[i], sizeof(vcPoChanFlag[i]));

			PoChanCardNoticeMsgDef pPCInfo;
			memset(&pPCInfo, 0, sizeof(PoChanCardNoticeMsgDef));

			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (j == i)
				{
					tableItem->pPlayers[i]->iCurWinMoney += viWinMoney[i][j];
					pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iCurWinMoney;
				}
				else
				{
					tableItem->pPlayers[i]->iAllUserCurWinMoney[j] += viWinMoney[i][j];
					pMsgHu.viCurWinMoney[j] = tableItem->pPlayers[i]->iAllUserCurWinMoney[j];
				}
			}
			memcpy(pMsgHu.iMoneyResult, viWinMoney[i], sizeof(viWinMoney[i]));

			if (cNeedUpdateTing[i] == 1)		// 需要更新该玩家的听牌信息
			{
				pMsgHu.vcNeedUpdateTingInfo[i] = 1;
				for (int j = 0; j < sizeof(cTingHuCard[i]); j++)
				{
					pMsgHu.cTingHuCard[j] = cTingHuCard[i][j];
					pMsgHu.iTingHuFan[j] = iTingHuFan[i][j];
				}
			}
			else
			{
				pMsgHu.vcNeedUpdateTingInfo[i] = 0;
			}

			// 玩家i视角下点炮玩家破产
			int iSend = tableItem->iCurSendPlayer;
			if (vcPoChanFlag[i][iSend] == 1)
			{
				memcpy(pPCInfo.cHandCards[iSend], tableItem->pPlayers[iSend]->cHandCards, sizeof(tableItem->pPlayers[iSend]->cHandCards));
				memcpy(&pPCInfo.cpgInfo[iSend], &tableItem->pPlayers[iSend]->CPGInfo, sizeof(tableItem->pPlayers[iSend]->CPGInfo));
				bExistPoChan = true;
			}

			_log(_DEBUG, "HZXLGameLogic", "HandleDianPaoHu_log() viWinMoney[%d][%d]=[%d], cRealPoChan[%d], iSend[%d]", i, i, viWinMoney[i][i], cRealPoChan[i], iSend);
			if (bExistPoChan)
			{
				pPCInfo.iTableNumExtra = tableItem->pPlayers[i]->cTableNumExtra;
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pPCInfo, MJG_PLAYER_POCHAN_MSG, sizeof(PoChanCardNoticeMsgDef));	// 点炮胡后破产

				pMsgSpecial.cHaveHuNotice = 1;
				char *cMsgBuffer = new char[sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef)];
				memset(cMsgBuffer, 0, sizeof(cMsgBuffer));
				memcpy(cMsgBuffer, &pMsgSpecial, sizeof(SpecialCardsNoticeDef));
				memcpy(cMsgBuffer + sizeof(SpecialCardsNoticeDef), &pMsgHu, sizeof(SpecialHuNoticeDef));
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, (void *)cMsgBuffer, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef));
				delete[] cMsgBuffer;
				cMsgBuffer = NULL;
			}
			else
			{
				char *cMsgBuffer = new char[sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef)];
				memset(cMsgBuffer, 0, sizeof(cMsgBuffer));
				memcpy(cMsgBuffer, &pMsgSpecial, sizeof(SpecialCardsNoticeDef));
				memcpy(cMsgBuffer + sizeof(SpecialCardsNoticeDef), &pMsgHu, sizeof(SpecialHuNoticeDef));

				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, (void *)cMsgBuffer, MJG_SPECIAL_CARD_RES, sizeof(SpecialCardsNoticeDef) + sizeof(SpecialHuNoticeDef));
				delete[] cMsgBuffer;
				cMsgBuffer = NULL;
			}
		}
	}

	// 记录点炮流水
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		for (int j = 0; j < PLAYER_NUM; j++)				// 玩家i视角下，记录每个玩家的输赢状况
		{
			GameLiuShuiInfoDef TempLs;
			memset(&TempLs, 0, sizeof(GameLiuShuiInfoDef));
			if (tableItem->pPlayers[j] && tableItem->pPlayers[j]->iSpecialFlag >> 5 && tableItem->pPlayers[j]->cPoChan == 0)		// 每个胡牌玩家都记录一条流水
			{
				TempLs.cWinner |= (1 << j);
				TempLs.iWinMoney = viWinMoney[i][j];				// 本次结算赢分
				TempLs.iFan = tableItem->pPlayers[j]->iHuFan;
				TempLs.cLoser |= (1 << tableItem->iCurSendPlayer);
				TempLs.cFengDing = vcFengDingFlag[i][j];
				if (vcPoChanFlag[i][tableItem->iCurSendPlayer] == 1)
					TempLs.cPoChan |= (1 << tableItem->iCurSendPlayer);
				memcpy(TempLs.cFanResult, tableItem->pPlayers[j]->cFanResult, sizeof(tableItem->pPlayers[j]->cFanResult));

				tableItem->vLiuShuiList[i].push_back(TempLs);		// 点炮流水
			}
		}
	}

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			_log(_DEBUG, "HZXLMJGameLogic", "HandleDianPaoHu_Log()UID[%d], pPlayers[%d]->iCurWinMoney = %lld", tableItem->pPlayers[i]->iUserID, i, tableItem->pPlayers[i]->iCurWinMoney);
		}
	}
}

// 处理点炮胡中的呼叫转移(玩家杠牌后杠上炮，需将自己的杠牌所得转移给被点炮的玩家)
/*
	@param nodePlayers:胡牌玩家
	@param tableItem
	@return viHJZYScore:四个玩家视角下，呼叫转移的分数
*/
void HZXLGameLogic::HandleHJZYHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, long long viHJZYScore[4], int &iHJZYFan)
{
	long long  viTmpHJZYScore[4][4] = { 0 };
	int iSingleFan = 0;
	int iLastCPG = tableItem->pPlayers[tableItem->iCurSendPlayer]->CPGInfo.iAllNum - 1;				// 从后往前遍历
	for (; iLastCPG >= 0; iLastCPG--)
	{
		if (tableItem->pPlayers[tableItem->iCurSendPlayer]->CPGInfo.Info[iLastCPG].iCPGType == 2)	// 当前一组牌是杠牌	
		{
			int iGangFlag = tableItem->pPlayers[tableItem->iCurSendPlayer]->CPGInfo.Info[iLastCPG].cGangType;
			if (iGangFlag == 0)				// 明杠
				iSingleFan = 2;
			else if (iGangFlag == 1)		// 补杠
				iSingleFan = 3 - tableItem->iPoChanNum;
			else if (iGangFlag == 2)		// 暗杠
				iSingleFan = (3 - tableItem->iPoChanNum) * 2;
			tableItem->pPlayers[tableItem->iCurSendPlayer]->CPGInfo.Info[iLastCPG].cGangType = -1;		// 重置点炮玩家该组杠牌信息
			break;
		}
	}

	_log(_DEBUG, "HZXLGameLogic", "HandleHJZYHu_Log() UID[%d], iSingleFan[%d]", nodePlayers->iUserID, iSingleFan);

	// 根据玩家底分计算呼叫转移的分数
	for (int i = 0; i < PLAYER_NUM; i++)		// 玩家i视角下，点炮玩家和胡牌玩家的输赢分
	{
		if (tableItem->pPlayers[i])
		{
			int iPlayerRoomID = tableItem->pPlayers[i]->cRoomNum - 1;
			long long iBaseScore = stSysConfBaseInfo.stRoom[iPlayerRoomID].iBasePoint;				// 基础底分
			viTmpHJZYScore[i][nodePlayers->cTableNumExtra] = iBaseScore * iSingleFan;
			viTmpHJZYScore[i][tableItem->iCurSendPlayer] = -iBaseScore * iSingleFan;
			viHJZYScore[i] = iBaseScore * iSingleFan;
		}
	}

	iHJZYFan = iSingleFan;

	// 呼叫转移的分数单独计费
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (i == j)
				{
					tableItem->pPlayers[i]->iCurWinMoney += viTmpHJZYScore[i][j];
					tableItem->pPlayers[i]->iGangFen += viTmpHJZYScore[i][j];		// 更新杠分
				}
				else
				{
					tableItem->pPlayers[i]->iAllUserCurWinMoney[j] += viTmpHJZYScore[i][j];
				}
			}
		}
	}

	// 呼叫转移流水
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		GameLiuShuiInfoDef TempLs;
		memset(&TempLs, 0, sizeof(GameLiuShuiInfoDef));
		TempLs.cWinner |= (1 << nodePlayers->cTableNumExtra);
		TempLs.cLoser |= (1 << tableItem->iCurSendPlayer);
		TempLs.iFan = iSingleFan;
		TempLs.iSpecialFlag = WIN_HU_JIAO_ZHUAN_YI;			// 呼叫转移
		TempLs.iWinMoney = viTmpHJZYScore[i][nodePlayers->cTableNumExtra];
		TempLs.iMoney[tableItem->iCurSendPlayer] = viTmpHJZYScore[i][tableItem->iCurSendPlayer];

		tableItem->vLiuShuiList[i].push_back(TempLs);
	}
}

// 玩家胡牌时，如果没有听牌信息，计算胡牌玩家的听牌信息
void HZXLGameLogic::CalculateHuTingInfo(MJGTableItemDef *tableItem, char cTingHuCard[4][28], int iTingHuFan[4][28], char cNeedUpdateTing[4])
{
	memset(cTingHuCard, 0, sizeof(cTingHuCard));
	memset(iTingHuFan, 0, sizeof(iTingHuFan));
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			if ((tableItem->pPlayers[i]->iSpecialFlag >> 5) && (tableItem->pPlayers[i]->vcTingCard.empty()))		// 玩家没有听牌信息，计算听牌信息
			{
				cNeedUpdateTing[i] = 1;
				char cCard = tableItem->pPlayers[i]->cHuCard;
				vector<char> vcCards;
				for (int j = 0; j < tableItem->pPlayers[i]->iHandCardsNum; j++)
				{
					vcCards.push_back(tableItem->pPlayers[i]->cHandCards[j]);
				}

				vcCards.push_back(cCard);
				map<char, vector<char> > mpTing;
				map<char, map<char, int> > mpTingFan;
				int iFlag = JudgeSpecialTing(tableItem->pPlayers[i], tableItem, tableItem->pPlayers[i]->resultType, vcCards, mpTing, mpTingFan);		// 胡牌阶段，判断玩家能听的牌
				if (iFlag > 0 && !mpTing[cCard].empty())
				{
					vector<char>().swap(tableItem->pPlayers[i]->vcSendTingCard);
					//给玩家听的牌 赋值
					tableItem->pPlayers[i]->bTing = true;
					vector<char>().swap(tableItem->pPlayers[i]->vcTingCard);
					tableItem->pPlayers[i]->vcTingCard.insert(tableItem->pPlayers[i]->vcTingCard.begin(), mpTing[cCard].begin(), mpTing[cCard].end());
					map<char, int>().swap(tableItem->pPlayers[i]->mpTingFan);
					tableItem->pPlayers[i]->mpTingFan = mpTingFan[cCard];

					int iTempMaxFan = 0;
					map<char, int>::iterator pos = mpTingFan[cCard].begin();
					while (pos != mpTingFan[cCard].end())
					{
						if (pos->second > iTempMaxFan)
							iTempMaxFan = pos->second;
						pos++;
					}
					tableItem->pPlayers[i]->iTingHuFan = iTempMaxFan;
				}
			}
			if (tableItem->pPlayers[i]->vcTingCard.size() > 0)
			{
				cNeedUpdateTing[i] = 1;
				for (int j = 0; j < tableItem->pPlayers[i]->vcTingCard.size(); j++)
				{
					cTingHuCard[i][j] = tableItem->pPlayers[i]->vcTingCard[j];
					iTingHuFan[i][j] = tableItem->pPlayers[i]->mpTingFan[tableItem->pPlayers[i]->vcTingCard[j]];
				}
			}
		}
	}
}

// 玩家胡牌后的处理
void HZXLGameLogic::OperateAfterHuState(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem)
{
	bool bHuZiMo = false;
	if (nodePlayers->cTableNumExtra == tableItem->iCurMoPlayer)
		bHuZiMo = true;

	EscapeReqDef msgReq;
	memset(&msgReq, 0, sizeof(EscapeReqDef));
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->cPoChan)
		{
			_log(_DEBUG, "HZXLGameLogic", "OperateAfterHuState_Log() pPlayers[%d]->cPoChan = %d", i, tableItem->pPlayers[i]->cPoChan);
			if (tableItem->pPlayers[i]->bIfWaitLoginTime == true || tableItem->pPlayers[i]->cDisconnectType == 1)
			{
				if (tableItem->pPlayers[i]->bTempMidway)
					continue;
				JudgeMidwayLeave(tableItem->pPlayers[i]->iUserID, &msgReq);
			}
		}
	}

	nodePlayers->iWinFlag = 0;			// 胡牌结束，需要重置特殊胡的标志
	_log(_DEBUG, "HZXLGameLogic", "OperateAfterHuState_Log():END Total Hu Player Cnt[%d],  PoChan Player[%d]", tableItem->iHasHuNum, tableItem->iPoChanNum);
	if (tableItem->iPoChanNum >= 3)
	{
		_log(_DEBUG, "XLMJGameLogic", "OperateAfterHuState_Log() Hu Pai: tableItem->iPoChanNum = %d", tableItem->iPoChanNum);
		GameEndHu(nodePlayers, tableItem, false, nodePlayers->iSpecialFlag);		// 胡牌阶段，导致破产玩家数量大于等于3
		return;
	}
	else
	{
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->cPoChan == 0)
			{
				tableItem->pPlayers[i]->iStatus = PS_WAIT_SEND;
				memset(&tableItem->pPlayers[i]->pTempCPG, 0, sizeof(MJGTempCPGReqDef));
			}
		}
		if (tableItem->iAllHuNum > 1)		//一炮多响 放炮玩家摸牌
		{
			tableItem->iAllHuNum = 0;
			tableItem->iCurMoPlayer = tableItem->iCurSendPlayer;
			/*		if (tableItem->pPlayers[tableItem->iCurMoPlayer] && tableItem->pPlayers[tableItem->iCurMoPlayer]->cPoChan == 0)
					{
						tableItem->pPlayers[tableItem->iCurMoPlayer]->iSpecialFlag = 0;
						if (tableItem->bExistWaitRecharge)
							tableItem->iNextWaitSendExtra = tableItem->iCurMoPlayer;
						else
							SendCardToPlayer(tableItem->pPlayers[tableItem->iCurSendPlayer], tableItem);
						return;
					}
					else
					{*/
			int i;
			for (i = 1; i < PLAYER_NUM; i++)
			{
				tableItem->iCurMoPlayer = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
				if (tableItem->pPlayers[tableItem->iCurMoPlayer] && tableItem->pPlayers[tableItem->iCurMoPlayer]->cPoChan == 0)
				{
					tableItem->pPlayers[tableItem->iCurMoPlayer]->iSpecialFlag = 0;
					if (tableItem->bExistWaitRecharge)
						tableItem->iNextWaitSendExtra = tableItem->iCurMoPlayer;
					else
						SendCardToPlayer(tableItem->pPlayers[tableItem->iCurMoPlayer], tableItem);
					break;
				}
			}
			if (i == PLAYER_NUM)
			{
				_log(_ERROR, "HZXLGameLogic", "OperateAfterHuState_Log()T[%d]:玩家tableItem->iCurMoPlayer[%d] abnormal 没有找到对应玩家", nodePlayers->cRoomNum, tableItem->iCurMoPlayer);
			}
			//}
		}
		else		//胡牌玩家下家摸牌
		{
			tableItem->iAllHuNum = 0;
			int i;
			for (i = 1; i < PLAYER_NUM; i++)
			{
				tableItem->iCurMoPlayer = (nodePlayers->cTableNumExtra + i) % PLAYER_NUM;
				if (!bHuZiMo)
					tableItem->iCurMoPlayer = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
				if (tableItem->pPlayers[tableItem->iCurMoPlayer] && tableItem->pPlayers[tableItem->iCurMoPlayer]->cPoChan == 0)
				{
					_log(_DEBUG, "HZXLGameLogic", "OperateAfterHuState_Log(): MoCard R[%d] T[%d] 玩家tableItem->iCurMoPlayer[%d]  玩家摸牌", tableItem->pPlayers[tableItem->iCurMoPlayer]->cRoomNum, tableItem->iTableID, tableItem->iCurMoPlayer);
					tableItem->pPlayers[tableItem->iCurMoPlayer]->iSpecialFlag = 0;
					if (tableItem->bExistWaitRecharge)
						tableItem->iNextWaitSendExtra = tableItem->iCurMoPlayer;
					else
						SendCardToPlayer(tableItem->pPlayers[tableItem->iCurMoPlayer], tableItem);
					return;
				}
			}
			if (i == PLAYER_NUM)
			{
				_log(_ERROR, "HZXLGameLogic", "OperateAfterHuState_Log(): tableItem->iCurMoPlayer[%d] abnormal 没有找到对应玩家", tableItem->iCurMoPlayer);
				GameEndHu(nodePlayers, tableItem, false, nodePlayers->iSpecialFlag);			// 胡牌后，寻找摸牌玩家异常，游戏结束
			}
		}
		return;
	}
}

void HZXLGameLogic::ResetTableState(void *pTableItem)
{
	//初始化本桌游戏信息
	MJGTableItemDef *tableItem = (MJGTableItemDef*)pTableItem;
	tableItem->Reset();

}

//判断是否游戏允许中途离开  在此函数中完成需要中途离开的玩家的结算工作
bool HZXLGameLogic::JudgeMidwayLeave(int iUserID, void *pMsgData)
{
	bool bRet = false;
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)&iUserID, sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "XLMJGameLogic", " JudgeMidwayLeave_log() nodePlayers is NULL");
		return bRet;
	}
	_log(_DEBUG, "XLMJGameLogic", " JudgeMidwayLeave_log() userID =%d, iStatus = %d", iUserID, nodePlayers->iStatus);

	if (nodePlayers->bTempMidway)
	{
		_log(_DEBUG, "XLMJGameLogic", " JudgeMidwayLeave_log() R[%d] T[%d] nodePlayer userid = %d is  bTempMidway", nodePlayers->cRoomNum, nodePlayers->cTableNum);
		return bRet;
	}

	if (nodePlayers->iStatus == PS_WAIT_RESERVE_TWO)
	{
		_log(_ERROR, "XLMJGameLogic", "JudgeMidwayLeave_log() R[%d] T[%d] nodePlayer is  PS_WAIT_RESERVE_TWO", nodePlayers->cRoomNum, nodePlayers->cTableNum);

		if (m_pNewAssignTableLogic != NULL)
		{
			m_pNewAssignTableLogic->CheckIfInWaitTable(iUserID, true);
			m_pNewAssignTableLogic->CallBackUserLeave(iUserID, nodePlayers->iVTableID);
		}

		HandleNormalEscapeReq(nodePlayers, pMsgData);
		return true;
	}
	return bRet;
}

void HZXLGameLogic::DoOneHour()
{

}

// 五分钟定时器
void HZXLGameLogic::CallBackFiveMinutes()
{
	GameConf::GetInstance()->ReadGameConf("hzxlmj_game.conf");
	m_pHzxlAICTL->ReadRoomConf();
}


//判断玩家长时间没有操作剔除
void HZXLGameLogic::JudgePlayerKickOutTime(int iTime)
{
	//_log(_DEBUG, "HZXLGameLogic", "JudgePlayerKickOutTime: Start iTime[%d]", iTime);
	//遍历所有玩家
	MJGPlayerNodeDef *node = m_pNodePlayer;
	int iJudgeCnt = 0;
	while (node)
	{
		//_log(_DEBUG, "HZXLGameLogic", "JudgePlayerKickOutTime: UID[%d], iKickOutTime[%d]", node->iUserID, node->iKickOutTime);
		if (node->iKickOutTime > 0)
		{
			if (node->cPoChan == 1)
			{
				_log(_ERROR, "HZXLGameLogic", "JudgePlayerKickOutTime: UID[%d], PoChan, name:%s", node->iUserID, node->szNickName);
				node = node->pNext;
				continue;
			}
			node->iKickOutTime -= iTime;
			if (node->iKickOutTime <= 0)
			{
				if (node->bPlayerKick == false && node->iSocketIndex > 0)
				{
					_log(_ERROR, "HZXLGameLogic", "JudgePlayerKickOutTime:UID[%d] name:%s", node->iUserID, node->szNickName);
					node->cDisconnectType = 3;
					node->bPlayerKick = true;
					m_pClientEpoll->SetKillFlag(node->iSocketIndex, true);        //断开该用户的Socket
				}
			}
		}
		node = node->pNext;
		iJudgeCnt++;
		if (iJudgeCnt >= KICK_MAX_CNT)
		{
			_log(_ERROR, "HZXLGameLogic", "JudgePlayerKickOutTime:iJudgeCnt[%d]", iJudgeCnt);
			int iJudgeCnt2 = 0;
			MJGPlayerNodeDef *node2 = m_pNodePlayer;
			while (node2)
			{
				_log(_DEBUG, "HZXLGameLogic", "JudgePlayerKickOutTime:iJudgeCnt2[%d],iUserID[%d]", iJudgeCnt2, node2->iUserID);
				node2 = node2->pNext;
				iJudgeCnt2++;
				if (iJudgeCnt2 >= KICK_MAX_CNT + 50)
				{
					break;
				}
			}
			break;
		}
	}
	//_log(_DEBUG, "HZXLGameLogic", "JudgePlayerKickOutTime: End");
}



void HZXLGameLogic::HandleNormalEscapeReq(FactoryPlayerNodeDef *nodePlayers, void *pMsgData)
{
	if (nodePlayers->iStatus == PS_WAIT_DESK)
	{
		GameLogic::HandleNormalEscapeReq(nodePlayers, pMsgData);
		return;
	}
	int i;
	int iTableNum = nodePlayers->cTableNum;
	int iRoomNum = nodePlayers->cRoomNum;

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleNormalEsc:Error Name[%s]Room[%d]T[%d]", nodePlayers->szNickName, iRoomNum, iTableNum);
		return;
	}

	MJGTableItemDef *tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	_log(_DEBUG, "HZXLGameLogic", "HandleNormalEscapeReq  R[%d]T[%d]UID[%d],iStatus[%d]",iRoomNum, iTableNum, nodePlayers->iUserID, nodePlayers->iStatus);

	if (nodePlayers->iStatus == PS_WAIT_RESERVE_TWO)
	{
		int iTablePos = nodePlayers->cTableNumExtra;
		if (!tableItem->bIfExistAIPlayer || (tableItem->bIfExistAIPlayer && tableItem->pPlayers[iTablePos]->iPlayerType == NORMAL_PLAYER))
		{
			MJGPlayerNodeDef *pNode = (MJGPlayerNodeDef*)GetTmpFreeNode();
			if (pNode == NULL)
			{
				_log(_ERROR, "HZXLGameLogic", "HandleNormalEsc:Error Name[%s]Room[%d]T[%d] pNode == NULL", nodePlayers->szNickName, iRoomNum, iTableNum);
				return;
			}
			vector<char> vcSendTingCard;		// 定义一个临时容器
			vector<char> vcTingCard;			// 定义一个临时容器
			map<char, int> mpTingFan;
			vector<char> vcHuCard;

			_log(_DEBUG, "XLMJGameLogic", "JudgeMidwayLeave cTableNumExtra[%d], iUserID[%d] .", nodePlayers->cTableNumExtra, nodePlayers->iUserID);
			RDSendAccountReq(&((MJGPlayerNodeDef *)nodePlayers)->msgRDTemp,NULL);


			((MJGPlayerNodeDef *)nodePlayers)->vcSendTingCard.swap(vcSendTingCard);
			((MJGPlayerNodeDef *)nodePlayers)->vcTingCard.swap(vcTingCard);
			((MJGPlayerNodeDef *)nodePlayers)->mpTingFan.swap(mpTingFan);
			((MJGPlayerNodeDef *)nodePlayers)->vcHuCard.swap(vcHuCard);

			//保存玩家的数据到新建的节点上
			memcpy(pNode, (MJGPlayerNodeDef *)nodePlayers, sizeof(MJGPlayerNodeDef));

			pNode->vcSendTingCard.swap(vcSendTingCard);
			pNode->vcTingCard.swap(vcTingCard);
			pNode->mpTingFan.swap(mpTingFan);
			pNode->vcHuCard.swap(vcHuCard);

			pNode->iSocketIndex = -1;		// 清掉Stack里的SocketIndex
			pNode->bTempMidway = true; 		// 标示中途离开的临时玩家节点
			pNode->cDisconnectType = 1;		// 标示玩家断线

			if (tableItem->bIfExistAIPlayer)
			{
				pNode->iPlayerType = NORMAL_PLAYER;
			}

			//将新建立的节点保存到桌上 这样就可以让玩家的真实节点逃跑了
			tableItem->pFactoryPlayers[nodePlayers->cTableNumExtra] = pNode;
			tableItem->pPlayers[nodePlayers->cTableNumExtra] = pNode;

			//下面重置真实节点的信息好让玩家离开
			nodePlayers->iStatus = PS_WAIT_DESK;	//逃跑玩家的状态置回找座位
			nodePlayers->cTableNum = 0;				//没有座位		// 桌号不能重置，否则会导致中途破产继续游戏的时候，服务端不能踢人
			nodePlayers->cTableNumExtra = -1;		//没有入座位置


			vcSendTingCard.clear();
			vcTingCard.clear();
			mpTingFan.clear();
			vcHuCard.clear();
		}
	}

	GameLogic::HandleNormalEscapeReq(nodePlayers, pMsgData);
}

void HZXLGameLogic::CallBackDisconnect(FactoryPlayerNodeDef *nodePlayers)
{
	if (nodePlayers != NULL)
	{
		
	}
}

// 玩家坐下后，向radius发送消息，请求获取每日任务信息
void HZXLGameLogic::CallBackSitSuccedDownRes(FactoryTableItemDef *pTableItem, FactoryPlayerNodeDef *nodePlayers)
{
	GameLogic::CallBackSitSuccedDownRes(pTableItem, nodePlayers);
}

void HZXLGameLogic::CallbackHandleRadiusServerMsg(char* pMsgData, int iMsgLen)
{
	MsgHeadDef* pMsgHead = (MsgHeadDef*)pMsgData;
	if (pMsgHead->iMsgType == GAME_GET_PARAMS_MSG)
	{
		HandleBKGetParamsValueRes(pMsgData);
	}
}

// 换牌阶段，选择3张最优的牌进行交换
void HZXLGameLogic::SelectChangeCards(MJGPlayerNodeDef *nodePlayers, char *cResultChangeCard)
{
	char cChangeCard[3];
	memset(cChangeCard, 0, sizeof(cChangeCard));
	int iTempArr[5][10] = { 0 };
	int iMinType = 0;
	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		int iType = nodePlayers->cHandCards[i] >> 4;
		int iValue = nodePlayers->cHandCards[i] & 0x000f;

		iTempArr[iType][0]++;
		iTempArr[iType][iValue]++;
	}

	int iArrIndex[3] = { 0 };
	if (iTempArr[0][0] < iTempArr[1][0])
	{
		iArrIndex[0] = 0;
		if (iTempArr[1][0] < iTempArr[2][0])
		{
			iArrIndex[1] = 1;
			iArrIndex[2] = 2;
		}
		else
		{
			iArrIndex[1] = 2;
			iArrIndex[2] = 1;
			if (iTempArr[0][0] > iTempArr[2][0])
			{
				iArrIndex[0] = 1;
				iArrIndex[2] = 0;
			}
		}
	}
	else
	{
		iArrIndex[1] = 0;
		if (iTempArr[0][0] < iTempArr[2][0])
		{
			iArrIndex[0] = 1;
			iArrIndex[2] = 2;
		}
		else
		{
			iArrIndex[0] = 2;
			iArrIndex[1] = 0;
			iArrIndex[2] = 1;
			if (iTempArr[1][0] > iTempArr[2][0])
			{
				iArrIndex[1] = 1;
				iArrIndex[2] = 0;
			}
		}
	}

	for (int i = 0; i < 3; i++)
	{
		if (iTempArr[iMinType][0] < 3)
			iMinType++;
		if (iTempArr[i][0] < 3 || i == iMinType)
		{
			continue;
		}
		else
		{
			if (iTempArr[i][0] < iTempArr[iMinType][0])
			{
				iMinType = i;
			}
			else if (iTempArr[i][0] == iTempArr[iMinType][0])
			{
				int iTempA = 0;
				int iTempB = 0;
				for (int j = 0; j < 10; j++)
				{
					if (iTempArr[i][j] > 0)
						iTempA++;
					if (iTempArr[iMinType][j] > 0)
						iTempB++;
				}
				if (iTempA > iTempB)
					iMinType = i;
			}
		}
	}

	ChangeCardsReqDef msgChangeCardReq;
	memset(&msgChangeCardReq, 0, sizeof(ChangeCardsReqDef));

	int iChoice = 0;
	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		int iValue = nodePlayers->cHandCards[i] & 0x000f;
		if (nodePlayers->cHandCards[i] >> 4 == iMinType && iTempArr[iMinType][iValue] == 1)
		{
			msgChangeCardReq.cCards[iChoice] = nodePlayers->cHandCards[i];
			iChoice++;
		}
		if (iChoice >= 3)
			break;
	}
	if (iChoice < 3)
	{
		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
		{
			int iValue = nodePlayers->cHandCards[i] & 0x000f;
			if ((nodePlayers->cHandCards[i] >> 4) == iMinType && iTempArr[iMinType][iValue] > 1)
			{
				msgChangeCardReq.cCards[iChoice] = nodePlayers->cHandCards[i];
				iChoice++;
			}
			if (iChoice >= 3)
				break;
		}
	}
}

// 发送断线重连中的ResultType消息到客户端
void HZXLGameLogic::SendExtraAgainLoginToClient(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef* pTableItem)
{
	if (nodePlayers == NULL)
		return;

	int iTableNumExtra = nodePlayers->cTableNumExtra;
	ExtraLoginAgainServerDef pMsg;
	memset(&pMsg, 0, sizeof(ExtraLoginAgainServerDef));
	pMsg.iTableNumExtra = iTableNumExtra;

	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (pTableItem->pPlayers[i])
		{
			pMsg.cpgInfo[i] = pTableItem->pPlayers[i]->CPGInfo;
			_log(_DEBUG, "HZXLGameLogic", "SendExtraAgainLoginToClient_Log() i = %d, iAllNum[%d]", i, pMsg.cpgInfo[i].iAllNum);
		}
	}

	_log(_DEBUG, "HZXLGameLogic", "SendExtraAgainLoginToClient_Log() cTableNumExtra[%d]", iTableNumExtra);

	CLSendSimpleNetMessage(nodePlayers->iSocketIndex, &pMsg, XLMJ_AGAIN_LOGIN_EXTRA, sizeof(ExtraLoginAgainServerDef));	// 吃碰杠
}

// 计算胡的总倍数
int HZXLGameLogic::CalculateTotalMulti(MJGPlayerNodeDef *nodePlayers)
{
	int iTotalFan = 0;
	bool bCanHu = false;
	for (int i = 0; i < MJ_FAN_TYPE_CNT; i++)
	{
		if (nodePlayers->cFanResult[i] > 0)
		{
			bCanHu = true;
			break;
		}
	}

	if (bCanHu)
	{
		iTotalFan = 1;
		for (int i = 0; i < MJ_FAN_TYPE_CNT; i++)
		{
			if (nodePlayers->cFanResult[i] > 0)
			{
				//_log(_DEBUG, "HZXLGameLogic", "CalculateTotalMulti_Log() nodePlayers->cFanResult[%d] = %d", i, nodePlayers->cFanResult[i]);
				if (i == SHI_BA_LUO_HAN && nodePlayers->cFanResult[i] == 127)
					iTotalFan *= 128;
				else
					iTotalFan *= nodePlayers->cFanResult[i];
			}
		}
	}

	if (bCanHu && (iTotalFan > MAX_WIN_FAN || iTotalFan < 0))
		iTotalFan = MAX_WIN_FAN;

	//_log(_DEBUG, "HZXLGameLogic", "CalculateTotalMulti_Log() iTotalFan = %d", iTotalFan);
	return iTotalFan;
}

// 计算胡牌玩家胡的最大番型
int HZXLGameLogic::CalculateMaxFanType(MJGPlayerNodeDef *nodePlayers)
{
	char cMaxFan = 0;
	int iMaxFanType = 0;
	if (nodePlayers == NULL)
		return iMaxFanType;

	for (int i = 0; i < MJ_FAN_TYPE_CNT; i++)
	{
		if (nodePlayers->cFanResult[i] > 0)
		{
			if (i == MJ_GEN)		// 根不算
				continue;
			else if (i == HAI_DI_LAO_YUE || i == MIAO_SHOU_HUI_CHUN)		// 如果有海底捞月或者妙手回春，优先显示
			{
				iMaxFanType = i;
				break;
			}

			if (nodePlayers->cFanResult[i] > cMaxFan)
			{
				cMaxFan = nodePlayers->cFanResult[i];
				iMaxFanType = i;
			}
		}
	}

	if (iMaxFanType == 0)
	{
		if (nodePlayers->cFanResult[MJ_GEN] > 0)		// 仅有根
		{
			iMaxFanType = MJ_ONLY_GEN_FAN;
		}
	}

	return iMaxFanType;
}

// 计算本桌中未破产玩家数量
int HZXLGameLogic::GetNormalPlayerNum(MJGTableItemDef* pTableItem)
{
	int iPlayerCnt = 0;
	for (int j = 0; j < PLAYER_NUM; j++)
	{
		if (pTableItem->pPlayers[j] && pTableItem->pPlayers[j]->cPoChan == 0)
		{
			iPlayerCnt++;
		}
	}

	return iPlayerCnt;
}

// 获取某个玩家在指定房间显示的钱
/*
	如果模拟场次 和 本身场次相同不做处理
	如果模拟场次 比 本身场次低
		如果金币大于本身场次中的封顶值*最大胡牌次数180，则取低场次的封顶值*最大胡牌次数180  到低场次的最大入场限制之间的值
		其他情况，应该是金币 除以 （本身场次底分/低场次底分）
	如果模拟场次 比 本身场次高，金币 乘以 （高场次底分/本身场次底分）
	ps1:小数点问题客户端向下取整
	ps2 : 若最大入场限制为999，999，999特殊处理为 场次中的封顶值 * 500
*/
int HZXLGameLogic::GetSimulateMoney(int iRoomIndex, FactoryPlayerNodeDef *otherNodePlayers)
{
	long long iDstBaseScore = m_pServerBaseInfo->stRoom[iRoomIndex].iBasePoint;						// 当前玩家底分
	long long iOrigBaseScore = m_pServerBaseInfo->stRoom[otherNodePlayers->cRoomNum - 1].iBasePoint;	// 其他玩家底分
	int iDstFan = (int)m_pServerBaseInfo->stRoom[iRoomIndex].iMinPoint;								// 当前玩家封顶番
	int iOrigFan = (int)m_pServerBaseInfo->stRoom[otherNodePlayers->cRoomNum - 1].iMinPoint;		// 其他玩家封顶番

	long long iDstTableMoney = stSysConfBaseInfo.stRoom[iRoomIndex].iTableMoney;
	long long iOrigTableMoney = stSysConfBaseInfo.stRoom[otherNodePlayers->cRoomNum - 1].iTableMoney;

	long long iDstMaxWin = iDstBaseScore * iDstFan;		// 当前玩家封顶值
	long long iOrigMaxWin = iOrigBaseScore * iOrigFan;	// 其他玩家封顶值
	int iMoney = 0;

	if (iDstBaseScore == iOrigBaseScore)
	{
		iMoney = otherNodePlayers->iMoney;
	}
	else if (iDstBaseScore < iOrigBaseScore)		// 模拟场次低于本身场次
	{
		_log(_DEBUG, "HZXLGameLogic", "GetSimulateMoney:iMoney[%lld], iOrigMaxWin[%lld]", otherNodePlayers->iMoney, iOrigMaxWin);
		if (otherNodePlayers->iMoney > iOrigMaxWin * 90)		// 其他玩家金币大于所在房间的封顶值*90(该玩家不可能破产)
		{
			int iLowValue = iDstMaxWin * 90;
			int iHighValue = iDstMaxWin * 90 * 1.2;
			int iMoneyGap = iHighValue - iLowValue;
			int iRandValue = rand() % 100;
			int iTempMoney = iMoneyGap / 16;
			int iLeftMoney = iMoneyGap - iTempMoney;
			iMoney = iLowValue + iTempMoney + (iRandValue*iLeftMoney) / 100;
		}
		else
		{
			iMoney = (otherNodePlayers->iMoney - iOrigTableMoney) / (iOrigBaseScore / iDstBaseScore) + iDstTableMoney;
		}
	}
	else                 // 模拟场次高于本身场次
	{
		iMoney = (otherNodePlayers->iMoney - iOrigTableMoney) * (iDstBaseScore / iOrigBaseScore) + iDstTableMoney;
	}

	_log(_DEBUG, "HZXLGameLogic", "GetSimulateMoney:iDstBaseScore = [%lld], iOrigBaseScore = [%lld]", iDstBaseScore, iOrigBaseScore);
	_log(_DEBUG, "HZXLGameLogic", "GetSimulateMoney:iRoomIndex = [%d], iMoney = [%lld], iUserID[%d]", iRoomIndex, iMoney, otherNodePlayers->iUserID);
	return iMoney;
}

void HZXLGameLogic::SendRadiusGetParamsValueReq()
{
	_log(_DEBUG, "FL", "SendRadiusGetParamsValueReq");

	vector<string> vcParamKey;
	vcParamKey.push_back(KEY_HZXL_CONTROL_GAME_NUM);
	vcParamKey.push_back(KEY_HZXL_CONTROL_GAME_RATE);
	SendGetServerConfParams(vcParamKey);
}

void HZXLGameLogic::HandleBKGetParamsValueRes(void* msgData)
{
	GameGetParamsResDef* msgRes = (GameGetParamsResDef*)msgData;
	
	char cTmpKey[32] = { 0 };
	char cTmpValue[128] = { 0 };
	for (int i = 0; i < msgRes->iKeyNum; i++)
	{
		memset(cTmpKey, 0, sizeof(cTmpKey));
		memcpy(cTmpKey, (char*)msgData + sizeof(GameGetParamsResDef) + i * (32 + 128), 32);

		memset(cTmpValue, 0, sizeof(cTmpValue));
		memcpy(cTmpValue, (char*)msgData + sizeof(GameGetParamsResDef) + i * (32 + 128) + 32, 128);

		if (strcmp(cTmpKey, KEY_HZXL_CONTROL_GAME_NUM) == 0 || strcmp(cTmpKey, KEY_HZXL_CONTROL_GAME_RATE) == 0)
		{
			m_mapParams[cTmpKey] = cTmpValue;
		}
		_log(_DEBUG, "GL", "HandleBKGetParamsValueRes cTmpKey[%s] cTmpValue[%s]", cTmpKey, cTmpValue);
	}

	map<string, string>::iterator pos;
	for (pos = m_mapParams.begin(); pos != m_mapParams.end(); ++pos)
	{
		_log(_DEBUG, "FL", "HandleBKGetParamsValueRes mapParams[%s] = [%s]", pos->first.c_str(), pos->second.c_str());
	}

	static bool bFirst = true;
	if (bFirst)
	{
		bFirst = false; //第一次不用不处理 是底层请求的回应 不包含游戏内特殊的key
	}
	else
	{
		m_pHzxlAICTL->ReadParamValueConf();
	}
}

string HZXLGameLogic::GetBKParamsValueByKey(string key)
{
	string value = "";
	map<string, string>::iterator pos = m_mapParams.find(key);
	if (pos != m_mapParams.end())
	{
		value = pos->second;
	}

	return value;
}

void HZXLGameLogic::CallBackGameNetDelay(int iMsgType, int iStatusCode, int iUserID, void * pMsgData, int iDelayUs)
{
	switch (iMsgType)
	{
		_log(_DEBUG, "FL", "SendRadiusGetParamsValueReq iMsgType[%d]", iMsgType);
	}
}

void HZXLGameLogic::CallBackGetRoomInfo()
{
	//读取Parmas表中相关设定
	SendRadiusGetParamsValueReq();
	//获取控制牌库
	int iCTLCardSize = m_pHzxlAICTL->GetCtlCardSize();
	_log(_DEBUG, "FL", "SendRadiusGetParamsValueReq iCTLCardSize[%d]", iCTLCardSize);
	//第一次获取牌库时判断
	if (iCTLCardSize == 0)
	{
		_log(_DEBUG, "FL", "SendRadiusGetParamsValueReq 88");
		GetControlCardFromRadius();
	}
	//
}

// 转换玩家的输赢分，以自己的座位号为1为准
bool HZXLGameLogic::TransResultScore(int iSeat, int viResultScore[], int viTransScore[])
{
	if (iSeat < 0 || iSeat > 3)
		return false;

	for (int i = 0; i < 4; i++)
	{
		int iPos = (iSeat + i + 3) % 4;
		viTransScore[i] = viResultScore[iPos];
	}

	return true;
}

// 某个玩家有操作的时候，将消息告知其他玩家，用于倒计时重置与显示
// viSpecialSeatFlag:是否有特殊操作，1表示有，0表示没有
void HZXLGameLogic::SendSpecialInfoToClient(MJGTableItemDef *tableItem, int viSpecialSeatFlag[4])
{
	SpecialInfoServerDef pMsg;
	memset(&pMsg, 0, sizeof(SpecialInfoServerDef));
	pMsg.iSpecialFlag = 1024;

	for (int i = 0; i < 4; i++)
	{
		if (tableItem->pPlayers[i] && viSpecialSeatFlag[i] == 0)		// 其他玩家可以进行特殊操作，通知玩家i
		{
			pMsg.cTableNumExtra = i;
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsg, HZXLMJ_SPECIAL_INFO_MSG, sizeof(SpecialInfoServerDef));
		}
	}
}

// 统计所有玩家的胡牌番数情况
// nodePlayers:胡牌玩家
void HZXLGameLogic::StatisticAllHuFan(MJGTableItemDef *tableItem,MJGPlayerNodeDef *nodePlayers)
{
	if (nodePlayers == NULL || nodePlayers->iSpecialFlag >> 5 == 0 || nodePlayers->cPoChan == 1 )
		return;

	bool bHasHuFan = false;
	for (int i = 0; i < MJ_FAN_TYPE_CNT; i++)
	{
		if (nodePlayers->cFanResult[i] > 0)
		{
			bHasHuFan = true;
		}
	}
	if (tableItem->bIfExistAIPlayer && nodePlayers->iPlayerType != NORMAL_PLAYER)
	{
		return;
	}
	int iRoomIndex = nodePlayers->cRoomNum - 1;
	int iRoomType = m_pServerBaseInfo->stRoom[iRoomIndex].iRoomType;			// 房间类型

	_log(_DEBUG, "HZXLGameLogic", "StatisticAllHuFan_log() bHasHuFan[%d], iRoomIndex[%d], iRoomType[%d]", bHasHuFan, iRoomIndex, iRoomType);

	if (!bHasHuFan)
	{
		_log(_DEBUG, "HZXLGameLogic", "StatisticAllHuFan_log() bHasHuFan[%d]", bHasHuFan);
		return;
	}

	MJHuFanStatisticsDef huFanStatistics;
	memset(&huFanStatistics, 0, sizeof(MJHuFanStatisticsDef));

	huFanStatistics.iGameID = HZXLMJ_GAME_ID;
	huFanStatistics.iRoomType = iRoomType;
	memcpy(huFanStatistics.cFanResult, nodePlayers->cFanResult, sizeof(nodePlayers->cFanResult));

	//此处待确认统计数据的发送
	/*huFanStatistics.msgHeadInfo.cVersion = MESSAGE_VERSION;
	huFanStatistics.msgHeadInfo.iMsgType = MJ_HU_FAN_STATISTICS;

	
	EventMsgRadiusDef msgRD;
	memset(&msgRD, 0, sizeof(EventMsgRadiusDef));
	msgRD.iDataLen = sizeof(MJHuFanStatisticsDef);
	memcpy(msgRD.msgData, &huFanStatistics, sizeof(MJHuFanStatisticsDef));

	if (m_pMQRedis)
	{
		m_pMQRedis->EnQueue(&msgRD, sizeof(EventMsgRadiusDef));
		_log(_DEBUG, "HZXLGameLogic", "StatisticAllHuFan_log() m_pMQRedis != NULL iUserID[%d]", nodePlayers->iUserID);
	}
	else
		_log(_DEBUG, "HZXLGameLogic", "StatisticAllHuFan_log() m_pMQRedis == NULL iUserID[%d]", nodePlayers->iUserID);*/
}

bool HZXLGameLogic::JudgeIsPlayWithAI(FactoryPlayerNodeDef *nodePlayers)
{
	if (nodePlayers == NULL)
	{
		return false;
	}
	// 判断其他玩家是否是AI玩家
	int cRoomNum = nodePlayers->cRoomNum;
	int iTableNum = nodePlayers->cTableNum;
	MJGTableItemDef *pTableItem = (MJGTableItemDef*)GetTableItem(cRoomNum - 1, iTableNum - 1);
	if (pTableItem->bIfExistAIPlayer)
	{
		return true;
	}
	else
	{
		return false;
	}
}
void HZXLGameLogic::InitERMJAIInfo(MJGTableItemDef * pTableItem, const int iPlayerSeat)
{
	if (iPlayerSeat < 0 || iPlayerSeat > 3)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "InitERMJAIInfo_log() iPlayerSeat Error iPlayerSeat=[%d]", iPlayerSeat);
		return;
	}
	FactoryPlayerNodeDef *nodePlayers = pTableItem->pFactoryPlayers[iPlayerSeat];
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "HZXLGameLogic", "InitERMJAIInfo_log() nodePlayers == NULL");
		return;
	}

	nodePlayers->cTableNumExtra = iPlayerSeat; //玩家固定位置
	nodePlayers->iStatus = PS_WAIT_READY;//等待开始状态
	int iTalbeNum = nodePlayers->cTableNum;
	int iRoomID = nodePlayers->cRoomNum;

	_log(_DEBUG, "HZXLMJ_AIControl", "InitERMJAIInfo_Log() R[%d]T[%d], iUserID[%d], cTableNum[%d], iPlayerSeat[%d]", iRoomID, iTalbeNum, nodePlayers->iUserID, nodePlayers->cTableNum, iPlayerSeat);

	//容错处理，如果进入此函数时没有找到桌子，再找一次
	if (nodePlayers->cTableNum <= 0)
	{
		// 找空桌子
		bool bContinue = false;
		int iTableIndex = -1;
		for (int i = 0; i < MAX_TABLE_NUM; i++)
		{
			pTableItem = (MJGTableItemDef *)GetTableItem(0, i);
			if (!pTableItem)
			{
				continue;
			}
			bContinue = false;
			for (int m = 0; m < m_iMaxPlayerNum; m++)
			{
				if (pTableItem->pFactoryPlayers[m])		// 如果桌子上有人，继续寻找
				{
					bContinue = true;
					break;
				}
			}
			if (bContinue)
			{
				continue;
			}
			iTableIndex = i + 1;

			_log(_DEBUG, "HZXLMJ_AIControl", "InitERMJAIInfo_Log() 00 iUserID[%d] ,iTableIndex[%d]", nodePlayers->iUserID, iTableIndex);

			break;
		}

		_log(_DEBUG, "HZXLMJ_AIControl", "InitERMJAIInfo_Log() iUserID[%d],iTableIndex[%d]", nodePlayers->iUserID, iTableIndex);
		nodePlayers->cTableNum = iTableIndex;
	}

	int cRoomNum = nodePlayers->cRoomNum;
	int iTableNum = nodePlayers->cTableNum;
	char cTableNumExtra = nodePlayers->cTableNumExtra;

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || cRoomNum < 1 || cRoomNum > MAX_ROOM_NUM || cTableNumExtra < 0 || cTableNumExtra >= m_iMaxPlayerNum)
	{
		_log(_ERROR, "GL", "InitERMJAIInfo_Log:[%d][%d][%d] Error Req", cRoomNum, iTableNum, cTableNumExtra);
		return;
	}

	if (nodePlayers->cTableNum == -1)
	{
		char szMsgText[128] = { 0 };
		sprintf(szMsgText, "服务器人数爆满，请稍后再试！");
		SendKickOutWithMsg(nodePlayers, szMsgText, 62, 1);
		_log(_ERROR, "HZXLMJ_AIControl", "InitERMJAIInfo_Log(): 找不到座位 iUserID[%d]", nodePlayers->iUserID);
		return;
	}
	//将正常玩家放在第一个位置上
	pTableItem->pFactoryPlayers[iPlayerSeat] = nodePlayers;						// 第1个是正常玩家节点位置为0
																				//添加3个AI
	int iMainAIPos = -1;
	int iAssistAIPos = -1;
	int iDeputyAIPos = -1;
	for (int i = 0; i < 3; i++)
	{
		if (i == 0)
		{
			//处理主AI-1的位置，只能是普通玩家下家或对家即为1or2
			int iRandPos = rand() % 2 + 1;
			iMainAIPos = iRandPos;
		}
		else if (i == 1)
		{
			int iRandPos = rand() % 3 + 1;
			if (iRandPos == iMainAIPos)
			{
				i--;
			}
			else
			{
				iAssistAIPos = iRandPos;
			}
		}
		else if (i == 2)
		{
			int iRandPos = rand() % 3 + 1;
			if (iRandPos == iMainAIPos || iRandPos == iAssistAIPos)
			{
				i--;
			}
			else
			{
				iDeputyAIPos = iRandPos;
			}
		}
	}
	_log(_ERROR, "HZXLMJ_AIControl", "InitERMJAIInfo_Log(): iMainAIPos[%d] , iAssistAIPos[%d],iDeputyAIPos[%d]", iMainAIPos, iAssistAIPos, iDeputyAIPos);
	for (int i = 1; i < 4; i++)
	{
		int iAIExtra = -1;
		if (i == 1)
		{
			iAIExtra = iMainAIPos;
		}
		else if (i == 2)
		{
			iAIExtra = iAssistAIPos;
		}
		else if (i == 3)
		{
			iAIExtra = iDeputyAIPos;
		}
		_log(_ERROR, "HZXLMJ_AIControl", "InitERMJAIInfo_Log(): iAIExtra [%d]", iAIExtra);
		int iRoomID = pTableItem->pFactoryPlayers[iPlayerSeat]->cRoomNum - 1;
		if (iAIExtra >= 1 && iAIExtra <= 3)
		{
			//添加AI
			pTableItem->pFactoryPlayers[iAIExtra] = (MJGPlayerNodeDef*)GetFreeNode();
			//随机AI信息
			pTableItem->pFactoryPlayers[iAIExtra]->iUserID = nodePlayers->iUserID + (iAIExtra + 1);
			m_pHzxlAICTL->GetAIVirtualNickName(pTableItem, iAIExtra,pTableItem->pFactoryPlayers[iAIExtra]->szNickName, 16);
		
			pTableItem->pFactoryPlayers[iAIExtra]->iMoney = m_pHzxlAICTL->GetAIVirtualMoney(iRoomID);//根据玩家的所在房间生成AI金币
			
			pTableItem->pFactoryPlayers[iAIExtra]->iExp = m_pHzxlAICTL->GetAIVirtualLevel();
			

			pTableItem->pFactoryPlayers[iAIExtra]->iSocketIndex = -1;
			pTableItem->pFactoryPlayers[iAIExtra]->cRoomNum = nodePlayers->cRoomNum;
			pTableItem->pFactoryPlayers[iAIExtra]->cTableNum = nodePlayers->cTableNum;

			pTableItem->pFactoryPlayers[iAIExtra]->cTableNumExtra = iAIExtra;

			pTableItem->pFactoryPlayers[iAIExtra]->iStatus = PS_WAIT_READY;//等待开始状态
			((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[iAIExtra])->iPlayerType = i;
			pTableItem->bAutoHu[iAIExtra] = true;		// 默认听牌后自动胡
		}
	}
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		for (int j = 0; j < m_iMaxPlayerNum; j++)
		{
			if (i == j)
			{
				continue;
			}
			else
			{
				pTableItem->pFactoryPlayers[i]->otherMoney[j].llMoney = pTableItem->pFactoryPlayers[j]->iMoney;
			}
		}
	}
	char cIfSee = 0;
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		MJGTableItemDef *pTmpTableItem = (MJGTableItemDef *)GetTableItem(0, i);
		if (pTmpTableItem->pFactoryPlayers[i])
		{
			if (pTmpTableItem->pFactoryPlayers[i]->iStatus > PS_WAIT_READY)
			{
				cIfSee = 2;//标识是梭哈类游戏的玩家在游戏中加入
				break;
			}
		}
	}
	int iNormalPlayerPos = -1;
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			_log(_DEBUG, "HZXLMJ_AIControl", "InitERMJAIInfo_Log() cTableNum[%d]", pTableItem->pFactoryPlayers[i]->cTableNum);

			((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[i])->bNeedMatchAI = false;
			((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[i])->iWaitAiTime = 0;

			if (((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[i])->iPlayerType == NORMAL_PLAYER)
			{
				iNormalPlayerPos = pTableItem->pFactoryPlayers[i]->cTableNumExtra;
				GetTablePlayerInfo(pTableItem, pTableItem->pFactoryPlayers[i]);
				CLSendPlayerInfoNotice(pTableItem->pFactoryPlayers[i]->iSocketIndex, cIfSee);		// 发送玩家信息(防作弊隐藏昵称)

				SitDownResDef msgRes;
				memset((char*)&msgRes, 0x00, sizeof(SitDownResDef));

				msgRes.cResult = 1;
				msgRes.iTableNum = pTableItem->pFactoryPlayers[i]->cTableNum;
				msgRes.cTableNumExtra = pTableItem->pFactoryPlayers[i]->cTableNumExtra;

				CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgRes, SITDOWN_RES_MSG, sizeof(SitDownResDef));//在上面要先发CLSendPlayerInfoNotice.add at 2/20 by crystal

				CallBackSitSuccedDownRes(pTableItem, pTableItem->pFactoryPlayers[i]);
			}
			//最后通知坐下成功
			if (((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[i])->iPlayerType > 0)			// AI玩家直接准备好
			{
				pTableItem->bIfReady[i] = true;

				_log(_DEBUG, "HZXLMJ_AIControl", "InitERMJAIInfo_Log() READY_NOTICE_MSG: pTableItem->pFactoryPlayers[%d]->cTableNumExtra = %d", i, pTableItem->pFactoryPlayers[i]->cTableNumExtra);
				_log(_DEBUG, "HZXLMJ_AIControl", "InitERMJAIInfo_Log() READY_NOTICE_MSG: pTableItem->pFactoryPlayers[%d]->iSocketIndex = %d", i, pTableItem->pFactoryPlayers[i]->iSocketIndex);
			}
		}
	}
	
	//补发三个AI的ready消息给普通玩家客户端
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			if (i != iNormalPlayerPos)
			{
				ReadyNoticeDef msgNotice;
				msgNotice.cTableNumExtra = pTableItem->pFactoryPlayers[i]->cTableNumExtra;
				CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[iNormalPlayerPos]->iSocketIndex, &msgNotice, READY_NOTICE_MSG, sizeof(ReadyNoticeDef));
			}
		}
	}
}

void HZXLGameLogic::FindEmptyTableWithAI(FactoryPlayerNodeDef * nodePlayers)
{
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "HZXLGameLogic", "FindEmptyTableWithAI_Log() nodePlayer is Null");
		return;
	}
	//_log(_DEBUG, "HZXLGameLogic", "FindEmptyTableWithAI_Log() iUserID[%d]", nodePlayers->iUserID);

	nodePlayers->cTableNum = -1;
	nodePlayers->cTableNumExtra = 0;	//固定与AI对局时，普通玩家座位为0
	nodePlayers->iStatus = PS_WAIT_READY;//等待开始状态

										 //找空桌子
	bool bContinue;
	for (int i = 0; i < MAX_TABLE_NUM; i++)
	{
		MJGTableItemDef *pTableItem = (MJGTableItemDef *)GetTableItem(0, i);
		if (!pTableItem)
		{
			_log(_ERROR, "HZXLGameLogic", "FindEmptyTableWithAI_Log() pTableItem is null ");
			continue;
		}
		bContinue = false;
		for (int m = 0; m < m_iMaxPlayerNum; m++)
		{
			if (pTableItem->pFactoryPlayers[m])		// 如果桌子上有人，继续寻找
			{
				bContinue = true;
				break;
			}
		}
		if (bContinue)
		{
			continue;
		}
		_log(_ERROR, "HZXLGameLogic", "FindEmptyTableWithAI_Log() pTableItem found ");
		nodePlayers->cTableNum = i + 1;

		_log(_DEBUG, "HZXLGameLogic", "FindEmptyTableWithAI_Log() iUserID[%d] cTableNum[%d]", nodePlayers->iUserID, nodePlayers->cTableNum);

		pTableItem->pFactoryPlayers[0] = nodePlayers; //第1个是正常玩家节点
		break;
	}

	int cRoomNum = nodePlayers->cRoomNum;
	int iTableNum = nodePlayers->cTableNum;
	char cTableNumExtra = nodePlayers->cTableNumExtra;
	_log(_DEBUG, "HZXLGameLogic", "FindEmptyTableWithAI_Log() R[%d], T[%d], cExtra[%d], iUserID[%d]", cRoomNum, iTableNum, cTableNumExtra, nodePlayers->iUserID);

	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || cRoomNum < 1 || cRoomNum > MAX_ROOM_NUM || cTableNumExtra < 0 || cTableNumExtra >= m_iMaxPlayerNum)
	{
		_log(_ERROR, "GL", "FindEmptyTableWithAI_Log: R[%d] T[%d] cExtra[%d] iUserID[%d] Error Req", cRoomNum, iTableNum, cTableNumExtra, nodePlayers->iUserID);
		return;
	}
	if (nodePlayers->cTableNum == -1)
	{
		char szMsgText[128] = { 0 };
		sprintf(szMsgText, "服务器人数爆满，请稍后再试！");
		SendKickOutWithMsg(nodePlayers, szMsgText, 62, 1);
		_log(_ERROR, "HZXLGameLogic", "FindEmptyTableWithAI_Log(): 找不到座位 iUserID[%d]", nodePlayers->iUserID);
		return;
	}

	char cIfSee = 0;
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		MJGTableItemDef *pTableItem = (MJGTableItemDef *)GetTableItem(0, i);
		if (pTableItem->pFactoryPlayers[i])
		{
			if (pTableItem->pFactoryPlayers[i]->iStatus > PS_WAIT_READY)
			{
				cIfSee = 2;//标识是梭哈类游戏的玩家在游戏中加入
				break;
			}
		}
	}

	// 找到了座位，真实玩家坐下ER_SEN,AI玩家在定时器中坐下
	FactoryTableItemDef *pTableItem = GetTableItem(cRoomNum - 1, iTableNum - 1);
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			if (((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[i])->iPlayerType == NORMAL_PLAYER)
			{
				GetTablePlayerInfo(pTableItem, pTableItem->pFactoryPlayers[i]);
				CLSendPlayerInfoNotice(pTableItem->pFactoryPlayers[i]->iSocketIndex, cIfSee);		// 发送玩家信息(防作弊隐藏昵称)

				SitDownResDef msgRes;
				memset((char*)&msgRes, 0x00, sizeof(SitDownResDef));

				msgRes.cResult = 1;
				msgRes.iTableNum = pTableItem->pFactoryPlayers[i]->cTableNum;
				msgRes.cTableNumExtra = pTableItem->pFactoryPlayers[i]->cTableNumExtra;

				CLSendSimpleNetMessage(pTableItem->pFactoryPlayers[i]->iSocketIndex, &msgRes, SITDOWN_RES_MSG, sizeof(SitDownResDef));//在上面要先发CLSendPlayerInfoNotice.add at 2/20 by crystal

				CallBackSitSuccedDownRes(pTableItem, pTableItem->pFactoryPlayers[i]);

				_log(_DEBUG, "HZXLGameLogic", "FindEmptyTableWithAI_Log() iTableNum[%d]", iTableNum);

				((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[i])->bNeedMatchAI = true;				// 标识玩家是否需要匹配AI
			}
		}
	}
}

void HZXLGameLogic::OneSecTime()
{
	if (m_pHzxlAICTL->m_iIfOpenAI == 1)
	{
		OnTimeAIPlayerMsg();

		OnTimeMatchAiPlayer();
	}

	if (m_bOpenRecharge)
		OnTimeCheckOverTime();
}

/***
每秒定时检查AI玩家消息，消息类型包括：换牌消息、定缺消息、出牌消息、特殊操作消息(碰杠胡)
***/
void HZXLGameLogic::OnTimeAIPlayerMsg()
{
	int iLimitNum = 0;
	std::map<MJGPlayerNodeDef*, vector<AIDelayCardMsgDef> >::iterator it;
	for (it = m_pHzxlAICTL->m_mapAIOperationDelayMsg.begin(); it != m_pHzxlAICTL->m_mapAIOperationDelayMsg.end(); it++)
	{

		if (it->second.size() == 0)
			continue;
		it->second[0].iDelayMilliSeconds--;						// 最近的那条消息时间减1

		int iTableNum = it->first->cTableNum;
		int iRoomNum = it->first->cRoomNum;
		MJGTableItemDef* tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);

		iLimitNum = 0;
		while (it->second.size() > 0 && iLimitNum < 10)			// 1秒执行10次，超过10次等待下一秒执行
		{
			if (it->second[0].iDelayMilliSeconds > 0)
				break;

			int iMsgType = it->second[0].iMsgType;
			if (iMsgType == AI_DELAY_TYPE_CHANGE_CARD)			//AI玩家请求换牌
			{
				it->first->iStatus = PS_WAIT_CHANGE_CARD;

				ChangeCardsReqDef msgChangeCards;
				memset(&msgChangeCards, 0, sizeof(ChangeCardsReqDef));
				memcpy(&msgChangeCards, it->second[0].szBuffer, sizeof(ChangeCardsReqDef));

				_log(_DEBUG, "HZXLGameLogic", "OnTimeAIPlayerMsg_Log(): ChangeCards--cTableNumExtra = %d, msgChangeCards.cCard[0]= %d,msgChangeCards.cCard[1]= %d,msgChangeCards.cCard[2]= %d", it->first->cTableNumExtra, msgChangeCards.cCards[0], msgChangeCards.cCards[1], msgChangeCards.cCards[2]);

				HandleChangeCardsReqsAI(it->first, &msgChangeCards);
			}
			else if (iMsgType == AI_DELAY_TYPE_CONFIRM_QUE)		//AI玩家请求定缺
			{
				it->first->iStatus = PS_WAIT_CONFIRM_QUE;

				ConfirmQueReqDef msgConfirmQue;
				memset(&msgConfirmQue, 0, sizeof(ConfirmQueReqDef));
				memcpy(&msgConfirmQue, it->second[0].szBuffer, sizeof(ConfirmQueReqDef));

				_log(_DEBUG, "HZXLGameLogic", "OnTimeAIPlayerMsg_Log(): ConfirmQue--cTableNumExtra = %d,msg ConfirmQue.iQueType=[%d]", it->first->cTableNumExtra, msgConfirmQue.iQueType);
				HandleConfirmQueReqAI(it->first, &msgConfirmQue);
			}
			else if (iMsgType == AI_DELAY_TYPE_SEND_CARD)			//AI玩家请求出牌
			{
				it->first->iStatus = PS_WAIT_SEND;
				tableItem->iCurSendPlayer = it->first->cTableNumExtra;

				SendCardsReqDef msgSendCards;
				memset(&msgSendCards, 0, sizeof(SendCardsReqDef));
				memcpy(&msgSendCards, it->second[0].szBuffer, sizeof(SendCardsReqDef));

				_log(_DEBUG, "HZXLGameLogic", "OnTimeAIPlayerMsg_Log(): SendCards--cTableNumExtra = %d,msgSendCards.cCard=[%d],msgSendCards.cIfTing=[%d]", it->first->cTableNumExtra, msgSendCards.cCard, msgSendCards.cIfTing);
				HandleSendCardsReqAI(it->first, &msgSendCards);
			}
			else if (iMsgType == AI_DELAY_TYPE_SPECIAL_CARD)			//AI玩家请求特殊操作----碰杠胡操作
			{
				SpecialCardsReqDef msgSpecialCard;
				memset(&msgSpecialCard, 0, sizeof(SpecialCardsReqDef));
				memcpy(&msgSpecialCard, it->second[0].szBuffer, sizeof(SpecialCardsReqDef));

				_log(_DEBUG, "HZXLGameLogic", "OnTimeAISpecialCard_Log(): cTableNumExtra = %d, msgSpecialCard.iSpecialFlag=%d,msgSpecialCard.cCards[%d][%d][%d][%d]", it->first->cTableNumExtra, msgSpecialCard.iSpecialFlag, msgSpecialCard.cCards[0], msgSpecialCard.cCards[1], msgSpecialCard.cCards[2], msgSpecialCard.cCards[3]);

				HandleSpecialCardsReqAI(it->first, &msgSpecialCard);
			}

			it->second.erase(it->second.begin());
			iLimitNum++;

			_log(_DEBUG, "HZXLGameLogic", "22 OnTimeAIPlayerMsg_Log(): it->second.size() = %d, iLimitNum = %d", it->second.size(), iLimitNum);
		}

	}
	for (it = m_pHzxlAICTL->m_mapAIOperationDelayMsg.begin(); it != m_pHzxlAICTL->m_mapAIOperationDelayMsg.end(); it++)
	{
		int iTableNum = it->first->cTableNum;
		if (iTableNum <= 0)
		{
			m_pHzxlAICTL->m_mapAIOperationDelayMsg.erase(it);
			break;
		}
	}
}

void HZXLGameLogic::OnTimeMatchAiPlayer()
{
	for (int i = 0; i < MAX_TABLE_NUM; i++)
	{
		MJGTableItemDef *pTableItem = (MJGTableItemDef *)GetTableItem(0, i);
		if (!pTableItem)
		{
			continue;
		}
		int iPlayerCnt = 0;
		bool bNeedWaitAi = false;
		int iPlayerSeat = -1;
		for (int j = 0; j < m_iMaxPlayerNum; j++)
		{
			if (pTableItem->pFactoryPlayers[j])
			{
				int iUserID = ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[j])->iUserID;
				int iPlayerType = ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[j])->iPlayerType;
				//_log(_DEBUG, "HZXLGameLogic", "FindEmptyTableWithAI_Log() iPlayerType[%d], iUserID[%d]", iPlayerType, iUserID);
				if (((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[j])->iPlayerType == NORMAL_PLAYER && ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[j])->iStatus == PS_WAIT_READY)	// 某一桌只有一个等待AI的真实玩家
				{
					iPlayerCnt++;
					if (((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[j])->bNeedMatchAI && ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[j])->iWaitAiTime > 0 && ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[j])->tmSitDownTime > 0)
					{
						bNeedWaitAi = true;
						iPlayerSeat = j;
					}
				}
			}
		}

		if (iPlayerSeat >= 0 && iPlayerSeat < m_iMaxPlayerNum)
		{
			//_log(_DEBUG, "HZXLGameLogic", "FindEmptyTableWithAI_Log() iPlayerSeat[%d],bNeedWaitAi[%d]", iPlayerSeat, bNeedWaitAi);
			if (pTableItem->pFactoryPlayers[iPlayerSeat] && bNeedWaitAi == true && iPlayerCnt == 1 && iPlayerSeat > -1)	// 该桌玩家只有一个，且存在等待匹配AI的玩家
			{
				time_t now_time;
				time(&now_time);
				int iDiffTime = now_time - ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[iPlayerSeat])->tmSitDownTime;
				if (iDiffTime > ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[iPlayerSeat])->iWaitAiTime)
				{
					_log(_DEBUG, "HZXLMJ_AIControl", "OnTimeMatchAiPlayer_Log(): i=%d, iDiffTime[%d], cTableNum[%d], iUserID[%d],iPlayerSeat[%d]",
						i, iDiffTime, ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[iPlayerSeat])->cTableNum, ((MJGPlayerNodeDef*)pTableItem->pFactoryPlayers[iPlayerSeat])->iUserID, iPlayerSeat);
					InitERMJAIInfo(pTableItem, iPlayerSeat);
				}
			}
		}
	}
}

void HZXLGameLogic::ReleaseAIPlayerNode(MJGTableItemDef * tableItem, MJGPlayerNodeDef * pPlayers)
{
	if (pPlayers)
	{
		_log(_DEBUG, "HZXLGameLogic", " ReleaseAIPlayerNode_Log() ");
		int iTableNumExtra = pPlayers->cTableNumExtra;
		MJGPlayerNodeDef* pNodeTemp = pPlayers;
		RemoveTablePlayer(tableItem, iTableNumExtra);
		ReleaseNode(pNodeTemp);
	}
}

void HZXLGameLogic::HandleChangeCardsReqsAI(MJGPlayerNodeDef * pPlayers, void * pMsgData)
{
	//此处校验是否三个AI都已经发送过了
	pPlayers->bChangeCardReq = true;
	memset(pPlayers->cChangeIndex,0, sizeof(pPlayers->cChangeIndex));
	memset(pPlayers->cChangeReqCard, 0, sizeof(pPlayers->cChangeReqCard));

	int  iTableNum = pPlayers->cTableNum;                   //找出所在的桌
	char iTableNumExtra = pPlayers->cTableNumExtra;
	char iRoomNum = pPlayers->cRoomNum;
	MJGTableItemDef *tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleChangeCardsReqsAI_Log:[%d][%d]", iTableNum, iTableNumExtra);
		return;
	}
	_log(_ERROR, "HZXLGameLogic", "HandleChangeCardsReqsAI_Log:AI[%d]", pPlayers->iPlayerType);
	
	SureChangeCardNoticeDef msgNotice;
	memset(&msgNotice, 0, sizeof(SureChangeCardNoticeDef));
	msgNotice.iTableNumExtra = iTableNumExtra;
	memcpy(msgNotice.cSourceCards, pPlayers->cChangeReqCard, sizeof(msgNotice.cSourceCards));
	
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] != NULL)
		{
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotice, SURE_CHANGE_CARD_NOTICE_MSG, sizeof(SureChangeCardNoticeDef));
		}
	}

	int iAIFinishChangeCard = 0;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->iPlayerType != NORMAL_PLAYER)
		{
			if (tableItem->pPlayers[i]->bChangeCardReq)
			{
				iAIFinishChangeCard++;
			}
		}
	}
	_log(_DEBUG, "HZXLGameLogic", "HandleChangeCardsReqsAI_Log:iAIFinishChangeCard[%d]", iAIFinishChangeCard);
	if (iAIFinishChangeCard == 3)
	{
		CallBackChangeCard(tableItem);
	}
}

void HZXLGameLogic::HandleConfirmQueReqAI(MJGPlayerNodeDef * pPlayers, void * pMsgData)
{
	if (!pPlayers)
	{
		_log(_ERROR, "HZXLGameLogic", "●●●● HandleConfirmQueReqAI: nodePlayers is NULL ●●●●");
		return;
	}
	if (pPlayers->iPlayerType <= NORMAL_PLAYER)
	{
		_log(_ERROR, "HZXLGameLogic", "●●●● HandleConfirmQueReqAI: nodePlayers is not AI ●●●●");
		return;
	}
	if (pPlayers->iStatus != PS_WAIT_CONFIRM_QUE)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleConfirmQueReqAI_Log(): nodePlayers Status Error=[%d]", pPlayers->iStatus);
		return;
	}
	if (pPlayers->bConfirmQueReq)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleConfirmQueReqAI_Log(): nodePlayers Already Confirm Que");
		return;
	}

	int  iTableNum = pPlayers->cTableNum;                   //找出所在的桌
	char iTableNumExtra = pPlayers->cTableNumExtra;
	char iRoomNum = pPlayers->cRoomNum;

	MJGTableItemDef *tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleConfirmQueReqAI_Log:[%d][%d]", iTableNum, iTableNumExtra);
		return;
	}
	//设置AI节点对应字段
	ConfirmQueReqDef *pMsg = (ConfirmQueReqDef*)pMsgData;
	int iQueType = pMsg->iQueType;
	if (iQueType >= 0 && iQueType < 3)
	{
		pPlayers->iQueType = iQueType;
	}
	pPlayers->bConfirmQueReq = true;
	pPlayers->iKickOutTime = 0;

	//统计当前已完成定缺人数
	int iHasConfirmQueCount = 0;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i]->bConfirmQueReq)
		{
			iHasConfirmQueCount++;
		}
	}
	//发送消息
	ConfirmQueNoticeDef msgNotice;
	memset(&msgNotice, 0, sizeof(ConfirmQueNoticeDef));
	msgNotice.iQueType = iQueType;
	msgNotice.iCurCompleteQueCnt = iHasConfirmQueCount;
	msgNotice.cTableNumExtra = iTableNumExtra;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
		{
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &msgNotice, CONFIRM_QUE_NOTICE_MSG, sizeof(ConfirmQueNoticeDef));
		}
	}
	//如果都已经完成定缺
	if(iHasConfirmQueCount == PLAYER_NUM)
	{
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i])
			{
				tableItem->pPlayers[i]->iStatus = PS_WAIT_SEND;
			}
		}

		//给玩家发牌
		if (tableItem->pPlayers[tableItem->iZhuangPos])
		{
			_log(_ERROR, "HZXLGameLogic", "HandleConfirmQueReqAI_Log:iCurMoPlayer[%d]iZhuangPos[%d]", tableItem->iCurMoPlayer, tableItem->iZhuangPos);
			tableItem->iCurMoPlayer = tableItem->iZhuangPos;
			_log(_ERROR, "HZXLGameLogic", "HandleConfirmQueReqAI_Log:iCurMoPlayer[%d]iZhuangPos[%d]", tableItem->iCurMoPlayer, tableItem->iZhuangPos);
			if (tableItem->pPlayers[tableItem->iZhuangPos]->iPlayerType !=  NORMAL_PLAYER)
			{
				//庄家是AI玩家，不考虑AI玩家的天胡，所以直接出牌，创建出牌消息
				//发送一次摸牌牌通知
				SendCardsServerDef pSendMsg;
				memset(&pSendMsg, 0, sizeof(SendCardsServerDef));
				pSendMsg.cTableNumExtra = tableItem->iZhuangPos;

				pSendMsg.cFirst = 1;

				for (int i = 0; i < PLAYER_NUM; i++)
				{
					if (tableItem->pPlayers[i] && tableItem->pPlayers[i]->bTempMidway == false)
					{
						CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pSendMsg, MJG_MO_CARD_SERVER, sizeof(SendCardsServerDef));
					}
				}
				m_pHzxlAICTL->CreateAISendCardReq(tableItem, tableItem->pPlayers[tableItem->iZhuangPos]);
			}
			else
			{
				//庄家是普通玩家，考虑玩家天胡的可能性
				SendCardToPlayer(tableItem->pPlayers[tableItem->iZhuangPos], tableItem, false, true);
			}
		}
	}
}
//处理AI节点的出牌
void HZXLGameLogic::HandleSendCardsReqAI(MJGPlayerNodeDef * pPlayers, void * pMsgData)
{
	if (!pPlayers)
	{
		_log(_ERROR, "HZXLGameLogic", "●●●● HandleSendCardsReqAI: nodePlayers is NULL ●●●●");
		return;
	}

	int  iTableNum = pPlayers->cTableNum;                   //找出所在的桌
	char iTableNumExtra = pPlayers->cTableNumExtra;
	char iRoomNum = pPlayers->cRoomNum;
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleSendCardsReq:R[%d]T[%d] I[%d]", iRoomNum, iTableNum, iTableNumExtra);
		return;
	}
	MJGTableItemDef *tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	SendCardsReqDef *pSendCard = (SendCardsReqDef *)pMsgData;
	char cSendCard = pSendCard->cCard;
	//如果当前摸牌的玩家 不是打牌的这个玩家  踢出玩家
	if (tableItem->iCurMoPlayer != pPlayers->cTableNumExtra)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleSendCardsReqAI:R[%d]T[%d] [%s][%d], illegal Player[%d], Real Player[%d], CardType[%d]Value[%d]",
			iRoomNum, iTableNum, pPlayers->szNickName, pPlayers->iUserID, pPlayers->cTableNumExtra, tableItem->iCurMoPlayer, cSendCard >> 4, cSendCard & 0xf);
		SendKickOutWithMsg(pPlayers, "不轮到当前玩家出牌", 1301, 1);
		return;
	}
	//检查当前AI玩家手中是否有这张牌
	bool bFind = false;
	for (int i = 0; i < pPlayers->iHandCardsNum; i++)
	{
		if (pPlayers->cHandCards[i] == pSendCard->cCard)
		{
			bFind = true;
			break;
		}
	}
	if (bFind == false)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleSendCardsReqAI:R[%d]T[%d] [%s][%d], Player Hand Not Exist Card: CardType[%d]Value[%d]", iRoomNum, iTableNum, pPlayers->szNickName, pPlayers->iUserID, cSendCard >> 4, cSendCard & 0xf);
		SendKickOutWithMsg(pPlayers, "您手中没有这张牌", 1301, 1);
		return;
	}
	//AI玩家可以胡牌但是过胡，重置字段
	if (pPlayers->iSpecialFlag >> 6)
	{
		pPlayers->bGuoHu = true;
		pPlayers->iSpecialFlag = 0;
		pPlayers->iWinFlag = 0;
		pPlayers->iHuFlag = 0;
		tableItem->iWillHuNum = 0;
	}
	//针对AI的判断，如果当前AI不是主AI-1，那么需要重新计算AI的听胡牌
	if (pPlayers->iPlayerType != MIAN_AI_PLAYER)
	{
		vector<char> vcCards;
		for (int i = 0; i < pPlayers->iHandCardsNum; i++)
		{
			if (pPlayers->cHandCards[i] != 0)
			{
				//打印此时AI节点手牌，注意此时要出的牌还没删除
				//_log(_DEBUG, "HZXLMJ_AIControl", "HandleSendCardsReqAI_log() AI[%d] Pos[%d] HandCards[%d]", pPlayers->iPlayerType, pPlayers->cTableNumExtra, pPlayers->cHandCards[i]);
				vcCards.push_back(pPlayers->cHandCards[i]);
			}
		}
		map<char, vector<char> > mpTing;
		map<char, map<char, int> > mpTingFan;

		pPlayers->bTing = false;
		pPlayers->iTingHuFan = 0;
		int iFlag = HZXLGameLogic::JudgeSpecialTing(pPlayers, tableItem, pPlayers->resultType, vcCards, mpTing, mpTingFan); // 玩家出牌后，判断能否听牌
		_log(_DEBUG, "HZXLMJ_AIControl", "HandleSendCardsReqAI_log() iFlag = %d, cSendCard = %d, mpTing[cSendCard].size() = %d", iFlag, cSendCard, mpTing[cSendCard].size());

		if (iFlag > 0 && !mpTing[cSendCard].empty())
		{
			vector<char>().swap(pPlayers->vcSendTingCard);
			//给玩家听的牌 赋值
			pPlayers->bTing = true;
			vector<char>().swap(pPlayers->vcTingCard);
			pPlayers->vcTingCard.insert(pPlayers->vcTingCard.begin(), mpTing[cSendCard].begin(), mpTing[cSendCard].end());
			map<char, int>().swap(pPlayers->mpTingFan);
			pPlayers->mpTingFan = mpTingFan[cSendCard];

			int iTempMaxFan = 0;
			map<char, int>::iterator pos = mpTingFan[cSendCard].begin();
			while (pos != mpTingFan[cSendCard].end())
			{
				if (pos->second > iTempMaxFan)
					iTempMaxFan = pos->second;
				pos++;
			}
			pPlayers->iTingHuFan = iTempMaxFan;
			//桌子节点上听牌玩家保留
			for (int i = 0; i < PLAYER_NUM; i++)
			{
				if (tableItem->iTingPlyerPos[i] == -1)
				{
					tableItem->iTingPlyerPos[i] = pPlayers->cTableNumExtra;
					break;
				}
			}
		}
	}

	//给出牌的位置赋值
	tableItem->iWillHuNum = 0;
	tableItem->cCurMoCard = 0;
	tableItem->iCurMoPlayer = -1;
	tableItem->iCurSendPlayer = pPlayers->cTableNumExtra;
	tableItem->iLastSendPlayer = pPlayers->cTableNumExtra;
	//给当前出牌赋值
	tableItem->cTableCard = cSendCard;
	pPlayers->cSendCards[pPlayers->iAllSendCardsNum] = tableItem->cTableCard;
	pPlayers->iAllSendCardsNum++;

	//将玩家手中的牌去掉
	for (int i = 0; i < pPlayers->iHandCardsNum; i++)
	{
		if (pPlayers->cHandCards[i] == cSendCard)
		{
			for (int j = i; j < pPlayers->iHandCardsNum - 1; j++)
			{
				pPlayers->cHandCards[j] = pPlayers->cHandCards[j + 1];
			}
			pPlayers->cHandCards[pPlayers->iHandCardsNum - 1] = 0;
			pPlayers->iHandCardsNum--;
			break;
		}
	}
	if (cSendCard >> 4 != pPlayers->iQueType)
	{
		int iAllCard[5][10] = { 0 };
		for (int i = 0; i < pPlayers->iHandCardsNum; i++)
		{
			int iType = pPlayers->cHandCards[i] >> 4;
			int iValue = pPlayers->cHandCards[i] & 0x000f;
			//手上还有定缺的牌 直接返回 不能胡
			if (iType == pPlayers->iQueType)
			{
				continue;
			}
			iAllCard[iType][0] ++;
			iAllCard[iType][iValue] ++;
		}
	}

	pPlayers->iSpecialFlag = 0;
	pPlayers->iKickOutTime = 0;

	//给玩家发送AI出牌通知
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i]->iPlayerType ==  NORMAL_PLAYER)
		{
			SendCardsNoticeDef pMsgNotice;
			memset(&pMsgNotice, 0, sizeof(SendCardsNoticeDef));
			pMsgNotice.cTableNumExtra = pPlayers->cTableNumExtra;
			pMsgNotice.cCard = tableItem->cTableCard;
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &pMsgNotice, MJG_SEND_CARD_RES, sizeof(SendCardsNoticeDef));
		}
	}
	
	//判断当前出牌能否造成放炮胡的情况
	for (int i = 1; i < PLAYER_NUM; i++)
	{
		//按照当前玩家的顺序 判断玩家的是否可以胡
		int iTableExtra = (tableItem->iCurSendPlayer + i) % PLAYER_NUM;
		if (tableItem->pPlayers[iTableExtra] != NULL && tableItem->pPlayers[iTableExtra]->cPoChan == 0)
		{
			char cFanResult[MJ_FAN_TYPE_CNT];
			memset(cFanResult, 0, sizeof(cFanResult));
			int iHuFan = HZXLGameLogic::JudgeSpecialHu(tableItem->pPlayers[iTableExtra], tableItem, cFanResult);	//点炮胡判断
			if (iHuFan > 0)
			{
				if (tableItem->pPlayers[iTableExtra]->bGuoHu)
				{
					if (iHuFan > tableItem->pPlayers[iTableExtra]->iHuFan)
					{
						tableItem->pPlayers[iTableExtra]->iHuFan = iHuFan;
						tableItem->pPlayers[iTableExtra]->bGuoHu = false;
						memcpy(tableItem->pPlayers[iTableExtra]->cFanResult, cFanResult, sizeof(cFanResult));
						//_log(_DEBUG, "HZXLGameLogic", "HandleSendCard_Log(): bGuoHu   00000");
					}
					else
					{
						//_log(_DEBUG, "HZXLGameLogic", "HandleSendCard_Log(): bGuoHu   11111");
						tableItem->pPlayers[iTableExtra]->iHuFan = 0;
						memset(tableItem->pPlayers[iTableExtra]->cFanResult, 0, sizeof(tableItem->pPlayers[iTableExtra]->cFanResult));
						continue;
					}
				}
				else
				{
					tableItem->pPlayers[iTableExtra]->iHuFan = iHuFan;
					memcpy(tableItem->pPlayers[iTableExtra]->cFanResult, cFanResult, sizeof(cFanResult));
					//_log(_DEBUG, "HZXLGameLogic", "HandleSendCard_Log(): bGuoHu   22222");
				}
				tableItem->pPlayers[iTableExtra]->iSpecialFlag = 1 << 5;

				//tableItem->iYiPaoDuoXiangPlayer[iTableExtra] = 1;//0初值  1可以胡牌  2胡牌了 3放弃胡牌
				tableItem->iWillHuNum++;
			}
			else
			{
				tableItem->pPlayers[iTableExtra]->iHuFan = 0;
				memset(&tableItem->pPlayers[iTableExtra]->cFanResult, 0, sizeof(tableItem->pPlayers[iTableExtra]->cFanResult));
			}
		}
	}
	for (int i = 1; i < PLAYER_NUM; i++)
	{
		//按照当前玩家的顺序 判断其他玩家的是否可以碰杠
		int iTableExtra = (tableItem->iCurSendPlayer + i);
		if (iTableExtra >= PLAYER_NUM)
			iTableExtra = iTableExtra - PLAYER_NUM;

		if (tableItem->pPlayers[iTableExtra] && tableItem->pPlayers[iTableExtra]->cPoChan == 0)
		{
			int iSpecialFlag = 0;

			//判断其他玩家的碰杠状态
			int iFlag = HZXLGameLogic::JudgeSpecialPeng(tableItem->pPlayers[iTableExtra], tableItem);   // 0 1
			iSpecialFlag |= iFlag << 1;

			int iGangNum = 0;
			char cGangCard[14] = { 0 };
			char cGangType[14] = { 0 };
			iFlag = HZXLGameLogic::JudgeSpecialGang(tableItem->pPlayers[iTableExtra], tableItem, iGangNum, cGangCard, cGangType);
			iSpecialFlag |= iFlag << 2;
			_log(_DEBUG, "xlGameLogic", "HandleSendCardAI: Gang：------iSpecialFlag[%d]-----------cTableCard[%d] --------iTableExtra[%d]", iSpecialFlag, tableItem->cTableCard, iTableExtra);
			tableItem->pPlayers[iTableExtra]->iSpecialFlag |= iSpecialFlag;
		}
	}
	SendSpecialServerToPlayer(tableItem);
}

void HZXLGameLogic::HandleSpecialCardsReqAI(MJGPlayerNodeDef * pPlayers, void * pMsgData)
{
	//处理AI的特殊操作消息
	int  iTableNum = pPlayers->cTableNum;                   //找出所在的桌
	char iTableNumExtra = pPlayers->cTableNumExtra;
	char iRoomNum = pPlayers->cRoomNum;
	if (iTableNum < 1 || iTableNum > MAX_TABLE_NUM || iRoomNum < 1 || iRoomNum > MAX_ROOM_NUM || iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleSpecialCardsReqAI:R[%d]T[%d] I[%d]", iRoomNum, iTableNum, iTableNumExtra);
		return;
	}
	MJGTableItemDef* tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	SpecialCardsReqDef * pReq = (SpecialCardsReqDef *)pMsgData;
	int iSpecialFlag = pReq->iSpecialFlag;
	int iNormalPlayerPos = -1;
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i]->iPlayerType ==  NORMAL_PLAYER)
		{
			iNormalPlayerPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
	}
	if (iSpecialFlag & 0x0002)
	{
		_log(_DEBUG, "HZXLGameLogic", "AI碰牌 Peng Pai R[%d]T[%d]  nodePlayer->userID = %d  ,iTableNumExtra = %d", iRoomNum, iTableNum, tableItem->pPlayers[iNormalPlayerPos]->iUserID, iTableNumExtra);
		HandlePeng(pPlayers, tableItem, pMsgData);
		return;
	}

	if (iSpecialFlag & 0x000C)
	{
		_log(_DEBUG, "HZXLGameLogic", "AI杠牌 Gang Pai R[%d]T[%d]  nodePlayer->userID = %d  ,iTableNumExtra = %d", iRoomNum, iTableNum, tableItem->pPlayers[iNormalPlayerPos]->iUserID, iTableNumExtra);
		HandleGang(pPlayers, tableItem, pMsgData);
		return;
	}

	if (iSpecialFlag >> 5)
	{
		_log(_DEBUG, "HZXLGameLogic", "AI胡牌  Hu Pai R[%d]T[%d]  nodePlayer->userID = %d  ,iTableNumExtra = %d", iRoomNum, iTableNum, tableItem->pPlayers[iNormalPlayerPos]->iUserID, iTableNumExtra);
		HandleHu(pPlayers, tableItem, pMsgData);		// 客户端请求胡牌
		return;
	}

	if (iSpecialFlag == 0)
	{
		_log(_DEBUG, "HZXLGameLogic", "AI弃牌 Qi Pai R[%d]T[%d]  nodePlayer->userID = %d  ,iTableNumExtra = %d", iRoomNum, iTableNum, tableItem->pPlayers[iNormalPlayerPos]->iUserID, iTableNumExtra);
		HandleQi(pPlayers, tableItem, pMsgData);
		return;
	}
}

void HZXLGameLogic::GetControlCardFromRadius()
{
	//待重新调整控制牌部分相关内容
	/*_log(_DEBUG, "HZXLGameLogic", "GetControlCardFromRadius: Access Successful");
	GetHZXLControlCardReq msg;
	memset(&msg, 0, sizeof(GetHZXLControlCardReq));
	msg.msgHeadInfo.cMsgType = GET_HZXL_CONTROL_CARD_MSG;
	msg.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msg.msgHeadInfo.iMsgBodyLen = 0;

	EventMsgRadiusDef msgRD;
	memset(&msgRD, 0, sizeof(EventMsgRadiusDef));
	msgRD.iDataLen = sizeof(GetHZXLControlCardReq);
	memcpy(msgRD.msgData, &msg, sizeof(GetHZXLControlCardReq));

	if (m_pMQByRadius)
	{
		_log(_DEBUG, "HZXLGameLogic", "GetControlCardFromRadius: SetMsg Successfully");
		m_pMQByRadius->EnQueue(&msgRD, sizeof(EventMsgRadiusDef));
	}*/
}

void HZXLGameLogic::HandleGetControlCardMsg(void * pMsgData)
{
	_log(_DEBUG, "HZXLGameLogic", "GetControlCardFromRadius: GET MSG Successful");

	GetHZXLControlCardRes *msg = (GetHZXLControlCardRes*)pMsgData;

	HZXLControlCards* control = (HZXLControlCards*)((char*)pMsgData + sizeof(GetHZXLControlCardRes));
	vector<HZXLControlCards>vcCTLCards;
	_log(_DEBUG, "HZXLGameLogic", "HandleGetControlCardMsg num [%d]", msg->iConfNum);
	for (int i = 0; i < msg->iConfNum; i++)
	{
		vcCTLCards.push_back(*control);
		control++;
	}
	m_pHzxlAICTL->SetControlCard(vcCTLCards);

}



void * HZXLGameLogic::GetTmpFreeNode()
{
	_log(_DEBUG, "HZXLGameLogic", "GetTmpFreeNode_ Begin");

	MJGPlayerNodeDef *nodePlayer;
	if (m_vTmpPlayerNode.empty())
	{
		nodePlayer = new MJGPlayerNodeDef;
	}
	else
	{
		nodePlayer = m_vTmpPlayerNode.at(0);
		m_vTmpPlayerNode.erase(m_vTmpPlayerNode.begin());
	}

	if (nodePlayer)
	{
		memset(nodePlayer, 0, sizeof(FactoryPlayerNodeDef));
		nodePlayer->Reset();
	}
	else
	{
		_log(_ERROR, "HZXLGameLogic", "GetTmpFreeNode_: Cannot get free nodePlayer.");
		return NULL;
	}

	return (void*)nodePlayer;
}

void HZXLGameLogic::ReleaseTmpNode(void * pNode)
{
	MJGPlayerNodeDef *pCurNode = (MJGPlayerNodeDef*)pNode;
	_log(_DEBUG, "HZXLGameLogic", "ReleaseTmpNode_ Begin UID[%d]", pCurNode->iUserID);
	m_vTmpPlayerNode.push_back(pCurNode);

	_log(_DEBUG, "HZXLGameLogic", "ReleaseTmpNode_ End");
}

// 游戏中刷新用户信息的回调 iType为0表示成功 非0值表示错误类型
void HZXLGameLogic::CallBackGameRefreshUserInfo(FactoryPlayerNodeDef *nodePlayers, int iErrorType)
{
	if (!m_bOpenRecharge)
		return;

	MJGPlayerNodeDef *nodePlayer = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&nodePlayers->iUserID), sizeof(int)));
	if (nodePlayers == NULL || nodePlayer == NULL)
	{
		_log(_ERROR, "HZXLGameLogic", "CallBackGameRefreshUserInfo_log() nodePlayers == NULL");
		return;
	}

	if (iErrorType != 0)
	{
		_log(_ERROR, "HZXLGameLogic", "CallBackGameRefreshUserInfo_log() iErrorType[%d]", iErrorType);
		return;
	}

	int cRoomNum = nodePlayer->cRoomNum;
	int iTableNum = nodePlayer->cTableNum;
	MJGTableItemDef *pTableItem = (MJGTableItemDef*)GetTableItem(cRoomNum - 1, iTableNum - 1);
	long long iBaseScore = stSysConfBaseInfo.stRoom[cRoomNum - 1].iBasePoint;
	long long iTableMoney = stSysConfBaseInfo.stRoom[cRoomNum - 1].iTableMoney;

	if (pTableItem == NULL)
	{
		_log(_ERROR, "HZXLGameLogic", "CallBGRefI_log pTableItem == NULL, UID[%d]", nodePlayer->iUserID);
		return;
	}

	_log(_DEBUG, "HZXLGameLogic", "CallBGRefI_log() UID[%d]iMoney[%lld]iCurWinMoney[%lld]iBaseScore[%lld]cPoChan[%d]",
		nodePlayer->iUserID, nodePlayer->iMoney, nodePlayer->iCurWinMoney, iBaseScore, nodePlayer->cPoChan);

	UpdatePlayerInfoDef updatePlayerInfoMsg;
	// 给每个玩家广播该玩家的金币数量
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		long long  iPlayerMoney = 0;
		memset(&updatePlayerInfoMsg, 0, sizeof(UpdatePlayerInfoDef));

		if (i == nodePlayer->cTableNumExtra)
		{
			updatePlayerInfoMsg.cUpdateTableNumExtra = i;
			updatePlayerInfoMsg.iMoney = nodePlayer->iMoney + nodePlayer->iCurWinMoney;
			if(nodePlayer->cPoChan)
				updatePlayerInfoMsg.iMoney = nodePlayer->iMoney + nodePlayer->iCurWinMoney - iTableMoney;

			CLSendSimpleNetMessage(nodePlayer->iSocketIndex, &updatePlayerInfoMsg, HZXLMJ_UPDATE_PLAYER_INFO, sizeof(UpdatePlayerInfoDef));
		}
		else
		{
			if (pTableItem->pPlayers[i] && iBaseScore > 0)
			{
				int iRoomIndex = pTableItem->pPlayers[i]->cRoomNum - 1;
				long long iPlayerBaseScore = stSysConfBaseInfo.stRoom[iRoomIndex].iBasePoint;
				long long iTableMoney = stSysConfBaseInfo.stRoom[iRoomIndex].iTableMoney;
				long long iShowMoney = nodePlayer->iMoney * iPlayerBaseScore / iBaseScore;
				int iResultShowMoney = iShowMoney;
				pTableItem->pPlayers[i]->otherMoney[nodePlayer->cTableNumExtra].llMoney = iResultShowMoney;
				iPlayerMoney = iResultShowMoney;

				updatePlayerInfoMsg.cUpdateTableNumExtra = nodePlayer->cTableNumExtra;
				updatePlayerInfoMsg.iMoney = iPlayerMoney + pTableItem->pPlayers[i]->iAllUserCurWinMoney[nodePlayer->cTableNumExtra];
				if (nodePlayer->cPoChan)
					updatePlayerInfoMsg.iMoney = updatePlayerInfoMsg.iMoney - iTableMoney;

				CLSendSimpleNetMessage(pTableItem->pPlayers[i]->iSocketIndex, &updatePlayerInfoMsg, HZXLMJ_UPDATE_PLAYER_INFO, sizeof(UpdatePlayerInfoDef));
			}
		}
		_log(_DEBUG, "HZXLGameLogic", "CallBGRefI_log() i[%d],updatePlayerInfoMsg.iMoney[%lld]", i, updatePlayerInfoMsg.iMoney);
	}
}

// 给其他玩家自动发送魔法表情
void HZXLGameLogic::AutoSendMagicExpress(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char cTargetExtra)
{
	if (nodePlayers == NULL)
		return;

	bool bSuccessSend = false;
	char cUseExtra = nodePlayers->cTableNumExtra;
	int iUserID = nodePlayers->iUserID;
	if (nodePlayers->iContinousZiMoCnt == 3)	// 连续自摸胡三次，自动给其他三个玩家发送表情
	{
		nodePlayers->iContinousZiMoCnt = -1;	// 代表该玩家不能再连续发自摸表情了
		for (int i = 1; i < PLAYER_NUM; i++)
		{
			char cToExtra = (cUseExtra + i) % PLAYER_NUM;
			if (tableItem->pPlayers[cToExtra])
			{
				int iToUserID = tableItem->pPlayers[cToExtra]->iUserID;
				_log(_DEBUG, "HZXLGameLogic", "AutoSMagExp Cherrs iUserID[%d]iToUserID[%d]", iUserID, iToUserID);
				int iPropIdCheers = 51;
				SendMagicExpress(tableItem, iUserID, iToUserID, iPropIdCheers);
				bSuccessSend = true;
			}
		}
	}

	if (tableItem->cLeastPengExtra > -1 && tableItem->cLeastPengExtra < 4)	// 玩家打出一张牌，上家碰牌，然后自己自摸
	{
		char cToExtra = tableItem->cLeastPengExtra;
		tableItem->cLeastPengExtra = -1;		// 可以多次触发
		if (!bSuccessSend)
		{
			if (tableItem->pPlayers[cUseExtra] && tableItem->pPlayers[cToExtra])
			{
				int iUserId = tableItem->pPlayers[cUseExtra]->iUserID;
				int iToUserId = tableItem->pPlayers[cToExtra]->iUserID;
				_log(_DEBUG, "HZXLGameLogic", "AutoSMagExp Kiss iUserID[%d]iToUserID[%d]", iUserID, iToUserId);
				int iPropIdKiss = 52;
				SendMagicExpress(tableItem, iUserId, iToUserId, iPropIdKiss);
				bSuccessSend = true;
			}
		}
	}

	if (!bSuccessSend && tableItem->cCurMoCard == 65)		// 考虑多次自摸红中胡
	{
		if (nodePlayers->iZiMoHuHongZhongCnt >= 2)
		{
			for (int i = 1; i < PLAYER_NUM; i++)
			{
				char cToExtra = (cUseExtra + i) % PLAYER_NUM;
				if (tableItem->pPlayers[cToExtra])
				{
					int iToUserID = tableItem->pPlayers[cToExtra]->iUserID;
					int iPropIdFlower = 57;
					_log(_DEBUG, "HZXLGameLogic", "AutoSMagExp Flower iUserID[%d]iToUserID[%d]", iUserID, iToUserID);
					SendMagicExpress(tableItem, iUserID, iToUserID, iPropIdFlower);
					bSuccessSend = true;
				}
			}
		}
	}
}

// 达成指定条件自动发送表情消息
void HZXLGameLogic::SendMagicExpress(MJGTableItemDef *tableItem, int iUserID, int iToUserID, int iPropId)
{
	SpecialMagicExpressMsgDef magicMsg;
	memset(&magicMsg, 0, sizeof(SpecialMagicExpressMsgDef));

	magicMsg.iUserID = iUserID;
	magicMsg.iToUserID = iToUserID;
	magicMsg.iPropId = iPropId;

	_log(_DEBUG, "HZXLGameLogic", "SendMagicExpress_log iUserID[%d]iToUserID[%d]iPropId[%d]", iUserID, iToUserID, iPropId);
	for (int i = 0; i < PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i])
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &magicMsg, HZXLMJ_MAGIC_EXPRESS_MSG, sizeof(SpecialMagicExpressMsgDef));
	}
}

// 玩家游戏内破产请求充值/告知玩家充值结果
void HZXLGameLogic::HandleRechargeReqMsg(int iUserID, void *pMsgData)
{
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "XLMJGameLogic", "HandleRechargeReqMsg_log: nodePlayers is NULL");
		return;
	}

	int iTableNum = nodePlayers->cTableNum;   //找出所在的桌
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomNum = nodePlayers->cRoomNum;
	MJGTableItemDef *tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	if (tableItem == NULL)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleRechargeReqMsg_log tableItem is null userID[%d]", nodePlayers->iUserID);
		return;
	}

	RechargeReqMsgDef *msgRecharge = (RechargeReqMsgDef*)pMsgData;
	char cRechargeFlag = msgRecharge->cRechargeFlag;
	char cRechargeType = msgRecharge->cRechargeType;

	_log(_DEBUG, "HZXLGameLogic", "HandleRechargeReqMsg_log UID[%d]cRechargeFlag[%d]cRechargeType[%d]", nodePlayers->iUserID, cRechargeFlag, cRechargeType);

	// 玩家已经离桌，不能充值
	if (nodePlayers->bTempMidway == true && cRechargeFlag == 1)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleRechargeReqMsg_log bTempMidway R[%d] T[%d] userID[%d] iTableNumExtra[%d],iStatus[%d]",
			nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, iTableNumExtra, nodePlayers->iStatus);

		SendKickOutWithMsg(nodePlayers, "玩家已破产离桌，不能充值续费", 1301, 1);
		return;
	}

	// 玩家未处于破产充值状态，不能充值
	if (nodePlayers->bWaitRecharge == false && cRechargeFlag == 1)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleRechargeReqMsg_log cPoChan R[%d] T[%d] userID[%d] iTableNumExtra[%d],iStatus[%d]",
			nodePlayers->cRoomNum, tableItem->iTableID, nodePlayers->iUserID, iTableNumExtra, nodePlayers->iStatus);

		SendKickOutWithMsg(nodePlayers, "玩家未破产，不能充值续费", 1301, 1);
		return;
	}

	RechargeReqMsgDef rechargeReq;
	memset(&rechargeReq, 0, sizeof(rechargeReq));
	rechargeReq.cRechargeFlag = cRechargeFlag;
	rechargeReq.cRechargeType = cRechargeType;
	rechargeReq.cTableNumExtra = iTableNumExtra;

	if (cRechargeFlag == ASK_RECHARGE || cRechargeFlag == BEGIN_RECHARGE)						// 玩家请求充值
	{
		//if (cRechargeType == 2)					// 现金充值，提高玩家的踢出时间
		//	nodePlayers->iKickOutTime = 35;
		for (int i = 0; i<PLAYER_NUM; i++)
		{
			if (tableItem->pPlayers[i] != NULL)
			{
				CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &rechargeReq, HZXLMJ_RECHARGE_REQ, sizeof(RechargeReqMsgDef));
			}
		}
	}

	if (cRechargeFlag == BEGIN_RECHARGE)
	{
		_log(_DEBUG, "HZXLGameLogic", "HRechargeReq UID[%d]bExistWR[%d]iWaitRechargeExtra[%d],iTableNumExtra[%d]", 
			nodePlayers->iUserID, tableItem->bExistWaitRecharge, tableItem->iWaitRechargeExtra, iTableNumExtra);
		int iShift = 1 << iTableNumExtra;
		int iResult = tableItem->iWaitRechargeExtra & (1 << iTableNumExtra);
		if (tableItem->bExistWaitRecharge && (iResult > 0))
		{
			_log(_DEBUG, "HZXLGameLogic", "HRechargeReq UID[%d], cRechargeType[%d]", nodePlayers->iUserID, cRechargeType);
			time(&(nodePlayers->tmBeginCharge));
			if (cRechargeType == RECHARGE_FIRST_MONEY)		// 福券充值
				nodePlayers->iAllowChargeTime = 15;
			else if (cRechargeType == RECHARGE_CASH)		// 现金充值
				nodePlayers->iAllowChargeTime = 30;

			nodePlayers->cRechargeType = cRechargeType;
		}
	}
}

// 玩家游戏内破产充值结果
void HZXLGameLogic::HandleRechargeResultMsg(int iUserID, void *pMsgData)
{
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "XLMJGameLogic", "HandleRechargeResultMsg_log: nodePlayers is NULL");
		return;
	}

	int iTableNum = nodePlayers->cTableNum;   //找出所在的桌
	int iTableNumExtra = nodePlayers->cTableNumExtra;
	int iRoomNum = nodePlayers->cRoomNum;
	MJGTableItemDef *tableItem = (MJGTableItemDef*)GetTableItem(iRoomNum - 1, iTableNum - 1);
	if (tableItem == NULL)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleRechargeResultMsg_log tableItem is null userID[%d]", nodePlayers->iUserID);
		return;
	}

	if (iTableNumExtra < 0 || iTableNumExtra > 3)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleRechargeResultMsg_log ERROR iTableNumExtra[%d]", iTableNumExtra);
		return;
	}

	RechargeResultMsgDef *msgRecharge = (RechargeResultMsgDef*)pMsgData;
	char cRechargeFlag = msgRecharge->cRechargeFlag;

	_log(_DEBUG, "HZXLGameLogic", "HandleRechargeResultMsg_log UID[%d]cReF[%d],cExtra[%d],bWaitR[%d]",
		nodePlayers->iUserID, cRechargeFlag, nodePlayers->cTableNumExtra, nodePlayers->bWaitRecharge);

	// 充值结束，充值成功或者失败，或者放弃充值
	if (cRechargeFlag == RECHARGE_SUCCESS || cRechargeFlag == RECHARGE_FAIL || cRechargeFlag == ABANDON_RECHARGE)
	{
		if (cRechargeFlag == RECHARGE_SUCCESS)
			nodePlayers->bBankruptRecharge = true;
		ExchargeEnd(nodePlayers, tableItem, cRechargeFlag);
	}
	nodePlayers->bWaitRecharge = false;
}

// 玩家充值结束
void HZXLGameLogic::ExchargeEnd(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char cRechargeFlag, bool bOverTime)
{
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "HZXLGameLogic", "ExchEnd nodePlayers == NULL");
		return;
	}

	// 玩家充值成功但是已经超时
	if (cRechargeFlag == RECHARGE_SUCCESS && nodePlayers->iAllowChargeTime == 0 && nodePlayers->tmBeginCharge == 0)
	{
		_log(_DEBUG, "HZXLGameLogic", "ExchargeEnd_log() iUserID[%d],iAllowCT[%d],tBeginC[%d]", nodePlayers->iUserID, nodePlayers->iAllowChargeTime, nodePlayers->tmBeginCharge);
		cRechargeFlag = RECHARGE_OVERTIME;
	}

	if (cRechargeFlag == RECHARGE_SUCCESS)
	{
		int iRecordId = 0;
		int iRecordNum = 1;
		if (nodePlayers->cRechargeType == 1)			// 成功充值福券
			iRecordId = RECHARGE_BKB_SUCCESS_CNT;
		else if(nodePlayers->cRechargeType == 2)		// 成功充值金币
			iRecordId = RECHARGE_BKD_SUCCESS_CNT;

		SendRechargeRecordToRedis(iRecordId, iRecordNum);
	}

	RechargeResultMsgDef rechargeResult;
	memset(&rechargeResult, 0, sizeof(rechargeResult));

	rechargeResult.cTableNumExtra = nodePlayers->cTableNumExtra;
	rechargeResult.cRechargeFlag = cRechargeFlag;

	nodePlayers->iAllowChargeTime = 0;
	nodePlayers->tmBeginCharge = 0;
	nodePlayers->bWaitRecharge = false;
	nodePlayers->cRechargeType = 0;

	// 如果充值失败，需要把玩家的破产信息发送到客户端
	if (cRechargeFlag == RECHARGE_FAIL || cRechargeFlag == ABANDON_RECHARGE || cRechargeFlag == RECHARGE_OVERTIME)
	{
		if (nodePlayers && nodePlayers->cPoChan == 0)
		{
			_log(_DEBUG, "HZXLGameLogic", "ExchargeEnd_log() cExtra[%d], iUserID[%d],cReFlag[%d]", nodePlayers->cTableNumExtra, nodePlayers->iUserID, cRechargeFlag);
			nodePlayers->cPoChan = 1;
			nodePlayers->iStatus = PS_WAIT_RESERVE_TWO;		// 玩家胡牌后破产，在Escape时清除掉玩家的节点
			tableItem->iPoChanNum++;
		}

		if (tableItem->iPoChanNum >= 3)
		{
			GameEndHu(nodePlayers, tableItem, false, 0);
			return;
		}
	}

	for (int i = 0; i<PLAYER_NUM; i++)
	{
		if (tableItem->pPlayers[i] != NULL)
		{
			int iRoomIndex = tableItem->pPlayers[i]->cRoomNum - 1;
			long long  iCurTableMoney = stSysConfBaseInfo.stRoom[iRoomIndex].iTableMoney;			// 桌费
			rechargeResult.iTableMoney = iCurTableMoney;
			_log(_DEBUG, "HZXLGameLogic", "ExchargeEnd_log() i[%d], iUserID[%d],cReFlag[%d]", i, tableItem->pPlayers[i]->iUserID, rechargeResult.cRechargeFlag);
			CLSendSimpleNetMessage(tableItem->pPlayers[i]->iSocketIndex, &rechargeResult, HZXLMJ_RECHARGE_RESULT_MSG, sizeof(RechargeResultMsgDef));
		}
	}
	_log(_DEBUG, "HZXLGameLogic", "ExchargeEnd_log() iWaitExtra[%d], bExist[%d], iNextExtra[%d]cRechargeFlag[%d]",
		tableItem->iWaitRechargeExtra, tableItem->bExistWaitRecharge, tableItem->iNextWaitSendExtra, rechargeResult.cRechargeFlag);

	if (bOverTime)
	{
		//EscapeReqDef msgReq;
		//memset(&msgReq, 0, sizeof(EscapeReqDef));

		//_log(_DEBUG, "HZXLGameLogic", "OnTime Player recharge out time UID[%d]", pTableItem->pPlayers[j]->iUserID);
		//HandleNormalEscapeReq(pTableItem->pPlayers[j], &msgReq);
	}

	int iTmpExtra = 0;
	iTmpExtra |= 1 << nodePlayers->cTableNumExtra;
	iTmpExtra = ~iTmpExtra;
	tableItem->iWaitRechargeExtra &= iTmpExtra;
	if (tableItem->iWaitRechargeExtra == 0)
	{
		if (tableItem->iNextWaitSendExtra >= 0 && tableItem->iNextWaitSendExtra < 4)
		{
			int iNextExtra = tableItem->iNextWaitSendExtra;
			int iResultExtra = -1;
			for (int t = 0; t < 4; t++)
			{
				iResultExtra = (iNextExtra + t) % PLAYER_NUM;
				if (tableItem->pPlayers[iResultExtra]->cPoChan == 0)
				{
					break;
				}
			}
			_log(_DEBUG, "HZXLGameLogic", "ExchargeEnd_log() iResultExtra[%d]", iResultExtra);
			if (iResultExtra == -1)		// 找不到未破产的玩家
			{
				GameEndHu(nodePlayers, tableItem, false, 0);
				return;
			}

			if (tableItem->pPlayers[iResultExtra])
			{
				tableItem->iCurMoPlayer = iResultExtra;
				SendCardToPlayer(tableItem->pPlayers[iResultExtra], tableItem);
			}
		}
		else
			_log(_ERROR, "HZXLGameLogic", "ExchargeEnd_log() iNextWaitSendExtra[%d]", tableItem->iNextWaitSendExtra);

		tableItem->bExistWaitRecharge = false;
		tableItem->iNextWaitSendExtra = -1;
	}
}

// 每秒定时检查是否有玩家破产支付超时
void HZXLGameLogic::OnTimeCheckOverTime()
{
	for (int i = 0; i < MAX_TABLE_NUM; i++)
	{
		MJGTableItemDef *pTableItem = (MJGTableItemDef *)GetTableItem(0, i);
		if (!pTableItem)
			continue;
		
		if (pTableItem->bExistWaitRecharge)
		{
			_log(_DEBUG, "HZXLGameLogic", "OnTime iTableID[%d]", pTableItem->iTableID);
			time_t now_time;
			time(&now_time);
			for (int j = 0; j < PLAYER_NUM; j++)
			{
				if (pTableItem->pPlayers[j])
				{
					_log(_DEBUG, "HZXLGameLogic", "OnTime i[%d],j[%d], iAllowChargeTime[%d], tmBeginCharge[%d]", 
						i, j, pTableItem->pPlayers[j]->iAllowChargeTime, pTableItem->pPlayers[j]->tmBeginCharge);
					if (pTableItem->pPlayers[j]->iAllowChargeTime > 0 && pTableItem->pPlayers[j]->tmBeginCharge > 0)
					{
						int iGapTime = now_time - pTableItem->pPlayers[j]->tmBeginCharge;
						_log(_DEBUG, "HZXLGameLogic", "OnTime i[%d],j[%d], iGapTime[%d], tmBeginCharge[%d]",
							i, j, iGapTime, pTableItem->pPlayers[j]->tmBeginCharge);
						if (iGapTime > pTableItem->pPlayers[j]->iAllowChargeTime)	// 玩家游戏内破产支付超时
						{
							_log(_DEBUG, "HZXLGameLogic", "OnTime iAllCTime[%d],BeginCT[%d]", pTableItem->pPlayers[j]->iAllowChargeTime, pTableItem->pPlayers[j]->tmBeginCharge);
							pTableItem->pPlayers[j]->iAllowChargeTime = 0;
							pTableItem->pPlayers[j]->tmBeginCharge = 0;

							char cRechargeFlag = RECHARGE_OVERTIME;
							bool bOverTime = true;
							//if (pTableItem->pPlayers[j]->bIfWaitLoginTime == true || pTableItem->pPlayers[j]->cDisconnectType == 1)
							//	bOverTime = true;
							_log(_DEBUG, "HZXLGameLogic", "OnTime Player 00 UID[%d],bIfWLT[%d],cDisT[%d]", pTableItem->pPlayers[j]->iUserID, pTableItem->pPlayers[j]->bIfWaitLoginTime, pTableItem->pPlayers[j]->cDisconnectType);
							ExchargeEnd(pTableItem->pPlayers[j], pTableItem, cRechargeFlag, bOverTime);
							//pTableItem->pPlayers[j]->iStatus = PS_WAIT_RESERVE_TWO;

							//_log(_DEBUG, "HZXLGameLogic", "OnTime Player UID[%d],bIfWLT[%d],cDisT[%d]", pTableItem->pPlayers[j]->iUserID, pTableItem->pPlayers[j]->bIfWaitLoginTime, pTableItem->pPlayers[j]->cDisconnectType);
							//if (pTableItem->pPlayers[j]->bIfWaitLoginTime == true || pTableItem->pPlayers[j]->cDisconnectType == 1)
							//{
								//EscapeReqDef msgReq;
								//memset(&msgReq, 0, sizeof(EscapeReqDef));

								//_log(_DEBUG, "HZXLGameLogic", "OnTime Player recharge out time UID[%d]", pTableItem->pPlayers[j]->iUserID);
								//HandleNormalEscapeReq(pTableItem->pPlayers[j], &msgReq);
							//}
						}
					}
				}
			}
		}
	}
}

void HZXLGameLogic::HandleRechargeRecordMsg(int iUserID, void *pMsgData)
{
	MJGPlayerNodeDef *nodePlayers = (MJGPlayerNodeDef *)(hashUserIDTable.Find((void *)(&iUserID), sizeof(int)));
	if (nodePlayers == NULL)
	{
		_log(_ERROR, "XLMJGameLogic", "HandleRechargeRecordMsg_log: nodePlayers is NULL");
		return;
	}

	RechargeRecordMsgDef *recordMsg = (RechargeRecordMsgDef*)pMsgData;
	int iRecordId = recordMsg->iRecordId;
	int iRecordNum = recordMsg->iRecordNum;

	if (iRecordId < RECHARGE_BKB_POP_CNT || iRecordId > RECHARGE_BKD_SUCCESS_CNT)
	{
		_log(_ERROR, "HZXLGameLogic", "HandleReRMsg_log ERROR UID[%d]iRecordId[%d],iRecordNum[%d]",
			nodePlayers->iUserID, iRecordId, iRecordNum);
		return;
	}

	_log(_DEBUG, "HZXLGameLogic", "HandleRechargeRecordMsg_log UID[%d]iRecordId[%d],iRecordNum[%d]",
		nodePlayers->iUserID, iRecordId, iRecordNum);

	SendRechargeRecordToRedis(iRecordId, iRecordNum);
}

// 发送游戏内破产充值统计数据到redis
void HZXLGameLogic::SendRechargeRecordToRedis(int iRecordId, int iRecordNum)
{
	if (iRecordId < RECHARGE_BKB_POP_CNT || iRecordId > RECHARGE_BKD_SUCCESS_CNT || iRecordNum < 1)
	{
		_log(_ERROR, "HZXLGameLogic", "SendReRecordToRedis_log ERROR iRecordId[%d],iRecordNum[%d]", iRecordId, iRecordNum);
		return;
	}
	//此处待确认统计数据
	/*
	SRdGameDayRecordDef dayRecord;
	memset(&dayRecord, 0, sizeof(SRdGameDayRecordDef));
	dayRecord.msgHeadInfo.cVersion = MESSAGE_VERSION;
	dayRecord.msgHeadInfo.cMsgType = REDIS_GAME_DAY_LOG_RECORD;
	dayRecord.iGameID = HZXLMJ_GAME_ID;
	dayRecord.iEventID[0] = iRecordId;
	dayRecord.iEventAddCnt[0] = iRecordNum;

	EventMsgRadiusDef msgRD;
	memset(&msgRD, 0, sizeof(EventMsgRadiusDef));
	msgRD.iDataLen = sizeof(SRdGameDayRecordDef);
	memcpy(msgRD.msgData, &dayRecord, sizeof(SRdGameDayRecordDef));
	if (m_pMQRedis)
	{
		m_pMQRedis->EnQueue(&msgRD, sizeof(EventMsgRadiusDef));
		_log(_DEBUG, "HZXLGameLogic", "SendReRecordToRedis_log() m_pMQRedis iRecordId[%d], iRecordNum[%d]", iRecordId, iRecordNum);
	}
	else
		_log(_ERROR, "HZXLGameLogic", "SendReRecordToRedis_log() m_pMQRedis == NULL");*/
}


