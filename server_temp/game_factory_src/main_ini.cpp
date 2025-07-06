#include "main_ini.h"
//系统头文件
#include <stdio.h>
#include <unistd.h>      //sleep()
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
//64位需要的头文件
#include <sys/stat.h>  
#include <unistd.h>
#include <dirent.h>
#include "TcpSock.h"
#include "GlobalMethod.h"
#include "ClientEpollSockThread.h"
#include "ServerEpollSockThread.h"
#include "ServerLogicThread.h"

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

CMainIni::CMainIni()
{
	m_pQueToClient = NULL;
	m_pQueToAllClient = NULL;

	m_pQueToRoom = NULL;
	m_pQueLogic = NULL;

	m_pClientEpoll = NULL;
	m_pClientAllEpoll = NULL;

	m_pRoomEpoll = NULL;
}

CMainIni::~CMainIni()
{
}

int CMainIni::IniMainLogInfo(ServerBaseInfoDef *pSysConfBaseInfo, char *szServerName, char *szConf, GameLogic *pGL, char *pAppName)
{
	memset(pSysConfBaseInfo, 0, sizeof(ServerBaseInfoDef));

	struct stat buf;
	int ret = stat(pAppName, &buf);
	if (ret == 0)
	{
		pSysConfBaseInfo->iAppMTime = buf.st_mtime;
		printf("get file m_time,mtime:%lld\n", buf.st_mtime);
		struct tm *local_time = localtime(&buf.st_mtime);

		char sTime[100];
		sprintf(sTime, "%04d%02d%02d %02d:%02d:%02d", local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
		printf("sTime:%s\n", sTime);
		//gettime(buf.st_mtime);
	}
	else
	{
		printf("get file m_time,error_id:%d,errno:%s\n", ret, strerror(errno));
	}

	//GetValueStr(pSysConfBaseInfo->strLogFileDirectory, "log_file_directory", "server.conf", "System Base Info", "");
	GetValueInt(&(pSysConfBaseInfo->iServerPort), "server_port", "server.conf", "System Base Info", "");
	GetValueInt(&(pSysConfBaseInfo->iHeartBeatTime), "heart_beat_time", "server.conf", "System Base Info", "");
	GetValueInt(&(pSysConfBaseInfo->iServerID), "server_id", "server.conf", "System Base Info", "");

	strcpy(pSysConfBaseInfo->szConfFile, szConf);

	DIR *dir = opendir("../log");
	if (!dir)
		mkdir("../log", 0700);

	LogConfDef conf;
	memset(&conf, 0, sizeof(conf));
	conf.flag = _LOG_CONSOLE | _LOG_FILE | _LOG_FILE_DAILY;
	conf.format = LOG_DATE | LOG_TIME | LOG_LEVEL | LOG_MODULE | LOG_MESSAGE | LOG_HEX;
	conf.level = _DEBUG;
	sprintf(conf.log_file_name, "../log/%s", szServerName);
	init_log_conf(conf);

	_note("[System Base Info Version 2.0.0.0]");
	//_note("log_file_directory: [%s]", pSysConfBaseInfo->strLogFileDirectory);
	_note("server_port: [%d]", pSysConfBaseInfo->iServerPort);
	_note("heart_beat_time: [%d]", pSysConfBaseInfo->iHeartBeatTime);
	_note("server_id: [%d]", pSysConfBaseInfo->iServerID);
}

int CMainIni::IniMainInfo(ServerBaseInfoDef *pSysConfBaseInfo, char *szServerName, char *szConf, GameLogic *pGL, bool bEpoll)
{
	//应用文件修改时间
	string strAppName = ReadLink();
	struct stat buf;
	int ret = stat(strAppName.c_str(), &buf);
	if (ret == 0)
	{
		pSysConfBaseInfo->iAppMTime = buf.st_mtime;
		printf("get file[%s] m_time,mtime:%lld\n", strAppName.c_str(), buf.st_mtime);
	}
	else
	{
		printf("get file[%s] m_time,error_id:%d,errno:%s\n", strAppName.c_str(), ret, strerror(errno));
	}
	pSysConfBaseInfo->iStartTimeStamp = time(NULL);

	int iMsgQueueLen = 2048;

	GetValueInt(&iMsgQueueLen, "main_msg_queue_len", "server.conf", "System Base Info", "8000");

	pGL->m_pServerBaseInfo = pSysConfBaseInfo;
	m_pGL = pGL;

	int iClientSockNum = 0;
	int iClientAllSockNum = 0;
	GetValueInt(&iMsgQueueLen, "client_msg_queue_len", "server.conf", "System Base Info", "7000");
	GetValueInt(&iClientSockNum, "client_main_sock_num", "server.conf", "System Base Info", "1700");
	GetValueInt(&iClientAllSockNum, "client_all_sock_num", "server.conf", "System Base Info", "1700");

	//各队列分配空间
	m_pQueToClient = new MsgQueue(iMsgQueueLen, 1024 * 7 + 512);//向客户端发送发送消息队列
	m_pQueToClient->SetQueName("QueToClient");

	m_pClientEpoll = new ClientEpollSockThread(iClientSockNum, TcpSock::WRITE_SINGLE_THREAD_MODE, 1024 * 40 + 128, 1024 * 80 + 128);
	m_pClientAllEpoll = new ClientEpollSockThread(iClientAllSockNum, TcpSock::WRITE_SINGLE_THREAD_MODE, 1024 * 40 + 128, 1024 * 80 + 128, 1);
	GetValueInt(&iMsgQueueLen, "client_all_msg_queue_len", "server.conf", "System Base Info", "5000");
	m_pQueToAllClient = new MsgQueue(iMsgQueueLen, 1024 * 7 + 512);
	m_pQueToAllClient->SetQueName("QueToAllClient");


	GetValueInt(&iMsgQueueLen, "logic_msg_queue_len", "server.conf", "System Base Info", "8000");
	m_pQueLogic = new MsgQueue(iMsgQueueLen, 1024 * 7 + 512);
	m_pQueLogic->SetQueName("QueLogic");

	m_pClientEpoll->Ini(m_pQueLogic, m_pQueToClient, pSysConfBaseInfo->iServerPort, pSysConfBaseInfo->iHeartBeatTime);
	m_pClientEpoll->SetTcpSockName("client");
	m_pClientEpoll->Start();

	m_pClientAllEpoll->Ini(m_pQueLogic, m_pQueToAllClient, pSysConfBaseInfo->iServerPort + 1, pSysConfBaseInfo->iHeartBeatTime);
	m_pClientAllEpoll->SetTcpSockName("clientAll");
	m_pClientAllEpoll->Start();

	ServerLogicThread* pLogicThread = new ServerLogicThread();
	pLogicThread->Initialize(m_pGL, m_pQueLogic);

	//启动后只连接room服务器，获取游戏服务器所有需要连接的底层服务器信息
	GetValueInt(&iMsgQueueLen, "room_msg_queue_len", "server.conf", "System Base Info", "1000");
	m_pQueToRoom = new MsgQueue(iMsgQueueLen, 1024 * 7 + 512);
	m_pQueToRoom->SetQueName("QueToRoom");

	int iSugAddNodeNum = 1000;
	m_pRoomEpoll = new ServerEpollSockThread(iSugAddNodeNum, BASE_SEVER_TYPE_ROOM, TcpSock::WRITE_SINGLE_THREAD_MODE, 1020 * 10, 1024 * 10);
	m_pRoomEpoll->Ini(m_pQueLogic, m_pQueToRoom, "room.conf");

	m_pGL->Initialize(this, pSysConfBaseInfo);

	pLogicThread->Start();
	m_pRoomEpoll->Start();

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

	return 1;
}

void CMainIni::Release()
{
	//停止线程
	m_pClientEpoll->Stop();
	m_pClientAllEpoll->Stop();

	m_pRoomEpoll->Stop();

	//释放空间
	delete m_pClientEpoll;
	delete m_pClientAllEpoll;
	delete m_pRoomEpoll;


	delete m_pQueToClient;
	delete m_pQueToAllClient;
	delete m_pQueToRoom;
	delete m_pQueLogic;

	release_log_conf(); //释放log结构体
}