#ifndef _MAIN_H_
#define _MAIN_H_


#define DB_USER		0
#define DB_USER2	1
#define DB_GAME  	2
#define DB_GAME2  	3
#define DB_USER3	4

#define DB_GAME3 5
#define DB_GAME4 6
#define DB_GAME5 7

#define DB_USER4	8
#define DB_USER5	9
#define DB_USER6	10

#define DB_USER7	11

typedef struct SystemConfigBaseInfo
{
	int	 iServerPort;
	int	 iHeartTime;

	int	iServerID;

	char szDBGameHost[200];
	int iDBGamePort;
	char szDBGame[50];
	char szDBGamePasswd[100];
	char szDBGameDataBase[50];

	char szDBUserHost[200];
	int iDBUserPort;
	char szDBUser[50];
	char szDBUserPasswd[100];
	char szDBUserDataBase[50];

	int iAppMTime;

	char szRedisHost[128];
	int iRedisPort;
	char szRedisPwd[100];
	int iDBIndex;

	char szRedisHost2[128];
	int iRedisPort2;
	char szRedisPwd2[100];
	int iDBIndex2;

	int iIfTestPlatform;//是否是测试平台
	int iIfMainRedis;//是否是主redis（所有redis中仅有一个是主redis）

	int iAppStartTime;
}SystemConfigBaseInfoDef;



#endif /* PROXY_H */


