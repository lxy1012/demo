#ifndef _SERVER_EPOLL_SOCK_THREAD_H_
#define _SERVER_EPOLL_SOCK_THREAD_H_

#include "TcpSock.h"
#include "thread.h"
#include "msgqueue.h"
#include "factorymessage.h"

class GameLogic;
class ServerEpollSockThread :public Thread, public TcpSock
{
public :
	ServerEpollSockThread(int iAllNodeNum, int iThreadType, int iWriteMode = WRITE_SINGLE_THREAD_MODE, int iReadBufferLen = 8192, int iWriteBufferLen = 8192);
	~ServerEpollSockThread();

	int Ini(MsgQueue *pGetQueue, MsgQueue *pSendQueue, char * strFileName);
	int Ini(MsgQueue *pGetQueue, MsgQueue *pSendQueue, char *szServerIP, int iServerPort, char *szServerIP2, int iServerPort2);
	void IniServerBaseInfo(ServerBaseInfoDef serverBaseInfo);
	int GetSocketIndex();

	int GetServerPort(){return m_iServerPort;};
	char *GetServerIP(){return m_szServerIP;};
	//重设对应的底层服务器的ip和端口信息，若和当前不一致，则切换重连
	void ResetSeverIPInfo(char *szServerIP,int iServerPort, char* szServerIP2, int iServerPort2);

	void SetGameLogic(GameLogic* pGL);

	bool m_bServerAuthenOK;

protected:
	virtual void CallbackAddSocketNode(int iSocketIndex);
	virtual void CallbackDelSocketNode(int iSocketIndex,int iDisType,int iProxy);
	virtual bool CallbackTCPReadData(int iSocketIndex,char *szMsg,int iLen,int iAesFlag);

	int m_iThreadType;
	
private :
	int	Run();

	MsgQueue *m_pGetQueue;
	MsgQueue *m_pSendQueue;

	char m_szConfFileName[32];

	char m_szServerIP[20];
	int m_iServerPort;
	char m_szServerIP2[20];
	int m_iServerPort2;

	int m_iConnectIndex;//0主ip连接 1备用ip连接
	bool m_bOnReConnect;//是否正在切换重连

	char m_szPasswd[50];
	char m_szDecBuffer[1024 * 6];
	int m_iDecBufferLen;

	char m_szEncBuffer[1024 * 6];
	int	m_iEncBufferLen;

	int m_iSocketIndex;
	int m_iOldSocketIndex;

	GameLogic* m_pGL;

	void SendServerAuthenMsg();
	int ConnectBaseServer();
	int SendRadiusMsg(void* msgData, int iDataLen);
};

#endif