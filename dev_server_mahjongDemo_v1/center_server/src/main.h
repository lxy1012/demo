#ifndef _MAIN_H_
#define _MAIN_H_

typedef struct SystemConfigBaseInfo
{
	int	 iServerPort;
	int	 iHeartTime;

	int iServerID;
	int iStartTimeStamp;
	int iAppMTime;

	//根据登录回应填充
	int iGameID;
	char szGameName[32];
	char szServerName[100];
	char szRoomName[60];
}SystemConfigBaseInfoDef;

#endif /* PROXY_H */