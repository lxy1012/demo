#ifndef _TCP_SOCK_H_
#define _TCP_SOCK_H_

#ifdef _WIN32
#include "Net.h"
#include "winsock.h"
#include "Proxy.h"
#else
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "emutex.h"
#endif

typedef enum TcpSocketType
{
	UNKOWN_SOCKET	 = 0,			//初始化类型
	SERVER_SOCKET,					//服务器监听节点类型
	ACCEPT_SOCKET,					//服务器接受到客户端连接与之建立的节点类型
	CLIENT_SOCKET,					//客户端连接服务器与子建立的节点类型
}TcpSocketTypeDef;

typedef struct TcpSocketNode
{
	TcpSocketTypeDef	iSocketType;	//节点类型
	int				iHeartBeatTime;				//设置的心跳时间
	int				iServerPort;					//如果为TCP_SERVER_SOCKET的话，记录自己的PORT
	char			szPeerIP[20];					//链接对象的IP
	unsigned short	iPeerPort;			//链接对象的端口
	int				socket;								//socket标识符
	time_t 			tmOutTime;					//心跳检验时间
	int				iWaitOutTime;					//释放节点后必须等待WaitOutTime秒后才能被再次使用，因为要考虑到SEND_QUEUE的问题，
	bool			bKill;								//是否需要释放节点，多线程的时候不能直接调用DelSocketNode，只能通过这个标记位来释放
	int				iTableID;							//桌ID，用于一桌群发时候
	
	char			*szReadBuffer;				//接收缓存，用于沾包情况
	int				iReadPos;							//接收缓存的当前使用大小
	char			*szWriteBuffer;				//发送缓存，用于底层缓存不够的情况导致一个包无法发送完整，以及多线程发送模式适用
	int				iWritePos;						//发送缓存的当前使用大小
	
#ifdef _WIN32
	bool			bProxy;
	Proxy			proxy;
#endif
	
	int				iUserID;
	char			szUserName[16];
	int				iFlag1;
	int				iFlag2;
	int				iFlag3;
	int				iFlag4;
	
	time_t		tmCreateTime;
	int				iRecPackNum;
	time_t		tmLastRecPackTime;
	int		    iNoAesFlag;//是否不需要aes加密解密
	int			iRepeatSendFailCnt;//重复发送沾包次数
	long long iNextSendTm;//因缓冲区满下次等待发送包的时间
}TcpSocketNodeDef;

class TcpSock
{
public:
	TcpSock(int iAllNodeNum,int iWriteMode = WRITE_MUL_THREAD_MODE,int iReadBufferLen = 8192,int iWriteBufferLen = 8192);
	~TcpSock();
	
	
	enum{WRITE_SINGLE_THREAD_MODE = 0,WRITE_MUL_THREAD_MODE};//发送包模式（单线程模式，多线程模式）
	
	int 				AddTCPServerNode(int iServerPort, int iHeartBeatTime = 0);
	int 				AddTCPClientNode(char *szServerIP, int iServerPort,int iHeartBeatTime = 0);
	
	enum{AES_PACKET = 0,NO_AES_PACKET};
	virtual int	WriteSocketNodeData(int iSocketIndex, char *szData, int iLen,int iAesFlag = NO_AES_PACKET);
	virtual int	WriteAllSocketNode(char *szData, int iLen,int iTableID = -1,int iAesFlag = NO_AES_PACKET);//群发函数
	
	void				SetKillFlag(int iSocketIndex,bool bFlag = true);
	
	void				SetNodeTableID(int iSocketIndex,int iTableID);
	enum{NORMAL_DIS = 0,FLAG_DIS,TIMEOUT_DIS};
	
	int GetNowSocketNodeNum(){return m_iNowSocketNodeNum;};
	
	TcpSocketNodeDef* GetSocketNode(int iSocketIndex);
	
	void SetTcpSockName(char *szName){strcpy(m_szTcpSockName,szName);}
	
	void SetSocketNodeWaitTime(int iWaitTime){m_iSocketNodeWaitTime = iWaitTime;}
protected:
	
	int ReadSocketNodeData();
	
	virtual void CallbackAddSocketNode(int iSocketIndex){};

	
	virtual void CallbackDelSocketNode(int iSocketIndex,int iDisType, int iProxy = 0){};

	virtual bool CallbackTCPReadData(int iSocketIndex,char *szMsg,int iLen,int iAesFlag) = 0;
	virtual void CallbackParseDataError(int iSocketIndex,char *szBuffer,int iLen,int iPos){};
#ifdef _WIN32
	virtual void		CallbackProxyConnInfo(int iResult){};
#endif
	
	void 				ResetSocketNodeHeartTime(int iSocketIndex);
	
	TcpSocketNodeDef *m_pSokcetNode;//节点数组
	
	int					m_iMaxNodeIndex;//当前最大可用节点的下标值
	
	int  				AddSocketNode(int socket, TcpSocketTypeDef type,int iHeartBeatTime = 0, bool bCallBackFlag = false,int iPort = 0,sockaddr_in *pAddrPeer = NULL);
	void 				DelSocketNode(int iSocketIndex, bool bCallBackFlag = false,int iDisType = NORMAL_DIS, int iProxy = 0);
	
	int 				SafeWriteNodeBuffer(int iSocketIndex, char *szData, int iLen);//内部调用的安全发包函数，如果发包没有完整的话，会启用发送缓存保存等待下次一起发送
	
	int					m_iNowSocketNodeNum;//当前的总节点数
	
	int					m_iAllNodeNum;	//预定义的最大维护节点数
	
	int					m_iTotalNodeReadBufferLen;
	int					m_iTotalNodeWriteBufferLen;
	
	int		m_iNodeReadBufferLen;
	int		m_iNodeWriteBufferLen;
#ifdef _WIN32
	fd_set 			m_rfds;
	fd_set 			m_wfds;
	struct timeval	m_tv;
#else
	int		m_fdMainEpoll;	//epool的主句柄
	struct epoll_event m_evTmp;	//临时的事件模型
	struct epoll_event *m_pEpollEvent;//事件触发模型
#endif


	char *m_szReadBuffer;
	char *m_szTmpReadBuffer;
	virtual void ParseReadBuffer(int iSocketIndex,char *szBuffer,int iLen);
	
	char *m_szWriteBuffer;
	char *m_szTmpWriteBuffer;
	
#ifdef _WIN32
	HANDLE				m_hWriteMutex;
#else
	EMutex m_lockWriteBuffer;	//发送缓存的保护锁，只对WRITE_MUL_THREAD_MODE模式有意义
#endif

	int 				m_iWirteMode;
	
	char				m_szTcpSockName[32];
	
	int m_iSocketNodeWaitTime;

	long long GetMillisecond();//获取ms时间
};



#endif
