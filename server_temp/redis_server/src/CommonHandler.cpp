#include "CommonHandler.h"
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
#include "SQLWrapper.h"

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*ÏµÍ³ÅäÖÃ*/
CommonHandler::CommonHandler(MsgQueue *pSendQueue)
{
	m_pSendQueue = pSendQueue;	
	
}

void CommonHandler::HandleMsg(int iMsgType, char* szMsg, int iLen,int iSocketIndex)
{
	if (m_redisCt == NULL)
	{
		_log(_ERROR,"TaskHandler::HandleMsg","error m_redisCt == NULL");
		return;
	}
	//_log(_DEBUG, "TaskHandler::HandleMsg", "iMsgType == %x", iMsgType);
	if (iMsgType == REDIS_COMMON_SET_BY_KEY)
	{
		HandleCommonSetByKey(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_SET_BY_HASH)
	{
		HandleCommonSetByHash(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_GET_BY_KEY)
	{
		HandleCommonGetByKey(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_GET_BY_HASH)
	{
		HandleCommonGetByHash(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_SET_BY_HASH_MAP)
	{
		HandleCommonSetByHashMap(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_GET_BY_HASH_MAP)
	{
		HandleCommonGetByHashMap(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_HASH_DEL)
	{
		HandleCommonHDel(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_SORTED_SET_ZADD)
	{
		HandleSortSetZAdd(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_SORTED_SET_ZINCRBY)
	{
		HandleSortSetZIncrby(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_SORTED_SET_ZRANGEBYSCORE)
	{
		HandleSortSetZRangeByScore(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_HGETALL)
	{
		HandleCommonHGetAll(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_LPUSH)
	{
		HandleListLPush(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_LRANGE)
	{
		HandleListLRange(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_HINCRBY)
	{
		HandleHashIncrby(szMsg, iLen, iSocketIndex);
	}
	else if (iMsgType == REDIS_COMMON_SET_KEY_EXPIRE)
	{
		HandleCommonSetKeyExpireTime(szMsg, iLen, iSocketIndex);
	}	
}

void CommonHandler::CallBackOnTimer(int iTimeNow)
{
	
}



void CommonHandler::HandleCommonSetByKey(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonSetByKeyDef* msgReq = (SRCommonSetByKeyDef*)szMsg;
	string strKey = msgReq->cKey;
	string strValue = msgReq->cValue;
	bool bExist = RedisHelper::exists(m_redisCt,strKey);
	long long llTtl = -1;
	if (bExist)
	{
		llTtl = RedisHelper::ttl(m_redisCt, strKey);
	}
	RedisHelper::set(m_redisCt, strKey, strValue);

	if (bExist == false)
	{
		if (msgReq->iExpire > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, msgReq->iExpire);
		}
	}
	else
	{
		if (llTtl > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, llTtl);
		}
	}

}
void CommonHandler::HandleCommonSetByHash(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonSetByHashDef* msgReq = (SRCommonSetByHashDef*)szMsg;
	string strKey = msgReq->cKey;
	string strFeild = msgReq->cField;
	string strValue = msgReq->cValue;
	//_log(_DEBUG, "HandleCommonSetByHash", "strKey[%s] strFeild[%s] strValue[%s] iExpire[%d]", msgReq->cKey, msgReq->cField,msgReq->cValue, msgReq->iExpire);
	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	long long llTtl = -1;
	if (bExist)
	{
		llTtl = RedisHelper::ttl(m_redisCt, strKey);
	}
	RedisHelper::hset(m_redisCt, strKey, strFeild, strValue);

	if (bExist == false)
	{
		if (msgReq->iExpire > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, msgReq->iExpire);
		}
	}
	else
	{
		if (llTtl > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, llTtl);
		}
	}
}
void CommonHandler::HandleCommonGetByKey(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonGetByKeyReqDef* msgReq = (SRCommonGetByKeyReqDef*)szMsg;
	string strKey = msgReq->cKey;
	string strValue = RedisHelper::get(m_redisCt, strKey);
	SRCommonGetByKeyResDef msgRes;
	memset(&msgRes,0,sizeof(SRCommonGetByKeyResDef));
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = REDIS_COMMON_GET_BY_KEY;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	memcpy(msgRes.cKey, msgReq->cKey, sizeof(msgReq->cKey));
	if (strValue.length() > 0)
	{
		int iLen = strValue.length() > sizeof(msgRes.cValue) ? sizeof(msgRes.cValue) - 1 : strValue.length();
		memcpy(msgRes.cValue, strValue.c_str(), iLen);
	}
	if (m_pSendQueue)
		m_pSendQueue->EnQueue((char*)&msgRes, sizeof(SRCommonGetByKeyResDef));
}
void CommonHandler::HandleCommonGetByHash(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonGetByHashReqDef* msgReq = (SRCommonGetByHashReqDef*)szMsg;
	string strKey = msgReq->cKey;
	string strFeild = msgReq->cField;
	
	string strValue = RedisHelper::hget(m_redisCt, strKey, strFeild);
	SRCommonGetByHashResDef msgRes;
	memset(&msgRes, 0, sizeof(SRCommonGetByHashResDef));
	
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = REDIS_COMMON_GET_BY_HASH;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;

	memcpy(msgRes.cKey, msgReq->cKey, sizeof(msgReq->cKey));
	memcpy(msgRes.cField, msgReq->cField, sizeof(msgReq->cField));
	if (strValue.length() > 0)
	{
		int iLen = strValue.length() > sizeof(msgRes.cValue) ? sizeof(msgRes.cValue) - 1 : strValue.length();
		memcpy(msgRes.cValue, strValue.c_str(), iLen);
	}
	if (m_pSendQueue)
		m_pSendQueue->EnQueue((char*)&msgRes, sizeof(SRCommonGetByHashResDef));
}
void CommonHandler::HandleCommonSetByHashMap(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonSetByHashMapDef* msgReq = (SRCommonSetByHashMapDef*)szMsg;
	
	string strKey = msgReq->cKey;
	
	int iAllFieldNum = msgReq->iAllFeildNum;
	_log(_DEBUG, "CommonHandler", " HandleCommonSetByHashMap strKey = %s iAllFieldNum = %d iLen = %d", strKey.c_str(), iAllFieldNum, iLen);
	map<string, string> mpField;
	for (int i = 0; i < iAllFieldNum; i++)
	{
		char cField[32] = { 0 };
		char cValue[256] = { 0 };
		memcpy(cField, szMsg + sizeof(SRCommonSetByHashMapDef) + (32+64) * i, 32);
		memcpy(cValue, szMsg + sizeof(SRCommonSetByHashMapDef) + (32 + 64) * i+32, 64);
		
		if (strlen(cValue) > 0)
		{
			mpField[cField] = cValue;
			_log(_DEBUG, "CommonHandler", " HandleCommonSetByHashMap feild = %s value = %s", cField, cValue);
		}
	}
		
	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	long long llTtl = -1;
	if (bExist)
	{
		llTtl = RedisHelper::ttl(m_redisCt, strKey);
	}
	RedisHelper::hmset(m_redisCt, strKey, mpField);
	//(redisReply*)redisCommand(m_redisCt, "hmset BK_ARCHIEVE_USER_INFO:590170629 21301 11111 99901 11111");
	if (bExist == false)
	{
		if (msgReq->iExpire > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, msgReq->iExpire);
		}
	}
	else
	{
		if (llTtl > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, llTtl);
		}
	}

}
void CommonHandler::HandleCommonGetByHashMap(char* szMsg, int iLen, int iSocketIndex)
{

	SRCommonGetByHashMapReqDef* msgReq = (SRCommonGetByHashMapReqDef*)szMsg;
	string strKey = msgReq->cKey;
	int iAllFeildNum = msgReq->iAllFeildNum;
	map<string, string> mpField;
	_log(_DEBUG, "CommonHandler", "HandleCommonGetByHashMap strKey = %s ", msgReq->cKey);
	for (int i = 0; i < iAllFeildNum; i++)
	{
		char cFeild[32] = { 0 };
		memcpy(cFeild, szMsg + sizeof(SRCommonGetByHashMapReqDef) + 32*i,32);
		cFeild[31] = '\0';
		mpField[cFeild] = "nil";
		//mpField.insert(make_pair<string,string>(cFeild,""));
		_log(_DEBUG, "CommonHandler", "HandleCommonGetByHashMap key = %s value = %s ", cFeild, mpField[cFeild].c_str());
	}

	SRCommonGetByHashMapResDef msgRes;
	memset(&msgRes, 0, sizeof(SRCommonGetByHashMapResDef));

	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = REDIS_COMMON_GET_BY_HASH_MAP;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;

	memcpy(msgRes.cKey, msgReq->cKey, 64);
	msgRes.iResult = 1;
	int iAllFieldNum = 0;
	bool bSuc = RedisHelper::hmget(m_redisCt, strKey, mpField);
	if (bSuc)
	{
		msgRes.iResult = 0;
		iAllFieldNum = mpField.size();
		msgRes.iAllFeildNum = iAllFieldNum;
		int iNeedLen = sizeof(SRCommonGetByHashMapResDef) + (32 + 64) * iAllFieldNum;
		CharMemManage* pCharMem = GetCharMem(iNeedLen);
		if (pCharMem != NULL)
		{			
			char* cAllMsg = pCharMem->pBuffer;		
			memcpy(cAllMsg, (char *)&msgRes, sizeof(SRCommonGetByHashMapResDef));
			map<string, string>::iterator ite = mpField.begin();
			int i = 0;
			while (ite != mpField.end())
			{
				_log(_DEBUG, "CommonHandler", "HandleCommonGetByHashMap cFeild = %s value = %s", ite->first.c_str(), ite->second.c_str());
				if (ite->second == "nil")
				{
					ite->second = "";
				}
				memcpy(cAllMsg + sizeof(SRCommonGetByHashMapResDef) + (32 + 64)*i, ite->first.c_str(), 32);
				memcpy(cAllMsg + sizeof(SRCommonGetByHashMapResDef) + (32 + 64) * i + 32, ite->second.c_str(), 64);
				ite++;
				i++;
			}
			if (m_pSendQueue)
				m_pSendQueue->EnQueue(cAllMsg, sizeof(SRCommonGetByHashMapResDef) + (32 + 64) * iAllFieldNum);
			//ReleaseCharMem(pCharMem);
			delete[] cAllMsg;
		}
		
	}
	else
	{
		msgRes.iAllFeildNum = iAllFieldNum;
		//_log(_DEBUG, "CommonHandler", "HandleCommonGetByHashMap msgRes.iAllFeildNum  %d", msgRes.iAllFeildNum);	
		if (m_pSendQueue)
			m_pSendQueue->EnQueue((char*)&msgRes, sizeof(SRCommonGetByHashResDef));
	}
}
void CommonHandler::HandleCommonHDel(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonHDelReqDef* msgReq = (SRCommonHDelReqDef*)szMsg;
	string strKey = msgReq->cKey;
	string strFeild = msgReq->cField;

	bool bRet = RedisHelper::hdel(m_redisCt, strKey, strFeild);
	if (bRet == false)
	{
		_log(_ERROR, "CommonHandler", "HandleCommonHDel strKey = %s strFeild = %s", msgReq->cKey, msgReq->cField);
	}
}
void CommonHandler::HandleSortSetZAdd(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonZAddReqDef * msgReq = (SRCommonZAddReqDef*)szMsg;
	string strKey = msgReq->cKey;
	string strFeild = msgReq->cMember;
	
	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	long long llTtl = -1;
	if (bExist)
	{
		llTtl = RedisHelper::ttl(m_redisCt, strKey);
	}
	RedisHelper::zadd(m_redisCt, strKey, msgReq->iScore, strFeild);
	if (bExist == false)
	{
		if (msgReq->iExpire > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, msgReq->iExpire);
		}
	}
	else
	{
		if (llTtl > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, llTtl);
		}
	}
}
void CommonHandler::HandleSortSetZRangeByScore(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonZRangeByScoreReqDef * msgReq = (SRCommonZRangeByScoreReqDef*)szMsg;
	string strKey = msgReq->cKey;
	
	map<string,int> vcInfo = RedisHelper::zrangeByScore(m_redisCt, strKey, msgReq->iMin, msgReq->iMax, msgReq->iOffSet, msgReq->iCount);

	SRCommonZRangeByScoreRes szMsgRes;
	memset(&szMsgRes,0,sizeof(SRCommonZRangeByScoreRes));
	szMsgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	szMsgRes.msgHeadInfo.iMsgType = REDIS_COMMON_SORTED_SET_ZRANGEBYSCORE;
	szMsgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	memcpy(szMsgRes.cKey, msgReq->cKey,sizeof(msgReq->cKey));
	memcpy(szMsgRes.cTemp, msgReq->cTemp, sizeof(msgReq->cTemp));
	szMsgRes.iAllNum = vcInfo.size();
	if (szMsgRes.iAllNum > 0)
	{
		int iNeedLen = sizeof(SRCommonZRangeByScoreRes) + (32) * szMsgRes.iAllNum;
		CharMemManage* pCharMem = GetCharMem(iNeedLen);
		if (pCharMem != NULL)
		{
			char* cAllMsg = pCharMem->pBuffer;
			memcpy(cAllMsg, (char *)&szMsgRes, sizeof(SRCommonZRangeByScoreRes));
			
			map<string, int>::iterator ite = vcInfo.begin();
			int i = 0;
			while (ite != vcInfo.end())
			{
				memcpy(cAllMsg + sizeof(SRCommonZRangeByScoreRes) + (32 + sizeof(int))* i, ite->first.c_str(), 32);
				memcpy(cAllMsg + sizeof(SRCommonZRangeByScoreRes) + (32 + sizeof(int))* i+32, &(ite->second), sizeof(int));
				ite++;
				i++;
			}
			if (m_pSendQueue)
				m_pSendQueue->EnQueue(cAllMsg, sizeof(SRCommonZRangeByScoreRes) + (32 + sizeof(int)) * szMsgRes.iAllNum);
			ReleaseCharMem(pCharMem);
		}
	}
	
}
void CommonHandler::HandleSortSetZIncrby(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonZIncrbyReqDef * msgReq = (SRCommonZIncrbyReqDef*)szMsg;
	string strKey = msgReq->cKey;
	string strFeild = msgReq->cMember;

	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	long long llTtl = -1;
	if (bExist)
	{
		llTtl = RedisHelper::ttl(m_redisCt, strKey);
	}
	string str = RedisHelper::zincrby(m_redisCt, strKey, msgReq->iScore, strFeild);
	if (str.length() > 0 && str != "nil")
	{
		int iScore = atoi(str.c_str());
		SRCommonZIncrbyResDef msgRes;
		memset(&msgRes, 0, sizeof(SRCommonZIncrbyResDef));
		msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
		msgRes.msgHeadInfo.iMsgType = REDIS_COMMON_SORTED_SET_ZINCRBY;
		msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
		memcpy(msgRes.cKey, msgReq->cKey,sizeof(msgReq->cKey));
		memcpy(msgRes.cMember, msgReq->cMember, sizeof(msgReq->cMember));
		memcpy(msgRes.cTemp, msgReq->cTemp, sizeof(msgReq->cTemp));
		msgRes.iScore = iScore;
		if (m_pSendQueue)
			m_pSendQueue->EnQueue((char*)&msgRes, sizeof(SRCommonZIncrbyResDef));
	}
	if (bExist == false)
	{
		if (msgReq->iExpire > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, msgReq->iExpire);
		}
	}
	else
	{
		if (llTtl > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, llTtl);
		}
	}
}
void CommonHandler::HandleCommonHGetAll(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonHGetAllReqDef* msgReq = (SRCommonHGetAllReqDef*)szMsg;
	string strKey = msgReq->cKey;
	
	SRCommonHGetAllResDef msgRes;
	memset(&msgRes, 0, sizeof(SRCommonHGetAllResDef));
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = REDIS_COMMON_HGETALL;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	memcpy(msgRes.cKey, msgReq->cKey, 64);
	memcpy(msgRes.cTemp, msgReq->cTemp, sizeof(msgReq->cTemp));
	int iAllFieldNum = 0;
	map<string, string> mapValue;
	bool bSuc = RedisHelper::hgetAll(m_redisCt, strKey, mapValue);
	if (bSuc)
	{
		iAllFieldNum = mapValue.size();
		msgRes.iAllNum = iAllFieldNum;
		int iNeedLen = sizeof(SRCommonHGetAllResDef) + (32 + 64) * iAllFieldNum;
		CharMemManage* pCharMem = GetCharMem(iNeedLen);
		if (pCharMem != NULL)
		{
			char* cAllMsg = pCharMem->pBuffer;
			memcpy(cAllMsg, (char *)&msgRes, sizeof(SRCommonHGetAllResDef));
			map<string, string>::iterator ite = mapValue.begin();
			int i = 0;
			while (ite != mapValue.end())
			{
				//_log(_DEBUG, "CommonHandler", "HandleCommonHGetAll cFeild = %s value = %s", ite->first.c_str(), ite->second.c_str());
				memcpy(cAllMsg + sizeof(SRCommonHGetAllResDef) + (32 + 64)*i, ite->first.c_str(), 32);
				memcpy(cAllMsg + sizeof(SRCommonHGetAllResDef) + (32 + 64) * i + 32, ite->second.c_str(), 64);
				ite++;
				i++;
			}
			if (m_pSendQueue)
				m_pSendQueue->EnQueue(cAllMsg, sizeof(SRCommonHGetAllResDef) + (32 + 64) * iAllFieldNum);
			ReleaseCharMem(pCharMem);
		}
	}
	else
	{
		msgRes.iAllNum = iAllFieldNum;
		//_log(_DEBUG, "CommonHandler", "HandleCommonHGetAll msgRes.iAllFeildNum  %d", msgRes.iAllNum);
		if (m_pSendQueue)
			m_pSendQueue->EnQueue((char*)&msgRes, sizeof(SRCommonHGetAllResDef));
	}
}
void CommonHandler::HandleListLPush(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonLPushReqDef * msgReq = (SRCommonLPushReqDef*)szMsg;
	string strKey = msgReq->cKey;
	string strValue = msgReq->cValue;

	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	long long llTtl = -1;
	if (bExist)
	{
		llTtl = RedisHelper::ttl(m_redisCt, strKey);
	}
	int iNum = RedisHelper::lpush(m_redisCt, strKey, strValue);
	if (iNum > 0 && msgReq->iLen > 0 && iNum > msgReq->iLen)
	{
		RedisHelper::ltrim(m_redisCt, strKey,0, msgReq->iLen);
	}
	if (bExist == false)
	{
		if (msgReq->iExpire > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, msgReq->iExpire);
		}
	}
	else
	{
		if (llTtl > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, llTtl);
		}
	}
}
void CommonHandler::HandleListLRange(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonLRangeReqDef* msgReq = (SRCommonLRangeReqDef*)szMsg;
	string strKey = msgReq->cKey;

	SRCommonLRangeResDef msgRes;
	memset(&msgRes, 0, sizeof(SRCommonLRangeResDef));
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = REDIS_COMMON_LRANGE;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	memcpy(msgRes.cKey, msgReq->cKey, 64);
	memcpy(msgRes.cTemp, msgReq->cTemp, sizeof(msgReq->cTemp));
	int iAllFieldNum = 0;
	vector<string> vcValue;
	bool bSuc = RedisHelper::lrange(m_redisCt, strKey, msgReq->iStart, msgReq->iEnd, vcValue);
	if (bSuc)
	{
		iAllFieldNum = vcValue.size();
		msgRes.iAllNum = iAllFieldNum;
		int iNeedLen = sizeof(SRCommonLRangeResDef) + 32  * iAllFieldNum;
		CharMemManage* pCharMem = GetCharMem(iNeedLen);
		if (pCharMem != NULL)
		{
			char* cAllMsg = pCharMem->pBuffer;	
			memcpy(cAllMsg, (char *)&msgRes, sizeof(SRCommonLRangeResDef));
			vector<string>::iterator ite = vcValue.begin();
			int i = 0;
			while (ite != vcValue.end())
			{
				//_log(_DEBUG, "CommonHandler", "HandleCommonHGetAll cFeild = %s value = %s", ite->first.c_str(), ite->second.c_str());
				memcpy(cAllMsg + sizeof(SRCommonLRangeResDef) + 32 *i, ite->c_str(), 32);
				ite++;
				i++;
			}
			if (m_pSendQueue)
				m_pSendQueue->EnQueue(cAllMsg, sizeof(SRCommonLRangeResDef) + 32 * iAllFieldNum);
			ReleaseCharMem(pCharMem);
		}

	}
	else
	{
		msgRes.iAllNum = iAllFieldNum;
		//_log(_DEBUG, "CommonHandler", "HandleCommonHGetAll msgRes.iAllFeildNum  %d", msgRes.iAllNum);
		if (m_pSendQueue)
			m_pSendQueue->EnQueue((char*)&msgRes, sizeof(SRCommonLRangeResDef));
	}
}
void CommonHandler::HandleHashIncrby(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonHIncrbyReqDef * msgReq = (SRCommonHIncrbyReqDef*)szMsg;
	string strKey = msgReq->cKey;
	string strFeild = msgReq->cField;

	bool bExist = RedisHelper::exists(m_redisCt, strKey);
	long long llTtl = -1;
	if (bExist)
	{
		llTtl = RedisHelper::ttl(m_redisCt, strKey);
	}
	long long iNum = RedisHelper::hincrby(m_redisCt, strKey, strFeild, msgReq->iNum);
	_log(_DEBUG, "CommonHandler", "HandleHashIncrby strKey = %s strFeild = %s iNum = %d msgReq->iNum = %d", strKey.c_str(), strFeild.c_str(),(int)iNum, msgReq->iNum);
	
	SRCommonHIncrbyResDef msgRes;
	memset(&msgRes, 0, sizeof(SRCommonHIncrbyResDef));
	msgRes.msgHeadInfo.cVersion = MESSAGE_VERSION;
	msgRes.msgHeadInfo.iMsgType = REDIS_COMMON_HINCRBY;
	msgRes.msgHeadInfo.iSocketIndex = iSocketIndex;
	memcpy(msgRes.cKey, msgReq->cKey, sizeof(msgReq->cKey));
	memcpy(msgRes.cMember, msgReq->cField, sizeof(msgReq->cField));
	memcpy(msgRes.cTemp, msgReq->cTemp, sizeof(msgReq->cTemp));
	msgRes.iNum = iNum;
	if (m_pSendQueue)
		m_pSendQueue->EnQueue((char*)&msgRes, sizeof(SRCommonHIncrbyResDef));
	
	if (bExist == false)
	{
		if (msgReq->iExpire > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, msgReq->iExpire);
		}
	}
	else
	{
		if (llTtl > 0)
		{
			RedisHelper::expire(m_redisCt, strKey, llTtl);
		}
	}
}
void CommonHandler::HandleCommonSetKeyExpireTime(char* szMsg, int iLen, int iSocketIndex)
{
	SRCommonSetKeyExpireReqDef * msgReq = (SRCommonSetKeyExpireReqDef*)szMsg;
	string strKey = msgReq->cKey;
	int iExpire = msgReq->iExpire;

	RedisHelper::expire(m_redisCt, strKey,iExpire);
}