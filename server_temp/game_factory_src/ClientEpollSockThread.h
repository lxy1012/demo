#ifndef _CLIENT_EPOLL_SOCK_THREAD_H_
#define _CLIENT_EPOLL_SOCK_THREAD_H_

#include "TcpSock.h"
#include "thread.h"
#include "msgqueue.h"



class ClientEpollSockThread : public Thread , public TcpSock
{
public:
	ClientEpollSockThread(int iAllNodeNum,int iWriteMode = WRITE_SINGLE_THREAD_MODE,int iReadBufferLen = 8192,int iWriteBufferLen = 8192,int iAllSend = 0);
	~ClientEpollSockThread();

	
	int Ini(MsgQueue *pGetQueue,MsgQueue *pSendQueue,int iServerPort,int iHeartBeatTime);
	
protected:
	
	virtual void CallbackAddSocketNode(int iSocketIndex);

	virtual void CallbackDelSocketNode(int iSocketIndex,int iDisType, int iProxy);

	virtual bool CallbackTCPReadData(int iSocketIndex,char *szMsg,int iLen,int iAesFlag);
	
	virtual void CallbackParseDataError(int iSocketIndex,char *szBuffer,int iLen,int iPos);
	
private:
	int 				Run();
	
	MsgQueue *m_pSendQueue;//要发送给客户端的消息队列
	MsgQueue *m_pGetQueue;//收到的客户端的消息队列
	
	char    m_szDecBuffer[1024 * 6];
	int		m_iDecBufferLen;
	int m_iAllSend;

	char m_szKeepAliveRes[512];
	
};



#endif
