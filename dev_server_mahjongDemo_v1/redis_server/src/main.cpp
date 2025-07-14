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
#include "NewRedisThread.h"
#include "main.h"
#include "MaillHandler.h"
#include "TaskHandler.h"
#include "msg.h"
#include "EventLogHandler.h"
#include "CommonHandler.h"
#include "NCenterHandler.h"
#include "MySQLConnection.h"
#include "SQLWrapper.h"
#include "AESUtil.hpp"
#include <fstream>
#include "ConfUtil.h"
#include "RankHandler.h"

char g_szEncryptionKey[32] = "823^7viSPKuio48@"; 

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

		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBUserHost, "db_user_host", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueInt(&stSysConfigBaseInfo.iDBUserPort, "db_user_port", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBUser, "db_user_user", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBUserPasswd, "db_user_passwd", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBUserDataBase, "db_user_database", (char*)fileName.c_str(), "System Base Info", "");

		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBGameHost, "db_game_host", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueInt(&stSysConfigBaseInfo.iDBGamePort, "db_game_port", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBGame, "db_game_user", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBGamePasswd, "db_game_passwd", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szDBGameDataBase, "db_game_database", (char*)fileName.c_str(), "System Base Info", "");
	}
	else
	{
		GetValueStr(stSysConfigBaseInfo.szDBUserHost, "db_user_host", "server.conf", "System Base Info", "");
		GetValueInt(&stSysConfigBaseInfo.iDBUserPort, "db_user_port", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBUser, "db_user_user", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBUserPasswd, "db_user_passwd", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBUserDataBase, "db_user_database", "server.conf", "System Base Info", "");

		GetValueStr(stSysConfigBaseInfo.szDBGameHost, "db_game_host", "server.conf", "System Base Info", "");
		GetValueInt(&stSysConfigBaseInfo.iDBGamePort, "db_game_port", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBGame, "db_game_user", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBGamePasswd, "db_game_passwd", "server.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szDBGameDataBase, "db_game_database", "server.conf", "System Base Info", "");		
	}

	fileName = "redis1.cla";
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

		ConfUtil::GetValueInt(&stSysConfigBaseInfo.iRedisPort, "redis_port", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szRedisHost, "redis_host", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szRedisPwd, "redis_pwd", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueInt(&stSysConfigBaseInfo.iDBIndex, "redis_db", (char*)fileName.c_str(), "System Base Info", "");
	}
	else
	{
		GetValueInt(&stSysConfigBaseInfo.iRedisPort, "redis_port", "redis.conf", "System Base Info", "0");
		GetValueStr(stSysConfigBaseInfo.szRedisHost, "redis_host", "redis.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szRedisPwd, "redis_pwd", "redis.conf", "System Base Info", "");
		GetValueInt(&stSysConfigBaseInfo.iDBIndex, "redis_db", "redis.conf", "System Base Info", "0");
	}

	fileName = "redis2.cla";
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

		ConfUtil::GetValueInt(&stSysConfigBaseInfo.iRedisPort2, "redis_port", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szRedisHost2, "redis_host", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueStr(stSysConfigBaseInfo.szRedisPwd2, "redis_pwd", (char*)fileName.c_str(), "System Base Info", "");
		ConfUtil::GetValueInt(&stSysConfigBaseInfo.iDBIndex2, "redis_db", (char*)fileName.c_str(), "System Base Info", "");
	}
	else
	{
		GetValueInt(&stSysConfigBaseInfo.iRedisPort2, "redis_port", "redis2.conf", "System Base Info", "0");
		GetValueStr(stSysConfigBaseInfo.szRedisHost2, "redis_host", "redis2.conf", "System Base Info", "");
		GetValueStr(stSysConfigBaseInfo.szRedisPwd2, "redis_pwd", "redis2.conf", "System Base Info", "");
		GetValueInt(&stSysConfigBaseInfo.iDBIndex2, "redis_db", "redis2.conf", "System Base Info", "0");
	}

	GetValueInt(&stSysConfigBaseInfo.iHeartTime, "heart_time", "server.conf", "System Base Info", "");
	GetValueInt(&stSysConfigBaseInfo.iServerPort, "server_port", "server.conf", "System Base Info", "");
	GetValueInt(&stSysConfigBaseInfo.iServerID, "server_id", "server.conf", "System Base Info", "");

	
	//解开密码重新赋值
	//先解UserDBPasswd						
	/*char szHexPasswd[100], szStrPasswd[50];
	memset(szHexPasswd, 0, 100);
	memset(szStrPasswd, 0, 50);
	memcpy(szHexPasswd, stSysConfigBaseInfo.szDBGamePasswd, 100);
	memset(stSysConfigBaseInfo.szDBGamePasswd, 0, 100);
	HextoStr((unsigned char*)szHexPasswd, szStrPasswd, strlen(szHexPasswd));

	int iLen = strlen(stSysConfigBaseInfo.szDBGamePasswd);
	printf("log_pwd=%s,pwd[%s]\n", stSysConfigBaseInfo.szDBGamePasswd, g_szPassword);
	int rtn = aes_dec_r(szStrPasswd, strlen(szStrPasswd), g_szPassword, strlen(g_szPassword), stSysConfigBaseInfo.szDBGamePasswd, &iLen);
	if (rtn == -1)
	{
		printf("Cann't open log db password.\n");
		return -1;
	}*/

	//解开密码重新赋值
	/*memset(szHexPasswd, 0, 100);
	memset(szStrPasswd, 0, 50);
	memcpy(szHexPasswd, stSysConfigBaseInfo.szDBUserPasswd, 100);
	memset(stSysConfigBaseInfo.szDBUserPasswd, 0, 100);
	HextoStr((unsigned char*)szHexPasswd, szStrPasswd, strlen(szHexPasswd));

	iLen = strlen(stSysConfigBaseInfo.szDBUserPasswd);
	printf("log_pwd=%s,pwd[%s]\n", stSysConfigBaseInfo.szDBUserPasswd, g_szPassword);
	rtn = aes_dec_r(szStrPasswd, strlen(szStrPasswd), g_szPassword, strlen(g_szPassword), stSysConfigBaseInfo.szDBUserPasswd, &iLen);
	if (rtn == -1)
	{
		printf("Cann't open log db password.\n");
		return -1;
	}*/

	//redis密码解析
	/*memset(szHexPasswd, 0, 100);
	memset(szStrPasswd, 0, 50);
	memcpy(szHexPasswd, stSysConfigBaseInfo.szRedisPwd, 100);
	memset(stSysConfigBaseInfo.szRedisPwd, 0, 100);
	HextoStr((unsigned char*)szHexPasswd, szStrPasswd, strlen(szHexPasswd));
	iLen = strlen(stSysConfigBaseInfo.szRedisPwd);
	rtn = aes_dec_r(szStrPasswd, strlen(szStrPasswd), g_szPassword,
		strlen(g_szPassword), stSysConfigBaseInfo.szRedisPwd, &iLen);
	if (strlen(stSysConfigBaseInfo.szRedisHost) == 0 || stSysConfigBaseInfo.iRedisPort == 0 || strlen(stSysConfigBaseInfo.szRedisPwd) == 0)
	{
		printf("error redis config\n");
		return -2;
	}*/

	//memset(szHexPasswd, 0, 100);
	//memset(szStrPasswd, 0, 50);
	//memcpy(szHexPasswd, stSysConfigBaseInfo.szRedisPwd2, 100);
	//memset(stSysConfigBaseInfo.szRedisPwd2, 0, 100);
	//HextoStr((unsigned char*)szHexPasswd, szStrPasswd, strlen(szHexPasswd));
	//iLen = strlen(stSysConfigBaseInfo.szRedisPwd2);
	//rtn = aes_dec_r(szStrPasswd, strlen(szStrPasswd), g_szPassword,
	//	strlen(g_szPassword), stSysConfigBaseInfo.szRedisPwd2, &iLen);
	//if (strlen(stSysConfigBaseInfo.szRedisHost2) == 0 || stSysConfigBaseInfo.iRedisPort2 == 0 || strlen(stSysConfigBaseInfo.szRedisPwd2) == 0)
	//{
	//	printf("error redis2 config\n");
	//	return -2;
	//}
	return 0;
}

//显示配置
int ShowConfig()
{
	_note("ConfigFileName: %s", "server.conf");
	_note("[System Base Info]");
	_note("server_port: %d", stSysConfigBaseInfo.iServerPort);
	_note("heart_time: %d", stSysConfigBaseInfo.iHeartTime);

	_note("db_game: %s", stSysConfigBaseInfo.szDBGame);
	_note("db_game_database: %s", stSysConfigBaseInfo.szDBGameDataBase);

	_note("db_user: %s", stSysConfigBaseInfo.szDBUser);
	_note("db_user_database: %s", stSysConfigBaseInfo.szDBUserDataBase);
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
	
	strcpy(conf.log_file_name, "../log/redis");
	init_log_conf(conf);

	//写版本号
	_log(_DEBUG, "redis version", "v2.02-----20220609");

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
	stSysConfigBaseInfo.iAppStartTime = time(NULL);

	MySQLConfig stMySQLConfigGame;
	stMySQLConfigGame.strHost = stSysConfigBaseInfo.szDBGameHost;
	stMySQLConfigGame.strDBName = stSysConfigBaseInfo.szDBGameDataBase;
	stMySQLConfigGame.nPort = stSysConfigBaseInfo.iDBGamePort;
	stMySQLConfigGame.strUser = stSysConfigBaseInfo.szDBGame;
	stMySQLConfigGame.strPassword = stSysConfigBaseInfo.szDBGamePasswd;

	//连接数据库gcgame
	// ProbeMySQLConnection can tell the status of mysql
	CMySQLConnection::ProbeMySQLConnection(stMySQLConfigGame);
	CMySQLConnection hMySQLConnectioForLogic;//给主逻辑线程创建的数据库连接
	hMySQLConnectioForLogic.SetConnectionConfig(stMySQLConfigGame);
	hMySQLConnectioForLogic.SetConnectionName("gcgame");
	bool bConnectOK = hMySQLConnectioForLogic.OpenConnection();
	if (!bConnectOK)
	{
		_log(_ERROR, "DB", "cant't connect to mySql for logic:host[%s:%d],[%s]:[%s]", stSysConfigBaseInfo.szDBGameHost, stSysConfigBaseInfo.iDBGamePort, stSysConfigBaseInfo.szDBGameDataBase, stSysConfigBaseInfo.szDBGame);
		exit(0);
	}

	//连接数据库gcgame
	// ProbeMySQLConnection can tell the status of mysql
	CMySQLConnection::ProbeMySQLConnection(stMySQLConfigGame);
	CMySQLConnection hMySQLConnectioForEventLog;//给EventLogHandler创建的数据库连接
	hMySQLConnectioForEventLog.SetConnectionConfig(stMySQLConfigGame);
	hMySQLConnectioForEventLog.SetConnectionName("gcgameEvent");
	bConnectOK = hMySQLConnectioForEventLog.OpenConnection();
	if (!bConnectOK)
	{
		_log(_ERROR, "DB", "cant't connect to mySql for eventLog:host[%s:%d],[%s]:[%s]", stSysConfigBaseInfo.szDBGameHost, stSysConfigBaseInfo.iDBGamePort, stSysConfigBaseInfo.szDBGameDataBase, stSysConfigBaseInfo.szDBGame);
		exit(0);
	}

	//连接数据库gcgame
	// ProbeMySQLConnection can tell the status of mysql
	CMySQLConnection::ProbeMySQLConnection(stMySQLConfigGame);
	CMySQLConnection hMySQLConnectioForTask;//给TaskHandler创建的数据库连接
	hMySQLConnectioForTask.SetConnectionConfig(stMySQLConfigGame);
	hMySQLConnectioForTask.SetConnectionName("gcgameTask");
	bConnectOK = hMySQLConnectioForTask.OpenConnection();
	if (!bConnectOK)
	{
		_log(_ERROR, "DB", "cant't connect to mySql for task:host[%s:%d],[%s]:[%s]", stSysConfigBaseInfo.szDBGameHost, stSysConfigBaseInfo.iDBGamePort, stSysConfigBaseInfo.szDBGameDataBase, stSysConfigBaseInfo.szDBGame);
		exit(0);
	}

	//连接数据库gcgame
	// ProbeMySQLConnection can tell the status of mysql
	CMySQLConnection::ProbeMySQLConnection(stMySQLConfigGame);
	CMySQLConnection hMySQLConnectioForRank;//给RankHandler创建的数据库连接
	hMySQLConnectioForRank.SetConnectionConfig(stMySQLConfigGame);
	hMySQLConnectioForRank.SetConnectionName("gcgameRank");
	bConnectOK = hMySQLConnectioForRank.OpenConnection();
	if (!bConnectOK)
	{
		_log(_ERROR, "DB", "cant't connect to mySql for task:host[%s:%d],[%s]:[%s]", stSysConfigBaseInfo.szDBGameHost, stSysConfigBaseInfo.iDBGamePort, stSysConfigBaseInfo.szDBGameDataBase, stSysConfigBaseInfo.szDBGame);
		exit(0);
	}
	
	//连接数据库gcuser
	MySQLConfig stMySQLConfigUser;
	stMySQLConfigUser.strHost = stSysConfigBaseInfo.szDBUserHost;
	stMySQLConfigUser.strDBName = stSysConfigBaseInfo.szDBUserDataBase;
	stMySQLConfigUser.nPort = stSysConfigBaseInfo.iDBUserPort;
	stMySQLConfigUser.strUser = stSysConfigBaseInfo.szDBUser;
	stMySQLConfigUser.strPassword = stSysConfigBaseInfo.szDBUserPasswd;

	// ProbeMySQLConnection can tell the status of mysql
	CMySQLConnection::ProbeMySQLConnection(stMySQLConfigUser);
	CMySQLConnection hMySQLConnectionUserForTask;//给Task创建的数据库连接
	hMySQLConnectionUserForTask.SetConnectionConfig(stMySQLConfigUser);
	hMySQLConnectionUserForTask.SetConnectionName("gcuserTask");
	bConnectOK = hMySQLConnectionUserForTask.OpenConnection();
	if (!bConnectOK)
	{
		_log(_ERROR, "DB", "cant't connect to mySql for task:host[%s:%d],[%s]:[%s]", stSysConfigBaseInfo.szDBUserHost, stSysConfigBaseInfo.iDBUserPort, stSysConfigBaseInfo.szDBUserDataBase, stSysConfigBaseInfo.szDBUser);
		exit(0);
	}

	// ProbeMySQLConnection can tell the status of mysql
	CMySQLConnection::ProbeMySQLConnection(stMySQLConfigUser);
	CMySQLConnection hMySQLConnectionUserForMail;//给MaillHandler创建的数据库连接
	hMySQLConnectionUserForMail.SetConnectionConfig(stMySQLConfigUser);
	hMySQLConnectionUserForMail.SetConnectionName("gcuserMail");
	bConnectOK = hMySQLConnectionUserForMail.OpenConnection();
	if (!bConnectOK)
	{
		_log(_ERROR, "DB", "cant't connect to mySql for mail:host[%s:%d],[%s]:[%s]", stSysConfigBaseInfo.szDBUserHost, stSysConfigBaseInfo.iDBUserPort, stSysConfigBaseInfo.szDBUserDataBase, stSysConfigBaseInfo.szDBUser);
		exit(0);
	}

	//排行线程连接数据库gcuser
	CMySQLConnection::ProbeMySQLConnection(stMySQLConfigUser);
	CMySQLConnection hMySQLConnectionUserForRank;//给RankHandler创建的数据库连接
	hMySQLConnectionUserForRank.SetConnectionConfig(stMySQLConfigUser);
	hMySQLConnectionUserForRank.SetConnectionName("gcuserRank");
	bConnectOK = hMySQLConnectionUserForRank.OpenConnection();
	if (!bConnectOK)
	{
		_log(_ERROR, "DB", "cant't connect to mySql for rank:host[%s:%d],[%s]:[%s]", stSysConfigBaseInfo.szDBUserHost, stSysConfigBaseInfo.iDBUserPort, stSysConfigBaseInfo.szDBUserDataBase, stSysConfigBaseInfo.szDBUser);
		exit(0);
	}

	const int iSugAddNodeNum = 2048;

	
	//创建并启动通信线程
	MsgQueue *pSendQueue = new MsgQueue(6000,1024*7+512);
	pSendQueue->SetQueName("pSendQueue");

	EpollSockThread *pEpoll = new EpollSockThread(iSugAddNodeNum, TcpSock::WRITE_SINGLE_THREAD_MODE, 1024*10, 1024*10);
	pEpoll->Initialize(pSendQueue, stSysConfigBaseInfo.iServerPort, stSysConfigBaseInfo.iHeartTime);
	pEpoll->Start();

	//逻辑处理线程
	MsgQueue *pLogicGetQueue = new MsgQueue(5000, 1024*4 + 512);
	pLogicGetQueue->SetQueName("logicGetQue");
	ServerLogicThread *pLogicThread = new ServerLogicThread(pLogicGetQueue, pSendQueue);
	pLogicThread->SetMySqlConnect(&hMySQLConnectioForLogic);
	pEpoll->addQueueForMsgType(GAME_AUTHEN_REQ_RADIUS_MSG, pLogicGetQueue);
	pLogicThread->Start();
	
	//游戏统计日志处理线程
	MsgQueue* pRedisEventGetQueue = new MsgQueue(5000, 1024 * 2 + 256);
	pRedisEventGetQueue->SetQueName("eventLogGetQue");
	EventLogHandler* pEventLogHandler = EventLogHandler::shareEventHandler();
	NewRedisThread* pRedisEventLogThread = new NewRedisThread(2, &stSysConfigBaseInfo, pEventLogHandler, "eventLogTrd");
	pRedisEventLogThread->SetRedisQueue(pRedisEventGetQueue);
	pEventLogHandler->SetMySqlGame(&hMySQLConnectioForEventLog);
	pEventLogHandler->GetDBEventConf();
	pEpoll->addQueueForMsgType(SEVER_REDIS_COM_STAT_MSG, pRedisEventGetQueue);
	pEpoll->addQueueForMsgType(REDIS_USE_PROP_STAT_MSG, pRedisEventGetQueue);
	pEpoll->addQueueForMsgType(REDIS_GAME_BASE_STAT_INFO_MSG, pRedisEventGetQueue);
	pEpoll->addQueueForMsgType(REDIS_RECORD_NCENTER_STAT_INFO, pRedisEventGetQueue);
	pEpoll->addQueueForMsgType(REDIS_ASSIGN_NCENTER_STAT_INFO, pRedisEventGetQueue);
	pEpoll->addQueueForMsgType(RD_REPORT_COMMON_EVENT_MSG, pRedisEventGetQueue);
	pRedisEventLogThread->Start();

	//任务处理线程
	MsgQueue* pRedisTaskGetQueue = new MsgQueue(5000, 1024 * 5 + 256);
	pRedisTaskGetQueue->SetQueName("taskGetQue");
	TaskHandler* pTaskHandler = new TaskHandler(pSendQueue, pRedisEventGetQueue);
	NewRedisThread* pRedisTaskThread = new NewRedisThread(1, &stSysConfigBaseInfo, pTaskHandler, "taskTrd");
	pRedisTaskThread->SetRecordMsgQueue(true);
	pRedisTaskThread->SetRedisQueue(pRedisTaskGetQueue);
	pTaskHandler->SetMySqlGame(&hMySQLConnectioForTask);
	pTaskHandler->SetMySqlConnect(&hMySQLConnectionUserForTask);
	pTaskHandler->GetBKTaskConf();
	pEpoll->addQueueForMsgType(REDIS_TASK_GAME_RSULT_MSG, pRedisTaskGetQueue);
	pEpoll->addQueueForMsgType(GAME_REDIS_GET_USER_INFO_MSG, pRedisTaskGetQueue);
	pEpoll->addQueueForMsgType(RD_COM_TASK_COMP_INFO, pRedisTaskGetQueue);
	pEpoll->addQueueForMsgType(RD_UPDATE_USER_ACHIEVE_INFO_MSG, pRedisTaskGetQueue);
	pEpoll->addQueueForMsgType(RD_GAME_TOGETHER_USER_MSG, pRedisTaskGetQueue);
	pEpoll->addQueueForMsgType(RD_GAME_ROOM_TASK_INFO_MSG, pRedisTaskGetQueue);
	pEpoll->addQueueForMsgType(RD_GET_INTEGRAL_TASK_CONF, pRedisTaskGetQueue);
	pEpoll->addQueueForMsgType(RD_INTEGRAL_TASK_HIS_RES, pRedisTaskGetQueue); 
	pEpoll->addQueueForMsgType(RD_USER_DAY_INFO_MSG, pRedisTaskGetQueue);
	pEpoll->addQueueForMsgType(RD_GET_USER_LAST_MONTHS_VAC, pRedisTaskGetQueue);
	pRedisTaskThread->Start();

	//公用redis消息
	MsgQueue* pRedisCommonGetQueue = new MsgQueue(5000, 1024 * 5 + 256);
	pRedisCommonGetQueue->SetQueName("comGetQue");
	CommonHandler* pCommonHandler = new CommonHandler(pSendQueue);
	NewRedisThread* pCommonThread = new NewRedisThread(1, &stSysConfigBaseInfo, pCommonHandler, "commonTrd");
	pCommonThread->SetRedisQueue(pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_SET_BY_KEY, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_SET_BY_HASH, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_GET_BY_KEY, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_GET_BY_HASH, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_SET_BY_HASH_MAP, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_GET_BY_HASH_MAP, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_HASH_DEL, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_SORTED_SET_ZADD, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_SORTED_SET_ZINCRBY, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_SORTED_SET_ZRANGEBYSCORE, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_HGETALL, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_LPUSH, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_LRANGE, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_HINCRBY, pRedisCommonGetQueue);
	pEpoll->addQueueForMsgType(REDIS_COMMON_SET_KEY_EXPIRE, pRedisCommonGetQueue);
	pCommonThread->Start();
	
	//新中心服务器处理线程
	MsgQueue* pRedisNCenterGetQueue = new MsgQueue(5000, 1024 * 2 + 256);
	pRedisNCenterGetQueue->SetQueName("NCenterGetQue");
	NCenterHandler* pNCenterHandler = new NCenterHandler(pSendQueue);
	NewRedisThread* pRedisNCenterThread = new NewRedisThread(1, &stSysConfigBaseInfo, pNCenterHandler, "NCenterTrd");
	pRedisNCenterThread->SetRedisQueue(pRedisNCenterGetQueue);
	pEpoll->addQueueForMsgType(REDID_NCENTER_SET_LATEST_USER_MSG, pRedisNCenterGetQueue);
	pEpoll->addQueueForMsgType(GAME_REDIS_GET_USER_INFO_MSG, pRedisNCenterGetQueue);//GAME_REDIS_GET_USER_INFO_MSG比较特殊，多个线程需要处理
	pRedisNCenterThread->Start();

	//邮件处理线程
	MsgQueue* pRedisGetQueue = new MsgQueue(5000, 1024 * 2 + 256);
	pRedisGetQueue->SetQueName("mailGetQue");
	MaillHandler* pMailHandler = new MaillHandler(pSendQueue, pLogicGetQueue);
	NewRedisThread* pRedisThread = new NewRedisThread(1, &stSysConfigBaseInfo, pMailHandler, "mailTrd");
	pRedisThread->SetRedisQueue(pRedisGetQueue);
	pMailHandler->SetMySqlConnect(&hMySQLConnectionUserForMail);
	pEpoll->addQueueForMsgType(GAME_AUTHEN_REQ_RADIUS_MSG, pRedisGetQueue);
	pEpoll->addQueueForMsgType(SERVER_REDIS_MAIL_COM_MSG, pRedisGetQueue);
	pEpoll->addQueueForMsgType(RD_GAME_CHECK_ROOMNO_MSG, pRedisGetQueue);
	pEpoll->addQueueForMsgType(RD_GAME_UPDATE_ROOMNO_STATUS, pRedisGetQueue);
	pEpoll->addQueueForMsgType(RD_GET_VIRTUAL_AI_INFO_MSG, pRedisGetQueue);
	pEpoll->addQueueForMsgType(RD_SET_VIRTUAL_AI_RT_MSG, pRedisGetQueue);
	pEpoll->addQueueForMsgType(RD_USE_CTRL_AI_MSG, pRedisGetQueue);
	
	pRedisThread->Start();

	//排行相关线程
	MsgQueue* pRedisRankGetQueue = new MsgQueue(5000, 1024 * 2 + 256);
	pRedisRankGetQueue->SetQueName("rankGetQue");
	RankHandler* pRankHandler = new RankHandler(pSendQueue, pLogicGetQueue, pRedisEventGetQueue);
	NewRedisThread* pRankThread = new NewRedisThread(1, &stSysConfigBaseInfo, pRankHandler, "rankTrd");
	pRankThread->SetRedisQueue(pRedisRankGetQueue);
	pRankHandler->SetMySqlGame(&hMySQLConnectioForRank);
	pRankHandler->SetMySqlConnect(&hMySQLConnectionUserForRank);
	pEpoll->addQueueForMsgType(RD_GET_RANK_CONF_INFO_MSG, pRedisRankGetQueue);
	pEpoll->addQueueForMsgType(RD_GET_USER_RANK_INFO_MSG, pRedisRankGetQueue);
	pEpoll->addQueueForMsgType(RD_UPDATE_USER_RANK_INFO_MSG, pRedisRankGetQueue);
	pEpoll->addQueueForMsgType(REDIS_GET_PARAMS_MSG, pRedisRankGetQueue);
	pRankThread->Start();

	_log(_ERROR, "BK_REDIS_SERVER", "v1.01 server run ok---serverport[%d]",stSysConfigBaseInfo.iServerPort);

	sem_wait(&ProcBlock);		//在此永远阻塞主线程

	pEpoll->Stop();
	delete pEpoll;
	pEpoll = NULL;

	delete pMailHandler;
	delete pRedisGetQueue;
	delete pLogicGetQueue;
	delete pRedisNCenterGetQueue;
	delete pRedisCommonGetQueue;
	delete pRedisTaskGetQueue;
	delete pRedisEventGetQueue;

	pRankThread->Stop();
	delete pRankHandler;
	delete pRedisRankGetQueue;	
	delete pRankThread;

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