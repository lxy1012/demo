
#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

//系统头文件
#include <stdio.h>

//公共模块头文件
#include "thread.h"
#include "msgqueue.h"

#include "factorymessage.h"
#include "gamelogic.h"

class GameLogic;

class ServerLogicThread : public Thread
{
public:
	ServerLogicThread();
	~ServerLogicThread();
	void Initialize(GameLogic *pGL, MsgQueue*  pQueLogic);
private:
	int Run();
	
	MsgQueue *m_pQueLogic;      //Dispatcher的消息对列
	GameLogic *m_pGL;               //游戏逻辑指针
};

#endif //__DISPATCHER_H__
