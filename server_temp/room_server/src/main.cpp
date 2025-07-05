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
#include "EpollSockThread.h"
#include "../game_factory_src/ServerLogicThread.h"
#include "main.h"
#include "MySQLConnection.h"
#include "SQLWrapper.h"
#include "AESUtil.hpp"
#include <fstream>
#include "ConfUtil.h"

char g_szPassword[12] = "asdfasdf";

char g_szEncryptionKey[32] = "823^7viSPKuio48@";

//配置文件信息
SystemConfigBaseInfoDef stSysConfigBaseInfo;

//读取配置文件，并解密密码
int GetConfig()
{	
	memset(&stSysConfigBaseInfo, 0, sizeof(SystemConfigBaseInfoDef));

	string fileName = "server.cla";
	struct stat buff;
	if (0 == stat(fileName.c_str(), &buff))
	{
		std::ifstream ifs(fileName.c_str(), std::ios::binary);
		if (!ifs.is_open())
		{
			printf("opne file %s fail\n", fileName.c_str());
			return 0;
		}
		ifs.seekg(0, std::ios::end);
		int len = (int)ifs.tellg();
		ifs.seekg(0, std::ios::beg);
		char* buff = new char[len + 1];
		ifs.read(buff, len);
		ifs.close();
		buff[len] = '\0';
		std::string strSrc(buff);
		delete[] buff;
		printf("strSrc = %s\n", strSrc.c_str());
		std::string strDst = AESUtil::AESDecryptPKCS5CBCBase64(strSrc, g_szEncryptionKey, g_szEncryptionKey);
		printf("strDst = \n%s\n", strDst.c_str());

		ConfUtil::AnalyzeFileContent(fileName, strDst);

		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBUsersHost, "db_host", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueInt(&stSysConfigBaseInfo.iDBUsersPort, "db_port", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBUsers, "db_user", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBUsersPasswd, "db_passwd", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBUsersDataBase, "db_database", (char*)fileName.c_str(), "System Base Info", "");
	}
	else
	{
		GetValueStr(stSysConfigBaseInfo.szDBUsersHost, "db_host", "server.conf", "System Base Info", "");
		GetValueInt(&stSysConfigBaseInfo.iDBUsersPort, "db_port", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBUsers, "db_user", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBUsersPasswd, "db_passwd", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBUsersDataBase, "db_database", "server.conf", "System Base Info", "");
	}

	GetValueInt(&stSysConfigBaseInfo.iHeartTime, "heart_time", "server.conf", "System Base Info", "");
	GetValueInt(&stSysConfigBaseInfo.iServerPort, "server_port", "server.conf", "System Base Info", "");
	GetValueInt(&stSysConfigBaseInfo.iServerID, "server_id", "server.conf", "System Base Info", "");
	GetValueInt(&stSysConfigBaseInfo.iIfLogDBDelay, "if_log_db_delay", "server.conf", "System Base Info", "0");

	//解开密码重新赋值
	//先解UserDBPasswd						
	/*char szHexPasswd[100], szStrPasswd[50];
	memset(szHexPasswd, 0, 100);
	memset(szStrPasswd, 0, 50);
	memcpy(szHexPasswd, stSysConfigBaseInfo.szDBUsersPasswd, 100);
	memset(stSysConfigBaseInfo.szDBUsersPasswd, 0, 100);
	HextoStr((unsigned char *)szHexPasswd, szStrPasswd, strlen(szHexPasswd));

	int iLen = strlen(stSysConfigBaseInfo.szDBUsersPasswd);
	printf("log_pwd=%s,pwd[%s]\n",stSysConfigBaseInfo.szDBUsersPasswd,g_szPassword);
	int rtn = aes_dec_r(szStrPasswd, strlen(szStrPasswd), g_szPassword, strlen(g_szPassword), stSysConfigBaseInfo.szDBUsersPasswd, &iLen);
	if (rtn == -1)
	{
		printf("Cann't open log db password.\n");
		return -1;
	}
	printf("log_pwd_2=%s\n", stSysConfigBaseInfo.szDBUsersPasswd);*/
	return 0;
}

//显示配置
int ShowConfig()
{
	_note("ConfigFileName: %s", "server.conf");
	_note("[System Base Info]");
	_note("server_port: %d", stSysConfigBaseInfo.iServerPort);
	_note("heart_time: %d", stSysConfigBaseInfo.iHeartTime);

	_note("db_game_log: %s", stSysConfigBaseInfo.szDBUsers);
	_note("db_game_log_database: %s", stSysConfigBaseInfo.szDBUsersDataBase);

	_note("if_log_db_delay: %d", stSysConfigBaseInfo.iIfLogDBDelay);
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
	conf.level = _ERROR;
	strcpy(conf.log_file_name, "../log/room");
	init_log_conf(conf);

	//写版本号
	_log(_ERROR, "room version", "v1.01-----20200311");

	struct sigaction act, oact;
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_flags |= SA_RESTART;
	if(sigaction(SIGPIPE, &act, &oact) < 0) 
	{
		printf("--------SIGPIPE ingnore error-----------\n");
	}	
	else
	{
		printf("SIGPIPE ingnore success-----------\n");
	}
	//应用文件修改时间
	string strAppName = ReadLink();
	struct stat buf;
	int ret = stat(strAppName.c_str(), &buf);
	if (ret == 0)
	{
		stSysConfigBaseInfo.iAppMTime = buf.st_mtime;
		printf("get file[%s] m_time,mtime:%lld\n", strAppName.c_str(),buf.st_mtime);
	}
	else
	{
		printf("get file[%s] m_time,error_id:%d,errno:%s\n", strAppName.c_str(),ret, strerror(errno));
	}
	//连接数据库
	MySQLConfig stMySQLConfig;
	stMySQLConfig.strHost = stSysConfigBaseInfo.szDBUsersHost;
	stMySQLConfig.strDBName = stSysConfigBaseInfo.szDBUsersDataBase;
	stMySQLConfig.nPort = stSysConfigBaseInfo.iDBUsersPort;
	stMySQLConfig.strUser = stSysConfigBaseInfo.szDBUsers;
	stMySQLConfig.strPassword = stSysConfigBaseInfo.szDBUsersPasswd;

	// ProbeMySQLConnection can tell the status of mysql
	CMySQLConnection::ProbeMySQLConnection(stMySQLConfig);
	CMySQLConnection hMySQLConnection;
	hMySQLConnection.SetConnectionConfig(stMySQLConfig);
	hMySQLConnection.SetConnectionName("game");
	bool bConnectOK = hMySQLConnection.OpenConnection();
	if (!bConnectOK)
	{
		_log(_ERROR, "DB", "cant't connect to mySql:host[%s:%d],[%s]:[%s]", stSysConfigBaseInfo.szDBUsersHost, stSysConfigBaseInfo.iDBUsersPort, stSysConfigBaseInfo.szDBUsersDataBase, stSysConfigBaseInfo.szDBUsers);
		exit(0);
	}

	MsgQueue *pGetQueue = new MsgQueue(2000,1024*7+512);
	MsgQueue *pSendQueue = new MsgQueue(2000,1024*7+512);
	

	//创建并启动通信线程
	EpollSockThread *pEpoll = new EpollSockThread(100,TcpSock::WRITE_SINGLE_THREAD_MODE,1020*10,1024*10);
	pEpoll->Initialize(pGetQueue,pSendQueue,stSysConfigBaseInfo.iServerPort,stSysConfigBaseInfo.iHeartTime);
	pEpoll->Start();

	ServerLogicThread *pLogicThread = new ServerLogicThread(pGetQueue,pSendQueue,&hMySQLConnection);
	pLogicThread->Start();

	_log(_ERROR, "ROOM_SERVER", "v1.01 server run ok---serverport[%d]",stSysConfigBaseInfo.iServerPort);

	sem_wait(&ProcBlock);		//在此永远阻塞主线程

	hMySQLConnection.CloseConnection();	//退出数据库

	pEpoll->Stop();
	delete pEpoll;
	pEpoll = NULL;

	pLogicThread->Stop();
	delete pLogicThread;
	pLogicThread = NULL;
	
	delete pGetQueue;
	delete pSendQueue;
	
	release_log_conf();
}

//16进制转字符串
void HextoStr(const unsigned char *pHex, char *pStr,  int iLen)
{
	int j = 0;
	char cTemp[200];
	memset(cTemp,0,200);
	for(int i=0; i<iLen; i++)
	{
		if(pHex[i] >= 'a')
		{
			cTemp[i] = pHex[i] - 97 + 10;
		}
		else if(pHex[j] >= '0')
		{
			cTemp[i] = pHex[i] - 48;
		}

		if(i%2 == 1)
		{
			pStr[i/2] = cTemp[i-1] * 16 + cTemp[i];
		}
	}
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