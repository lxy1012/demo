#include "GameEvent.h"
#include "factorymessage.h"
#include "log.h"

GameEvent::GameEvent()
{
	ClearEventMap();
}
void GameEvent::ClearEventMap()
{
	std::map<int, map<int, long long>>::iterator iter;
	for (iter = m_mapGameEvent.begin(); iter != m_mapGameEvent.end(); ++iter)
	{
		map<int, long long>::iterator iSub;
		for (iSub = iter->second.begin(); iSub != iter->second.end(); ++iSub)
		{
			iter->second.erase(iSub);
		}
		m_mapGameEvent.erase(iter);
	}
}

GameEvent::~GameEvent()
{
}

//加入需要上报事件缓存
void GameEvent::AddComEventData(int iEventID, int iSubID, long long iAddNum)
{
	//_log(_DEBUG, "GAMEEVENT", "AddComEventData --->> iEventID[%d] iSubID[%d] iAddNum[%d]", iEventID, iSubID, iAddNum);
	std::map<int, map<int, long long>>::iterator iter = m_mapGameEvent.find(iEventID);
	if (iter != m_mapGameEvent.end())
	{
		map<int, long long>::iterator iSub = iter->second.find(iSubID);
		if (iSub != iter->second.end())
		{
			m_mapGameEvent[iEventID][iSubID] += iAddNum;
		}
		else
		{
			m_mapGameEvent[iEventID][iSubID] = iAddNum;
		}
	}
	else
	{
		map<int, long long> mapSub;
		mapSub[iSubID] = iAddNum;
		m_mapGameEvent[iEventID] = mapSub;
	}
}

//每隔半小时上报一次本地缓存
void GameEvent::SendComStatRedis(GameLogic *pLogic)
{

	SRedisComStatMsgDef dayRecord;

	std::map<int, map<int, long long>>::iterator pos;
	for (pos = m_mapGameEvent.begin(); pos != m_mapGameEvent.end(); ++pos)
	{
		map<int, long long> mapSub = pos->second;
		map<int, long long> ::iterator iSub;
		int iSendNum = 0;
		for (iSub = mapSub.begin(); iSub != mapSub.end(); ++iSub)
		{
			if (iSendNum == 0)
			{
				memset(&dayRecord, 0, sizeof(SRedisComStatMsgDef));
				dayRecord.iEventID = pos->first | (pLogic->m_pServerBaseInfo->iGameID << 16);
				dayRecord.msgHeadInfo.cVersion = MESSAGE_VERSION;
				dayRecord.msgHeadInfo.iMsgType = SEVER_REDIS_COM_STAT_MSG;
			}
			dayRecord.iSubEventID[iSendNum] = iSub->first;
			dayRecord.cllEventAddCnt[iSendNum] = iSub->second;

			iSendNum++;
			if (iSendNum >= 20)
			{
				//攒够20个先上报，剩下的等下一批
				if (pLogic->m_pQueToRedis != nullptr)
				{
					pLogic->m_pQueToRedis->EnQueue(&dayRecord, sizeof(SRedisComStatMsgDef));
				}
				iSendNum = 0;
			}
		}
		if (iSendNum > 0)   //最后不够20的在这里发
		{
			if (pLogic->m_pQueToRedis != nullptr)
			{
				pLogic->m_pQueToRedis->EnQueue(&dayRecord, sizeof(SRedisComStatMsgDef));
			}
		}
	}

	//REDIS_USE_PROP_STAT_MSG
	SUsePropStatMsgDef propStat;
	std::map<int, int>::iterator posProp;
	for (int i = 0; i < 4; i++)
	{
		int iSendNum = 0;
		for (posProp = m_mapUsePropInfo[i].begin(); posProp != m_mapUsePropInfo[i].end(); ++posProp)
		{
			if (iSendNum == 0)
			{
				memset(&propStat, 0, sizeof(SUsePropStatMsgDef));
				propStat.iRoomType = i+5 | (pLogic->m_pServerBaseInfo->iGameID << 4);
				propStat.msgHeadInfo.cVersion = MESSAGE_VERSION;
				propStat.msgHeadInfo.iMsgType = REDIS_USE_PROP_STAT_MSG;
			}
			propStat.iPropID[iSendNum] = posProp->first;
			propStat.iPropNum[iSendNum] = posProp->second;

			iSendNum++;
			if (iSendNum >= 20)
			{
				//攒够20个先上报，剩下的等下一批
				if (pLogic->m_pQueToRedis != nullptr)
				{
					pLogic->m_pQueToRedis->EnQueue(&propStat, sizeof(SUsePropStatMsgDef));
				}
				iSendNum = 0;
			}
		}
		if (iSendNum > 0)   //最后不够20的在这里发
		{
			if (pLogic->m_pQueToRedis != nullptr)
			{
				pLogic->m_pQueToRedis->EnQueue(&propStat, sizeof(SUsePropStatMsgDef));
			}
		}
	}

	for (int i = 0; i < 4; i++)
	{
		map<int, int>().swap(m_mapUsePropInfo[i]);
	}
	ClearEventMap();
}

void GameEvent::AddUesPropData(int iRoomTypeIdx, int iPropID, int iPropNum)
{
	if (iRoomTypeIdx < 4)
	{
		map<int, int>::iterator iter = m_mapUsePropInfo[iRoomTypeIdx].find(iPropID);
		if (iter != m_mapUsePropInfo[iRoomTypeIdx].end())
		{
			m_mapUsePropInfo[iRoomTypeIdx][iPropID] += iPropNum;
		}
		else
		{
			m_mapUsePropInfo[iRoomTypeIdx][iPropID] = iPropNum;
		}
	}
}
