#ifndef _MAIN_H_
#define _MAIN_H_


#define DB_LOG 0
#define DB_GAME_LOG 1

typedef struct SystemConfigBaseInfo
{
	int	 iServerPort;
	int	 iHeartTime;
	int  iIfLogDBDelay;		//是否记录数据库延迟

	int	iServerID;

	char szDBGameHost[200];
	int iDBGamePort;
	char szDBGame[50];
	char szDBGamePasswd[100];
	char szDBGameDataBase[50];

	int iAppMTime;
	int iAppStartTime;
	int iSendAppOpenTmCnt;//由连接过来的游戏服务器代为转发，最多转发3次
}SystemConfigBaseInfoDef;

void HextoStr(const unsigned char *pHex, char *pStr,  int iLen);
string ReadLink();

#endif /* PROXY_H */


