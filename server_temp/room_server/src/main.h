
#ifndef _MAIN_H_
#define _MAIN_H_

#define DB_LOG 0
#define DB_GAME_LOG 1
#include <string.h>

typedef struct SystemConfigBaseInfo
{
	int	 iServerPort;
	int	 iHeartTime;
	int  iIfLogDBDelay;		//ÊÇ·ñ¼ÇÂ¼Êý¾Ý¿âÑÓ³Ù

	int	iServerID;

	char szDBUsersHost[200];
	int iDBUsersPort;
	char szDBUsers[50];
	char szDBUsersPasswd[100];
	char szDBUsersDataBase[50];

	int iAppMTime;
}SystemConfigBaseInfoDef;

void HextoStr(const unsigned char *pHex, char *pStr,  int iLen);
string ReadLink();

#endif /* PROXY_H */


