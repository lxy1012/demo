//系统头文件
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

//公共库头文件
#include "aeslib.h"		//aes加密
#include "conf.h"		//从文件中读入配置
#include "emutex.h"		//临界资源多线程访问的锁定与释放
#include "log.h"		//写log用
#include "msgqueue.h"	//消息队列


#include "TcpSock.h"
#include "ServerLogicThread.h"
#include "EpollSockThread.h"
#include "main.h"

#include "msg.h"
#include "AssignTableHandler.h"
#include "ClientAuthenHandler.h"

#include "ServerEpollSockThread.h"
#include "CommonThread.h"
//配置文件信息
SystemConfigBaseInfoDef stSysConfigBaseInfo;
char g_szPassword[12] = "asdfasdf";
void HextoStr(const unsigned char* pHex, char* pStr, int iLen);

int DBPwdDeciphering(char *szDBPasswd)
{
	char szHexPasswd[100], szStrPasswd[50];
	memset(szHexPasswd, 0, 100);
	memset(szStrPasswd, 0, 50);
	memcpy(szHexPasswd, szDBPasswd, 100);
	memset(szDBPasswd, 0, 100);
	HextoStr((unsigned char *)szHexPasswd, szStrPasswd, strlen(szHexPasswd));

	int iLen = strlen(szDBPasswd);
	int rtn = aes_dec_r(szStrPasswd, strlen(szStrPasswd), g_szPassword,
		strlen(g_szPassword), szDBPasswd, &iLen);

	return rtn;
}

string ReadLink()
{
	char name[100];
	int rval = readlink("/proc/self/exe", name, sizeof(name) - 1);
	if (rval == -1)
	{
		_log(_ERROR, "Main", "ReadLink error");
	}
	name[rval] = '\0';
	return "./" + string(strrchr(name, '/') + 1);
}
//读取配置文件，并解密密码
int GetConfig()
{
	memset(&stSysConfigBaseInfo, 0, sizeof(SystemConfigBaseInfoDef));
	GetValueInt(&stSysConfigBaseInfo.iHeartTime, "heart_time", "server.conf", "System Base Info", "0");
	GetValueInt(&stSysConfigBaseInfo.iServerPort, "server_port", "server.conf", "System Base Info", "0");
	GetValueInt(&stSysConfigBaseInfo.iServerID, "server_id", "server.conf", "System Base Info", "0");

	return 0;
}

//显示配置
int ShowConfig()
{
	_note("ConfigFileName: %s", "server.conf");
	_note("[System Base Info]");
	_note("server_port: %d", stSysConfigBaseInfo.iServerPort);
	_note("heart_time: %d", stSysConfigBaseInfo.iHeartTime);
	_note("server_id: %d", stSysConfigBaseInfo.iServerID);
	_note("");
	return 0;
}


//主函数
int main(int argc, char *argv[])
{
	sem_t ProcBlock;		//用于将主线程永远阻塞的信号量
	sem_init(&ProcBlock, 0, 0);

	//取得系统配置文件
	if (GetConfig() != 0)
	{
		return -1;
	}
	//显示一下配置文件
	ShowConfig();

	DIR *dir = opendir("../log");
	if (!dir)
	{
		mkdir("../log", 0700);
	}

	//log文件格式控制
	LogConfDef conf;
	memset(&conf, 0, sizeof(conf));
	conf.flag = _LOG_CONSOLE | _LOG_FILE | _LOG_FILE_DAILY;
	conf.format = LOG_DATE | LOG_TIME | LOG_LEVEL | LOG_MODULE | LOG_MESSAGE | LOG_HEX;
	conf.level = _DEBUG;	
	strcpy(conf.log_file_name, "../log/center");
	init_log_conf(conf);

	//写版本号
	_log(_DEBUG, "center version", "v2.02-----20220609");

	struct sigaction act, oact;
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_flags |= SA_RESTART;
	if (sigaction(SIGPIPE, &act, &oact) < 0)
	{
		printf("--------SIGPIPE ingnore error-----------\n");
	}
	else
	{
		printf("SIGPIPE ingnore success-----------\n");
	}
	set_log_level(_DEBUG);//刚启动时默认log等级
	//应用文件修改时间
	string strAppName = ReadLink();
	struct stat buf;
	int ret = stat(strAppName.c_str(), &buf);
	if (ret == 0)
	{
		stSysConfigBaseInfo.iAppMTime = buf.st_mtime;
		printf("get file[%s] m_time,mtime:%lld\n", strAppName.c_str(), buf.st_mtime);
	}
	else
	{
		printf("get file[%s] m_time,error_id:%d,errno:%s\n", strAppName.c_str(), ret, strerror(errno));
	}
	stSysConfigBaseInfo.iStartTimeStamp = time(NULL);
	const int iSugAddNodeNum = 2048;

	printf("test autheReqSize=[%d]\n", sizeof(AuthenReqDef));
	//创建并启动通信线程
	MsgQueue *pSendQueue = new MsgQueue(6000, 1024 * 7 + 512);//给主动连接过来的游戏服务器和玩家客户端发送消息的队列
	pSendQueue->SetQueName("pToGameQueue");

	EpollSockThread *pEpoll = new EpollSockThread(iSugAddNodeNum, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
	pEpoll->Initialize(pSendQueue, stSysConfigBaseInfo.iServerPort, stSysConfigBaseInfo.iHeartTime);

	//客户端登录中心服务器线程
	MsgQueue* pClientGetQueue = new MsgQueue(5000, 1024 * 5 + 256);//从玩家客户端收到的消息队列
	pClientGetQueue->SetQueName("clientGetQue");
	ClientAuthenHandler* pClientHandler = new ClientAuthenHandler(pSendQueue);
	CommonThread* pRdClientAuthenThread = new CommonThread(&stSysConfigBaseInfo, pClientHandler, "clientTrd");
	pRdClientAuthenThread->SetQueue(pClientGetQueue);
	pEpoll->addQueueForMsgType(AUTHEN_REQ_MSG, pClientGetQueue);
	pRdClientAuthenThread->Start();

	//分配桌子处理线程
	MsgQueue* pAssignTableGetQueue = new MsgQueue(5000, 1024 * 5 + 256);//从游戏服务器收到的关于配桌的消息队列
	pAssignTableGetQueue->SetQueName("assignGetQue");

	AssignTableHandler* pAssignTableHandler = new AssignTableHandler(pSendQueue);

	CommonThread* pRdAssignTableThread = new CommonThread(&stSysConfigBaseInfo, pAssignTableHandler, "assignTrd");
	pRdAssignTableThread->SetQueue(pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(NEW_CENTER_GET_USER_SIT_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(NEW_CNNTER_USER_SIT_ERR, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(NEW_CENTER_USER_LEAVE_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(NEW_CENTER_ASSIGN_TABLE_OK_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(NEW_CENTER_FREE_VTABLE_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(NEW_CENTER_TABLE_NUM_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(REDIS_GET_PARAMS_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(RD_GET_VIRTUAL_AI_INFO_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(RD_GET_VIRTUAL_AI_RES_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(RD_USE_CTRL_AI_MSG, pAssignTableGetQueue);
	pEpoll->addQueueForMsgType(NEW_CNNTER_USER_ASSIGN_ERR, pAssignTableGetQueue);
	pRdAssignTableThread->Start();

	MsgQueue* pLogicGetQueue = new MsgQueue(5000, 1024 * 5 + 512);
	pLogicGetQueue->SetQueName("logicGetQue");

	//和room服务器通信
	MsgQueue *pToRoomServerQueue = new MsgQueue(1000, 1024 * 5 + 256);//需要发送到room服务器的消息队列
	pToRoomServerQueue->SetQueName("pToRoomQueue");
	ServerEpollSockThread *pServerEpoll = new ServerEpollSockThread(iSugAddNodeNum, TO_ROOM_SERVER_THREAD, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
	pServerEpoll->Ini(pLogicGetQueue, pToRoomServerQueue, "room.conf");
	pServerEpoll->SetQueueMsg(pClientGetQueue, pAssignTableGetQueue);
	pServerEpoll->Start();

	pEpoll->SetQueueMsg(pClientGetQueue, pAssignTableGetQueue);
	pEpoll->Start();

	//逻辑处理线程
	ServerLogicThread *pLogicThread = new ServerLogicThread(pAssignTableHandler, pLogicGetQueue, pToRoomServerQueue, NULL);
	pEpoll->addQueueForMsgType(GAME_SYS_ONLINE_TO_CENTER_MSG, pLogicGetQueue);
	pEpoll->addQueueForMsgType(GAME_SYS_DETAIL_TO_CENTER_MSG, pLogicGetQueue);
	pEpoll->addQueueForMsgType(GAME_AUTHEN_REQ_RADIUS_MSG, pLogicGetQueue);
	pEpoll->addQueueForMsgType(GAME_ROOM_INFO_REQ_RADIUS_MSG, pLogicGetQueue);
	pEpoll->addQueueForMsgType(SERVER_LOG_LEVEL_SYNC_MSG, pLogicGetQueue);
	pEpoll->addQueueForMsgType(NEW_CENTER_TABLE_NUM_MSG, pLogicGetQueue);
	pEpoll->addQueueForMsgType(RD_GET_VIRTUAL_AI_RES_MSG, pLogicGetQueue);
	pEpoll->addQueueForMsgType(RD_USE_CTRL_AI_MSG, pLogicGetQueue);
	pLogicThread->SetAssignQue(pAssignTableGetQueue);

	pLogicThread->Start();
	_log(_ERROR, "BK_NEW_CENTER_SERVER", "v1.01 server run ok---serverport[%d]", stSysConfigBaseInfo.iServerPort);

	sem_wait(&ProcBlock);		//在此永远阻塞主线程

	pEpoll->Stop();
	delete pEpoll;
	pEpoll = NULL;

	delete pLogicGetQueue;
	//delete pLogicSendQueue;

	release_log_conf();
}

//16进制转字符串
void HextoStr(const unsigned char *pHex, char *pStr, int iLen)
{
	int j = 0;
	char cTemp[200];
	memset(cTemp, 0, 200);
	for (int i = 0; i<iLen; i++)
	{
		if (pHex[i] >= 'a')
		{
			cTemp[i] = pHex[i] - 97 + 10;
		}
		else if (pHex[j] >= '0')
		{
			cTemp[i] = pHex[i] - 48;
		}

		if (i % 2 == 1)
		{
			pStr[i / 2] = cTemp[i - 1] * 16 + cTemp[i];
		}
	}
}