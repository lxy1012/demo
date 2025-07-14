#include "TaskHandler.h"
#include "RedisHelper.h"
#include <time.h>
#include "Util.h"
#include "log.h"
#include "ServerLogicThread.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "msg.h"
#include <algorithm>
#include "SQLWrapper.h"
#include "MySQLTableStruct.h"
#include "EventLogHandler.h"

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/
TaskHandler::TaskHandler(MsgQueue *pSendQueue, MsgQueue *pEventGetQueue)
{
	m_pSendQueue = pSendQueue;	
	m_pEventGetQueue = pEventGetQueue;
	m_iGetTaskConfCDTime = 900;//每15分钟读一次任务配置
	//m_iSendLogCDTime = 600;//每小时同步一次日志
	//test
	//stSysConfigBaseInfo.iIfTestPlatform = 1;
	if (stSysConfigBaseInfo.iIfTestPlatform)
	{
		m_iSendLogCDTime = 20;
	}
	else
	{
		m_iSendLogCDTime = 600;//每小时同步一次日志
	}
}

void TaskHandler::GetBKTaskConf()
{
	DBGetBKTaskConf(m_mapBKDayTask, m_mapBKWeekTask);
	OnTimerGetAchieveTask();
	GetParamsConf();
}

void TaskHandler::HandleMsg(int iMsgType, char* szMsg, int iLen,int iSocketIndex)
{
	//_log(_DEBUG, "TaskHandler::HandleMsg", "msg[0x%x]", iMsgType);
	if (m_redisCt == NULL)
	{
		_log(_DEBUG,"TaskHandler::HandleMsg","error m_redisCt == NULL");
		return;
	}
	if (iMsgType == GAME_REDIS_GET_USER_INFO_MSG)
	{
		HandleGetUserInfoReq(szMsg,iLen,iSocketIndex);
	}
	else if (iMsgType == REDIS_TASK_GAME_RSULT_MSG)
	{
		HandleGameResultMsg(szMsg,iLen,iSocketIndex);
	}
	else if (iMsgType == RD_COM_TASK_COMP_INFO)
	{
		HandleComTaskCompInfo(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == RD_GAME_TOGETHER_USER_MSG)
	{
		HandleTogetherUserMsg(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == RD_GAME_ROOM_TASK_INFO_MSG)
	{
		HandleGetRoomTaskReq(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == RD_GET_INTEGRAL_TASK_CONF)
	{
		HandleGetIntegralTaskInfoMsg(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == RD_INTEGRAL_TASK_HIS_RES)
	{
		HandleSetIntegralTaskHisMsg(szMsg, iLen, iSocketIndex);
	}
	else if(iMsgType == RD_USER_DAY_INFO_MSG)
	{
		HandleRefreshUserDayInfo(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == RD_GET_USER_LAST_MONTHS_VAC)
	{
		HandleGetLastMonthsVacNum(szMsg, iLen, iSocketIndex);
	}
}

void TaskHandler::CallBackOnTimer(int iTimeNow)
{
	m_iGetTaskConfCDTime--;
	if(m_iGetTaskConfCDTime <= 0)
	{
		m_mapBKDayTask.clear();
		m_mapBKWeekTask.clear();	
		DBGetBKTaskConf(m_mapBKDayTask,m_mapBKWeekTask);	
		m_iGetTaskConfCDTime = 900;
	}
	m_iSendLogCDTime--;
	if (m_iSendLogCDTime <= 0)
	{
		if (m_vcTaskLog.size() > 0)
		{
			char strKey[64];
			char strField[64];
			int iDateFlag = Util::GetDayTimeFlag(time(NULL));
			sprintf(strKey, "%s%d", KEY_BK_TASK_LOG.c_str(), iDateFlag);
		
			for (int i= 0;i<m_vcTaskLog.size();i++)
			{
				if (m_vcTaskLog[i].iTaskType == 0)
				{					
					sprintf(strField, "task_0_%d_comp", m_vcTaskLog[i].iID);					
				}
				else if (m_vcTaskLog[i].iTaskType == 1)
				{
					sprintf(strField, "task_1_%d_comp", m_vcTaskLog[i].iID);
				}
				else if (m_vcTaskLog[i].iTaskType == 2)
				{
					sprintf(strField, "grow_task_%d_comp", m_vcTaskLog[i].iID);
				}
				else if (m_vcTaskLog[i].iTaskType == 1000)
				{
					sprintf(strField, "%s", "comp_all_day_user");
				}
				else if (m_vcTaskLog[i].iTaskType == 2000)
				{
					sprintf(strField, "%s", "comp_all_week_user");
				}
				//_log(_DEBUG, "CallBackOnTimer", "m_vcTaskLog[%d] type[%d] add[%d] key[%s] field[%s]", i, m_vcTaskLog[i].iTaskType, m_vcTaskLog[i].iCompTaskUser, strKey, strField);
				EventLogHandler::shareEventHandler()->AddTaskEventStat(strKey, strField, m_vcTaskLog[i].iCompTaskUser);
			}
			m_vcTaskLog.clear();
		}

		if (stSysConfigBaseInfo.iIfTestPlatform)
		{
			m_iSendLogCDTime = 20;
		}
		else
		{
			m_iSendLogCDTime = 600;//每小时同步一次日志
		}		
	}

	static int iGetAchieveTaskCDTime = 0;

	iGetAchieveTaskCDTime++;
	if (iGetAchieveTaskCDTime > 600)	//10分钟获取一次配置
	{
		iGetAchieveTaskCDTime = 0;
		OnTimerGetAchieveTask();
		m_mapActInvite.clear();
		GetParamsConf();
	}	
}

void TaskHandler::HandleGetUserInfoReq(char* szMsg, int iLen, int iSocketIndex)
{
	GameRedisGetUserInfoReqDef* pMsgReq = (GameRedisGetUserInfoReqDef*)szMsg;
	//信息请求标记位，第1位是否需要近期同桌信息，第2位是否获取该游戏任务信息，第3位是否获取当前装扮信息，第4位是否需要获取通用活动任务信息 5是否需要成就任务
	int iIfNeedTaskInfo = (pMsgReq->iReqFlag>>1) & 0x01;
	if (iIfNeedTaskInfo == 1)
	{
		CheckUserDayAndWeekTask(pMsgReq->iGameID, pMsgReq->iRoomType,pMsgReq->iUserID, iSocketIndex);
	}
	int iIfNeedDecorateInfo = (pMsgReq->iReqFlag >> 2) & 0x01;
	if (iIfNeedDecorateInfo == 1)
	{
		CheckGetUserDecoratePropInfo(pMsgReq->iGameID, pMsgReq->iUserID, iSocketIndex);
	}
	int iIfNeedComTaskInfo = (pMsgReq->iReqFlag >> 3) & 0x01;
	if (iIfNeedComTaskInfo == 1)
	{
		CheckGetComTaskInfo(pMsgReq->iGameID, pMsgReq->iUserID, iSocketIndex);
	}
	int iIfNeedRecentTask = (pMsgReq->iReqFlag >> 5) & 0x01;
	if (iIfNeedRecentTask == 1)
	{
		CheckGetRecentTask(pMsgReq->iGameID, pMsgReq->iUserID, iSocketIndex);
	}
	if ((pMsgReq->iReqFlag >> 6) & 1 == 1)  //获取玩家当天一些数据
	{
		CheckGetUserDayInfo(pMsgReq->iGameID, pMsgReq->iUserID, iSocketIndex);
	}
}
void TaskHandler::CheckUserDayAndWeekTask(int iGameID ,int iRoomType, int iUserID, int iSocketIndex)
{
	//日常任务
	RdUserTaskInfoDef userDayTask[20];//最多20个
	memset(userDayTask,0,sizeof(userDayTask));
	int iUserDayTaskNum = 0;
	JudgeRefreshDayOrWeekTask(iUserID, iGameID, iRoomType,true,userDayTask,20,iUserDayTaskNum);

	//周常任务
	RdUserTaskInfoDef userWeekTask[20];//最多20个
	memset(userWeekTask,0,sizeof(userWeekTask));
	int iUserWeekTaskNum = 0;
	JudgeRefreshDayOrWeekTask(iUserID, iGameID, iRoomType,false,userWeekTask,20,iUserWeekTaskNum);

	
	//成长任务(暂无)

	//所有任务发送至服务器
	SRedisGetUserTaskResDef msgRes;
	memset(&msgRes, 0, sizeof(SRedisGetUserTaskResDef));
	msgRes.iUserID = iUserID;
	msgRes.iTaskNum = iUserDayTaskNum + iUserWeekTaskNum;
	int iMsgLen = sizeof(SRedisGetUserTaskResDef) + (msgRes.iTaskNum) * sizeof(RdUserTaskInfoDef);
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;		
	msgRes.msgHeadInfo.iMsgType = REDIS_RETURN_USER_TASK_INFO;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	char cBuffer[2048];
	memset(cBuffer,0,sizeof(cBuffer));
	memcpy(cBuffer,&msgRes,sizeof(SRedisGetUserTaskResDef));
	int iBufferPos = sizeof(SRedisGetUserTaskResDef);
	for (int i = 0; i < iUserDayTaskNum; i++)
	{
		memcpy(cBuffer+ iBufferPos, &userDayTask[i], sizeof(RdUserTaskInfoDef));
		iBufferPos += sizeof(RdUserTaskInfoDef);
	}
	for (int i = 0; i < iUserWeekTaskNum; i++)
	{
		memcpy(cBuffer + iBufferPos, &userWeekTask[i], sizeof(RdUserTaskInfoDef));
		iBufferPos += sizeof(RdUserTaskInfoDef);
	}
	if (m_pSendQueue)
		m_pSendQueue->EnQueue(cBuffer, iMsgLen);
	_log(_DEBUG, "TH", "HandleGetUserTaskMsg: iSocketIndex[%d], user[%d],iGameID[%d], iRoomType[%d],iTaskNum[%d]=[%d,%d]", iSocketIndex, iUserID, iGameID, iRoomType, msgRes.iTaskNum, iUserDayTaskNum, iUserWeekTaskNum);
}

void TaskHandler::HandleGameResultMsg(char* szMsg, int iLen,int iSocketIndex)
{
	SRedisTaskGameResultReqDef* pMsgReq = (SRedisTaskGameResultReqDef*)szMsg;
	_log(_DEBUG, "TH", "HandleGameResultMsg: user[%d],gameID[%d],iUpdateTaskNum[%d]", pMsgReq->iUserID, pMsgReq->iGameID, pMsgReq->iUpdateTaskNum);
	if (pMsgReq->iUserID <= 0)
	{
		return;
	}
	int iAddDayTaskAwardNum = 0;
	int iAddWeekTaskAwardNum = 0;
	char strKey[64];
	char strField[32];
	vector<int>vcJudgeGroup;
	SRdTaskGameResultReqExtraDef *pMsgReqExtra = (SRdTaskGameResultReqExtraDef *)(szMsg + sizeof(SRedisTaskGameResultReqDef));
	for (int i = 0; i < pMsgReq->iUpdateTaskNum; i++)
	{
		int iID = pMsgReqExtra->iID;
		int iTaskType = pMsgReqExtra->iTaskMainType;
		int iGroupID = 0;
		_log(_DEBUG, "TH", "HandleGameResultMsg: [%d],iTaskType[%d],iID[%d],add[%d],comp[%d]", i, iTaskType, iID, pMsgReqExtra->iAddTaskComplete, pMsgReqExtra->iIfCompTask);
		if (iTaskType != 0 && iTaskType != 1)
		{
			pMsgReqExtra++;
			continue;
		}		
		map<int, BKTaskConfDef>::iterator pos = (iTaskType == 0) ? m_mapBKDayTask.find(iID) : (m_mapBKWeekTask.find(iID));
		if (iTaskType == 0)
		{
			sprintf(strKey, "%s%d", KEY_USER_DAY_TASK.c_str(), pMsgReq->iUserID);
			if (pMsgReqExtra->iIfCompTask == 1)
			{
				sprintf(strField, "task_%d_state", iID);
				RedisHelper::hset(m_redisCt, strKey, strField, "1");
				if (pos != m_mapBKDayTask.end())
				{
					AddCompTaskLog(0, pos->second.iSubType);
				}
				iAddDayTaskAwardNum++;
			}
		}
		else if (iTaskType == 1)
		{
			sprintf(strKey, "%s%d", KEY_USER_WEEK_TASK.c_str(), pMsgReq->iUserID);
			if (pMsgReqExtra->iIfCompTask == 1)
			{
				sprintf(strField, "task_%d_state", iID);
				RedisHelper::hset(m_redisCt, strKey, strField, "1");
				if (pos != m_mapBKWeekTask.end())
				{
					AddCompTaskLog(1, pos->second.iSubType);
				}

				iAddWeekTaskAwardNum++;
			}
		}
		sprintf(strField, "task_%d_complete", iID);
		int iNewNum = RedisHelper::hincrby(m_redisCt, strKey, strField, pMsgReqExtra->iAddTaskComplete);		
		pMsgReqExtra++;
	}
	_log(_DEBUG, "TH", "HandleGameResultMsg:iAddWeekTaskAwardNum[%d],[%d]", iAddDayTaskAwardNum, iAddWeekTaskAwardNum);
	if (iAddDayTaskAwardNum > 0)
	{
		sprintf(strKey, "%s%d", KEY_USER_DAY_TASK.c_str(), pMsgReq->iUserID);
		sprintf(strField, "%s", "wait_award_num");
		RedisHelper::hincrby(m_redisCt, strKey, strField, iAddDayTaskAwardNum);

		int iValue = RedisHelper::hincrby(m_redisCt, strKey, "comp_task_num", iAddDayTaskAwardNum);	
		
		string strRt = RedisHelper::hget(m_redisCt, strKey, "all_task_num");
		_log(_DEBUG, "TH", "HandleGameResultMsg:iAddDayTaskAwardNum[%d],[%d],[%s]", iAddDayTaskAwardNum,iValue,strRt.c_str());
		if(strRt.size() > 0)
		{
			int iAllTask = atoi(strRt.c_str());
			if(iValue >= iAllTask)
			{				
				AddCompTaskLog(1000, 0);
			}
		}
	}
	if (iAddWeekTaskAwardNum > 0)
	{
		sprintf(strKey, "%s%d",KEY_USER_WEEK_TASK.c_str(), pMsgReq->iUserID);
		sprintf(strField, "%s", "wait_award_num");
		RedisHelper::hincrby(m_redisCt, strKey, strField, iAddWeekTaskAwardNum);

		int iValue = RedisHelper::hincrby(m_redisCt, strKey, "comp_task_num", iAddWeekTaskAwardNum);	
		
		string strRt = RedisHelper::hget(m_redisCt, strKey, "all_task_num");
		_log(_DEBUG, "TH", "HandleGameResultMsg:iAddWeekTaskAwardNum[%d],[%d],[%s]", iAddWeekTaskAwardNum,iValue,strRt.c_str());
		if(strRt.size() > 0)
		{
			int iAllTask = atoi(strRt.c_str());
			if(iValue >= iAllTask || (iValue == iAllTask-1 && IfWeekHaveCompAllWeekTask()))
			{
				AddCompTaskLog(2000, 0);

				//完成所有周任务的进度
				string strTaskID = RedisHelper::hget(m_redisCt, strKey, "all_week_comp_id");
				if (strTaskID.size() > 0)
				{
					RedisHelper::hincrby(m_redisCt, strKey, "comp_task_num", 1);

					int iAllCompID = atoi(strTaskID.c_str());
					sprintf(strField, "task_%d_state", iAllCompID);
					RedisHelper::hset(m_redisCt, strKey, strField, "1");

					sprintf(strField, "task_%d_complete", iAllCompID);
					int iNewNum = RedisHelper::hset(m_redisCt, strKey, strField, "1");
				}
			}
		}
	}
}

int TaskHandler::JudgeIfNeedRefreshTask(int iLastRefreshTm,bool bDayTask)
{
	time_t iTmNow = time(NULL);
	time_t iJudgeTm = 0;
	if(bDayTask)
	{
		iJudgeTm = Util::GetTodayStartTimestamp();
	}
	else
	{
		iJudgeTm = Util::GetWeekStartTimestamp();
	}	
	if(iLastRefreshTm < iJudgeTm && iTmNow >= iJudgeTm)
	{
		//_log(_DEBUG, "TH", "JudgeIfNeedRefreshTask00: bDayTask[%d],iTmNow[%d],iJudgeTm[%d],iLastRefreshTm[%d]", bDayTask, iTmNow, iJudgeTm, iLastRefreshTm);
		return iTmNow;
	}
	//_log(_DEBUG, "TH", "JudgeIfNeedRefreshTask11: bDayTask[%d],iTmNow[%d],iJudgeTm[%d],iLastRefreshTm[%d]", bDayTask, iTmNow, iJudgeTm, iLastRefreshTm);
	return 0;
}

bool TaskHandler::JudgeRefreshDayOrWeekTask(int iUserID,int iGameID, int iRoomType,bool bDayTask,RdUserTaskInfoDef userTask[],int iArrayLen,int &iUserTaskNum)
{
	char strKey[64];
	if(bDayTask)
	{
		sprintf(strKey, "%s%d",KEY_USER_DAY_TASK.c_str(), iUserID);
	}
	else
	{
		sprintf(strKey,"%s%d", KEY_USER_WEEK_TASK.c_str(),iUserID);
	}
	
	int iLastRefreshTaskTm = 0;

	char strField[64];
	sprintf(strField,"%s","last_task_tm");

	string strRt = RedisHelper::hget(m_redisCt,strKey,strField);
	if(strRt.size() > 0)
	{
		iLastRefreshTaskTm = atoi(strRt.c_str());
	}

	int iNewRefreshTm = 0;
	if(iLastRefreshTaskTm > 0)
	{
		iNewRefreshTm = JudgeIfNeedRefreshTask(iLastRefreshTaskTm,bDayTask);
	}
	else
	{
		iNewRefreshTm = time(NULL);
	}
	iUserTaskNum = 0;
	if(iNewRefreshTm > 0)//刷新每日/每周任务
	{
		for(int i = 0;i<iArrayLen;i++)
		{
			memset(&userTask[i],0,sizeof(RdUserTaskInfoDef));
		}
		char szIntValue[32];
		char szStrValue[128];
		char szStrValueTemp[32];

		memset(szStrValue,0,sizeof(szStrValue));

		map<string,string>mapValues;

		sprintf(szIntValue,"%d",iNewRefreshTm);
		mapValues["last_task_tm"] = szIntValue;
		mapValues["wait_award_num"] = "0";

		int iExpireEndTm = 0;
		if (bDayTask)
		{
			iExpireEndTm = Util::GetTodayStartTimestamp() + (24 + 7) * 3600;
		}
		else
		{
			iExpireEndTm = Util::GetWeekStartTimestamp() + (24*7 + 7) * 3600;
		}
		_log(_DEBUG, "TH", "JudgeRefresh: bDayTask[%d],iExpireEndTm[%d],[%d],weekStart[%lld]", bDayTask, iExpireEndTm,(iExpireEndTm - iNewRefreshTm), Util::GetWeekStartTimestamp());
		
		bool bHaveLoginTask = false;
		int iChargeTaskID = 0;
		int iAllWeekCompID = 0;
		int iIndex = 0;
		map<int,BKTaskConfDef>::iterator pos = m_mapBKDayTask.begin();
		if(!bDayTask)
		{
			pos = m_mapBKWeekTask.begin();
		}
		for(;bDayTask?(pos != m_mapBKDayTask.end()):(pos != m_mapBKWeekTask.end());pos++)
		{			
			int iTaskDBID = pos->first;
			if (iIndex > 0)
			{
				sprintf(szStrValueTemp,"&%d",iTaskDBID);
				strcat(szStrValue, szStrValueTemp);
			}
			else
			{
				sprintf(szStrValue,"%d", iTaskDBID);
			}
			bool bHaveGameID = false;
			for(int j = 0;j<20;j++)
			{
				if(pos->second.iGameID[j] == 0)
				{
					break;
				}
				if((pos->second.iGameID[j] == iGameID || pos->second.iGameID[j]  == 99) && 
					(pos->second.iRoomType[j] == 0 || pos->second.iRoomType[j] == iRoomType))
				{
					bHaveGameID = true;
					break;
				}
			}
			if(bHaveGameID && iUserTaskNum < iArrayLen)
			{
				userTask[iUserTaskNum].iID = pos->second.iID;
				userTask[iUserTaskNum].iHaveComplete = 0;
				userTask[iUserTaskNum].iNeedComplete = pos->second.iTaskNeedComplete;
				userTask[iUserTaskNum].iTaskMainType =  pos->second.iMainType;
				userTask[iUserTaskNum].iTaskSubType =  pos->second.iSubType;
				iUserTaskNum++;
			}

			if(pos->second.iSubType == (int)DayWeekTaskType::CHARGE)//充值任务
        	{
        		iChargeTaskID  = iTaskDBID;
        	}
			if (pos->second.iSubType == (int)DayWeekTaskType::COMPLETE_ALL_WEEK)
			{
				iAllWeekCompID = iTaskDBID;
			}
			if(pos->second.iSubType == (int)DayWeekTaskType::DAY_LOGIN || pos->second.iSubType == (int)DayWeekTaskType::WEEK_LOGIN || pos->second.iSubType == (int)DayWeekTaskType::CONTINUE_LOGIN)//登陆任务
        	{
        		bHaveLoginTask  = true;
        	}
			sprintf(strField,"task_%d_subtype",iTaskDBID);
			sprintf(szIntValue,"%d",pos->second.iSubType);
			mapValues[strField] = szIntValue;

			sprintf(strField,"task_%d_neednum", iTaskDBID);
			sprintf(szIntValue,"%d",pos->second.iTaskNeedComplete);
			mapValues[strField] = szIntValue;

			sprintf(strField,"task_%d_complete", iTaskDBID);
			sprintf(szIntValue,"%d",0);
			mapValues[strField] = szIntValue;

			sprintf(strField,"task_%d_state", iTaskDBID);
			sprintf(szIntValue,"%d",0);
			mapValues[strField] = szIntValue;

			sprintf(strField, "task_%d_id", iTaskDBID);
			sprintf(szIntValue, "%d", iTaskDBID);
			mapValues[strField] = szIntValue;

			sprintf(strField, "task_%d_awardid1", iTaskDBID);
			sprintf(szIntValue, "%d", pos->second.iTaskAwardProp1);
			mapValues[strField] = szIntValue;

			sprintf(strField, "task_%d_awardid2", iTaskDBID);
			sprintf(szIntValue, "%d", pos->second.iTaskAwardProp2);
			mapValues[strField] = szIntValue;

			sprintf(strField, "task_%d_awardnum1", iTaskDBID);
			sprintf(szIntValue, "%d", pos->second.iTaskAwardNum1);
			mapValues[strField] = szIntValue;

			sprintf(strField, "task_%d_awardnum2", iTaskDBID);
			sprintf(szIntValue, "%d", pos->second.iTaskAwardNum2);
			mapValues[strField] = szIntValue;

			iIndex++;
		}
		sprintf(szIntValue,"%d", iChargeTaskID);
		mapValues["charge_task_id"] = szIntValue;

		sprintf(szIntValue, "%d", iAllWeekCompID);
		mapValues["all_week_comp_id"] = szIntValue;

		mapValues["comp_task_num"] = "0";

		sprintf(szIntValue,"%d",iIndex);
		mapValues["all_task_num"] = szIntValue;

		mapValues["task_ids"] = szStrValue;
		if(bHaveLoginTask)//有登录任务，把时间置为昨天，web服务器会判断跨天登陆完成任务
		{
			sprintf(szIntValue,"%d",(iNewRefreshTm-24*3600));
			mapValues["last_login_tm"] = szIntValue;
		}
		else
		{
			sprintf(szIntValue, "%d", iNewRefreshTm);
			mapValues["last_login_tm"] = szIntValue;
		}
		RedisHelper::hmset(m_redisCt,strKey,mapValues);
		RedisHelper::expire(m_redisCt, strKey, iExpireEndTm - iNewRefreshTm);
	}
	else
	{
		int iTaskNum = 0;
		sprintf(strField,"%s","task_ids");
		strRt = RedisHelper::hget(m_redisCt,strKey,strField);
		_log(_DEBUG, "TH", "JudgeRefreshDayOrWeekTask: bDayTask[%d],strKey[%s],allTask[%s]", bDayTask, strKey, strRt.c_str());
		int iTaskIDs[30];
		memset(iTaskIDs, 0, sizeof(iTaskIDs));
		if(strRt.size() > 0)
		{
			Util::ParaseParamValue((char*)strRt.c_str(), iTaskIDs,"&");
		}	
		
		int iWaitAward = 0;
		for (int i = 0; i < 30; i++)
		{
			int iTaskID = iTaskIDs[i];
			if (iTaskID == 0)
			{
				break;
			}		
			map<int, BKTaskConfDef>::iterator pos = bDayTask ? m_mapBKDayTask.find(iTaskID) : (m_mapBKWeekTask.find(iTaskID));
			if ((bDayTask && pos != m_mapBKDayTask.end()) || (!bDayTask && pos != m_mapBKWeekTask.end()))
			{				
				bool bHaveGameID = false;
				for (int j = 0; j < 20; j++)
				{
					if (pos->second.iGameID[j] == 0)
					{
						break;
					}
					if ((pos->second.iGameID[j] == iGameID || pos->second.iGameID[j] == 99) &&
						(pos->second.iRoomType[j] == 0 || pos->second.iRoomType[j] == iRoomType))
					{
						bHaveGameID = true;
						break;
					}
				}
				if (bHaveGameID  && iUserTaskNum < iArrayLen)
				{
					userTask[iUserTaskNum].iID = iTaskID;					
					userTask[iUserTaskNum].iTaskMainType = bDayTask ? 0 : 1;
					sprintf(strField, "task_%d_subtype", iTaskID);
					strRt = RedisHelper::hget(m_redisCt, strKey, strField);
					if (strRt.size() > 0)
					{
						userTask[iUserTaskNum].iTaskSubType = atoi(strRt.c_str());
						sprintf(strField, "task_%d_neednum", iTaskID);
						strRt = RedisHelper::hget(m_redisCt, strKey, strField);
						if (strRt.size() > 0)
						{
							userTask[iUserTaskNum].iNeedComplete = atoi(strRt.c_str());
						}
						sprintf(strField, "task_%d_complete", iTaskID);
						strRt = RedisHelper::hget(m_redisCt, strKey, strField);
						if (strRt.size() > 0)
						{
							userTask[iUserTaskNum].iHaveComplete = atoi(strRt.c_str());
						}

						//完成并且没有领取
						if (userTask[iUserTaskNum].iNeedComplete > 0 && userTask[iUserTaskNum].iNeedComplete <= userTask[iUserTaskNum].iHaveComplete)
						{
							sprintf(strField, "task_%d_state", iTaskID);
							strRt = RedisHelper::hget(m_redisCt, strKey, strField);
							if (strRt.size() > 0)
							{
								int iStatus = atoi(strRt.c_str());
								if (iStatus == 0)
								{
									iWaitAward++;
								}
							}
						}

						iUserTaskNum++;
					}					
				}
			}			
		}	

		if (iWaitAward > 0)
		{
			char szValue[128] = {0};
			sprintf(szValue, "%d", iWaitAward);
			RedisHelper::hset(m_redisCt, strKey, "wait_award_num", szValue);
		}
	}
}


bool TaskHandler::IfWeekHaveCompAllWeekTask()
{
	map<int, BKTaskConfDef>::iterator pos = m_mapBKWeekTask.begin();
	for (; pos != m_mapBKWeekTask.end(); pos++)
	{
		if (pos->second.iSubType == (int)DayWeekTaskType::COMPLETE_ALL_WEEK)
		{
			return true;
		}
	}
	return false;
}

void TaskHandler::AddCompTaskLog(int iType, int iID)
{
	if (iType == 1000 || iType == 2000)
	{
		for (int i = 0; i < m_vcTaskLog.size(); i++)
		{
			if (m_vcTaskLog[i].iTaskType == iType)
			{
				m_vcTaskLog[i].iCompTaskUser++;
				//_log(_DEBUG, "TH", "AddCompTaskLog00: iType[%d],[%d],size[%d]", iType, m_vcTaskLog[i].iCompTaskUser, m_vcTaskLog.size());
				return;
			}
		}
	}
	else
	{
		for (int i = 0; i < m_vcTaskLog.size(); i++)
		{
			if (m_vcTaskLog[i].iID == iID)
			{
				m_vcTaskLog[i].iCompTaskUser++;
				//_log(_DEBUG, "TH", "AddCompTaskLog11: iType[%d],[%d],size[%d]", iType, m_vcTaskLog[i].iCompTaskUser, m_vcTaskLog.size());
				return;
			}
		}
	}
	BKTaskLogDef log;
	log.iTaskType = iType;
	log.iID = iID;
	log.iCompTaskUser = 1;
	m_vcTaskLog.push_back(log);
	//_log(_DEBUG, "TH", "AddCompTaskLog22: iType[%d],id[%d],size[%d]", iType, iID, m_vcTaskLog.size());
}

void TaskHandler::CheckGetUserDecoratePropInfo(int iGameID, int iUserID, int iSocketIndex)
{
	//暂时没有装扮功能，有功能时数据存在user_growup_event和对应的redis缓存信息内
}

void TaskHandler::HandleComTaskCompInfo(char* szMsg, int iLen, int iSocketIndex)
{
	RdComTaskCompInfoDef* pMsgReq = (RdComTaskCompInfoDef*)szMsg;
	int iUserID = pMsgReq->iUserID;
	RdComOneTaskCompInfoDef* pOneTaskCompInfo = (RdComOneTaskCompInfoDef*)(szMsg + sizeof(RdComTaskCompInfoDef));
	for (int  i = 0; i < pMsgReq->iAllTaskNum; i++)
	{
		int iActType = pOneTaskCompInfo->iTaskInfo & 0xffff;
		int iActTaskID = (pOneTaskCompInfo->iTaskInfo>>16);
		_log(_DEBUG, "TH", "HandleComTaskCompInfo act[%d] ID[%d]", iActType, iActTaskID);
		switch ((COMTASK)iActType)
		{
		case COMTASK::ACHIEVE:
			UpdateUserAchieveInfo(pOneTaskCompInfo, iUserID, iSocketIndex);
			break;
		case COMTASK::TEST_COMTASK:
			break;
		case COMTASK::ACT_INVITE:
			UpdateActInviteInfo(pOneTaskCompInfo, iUserID, iSocketIndex);
			break;
		}
		pOneTaskCompInfo++;
	}
}
void TaskHandler::CheckGetComTaskInfo(int iReqGameID, int iUserID, int iSocketIndex)
{	
	char cBuffer[2048];	//扩容到2048
	memset(cBuffer, 0, sizeof(cBuffer));
	RdComTaskCompInfoDef *pMsgRes = (RdComTaskCompInfoDef*)cBuffer;
	int iMsgLen = sizeof(RdComTaskCompInfoDef);

	int iAllTaskNum = 0;

	//test code
	iAllTaskNum += JudgeComTestTask(iUserID, iReqGameID, cBuffer, iMsgLen);
	//成就任务
	iAllTaskNum += CheckGetUserAchieveInfoReq(iUserID, iReqGameID, cBuffer, iMsgLen);
	//邀请活动任务
	iAllTaskNum += CheckActInviteInfo(iUserID, iReqGameID, cBuffer, iMsgLen);

	if (iAllTaskNum > 0)
	{
		pMsgRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
		pMsgRes->msgHeadInfo.iMsgType = RD_COM_TASK_SYNC_INFO;
		pMsgRes->msgHeadInfo.iSocketIndex = iSocketIndex;
		pMsgRes->iUserID = iUserID;
		pMsgRes->iAllTaskNum = iAllTaskNum;
		if (m_pSendQueue)
		{
			m_pSendQueue->EnQueue(cBuffer, iMsgLen);
		}
	}
}

void TaskHandler::CheckGetRecentTask(int iReqGameID, int iUserID, int iSocketIndex)
{
	//获取玩家在iReqGameID中历史触发任务信息
	char cBuffer[1280] = { 0 };
	RdGetIntegralTaskMsgDef *pMsgRes = (RdGetIntegralTaskMsgDef*)cBuffer;
	pMsgRes->iGameID = iReqGameID;
	pMsgRes->iUserID = iUserID;
	pMsgRes->iRecentNum = 0;
	pMsgRes->msgHeadInfo.iMsgType = RD_INTEGRAL_TASK_HIS_RES;
	pMsgRes->msgHeadInfo.cVersion = MESSAGE_VERSION;
	pMsgRes->msgHeadInfo.iSocketIndex = iSocketIndex;
	int iLen = sizeof(RdGetIntegralTaskMsgDef);

	char cKey[128] = { 0 };
	sprintf(cKey, KEY_INTEGRAL_TASK_HIS.c_str(), iReqGameID, iUserID);
	string str = RedisHelper::get(m_redisCt, cKey);
	if (str.length() > 0)
	{
		vector<int> vcRt;
		Util::CutIntString(vcRt, str, "_");
		for (int i = 0; i < vcRt.size(); i++)
		{
			memcpy(cBuffer+iLen, &vcRt[i], sizeof(int));
			pMsgRes->iRecentNum++;
			iLen += sizeof(int);
		}
		m_pSendQueue->EnQueue(cBuffer, iLen);
	}
}

bool CompareRecentInfo(const pair<string, string>& x, const pair<string, string>& y) 
{
	int ti = 0, tj = 0, tk = 0;
	sscanf(x.second.c_str(), "%d_%d_%d_%d_%d", &ti, &ti, &ti, &ti, &tj);
	sscanf(y.second.c_str(), "%d_%d_%d_%d_%d", &ti, &ti, &ti, &ti, &tk);
	return tj > tk;
}

void TaskHandler::CheckGetUserDayInfo(int iReqGameID, int iUserID, int iSocketIndex)
{
	RdUserDayInfoResMsgDef resMsg;
	memset(&resMsg, 0, sizeof(resMsg));

	resMsg.msgHeadInfo.iMsgType = RD_USER_DAY_INFO_MSG;
	resMsg.msgHeadInfo.cVersion = MESSAGE_VERSION;
	resMsg.msgHeadInfo.iSocketIndex = iSocketIndex;
	resMsg.iUserID = iUserID;

	char cKey[128] = { 0 };
	sprintf(cKey, KEY_USER_DAY_INFO.c_str(), iUserID);
	bool bExist = RedisHelper::exists(m_redisCt, cKey);
	if (bExist)
	{
		string strRt = RedisHelper::hget(m_redisCt, cKey, "day_task_get_integral");
		if (strRt.length() > 0)
		{
			resMsg.iDayTaskIntegral = atoi(strRt.c_str());
			m_pSendQueue->EnQueue(&resMsg, sizeof(RdUserDayInfoResMsgDef));
		}
	}
}

void TaskHandler::DBGetBKTaskConf(map<int, BKTaskConfDef>& mapDayTask, map<int, BKTaskConfDef>& mapWeekTask)
{
	if (m_pMySqlGame == NULL)
	{
		_log(_ERROR, "ELH", "DBGetBKTaskConf -->>>>> m_pMySqlGame is null");
		return;
	}
	CSQLWrapperSelect hSQLSelect("user_task_conf", m_pMySqlGame->GetMYSQLHandle());
	hSQLSelect.AddQueryField("TaskID");
	hSQLSelect.AddQueryField("MainType");
	hSQLSelect.AddQueryField("SubType");
	hSQLSelect.AddQueryField("TaskGameID");
	hSQLSelect.AddQueryField("TaskName");
	hSQLSelect.AddQueryField("TaskNeedComplete");
	hSQLSelect.AddQueryField("TaskAwardProp1");
	hSQLSelect.AddQueryField("TaskAwardNum1");
	hSQLSelect.AddQueryField("TaskAwardProp2");
	hSQLSelect.AddQueryField("TaskAwardNum2");
	hSQLSelect.AddQueryField("IfStart");
	hSQLSelect.AddQueryConditionNumber("IfStart", 1, CSQLWrapper::ERelationType::Equal);
	//hSQLSelect.AddQueryConditionNumber("TaskID", 0, CSQLWrapper::ERelationType::Greater);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlGame->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "TH", "DBGetBKTaskConf err");
	}
	else
	{
		const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
		_log(_ERROR, "TH", "vecRecordList =[%d]", vecRecordList.size());
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			int iIfStart = hRecord.GetFieldAsInt32("IfStart");
			if (iIfStart == 1)
			{
				int TaskID = hRecord.GetFieldAsInt32("TaskID");
				BKTaskConfDef vcConf;
				memset(&vcConf, 0, sizeof(BKTaskConfDef));
				vcConf.iTaskID = TaskID;
				vcConf.iMainType = hRecord.GetFieldAsInt32("MainType");
				vcConf.iSubType = hRecord.GetFieldAsInt32("SubType");
				char szTaskGame[128] = {0};
				vector<string> vcStrInfo;
				strcpy(szTaskGame, hRecord.GetFieldAsString("TaskGameID").c_str());
				Util::CutString(vcStrInfo, szTaskGame, "&");
				for (int i = 0; i < vcStrInfo.size(); i++)
				{
					int iRoomType = 0;
					vector<string> vcStrID;
					Util::CutString(vcStrID, vcStrInfo[i].c_str(), "_");
					if (vcStrID.size() > 0)
					{
						vcConf.iGameID[i] = atoi(vcStrID[0].c_str());
						vcConf.iRoomType[i] = 0;
						if (vcStrID.size() > 1)
						{
							vcConf.iRoomType[i] = atoi(vcStrID[1].c_str());
						}
					}
				}
				
				strcpy(vcConf.szTaskName, hRecord.GetFieldAsString("TaskName").c_str());
				vcConf.iTaskNeedComplete = hRecord.GetFieldAsInt32("TaskNeedComplete");
				vcConf.iTaskAwardProp1 = hRecord.GetFieldAsInt32("TaskAwardProp1");
				vcConf.iTaskAwardNum1 = hRecord.GetFieldAsInt32("TaskAwardNum1");
				vcConf.iTaskAwardProp2 = hRecord.GetFieldAsInt32("TaskAwardProp2");
				vcConf.iTaskAwardNum2 = hRecord.GetFieldAsInt32("TaskAwardNum2");

				if (vcConf.iMainType == 0)
				{
					mapDayTask[TaskID] = vcConf;
				}
				else if(vcConf.iMainType == 1)
				{
					mapWeekTask[TaskID] = vcConf;
				}
			}
		}
	}
	//test
	map<int, BKTaskConfDef>::iterator pos = m_mapBKDayTask.begin();
	string strLog;
	char cTempLog[32] = { 0 };
	for (; pos != m_mapBKDayTask.end(); pos++)
	{
		strLog.clear();
		for (int j = 0; j < 20; j++)
		{
			if (pos->second.iGameID[j] > 0)
			{
				memset(cTempLog, 0, sizeof(cTempLog));
				sprintf(cTempLog, "%d,", pos->second.iGameID[j]);
				strLog += cTempLog;
			}
		}
		_log(_DEBUG, "TaskHandler", "day:id[%d][%d],sub[%d],award[%d,%d,%d,%d,%d],game[%s]", pos->second.iID, pos->second.iTaskID, pos->second.iSubType, pos->second.iTaskNeedComplete, pos->second.iTaskAwardProp1, pos->second.iTaskAwardNum1, pos->second.iTaskAwardProp2, pos->second.iTaskAwardNum2, strLog.c_str());
	}
	pos = m_mapBKWeekTask.begin();
	for (; pos != m_mapBKWeekTask.end(); pos++)
	{
		strLog.clear();
		for (int j = 0; j < 20; j++)
		{
			if (pos->second.iGameID[j] > 0)
			{
				memset(cTempLog, 0, sizeof(cTempLog));
				sprintf(cTempLog, "%d,", pos->second.iGameID[j]);
				strLog += cTempLog;
			}
		}
		_log(_DEBUG, "TaskHandler", "week:id[%d][%d],sub[%d],award[%d,%d,%d,%d,%d],game[%s]", pos->second.iID, pos->second.iTaskID, pos->second.iSubType, pos->second.iTaskNeedComplete, pos->second.iTaskAwardProp1, pos->second.iTaskAwardNum1, pos->second.iTaskAwardProp2, pos->second.iTaskAwardNum2, strLog.c_str());
	}
	//test end
}

void TaskHandler::OnTimerGetAchieveTask()
{
	CSQLWrapperSelect hSQLSelect("achieve_task_conf", m_pMySqlGame->GetMYSQLHandle());
	hSQLSelect.AddQueryField("TaskID");
	hSQLSelect.AddQueryField("TaskType");
	hSQLSelect.AddQueryField("GameIds");
	hSQLSelect.AddQueryField("TaskAward");
	hSQLSelect.AddQueryConditionNumber("IfStart", 1, CSQLWrapper::ERelationType::Equal);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlGame->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "TH", "OnTimerGetAchieveTask error");
	}
	else
	{
		m_mapDKAchieveTask.clear();
		const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
		std::vector<std::string> strGameIds;
		std::vector<std::string> strTaskAward;
		std::vector<std::string> strTemp;
		_log(_DEBUG, "TH", "OnTimerGetAchieveTask vecRecordList size[%d]", vecRecordList.size());
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			DKAchieveTaskDef achieve;

			achieve.iTaskID = hRecord.GetFieldAsInt32("TaskID");
			achieve.iTaskType = hRecord.GetFieldAsInt32("TaskType");

			char szGameIds[32] = { 0 };
			strcpy(szGameIds, hRecord.GetFieldAsString("GameIds").c_str());

			strGameIds.clear();
			Util::CutString(strGameIds, szGameIds, "_");		//格式：99_11_12

			for (int i = 0; i < strGameIds.size(); i++)
			{
				int iGameID = atoi(strGameIds.at(i).c_str());
				achieve.vcGameId.push_back(iGameID);
			}

			char szTaskAward[128] = { 0 };
			strcpy(szTaskAward, hRecord.GetFieldAsString("TaskAward").c_str());

			strTaskAward.clear();
			Util::CutString(strTaskAward, szTaskAward, "#");	//格式：1_1_10#2_3_30#3_5_50

			for (int i = 0; i < strTaskAward.size(); i++)
			{
				strTemp.clear();
				Util::CutString(strTemp, strTaskAward[i].c_str(), "_");	//格式：1_1_10

				if (strTemp.size() == 3)
				{
					AchieveLevel level;

					level.iLevel = atoi(strTemp.at(0).c_str());
					level.iNeedNum = atoi(strTemp.at(1).c_str());
					level.iAchievePoints = atoi(strTemp.at(2).c_str());

					achieve.vcTaskAward.push_back(level);
				}
			}
			
			m_mapDKAchieveTask[achieve.iTaskID] = achieve;

			_log(_DEBUG, "TH", "OnTimerGetAchieveTask num[%d] tid[%d] gameid[%s] award[%s]", m_mapDKAchieveTask.size(), achieve.iTaskID, szGameIds, szTaskAward);
		}		
	}
}

DKAchieveTaskDef* TaskHandler::findDKAchieveTask(int iTaskID)
{
	map<int, DKAchieveTaskDef>::iterator pos = m_mapDKAchieveTask.find(iTaskID);
	if (pos != m_mapDKAchieveTask.end())
	{
		return &pos->second;
	}

	return NULL;
}

int TaskHandler::CheckGetUserAchieveInfoReq(int iUserID, int iGameID, char * cBuffer, int & iMsgLen)
{
	int iAllArchieveTaskNum = 0;
	_log(_DEBUG, "TH", "CheckGetUserAchieveInfoReq uid[%d] gameid[%d]", iUserID, iGameID);
	CSQLWrapperSelect hSQLSelect("user_achieve_info", m_pMySqlUser->GetMYSQLHandle());
	hSQLSelect.AddQueryField("TaskID");
	hSQLSelect.AddQueryField("TaskType");
	hSQLSelect.AddQueryField("AwardIndex");
	hSQLSelect.AddQueryField("CompleteNum");
	hSQLSelect.AddQueryField("BuffExtra");
	hSQLSelect.AddQueryConditionNumber("Userid", iUserID);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlUser->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "TH", "CheckGetUserAchieveInfoReq find uid[%d] info err", iUserID);
		return 0;
	}

	const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
	if (vecRecordList.empty())
	{
		_log(_ERROR, "TH", "CheckGetUserAchieveInfoReq no find uid[%d] info", iUserID);
		return 0;
	}
	for (const CMySQLTableRecord& hRecord : vecRecordList)
	{
		int iTaskID = hRecord.GetFieldAsInt32("TaskID");
		int iTaskType = hRecord.GetFieldAsInt32("TaskType");
		int iCompleteNum = hRecord.GetFieldAsInt32("CompleteNum");
				
		DKAchieveTaskDef* taskConf = findDKAchieveTask(iTaskID);
		if (taskConf == NULL)  //找不到这个任务 应该是关掉了 不处理
		{
			continue;
		}

		int iMaxCompleteNum = 0;	//最大档位完成值
		for (int i = 0; i < taskConf->vcTaskAward.size(); i++)
		{
			int iNeedNum = taskConf->vcTaskAward.at(i).iNeedNum;
			if (iMaxCompleteNum < iNeedNum)
			{
				iMaxCompleteNum = iNeedNum;
			}
		}

		if (iCompleteNum >= iMaxCompleteNum)	//已完成任务的所有档位 不用累计了
		{
			continue;
		}

		bool bIfHaveGameId = false;	 //当前游戏是否参与任务
		for (int i = 0; i < taskConf->vcGameId.size(); i++)
		{
			if (taskConf->vcGameId.at(i) == iGameID)
			{
				bIfHaveGameId = true;
				break;
			}
		}

		if (bIfHaveGameId)
		{
			RdComOneTaskInfoDef* pOneTaskInfo = (RdComOneTaskInfoDef*)(cBuffer + iMsgLen);
			iMsgLen += sizeof(RdComOneTaskInfoDef);
			pOneTaskInfo->iTaskInfo = (iTaskID << 16) | (int)COMTASK::ACHIEVE;
			pOneTaskInfo->iTaskType = iTaskType;
			pOneTaskInfo->iNowComp = iCompleteNum;
			pOneTaskInfo->iAllNeedComp = iMaxCompleteNum;
			iAllArchieveTaskNum++;
		}
	}
	return iAllArchieveTaskNum;
}

void TaskHandler::UpdateUserAchieveInfo(RdComOneTaskCompInfoDef* pTaskInfo, int iUserID, int iSocketIndex)
{
	int iTaskID = pTaskInfo->iTaskInfo >> 16;
	_log(_DEBUG, "TH", "UpdateUserAchieveInfo uid[%d] tid[%d] num[%d]", iUserID, iTaskID, pTaskInfo->iCompAddNum);
	DKAchieveTaskDef* taskConf = findDKAchieveTask(iTaskID);
	if (taskConf == NULL)  //找不到这个任务 应该是关掉了 不处理
	{
		_log(_ERROR, "TH", "UpdateUserAchieveInfo no find task uid[%d] tid[%d]", iUserID, iTaskID);
		return;
	}

	CSQLWrapperSelect hSQLSelect("user_achieve_info", m_pMySqlUser->GetMYSQLHandle());
	hSQLSelect.AddQueryField("TaskType");
	hSQLSelect.AddQueryField("AwardIndex");
	hSQLSelect.AddQueryField("CompleteNum");
	hSQLSelect.AddQueryField("BuffExtra");
	hSQLSelect.AddQueryConditionNumber("Userid", iUserID);
	hSQLSelect.AddQueryConditionNumber("TaskID", iTaskID);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlUser->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult, true))
	{
		_log(_ERROR, "TH", "UpdateUserAchieveInfo find uid[%d] tid[%d] info err", iUserID, iTaskID);
		return;
	}

	const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
	if (vecRecordList.empty())
	{
		_log(_ERROR, "TH", "UpdateUserAchieveInfo no find uid[%d] tid[%d] info", iUserID, iTaskID);
		return;
	}	

	int iCompleteNum = 0;
	char szGetAwardIndex[32] = { 0 };
	for (const CMySQLTableRecord& hRecord : vecRecordList)
	{
		iCompleteNum = hRecord.GetFieldAsInt32("CompleteNum");
		strcpy(szGetAwardIndex, hRecord.GetFieldAsString("AwardIndex").c_str());
	}

	//已经完成的任务档位
	std::vector<std::string> strGetIndex;
	Util::CutString(strGetIndex, szGetAwardIndex, "_");		//格式：1_2_3

	std::vector<int> vcGetIndex;
	for (int i = 0; i < strGetIndex.size(); i++)
	{
		int iIndex = atoi(strGetIndex.at(i).c_str());
		vcGetIndex.push_back(iIndex);
	}
	
	int iMaxCompleteNum = 0;	//最大的档位完成值
	vector<AchieveLevel> vcAllAward = taskConf->vcTaskAward;	//所有的任务档位	
	for (int i = 0; i < vcAllAward.size(); i++)
	{
		if (iMaxCompleteNum < vcAllAward.at(i).iNeedNum)
		{
			iMaxCompleteNum = vcAllAward.at(i).iNeedNum;
		}		
	}

	int iAddCompleteNum = pTaskInfo->iCompAddNum;
	if (iCompleteNum + iAddCompleteNum > iMaxCompleteNum)	//不能超过最大档位的完成值
	{
		iAddCompleteNum = iMaxCompleteNum - iCompleteNum;
	}
	if (iCompleteNum + iAddCompleteNum < 0)	//不能小于0
	{
		iAddCompleteNum = -iCompleteNum;
	}

	_log(_DEBUG, "TH", "UpdateUserAchieveInfo uid[%d] tid[%d] type[%d] copNum[%d] addNum[%d] maxNum[%d]", iUserID, iTaskID, taskConf->iTaskType, iCompleteNum, iAddCompleteNum, iMaxCompleteNum);

	//等待完成的任务档位
	std::vector<AchieveLevel> vcWaitAward;
	for (int i = 0; i < vcAllAward.size(); i++)
	{
		bool bFind = false;
		for (int j = 0; j < vcGetIndex.size(); j++)
		{
			if (vcAllAward.at(i).iLevel == vcGetIndex.at(j))
			{
				bFind = true;
				break;
			}
		}

		if (!bFind)
		{
			vcWaitAward.push_back(vcAllAward.at(i));
		}
	}

	//累计进度后 判断待完成任务里是否有可以完成的任务档位
	int iNextCompleteNum = iCompleteNum + iAddCompleteNum;

	int iGetAchievePoints = 0;	//获得的成就点
	for (int i = 0; i < vcWaitAward.size(); i++)
	{
		if (iNextCompleteNum >= vcWaitAward.at(i).iNeedNum)
		{
			SRedisComStatMsgDef comEvent;
			memset(&comEvent, 0, sizeof(SRedisComStatMsgDef));
			comEvent.msgHeadInfo.iMsgType = RD_REPORT_COMMON_EVENT_MSG;
			comEvent.msgHeadInfo.cVersion = MESSAGE_VERSION;
			comEvent.cllEventAddCnt[0] = 1;
			comEvent.iEventID = 13;
			comEvent.iSubEventID[0] = taskConf->iTaskType * 100000 + vcWaitAward.at(i).iNeedNum;
			int iRes = m_pEventGetQueue->EnQueue((void*)&comEvent, sizeof(SRedisComStatMsgDef));
			if (iRes!=0 )
			{
				_log(_ERROR,"EnQueue_log","UpdateUserAchieveInfo iUserID[%d] iTaskID[%d]",iUserID, iTaskID);
			}
			vcGetIndex.push_back(vcWaitAward.at(i).iLevel);
			iGetAchievePoints += vcWaitAward.at(i).iAchievePoints;
		}
	}

	if (iGetAchievePoints > 0)
	{
		_log(_DEBUG, "TH", "UpdateUserAchieveInfo get achieve uid[%d] num[%d]", iUserID, iGetAchievePoints);

		//更新成就点
		CSQLWrapperUpdate hSQLUpdate("user_extra_info", m_pMySqlUser->GetMYSQLHandle());
		hSQLUpdate.AddFieldValueNumber("AchievePoints", iGetAchievePoints);
		hSQLUpdate.AddQueryConditionNumber("Userid", iUserID);
		const std::string& strSQLUpdate = hSQLUpdate.GetFinalSQLString();
		if (!m_pMySqlUser->RunSQL(strSQLUpdate, true))
		{
			_log(_ERROR, "TH", "UpdateUserAchieveInfo add achieve fail uid[%d]", iUserID);
			return;
		}

		//更新当前已领取的任务档位
		memset(szGetAwardIndex, 0, sizeof(szGetAwardIndex));
		for (int i = 0; i < vcGetIndex.size(); i++)
		{
			if (i == 0)
			{
				sprintf(szGetAwardIndex, "%d", vcGetIndex.at(i));
			}
			else
			{
				sprintf(szGetAwardIndex, "%s_%d", szGetAwardIndex, vcGetIndex.at(i));
			}
		}
	}

	//完成某个档位 发放成就点
	CSQLWrapperUpdate hSQLUpdate("user_achieve_info", m_pMySqlUser->GetMYSQLHandle());
	hSQLUpdate.AddFieldValueNumber("CompleteNum", iAddCompleteNum);
	hSQLUpdate.SetFieldValueString("AwardIndex", szGetAwardIndex);
	hSQLUpdate.AddQueryConditionNumber("Userid", iUserID);
	hSQLUpdate.AddQueryConditionNumber("TaskID", iTaskID);
	const std::string& strSQLUpdate = hSQLUpdate.GetFinalSQLString();
	if (!m_pMySqlUser->RunSQL(strSQLUpdate, true))
	{
		_log(_ERROR, "TH", "UpdateUserAchieveInfo update fail uid[%d] tid[%d]", iUserID, iTaskID);
	}

	if (iGetAchievePoints > 0) //获得成就点 给服务器回应
	{
		RdUpdateUserAchieveInfoResDef msgRes;
		memset(&msgRes, 0, sizeof(RdUpdateUserAchieveInfoResDef));
		msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgRes.msgHeadInfo.iMsgType = RD_UPDATE_USER_ACHIEVE_INFO_MSG;
		msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
		msgRes.iUserID = iUserID;
		msgRes.iTaskType = taskConf->iTaskType;
		msgRes.iCompleteNum = iNextCompleteNum;
		msgRes.iAchievePoint = iGetAchievePoints;

		if (m_pSendQueue)
		{
			m_pSendQueue->EnQueue((void*)&msgRes, sizeof(RdUpdateUserAchieveInfoResDef));
		}
	}
}

int TaskHandler::JudgeComTestTask(int iUserID, int iReqGameID, char * cBuffer, int & iMsgLen)
{
	char szKey[32];
	memset(szKey, 0, sizeof(szKey));
	sprintf(szKey, "TEST_COMTASK:%d", iUserID);
	bool bExist = RedisHelper::exists(m_redisCt, szKey);
	if (bExist)
	{
		//已触发任务并未完成
		string strComplete = RedisHelper::hget(m_redisCt, szKey, "complete");
		int iComplete = 0;
		if (strComplete.size() > 0)
		{
			iComplete = atoi(strComplete.c_str());
		}
		string strMaxNeed = RedisHelper::hget(m_redisCt, szKey, "need");
		int iMaxNeed = 0;
		if (strMaxNeed.size() > 0)
		{
			iMaxNeed = atoi(strMaxNeed.c_str());
		}
		//检查请求的gameid是否是任务相关id
		string strGameID = RedisHelper::hget(m_redisCt, szKey, "game");
		std::vector<int> resVec;
		if (strGameID.size() > 0)
		{
			//方便截取最后一段数据
			std::string strs = strGameID + ',';

			size_t pos = strs.find(',');
			size_t size = strs.size();

			while (pos != std::string::npos)
			{
				std::string x = strs.substr(0, pos);
				resVec.push_back(atoi(x.c_str()));
				strs = strs.substr(pos + 1, size);
				pos = strs.find(',');
			}
		}
		bool bIfTaskGame = false;
		for (int i = 0; i<resVec.size(); i++)
		{
			if (resVec[i] == iReqGameID)
			{
				bIfTaskGame = true;
				break;
			}
		}
		if (bIfTaskGame && iComplete < iMaxNeed)
		{
			RdComOneTaskInfoDef* pOneTaskInfo = (RdComOneTaskInfoDef*)(cBuffer + iMsgLen);
			iMsgLen += sizeof(RdComOneTaskInfoDef);
			//对pOneTaskInfo赋值任务详情
			pOneTaskInfo->iAllNeedComp = iMaxNeed;
			pOneTaskInfo->iNowComp = iComplete;
			pOneTaskInfo->iTaskInfo = (int)COMTASK::TEST_COMTASK;
			pOneTaskInfo->iTaskType = (int)DayWeekTaskType::HIT_BACK_IN_GAME;
			return 1;
		}
	}
	return 0;
}

void TaskHandler::UpdateActInviteInfo(RdComOneTaskCompInfoDef* pTaskInfo, int iUserID, int iSocketIndex)
{
	if (m_mapActInvite.size() == 0)
	{
		_log(_ERROR, "TH", "UpdateActInviteInfo params not conf user[%d]", iUserID);
		return;
	}

	//更新玩家邀请活动进度
	CSQLWrapperSelect hSQLUser("user_invite_info", m_pMySqlUser->GetMYSQLHandle());
	hSQLUser.AddQueryField("InviteId");
	hSQLUser.AddQueryField("AwardStatus");
	hSQLUser.AddQueryField("CompleteNum");
	hSQLUser.AddQueryConditionNumber("Userid", iUserID);
	CMySQLTableResult hUserSQLResult;

	if (!m_pMySqlUser->RunSQL(hSQLUser.GetFinalSQLString(), hUserSQLResult))
	{
		_log(_ERROR, "TH", "UpdateActInviteInfo find uid[%d] info err", iUserID);
		return;
	}
	const std::vector<CMySQLTableRecord>& vecRetList = hUserSQLResult.GetRecordList();
	if (vecRetList.empty())
	{
		_log(_ERROR, "TH", "UpdateActInviteInfo no find uid[%d] info", iUserID);
		return;
	}

	//检查是否完成某档任务，更新奖励待领取信息
	int iCmopNum = 0;
	string strNowAward = "";
	vector<int> vcAwardID;   //已完成任务编号
	for (const CMySQLTableRecord& hRecord : vecRetList)
	{
		int iInviteID = hRecord.GetFieldAsInt32("InviteId");
		iCmopNum = hRecord.GetFieldAsInt32("CompleteNum");
		strNowAward = hRecord.GetFieldAsString("AwardStatus");
		std::vector<std::string> strInfo;
		Util::CutString(strInfo, strNowAward.c_str(), "&");
		if (!strInfo.empty())
		{
			for (int i = 0; i < strInfo.size(); i++)
			{
				int iConf[4] = { 0 };  //奖励编号_领取状态&
				Util::ParaseParamValue((char*)strInfo[i].c_str(), iConf, "_");
				vcAwardID.push_back(iConf[0]);
			}
		}
	}
	if (iCmopNum < m_iActInviteNeed)  //还可以累计
	{
		//检查当前可以完成第几档任务
		int iNewComp = iCmopNum + pTaskInfo->iCompAddNum;
		bool bSendToClient = false;
		map<int, int>::iterator pos = m_mapActInvite.begin();
		for (; pos != m_mapActInvite.end(); pos++)
		{
			bool bFind = false;
			for (int i = 0; i < vcAwardID.size(); i++)
			{
				if (vcAwardID[i] == pos->first)
				{
					bFind = true;
					break;
				}
			}
			if (!bFind)  //待完成任务，检测是否完成
			{
				if (iNewComp >= pos->second)
				{
					char cTmp[32] = { 0 };
					if (strNowAward.length() > 0)
						sprintf(cTmp, "&%d_1", pos->first);
					else
						sprintf(cTmp, "%d_1", pos->first);
					strNowAward += cTmp;
					bSendToClient = true;
				}
			}
		}

		//刷新玩家待领取奖励及进度
		CSQLWrapperUpdate hSQLUpdate("user_invite_info", m_pMySqlUser->GetMYSQLHandle());
		hSQLUpdate.AddFieldValueNumber("CompleteNum", pTaskInfo->iCompAddNum);
		hSQLUpdate.SetFieldValueString("AwardStatus", strNowAward);
		hSQLUpdate.AddQueryConditionNumber("Userid", iUserID);
		const std::string& strSQLUpdate = hSQLUpdate.GetFinalSQLString();
		if (!m_pMySqlUser->RunSQL(strSQLUpdate, true))
		{
			_log(_ERROR, "TH", "UpdateActInviteInfo fail [%s]", strSQLUpdate.c_str());
		}

		if (bSendToClient)  //完成任务
		{		
			//插入玩家待领取状态，growup event 5
			UpdateUserActInviteRed(iUserID, true);
		}
	}
}

void TaskHandler::UpdateUserActInviteRed(int iUserID, bool bRed)
{
	int iGrowEventId = 5;
	CSQLWrapperSelect hSQLSelect("user_growup_event", m_pMySqlUser->GetMYSQLHandle());
	hSQLSelect.AddQueryField("GrowNumInfo");
	hSQLSelect.AddQueryField("GrowStrInfo");
	hSQLSelect.AddQueryConditionNumber("Userid", iUserID);
	hSQLSelect.AddQueryConditionNumber("GrowEventId", iGrowEventId);

	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlUser->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult, true))
	{
		_log(_ERROR, "TH", "UpdateUserActInviteRed find error uid[%d] iGrowEventId[%d]", iUserID, iGrowEventId);
		return;
	}

	int iGrowNumInfo = 0;
	char szGrowStrInfo[128] = { 0 };
	const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
	if (vecRecordList.empty())
	{
		iGrowNumInfo |= 1;
		CSQLWrapperInsert hSQLInsert("user_growup_event", m_pMySqlUser->GetMYSQLHandle());
		hSQLInsert.SetFieldValueNumber("Userid", iUserID);
		hSQLInsert.SetFieldValueNumber("GrowEventId", iGrowEventId);
		hSQLInsert.SetFieldValueNumber("GrowNumInfo", iGrowNumInfo);
		const std::string& strSQLInsert = hSQLInsert.GetFinalSQLString();
		m_pMySqlUser->RunSQL(strSQLInsert, true);
	}
	else
	{
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			iGrowNumInfo = hRecord.GetFieldAsInt32("GrowNumInfo");
		}
		iGrowNumInfo |= 1;
		CSQLWrapperUpdate hSQLUpdate("user_growup_event", m_pMySqlUser->GetMYSQLHandle());
		hSQLUpdate.SetFieldValueNumber("GrowNumInfo", iGrowNumInfo);
		hSQLUpdate.AddQueryConditionNumber("Userid", iUserID);
		hSQLUpdate.AddQueryConditionNumber("GrowEventId", iGrowEventId);
		const std::string& strSQLUpdate = hSQLUpdate.GetFinalSQLString();
		bool bRt = m_pMySqlUser->RunSQL(strSQLUpdate, true);
		if (!bRt)
		{
			_log(_ERROR, "TH", "UpdateUserActInviteRed update error uid[%d] iGrowEventId[%d] iGrowNumInfo[%d]", iUserID, iGrowEventId, iGrowNumInfo);
			return;
		}
	}
}

void TaskHandler::GetParamsConf()
{
	string strReq = "";
	strReq += "activity_begin_time_1";
	strReq += "&activity_end_time_1";
	strReq += "&invite_player_award_jushu";
	strReq += "&game11_room_task_conf";
	strReq += "&if_open_slot_pusch_activity";
	strReq += "&if_open_Xmas_activity";
	CSQLWrapperSelect hSQLSelect("params", m_pMySqlUser->GetMYSQLHandle());
	hSQLSelect.AddQueryField("ParamKey");
	hSQLSelect.AddQueryField("ParamValue");
	hSQLSelect.AddQueryConditionStr("ParamKey", strReq, CSQLWrapper::ERelationType::StringIn);
	CMySQLTableResult hMySQLTableResult;
	if (!m_pMySqlUser->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult))
	{
		_log(_ERROR, "TH", "GetActInviteConf find params info err");
		return;
	}

	const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
	if (vecRecordList.empty())
	{
		_log(_ERROR, "TH", "GetActInviteConf params no find");
		return;
	}
	//检查是否在活动期间
	m_iActInviteNeed = 0;
	time_t tmNow = time(NULL);
	for (const CMySQLTableRecord& hRecord : vecRecordList)
	{
		string strKey = hRecord.GetFieldAsString("ParamKey");
		string strValue = hRecord.GetFieldAsString("ParamValue");
		if (strcmp(strKey.c_str(), "activity_begin_time_1") == 0)
		{
			struct tm timeinfo;
			strptime(strValue.c_str(), "%Y-%m-%d %H:%M:%S", &timeinfo);
			time_t timeStamp = mktime(&timeinfo);
		}
		else if (strcmp(strKey.c_str(), "activity_end_time_1") == 0)
		{
			struct tm timeinfo;
			strptime(strValue.c_str(), "%Y-%m-%d %H:%M:%S", &timeinfo);
			time_t timeStamp = mktime(&timeinfo);
		}
		else if (strcmp(strKey.c_str(), "invite_player_award_jushu") == 0)
		{
			std::vector<std::string> strInfo;
			Util::CutString(strInfo, strValue.c_str(), "&");
			if (!strInfo.empty())
			{
				for (int i = 0; i < strInfo.size(); i++)
				{
					int iConf[4] = { 0 };  //奖励编号_局数_道具id_道具数量
					Util::ParaseParamValue((char*)strInfo[i].c_str(), iConf, "_");
					m_mapActInvite[iConf[0]] = iConf[1];
					if (iConf[1] > 0 && m_iActInviteNeed < iConf[1])
					{
						m_iActInviteNeed = iConf[1];
					}
				}
			}
		}
	}
}

int TaskHandler::CheckActInviteInfo(int iUserID, int iGameID, char * cBuffer, int & iMsgLen)
{
	int iAllInviteTaskNum = 0;
	_log(_DEBUG, "TH", "CheckActInviteInfo uid[%d] gameid[%d]", iUserID, iGameID);

	//检查是否在活动期间
	int iMaxNeed = m_iActInviteNeed;
	//检查是否触发邀请任务，并且任务尚未完成
	if (m_mapActInvite.size() > 0)
	{
		CSQLWrapperSelect hSQLUser("user_invite_info", m_pMySqlUser->GetMYSQLHandle());
		hSQLUser.AddQueryField("InviteId");
		hSQLUser.AddQueryField("CompleteNum");
		hSQLUser.AddQueryConditionNumber("Userid", iUserID);
		CMySQLTableResult hUserSQLResult;

		if (!m_pMySqlUser->RunSQL(hSQLUser.GetFinalSQLString(), hUserSQLResult))
		{
			_log(_ERROR, "TH", "CheckActInviteInfo find uid[%d] info err", iUserID);
			return 0;
		}
		const std::vector<CMySQLTableRecord>& vecRetList = hUserSQLResult.GetRecordList();
		if (vecRetList.empty())
		{
			_log(_ERROR, "TH", "CheckActInviteInfo no find uid[%d] info", iUserID);
			return 0;
		}

		//检查任务数量
		for (const CMySQLTableRecord& hRecord : vecRetList)
		{
			int iInviteID = hRecord.GetFieldAsInt32("InviteId");
			int iCmopNum = hRecord.GetFieldAsInt32("CompleteNum");
			if (iInviteID > 0 && iCmopNum < iMaxNeed)
			{
				RdComOneTaskInfoDef* pOneTaskInfo = (RdComOneTaskInfoDef*)(cBuffer + iMsgLen);
				iMsgLen += sizeof(RdComOneTaskInfoDef);
				pOneTaskInfo->iTaskInfo = (1 << 16) | (int)COMTASK::ACT_INVITE;
				pOneTaskInfo->iTaskType = (int)DayWeekTaskType::COMPLETE_GAME;
				pOneTaskInfo->iNowComp = iCmopNum;
				pOneTaskInfo->iAllNeedComp = iMaxNeed;
				iAllInviteTaskNum++;
			}
		}
	}
	return iAllInviteTaskNum;
}

//同桌玩家信息
void TaskHandler::HandleTogetherUserMsg(char * msgData, int iDataLen, int iSocketIndex)
{
	RdGameTogetherUserReqDef *pMsgReq = (RdGameTogetherUserReqDef*)msgData;
	int iUserID = pMsgReq->iUserID;
	char strKey[64] = { 0 };
	char cField[128] = { 0 };
	char cValue[256] = { 0 };
	sprintf(strKey, KEY_RECENT_GAME_INFO.c_str(), pMsgReq->iGameID, iUserID);
	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	TogetherUserDef *pUserInfo = (TogetherUserDef*)(pMsgReq + 1);
	time_t iTmNow = time(NULL);
	if (pMsgReq->iUserNum > 0)
	{
		vector<int> allIds;

		string strRt = RedisHelper::hget(m_redisCt, strKey, "all_ids");
		if (!strRt.empty())
		{
			std::vector<std::string> vcVal;
			Util::CutString(vcVal, strRt, "_");
			for (int i = 0; i < vcVal.size(); i++)
			{
				int iUserID = atoi(vcVal.at(i).c_str());
				allIds.push_back(iUserID);
			}
		}

		for (int i = 0; i < pMsgReq->iUserNum; i++)
		{
			for (int j = 0; j < allIds.size(); j++)
			{
				if (allIds.at(j) == pUserInfo->iUserID)
				{
					allIds.erase(allIds.begin() + j);	//先删掉原来的id链表
					break;
				}
			}

			allIds.push_back(pUserInfo->iUserID);	//连接到链表后面

			sprintf(cField, "%d", pUserInfo->iUserID);
			sprintf(cValue, "6_%d_%d_%d_%d_%d_%s", pUserInfo->iHeadImg, pUserInfo->iHeadFrameId, pUserInfo->iExp, iTmNow, pMsgReq->iBasePoint, pUserInfo->szNickName);
			RedisHelper::hset(m_redisCt, strKey, cField, cValue);

			pUserInfo++;
		}

		//超过30条 删除前面的数据 只保留后面的30条
		while (allIds.size() > 30)
		{
			sprintf(cField, "%d", allIds.at(0));
			RedisHelper::hdel(m_redisCt, strKey, cField);

			allIds.erase(allIds.begin());
		}

		//更新id链表
		string strIds;
		for (int i = 0; i < allIds.size(); i++)
		{
			if (i > 0)
			{
				strIds.append("_");
			}

			strIds.append(std::to_string(allIds.at(i)));
		}
			
		RedisHelper::hset(m_redisCt, strKey, "all_ids", strIds);

		RedisHelper::expire(m_redisCt, strKey, 3600 * 24 * 7);
	}
}

void TaskHandler::HandleGetRoomTaskReq(char * szMsg, int iLen, int iSocketIndex)
{
	RdGetRoomTaskInfoReqDef *pMsgReq = (RdGetRoomTaskInfoReqDef*)szMsg;
	int iGameID = pMsgReq->iGameID;

	RdGameRoomTaskInfoResDef msgRes;
	memset(&msgRes, 0, sizeof(RdGameRoomTaskInfoResDef));
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = RD_GAME_ROOM_TASK_INFO_MSG;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	msgRes.iUserID = pMsgReq->iUserID;
	msgRes.iRoomType = pMsgReq->iRoomType;
	msgRes.iGameID = iGameID;

	//检查是否已触发
	char strKey[128];
	memset(strKey, 0, sizeof(pMsgReq));
	sprintf(strKey, KEY_ROOM_TASK.c_str(), pMsgReq->iUserID, pMsgReq->iGameID, pMsgReq->iRoomType);
	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	if (bExist)
	{
		//存在则返回当前存储数据
		string strValue = RedisHelper::get(m_redisCt, strKey);
		vector<string> vcStrInfo;
		Util::CutString(vcStrInfo, strValue, "_");
		if (vcStrInfo.size() != 4)
		{
			bExist = false;
		}
		else
		{
			sscanf(strValue.c_str(), "%d_%d_%d_%d", &msgRes.iCompNum, &msgRes.iNeedNum, &msgRes.iAwardID, &msgRes.iAwardNum);
		}
		if(pMsgReq->iReqType > 0)
		{
			msgRes.iCompNum += pMsgReq->iReqType;
			if (msgRes.iCompNum >= msgRes.iNeedNum)  //完成后直接清掉，并取消返回
			{
				RedisHelper::del(m_redisCt, strKey);
				return;
			}
			else  //更新进度
			{
				char szVale[128] = { 0 };
				sprintf(szVale, "%d_%d_%d_%d", msgRes.iCompNum, msgRes.iNeedNum, msgRes.iAwardID, msgRes.iAwardNum);
				RedisHelper::set(m_redisCt, strKey, szVale);
				RedisHelper::expire(m_redisCt, strKey, 7*24*3600);
			}
		}
	}
	if (!bExist) //如果未触发，查找配置并触发任务
	{
		map<int, string>::iterator itor = m_mapRoomTask.find(iGameID);
		if (itor == m_mapRoomTask.end())  //没有配置，直接返回
		{
			_log(_ERROR, "TH", "HandleGetRoomTaskReq not find taks conf!");
			return;
		}

		std::vector<std::string> strInfo;
		Util::CutString(strInfo, itor->second.c_str(), "&");
		if (strInfo.empty())
		{
			return;
		}

		for (int i = 0; i < strInfo.size(); i++)
		{
			std::vector<std::string> strConf;
			Util::CutString(strConf, strInfo[i].c_str(), "_");
			if (strConf.size() == 4)
			{
				int iRoomType, iNeedNum, iAwardID, iAwardNum;
				int iRt = sscanf(strInfo[i].c_str(), "%d_%d_%d_%d", &iRoomType, &iNeedNum, &iAwardID, &iAwardNum);
				if (iRt > 0 && iRoomType == pMsgReq->iRoomType)
				{
					char szVale[128] = { 0 };
					msgRes.iNeedNum = iNeedNum;
					msgRes.iAwardID = iAwardID;
					msgRes.iAwardNum = iAwardNum;
					sprintf(szVale, "%d_%d_%d_%d", 0, msgRes.iNeedNum, msgRes.iAwardID, msgRes.iAwardNum);
					RedisHelper::set(m_redisCt, strKey, szVale);
					RedisHelper::expire(m_redisCt, strKey, 7 * 24 * 3600);
					break;
				}
			}
		}
	}

	if (m_pSendQueue && msgRes.iNeedNum > 0)
	{
		m_pSendQueue->EnQueue((void*)&msgRes, sizeof(RdGameRoomTaskInfoResDef));
	}
}

void TaskHandler::HandleGetIntegralTaskInfoMsg(char *msgData, int iLen, int iSocketIndex)
{
	GameGetIntegralTaskInfoReqDef* msgReq = (GameGetIntegralTaskInfoReqDef*)msgData;

	int iGetTaskTable = msgReq->iReqFlag & 0x01;
	int iGetTaskPool = (msgReq->iReqFlag >> 1) & 0x01;

	_log(_DEBUG, "TH", "HandleGetIntegralTaskInfoMsg iGameID[%d] iGetTaskTable[%d] iGetTaskPool[%d]", msgReq->iGameID, iGetTaskTable, iGetTaskPool);

	vector<IntegralTaskTableDef> vcTaskTable;

	if (iGetTaskTable == 1)
	{
		CSQLWrapperSelect hSQLSelect("integral_task_table", m_pMySqlGame->GetMYSQLHandle());
		hSQLSelect.AddQueryField("table_grade");
		hSQLSelect.AddQueryField("min_integral");
		hSQLSelect.AddQueryField("max_integral");
		hSQLSelect.AddQueryField("award_weight1");
		hSQLSelect.AddQueryField("award_weight2");
		hSQLSelect.AddQueryField("award_weight3");
		hSQLSelect.AddQueryField("award_weight4");
		hSQLSelect.AddQueryField("award_weight5");
		hSQLSelect.AddQueryField("award_weight6");
		hSQLSelect.AddQueryConditionNumber("game_id", msgReq->iGameID);

		CMySQLTableResult hMySQLTableResult;
		if (!m_pMySqlGame->RunSQL(hSQLSelect.GetFinalSQLString(), hMySQLTableResult, true))
		{
			_log(_ERROR, "TH", "HandleGetIntegralTaskInfoMsg find iGameID[%d] err", msgReq->iGameID);
			return;
		}

		const std::vector<CMySQLTableRecord>& vecRecordList = hMySQLTableResult.GetRecordList();
		for (const CMySQLTableRecord& hRecord : vecRecordList)
		{
			IntegralTaskTableDef table;
			memset(&table, 0, sizeof(IntegralTaskTableDef));

			table.iGameID = msgReq->iGameID;
			table.iTableGrade = hRecord.GetFieldAsInt32("table_grade");
			table.iMinIntegral = hRecord.GetFieldAsInt32("min_integral");
			table.iMaxIntegral = hRecord.GetFieldAsInt32("max_integral");
			table.iWeight[0] = hRecord.GetFieldAsInt32("award_weight1");
			table.iWeight[1] = hRecord.GetFieldAsInt32("award_weight2");
			table.iWeight[2] = hRecord.GetFieldAsInt32("award_weight3");
			table.iWeight[3] = hRecord.GetFieldAsInt32("award_weight4");
			table.iWeight[4] = hRecord.GetFieldAsInt32("award_weight5");
			table.iWeight[5] = hRecord.GetFieldAsInt32("award_weight6");

			_log(_DEBUG, "TH", "HandleGetIntegralTaskInfoMsg iGameID[%d] table[%d] min[%d] max[%d] weight[%d][%d][%d][%d][%d][%d]", table.iGameID, table.iTableGrade, table.iMinIntegral, \
				table.iMaxIntegral, table.iWeight[0], table.iWeight[1], table.iWeight[2], table.iWeight[3], table.iWeight[4], table.iWeight[5]);

			vcTaskTable.push_back(table);
		}
	}

	vector<IntegralTaskConfDef> vcTaskPool;

	if (iGetTaskPool == 1)
	{
		CSQLWrapperSelect hSQLSelect1("integral_task_relation", m_pMySqlGame->GetMYSQLHandle());
		hSQLSelect1.AddQueryField("task_id");
		hSQLSelect1.AddQueryConditionNumber("game_id", msgReq->iGameID);
		hSQLSelect1.AddQueryConditionNumber("ifstart", 1);

		CMySQLTableResult hMySQLTableResult1;
		if (!m_pMySqlGame->RunSQL(hSQLSelect1.GetFinalSQLString(), hMySQLTableResult1, true))
		{
			_log(_ERROR, "TH", "HandleGetIntegralTaskInfoMsg find iGameID[%d] err", msgReq->iGameID);
			return;
		}

		vector<int> vcTaskID;
		const std::vector<CMySQLTableRecord>& vecRecordList1 = hMySQLTableResult1.GetRecordList();
		for (const CMySQLTableRecord& hRecord : vecRecordList1)
		{
			vcTaskID.push_back(hRecord.GetFieldAsInt32("task_id"));
		}

		for (const int& iTaskID : vcTaskID)
		{
			CSQLWrapperSelect hSQLSelect2("integral_task_conf", m_pMySqlGame->GetMYSQLHandle());
			hSQLSelect2.AddQueryField("task_name");
			hSQLSelect2.AddQueryField("play_type");
			hSQLSelect2.AddQueryField("trigger_weight1");
			hSQLSelect2.AddQueryField("trigger_weight2");
			hSQLSelect2.AddQueryField("trigger_weight3");
			hSQLSelect2.AddQueryField("trigger_weight4");
			hSQLSelect2.AddQueryField("trigger_weight5");
			hSQLSelect2.AddQueryField("trigger_weight6");
			hSQLSelect2.AddQueryField("award_type");
			hSQLSelect2.AddQueryField("award_num1");
			hSQLSelect2.AddQueryField("award_num2");
			hSQLSelect2.AddQueryField("award_num3");
			hSQLSelect2.AddQueryField("award_num4");
			hSQLSelect2.AddQueryField("award_num5");
			hSQLSelect2.AddQueryField("award_num6");
			hSQLSelect2.AddQueryField("task_type");
			hSQLSelect2.AddQueryField("task_value");
			hSQLSelect2.AddQueryConditionNumber("task_id", iTaskID);

			CMySQLTableResult hMySQLTableResult2;
			if (!m_pMySqlGame->RunSQL(hSQLSelect2.GetFinalSQLString(), hMySQLTableResult2, true))
			{
				_log(_ERROR, "TH", "HandleGetIntegralTaskInfoMsg find iGameID[%d] err", msgReq->iGameID);
				return;
			}

			const std::vector<CMySQLTableRecord>& vecRecordList2 = hMySQLTableResult2.GetRecordList();
			for (const CMySQLTableRecord& hRecord : vecRecordList2)
			{
				IntegralTaskConfDef task;

				task.iTaskID = iTaskID;
				task.strTaskName = hRecord.GetFieldAsString("task_name");
				task.iPlayType = hRecord.GetFieldAsInt32("play_type");
				task.iWeight[0] = hRecord.GetFieldAsInt32("trigger_weight1");
				task.iWeight[1] = hRecord.GetFieldAsInt32("trigger_weight2");
				task.iWeight[2] = hRecord.GetFieldAsInt32("trigger_weight3");
				task.iWeight[3] = hRecord.GetFieldAsInt32("trigger_weight4");
				task.iWeight[4] = hRecord.GetFieldAsInt32("trigger_weight5");
				task.iWeight[5] = hRecord.GetFieldAsInt32("trigger_weight6");
				task.iAwardType = hRecord.GetFieldAsInt32("award_type");
				task.iAwardNum[0] = hRecord.GetFieldAsInt32("award_num1");
				task.iAwardNum[1] = hRecord.GetFieldAsInt32("award_num2");
				task.iAwardNum[2] = hRecord.GetFieldAsInt32("award_num3");
				task.iAwardNum[3] = hRecord.GetFieldAsInt32("award_num4");
				task.iAwardNum[4] = hRecord.GetFieldAsInt32("award_num5");
				task.iAwardNum[5] = hRecord.GetFieldAsInt32("award_num6");
				task.iTaskType = hRecord.GetFieldAsInt32("task_type");
				task.iTaskValue = hRecord.GetFieldAsInt32("task_value");

				vcTaskPool.push_back(task);

				//_log(_DEBUG, "TH", "HandleGetIntegralTaskInfoMsg index[%d] iTaskID[%d] type[%d] taskName[%s]", vcTaskTable.size(), task.iTaskID, task.iTaskType, task.strTaskName.c_str());
				//_log(_DEBUG, "TH", "HandleGetIntegralTaskInfoMsg iPlayType[%d] weight[%d][%d][%d][%d][%d][%d]", task.iPlayType, task.iWeight[0], task.iWeight[1], task.iWeight[2], task.iWeight[3], task.iWeight[4], task.iWeight[5]);
				//_log(_DEBUG, "TH", "HandleGetIntegralTaskInfoMsg iAwardType[%d] awardNum[%d][%d][%d][%d][%d][%d] value[%d]", task.iAwardType, task.iAwardNum[0], task.iAwardNum[1], task.iAwardNum[2], task.iAwardNum[3], task.iAwardNum[4], task.iAwardNum[5], task.iTaskValue);

				break;
			}
		}
	}

	int iSendCnt = 0;
	int iSizeInt = sizeof(int);

	while (!vcTaskTable.empty() || !vcTaskPool.empty())
	{		
		GameGetIntegralTaskInfoResDef msgRes;
		memset(&msgRes, 0, sizeof(GameGetIntegralTaskInfoResDef));

		msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgRes.msgHeadInfo.iMsgType = RD_GET_INTEGRAL_TASK_CONF;
		msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;		

		char szBuffer[4096] = { 0 };
		memcpy(szBuffer, &msgRes, sizeof(MsgHeadDef));

		int iMsgLen = sizeof(MsgHeadDef);

		int* iIfLastMsg = (int*)(szBuffer + iMsgLen);
		iMsgLen += iSizeInt;
		int* iTableSize = (int*)(szBuffer + iMsgLen);
		iMsgLen += iSizeInt;

		(*iIfLastMsg) = 1;
		
		while (!vcTaskTable.empty())
		{
			(*iTableSize)++;

			memcpy(szBuffer + iMsgLen, &(vcTaskTable.at(0).iTableGrade), iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, &(vcTaskTable.at(0).iMinIntegral), iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, &(vcTaskTable.at(0).iMaxIntegral), iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, vcTaskTable.at(0).iWeight, 6 * iSizeInt);
			iMsgLen += 6 * iSizeInt;

			vcTaskTable.erase(vcTaskTable.begin());
		}

		int* iTaskInfoSize = (int*)(szBuffer + iMsgLen);

		iMsgLen += iSizeInt;

		while (!vcTaskPool.empty())
		{
			if (iMsgLen > 4000)
			{
				(*iIfLastMsg) = 0;		//还有内容没发完 等待下次补发
				break;
			}

			(*iTaskInfoSize)++;

			memcpy(szBuffer + iMsgLen, &(vcTaskPool.at(0).iTaskID), iSizeInt);
			iMsgLen += iSizeInt;

			int iTaskNameCHLen = vcTaskPool.at(0).strTaskName.size();
			memcpy(szBuffer + iMsgLen, &iTaskNameCHLen, iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, vcTaskPool.at(0).strTaskName.c_str(), iTaskNameCHLen);
			iMsgLen += iTaskNameCHLen;


			memcpy(szBuffer + iMsgLen, &(vcTaskPool.at(0).iPlayType), iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, vcTaskPool.at(0).iWeight, 6 * iSizeInt);
			iMsgLen += 6 * iSizeInt;
			memcpy(szBuffer + iMsgLen, &(vcTaskPool.at(0).iAwardType), iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, vcTaskPool.at(0).iAwardNum, 6 * iSizeInt);
			iMsgLen += 6 * iSizeInt;
			memcpy(szBuffer + iMsgLen, &(vcTaskPool.at(0).iTaskType), iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, &(vcTaskPool.at(0).iTaskValue), iSizeInt);
			iMsgLen += iSizeInt;

			IntegralTaskConfDef* pTask = &vcTaskPool.at(0);
			_log(_ALL, "TH", "GetIntegralTask id[%d]type[%d]award[%d]value[%d]", pTask->iTaskID, pTask->iTaskType, pTask->iAwardType, pTask->iTaskValue);
			vcTaskPool.erase(vcTaskPool.begin());
		}

		_log(_ERROR, "TH", "HandleGetIntegralTaskInfoMsg iGameID[%d] iSendCnt[%d] iIfLastMsg[%d] iSize[%d][%d] iMsgLen[%d]", msgReq->iGameID, iSendCnt, *iIfLastMsg, *iTableSize, *iTaskInfoSize, iMsgLen);

		iSendCnt++;

		if (m_pSendQueue)
		{
			m_pSendQueue->EnQueue(szBuffer, iMsgLen);
		}
	}
}

void TaskHandler::HandleSetIntegralTaskHisMsg(char *msgData, int iLen, int iSocketIndex)
{
	RdGetIntegralTaskMsgDef* pMsgReq = (RdGetIntegralTaskMsgDef*)msgData;
	char strKey[128];
	sprintf(strKey, KEY_INTEGRAL_TASK_HIS.c_str(), pMsgReq->iGameID, pMsgReq->iUserID);
	string str = RedisHelper::get(m_redisCt, strKey);
	vector<int> vcRt;
	if (str.length() > 0)
	{
		Util::CutIntString(vcRt, str, "_");
	}
	int* pTaskID = (int*)(pMsgReq + 1);
	for (int i = 0; i < pMsgReq->iRecentNum; i++)
	{
		vcRt.push_back(*pTaskID);
		pTaskID++;
	}
	string strRt = "";
	char cTmp[128] = { 0 };
	int iBegin = vcRt.size() - 3;
	iBegin = iBegin > 0 ? iBegin : 0;
	for (int i = iBegin; i < vcRt.size(); i++)
	{
		sprintf(cTmp, "%d_", vcRt[i]);
		strRt += cTmp;
	}
	RedisHelper::set(m_redisCt, strKey, strRt);
	RedisHelper::expire(m_redisCt, strKey, 7 * 24 * 3600);
}

void TaskHandler::HandleRefreshUserDayInfo(char *msgData, int iLen, int iSocketIndex)
{
	RdUserDayInfoMsgReqDef* pMsgReq = (RdUserDayInfoMsgReqDef*)msgData;
	char cKey[128] = { 0 };
	sprintf(cKey, KEY_USER_DAY_INFO.c_str(), pMsgReq->iUserID);
	bool bExist = RedisHelper::exists(m_redisCt, cKey);

	_log(_DEBUG, "TH", "HandleRefreshUserDayInfo user[%d] addIntegral[%d] addGameCnt[%d]", pMsgReq->iUserID, pMsgReq->iAddTaskIntegral, pMsgReq->iAddGameCnt);
	RedisHelper::hincrby(m_redisCt, cKey, "day_task_get_integral", pMsgReq->iAddTaskIntegral);
	RedisHelper::hincrby(m_redisCt, cKey, "day_play_game_num", pMsgReq->iAddGameCnt);

	time_t iTmNow = time(NULL);
	tm *temptm;
	temptm = localtime(&iTmNow);
	temptm->tm_hour = 0;
	temptm->tm_min = 0;
	temptm->tm_sec = 0;
	time_t iNextDay = mktime(temptm) + 3600 * 24 - 1;  //今日23:59:59
	RedisHelper::expire(m_redisCt, cKey, iNextDay - iTmNow);

}

void TaskHandler::HandleGetLastMonthsVacNum(char * msgData, int iLen, int iSocketIndex)
{
	GetLastMonthsVacReqDef * msgReq = (GetLastMonthsVacReqDef*)msgData;
	_log(_DEBUG,"TaskHandler","HandleGetLastMonthsVacNum Node[%d],iType[%d],iRechargeRatio[%d]",msgReq->iUserID,msgReq->iType);
	char strKey[64] = { 0 };
	sprintf(strKey, KEY_USER_RECENT_VAC.c_str(), msgReq->iUserID);
	GetLastMonthsVacResDef msgRes;
	memset(&msgRes, 0, sizeof(GetLastMonthsVacResDef));
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = RD_SET_USER_LAST_MONTHS_VAC;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	msgRes.iUserID = msgReq->iUserID;
	if (RedisHelper::exists(m_redisCt, strKey) == false)
	{
		if (m_pSendQueue)
		{
			m_pSendQueue->EnQueue((char*)&msgRes, sizeof(GetLastMonthsVacResDef));
		}
		return;
	}
	int iAllInfullNum = 0;//总金额
	int iAllInfullCount = 0;//总次数
	vector<LastMonthRecharge> vcResult = GetLastMonthsVac(m_redisCt, msgReq->iUserID, msgReq->iType,msgReq->iFlag, iAllInfullNum, iAllInfullCount);
	if (vcResult.size() == 0)
	{
		return;
	}
	//对vcResult做一次数据处理，其内保存的都是原生数据
	char szBuffer[1024];
	memcpy(szBuffer, &msgRes, sizeof(GetLastMonthsVacResDef));
	int iMsgLen = sizeof(GetLastMonthsVacResDef);
	msgRes.iAllVacCnt = iAllInfullCount;
	msgRes.iAllVacNum = iAllInfullNum;
	int iNeedDetail = msgReq->iFlag & 0x1;
	if (iNeedDetail == 1)
	{
		msgRes.iTotalCnt = vcResult.size();
		int iSizeInt = sizeof(int);
		for (int i = 0; i < vcResult.size(); i++)
		{
			if (iMsgLen + 3 * iSizeInt >= 1024)
			{
				//越界不存入
				msgRes.iTotalCnt -= 1;
				continue;
			}
			int iIndex = vcResult[i].iVacIndex;
			int iTimeFlag = vcResult[i].iTimeFlag;
			int iNum = vcResult[i].iVacValue;

			//数据拼接
			memcpy(szBuffer + iMsgLen, &iIndex, iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, &iTimeFlag, iSizeInt);
			iMsgLen += iSizeInt;
			memcpy(szBuffer + iMsgLen, &iNum, iSizeInt);
			iMsgLen += iSizeInt;
		}
	}
	memcpy(szBuffer, &msgRes, sizeof(GetLastMonthsVacResDef));
	if (m_pSendQueue)
	{
		m_pSendQueue->EnQueue(szBuffer, iMsgLen);
	}
}

vector<LastMonthRecharge> TaskHandler::GetLastMonthsVac(redisContext * context, int iUserID, int iType, int iFlag, int& iAllInfullNum, int& iAllInfullCount)
{
	iAllInfullNum = 0;
	iAllInfullCount = 0;
	time_t tmNow = time(NULL);
	struct tm * dtNow = localtime(&tmNow);
	int iDateFlag = (dtNow->tm_year + 1900) * 10000 + (dtNow->tm_mon + 1) * 100 + dtNow->tm_mday;
	string strFeildDate[] = { "1m_all_vac_date","3m_all_vac_date" };
	string strFeildVacNum[] = { "1m_all_vac","3m_all_vac" };
	string strFeildVacCount[] = { "1m_all_vac_cnt","3m_all_vac_cnt" };
	int iDayGap[] = { 29,89 };
	int iJudgeType = iType;
	if (iJudgeType >= 2 || iJudgeType < 0)
	{
		iJudgeType = 1;
	}
	char strKey[64] = { 0 };
	sprintf(strKey, "RUS_USER_CHARGE_INFO:%d", iUserID);
	string strLastDate = RedisHelper::hget(context, strKey, strFeildDate[iJudgeType]);
	bool bNeedFresh = true;
	if (strLastDate != "0" && strLastDate.length() > 0)
	{
		char cTemp[16] = { 0 };
		sprintf(cTemp, "%d", iDateFlag);
		if (strLastDate == cTemp)
		{
			bNeedFresh = false;
			string strObj = RedisHelper::hget(context, strKey, strFeildVacNum[iJudgeType]);
			iAllInfullNum = atoi(strObj.c_str());
			strObj = RedisHelper::hget(context, strKey, strFeildVacCount[iJudgeType]);
			iAllInfullCount = atoi(strObj.c_str());
		}
	}
	if (bNeedFresh == true)
	{
		int iMinIndex = 1;
		int iMaxIndex = 1;
		string strObj = RedisHelper::hget(context, strKey, "vac_min_index");
		if (strObj.length() > 0 && strObj != "0")
		{
			iMinIndex = atoi(strObj.c_str());
		}
		strObj = RedisHelper::hget(context, strKey, "vac_max_index");
		if (strObj.length() > 0 && strObj != "0")
		{
			iMaxIndex = atoi(strObj.c_str());
		}
		char cFlag[64] = { 0 };
		sprintf(cFlag, "%d", iDateFlag);
		RedisHelper::hset(context, strKey, strFeildDate[iJudgeType], cFlag);
		time_t tmTempFlag = tmNow - 86400 * iDayGap[iJudgeType];
		struct tm * dtTemp = localtime(&tmTempFlag);
		int iTempFlag = (dtTemp->tm_year + 1900) * 10000 + (dtTemp->tm_mon + 1) * 100 + dtTemp->tm_mday;
		time_t tmTempFlag1 = tmNow - 86400 * iDayGap[1];
		struct tm *  dtTemp1 = localtime(&tmTempFlag1);
		int iTempFlag1 = (dtTemp1->tm_year + 1900) * 10000 + (dtTemp1->tm_mon + 1) * 100 + dtTemp1->tm_mday;
		for (int j = iMinIndex; j < iMaxIndex + 1; j++)
		{
			char strTempField[16] = { 0 };
			sprintf(strTempField, "vac_%d", j);
			string strObjTemp = RedisHelper::hget(context, strKey, strTempField);
			if (strObjTemp == "0" || strObjTemp.length() == 0)
			{
				continue;
			}
			vector<string> strArr;
			Util::CutString(strArr, strObjTemp, "_");
			if (strArr.size() != 3)
			{
				continue;
			}
			int iTimeFlag = atoi(strArr[0].c_str());
			int iCount = atoi(strArr[1].c_str());
			int iNum = atoi(strArr[2].c_str());
			if (iTimeFlag < iTempFlag)
			{
				if (iTimeFlag < iTempFlag1)
				{
					RedisHelper::hdel(context, strKey, strTempField);
					iMinIndex = j + 1;
				}
				continue;
			}
			iAllInfullCount += iCount;
			iAllInfullNum += iNum;
		}
		memset(cFlag, 0, sizeof(cFlag));
		sprintf(cFlag, "%d", iMinIndex);
		RedisHelper::hset(context, strKey, "vac_min_index", cFlag);
		memset(cFlag, 0, sizeof(cFlag));
		sprintf(cFlag, "%d", iDateFlag);
		RedisHelper::hset(context, strKey, strFeildDate[iJudgeType], cFlag);
		memset(cFlag, 0, sizeof(cFlag));
		sprintf(cFlag, "%d", iAllInfullNum);
		RedisHelper::hset(context, strKey, strFeildVacNum[iJudgeType], cFlag);
		memset(cFlag, 0, sizeof(cFlag));
		sprintf(cFlag, "%d", iAllInfullCount);
		RedisHelper::hset(context, strKey, strFeildVacCount[iJudgeType], cFlag);
	}

	vector <LastMonthRecharge> vcRes;
	int iMinIndex = 0;
	int iMaxIndex = 0;

	string strObj = RedisHelper::hget(context, strKey, "vac_min_index");
	if (strObj.length() > 0 && strObj != "0")
	{
		iMinIndex = atoi(strObj.c_str());
	}

	strObj = RedisHelper::hget(context, strKey, "vac_max_index");
	if (strObj.length() > 0 && strObj != "0")
	{
		iMaxIndex = atoi(strObj.c_str());
	}
	int iNeedDetail = iFlag & 0x1;
	if ((iType == 0 || iType == 1) && iNeedDetail == 0)
	{
		//只获取近1个月或近3个月且不需要获取每天充值详情，就不需要继续了
		return vcRes;
	}
	int iTimeGap = 0;
	iTimeGap = abs(iType) - 1;
	time_t tmTempFlag = tmNow - 86400 * iTimeGap;//获取指定n天的充值
	struct tm * dtTemp = localtime(&tmTempFlag);
	int iTempFlag = (dtTemp->tm_year + 1900) * 10000 + (dtTemp->tm_mon + 1) * 100 + dtTemp->tm_mday;
	//刷新结束开始获取最新的所有充值记录
	for (int j = iMinIndex; j < iMaxIndex + 1; j++)
	{
		//获取需要的单条记录
		char strSingleRecord[32] = { 0 };
		char strFieldVac[16] = { 0 };
		sprintf(strFieldVac, "vac_%d", j);
		string strValueVac = RedisHelper::hget(context, strKey, strFieldVac);
		if (strValueVac == "0" || strValueVac.length() == 0)
		{
			continue;
		}
		vector<string> strArrVac;
		Util::CutString(strArrVac, strValueVac, "_");
		if (strArrVac.size() != 3)
		{
			continue;
		}
		int iTimeFlag = atoi(strArrVac[0].c_str());
		int iCount = atoi(strArrVac[1].c_str());
		int iNum = atoi(strArrVac[2].c_str());
	
		if (iTimeFlag < iTempFlag)
		{
			continue;
		}
		if (iType != 0 && iType  != 1)
		{
			iAllInfullCount += iCount;
			iAllInfullNum += iNum;
		}
		LastMonthRecharge Tmp;
		Tmp.iVacIndex = j;
		Tmp.iTimeFlag = iTimeFlag;
		Tmp.iVacValue = iNum;
		vcRes.push_back(Tmp);
	}

	return vcRes;
}

void TaskHandler::ReportTaskCommonEvent(vector<CommonEvent> vcCommonEvent)
{
	if (vcCommonEvent.empty())
	{
		return;
	}
	
	char szBuffer[2048];
	memset(&szBuffer, 0, sizeof(szBuffer));

	SRedisCommonEventMsgDef* comEvent = (SRedisCommonEventMsgDef*)szBuffer;
	comEvent->msgHeadInfo.iMsgType = RD_REPORT_COMMON_EVENT_MSG;
	comEvent->msgHeadInfo.cVersion = MESSAGE_VERSION;

	int iMsgLen = sizeof(SRedisCommonEventMsgDef);

	for (int i = 0; i < vcCommonEvent.size(); i++)
	{
		comEvent->iEventItemNum++;

		memcpy(szBuffer+iMsgLen, &vcCommonEvent.at(i), sizeof(CommonEvent));

		iMsgLen += sizeof(CommonEvent);

		//一次最多发送40个事件
		if (comEvent->iEventItemNum >= 40)
		{
			break;
		}
	}

	_log(_DEBUG, "TH", "ReportTaskCommonEvent size[%d]", vcCommonEvent.size());

	if (m_pEventGetQueue)
	{
		int iRes = m_pEventGetQueue->EnQueue(szBuffer, iMsgLen);
		if (iRes != 0)
		{
			_log(_ERROR, "EnQueue_log", "ReportTaskCommonEvent iEventItemNum[%d]", comEvent->iEventItemNum);
		}
	}
}

