#include "NewRedisThread.h"
#include "conf.h"	
#include "log.h"
#include "main.h"
#include "hiredis.h"
#include "RedisHelper.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "Util.h"
#include "msg.h"
#include <sys/timeb.h>

extern SystemConfigBaseInfoDef stSysConfigBaseInfo;		/*系统配置*/

NewRedisThread::NewRedisThread(int index, SystemConfigBaseInfo* conf,MsgHandleBase* pMsgHandler, char* szThreadName)
{
	memset(m_szThreadName,0,sizeof(m_szThreadName));
	if (szThreadName != NULL)
	{
		strcpy(m_szThreadName, szThreadName);
	}
	m_index = index;
	m_SystemConfigBaseInfoDef = conf;
	m_pRedisMsgQueue = NULL;
	m_iTestTime = time(NULL);
	m_pMsgHandler = pMsgHandler;

	m_iDayLeftSecond = Util::GetTodayLeftSeconds();
	
	m_bRecordQueueCount = false;
	m_llSecDeQueueCount = 0;
	m_llDayDeQueueCount = 0;

	_log(_ERROR,"DB", "NewRedisThread:m_szThreadName[%s,]m_iDayLeftSecond=%d,m_index=%d", m_szThreadName,m_iDayLeftSecond, m_index);
}

void NewRedisThread::SetRedisQueue(MsgQueue* msgQueue)
{
	m_pRedisMsgQueue = msgQueue;
}

redisContext* NewRedisThread::RedisLogin(int iCtxID, char* szHost, int iPort, char* szPwd,int iDBIndex) /*登陆redis*/
{
	printf("RedisLogin(id:%d,host:%s,port:%d,db:%d)\n", iCtxID, szHost, iPort,iDBIndex);
	printf("RedisLogin(szPwd:%s)\n", szPwd);
	redisContext* c = redisConnect(szHost, iPort);
	//struct timeval timeout = { 120, 0 }; // 设置超时时间
	//redisContext* c = redisConnectWithTimeout(szHost, iPort, timeout);
	//redisContext* c = redisConnectNonBlock(szHost, iPort);
	if (c == NULL)
	{
		printf("Connection error: can't allocate redis context\n");
		return NULL;
	}
	else if (c->err)
	{
		printf("Connection error: %s\n", c->errstr);
		redisFree(c);
		return NULL;
	}

	if (strlen(szPwd) > 0)
	{
		char auth[128] = "auth ";
		if (strlen(szPwd) > 32)
		{
			strncat(auth, szPwd, 32);
		}
		else
		{
			strcat(auth, szPwd);
		}
		redisReply* reply;
		reply = (redisReply*)redisCommand(c, auth);
		if (reply == NULL)
		{
			redisFree(c);
			return NULL;
		}
		if (reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str, "ok") == 0)
		{
			//通过检查
			printf("auth ok %d\n", iDBIndex);
			//选择数据库
			freeReplyObject(reply);
			//reply = (redisReply*)redisCommand(c, "select %d", iDBIndex);
			char szCommand[32];
			sprintf(szCommand, "SELECT %d", iDBIndex);
			printf("auth ok2 szCommand %s\n", szCommand);
			reply = (redisReply*)redisCommand(c, szCommand);
			if (reply)
			{
				freeReplyObject(reply);
			}
			else
			{
				freeReplyObject(reply);
				redisFree(c);
				printf("select db error\n");
				return NULL;
			}
		}
		else
		{
			printf("auth error erInfo:%s\n", reply->str);
			freeReplyObject(reply);
			redisFree(c);
			return NULL;
		}
	}
	else
	{
		printf("don't need pwd auth\n");
	}

	if (c == NULL || c->err != 0)
	{
		return NULL;
	}

	int iKeepAliveRt = redisEnableKeepAlive(c);
	printf("redis KeepAliveRt %d\n", iKeepAliveRt);
	return c;
}

int NewRedisThread::Run()
{
	//链接redis
	redisContext* context = NULL;
	if (m_index == 1)
	{
		context = RedisLogin(m_index, m_SystemConfigBaseInfoDef->szRedisHost, m_SystemConfigBaseInfoDef->iRedisPort, m_SystemConfigBaseInfoDef->szRedisPwd, m_SystemConfigBaseInfoDef->iDBIndex);
	}
	else
	{
		context = RedisLogin(m_index, m_SystemConfigBaseInfoDef->szRedisHost2, m_SystemConfigBaseInfoDef->iRedisPort2, m_SystemConfigBaseInfoDef->szRedisPwd2, m_SystemConfigBaseInfoDef->iDBIndex2);
	}
	if (context == NULL)
	{
		_log(_ERROR, "INIT", "[%s] can't connect redis", m_szThreadName);
		exit(0);
	}
	else
	{
		printf("[%s] connect redis success\n", m_szThreadName);
	}
	int tmp;

	srand(tmp);

	int iRand = rand() % 1000000;

	RedisHelper::test(context, iRand);
	int index = 0;
	int iLen = 0;

	while (m_pRedisMsgQueue == NULL)
	{
		_log(_ERROR, "INIT", "can't connect redis");
		sleep(10);
	}

	if (m_pMsgHandler)
	{
		m_pMsgHandler->SetRedisContext(context);
	}
	char* szDequeueBuffer = new char[m_pRedisMsgQueue->max_msg_len];

	time_t timePrev = 0;
	time_t timeCurr = 0;

	time(&timeCurr);
	struct tm* timenow;
	timenow = localtime(&timeCurr);
	int year = 1900 + timenow->tm_year;
	int month = 1 + timenow->tm_mon;
	int day = timenow->tm_mday;
	int iLastTimeFlag =  year * 10000 + month * 100 + day;

	int iTenMinSeconds = 0;

	while (true)
	{
		time(&timeCurr);
		int iGapTime = timeCurr - timePrev;
		if (iGapTime > 0)
		{
			if (m_llSecDeQueueCount > 0)
			{
				m_llDayDeQueueCount += m_llSecDeQueueCount;	//每秒处理的数量统计到每天里

				_log(_ERROR, "DB", "NewRedisThread timeCurr[%d] m_llSecDeQueueCount[%d] m_llDayDeQueueCount[%d]", timeCurr, m_llSecDeQueueCount, m_llDayDeQueueCount);

				m_llSecDeQueueCount = 0;
			}

			m_iDayLeftSecond -= iGapTime;
			//_log(_DEBUG,"DB", "[%s],m_iDayLeftSecond=%d,iLastTimeFlag[%d]", m_szThreadName,m_iDayLeftSecond,iLastTimeFlag);
			if(m_iDayLeftSecond <= 0)
			{
				struct tm* timenow;
				timenow = localtime(&timeCurr);
			
				int year = 1900 + timenow->tm_year;
				int month = 1 + timenow->tm_mon;
				int day = timenow->tm_mday;
				int iTimeNowFlag = year * 10000 + month * 100 + day;
				//_log(_ERROR,"DB", "22m_iDayLeftSecond=%d,iLastTimeFlag[%d],iTimeNowFlag[%d]",m_iDayLeftSecond,iLastTimeFlag,iTimeNowFlag);
				if(iTimeNowFlag != iLastTimeFlag)
				{
					if (m_llDayDeQueueCount > 0)
					{
						_log(_ERROR, "DB", "NewRedisThread iLastTimeFlag[%d] m_llDayDeQueueCount[%d]", iLastTimeFlag, m_llDayDeQueueCount);

						m_llDayDeQueueCount = 0;	//跨天清掉，从0开始
					}

					m_iDayLeftSecond = Util::GetTodayLeftSeconds();
					_log(_ERROR,"DB", "NewRedisThread:m_iDayLeftSecond=%d,iLastTimeFlag=%d,iTimeNowFlag=%d",m_iDayLeftSecond,iLastTimeFlag,iTimeNowFlag);
					if (m_pMsgHandler != NULL)
					{
						m_pMsgHandler->CallBackNextDay(iLastTimeFlag,iTimeNowFlag);
					}
					iLastTimeFlag = iTimeNowFlag;
				}
			}
			timePrev = timeCurr;

			if (m_pMsgHandler != NULL)
			{
				m_pMsgHandler->CallBackOnTimer(timeCurr);
			}

			iTenMinSeconds += iGapTime;
			if (iTenMinSeconds > 60)	//检查redis连接状态
			{
				iTenMinSeconds = 0;
				int iEnableRt = redisEnableKeepAlive(context);
				if (iEnableRt == -1)
				{
					_log(_ERROR, "DB", "NewRedisThread redisEnableKeepAlive failed thread[%s]", m_szThreadName);
				}
				else
				{
					_log(_ERROR, "DB", "NewRedisThread redisEnableKeepAlive alive thread[%s]", m_szThreadName);
				}
			}
		}
		iLen = m_pRedisMsgQueue->DeQueue((void*)szDequeueBuffer, m_pRedisMsgQueue->max_msg_len, 1);
		//_log(_DEBUG, "DB", "[%s],iLen[%d]", m_szThreadName, iLen);
		if (iLen > 0)
		{	
			MsgHeadDef* pHead = (MsgHeadDef*)szDequeueBuffer;
			//_log(_DEBUG, "DB", "[%s],iLen[%d] type[%d]", m_szThreadName, iLen, pHead->iMsgType);
			if (m_pMsgHandler != NULL)
			{
				/*timeb t;
				ftime(&t);
				long long  tim = t.time * 1000 + t.millitm;
				_log(_ERROR, "NewRedisThread", "run tmTime = %lld cMsgType =%x", tim, pHead->cMsgType);*/
				
				int iSocketIndex = pHead->iSocketIndex;
				m_pMsgHandler->HandleMsg(pHead->iMsgType, szDequeueBuffer, iLen, iSocketIndex);
			}

			if (m_bRecordQueueCount)
			{
				m_llSecDeQueueCount++;
			}
		}
	}

	return 1;
}

void NewRedisThread::SetRecordMsgQueue(bool bRecordQueue)
{
	m_bRecordQueueCount = bRecordQueue;
}