#include "CommonThread.h"
#include "conf.h"	
#include "log.h"
#include "main.h"


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

CommonThread::CommonThread(SystemConfigBaseInfo* conf,MsgHandleBase* pMsgHandler, char* szThreadName)
{
	memset(m_szThreadName,0,sizeof(m_szThreadName));
	if (szThreadName != NULL)
	{
		strcpy(m_szThreadName, szThreadName);
	}
	m_SystemConfigBaseInfoDef = conf;
	m_pMsgQueue = NULL;
	m_iTestTime = time(NULL);
	m_pMsgHandler = pMsgHandler;

	m_iDayLeftSecond = Util::GetTodayLeftSeconds();
	_log(_ERROR,"DB", "CommonThread:m_szThreadName[%s,]m_iDayLeftSecond=%d", m_szThreadName,m_iDayLeftSecond);
}

void CommonThread::SetQueue(MsgQueue* msgQueue)
{
	m_pMsgQueue = msgQueue;
}


int CommonThread::Run()
{
	
	char* szDequeueBuffer = new char[m_pMsgQueue->max_msg_len];

	time_t timePrev = 0;
	time_t timeCurr = 0;

	time(&timeCurr);
	struct tm* timenow;
	timenow = localtime(&timeCurr);
	int year = 1900 + timenow->tm_year;
	int month = 1 + timenow->tm_mon;
	int day = timenow->tm_mday;
	int iLastTimeFlag =  year * 10000 + month * 100 + day;

	while (true)
	{
		time(&timeCurr);
		int iTimeGap = timeCurr - timePrev;
		if (iTimeGap> 0)
		{			
			m_iDayLeftSecond--;
			//_log(_DEBUG,"DB", "[%s],m_iDayLeftSecond=%d,iLastTimeFlag[%d]", m_szThreadName,m_iDayLeftSecond,iLastTimeFlag);
			if(m_iDayLeftSecond <= 0)
			{
				struct tm* timenow;
				timenow = localtime(&timeCurr);
			
				int year = 1900 + timenow->tm_year;
				int month = 1 + timenow->tm_mon;
				int day = timenow->tm_mday;
				int iTimeNowFlag = year * 10000 + month * 100 + day;		
				_log(_ERROR,"DB", "22m_iDayLeftSecond=%d,iLastTimeFlag[%d],iTimeNowFlag[%d]",m_iDayLeftSecond,iLastTimeFlag,iTimeNowFlag);
				if(iTimeNowFlag != iLastTimeFlag)
				{
					m_iDayLeftSecond = Util::GetTodayLeftSeconds();
					_log(_ERROR,"DB", "CommonThread:m_iDayLeftSecond=%d,iLastTimeFlag=%d,iTimeNowFlag=%d",m_iDayLeftSecond,iLastTimeFlag,iTimeNowFlag);
					if (m_pMsgHandler != NULL)
					{
						m_pMsgHandler->CallBackNextDay(iLastTimeFlag,iTimeNowFlag);
					}
					iLastTimeFlag = iTimeNowFlag;
				}				
			}
			if (timePrev == 0)
			{
				iTimeGap = 0;
			}
			timePrev = timeCurr;

			if (m_pMsgHandler != NULL)
			{
				m_pMsgHandler->CallBackOnTimer(timeCurr);
				m_pMsgHandler->CallBackOnTimer(timeCurr, iTimeGap);
			}
		}
		int iLen = m_pMsgQueue->DeQueue((void*)szDequeueBuffer, m_pMsgQueue->max_msg_len, 1);
		//_log(_DEBUG, "DB", "[%s],iLen[%d]", m_szThreadName, iLen);
		if (iLen > 0)
		{	
			if (m_pMsgHandler != NULL)
			{
				MsgHeadDef* pHead = (MsgHeadDef*)szDequeueBuffer;
				int iSocketIndex = pHead->iSocketIndex;
				m_pMsgHandler->HandleMsg(pHead->iMsgType, szDequeueBuffer, iLen,iSocketIndex);
			}
		}
	}
	return 1;
}
