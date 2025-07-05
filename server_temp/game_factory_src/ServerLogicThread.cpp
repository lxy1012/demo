//系统头文件
#include <stdio.h>
#include <unistd.h>      //sleep()
#include <time.h>
#include <string.h>

//公共模块头文件
#include "msgqueue.h"
#include "log.h"
#include "ServerLogicThread.h"

ServerLogicThread::ServerLogicThread()
{
	m_pQueLogic = NULL;
	m_pGL = NULL;
}

ServerLogicThread::~ServerLogicThread()
{
}

void ServerLogicThread::Initialize(GameLogic* pGL, MsgQueue* pQueLogic)
{
	m_pGL = pGL;
	m_pQueLogic = pQueLogic;
}

//这个Run主要负责时间的计算和DeQueue Dispatcher的消息队列
//1、不断DeQueue，有消息情况下调用GL的OnEvent
//2、每次while循环取得一个时间，同上次while取得的时间作比较，有差别的情况下调用GL的OnTimer
int ServerLogicThread::Run()
{
	int iLen = 0;
	time_t timePrev = 0;
	time_t timeCurr = 0;
	time(&timePrev);

	char* szDequeuBuffer = new char[m_pQueLogic->max_msg_len];
	int iDequeueLen = 0;

	while(1)
	{
		time(&timeCurr);
		int iSecGap = timeCurr - timePrev;
		if (iSecGap > 0)   //发生时间事件
		{
			timePrev = timeCurr;
			m_pGL->OnTimer(iSecGap, timeCurr);
		}
		
		iDequeueLen = m_pQueLogic->DeQueue(szDequeuBuffer, m_pQueLogic->max_msg_len, 1);
		if(iDequeueLen > 0)
		{			
			m_pGL->OnEvent((void *)szDequeuBuffer, iDequeueLen);
		}
	}
	return 0;
}
