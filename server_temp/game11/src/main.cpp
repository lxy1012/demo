
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
//公共库头文件
#include "log.h"         //写log用
#include "conf.h"        //从文件中读入配置
#include "emutex.h"      //临界资源多线程访问的锁定与释放
#include "msgqueue.h"    //消息队列
#include "aeslib.h"			 //解密用
//本系统相关头文件
#include "main.h"
#include "gamefactorycomm.h"
#include "hzxlmj_gamelogic.h" //对不同游戏，需要有所改变
#include "msg.h"
ServerBaseInfoDef stSysConfBaseInfo;     //系统配置参数结构体#include "main_ini.h"
int main(int argc, char *argv[])
{
	_note("####################### HZXLV1.0 Server Starting #######################");
	sem_t procBlock;
	sem_init(&procBlock, 0, 0);
	
	DIR *dir = opendir("./log");
	if (!dir)
		mkdir("./log",0700);	
	CMainIni iniInfo;
	iniInfo.IniMainLogInfo(&stSysConfBaseInfo,"hzxlmj_server","./hzxlmj_server.conf",NULL, argv[0]);	HZXLGameLogic *pGameLogic = new HZXLGameLogic();		/*****************下面可以加入自己需要的东西**************/	int rt = iniInfo.IniMainInfo(&stSysConfBaseInfo,"hzxlmj_server","./hzxlmj_server.conf",pGameLogic);	if(rt < 0)	{		_log(_ERROR, "MAIN", "ini error.");		return 0;
	}		//等待结束
	sem_wait(&procBlock);	
	iniInfo.Release();	_log(_DEBUG, "MAIN", "sem_wait(&procBlock) END");
	return 0;
}
void HextoString(const unsigned char *pHex, char *pStr,  int iLen)
{
	int j = 0;
	char cTemp[200];
	memset(cTemp,0,200);
	for(int i=0; i<iLen; i++)
	{
		if(pHex[i] >= 'a')
		{
			cTemp[i] = pHex[i] - 97 +10;
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
