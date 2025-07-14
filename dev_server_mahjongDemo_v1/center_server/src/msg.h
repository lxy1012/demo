#ifndef __MSG_H__
#define __MSG_H__

#include <time.h>
#include <string>
using namespace  std;

#define MESSAGE_VERSION						0x22		//消息版本

const int BASE_SEVER_TYPE_CENTER = 1;
const int BASE_SEVER_TYPE_RADIUS = 2;
const int BASE_SEVER_TYPE_REDIS = 3;
const int BASE_SEVER_TYPE_BULL = 4;
const int BASE_SEVER_TYPE_ROOM = 5;
const int BASE_SEVER_TYPE_LOG = 6;
const int BASE_SEVER_TYPE_CHAT = 7;
const int BASE_SEVER_TYPE_OTHER1 = 8;
const int BASE_SEVER_TYPE_OTHER2 = 9;


//1:公共消息
#define KEEP_ALIVE_MSG                  0xE0	//保持连接消息
//5: 游戏Server和room通信
#define GAME_AUTHEN_REQ_RADIUS_MSG		0x100	//游戏服务器登陆请求/回应
#define GAME_ROOM_INFO_REQ_RADIUS_MSG	0x101	//游戏服务器请求房间信息，结构体中附带房间人数
#define GAME_SYS_APPTIME_RADIUS_MSG		0x102	//底层服务器通过游戏服务器更信息应用启动时间等信息
#define GAME_SERVER_EPOLL_RESET_IP_MSG  0x103	//服务器线通信线程内重置ip信息
#define SERVER_COM_INNER_MSG			0x104	//服务器多线程之间传递消息通用
#define GAME_UPDATE_LAST_GAMENUM		0x105	//每局结束后更新服务器最近一次牌局编号

//server和radius消息
#define GAME_USER_AUTHEN_INFO_MSG		0x200	//游戏服务器通过radius获取用户登录信息
#define GAME_USER_PROP_INFO_MSG			0x201	//用户道具信息回应
#define GAME_USER_ACCOUNT_MSG			0x202	//游戏服务器计费请求信息
#define GAME_USER_MAIN_INFO_SYS_MSG		0x203	//游戏服务器请求同步用户主要信息
#define GAME_GET_PARAMS_MSG				0x204	//游戏服务器获取params配置
#define SERVER_LOG_LEVEL_SYNC_MSG	    0x301	//各服务器同步日志等级信息

//server和log消息
#define GMAE_RECORD_GAME_LOG_MSG		0x260	//游戏服务器给log服务器发送统计每局游戏日志
#define GAME_MAIN_MONETARY_LOG_MSG		0x261	//主要货币（游戏币/钻石/奖券）日志
#define GAME_PROP_LOG_MSG				0x262	//道具日志

//server向中心服务器发送请求
#define GAME_SYS_ONLINE_TO_CENTER_MSG	0x160	//游戏服务器向中心服务器同步在线人数
#define GAME_SYS_DETAIL_TO_CENTER_MSG	0x161	//游戏服务器信息发生变化时同步服务器详细信息至中心服务器
#define NEW_CENTER_GET_USER_SIT_MSG		0x162	//游戏服务器请求分配座位请求以及中心服务器的回应
#define NEW_CNNTER_USER_SIT_ERR			0x163	//游戏服务器通知中心服务器用户入座异常
#define NEW_CENTER_USER_CHANGE_MSG		0x164	//中心服务器通知游戏服务器内某玩家换服配桌
#define NEW_CENTER_ASSIGN_TABLE_OK_MSG	0x165	//游戏服务器通知中心服务器最终配桌成功消息 
#define NEW_CENTER_USER_LEAVE_MSG		0x166   //游戏服务器通知中心服务器用户在等待配桌的过程中离开
#define NEW_CENTER_REQ_USER_LEAVE_MSG   0x167	//中心服务器通知游戏服务器内某玩家该服务器不处于开放状态需要换服或提示房间已关闭
#define NEW_CENTER_VIRTUAL_ASSIGN_MSG   0x168 //中心服务器通知游戏服务器可先虚拟配桌消息
#define NEW_CENTER_FREE_VTABLE_MSG		0x170 //游戏服务器通知中心服务器回收虚拟桌号
#define NEW_CENTER_TABLE_NUM_MSG		0x16A //游戏服务器通知中心服务器桌子数量变化
#define NEW_CNNTER_USER_ASSIGN_ERR		0x16B //游戏服务器通知中心服务器配桌异常

//server和redis消息
#define GAME_REDIS_GET_USER_INFO_MSG	0x180	//游戏服务器通过redis获取用户登录信息回应

//centerServer和redis消息
#define RD_GET_VIRTUAL_AI_INFO_MSG		0x209	//新增:收到游戏服请求获取虚拟AI信息，转发给到redis
#define RD_USE_CTRL_AI_MSG				0x213	//新增：收到游戏服请求刷新虚拟AI的占用状态消息 转发给到redis
#define RD_GET_VIRTUAL_AI_RES_MSG           0x214   //获取多个ai占用信息

#define REDID_NCENTER_SET_LATEST_USER_MSG	0x280 //新配桌流程中配桌成功后记录同桌玩家
#define REDIS_RECORD_NCENTER_STAT_INFO		0x281 //记录新中心服务器统计信息(中心服务器统计)
#define REDIS_ASSIGN_NCENTER_STAT_INFO		0x282 //配桌时长统计
#define REDIS_GET_PARAMS_MSG			    0x283	//中心服务器获取params配置

#define RD_GET_BATCH_ROOM_ID_MSG    0x318  //批量获取房号(cener<--->redis)
/***和redis交互信息 end****/

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char ucahr;

/**************非消息结构体定义**************************/
typedef struct MsgHead	//消息头结构体
{
	uint iMsgType;  //类型
	uint iSocketIndex;
	int iFlagID;		//客户端交互消息用userid填充
	char cMsgFromType;	//来源标记	
	char cVersion;      //版本
	char cFlag1;		//预留标记1    低位起第一位是否加密
	char cFlag2;		//预留标记2
}MsgHeadDef;

/**************非消息结构体定义结束**************************/
typedef struct KeepAlive			//KEEP_ALIVE_MSG  保持连接消息
{
	MsgHeadDef msgHeadInfo;
	char szEmpty[4];
} KeepAliveDef;

typedef struct GameRDAuthenReqPre 		//GAME_AUTHEN_REQ_RADIUS_MSG		服务器登陆请求/回应
{
	MsgHeadDef msgHeadInfo;
	int iReqType;//请求类型（0游戏登录room服务器，1中心服务器登录room服务器,2游戏服务器登录中心以及其他底层服务器）
	int iServerID;
}GameRDAuthenReqPreDef;

typedef struct GameRDAuthenSimpleReq 		//GAME_AUTHEN_REQ_RADIUS_MSG		服务器登陆请求/回应（对应msgPre.iReqType == 0或msgPre.iReqType == 1）
{
	GameRDAuthenReqPreDef msgPre;
	int iStartTimeStamp;//程序启动时间戳
	int iAppTimeStamp;//程序时间戳
}GameRDAuthenSimpleReqDef;

typedef struct GameRDAuthenReq 		//GAME_AUTHEN_REQ_RADIUS_MSG		服务器登陆请求/回应（对应msgPre.iReqType == 2）
{
	GameRDAuthenReqPreDef msgPre;
	int iGameID;
	char szGameName[32];
	char szServerName[100];
	uint iServerIP;		//自己服务器的ip
	ushort sServerPort;//自己服务器的端口
}GameRDAuthenReqDef;

typedef struct GameRDAuthenRes		//GAME_AUTHEN_REQ_RADIUS_MSG		服务器登陆请求/回应
{
	MsgHeadDef msgHeadInfo;
	int iResult;//0失败 1成功
}GameRDAuthenResDef;

typedef struct GameSysAppTimeRDMsg	//GAME_SYS_APPTIME_RADIUS_MSG	 底层服务器通过游戏服务器更信息应用启动时间等信息
{
	MsgHeadDef msgHeadInfo;
	int iServerID;
	int iStartTimeStamp;//程序启动时间戳
	int iAppTimeStamp;//程序时间戳
}GameSysAppTimeRDMsgDef;


typedef struct GameRoomInfoReqRadius		//GAME_ROOM_INFO_REQ_RADIUS_MSG	游戏服务器请求房间信息，结构体中附带房间人数
{
	MsgHeadDef	 msgHeadInfo;
	int	iServerType;//0游戏服务，1中心服务器
	int    iServerID;
	int	iNeedRoomInfo;//是否需要服务器和房间配置信息(0不需要，仅更新人数，1需要获取所有配置信息)
	int    iRoomNum;//后接iRoonNum个GameOneRoomOnlineDef（或CenterRoomOnlineDef）信息
}GameRoomInfoReqRadiusDef;

typedef struct GameOneRoomOnline
{
	int iRoomType;
	int iOnlineNum;//该类型房间总在线人数
	int iOnlineAndroid;//该类型房间android在线人数
	int iOnlineIos;//该类型房间ios在线人数
	int iOnlinePC;//该类型房间PC在线人数
	int iOnlineOther;//该类型房间其他在线人数
	int iOnlineAI;//该类型房间AI在线人数
}GameOneRoomOnlineDef;

typedef struct CenterRoomOnline
{
	int	iSubServerNum;//当前下线服务器数量
	int	iMaxPlayerNum;//所有下线服务器容纳玩家数量
	int	iAllPlayerNum;//当前下线服务器总玩家数量	
}CenterRoomOnlineDef;

typedef struct GameRoomInfoResRadius		//GAME_ROOM_INFO_REQ_RADIUS_MSG，room服务器回应游戏/中心服务器房间信息
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	char szGameName[32];//游戏名称
	char szServerName[100];//服务器名称
	int	iBeginTime;	//开始时间
	int	iOpenTime;//开放持续时间
	int	iServerState;//服务器状态(0关闭，1正常，2维护)
	int	iServerRoomType;//游戏服务器用 （0混服，4新手，5初级，6中级，7高级，8vip）
	int iMaxPlayerNum;
	char cGameNum[20];//游戏服务器的牌局编号(只有首次回应需要用到)
	uint iServerIP;//游戏服务器的ip
	int iBaseServerNum;// 需要连接的底层服务器数量，后接iBaseServerNum个GameServerIPInfo
	int	iRoomNum;//该游戏所有配置的房间数量，后接iRoonNum个GameOneRoomDetail（对游戏服务器）或GameOneRoomBrief（对中心服务器）
	uint iInnerIP;      //内网ip
	short sInnerPort;
}GameRoomInfoResRadiusDef;

typedef struct GameServerIPInfo
{
	int iServerType;//1:中心服务器，2:Radius，3:Redis，4:Bull，5:room，6:log,7chat,8其他服务器1,9其他服务器2
	int iServerID;
	int iServerIDBak;	//备份的serverid
	uint iServerIP;		//服务器ip
	ushort sServerPort;	//服务器端口
	uint iServerIPBak;	//服务器ip备份
	ushort sServerPortBak;//服务器端口备份
}GameServerIPInfoDef;

typedef struct GameOneRoomDetail  //每个房间详情(回复给游戏服务器)
{
	int iRoomType;	//房间类型(4新手，5初级，6中级，7高级，8vip)
	char szRoomName[60];	//房间名称
	int iRoomState;	//房间状态(0关闭，1正常，2维护)
	long long iTableMoney;//桌费
	long long iBasePoint;	//底分
	long long iMinMoney;	//最低进入限制
	long long iMaxMoney;	//最高进入限制
	long long iKickMoney;	//游戏内踢出限制
	int iMinGameVer;	//需要最低的游戏功能版本号
	long long iMaxWin;	//单局封顶
	long long iMinPoint;	//最小玩分
	long long iMaxPoint;	//最大玩分
	int iMinTimes;	//最小倍数
	int iMaxTimes;	//最大倍数
	int iSpeFlag1;	//特殊标记1
	int iSpeFlag2;	//特殊标记2
	int iSpeFlag3;	//特殊标记3
	int iIPLimit;	//是否有ip限制
	int iBeginTime;	//房间每日开始时间
	int iEndTime;	//房间每日持续时间
}GameOneRoomDetailDef;


typedef struct GameOneRoomBrief //每个房间简况(回复给中心服务器)
{
	int iRoomType;//房间类型(4新手，5初级，6中级，7高级，8vip)
	int iRoomState;//房间状态(0关闭，1正常，2维护)
	int iIfIPLimit;//是否有ip限制
	int iSpeFlag1;//特殊标记1
	int iSpeFlag2;//特殊标记2
	int iSpeFlag3;//特殊标记3
}GameOneRoomBriefDef;

typedef struct GameSysOnlineToCenterMsg     //GAME_SYS_ONLINE_TO_CENTER_MSG 游戏服务器和中心服务器同步在线人数
{
	MsgHeadDef msgHeadInfo;
	int iServerID;
	int iMaxNum;
	int	iNowPlayerNum;
}GameSysOnlineToCenterMsgDef;

typedef struct   GameSysDetailToCenterMsg//GAME_SYS_DETAIL_TO_CENTER_MSG 游戏服务器信息发生变化时同步服务器详细信息至中心服务器
{
	MsgHeadDef msgHeadInfo;
	int iServerID;
	uint iIP;
	short sPort;
	int iBeginTime;
	int iOpenTime;
	int iMaxNum;
	int iServerState;//房间状态(0关闭，1正常，2维护)
	uint iInnerIP;
	short sInnerPort;
}GameSysDetailToCenterMsgDef;

typedef struct   GameServerEpollResetIPMsg //GAME_SERVER_EPOLL_RESET_IP_MSG 服务器线通信线程内重置ip信息
{
	MsgHeadDef msgHeadInfo;
	char szIP[20];
	ushort sServerPort;//服务器端口
	char szIPBak[20];
	ushort sServerPortBak;//服务器端口备份
}GameServerEpollResetIPMsgDef;

typedef struct ServerComInnerMsg //SERVER_COM_INNER_MSG	服务器多线程之间传递消息通用
{
	MsgHeadDef msgHeadInfo;
	int iExtraMsgType;//实际消息类型，每个服务器内部视实际需求设定
	int iExtraMsgLen;//实际消息长度
}ServerComInnerMsgDef;

typedef struct GameUpdateLastGameNum  //GAME_UPDATE_LAST_GAMENUM	每局结束后更新服务器最近一次牌局编号
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	int iServerID;
	char szGameName[20];
}GameUpdateLastGameNumDef;

//2_end:和游戏服务器原中心服务器消息

//server和radius消息
typedef struct GameUserAuthenInfoReq	//GAME_USER_AUTHEN_INFO_MSG 游戏服务器通过radius获取用户登录信息
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	char szUserToken[32];
	int	iGameID;
	int	iServerID;
	int	iRoomType;
	int	iAuthenFlag;//登录标记用(第1位是否需要平台道具，第2位是否需要指定对应gameid道具（见extrainfo，最多5个），第3位是否需要获取指定道具，（见extrainfo，最多5个）,2和3位不能同时指定)
	int	iExtraInfo[5];
} GameUserAuthenInfoReqDef;

typedef struct ServerLogLevelConfMsg	//SERVER_LOG_LEVEL_SYNC_MSG 各服务器同步日志等级信息
{
	MsgHeadDef msgHeadInfo;
	int iServerType;	//0game 1center 2radius 3redis 4bull 5room 6log 7chat 8other1 9other2 
	int	iServerId;
} ServerLogLevelConfMsgDef;

const int g_iTokenError = 1;//玩家游戏登录验证token错误
const int g_iUserInfoError = 2;//玩家登录验证用户信息读取/操作失败
const int g_iGameInfoError = 3;//玩家登录验证游戏信息读取/操作失败
const int g_iRegInfoError = 4;//玩家登录验证游戏信息读取/操作失败
const int g_iAccountForbid = 5;//玩家登录验证账号被禁用
const int g_iLockInOtherServer = 6;//玩家登录验证卡在其他服务器了
const int g_iLowClientVer = 7;//客户端功能版本过低
const int g_iNoEnoughMoney = 8;//游戏币低于房间限制
const int g_iTooManyMoney = 9;//游戏币高于房间限制
const int g_iRoomClosed = 10;//房间已关闭
const int g_iRoomMaintian = 11;//房间维护
const int g_iServerClosed = 12;//游戏服务器已关闭
const int g_iServerMaintain = 13;//服务器进入维护状态
const int g_iRepeatLoginError = 14;/* 重复的登录请求 */
const int g_iMoneySysError = 15;//游戏币结算后不同步
const int g_iDiamondSysError = 16;//星钻结算后不同步
const int g_iForbidError = 17;//账号被禁
const int g_iUserAccountDBError = 18;//计费时数据读取/操作异常
const int g_iIntegralSysError = 19;//奖券结算后不同步
const int g_iServerIDSysError = 20;//计费时发现和当前serverid不同步
const int g_iGameLogicKick = 21;//游戏逻辑判断需要踢出
const int g_iAuthenParamErr = 22;//登录传参错误
const int g_iRadiusAuthenErr = 23;//游戏服务器尚未连接radius服务器
const int g_iRoomAuthenErr = 24;//游戏服务器尚未收到room服务器房间信息回应
const int g_iNeedChangeServer = 25;//需要换到指定服务器
const int g_iRoomIdInvalid = 26;//好友房房间号失效

const int g_iNoTCPConnect = 125;    //网络异常
const int g_iNoRoomList = 126;    //中心服务器返回房间列表为空
const int g_iRoomClose = 127;    //房间关闭
const int g_iRoomMaintain = 128;    //房间维护
const int g_iCenterMaintain = 129;    //中心服务器维护

//登录验证回应
typedef struct GameUserAuthenResPre
{
	MsgHeadDef msgHeadInfo;
	int	iAuthenRt;//0成功  >0失败
	int	iUserID;
}GameUserAuthenResPreDef;

typedef struct GameUserAuthenInfoFailRes
{
	GameUserAuthenResPre stResPre;
	int	iResMsgType;//0登录回应获取用户信息失败，1同步用户信息时获取用户信息失败
	int	iLastServerID;//所卡房间gameid
	int	iLastGameAndRoom;//所卡房间的gameiD和房间类型（低16位gameid，高16位房间类型）
}GameUserAuthenInfoFailResDef;

typedef struct GameUserAuthenInfoOKRes
{
	GameUserAuthenResPre stResPre;
	char szUserName[20];
	char szNickName[60];
	int	iVipType;
	int	iGender;//性别0未知1女2男
	int	iExp;
	int	iHeadImg;//头像
	long long iMoney;
	int	iDiamond;
	int	iIntegral;//奖券
	int	iTotalCharge;//总充值金额
	int	iSpreadID;//推广员id
	int	iTempChargeMoney;//暂时记录的充值的游戏币（游戏内登录时会转入money内，该值用于客户端内提示）
	int	iTempChargeDiamond;//暂时记录的充值的钻石（游戏内登录时会转入diamond内，该值用于客户端内提示）
	int	iSpeMark;//账号特殊标记
	int	iRegTime;//注册时间
	int	iRegType;//注册类型
	int	iAllGameTime;//总游戏时间
	int iAuthenResFalag;//第1位是否是跨天登录游戏
	//以下为对应的游戏信息
	int	iGameTime;//本游戏时间
	int	iWinNum;//总赢局数
	int	iLoseNum;//总输局数
	int	iAllNum;//总局数
	int	iWinMoney;//总赢游戏币（缩小了1000倍）
	int	iLoseMoney;//总输游戏币（缩小了1000倍）
	int	iGetIntegral;//游戏内总获得奖券数
	int	iFirstGameTime;//首次游戏时间
	int	iLastGameTime;//上次游戏时间
	int	iGameExp;//游戏经验值
	int	iBuffA0;//游戏控制值A0（赋值型）
	int	iBuffA1;//游戏控制值A1（赋值型）
	int	iBuffA2;//游戏控制值A2（赋值型）
	int	iBuffA3;//游戏控制值A3（赋值型）
	int	iBuffA4;//游戏控制值A4（赋值型）
	int	iBuffB0;//游戏控制值B0（增加型）
	int	iBuffB1;//游戏控制值B1（增加型）
	int	iBuffB2;//游戏控制值B2（增加型）
	int	iBuffB3;//游戏控制值B3（增加型）
	int	iBuffB4;//游戏控制值B4（增加型）
	int	iContinueWin;//连赢次数
	int	iContinueLose;//连输次数
	int	iDayWinNum;//今日总赢局数
	int	iDayLoseNum;//今日总数局数
	int	iDayAllNum;//当日总游戏局数
	int	iDayWinMoney;//今日游戏总赢游戏币（缩小了1000倍）
	int	iDayLoseMoney;//今日游戏总输游戏币（缩小了1000倍）
	int	iDayIntegral;//今日在该游戏获得奖券数目
	int	iRecentPlayCnt;//近期总玩次数
	int	iRecentWinCnt;//近期赢次数
	int	iRecentLoseCnt;//近期输次数
	int	iRecentWinMoney;//近期游戏赢分(不包括桌费)
	int	iRecentLoseMoney;//近期游戏输分(不包括桌费)
} GameUserAuthenInfoOKResDef;

//近期游戏数据，每日数据：日期1_局数1_赢局数1_输局数1_游戏赢分1_游戏输分1_桌费1_奖券1_耗时秒1
typedef struct RecentGameInfo
{
	int iDayFlag;
	int iAllNum;
	int iWinNum;
	int iLoseNum;
	int iWinMoney;
	int iLoseMoney;
	int iTabelMoney;
	int iGameGetIntegral;
	int iGameTime;
}RecentGameInfoDef;

typedef struct GameUserProReq //define GAME_USER_PROP_INFO_MSG 用户道具信息回应
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iFlag;//登录标记用(第1位是否需要平台道具，第2位是否需要指定对应gameid道具（见extrainfo，最多5个），第3位是否需要获取指定道具，（见extrainfo，最多5个）,2和3位不能同时指定)
	int	iExtraInfo[5];
}GameUserProReqDef;

typedef struct GameUserProRes//define GAME_USER_PROP_INFO_MSG 用户道具信息回应
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iFlag;
	int	iPropNum;//后接iPropNum个GameUserOnePropDef
}GameUserProResMsg;

typedef struct GameUserOneProp
{
	int	iPropID;
	int	iPropNum;
	int	iLastTime;//有效期
}GameUserOnePropDef;

typedef struct GameUserAccountReq   //GAME_USER_ACCOUNT_MSG	游戏服务器计费请求信息
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iGameID;
	int	iServerID;
	char szUserToken[32];
	long long	llRDMoney;//需要计费的游戏币
	int	iRDDiamond;//需要计费的星钻
	int	iRDIntegral;//需要计费的奖券
	int	iRDGameTime;//游戏时长(秒)
	int	iRDExp;
	int	iIfQuit;//是否退出游戏
	int	iNowSpeMark;//当前特殊标记(和数据库不一致时，在计费回应内重新同步)
	long long  iNowMoney;//当前计费后的游戏币（和数据库计费不一致时，要回复计费异常）
	int	iNowDiamond;//当前计费后的星钻（和数据库计费不一致时，要回复计费异常）
	int	iReqFlag;//计费请求标记位，第1位是否有游戏信息需要更新(有的话后接GameUserAccountGame)
}GameUserAccountReqDef;

typedef struct GameUserAccountGame
{
	int	iWinNum;//游戏赢增加局数
	int	iLoseNum;//游戏输增加局数
	int	iAllNum;//总游戏增加局数
	long long llWinMoney;//总赢游戏币（不包括桌费，仅指游戏输赢）
	long long llLoseMoney;//总输游戏币（不包括桌费，仅指游戏输赢）
	long long llTableMoney;//总桌费
	long long llGameAmount;//游戏总净分
	int	iGameExp;//游戏自己经验值
	int	iBuffA0;//buffeA0的新值
	int	iBuffA1;//buffeA1的新值
	int	iBuffA2;//buffeA2的新值
	int	iBuffA3;//buffeA3的新值
	int	iBuffA4;//buffeA4的新值
	int	iAddBuffB0;//buffeB0的增加值
	int	iAddBuffB1;//buffeB1的增加值
	int	iAddBuffB2;//buffeB2的增加值
	int	iAddBuffB3;//buffeB3的增加值
	int	iAddBuffB4;//buffeB4的增加值
	int	iContinueWin;//当前连赢局数新值
	int	iContinueLose;//当前连输局数新值
	int	iDisonlineCnt;//掉线次数
}GameUserAccountGameDef;


typedef struct GameUserAccountRes
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int iAccountType;//0游戏正常结算，1游戏特殊计费
	int	iRt;//（0正常，其他见上述g_iAccountTokenError等）
	int	iSpeMark;//账号标记（游戏中可以同步账号标记）
}GameUserAccountResDef;

typedef struct GameUserInfoSysReq// GAME_USER_MAIN_INFO_SYS_MSG				0x203	//游戏服务器请求同步用户核心信息
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int iIfConvertCharge;//是否将iTempChargeMoney和iTempChargeDiamond转移到用户节点内
}GameUserInfoSysReqDef;

typedef struct GameUserInfoSysRes
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iHeadImg;//头像
	long long iMoney;
	int	iDiamond;
	int	iIntegral;//奖券
	int	iTotalCharge;//总充值金额
	int iIfConvertCharge;
	int	iTempChargeMoney;//暂时记录的充值的游戏币（游戏内登录时会转入money内，该值用于客户端内提示）
	int	iTempChargeDiamond;//暂时记录的充值的钻石（游戏内登录时会转入diamond内，该值用于客户端内提示）
}GameUserInfoSysResDef;


typedef struct GameGetParamsReq//GAME_GET_PARAMS_MSG	游戏服务器获取params配置
{
	MsgHeadDef msgHeadInfo;
	int iKeyNum;
	//char cParamsKey[32];//后接iKeyNum个cParamsKey
}GameGetParamsReqDef;

typedef struct GameGetParamsRes
{
	MsgHeadDef msgHeadInfo;
	int iKeyNum;
	//char cParamsKey[32];//后顺序接iKeyNum个(cParamsKey+cParamsValue)
	//char cParamsValue[128];		
}GameGetParamsResDef;

//和redis交互消息

typedef struct GameRedisGetUserInfoReq//GAME_REDIS_GET_USER_INFO_MSG 0x310 //游戏服务器通过redis获取用户登录信息回应
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iGameID;
	int	iServerID;
	int	iRoomType;
	int	iPlayType;
	int	iPlatform;
	int	iLoginType;
	char szIP[20];
	int	iReqFlag;//信息请求标记位，第1位是否需要近期同桌信息
}GameRedisGetUserInfoReqDef;

typedef struct GameRedisGetUserInfoRes
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iDayReliefCnt;//今日救济次数
	int	iDayReliefAmount;//今日救济总额
	int	iReqFlag;//信息回应标记位，第1位是否有近期同桌信息
}GameRedisGetUserInfoResDef;

typedef struct GameRedisLatestPlayers
{
	int iLastPlayerIndex;//近期同桌玩家索引
	int iLatestPlayUser[10];//近期玩家，最多保留10个
}GameRedisLatestPlayersDef;

typedef struct ServerLogLevelConfRes	//SERVER_LOG_LEVEL_SYNC_MSG 各服务器同步日志等级信息
{
	MsgHeadDef msgHeadInfo;
	int iServerType;	//0game 1center 2radius 3redis 4bull 5room 6log 7chat 8other1 9other2 
	int iLogLevel;
} ServerLogLevelConfResDef;

//end:server和radius消息

//server和log消息
typedef struct GameRecordGameLogMsg   //GMAE_RECORD_GAME_LOG_MSG游戏服务器给log服务器发送统计每局游戏日志
{
	MsgHeadDef msgHeadInfo;
	char szGameNum[20];//游戏编号
	int	iGameID;
	int	iServerID;
	int	iGameTime;//本局游戏时长(秒)
	char cPlayName[150];//同桌用户名(用","隔开)
	int iPlayerNum;//后接iPlayerNum个GameLogOnePlayerLog	
}GameRecordGameLogMsgDef;

typedef struct GameLogOnePlayerLog
{
	int	iUserID;
	char szUserName[20];
	int	iAgentID;
	int	iPlatform;
	int	iGameVer;
	int	iRoomType;
	int	iTableID;//(桌号*1000000+座位号)
	long long llBeginMoney;//开始游戏币
	long long llEndMoney;//结束时游戏币
	long long llWinMoney;//本局赢游戏币(不包括桌费)
	long long llLoseMoeny;//本局输游戏币(不包括桌费)
	long long iExtraAddMoney;//本局内非游戏获得的游戏币（如充值，抽奖等）
	long long iExtraLoseMoney;//本局内非游戏减少的游戏币(如使用道具扣除等)
	char cExtraMoneyDetail[160];//非游戏币计费详情(计费类型1_计费数目1&计费类型2_计费数目2...)
	int iPlayType;//本局玩法
	long long	iTableMoney;//本局桌费
	int	iBeginIntegral;//本局开始时奖券
	int	iEndIntegral;//本局结束时奖券
	int	iBeginDiamond;//本局开始时钻石
	int	iEndDiamond;//本局结束时钻石
	int	iGameRank;//游戏角色或结算排名
	int	iIfDisConnect;//是否掉线
	char cStrInfo1[100];//记录游戏信息字符串1
	char cStrInfo2[100];//记录游戏信息字符串2
	char cStrInfo3[100];//记录游戏信息字符串3
	char cStrInfo4[100];//记录游戏信息字符串4
	char cStrInfo5[100];//记录游戏信息字符串5
	char cStrInfo6[100];//记录游戏信息字符串6
	int	iNumInfo1;//记录游戏信息整形值1
	int	iNumInfo2;//记录游戏信息整形值2
	int	iNumInfo3;//记录游戏信息整形值3
	int	iNumInfo4;//记录游戏信息整形值4
	int	iNumInfo5;//记录游戏信息整形值5
	int	iNumInfo6;//记录游戏信息整形值6
}GameLogOnePlayerLogDef;

typedef struct GameMainMonetaryLogMsg//GAME_MAIN_MONETARY_LOG_MSG	主要货币（游戏币/钻石/奖券）日志
{
	MsgHeadDef msgHeadInfo;
	int	iMoneyType;//0游戏币 1钻石 2奖券
	int	iUserID;
	int	iGameID;
	char szUserName[20];
	int	iServerID;
	int	iLogType;
	long long llBeginNum;
	long long	iAddNum;
	long long	iExtraInfo;//额外信息，备用
}GameMainMonetaryLogMsgDef;

typedef struct  GamePropLog //GAME_PROP_LOG_MSG道具日志
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	char szUserName[20];
	int	iGameID;
	int	iServerID;
	int	iPropID;
	int	iLogType;
	int	iBeginNum;
	int	iAddNum;
	int	iExtraInfo;//额外信息，备用
}GamePropLogDef;


//3:和客户端玩家消息
#define   AUTHEN_REQ_MSG				0xE1  //玩家登陆验证请求
#define	  CLIENT_GET_CSERVER_LIST_MSG	0xF0 //客户端获得中心服务器的服务器列表消息（仅客户端和中心服务器使用）

typedef struct AuthenReq			//AUTHEN_REQ_MSG  玩家登陆验证请求
{
	MsgHeadDef msgHeadInfo;
	int  iUserID;  //用户ID
	char szUserToken[32];//用户TOKEN
	char szIP[20];
	char szMac[40];
	char cEnterRoomType;//进入的房间类型
	char cLoginType;	//平台类型0Android，1iPhone，2ipad，3微小，4抖小
	char cExtraFlag1;
	char cExtraFlag2;
	int iRoomID;		//创建的房间id，=0表示正常普通入座
	int iNowClientVer;	//当前功能版本号
	int iServerID;		//掉线重入需要的真正的二级server_id
	int iAgentID;
	int iVipType;
	int iSpeMark;
	int iPlayType;	//游戏玩法（每个游戏根据需要通过位移运算自行设定）
}AuthenReqDef;

typedef struct CenterServerRes	//CENTER_SERVER_RES_MSG
{
	MsgHeadDef	 msgHeadInfo;
	int iNum; //后跟多少个ServerInfoDef(-1房间已关闭，-2房间正在维护，-3该中心服务器已关闭/维护/隐藏需要重新获取信息)
}CenterServerResDef;

typedef struct ServerInfo
{
	uint iIP;
	short sPort;
	uint iInnerIP;
	short sInnerPort;
}ServerInfoDef;
/**************新增配桌等信息******************/
typedef struct RoomAllRoomInfoMsg		//NEW_CENTER_ALL_ROOM_INFO_MSG 0x61 //同步其他所有房间信息
{
	MsgHeadDef msgHeadInfo;
	int iRoomNum;//后接iRoomNum个OneRoomInfoDef
}RoomAllRoomInfoMsgDef;

typedef struct NCenterUserSitReq //NEW_CENTER_GET_USER_SIT_MSG
{
	MsgHeadDef  msgHeadInfo;
	int iUserID;
	int iAllGameCnt;
	int iWinCnt;
	int iLoseCnt;
	int iEnterIfFreeRoom;
	int iServerID;
	int iUserMarkLv;
	int iReqAssignCnt;		//用0表示今日首次入座,-1表示等待新一轮开局，其他1
	int iLastPlayerIndex;	//近期同桌玩家索引
	int iLatestPlayUser[10];//近期玩家，最多保留10个
	char szIP[20];
	int iVipType;	//90普通号，96维护号
	int iPlayType;	//玩法类型区分（只匹配相同玩法的玩家）
	int iMinPlayer;	//最少需要玩家数量
	int iMaxPlayer;	//最多需要玩家数量=iMinPlayer即固定玩家数量
	int iSpeMark;
}NCenterUserSitReqDef;

typedef struct NCenterUserSitReqNew
{
	NCenterUserSitReqDef msgSitInfo;
	int iHeadImg;//发起入座请求的玩家头像
	int iFrameID;//发起入座请求的玩家头像框
	char szNickName[60];//发起入座请求的玩家昵称	
	int iExtraInfo;//低位起，第1位是否有邀请其组队的玩家，第2位是否携带补发的组队id,第3位携带补发的虚拟桌id,第4位是否是来源于客户端入座请求
	//对应第一位，后接int邀请其组队玩家(首次登录时才传递,>0要进行组队判断)
	//对应第二位，后接int
	//对应第三位，后接int
}NCenterUserSitReqNewDef;


typedef struct NCenterUserSitRes //NEW_CENTER_GET_USER_SIT_MSG
{
	MsgHeadDef msgHeadInfo;
	int iUsers[10];//可同桌游戏的玩家（目前最多只有4个，预留一个）
	int iUserServerID[10];//这些玩家所在游戏服务器
	int iIfFreeRoom[10];//这些玩家所在房间类型
	int iToServerSocket;//该桌被分配进的服务器的socket索引
	int iNextVTableID;
	int iTablePlayType;//桌子玩法
}NCenterUserSitRes;

typedef struct NCenterSyncUserSitErr  //NEW_CNNTER_USER_SIT_ERR
{
	MsgHeadDef msgHeadInfo;
	int iUsers[10];
	int iSocketIndex;//0结果无需转发其他游戏服务器，>0需要转发的游戏服务器的socket
	int iSitRt[10];//1本服查无此人 2状态不对，用户已经被分配好了
	int iNVTableID;//配桌成功后分配的下一局的桌号
}NCenterSyncUserSitErrDef;

typedef struct NCenterUserAssignErr  //NEW_CNNTER_USER_ASSIGN_ERR
{
	MsgHeadDef  msgHeadInfo;
	int iUserID[10];  //配桌异常的玩家
	int iNVTableID;
}NCenterUserAssignErrDef;

typedef struct NCenterUserChangeMsg // NEW_CENTER_USER_CHANGE_MSG 0x64 //中心服务器通知游戏服务器内某玩家换服配桌
{
	MsgHeadDef msgHeadInfo;
	int iUserID[10];//请求换服的用户id
	int iNewServerSocket;
	int iNewServerIP;
	int iNewServerPort;
	int iNewInnerIP;
	int iNewInnerPort;
}NCenterUserChangeMsgDef;

typedef struct NCenterAssignTable0kMsg//NEW_CENTER_ASSIGN_TABLE_OK_MSG 0x65 //游戏服务器通知中心服务器最终配桌成功消息 
{
	MsgHeadDef msgHeadInfo;
	int iUserID[10];
	int iLastPlayerIndex[10];
	int iServerID;
}NCenterAssignTable0kMsgDef;

typedef struct NCenterUserLeaveMsg //NEW_CENTER_USER_LEAVE_MSG 0x66     //游戏服务器通知中心服务器用户在等待配桌的过程中离开
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iExtraInfo;//低位起，第1位仅重置可配桌状态
}NCenterUserLeaveMsgDef;

typedef struct NCenterReqUserLeaveMsg // NEW_CENTER_REQ_USER_LEAVE_MSG   0x67 //中心服务器通知游戏服务器内某玩家该服务器不处于开放状态需要换服或提示房间已关闭
{
	MsgHeadDef msgHeadInfo;
	int iUserID;//请求换服的用户id
	int iReqType;//0服务器不在开放时间内，1服务器进入维护状态且无其他可切换服务器，2服务器进入维护状态需要切换至指定服务器
	int iNewServerIP;
	int iNewServerPort;
	int iNewInnerIP;
	int iNewInnerPort;
}NCenterReqUserLeaveMsgDef;

typedef struct NCenterVirtualAssignMsg//NEW_CENTER_VIRTUAL_ASSIGN_MSG 中心服务器通知游戏服务器可先虚拟配桌消息
{
	MsgHeadDef  msgHeadInfo;
	int iMinPlayerNum;
	int iVTableID;
	int iUsers[10];//可虚拟同桌的玩家(一定都在本服)
	int iPlayType;
}NCenterVirtualAssignMsgDef;


typedef struct NCenterTabNumMsg  //NEW_CENTER_TABLE_NUM_MSG
{
	MsgHeadDef  msgHeadInfo;
	int iServerID;
	int iMaxNum;  //最大桌子数
	int iCurNum;  //当前
}NCenterTabNumMsgDef;

typedef struct NewCenterFreeVTableMsg//NEW_CENTER_FREE_VTABLE_MSG	游戏服务器通知中心服务器回收虚拟桌号
{
	MsgHeadDef  msgHeadInfo;
	int iVTableID;
}NewCenterFreeVTableMsgDef;
/**************新增配桌等信息 end******************/

typedef struct RDNCGetUserRoomReq// REDIS_NCENTER_GET_USER_ROOM_MSG 0x2C //新配桌流程中获取用户进入房间配桌信息
{
	MsgHeadDef msgHeadInfo;
	int iJudgeGameID;
	int iServerID;
	int iUserID;
	char szIP[20];
}RDNCGetUserRoomReqDef;

typedef struct RDNCGetUserRoomRes
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iIfFreeRoom;
	int iVipType;
	int iLastPlayerIndex;//近期同桌玩家索引
	int iLatestPlayUser[10];//近期玩家，最多保留10个
}RDNCGetUserRoomResDef;

typedef struct RDNCSetLatestUser//REDID_NCENTER_SET_LATEST_USER_MSG 0X2D //新配桌流程中配桌成功后记录同桌玩家
{
	MsgHeadDef msgHeadInfo;
	int iJudgeGameID;
	int iLatestIndex[10];
	int iUserID[10];
}RDNCSetLatestUserDef;

typedef struct NCAssignStaInfo //配桌统计信息
{
	int iAbnormalSitReq;//异常发送入座请求次数
	int iAbnormalSitUser;//异常发送入座请求玩家数
	int iSendReq2Num;// 超过5s还未入座玩家数
	int iSendReq3Num;// 超过10s还未入座玩家数
	int iSendReqMoreNum;// 超过15s还未入座玩家数
	int iNeedChangeNum;//需要换服次数
	int iSucTableOkNum;//成功开桌次数
	int iStartGameOk[5];//5种房间开局次数
	int iRoomWaitCnt[5];//5种房间等待配桌人数
	int iHighUserCnt;//等待配桌或已配桌高几率人数
	int iMidUserCnt;//等待配桌或已配桌中几率人数
	int iLowUserCnt;//等待配桌或已配桌低几率人数
	int iRightSecTableCnt;//同几率开桌次数
	int iRightRoomTableCnt;//同房间开桌次数
	int iUserRateCnt[11];//用户首次进入时落在各档几率的人数
	int iTmFlag;

	//20230617 added by cent
	int iSucTabOkExtra[4];  //各种情况开桌成功统计， 比赛中0全预赛开桌次数 1全决赛开桌次数 2预赛与决赛同桌开桌次数
}NCAssignStaInfoDef;

typedef struct RDNCRecordStatInfo//REDIS_RECORD_NCENTER_STAT_INFO  0x2E //记录新中心服务器统计信息
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	NCAssignStaInfoDef stAssignStaInfo;
}RDNCRecordStatInfoDef;

typedef struct AssignTmInfo
{
	int iAllCnt;
	long long llAllTm;
}AssignTmInfoDef;

typedef struct RDNCAssignTmStat//REDIS_ASSIGN_NCENTER_STAT_INFO
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	int iSubNum;    //后接iSubNum个数据块
	//int iExtraType;
	//int iAllCnt;      //记录次数
	//int iAverageTm;   //上次统计到这次之间iAllCnt平均时长
}RDNCAssignTmStatDef;



typedef struct RdGetBatchRoomIDReq//RD_GET_BATCH_ROOM_ID_MSG 批量获取房号
{
	MsgHeadDef msgHeadInfo;
	int iNeedNum;//需要批量获取的房间数量
}RdGetBatchRoomIDReqDef;

typedef struct RdGetBatchRoomIDRes//RD_GET_BATCH_ROOM_ID_MSG 批量获取房号
{
	MsgHeadDef msgHeadInfo;
	int iGetNum;//获取到的房间号数量
	//后接iGetNum个int型房间号
}RdGetBatchRoomIDResDef;

typedef struct RdGetVirtualAIInfoReq //RD_GET_VIRTUAL_AI_INFO_MSG 通过redis获取虚拟ai的信息请求
{
	MsgHeadDef msgHeadInfo;
	int iVirtualID;
	int iVTableID;
	int iGameID;
	int iServerType;
	int iServerID;		//当前消息需要发往哪个游戏服
	int iNeedNum;		//请求AI的数量
}RdGetVirtualAIInfoReqDef;

typedef struct RdGetVirtualAIRtMsg  //RD_GET_VIRTUAL_AI_RES_MSG
{
	MsgHeadDef msgHeadInfo;
	int iVTableID;//将请求的转发回来即可
	int iServerID;                //当前消息需要发往哪个游戏服
	int iRetNum;    //返回ai信息的数量  

	//每个游戏自己需要的数据结构 iRetNum个
	//int iVirtualID;//AI的ID
	//int game%d_iAchieveLevel;//成就等级
	//int game%d_iHeadFrame;//头像框
	//int game%d_iExp;     //经验值
	//int game%d_iPlayCnt;//总玩局数
	//int game%d_iWinCnt;//总赢局数
	//int game%d_iBufferA0
	//int game%d_iBufferA1
	//int game%d_iBufferA2				
	//int game%d_iBufferA3
	//int game%d_iBufferA4
	//int game%d_iBufferB0
	//int game%d_iBufferB1
	//int game%d_iBufferB2
	//int game%d_iBufferB3
	//int game%d_iBufferB4
	//int istrBuffer1Len;
	//game%d_strBuffer0
	//int istrBuffer2Len;
	//game%d_strBuffer1
};
#endif