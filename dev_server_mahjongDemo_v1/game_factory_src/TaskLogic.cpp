#include "TaskLogic.h"
#include "gamelogic.h"
#include "log.h"

TaskLogic::TaskLogic(GameLogic * pGameLogic)
{
	m_pGameLogic = pGameLogic;
	m_iTimeReadConf = 0;
	m_bNeedRateTable = false;
	m_bNeedIntegralTask = false;
}

TaskLogic::~TaskLogic()
{
}

void TaskLogic::CallBackOneSec(long iTime)
{
	m_iTimeReadConf--;
	if (m_iTimeReadConf <= 0)
	{
		m_iTimeReadConf = 600;
		ReadConf();
	}
}

void TaskLogic::ReadConf()
{
	GameGetIntegralTaskInfoReqDef pIntegralTaskReq;
	memset(&pIntegralTaskReq, 0, sizeof(pIntegralTaskReq));
	pIntegralTaskReq.msgHeadInfo.iMsgType = RD_GET_INTEGRAL_TASK_CONF;
	pIntegralTaskReq.msgHeadInfo.cVersion = MESSAGE_VERSION;
	pIntegralTaskReq.iGameID = m_pGameLogic->m_pServerBaseInfo->iGameID;
	pIntegralTaskReq.iReqFlag = m_bNeedRateTable | m_bNeedIntegralTask << 1;  //第1位获取表概率配置 第2位获取任务池配置
	if (m_pGameLogic && m_pGameLogic->m_pQueToRedis && pIntegralTaskReq.iReqFlag != 0)
	{
		m_pGameLogic->m_pQueToRedis->EnQueue(&pIntegralTaskReq, sizeof(GameGetIntegralTaskInfoReqDef));
	}
}

void TaskLogic::HandleGetRedisTaskMsg(void* pMsgData)
{
	SRedisGetUserTaskResDef* msg = (SRedisGetUserTaskResDef*)pMsgData;
	int iUserIDTemp = msg->iUserID;

	FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)m_pGameLogic->hashUserIDTable.Find((void*)&(iUserIDTemp), sizeof(int));
	if (nodePlayers)
	{
		vector<RdUserTaskInfoDef>().swap(nodePlayers->vcUserDayWeekTask);
		nodePlayers->vcUserDayWeekTask.clear();
		RdUserTaskInfoDef* pTaskInfo = (RdUserTaskInfoDef*)((char*)pMsgData + sizeof(SRedisGetUserTaskResDef));
		for (int i = 0; i < msg->iTaskNum; i++)
		{
			RdUserTaskInfoDef stTaskInfo;
			memcpy(&stTaskInfo, pTaskInfo, sizeof(RdUserTaskInfoDef));
			nodePlayers->vcUserDayWeekTask.push_back(stTaskInfo);

			/*if (pTaskInfo->iTaskSubType == (int)DayWeekTaskType::PLAY_WITH_FRIEND && pTaskInfo->iNeedComplete > pTaskInfo->iHaveComplete)
			{
				nodePlayers->bCheckFriendship = true;  //需要在开局前判断与同桌玩家的好友关系
			}*/

			pTaskInfo++;
		}
		_log(_DEBUG, "GL", "HandleGetRedisTaskMsg: [%d] iTaskNum[%d]", iUserIDTemp, nodePlayers->vcUserDayWeekTask.size());
		nodePlayers->iGetRdTaskInfoRt = 1;
	}
	else
	{
		_log(_DEBUG, "GL", "HandleGetRedisTaskMsg: [%d] nodePlayer null", iUserIDTemp);
	}
}

void TaskLogic::HandleGetUserRdComTask(void* pMsgData)
{
	RdComTaskSyncInfoDefDef* pMsg = (RdComTaskSyncInfoDefDef*)pMsgData;
	int iUserIDTemp = pMsg->iUserID;
	FactoryPlayerNodeDef* nodePlayers = (FactoryPlayerNodeDef*)m_pGameLogic->hashUserIDTable.Find((void*)&(iUserIDTemp), sizeof(int));
	if (nodePlayers)
	{
		vector<RdComOneTaskInfoDef>().swap(nodePlayers->vcUserComTask);
		nodePlayers->vcUserComTask.clear();
		RdComOneTaskInfoDef* pTaskInfo = (RdComOneTaskInfoDef*)((char*)pMsgData + sizeof(RdComTaskSyncInfoDefDef));
		for (int i = 0; i < pMsg->iAllTaskNum; i++)
		{
			RdComOneTaskInfoDef stOneTask;
			memcpy(&stOneTask, pTaskInfo, sizeof(RdComOneTaskInfoDef));
			nodePlayers->vcUserComTask.push_back(stOneTask);

			/*if (pTaskInfo->iTaskType == (int)DayWeekTaskType::PLAY_WITH_FRIEND && pTaskInfo->iAllNeedComp > pTaskInfo->iNowComp)
			{
				nodePlayers->bCheckFriendship = true;  //需要在开局前判断与同桌玩家的好友关系
			}*/

			_log(_DEBUG, "GL", "HandleGetUserRdComTask[%d] iTaskNum[%d] iAct[%d] iTaskID[%d] iTaskType[%d] now[%d] need[%d]", iUserIDTemp, i, pTaskInfo->iTaskInfo&0xffff, pTaskInfo->iTaskInfo>>16, pTaskInfo->iTaskType, pTaskInfo->iNowComp, pTaskInfo->iAllNeedComp);
			pTaskInfo++;
		}
	}
	else
	{
		_log(_DEBUG, "GL", "HandleGetUserRdComTask: [%d] nodePlayer null", iUserIDTemp);
	}
}

void TaskLogic::JudgeUserRedisTask(FactoryTableItemDef* pTableItem)
{
	if (m_pGameLogic->m_pQueToRedis == NULL)
	{
		return;
	}
	for (int i = 0; i < m_pGameLogic->m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			JudgeUserRedisTask(pTableItem, pTableItem->pFactoryPlayers[i]);
		}
	}
}

void TaskLogic::JudgeUserRedisTask(FactoryTableItemDef * pTableItem, FactoryPlayerNodeDef * nodePlayers)
{
	if (nodePlayers == nullptr || pTableItem == nullptr)
	{
		return;
	}
	int iBufferLen = 0;
	if (nodePlayers->iGameResultForTask > 0)
	{
		int iResultTask = nodePlayers->iGameResultForTask & 0x000f;
		if (iResultTask != 1)
		{
			nodePlayers->iContinueWin = 0;
		}
		else
		{
			nodePlayers->iContinueWin++;
		}
		if (iResultTask != 2)
		{
			nodePlayers->iContinueLose = 0;
		}
		else
		{
			nodePlayers->iContinueLose++;
		}
	}
	memset(m_cTaskBuff, 0, sizeof(m_cTaskBuff));

	SRedisTaskGameResultReqDef* pMsgUpTaskReq = (SRedisTaskGameResultReqDef*)m_cTaskBuff;
	iBufferLen = sizeof(SRedisTaskGameResultReqDef);
	SRdTaskGameResultReqExtraDef* pReqExtra = (SRdTaskGameResultReqExtraDef*)(m_cTaskBuff + iBufferLen);
	_log(_DEBUG, "GL", "JudgeUserRedisTask[%d],task[%d][%d]", nodePlayers->iUserID, nodePlayers->vcUserDayWeekTask.size(), nodePlayers->iGameResultForTask);
	int iAllTaskNum = nodePlayers->vcUserDayWeekTask.size();
	int iResulTask = nodePlayers->iGameResultForTask & 0x000f;
	int iRank = (nodePlayers->iGameResultForTask) >> 4 & 0x000f;

	if (iAllTaskNum > 0 && iResulTask > 0)
	{
		for (int m = 0; m < iAllTaskNum; m++)
		{
			RdUserTaskInfoDef* pOneTask = &nodePlayers->vcUserDayWeekTask[m];
			pReqExtra->iID = pOneTask->iID;
			pReqExtra->iTaskMainType = pOneTask->iTaskMainType;
			int iRoomType = m_pGameLogic->m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iRoomType;
			if (pOneTask->iHaveComplete > 0)
			{
				_log(_DEBUG, "GL", "JudgeUserRedisTask iID[%d]iTaskMainType[%d]iHaveComplete[%d][%d] iRoomType[%d]", pOneTask->iTaskSubType, pReqExtra->iTaskMainType, pOneTask->iHaveComplete, pOneTask->iNeedComplete, iRoomType);
			}
			if (pOneTask->iID > 0 && pOneTask->iHaveComplete < pOneTask->iNeedComplete)
			{
				if (pOneTask->iTaskSubType == (int)DayWeekTaskType::ACCUMLATE_FIRST && iRank == 1)//0累积获胜任务
				{
					pMsgUpTaskReq->iUpdateTaskNum++;
					pReqExtra->iAddTaskComplete = 1;
					pOneTask->iHaveComplete += 1;
					if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
					{
						pReqExtra->iIfCompTask = 1;
					}
					_log(_ALL, "GL", "JudgeUserRedisTask1 iID[%d] send", pReqExtra->iID);
					pReqExtra++;
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::ACCUMLATE_WIN && iResulTask == 1)
				{
					pMsgUpTaskReq->iUpdateTaskNum++;
					pReqExtra->iAddTaskComplete = 1;
					pOneTask->iHaveComplete += 1;
					if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
					{
						pReqExtra->iIfCompTask = 1;
					}
					_log(_ALL, "GL", "JudgeUserRedisTask1 iID[%d] send", pReqExtra->iID);
					pReqExtra++;
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::CONTINUE_FIRST)
				{
					if (iRank == 1)
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						pReqExtra->iAddTaskComplete = 1;
						pOneTask->iHaveComplete += 1;
						if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
						{
							pReqExtra->iIfCompTask = 1;
						}
						pReqExtra++;
						_log(_ALL, "GL", "JudgeUserRedisTask2 iID[%d] send", pReqExtra->iID);
					}
					else if (pOneTask->iHaveComplete != 0)
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						pReqExtra->iAddTaskComplete = -pOneTask->iHaveComplete;
						pOneTask->iHaveComplete = 0;
						pReqExtra++;
						_log(_ALL, "GL", "JudgeUserRedisTask2_2 iID[%d] send", pReqExtra->iID);
					}
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::CONTINUE_WIN)
				{
					if (iResulTask == 1)
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						pReqExtra->iAddTaskComplete = 1;
						pOneTask->iHaveComplete += 1;
						if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
						{
							pReqExtra->iIfCompTask = 1;
						}
						pReqExtra++;
						_log(_ALL, "GL", "JudgeUserRedisTask2 iID[%d] send", pReqExtra->iID);
					}
					else if (pOneTask->iHaveComplete != 0)
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						pReqExtra->iAddTaskComplete = -pOneTask->iHaveComplete;
						pOneTask->iHaveComplete = 0;
						pReqExtra++;
						_log(_ALL, "GL", "JudgeUserRedisTask2_2 iID[%d] send", pReqExtra->iID);
					}
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::COMPLETE_GAME)//2,完成局数
				{
					pMsgUpTaskReq->iUpdateTaskNum++;
					pReqExtra->iAddTaskComplete = 1;
					pOneTask->iHaveComplete += 1;
					if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
					{
						pReqExtra->iIfCompTask = 1;
					}
					_log(_ALL, "GL", "JudgeUserRedisTask3 iID[%d] send", pReqExtra->iID);
					pReqExtra++;
				}
				else if (pOneTask->iTaskSubType >= (int)DayWeekTaskType::ROOMTYPE5_PLAY && pOneTask->iTaskSubType <= (int)DayWeekTaskType::ROOMTYPE8_PLAY)   //指定房间游戏局数
				{
					if (iRoomType == pOneTask->iTaskSubType - 9)
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						pReqExtra->iAddTaskComplete = 1;
						pOneTask->iHaveComplete += 1;
						if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
						{
							pReqExtra->iIfCompTask = 1;
						}
						_log(_ALL, "GL", "JudgeUserRedisTask subType[%d] iID[%d] send", pOneTask->iTaskSubType, pReqExtra->iID);
						pReqExtra++;
					}
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::GAME_TIME)		//游戏时间达到多少分钟
				{
					time_t tmNow = time(&tmNow);
					int iGap = tmNow - pTableItem->iBeginGameTime;
					pMsgUpTaskReq->iUpdateTaskNum++;
					pReqExtra->iAddTaskComplete = iGap;
					pOneTask->iHaveComplete += iGap;
					if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
					{
						pReqExtra->iIfCompTask = 1;
					}
					_log(_ALL, "GL", "JudgeUserRedisTask3 iID[%d] send", pReqExtra->iID);
					pReqExtra++;
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::MAGIC_CNT)   //使用魔法表情数量
				{
					if (nodePlayers->iGameMagicCnt > 0)
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						pReqExtra->iAddTaskComplete = nodePlayers->iGameMagicCnt;
						pOneTask->iHaveComplete += nodePlayers->iGameMagicCnt;
						if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
						{
							pReqExtra->iIfCompTask = 1;
						}
						_log(_ALL, "GL", "JudgeUserRedisTask [%d] iID[%d] send", pOneTask->iTaskSubType, pReqExtra->iID);
						pReqExtra++;
					}
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::WIN_MONEY_INGMAE)   //累计赢的游戏币数量
				{
					if (iResulTask == 1)
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						pReqExtra->iAddTaskComplete = nodePlayers->llGameRtAmount;
						pOneTask->iHaveComplete += nodePlayers->llGameRtAmount;
						if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
						{
							pReqExtra->iIfCompTask = 1;
						}
						_log(_ALL, "GL", "JudgeUserRedisTask [%d] iID[%d] send", pOneTask->iTaskSubType, pReqExtra->iID);
						pReqExtra++;
					}
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::GET_INTEGRAL_INGMAE)   //游戏中获得多少奖券
				{
					if (nodePlayers->iGameGetIntegral > 0)
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						pReqExtra->iAddTaskComplete = nodePlayers->iGameGetIntegral;
						pOneTask->iHaveComplete += nodePlayers->iGameGetIntegral;
						if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
						{
							pReqExtra->iIfCompTask = 1;
						}
						_log(_ALL, "GL", "JudgeUserRedisTask [%d] iID[%d] send", pOneTask->iTaskSubType, pReqExtra->iID);
						pReqExtra++;
					}
				}
				/*else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::CREATE_AND_PLAY && nodePlayers->bCreateRoom)
				{
					pMsgUpTaskReq->iUpdateTaskNum++;
					pReqExtra->iAddTaskComplete = 1;
					pOneTask->iHaveComplete += 1;
					if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
					{
						pReqExtra->iIfCompTask = 1;
					}
					_log(_ALL, "GL", "JudgeUserRedisTask3 iID[%d] send", pReqExtra->iID);
					pReqExtra++;
				}
				else if (pOneTask->iTaskSubType == (int)DayWeekTaskType::PLAY_WITH_FRIEND && nodePlayers->bPlayWithFriend)
				{
					pMsgUpTaskReq->iUpdateTaskNum++;
					pReqExtra->iAddTaskComplete = 1;
					pOneTask->iHaveComplete += 1;
					if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
					{
						pReqExtra->iIfCompTask = 1;
					}
					_log(_ALL, "GL", "JudgeUserRedisTask3 iID[%d] send", pReqExtra->iID);
					pReqExtra++;
				}*/
				else   //由游戏自己处理
				{
					if (m_pGameLogic->CallBackJudgeTask(nodePlayers, pOneTask->iTaskSubType, pReqExtra->iAddTaskComplete, pOneTask->iHaveComplete))
					{
						pMsgUpTaskReq->iUpdateTaskNum++;
						if (pOneTask->iHaveComplete >= pOneTask->iNeedComplete)
						{
							pReqExtra->iIfCompTask = 1;
						}
						pReqExtra++;
					}
				}
			}
		}
		if (pMsgUpTaskReq->iUpdateTaskNum > 0)
		{
			_log(_DEBUG, "GL", "JudgeUserRedisTask3 iUpdateTaskNum[%d] send", pMsgUpTaskReq->iUpdateTaskNum);
			pMsgUpTaskReq->iUserID = nodePlayers->iUserID;
			pMsgUpTaskReq->iGameID = m_pGameLogic->m_pServerBaseInfo->iGameID;

			pMsgUpTaskReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
			pMsgUpTaskReq->msgHeadInfo.iMsgType = REDIS_TASK_GAME_RSULT_MSG;

			iBufferLen += pMsgUpTaskReq->iUpdateTaskNum * sizeof(SRdTaskGameResultReqExtraDef);

			m_pGameLogic->m_pQueToRedis->EnQueue(m_cTaskBuff, iBufferLen);
			//同时发送至客户端
			m_pGameLogic->CLSendSimpleNetMessage(nodePlayers->iSocketIndex, m_cTaskBuff, SYN_USER_DAYWEEK_TASK, iBufferLen);
		}
	}
}

void TaskLogic::CheckFriendship(FactoryTableItemDef * pTableItem, FactoryPlayerNodeDef * nodePlayers)
{
	/*CheckFriendshipReqDef friendshipReq;
	memset(&friendshipReq, 0, sizeof(CheckFriendshipReqDef));

	_log(_DEBUG, "TaskLogic", "CheckFriendship req User[%d]", nodePlayers->iUserID);

	friendshipReq.msgHeadInfo.iMsgType = GAME_CHECK_FRIENDSHIP_MSG;
	friendshipReq.msgHeadInfo.cVersion = MESSAGE_VERSION;

	friendshipReq.iUserID = nodePlayers->iUserID;
	int iRealIdx = 0;
	for (int i = 0; i < 10; i++)
	{
		if (pTableItem->pFactoryPlayers[i] && pTableItem->pFactoryPlayers[i]->iUserID != nodePlayers->iUserID)
		{
			friendshipReq.iCheckID[iRealIdx] = pTableItem->pFactoryPlayers[i]->iUserID;
			iRealIdx++;
		}
	}
	if (m_pGameLogic->m_pQueToRadius)
	{
		m_pGameLogic->m_pQueToRadius->EnQueue(&friendshipReq, sizeof(CheckFriendshipReqDef));
	}*/
}

void TaskLogic::JudgeRdComTask(FactoryTableItemDef* pTableItem)
{
	for (int i = 0; i < m_pGameLogic->m_iMaxPlayerNum; i++)
	{
		if (pTableItem->pFactoryPlayers[i])
		{
			JudgeRdComTask(pTableItem, pTableItem->pFactoryPlayers[i]);
		}
	}
}

void TaskLogic::JudgeRdComTask(FactoryTableItemDef * pTableItem, FactoryPlayerNodeDef * nodePlayers)
{
	if (pTableItem == nullptr || nodePlayers == nullptr)
	{
		_log(_DEBUG, "GL", "JudgeRdComTask: [%d] pTableItem or nodePlayers null");
		return;
	}

	int iAllUpdateNum = 0;
	memset(m_cTaskBuff, 0, sizeof(m_cTaskBuff));
	int iBuffLen = sizeof(RdComTaskCompInfoDef);
	RdComTaskCompInfoDef* pMsgReq = (RdComTaskCompInfoDef*)m_cTaskBuff;
	RdComOneTaskCompInfoDef* pOneReqInfo = (RdComOneTaskCompInfoDef*)(m_cTaskBuff + iBuffLen);

	int iRoomID = nodePlayers->cRoomNum - 1;
	int iAllTaskSize = nodePlayers->vcUserComTask.size();

	int iWin = nodePlayers->iGameResultForTask & 0x000f;
	int iRank = (nodePlayers->iGameResultForTask) >> 4 & 0x000f;

	for (int j = 0; j < iAllTaskSize; j++)
	{
		RdComOneTaskInfoDef* pOneTask = &nodePlayers->vcUserComTask[j];
		if (pOneTask->iNowComp >= pOneTask->iAllNeedComp)
		{
			continue;
		}
		_log(_DEBUG, "GL", "JudgeRdComTask uid[%d] ID[%d] type[%d] comp[%d][%d]", nodePlayers->iUserID, pOneTask->iTaskInfo, pOneTask->iTaskType,pOneTask->iNowComp, pOneTask->iAllNeedComp);
		int iGameTaskType = pOneTask->iTaskType;
		//如果游戏任务类型是 1  那么表示是游戏几局  底层统一处理
		if (iGameTaskType == (int)DayWeekTaskType::WIN_MONEY_INGMAE)   //游戏中赢得多少游戏币
		{
			if (nodePlayers->llGameRtAmount > 0)
			{
				pOneTask->iNowComp += nodePlayers->llGameRtAmount;
				pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
				pOneReqInfo->iCompAddNum = nodePlayers->llGameRtAmount;
				pOneReqInfo++;
				pMsgReq->iAllTaskNum++;
			}
		}
		else if (iGameTaskType == (int)DayWeekTaskType::GET_INTEGRAL_INGMAE)   //游戏中获得多少奖券
		{
			if (nodePlayers->iGameGetIntegral > 0)
			{
				pOneTask->iNowComp += nodePlayers->iGameGetIntegral;
				pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
				pOneReqInfo->iCompAddNum = nodePlayers->iGameGetIntegral;
				pOneReqInfo++;
				pMsgReq->iAllTaskNum++;
			}
		}
		else if (iGameTaskType == (int)DayWeekTaskType::CONTINUE_WIN)
		{
			if (iWin == 1)
			{
				pOneTask->iNowComp += 1;
				pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
				pOneReqInfo->iCompAddNum = 1;
				pOneReqInfo++;
				pMsgReq->iAllTaskNum++;
			}
			else if (pOneTask->iNowComp > 0)
			{
				pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
				pOneReqInfo->iCompAddNum = -pOneTask->iNowComp;
				pOneReqInfo++;
				pOneTask->iNowComp = 0;
				pMsgReq->iAllTaskNum++;
			}
		}
		else if (iGameTaskType == (int)DayWeekTaskType::CONTINUE_FIRST)
		{
			if (iRank == 1)
			{
				pOneTask->iNowComp += 1;
				pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
				pOneReqInfo->iCompAddNum = 1;
				pOneReqInfo++;
				pMsgReq->iAllTaskNum++;
			}
			else if(pOneTask->iNowComp > 0)
			{
				pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
				pOneReqInfo->iCompAddNum = -pOneTask->iNowComp;
				pOneReqInfo++;
				pOneTask->iNowComp = 0;
				pMsgReq->iAllTaskNum++;
			}
		}
		else if (iGameTaskType == (int)DayWeekTaskType::COMPLETE_GAME)  //完成局数
		{
			pOneTask->iNowComp++;
			pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
			pOneReqInfo->iCompAddNum = 1;
			pOneReqInfo++;
			pMsgReq->iAllTaskNum++;
		}
		else if (iGameTaskType == (int)DayWeekTaskType::ACCUMLATE_WIN && iWin == 1)
		{
			pOneTask->iNowComp++;
			pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
			pOneReqInfo->iCompAddNum = 1;
			pOneReqInfo++;
			pMsgReq->iAllTaskNum++;
		}
		else if (iGameTaskType == (int)DayWeekTaskType::GAME_TIME)		//游戏时间达到多少分钟
		{
			time_t tmNow = time(&tmNow);
			int iGap = tmNow - pTableItem->iBeginGameTime;
			pOneTask->iNowComp += iGap;
			pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
			pOneReqInfo->iCompAddNum = iGap;
			pOneReqInfo++;
			pMsgReq->iAllTaskNum++;
		}
		else if (iGameTaskType == (int)DayWeekTaskType::ACCUMLATE_FIRST && iRank == 1)   //累积获胜
		{
			pOneTask->iNowComp++;
			pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
			pOneReqInfo->iCompAddNum = 1;
			pOneReqInfo++;
			pMsgReq->iAllTaskNum++;
		}
		else if (iGameTaskType >= (int)DayWeekTaskType::ROOMTYPE5_PLAY && iGameTaskType <= (int)DayWeekTaskType::ROOMTYPE8_PLAY)   //指定房间游戏局数
		{
			int iRoomType = m_pGameLogic->m_pServerBaseInfo->stRoom[nodePlayers->cRoomNum - 1].iRoomType;
			if (iRoomType == iGameTaskType - 9)
			{
				pOneTask->iNowComp++;
				pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
				pOneReqInfo->iCompAddNum = 1;
				pOneReqInfo++;
				pMsgReq->iAllTaskNum++;
			}
		}
		/*else if (iGameTaskType == (int)DayWeekTaskType::CREATE_AND_PLAY && nodePlayers->bCreateRoom)
		{
			pOneTask->iNowComp++;
			pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
			pOneReqInfo->iCompAddNum = 1;
			pOneReqInfo++;
			pMsgReq->iAllTaskNum++;
		}
		else if (iGameTaskType == (int)DayWeekTaskType::PLAY_WITH_FRIEND && nodePlayers->bPlayWithFriend)
		{
			pOneTask->iNowComp++;
			pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
			pOneReqInfo->iCompAddNum = 1;
			pOneReqInfo++;
			pMsgReq->iAllTaskNum++;
		}*/
		else
		{
			if (m_pGameLogic->CallBackJudgeTask(nodePlayers, iGameTaskType, pOneReqInfo->iCompAddNum, pOneTask->iNowComp))
			{
				pOneReqInfo->iTaskInfo = pOneTask->iTaskInfo;
				pOneReqInfo++;
				pMsgReq->iAllTaskNum++;
			}
		}
	}
	if (pMsgReq->iAllTaskNum > 0)
	{
		iBuffLen += pMsgReq->iAllTaskNum * sizeof(RdComOneTaskCompInfoDef);
		_log(_DEBUG, "GL", "JudgeRdComTask uid[%d] iAllUpdateNum[%d]", nodePlayers->iUserID, pMsgReq->iAllTaskNum);
		pMsgReq->msgHeadInfo.cVersion = MESSAGE_VERSION;
		pMsgReq->msgHeadInfo.iMsgType = RD_COM_TASK_COMP_INFO;
		pMsgReq->iUserID = nodePlayers->iUserID;

		m_pGameLogic->m_pQueToRedis->EnQueue(m_cTaskBuff, iBuffLen);

		m_pGameLogic->CLSendSimpleNetMessage(nodePlayers->iSocketIndex, m_cTaskBuff, RD_COM_TASK_COMP_INFO, iBuffLen);
	}
}

void TaskLogic::HandleIntegralTaskInfo(void * pMsgData)
{
	GameGetIntegralTaskInfoRes* pMsgRes = (GameGetIntegralTaskInfoRes*)pMsgData;
	int iIfLastMsg = pMsgRes->iIfLastMsg;   //是否最后一次补发消息
	int iRateSize = pMsgRes->iTableSize;
	_log(_DEBUG, "TL", "HandleIntegralTaskInfo iIfLastMsg[%d] iRateSize[%d]", iIfLastMsg, iRateSize);
	int iLen = sizeof(MsgHeadDef) + sizeof(int) * 2;
	char* pCur = (char*)pMsgData;

	for (int i = 0; i < iRateSize; i++)
	{
		IntegralTaskTableDef* pRateTable = (IntegralTaskTableDef*)(pCur + iLen);
		m_mapIntegralTable[pRateTable->iTableGrade] = *pRateTable;
		iLen += sizeof(IntegralTaskTableDef);
	}
	if (iLen < 4096)
	{
		int iTaskSize = *(int*)(pCur + iLen);
		iLen += sizeof(int);
		char szName[512];
		for (int i = 0; i < iTaskSize; i++)
		{
			IntegralTaskInfo taskInfo;
			taskInfo.iTaskID = *(int*)(pCur + iLen);
			iLen += sizeof(int);
			int iNameLenCN = *(int*)(pCur + iLen);
			iLen += sizeof(int);
			memset(szName, 0, sizeof(szName));
			strncpy(szName, pCur + iLen, iNameLenCN);
			taskInfo.strTaskNameCH = szName;
			iLen += iNameLenCN;
			int iNameLenRUS = *(int*)(pCur + iLen);
			iLen += sizeof(int);
			memset(szName, 0, sizeof(szName));
			strncpy(szName, pCur + iLen, iNameLenRUS);
			taskInfo.strTaskNameRU = szName;
			iLen += iNameLenRUS;

			taskInfo.iPlayType = *(int*)(pCur + iLen);
			iLen += sizeof(int);
			for (int j = 0; j < 6; j++)
			{
				taskInfo.iTriggerWeight = *(int*)(pCur + iLen);
				iLen += sizeof(int);
				if (taskInfo.iTriggerWeight > 0)
				{
					m_mapTmpTask[j + 1][taskInfo.iTaskID] = taskInfo;
				}
			}

			int iAwardType = *(int*)(pCur + iLen);
			iLen += sizeof(int);

			int iAwardNum[6] = { 0 };
			for (int j = 0; j < 6; j++)
			{
				iAwardNum[j] = *(int*)(pCur + iLen);
				iLen += sizeof(int);
			}
			for (int j = 0; j < 6; j++)
			{
				if (m_mapTmpTask[j + 1].find(taskInfo.iTaskID) != m_mapTmpTask[j + 1].end())
				{
					if (m_mapTmpTask[j + 1].find(taskInfo.iTaskID) != m_mapTmpTask[j + 1].end())
					{
						memcpy(m_mapTmpTask[j + 1][taskInfo.iTaskID].iAwardNum, iAwardNum, sizeof(iAwardNum));
					}
					m_mapTmpTask[j + 1][taskInfo.iTaskID].iAwardType = iAwardType;
				}
			}

			int iTaskType = *(int*)(pCur + iLen);
			iLen += sizeof(int);
			int iTaskValue = *(int*)(pCur + iLen);
			iLen += sizeof(int);
			for (int j = 0; j < 6; j++)
			{
				if (m_mapTmpTask[j + 1].find(taskInfo.iTaskID) != m_mapTmpTask[j + 1].end())
				{
					m_mapTmpTask[j + 1][taskInfo.iTaskID].iTaskType = iTaskType;
					m_mapTmpTask[j + 1][taskInfo.iTaskID].iTaskValue = iTaskValue;
				}
			}
		}
	}

	if (iIfLastMsg & 1 == 1)
	{
		map<int, map<int, IntegralTaskInfoDef> >().swap(m_mapIntegralTask);
		m_mapIntegralTask.clear();
		m_mapIntegralTask.swap(m_mapTmpTask);
		map<int, map<int, IntegralTaskInfoDef> >().swap(m_mapTmpTask);
		m_mapTmpTask.clear();
		//	map<int, map<int, IntegralTaskInfoDef> >::iterator itor = m_mapIntegralTask.begin();
		//	while (itor != m_mapIntegralTask.end())
		//	{
		//		map<int, IntegralTaskInfoDef>::iterator itor1 = itor->second.begin();
		//		while (itor1 != itor->second.end())
		//		{
		//			IntegralTaskInfoDef *pTask = &(itor1->second);
		//			_log(_DEBUG, "TL", "HandleIntegralTaskInfo tb[%d]id[%d]type[%d]iPlayType[%d]award[%d]iTaskValue[%d]", itor1->first, pTask->iTaskID, pTask->iTaskType, pTask->iPlayType, pTask->iAwardType, pTask->iTaskValue);
		//			itor1++;
		//		}
		//		itor++;
		//	}
	}
}