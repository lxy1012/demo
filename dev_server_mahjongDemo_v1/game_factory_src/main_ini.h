#ifndef __MAIN_INI_H__
#define __MAIN_INI_H__

//系统头文件
#include <stdio.h>
#include <time.h>
#include "gamefactorycomm.h"
#include "msgqueue.h"
#include "log.h"         
#include "conf.h"

class ClientEpollSockThread;
class ServerEpollSockThread;
class CMainIni
{
public:
	CMainIni();
	~CMainIni();
	
	//初始启动时需要的队列，其他根据底层返回的游戏服务需要的链接动态创建
	MsgQueue *m_pQueToClient;  //用于与客户端通信队列
	MsgQueue *m_pQueToAllClient;//用于与客户端群发通信队列

	MsgQueue* m_pQueToRoom;//用于和room通信队列

	MsgQueue* m_pQueLogic;//所有进入logic判断的消息队列

	ClientEpollSockThread *m_pClientEpoll;//客户端通讯线程
	ClientEpollSockThread *m_pClientAllEpoll;//客户群发端通讯线程

	ServerEpollSockThread* m_pRoomEpoll;//用于和room通信线程

	GameLogic* m_pGL;

	int IniMainLogInfo(ServerBaseInfoDef *pSysConfBaseInfo,char *szServerName,char *szConf,GameLogic *pGL,char *pAppName);
	
	int IniMainInfo(ServerBaseInfoDef *pSysConfBaseInfo,char *szServerName,char *szConf,GameLogic *pGL,bool bEpoll = false);
	
	void Release();
	
};

#endif //__MAIN_INI_H__