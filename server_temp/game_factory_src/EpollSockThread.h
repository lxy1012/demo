#ifndef _EPOLL_SOCK_THREAD_H_
#define _EPOLL_SOCK_THREAD_H_

#include "TcpSock.h"
#include "thread.h"
#include "msgqueue.h"


class EpollSockThread : public Thread,public TcpSock
{
public:
	EpollSockThread(int iAllNodeNum,int iWriteMode = WRITE_SINGLE_THREAD_MODE,int iReadBufferLen = 8192,int iWriteBufferLen = 8192);
	~EpollSockThread();

	int Initialize(MsgQueue *pGetQueue,MsgQueue *pSendQueue,int iPort,int iHeartTime);

protected:
	virtual void CallbackAddSocketNode(int iSocketIndex);

	virtual void CallbackDelSocketNode(int iSocketIndex,int iDisType, int iProxy);

	virtual bool CallbackTCPReadData(int iSocketIndex,char *szMsg,int iLen,int iAesFlag);

private:
	int 				Run();

	MsgQueue *m_pGetQueue;
	MsgQueue *m_pSendQueue;

};
#endif

