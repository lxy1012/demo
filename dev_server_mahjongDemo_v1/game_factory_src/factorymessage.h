#ifndef __FACTORY_MESSAGE_H__
#define __FACTORY_MESSAGE_H__

#include <time.h>
#include <string>
using namespace  std;

#define MESSAGE_VERSION			0x22		//消息版本



const int MAX_ROOM_NUM = 10;
const int MAX_PLAYER_NUM = 10;
const int MAX_FRIENDS_NUM = 200;

const int BASE_SEVER_TYPE_CENTER = 1;
const int BASE_SEVER_TYPE_RADIUS = 2;
const int BASE_SEVER_TYPE_REDIS = 3;
const int BASE_SEVER_TYPE_BULL = 4;
const int BASE_SEVER_TYPE_ROOM = 5;
const int BASE_SEVER_TYPE_LOG = 6;
const int BASE_SEVER_TYPE_CHAT = 7;
const int BASE_SEVER_TYPE_OTHER1 = 8;
const int BASE_SEVER_TYPE_OTHER2 = 9;
const int BASE_SEVER_TYPE_ACT = 10;

const int CLIENT_EPOLL_THREAD = 100;
const int CLIENT_ALL_EPOLL_THREAD = 101;



//1:公共消息
#define KEEP_ALIVE_MSG                   	0xE0	//保持连接消息

//5: 游戏Server和room通信
#define GAME_AUTHEN_REQ_RADIUS_MSG			0x100	//游戏服务器登陆请求/回应
#define GAME_ROOM_INFO_REQ_RADIUS_MSG		0x101	//游戏服务器请求房间信息，结构体中附带房间人数
#define GAME_SYS_APPTIME_RADIUS_MSG		    0x102	//底层服务器通过游戏服务器更信息应用启动时间等信息
#define GAME_SERVER_EPOLL_RESET_IP_MSG      0x103	//服务器线通信线程内重置ip信息
#define SERVER_COM_INNER_MSG				0x104	//服务器多线程之间传递消息通用
#define GAME_UPDATE_LAST_GAMENUM			0x105	//每局结束后更新服务器最近一次牌局编号

//server和radius消息
#define GAME_USER_AUTHEN_INFO_MSG			0x200	//游戏服务器通过radius获取用户登录信息
#define GAME_USER_PROP_INFO_MSG				0x201	//用户道具信息回应
#define GAME_USER_MAIN_INFO_SYS_MSG			0x203	//游戏服务器请求同步用户主要信息
#define GAME_USER_ACCOUNT_MSG				0x202	//游戏服务器计费请求信息
#define GAME_GET_PARAMS_MSG					0x204	//游戏服务器获取params配置
#define GAME_SPECIAL_ACCOUNT_MSG			0x205	//游戏中对主要货币特殊计费
#define GAME_UPDATE_USER_PROP_MSQ			0x206	//游戏中对道具计费
#define SERVER_LOG_LEVEL_SYNC_MSG			0x301	//各服务器同步日志等级信息
#define GAME_CHECK_FRIENDSHIP_MSG           0x20A   //检查玩家与同桌玩家是否有好友关系
#define GAME_GET_USER_GROWUP_EVENT_MSG		0x20B	//查询用户成长事件信息
#define GAME_UPDATE_USER_GROWUP_EVENT_MSG	0x20C	//更新用户成长事件信息
#define GAME_GET_FRIEND_LIST_MSG			0x20D	//获取好友列表
#define GAME_GET_MUL_GROWUP_EVENT_MSG		0x20E	//查询多条用户成长事件信息


//server和log消息
#define GMAE_RECORD_GAME_LOG_MSG		0x260	//游戏服务器给log服务器发送统计每局游戏日志
#define GAME_MAIN_MONETARY_LOG_MSG		0x261	//主要货币（游戏币/钻石/奖券）日志
#define GAME_PROP_LOG_MSG				0x262	//道具日志
#define GAME_ACHIEVE_TASK_LOG_MSG		0x263   //游戏内领取成就点通知log

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
#define SEVER_REDIS_COM_STAT_MSG		0x181   //通用事件统计消息
#define SERVER_REDIS_MAIL_COM_MSG		0x182	//通用邮件信息
#define SEVER_REDIS_COM_STAT_VALID_MSG 0x183	//存在有效期得到通用事件统计消息
#define REDIS_RETURN_USER_TASK_INFO		0x184	//返回用户在该游戏的任务信息
#define REDIS_TASK_GAME_RSULT_MSG		0x185	//根据游戏结果更新任务
#define REDIS_RETURN_DECORATE_INFO		0x186	//登录后返回用户装扮道具信息

#define REDIS_COMMON_SET_BY_KEY			0x188	//通过 key value的方式设置数据
#define REDIS_COMMON_SET_BY_HASH		0x189	//通过 hash的方式设置数据
#define REDIS_COMMON_GET_BY_KEY			0x190	//通过 key value的方式获取数据
#define REDIS_COMMON_GET_BY_HASH		0x191	//通过 hash的方式获取数据
#define REDIS_COMMON_SET_BY_HASH_MAP	0x192	//通过 hash的方式设置多条数据
#define REDIS_COMMON_GET_BY_HASH_MAP	0x193	//通过 hash的方式获取多条数据
#define REDIS_COMMON_HASH_DEL			0x194	//通过 hash 删除 field
#define REDIS_COMMON_SORTED_SET_ZADD	0x195	//有序集合 zadd
#define REDIS_COMMON_SORTED_SET_ZINCRBY			0x196  //有序集合 ZINCRBY
#define REDIS_COMMON_SORTED_SET_ZRANGEBYSCORE	0x197  //有序集合 获取指定区间的数据
#define REDIS_COMMON_HGETALL			0x198	//哈希 hgetall
#define REDIS_COMMON_LPUSH				0x199	//list lpush
#define REDIS_COMMON_LRANGE				0x200	//list lrange
#define REDIS_COMMON_HINCRBY			0x201	//HINCRBY
#define REDIS_COMMON_SET_KEY_EXPIRE		0x202	//设置当前key的过期时间

#define RD_COM_TASK_SYNC_INFO	0x203	//redis同步给游戏服务器当前某用户需要同步的所有通用任务
#define RD_COM_TASK_COMP_INFO	0x204	//游戏服务器向redis同步某玩家完成某个通用任务的请求
#define REDIS_RETURN_USER_ROOM_MSG		0x205	//新配桌流程中获取用户进入房间配桌信息
#define REDIS_GAME_BASE_STAT_INFO_MSG	0x207	//游戏次留等公用基础统计信息
#define REDIS_USE_PROP_STAT_MSG         0x208   //记录游戏服使用道具的日志

#define RD_GET_VIRTUAL_AI_INFO_MSG 0x209 //通过redis获取虚拟ai的信息
#define RD_SET_VIRTUAL_AI_RT_MSG 0x210  //结算时对虚拟AI的总局数等计费
#define RD_INTEGRAL_TASK_HIS_RES            0x211   //获取历史奖券任务记录回应
#define RD_USER_DAY_INFO_MSG                0x212   //获取/刷新玩家当日数据
#define RD_USE_CTRL_AI_MSG                  0x213   //刷新控制ai占用信息
#define RD_GET_VIRTUAL_AI_RES_MSG           0x214   //获取多个ai占用信息
#define RD_REPORT_COMMON_EVENT_MSG          0x215   //通用事件统计
#define RD_GET_USER_LAST_MONTHS_VAC			0x217	//游戏服向redis请求玩家最近的充值记录
#define RD_SET_USER_LAST_MONTHS_VAC			0x218	//redis向服务器返回近期所有的充值记录

//centerServer和redis消息
#define REDID_NCENTER_SET_LATEST_USER_MSG	0x280	//新配桌流程中配桌成功后记录同桌玩家
#define REDIS_RECORD_NCENTER_STAT_INFO		0x281	//记录新中心服务器统计信息(中心服务器统计)
#define REDIS_ASSIGN_NCENTER_STAT_INFO		0x282	//配桌时长统计
#define REDIS_GET_PARAMS_MSG			    0x283	//中心服务器获取params配置


#define RD_GAME_ROOM_TASK_INFO_MSG  0x300        //获取房间任务信息

//bull，redis,游戏服务器，客户端的好友相关交互信息
#define RD_GAME_CHECK_ROOMNO_MSG 0x302 //检测房号是否可用
#define RD_GAME_UPDATE_ROOMNO_STATUS 0x303 //更新房号最后状态
#define RD_GAME_TOGETHER_USER_MSG	0x31F	//玩家同桌玩家信息

//成就相关
#define RD_UPDATE_USER_ACHIEVE_INFO_MSG  0x319     //更新成就任务信息

//排行榜相关
#define RD_GET_RANK_CONF_INFO_MSG  0x31A     //获取排行榜配置信息
#define RD_GET_USER_RANK_INFO_MSG  0x31B     //获取用户当前排行积分信息
#define RD_UPDATE_USER_RANK_INFO_MSG  0x31C     //更新用户排行积分
#define RD_GET_INTEGRAL_TASK_CONF		 0x320	   //获取奖券任务信息

/***和redis交互信息 end***/

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char ucahr;

/**************非消息结构体定义**************************/
typedef struct MsgHead		//消息头结构体
{
	uint iMsgType;		//消息类型
	uint iSocketIndex;	//socket
	int iFlagID;		//客户端交互消息用userid填充
	char cMsgFromType;	//来源标记	
	char cVersion;		//版本号
	char cFlag1;		//预留标记1
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

typedef struct GameRDAuthenSimpleReq 	//GAME_AUTHEN_REQ_RADIUS_MSG		服务器登陆请求/回应（对应msgPre.iReqType == 0或msgPre.iReqType == 1）
{
	GameRDAuthenReqPreDef msgPre;
	int iStartTimeStamp;//程序启动时间戳
	int iAppTimeStamp;	//程序修改时间戳
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
	int iAppTimeStamp;	//程序修改时间戳
}GameSysAppTimeRDMsgDef;

typedef struct GameRoomInfoReqRadius		//GAME_ROOM_INFO_REQ_RADIUS_MSG	游戏服务器请求房间信息，结构体中附带房间人数
{
	MsgHeadDef msgHeadInfo;
	int	iServerType;//0游戏服务，1中心服务器
	int iServerID;
	int	iNeedRoomInfo;//是否需要服务器和房间配置信息(0不需要，仅更新人数，1需要获取所有配置信息)
	int iRoomNum;	//后接iRoonNum个GameOneRoomOnlineDef（或CenterRoomOnlineDef）信息
}GameRoomInfoReqRadiusDef;

typedef struct GameOneRoomOnline
{
	int iRoomType;
	int iOnlineNum;	//该类型房间总在线人数
	int iOnlineAndroid;//该类型房间android在线人数
	int iOnlineIos;	//该类型房间ios在线人数
	int iOnlinePC;	//该类型房间微小在线人数
	int iOnlineOther;//该类型房间其他在线人数
	int iOnlineAI;	//该类型房间AI在线人数
}GameOneRoomOnlineDef;

typedef struct CenterRoomOnline
{
	int	iSubServerNum;//当前下线服务器数量
	int	iMaxPlayerNum;//所有下线服务器容纳玩家数量
	int	iAllPlayerNum;//当前下线服务器总玩家数量	
}CenterRoomOnlineDef;

typedef struct GameRoomInfoResRadius	//GAME_ROOM_INFO_REQ_RADIUS_MSG，room服务器回应游戏/中心服务器房间信息
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	char szGameName[32];//游戏名称
	char szServerName[100];//服务器名称
	int	iBeginTime;	//开始时间
	int	iOpenTime;//开放持续时间
	int	iServerState;//服务器状态(0关闭，1正常，2维护)
	int	iServerRoomType;//游戏服务器用 （0混服，4新手，5初级，6中级，7高级，8vip）
	int iMaxPlayerNum;	//房间最大人数
	char cGameNum[20];	//游戏服务器的牌局编号(只有首次回应需要用到)
	uint iServerIP;		//游戏服务器的ip
	int iBaseServerNum;	//需要连接的底层服务器数量，后接iBaseServerNum个GameServerIPInfo
	int	iRoomNum;		//该游戏所有配置的房间数量，后接iRoonNum个GameOneRoomDetail（对游戏服务器）或GameOneRoomBrief（对中心服务器）
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
	ushort sServerPortBak;	//服务器端口备份
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
	int iRoomType;	//房间类型(4新手，5初级，6中级，7高级，8vip)
	int iRoomState;	//房间状态(0关闭，1正常，2维护)
	int iIfIPLimit;	//是否有ip限制
	int iSpeFlag1;	//特殊标记1
	int iSpeFlag2;	//特殊标记2
	int iSpeFlag3;	//特殊标记3
}GameOneRoomBriefDef;

typedef struct GameSysOnlineToCenterMsg     //GAME_SYS_ONLINE_TO_CENTER_MSG 游戏服务器和中心服务器同步在线人数
{
	MsgHeadDef msgHeadInfo;
	int iServerID;
	int iMaxNum;
	int	iNowPlayerNum;
}GameSysOnlineToCenterMsgDef;

typedef struct GameSysDetailToCenterMsg//GAME_SYS_DETAIL_TO_CENTER_MSG 游戏服务器信息发生变化时同步服务器详细信息至中心服务器
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

typedef struct GameServerEpollResetIPMsg //GAME_SERVER_EPOLL_RESET_IP_MSG 服务器线通信线程内重置ip信息
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
	int	iAuthenFlag;//登录标记用(第1位是否需要平台道具，第2位是否需要指定对应gameid道具（见extrainfo，最多5个），第3位是否需要获取指定道具，（见extrainfo，最多5个）,2和3位不能同时指定)，第四位是否获取好友列表,第5位是否仅是游戏跨天重新同步用户信息
	int	iExtraInfo[5];
} GameUserAuthenInfoReqDef;

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
const int g_iNoVacancies = 27;//好友房房间已满
const int g_iReadyTmOut = 28;//房间超时解散（30分钟内未开局）
const int g_iLandlordKick = 29;//好友房房主踢人
const int g_iRoomNoTable = 30;//服务器找不到可用桌子
const int g_iMatchError = 31;//比赛资格不足（不能决赛）
const int g_iTrialsError = 32;//比赛类型异常 (不能预赛)
const int g_iNoOperation = 33;//挂机踢出
const int g_iMatchTmOut = 34;//比赛未开放
const int g_iFDRoomCreateFail = 35;	//积分房创建失败
const int g_iFDRoomKickList = 36;	//积分房拉黑
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
	GameUserAuthenResPre	stResPre;
	char szUserName[20];
	char szNickName[60];
	int	iVipType;
	int	iGender;//性别0未知1女2男
	int	iExp;
	int	iHeadImg;//头像
	int	iFirstMoney;//一级货币福券
	long long iMoney;
	int	iDiamond;
	int	iIntegral;//奖券
	int	iTotalCharge;//总充值金额
	int	iSpreadID;//推广员id
	int	iTempChargeFirstMoney;//暂时记录的充值的福券（游戏内登录时会转入FirstMoney内，该值用于客户端内提示）
	long long	iTempChargeMoney;//暂时记录的充值的游戏币（游戏内登录时会转入money内，该值用于客户端内提示）
	int	iTempChargeDiamond;//暂时记录的充值的钻石（游戏内登录时会转入diamond内，该值用于客户端内提示）
	int	iSpeMark;//账号特殊标记
	int	iRegTime;//注册时间
	int	iRegType;//注册类型
	int	iAllGameTime;//总游戏时间
	int iAuthenResFlag;//第1位是否是跨天登录游戏,第2位是否是游戏内跨天重新同步用户信息
	//以下为对应的游戏信息
	int	iGameTime;//本游戏时间
	int	iWinNum;//总赢局数
	int	iLoseNum;//总输局数
	int	iAllNum;//总局数
	long long	iWinMoney;//总赢游戏币
	long long	iLoseMoney;//总输游戏币
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
	long long	iDayWinMoney;//今日游戏总赢游戏币
	long long	iDayLoseMoney;//今日游戏总输游戏币
	int	iDayIntegral;//今日在该游戏获得奖券数目
	int	iRecentPlayCnt;//近期总玩次数
	int	iRecentWinCnt;//近期赢次数
	int	iRecentLoseCnt;//近期输次数
	long long	iRecentWinMoney;//近期游戏赢分(不包括桌费)
	long long	iRecentLoseMoney;//近期游戏输分(不包括桌费)
	char szNickNameThird[64];	//第三方昵称
	int iHeadUrlLen;		//第三方头像url长度 动态长度
	//char szHeadUrl[];
	//int iAchieveLevel;	//成就等级
	//int iHisIntegral;	//历史总奖券
	//int iGEffectId[5];	//游戏特效id
	//int iGEffectTm[5];	//游戏特效时间
} GameUserAuthenInfoOKResDef;

//近期游戏数据，每日数据：日期1_局数1_赢局数1_输局数1_游戏赢分1_游戏输分1_桌费1_奖券1_耗时秒1
typedef struct RecentGameInfo
{
	int iDayFlag;
	int iAllNum;
	int iWinNum;
	int iLoseNum;
	long long iWinMoney;
	long long iLoseMoney;
	long long iTabelMoney;
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


typedef struct ServerLogLevelConfMsg	//SERVER_LOG_LEVEL_SYNC_MSG 各服务器同步日志等级信息
{
	MsgHeadDef msgHeadInfo;
	int iServerType;	//0game 1center 2radius 3redis 4bull 5room 6log 7chat 8other1 9other2 
	int	iServerId;
} ServerLogLevelConfMsgDef;


typedef struct UpdateUserPropReq			//GAME_UPDATE_USER_PROP_MSQ
{
	MsgHeadDef	 msgHeadInfo;
	int iGameID;
	int iUserID;
	int iPropID;
	int	iPropNum;		//道具数量增量，对于使用时效性道具时为可使用时间（即iUpdateType=3时值为lastTime）
	int iUpdateType;	//1获得 2使用 3使用有失效时间道具
}UpdateUserPropReqDef;

typedef struct GameGetUserGrowupEventReq		//GAME_GET_USER_GROWUP_EVENT_MSG  查询用户成长事件信息请求
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGrowEventId;
}GameGetUserGrowupEventReqDef;

typedef struct GameGetUserGrowupEventRes		//GAME_GET_USER_GROWUP_EVENT_MSG  查询用户成长事件信息回应
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGrowEventId;
	int iGrowNumInfo;
	int iGrowStrInfoLen;	//字符串内容长度
	//char szGrowStrInfo[];
}GameGetUserGrowupEventResDef;

typedef struct GameUpdateUserGrowupEventReq		//GAME_UPDATE_USER_GROWUP_EVENT_MSG 更新用户成长事件信息请求
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGrowEventId;
	int iGrowNumInfo;
	int iGrowStrInfoLen;	//字符串内容长度 最多100字节
	//char szGrowStrInfo[];
}GameUpdateUserGrowupEventReqDef;

const int g_iAccountTokenError = 1;//玩家游戏结算时token错误
const int g_iAccountMoneySysError = 2;//游戏币结算后不同步
const int g_iAccountDiamondSysError = 3;//星钻结算后不同步
const int g_iAccountForbidError = 4;//账号被禁
const int g_iAccountUserInfoError = 5;//数据读取/操作异常
const int g_iAccountIntegralSysError = 6;//奖券结算后不同步
const int g_iAccountSeverIDSysError = 7;//计费时发现和当前serverid不同步

typedef struct GameUserAccountReq   //GAME_USER_ACCOUNT_MSG	游戏服务器计费请求信息
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iGameID;
	int	iServerID;
	char szUserToken[32];
	long long llRDMoney;	//需要计费的游戏币
	int	iRDDiamond;	//需要计费的星钻
	int	iRDIntegral;//需要计费的奖券
	int	iRDGameTime;//游戏时长(秒)
	int	iRDExp;
	int	iIfQuit;	//是否退出游戏(0否，1退出，99退出且需要换服)
	int	iNowSpeMark;//当前特殊标记(和数据库不一致时，在计费回应内重新同步)
	long long llNowMoney;//当前计费后的游戏币（和数据库计费不一致时，要回复计费异常）
	int	iNowDiamond;	//当前计费后的星钻（和数据库计费不一致时，要回复计费异常）
	int	iReqFlag;	//计费请求标记位，第1位是否有游戏信息需要更新(有的话后接GameUserAccountGame)，第2位表示是否跳过游戏服与数据库玩家信息校验
}GameUserAccountReqDef;

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
	int	iFirstMoney;
	long long iMoney;
	int	iDiamond;
	int	iIntegral;//奖券
	int	iTotalCharge;//总充值金额
	int iIfConvertCharge;
	int iTempChargeFirstMoney;//暂时记录的充值的福券（游戏内登录时会转入FirstMoney内，该值用于客户端内提示）
	long long	iTempChargeMoney;//暂时记录的充值的游戏币（游戏内登录时会转入money内，该值用于客户端内提示）
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

typedef struct ServerLogLevelConfRes	//SERVER_LOG_LEVEL_SYNC_MSG 各服务器同步日志等级信息
{
	MsgHeadDef msgHeadInfo;
	int iServerType;	//0game 1center 2radius 3redis 4bull 5room 6log 7chat 8other1 9other2 
	int iLogLevel;
} ServerLogLevelConfResDef;

typedef struct GameSpecialAccontMsg//GAME_SPECIAL_ACCOUNT_MSG 游戏中对主要货币特殊计费
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iGameID;
	int	iServerID;
	char szUserToken[32];
	int	iMoneyType;//iMoneyType=1福券 2游戏币 3钻石 4奖券
	long long	iAddNum;
	long long iNowNum;
}GameSpecialAccontMsgDef;
//和redis交互消息

typedef struct GameRedisGetUserInfoReq//GAME_REDIS_GET_USER_INFO_MSG //游戏服务器通过redis获取用户登录信息回应
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iGameID;
	int	iServerID;
	int	iRoomType;
	int	iPlayType;
	int	iPlatform;
	char szIP[20];
	int	iReqFlag;//信息请求标记位，第1位是否需要近期同桌信息，第2位是否获取该游戏任务信息，第3位是否获取当前装扮信息，第4位是否需要获取通用活动任务信息,第5位是否是维护号
				//第6位是否需要本游戏的近期奖券任务记录 7是否需要玩家当日数据
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

typedef struct SRedisMailComMsg //SERVER_REDIS_MAIL_COM_MSG  //通用邮件信息
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iMailInfoLen;//低6位标题(utf_8)长度，再12位(utf_8)文本长度，再8位附件道具奖励信息
	int iAwardType;//奖励来源，无特殊约定写游戏gameid即可
	int iMailBtnInfoLen;//按钮信息长度(如"4_http:///...")
	//char cTitie[]
	//char cContent[]
	//char cPropInfo[]
	//char cBtnInfo[]
}SRedisMailComMsgDef;



typedef struct SRedisComStatMsg //SEVER_REDIS_COM_STAT_MSG 通用事件统计消息
{
	MsgHeadDef msgHeadInfo;
	int iEventID;     //低16位主事件id 高16位gameid
	int iSubEventID[20];//一次最多可以传20个子事件
	long long cllEventAddCnt[20];//服务器发送完统计数据后需清0重新开始
}SRedisComStatMsgDef;

typedef struct SRedisComStatWithValidMsg	//同SEVER_REDIS_COM_STAT_MSG 通用事件统计，但是需要校验有效期，有效期内不重复统计
{
	MsgHeadDef msgHeadInfo;
	int iEventID;     //低16位主事件id 高16位gameid
	int iSubEventID;	//子事件id
	int iAddEventCnt;
	int iUserID;
	int iValidType;		//有效时间
}SRedisComStatWithVaildMsgDef;

typedef struct SUsePropStatMsg //REDIS_USE_PROP_STAT_MSG 使用道具日志统计
{
	MsgHeadDef msgHeadInfo;
	int iRoomType;  //低位起前4位房间类型，再4位gameid
	int iPropID[20];//一次最多可以传20个道具，<0表示系统发放 >0表示玩家消耗
	int iPropNum[20];//服务器发送完统计数据后需清0重新开始
}SUsePropStatMsgDef;

typedef struct SRedisGetUserTaskRes //REDIS_RETURN_USER_TASK_INFO 返回用户在该游戏的任务信息
{
	MsgHeadDef  msgHeadInfo;
	int iUserID;
	int iTaskNum;
	//RdUserTaskInfoDef  task[iTaskNum];
}SRedisGetUserTaskResDef;

typedef struct RdUserTaskInfo
{
	int iID;
	int iTaskMainType;	//0日常任务 1周任务 2成长任务
	int iTaskSubType;	//0累计获胜局数,1连胜局数,2,完成局数,3每日登陆,4分享,5充值大于指定值,6一周登录指定天数,7完成所有周任务
	int iNeedComplete;	//任务需要完成数量
	int iHaveComplete;	//任务已完成数量
}RdUserTaskInfoDef;

typedef struct SRedisTaskGameResultReq //REDIS_TASK_GAME_RSULT_MSG  //根据游戏结果更新任务
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGameID;
	int iUpdateTaskNum;
	//SRdTaskGameResultReqExtraDef task[iUpdateTaskNum]
}SRedisTaskGameResultReqDef;

typedef struct SRdTaskGameResultReqExtra //REDIS_TASK_GAME_RSULT_MSG  //根据游戏结果更新任务
{
	int iTaskMainType;
	int iID;
	int iAddTaskComplete;
	int iIfCompTask;
}SRdTaskGameResultReqExtraDef;

typedef struct SRdGetDecoratePropInfo  //REDIS_RETURN_DECORATE_INFO 登录后返回用户装扮道具信息
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iHeadFrameID;//头像框道具id(0没有)
	int iHeadFrameTm;//头像框道具有效期
	int iClockPropID;//闹钟装扮id(0没有)
	int iClockPropTm;//闹钟道具有效期
	int iChatBubbleID;//聊天气泡道具id
	int iChatBubbleTm;//聊天气泡道具失效期
}SRdGetDecoratePropInfoDef;

typedef struct SRCommonSetByKey		//REDIS_COMMON_SET_BY_KEY		 通过 key value的方式设置数据
{
	MsgHeadDef  msgHeadInfo;
	char cKey[64];
	char cValue[512];
	int iExpire;
}SRCommonSetByKeyDef;

typedef struct SRCommonSetByHash		//REDIS_COMMON_SET_BY_HASH	通过 hash的方式设置数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cField[64];
	char cValue[512];
	int iExpire;
}SRCommonSetByHashDef;

typedef struct SRCommonGetByKeyReq		//REDIS_COMMON_GET_BY_KEY 通过 key value的方式获取数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
}SRCommonGetByKeyReqDef;

typedef struct SRCommonGetByKeyRes		//REDIS_COMMON_GET_BY_KEY 通过 key value的方式获取数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cValue[512];
}SRCommonGetByKeyResDef;

typedef struct SRCommonGetByHashReq		//REDIS_COMMON_GET_BY_HASH	 hash的方式获取数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cField[64];
}SRCommonGetByHashReqDef;

typedef struct SRCommonGetByHashRes		//REDIS_COMMON_GET_BY_HASH 通过 hash的方式获取数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cField[64];
	char cValue[512];
}SRCommonGetByHashResDef;

typedef struct SRCommonSetByHashMap		//REDIS_COMMON_SET_BY_HASH_MAP 通过 hash的方式设置多条数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	int iExpire;
	int iAllFeildNum;		//总共对应的  feild-value 的数量  feild 和 value的长度暂定32		
}SRCommonSetByHashMapDef;

typedef struct SRCommonGetByHashMapReq		//REDIS_COMMON_GET_BY_HASH_MAP 通过 hash的方式获取多条数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	int iAllFeildNum;
}SRCommonGetByHashMapReqDef;

typedef struct SRCommonGetByHashMapRes		//REDIS_COMMON_GET_BY_HASH_MAP	通过 hash的方式获取多条数据
{
	MsgHeadDef msgHeadInfo;
	int  iResult;			//iResult = 0  表示成功
	char cKey[64];
	int  iAllFeildNum;		//总共对应的  feild-value 的数量  feild 和 value的长度暂定32
}SRCommonGetByHashMapResDef;

typedef struct SRCommonHDelReq		//REDIS_COMMON_HASH_DEL 通过 hash 删除 field
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cField[64];
}SRCommonHDelReqDef;

typedef struct SRCommonZAddReq		// REDIS_COMMON_SORTED_SET_ZADD	有序集合 zadd
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cMember[32];
	int iScore;
	int iExpire;
}SRCommonZAddReqDef;

typedef struct SRCommonZIncrbyReq		//REDIS_COMMON_SORTED_SET_ZINCRBY	有序集合 ZINCRBY
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cMember[32];
	int iScore;
	int iExpire;
	char cTemp[64];
}SRCommonZIncrbyReqDef;

typedef struct SRCommonZIncrbyRes		// 添加有序集合数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cMember[32];
	int iScore;
	char cTemp[64];
}SRCommonZIncrbyResDef;

typedef struct SRCommonZRangeByScoreReq		// 获取指定集合数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	int iMax;
	int iMin;
	int iOffSet;
	int iCount;
	char cTemp[64];		//备用字段
}SRCommonZRangeByScoreReqDef;

typedef struct SRCommonZRangeByScoreRes		// REDIS_COMMON_SORTED_SET_ZRANGEBYSCORE 获取指定集合数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cTemp[64];		//备用字段
	int iAllNum;
}SRCommonZRangeByScoreResDef;


typedef struct SRCommonHGetAllReq		// REDIS_COMMON_HGETALL 哈希 hgetall
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cTemp[64];		//备用字段
}SRCommonHGetAllReqDef;

typedef struct SRCommonHGetAllRes		// 
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cTemp[64];		//备用字段
	int iAllNum;
}SRCommonHGetAllResDef;

typedef struct SRCommonLPushReq		// REDIS_COMMON_LPUSH	list lpush
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cValue[64];
	int iLen;		//列表长度 大于0 表示当前列表的最大长度 列表大于这个长度会调用 ltrim 移除多余的元素
	int iExpire;
}SRCommonLPushReqDef;

typedef struct SRCommonLRangeReq		// REDIS_COMMON_LRANGE   list lrange 
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	int iStart;
	int iEnd;
	char cTemp[64];		//备用字段
}SRCommonLRangeReqDef;

typedef struct SRCommonLRangeRes		// 获取指定集合数据
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cTemp[64];		//备用字段
	int iAllNum;		//  每个字段长度 32
}SRCommonLRangeResDef;

typedef struct SRCommonHIncrbyReq		// REDIS_COMMON_HINCRBY HINCRBY
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cField[32];
	char cTemp[64];
	int iNum;
	int iExpire;
}SRCommonHIncrbyReqDef;

typedef struct SRCommonHIncrbyRes		// 
{
	MsgHeadDef msgHeadInfo;
	char cKey[64];
	char cMember[32];
	char cTemp[64];
	int iNum;
}SRCommonHIncrbyResDef;

typedef struct SRCommonSetKeyExpireReq 	    //REDIS_COMMON_SET_KEY_EXPIRE	设置当前key的过期时间
{
	MsgHeadDef  msgHeadInfo;
	char cKey[64];
	int iExpire;
}SRCommonSetKeyExpireReqDef;

/***通用任务 begin***/
typedef struct RdComOneTaskInfo
{
	int iTaskInfo;//低16任务所属活动类型COMTASK，高16位taskID
	int iTaskType;//任务类型（见如下注释，沿用日常周长任务类型）0累计获胜局数,1连胜局数,2,完成局数,3每日登陆,4分享,5充值大于指定值,6一周登录指定天数,7完成所有周任务
	int iNowComp;//当前完成量
	int iAllNeedComp;//任务需要完成量
}RdComOneTaskInfoDef;

typedef struct RdComTaskSyncInfoDef  //RD_COM_TASK_SYNC_INFO redis同步给游戏服务器当前某用户需要同步的所有通用任务
{
	MsgHeadDef  msgHeadInfo;
	int iUserID;
	int iAllTaskNum;
	//后接iAllTaskNum个RdComOneTaskInfoDef
}RdComTaskSyncInfoDefDef;

typedef struct RdComOneTaskCompInfo
{
	int iTaskInfo;//低16任务所属活动类型COMTASK，高16位taskID
	int iCompAddNum;//任务完成增加量
}RdComOneTaskCompInfoDef;

typedef struct RdComTaskCompInfo //RD_COM_TASK_COMP_INFO 游戏服务器向redis同步某玩家完成某个通用任务的请求
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iAllTaskNum;
	//后接iAllTaskNum个RdComOneTaskCompInfoDef
}RdComTaskCompInfoDef;

/***通用任务 end ***/

//end:server和radius消息

//server和log消息
typedef struct GameRecordGameLogMsg   //GMAE_RECORD_GAME_LOG_MSG游戏服务器给log服务器发送统计每局游戏日志
{
	MsgHeadDef msgHeadInfo;
	char szGameNum[20];//游戏编号
	int	iGameID;
	int	iServerID;
	int	iGameTime;	//本局游戏时长(秒)
	char cPlayName[150];	//同桌用户名(用","隔开)
	int iPlayerNum;	//后接iPlayerNum个GameLogOnePlayerLog
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
	long long iBeginMoney;//开始游戏币
	long long	iEndMoney;//结束时游戏币
	long long	iWinMoney;//本局赢游戏币(不包括桌费)
	long long	iLoseMoeny;//本局输游戏币(不包括桌费)
	long long	iExtraAddMoney;//本局内非游戏获得的游戏币（如充值，抽奖等）
	long long	iExtraLoseMoney;//本局内非游戏减少的游戏币(如使用道具扣除等)
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

typedef  struct GameMainMonetaryLogMsg//GAME_MAIN_MONETARY_LOG_MSG	主要货币（游戏币/钻石/奖券）日志
{
	MsgHeadDef msgHeadInfo;
	int	iMoneyType;//1福券，2金币，3钻石 4奖券
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

typedef struct GameAchieveTaskLog //GAME_ACHIEVE_TASK_LOG_MSG 成就任务日志
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int iTaskType;
	int iCompleteNum;
	int iAchievePoint;
}GameAchieveTaskLogDef;

//底层服务器结构体

typedef struct ServerBaseInfo
{
	char strLogFileDirectory[20];	//本地的LOG文件路径
	char szConfFile[32];	//自己游戏的配置文件名称
	uint iServerIP;			//服务器自己的ip 
	int	 iServerPort;		//服务器所开的端口号
	int  iHeartBeatTime;	//心跳时间
	int  iServerID;			//服务器ID，一定要和数据库匹配，
	int iStartTimeStamp;	//服务器启动时间戳
	int iAppMTime;			//服务器文件时间戳

	GameOneRoomDetailDef stRoom[MAX_ROOM_NUM];	//所有房间的详细信息
	GameOneRoomOnlineDef stRoomOnline[MAX_ROOM_NUM];	//所有房间对应的当前在线

	int	iGameID;
	char cReplayGameNum[20];	//每次启动时保存游戏重放的最大的36进制的游戏编号
	char szGameName[20];
	char szServerName[100];
	int	iBeginTime;			//本服务器开始时间
	int	iLongTime;			//本服务器持续时间
	int	iServerState;		//服务器状态(0关闭，1正常，2维护)
	int	iServerRoomType;	//游戏服务器用 （0混服，4新手，5初级，6中级，7高级，8vip）
	int iMaxPlayerNum;		//本服务器支持的最大玩家数

	int	iAccountTimeGap;	//玩家上次计费距离本次计费超过多少秒就需要计费
	int	iAccountMoneyGap;	//玩家累积缓存内游戏币计费超过多少需要计费
	int	iAccountDiamondGap;	//玩家累积缓存内钻石计费超过多少需要计费
	int	iAccountIntegralGap;//玩家累积缓存内奖券计费超过多少需要计费
	int	iTestNet;			//记录网络延时log的值，是毫秒为单位

	int iLocalUpdateOnlineTm;	//本地测试用刷新人数时间间隔秒
	int iLocalUpdateOnlineCnt;
	int iLocalSysRoomInfoTm;	//同步房间信息时间间隔
	int iLocalSysRoomInfoCnt;
	int iLocalSysServerStatTm;	//同步房间统计数据时间间隔
	int iLocalSysServerStatCnt;
	char szLocalTestIP[32];			//本地测试用，设置自己服务器的ip和端口，方便发送到中心服务器
	int iLocalTestPort;				//本地测试用，自己服务器的端口
	char szLocalTestRoomIP[32];		//本地测试用，连接的本地room服务器的ip
	int iLocalTestRoomPort;			//本地测试用，连接的本地room服务器的端口
	char szLocalTestCenterIP[32];	//本地测试用，连接的本地中心服务器的ip
	int iLocalTestCenterPort;		//本地测试用，连接的本地中心服务器的端口
	char szLocalTestRadiusIP[32];	//本地测试用，连接的本地Radius的服务器ip
	int iLocalTestRadiusPort;		//本地测试用，连接的本地Radius服务器的端口
	char szLocalTestLogIP[32];		//本地测试用，连接的本地Log的服务器ip
	int iLocalTesttLogPort;			//本地测试用，连接的本地Log服务器的端口
	char szLocalTestRedisIP[32];	//本地测试用，连接的本地redis的服务器ip
	int iLocalTestRedisPort;		//本地测试用，连接的本地redis服务器的端口

	uint iInnerIP;
	short sInnerPort;
}ServerBaseInfoDef;

typedef struct PlayerInfoResNor		//玩家信息
{
	int  iUserID;			//用户ID
	char szNickName[64];	//昵称
	long long iMoney;
	char cTableNum;		//玩家所在桌号,没在桌上为0
	char cTableNumExtra;//玩家在桌上的位置，从最左边开始逆时针为0-3，
	char cIfReady;		//玩家是否ReadyOK
	char cLoginType;	//登录类型,电脑0 手机1
	char cGender;		//性别0未知1女2男
	char cVipType;
	char cLoginFlag1;	//客户端登录携带标记1
	char cLoginFlag2;	//客户端登录携带标记2

	int	iAchieveLv;  //成就等级
	int	iHeadImg;
	int	iExp;		//平台经验值
	int	iGameExp;	//游戏经验值
	int	iWinNum;	//总赢局数
	int	iLoseNum;	//总输局数
	int	iAllNum;	//总局数
	int	iBuffA0;	//游戏控制值A0（赋值型）
	int	iBuffA1;	//游戏控制值A1（赋值型）
	int	iBuffA2;	//游戏控制值A2（赋值型）
	int	iBuffA3;	//游戏控制值A3（赋值型）
	int	iBuffA4;	//游戏控制值A4（赋值型）
	int	iBuffB0;	//游戏控制值B0（增加型）
	int	iBuffB1;	//游戏控制值B1（增加型）
	int	iBuffB2;	//游戏控制值B2（增加型）
	int	iBuffB3;	//游戏控制值B3（增加型）
	int	iBuffB4;	//游戏控制值B4（增加型）
	int	iContinueWin;	//连赢次数
	int	iContinueLose;	//连输次数

	int iHeadFrameID;	//头像框道具id(0没有)
	int iClockPropID;	//闹钟装扮id(0没有)
	int iChatBubbleID;	//聊天气泡道具id(0没有)
}PlayerInfoResNorDef;


//底层服务器结构体_end
//3:和客户端玩家消息（这些消息类型和gamebase内保持一致）
#define AUTHEN_REQ_MSG				0xE1	//玩家登陆验证请求
#define	CLIENT_GET_CSERVER_LIST_MSG	0xF0	//客户端获得中心服务器的服务器列表消息（仅客户端和中心服务器使用）
#define SITDOWN_REQ_MSG				0xE2	//玩家“坐下”请求
#define READY_REQ_MSG				0xE3	//玩家按下“开始”请求，超时自动发
#define ESCAPE_REQ_MSG				0xE4	//玩家“逃跑”请求
#define	LEAVE_REQ_MSG				0xE6	//玩离开游戏请求
#define	URGR_CARD_MSG				0xF8	//玩家催牌消息 
#define	FACE_GROUND_AUTHEN_MSG		0xE7	//切后台登陆验证响应，二次确认
#define	AUTHEN_RES_MSG				0xE8	//登陆验证响应
#define	SITDOWN_RES_MSG				0xE9	//玩家“坐下”请求回应
#define	AGAIN_LOGIN_RES_MSG			0xEB	//发送断线重入响应
#define	KICK_OUT_SERVER_MSG			0xEC	//踢人
#define	GAME_RESULT_SERVER_MSG		0xED	//游戏结果
#define	GAME_BULL_NOTICE_MSG		0xEE	//游戏公告通知
#define PLAYER_INFO_NOTICE_MSG		0xD0	//玩家信息通知，有玩家加入游戏或者离开时发送
#define READY_NOTICE_MSG			0xD1	//玩家“开始”通知
#define ESCAPE_NOTICE_MSG			0xD2	//玩家“逃跑”通知
#define	WAIT_LOGIN_AGAIN_NOTICE_MSG	0xD3	//玩家掉线等待通知
#define	LOGIN_AGAIN_NOTICE_MSG		0xD4	//玩家掉线再次进入通知
#define	USER_PROPINFO_RES_MSG		0x9E	//玩家道具信息回应
#define	UPDATE_USER_PROP_REQ_MSQ	0xB1	//玩家获得道具请求（或者使用）
#define USE_INTERACT_PROP			0XC5    //魔法表情互动道具使用
#define SYN_USER_DAYWEEK_TASK		0XC6    //同步日常任务进度通知
#define	TEST_NET_MSG				0xF1	//网络测速消息

#define SYS_CLIENT_USER_MAIN_INFO	0x51	//同步用户主要信息至客户端
#define LISTEN_INVITER_ROOMNO_MSG	0x52	//客户端通知服务器监听邀请者房号
#define SYS_CHANGE_SERVER_MSG	    0x53	//客户端通知服务器需要换服

#define	USER_CHARGE_AND_REFRESH_REQ_MSG		0x70 //玩家游戏中支付和刷新结果请求

typedef struct AuthenReq	//AUTHEN_REQ_MSG  玩家登陆验证请求
{
	MsgHeadDef msgHeadInfo;
	int  iUserID;	//用户ID
	char szUserToken[32];	//用户TOKEN
	char szIP[20];
	char szMac[40];
	char cEnterRoomType;	//进入的房间类型
	char cLoginType;		//平台类型0Android，1iPhone，2ipad，3微小，4抖小
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

typedef struct CenterServerRes	//CLIENT_GET_CSERVER_LIST_MSG	 客户端获得中心服务器的服务器列表消息（仅客户端和中心服务器使用）
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

typedef struct SitDownReq			//SITDOWN_REQ_MSG		玩家“坐下”请求
{
	MsgHeadDef msgHeadInfo;
	int	iTableNum;		//桌号
	int	iBindUserID;//绑定账号ID(组队时候用)
	char cTableNumExtra;	//桌位置
	char cFlag1;
	char cFlag2;
	char cFlag3;
}SitDownReqDef;

typedef struct ReadyReq			//READY_REQ_MSG 玩家按下“开始”请求，超时自动发
{
	MsgHeadDef msgHeadInfo;
	char szEmpty[4];
}ReadyReqDef;

typedef struct EscapeReq   //ESCAPE_REQ_MSG  玩家“逃跑”请求
{
	MsgHeadDef msgHeadInfo;
	char cType;//10退出 0准备 1换桌 2清空玩法换桌
}EscapeReqDef;

typedef struct LeaveGame //LEAVE_REQ_MSG 玩离开游戏请求
{
	MsgHeadDef msgHeadInfo;
	char cType;//0在游戏中主动退出的，1大厅关闭后退出游戏的。
}LeaveGameDef;

typedef struct UrgeCard  //URGR_CARD_MSG 玩家催牌消息
{
	MsgHeadDef msgHeadInfo;
	char cWordID;			//催牌话语的ID
	char cTableNumExtra;	//催牌人的绝对位置
}UrgeCardDef;

typedef struct AuthenRes     //AUTHEN_RES_MSG 登陆验证响应
{
	MsgHeadDef msgHeadInfo;
	char cResult;//见g_iTokenError等定义
	int  iLockServerID;  //卡哪个游戏的serverid
	int  iLockGameAndRoom;//所卡房间的gameiD和房间类型（低16位gameid，高16位房间类型）
	int  iRoomType;   //房间类型
	int  iServerTime; //服务器的当前时间
	int  iDiamod;//钻石
	int iFirstMoney;//一级货币福券
	int  iTotalCharge;//总充值金额
	int iTempChargeFirstMoney;//当前充值补到账福券(radius已计费，给客户端提示用)
	long long  iTempChargeMoney;//当前充值补到账游戏币(radius已计费，给客户端提示用)
	int  iTempChargeDiamond;//当前充值补到账钻石(radius已计费，给客户端提示用)
	int  iPlayType;//游戏玩法（登录时传递，每个游戏根据需要通过位移运算自行设定）
	long long iBasePoint;         //玩家选择进入房间底分
	PlayerInfoResNorDef stPlayerInfo;	//自己个人信息
	//int iIntegralNum;   //奖券数量
}AuthenResDef;

typedef struct FaceGroundAuthen   //FACE_GROUND_AUTHEN_MSG
{
	MsgHeadDef msgHeadInfo;
	//每个游戏自己自己填充字段
}FaceGroundAuthenDef;

typedef struct SitDownRes  //SITDOWN_RES_MSG	玩家“坐下”请求回应
{
	MsgHeadDef msgHeadInfo;
	int iTableNum;	//桌号	
	char cResult;	//0失败，1成功
	char cTableNumExtra;//位置
}SitDownResDef;

typedef struct AgainLoginResBase	//AGAIN_LOGIN_RES_MSG	发送断线重入响应
{
	MsgHeadDef msgHeadInfo;
	int cTableNum;			//桌号
	char cTableNumExtra;	//位置
	char cPlayerNum;		//后续接PlayerInfoResExtraDef数量
	char bOffLine[MAX_PLAYER_NUM];
	char cGameNum[20];	//游戏编号
	int  iFirstMoney; //一级货币福券
	int  iDiamod;		//钻石
	int  iTotalCharge;	//总充值金额
	int  iRoomType;		//房间类型
	int  iPlayType;		//游戏玩法（登录时传递，每个游戏根据需要通过位移运算自行设定）
	int  iServerTime;	//服务器的当前时间
	int  iStructSize;	//实际每个游戏掉线重入结构体的长度（不包括PlayerInfoResExtraDef信息）
}AgainLoginResBaseDef;

typedef struct KickOutServer		//KICK_OUT_SERVER_MSG 踢人
{
	MsgHeadDef msgHeadInfo;
	int  iKickUserID;
	char cKickType;	//见和登陆返回错误那些提示编号g_iTokenError保持一致
	char cClueType;	//提示类型（0仅提示，1退出房间，2退出游戏返回大厅）
	char cKickSubType;	//游戏单独处理踢出类型（cKickType=KICK_BY_GAME_LOGIC时）
	unsigned char cFlag;	//>0踢出提示内容由服务器指定
	//char cKickMsg[cFlag];
}KickOutServerDef;

typedef struct GameResultServerBase		//GAME_RESULT_SERVER_MSG 游戏结果
{
	MsgHeadDef msgHeadInfo;
	int	iPlayerNum;//玩家数量
	int iGameRtInfoSize;//游戏自己的结果部分信息总长度（如每个玩家一个结算结构体，则iGameRtInfoSize=iPlayerNum*单个玩家结算结构体长度）
	//后接iPlayerNum个GameResultOneBaseInfo
	//后接对应的iGameRtInfoSize长度的游戏信息
}GameResultServerBaseDef;

typedef struct GameResultOneBaseInfo
{
	int iMoneyResult;	//游戏结果（正/负/0值，不包括桌费和额外扣除/补偿）
	long long iTableMoney;	//需要扣除的桌费（正值）
	long long iExtraAddMoney;	//额外补偿的游戏币（正值）
	long long iExtraLoseMoney;//额外被扣除的游戏币（正值）
	int iMoneyRtInfo;	//低8位游戏结果信息类型，再8位游戏币补偿类型，再8位游戏币扣除类型（每个游戏自行解析）
	long long iNowMoney;//当前游戏币=money+iMoneyResult-iTableMoney+iExtraAddMoney-iExtraLoseMoney
	int iNowExp;	//当前平台经验值
	int iNowGameExp;//当前游戏经验值
}GameResultOneBaseInfoDef;

typedef struct BullNotice//GAME_BULL_NOTICE_MSG 游戏公告通知
{
	MsgHeadDef msgHeadInfo;
	char cType;		//公告类型
	char cFlag1;	//备用标记1
	char cFlag2;	//备用标记2
	char cFlag3;	//备用标记3
	int iBullLen;	//后接公告长度
} BullNoticeDef;

typedef struct PlayerInfoNotice		//PLAYER_INFO_NOTICE_MSG	玩家信息通知,玩家在桌上游戏时每局结束后发送，有玩家加入游戏或者离开时发送
{
	MsgHeadDef msgHeadInfo;
	char cPlayerNum;
	char cFlag1;	//备用标记1  低四位表示配桌成功后桌上玩家总数 第5位表示结构体后面是否追加所有玩家成就等级信息
	char cFlag2;	//备用标记2
	char cFlag3;	//备用标记3
	// 后接cPlayerNum个PlayerInfoResExtraDef
}PlayerInfoNoticeDef;

typedef struct ReadyNotice		//READY_NOTICE_MSG 玩家“开始”通知
{
	MsgHeadDef msgHeadInfo;
	char cTableNumExtra;	//该玩家在桌上的位置
	char cFlag1;	//备用标记1
	char cFlag2;	//备用标记2
	char cFlag3;	//备用标记3
}ReadyNoticeDef;

typedef struct EscapeNotice		//ESCAPE_NOTICE_MSG 玩家“逃跑”通知,玩家收到判断如果是自己，那么就返回大厅,显示等待用户列表，收到用户列表后才刷新进入
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	char cTableNumExtra;
	char cType;		//0开始游戏前正常退出返回上级界面，1结束时换桌离开；2游戏中强行离开
	char cFlag1;	//备用标记1
	char cFlag2;	//备用标记2
}EscapeNoticeDef;

typedef struct WaitLoginAgainNotice	//WAIT_LOGIN_AGAIN_NOTICE_MSG	玩家掉线等待通知
{
	MsgHeadDef msgHeadInfo;
	char cTableNumExtra;
	char cDisconnectType;	//掉线类型  0没有掉线，1在游戏中主动离开，2多次不出牌被踢，3超时被踢掉，4 Socket断开（例如拔掉网线）
}WaitLoginAgianNoticeDef;

typedef struct LoginAgainNotice  //LOGIN_AGAIN_NOTICE_MSG 玩家掉线再次进入通知
{
	MsgHeadDef msgHeadInfo;
	char cTableNumExtra;
}LoginAgainNoticeDef;

typedef struct UserPropInfoRes	//USER_PROPINFO_RES_MSG	玩家道具信息回应
{
	MsgHeadDef msgHeadInfo;
	int iNum;
	int iUserID;
	//后接iNum个GameUserOnePropDef
}UserPropInfoResDef;

typedef struct ClientUpdateUserProp  //UPDATE_USER_PROP_REQ_MSQ 通知客户端玩家获得的道具
{
	MsgHeadDef msgHeadInfo;
	int iType;	//1是获得，2是使用
	GameUserOnePropDef propInfo;	//当前道具信息
}ClientUpdateUserPropDef;

typedef struct UseInteractPropReq //USE_INTERACT_PROP 使用互动表情道具请求
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iPropID;
	int iUseToUserID;
	int iUseType;	//1使用现有数量,2使用钻石
}UseInteractPropReqDef;

typedef struct UseInteractPropRes //USE_INTERACT_PROP 使用互动表情道具回应
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iErrRt;	//0成功，1道具数量不足 2钻石不够发
	int iPropID;
	int iUseToUserID;
	int iUseType;	//1使用现有数量 2使用钻石
	int iDecNum;	//扣除对应使用类型的数量
}UseInteractPropResDef;

typedef struct SysUserDayWeekTask        //SYN_USER_DAYWEEK_TASK		0XC6    //同步日常任务进度通知
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGameID;
	int iUpdateTaskNum;   //需要刷新的任务数量
	//后接iUpdateTaskNum个SysOneDayWeekTaskDef
}SysUserDayWeekTaskDefs;

typedef struct SysOneDayWeekTask
{
	int iTaskMainType;	//0日常任务 1周任务 2成长任务
	int iID;
	int iAddTaskComplete;	//本次需要增加的任务已完成数量
	int iIfCompTask;		//任务是否完成
}SysOneDayWeekTaskDef;

typedef struct TestNetMsg //TEST_NET_MSG 网络测速消息
{
	MsgHeadDef msgHeadInfo;
	char cType;//0为客户端请求包，2为客户端确认,
	char cTableNumExtra;
	int	iDelayTime;
}TestNetMsgDef;

typedef struct SysClientUserMainInfo //SYS_CLIENT_USER_MAIN_INFO 同步用户主要信息至客户端
{
	MsgHeadDef msgHeadInfo;
	int	iUserID;
	int	iHeadImg;//头像
	long long iMoney;
	//iUserID是自己的才会接SysClientUserSelfMainInfoDef
}SysClientUserMainInfoDef;

typedef struct SysClientUserSelfMainInfo
{
	int iFirstMoney;
	int	iDiamond;		//钻石
	int	iIntegral;		//奖券
	int	iTotalCharge;	//总充值金额
	int iIfConvertCharge;
	int iTempChargeFirstMoney;
	long long iTempChargeMoney;	//暂时记录的充值的游戏币（游戏内登录时会转入money内，该值用于客户端内提示）
	int	iTempChargeDiamond;	//暂时记录的充值的钻石（游戏内登录时会转入diamond内，该值用于客户端内提示）
}SysClientUserSelfMainInfoDef;

/**************新增配桌等信息******************/
typedef struct RoomAllRoomInfoMsg		//NEW_CENTER_ALL_ROOM_INFO_MSG 0x61 //同步其他所有房间信息
{
	MsgHeadDef msgHeadInfo;
	int iRoomNum;	//后接iRoomNum个OneRoomInfoDef
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
	int iReqAssignCnt;		//用0表示今日首次入座,其他>=1
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

typedef struct ClientChangeServer   //SYS_CHANGE_SERVER_MSG  0x53	//客户端通知服务器需要换服
{
	MsgHeadDef msgHeadInfo;
	int iUserID;//请求换服的用户id
	int iNewServerID;//需要换到serverid
	int iRoomNo;//携带的房号信息
}ClientChangeServerDef;

typedef struct UserChargeAndRefreshReq   //USER_CHARGE_AND_REFRESH_REQ_MSG  玩家游戏中支付和刷新结果请求
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iFlag;
}UserChargeAndRefreshReqDef;

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

typedef struct NCenterUserAssignErr  //NEW_CNNTER_USER_ASSIGN_ERR
{
	MsgHeadDef  msgHeadInfo;
	int iUserID[10];  //配桌异常的玩家
	int iNVTableID;
}NCenterUserAssignErrDef;

typedef struct NewCenterFreeVTableMsg//NEW_CENTER_FREE_VTABLE_MSG	游戏服务器通知中心服务器回收虚拟桌号
{
	MsgHeadDef  msgHeadInfo;
	int iVTableID;
}NewCenterFreeVTableMsgDef;


/**************新增配桌等信息 end******************/
typedef struct RDNCGetUserRoomRes// REDIS_RETURN_USER_ROOM_MSG  新配桌流程中获取用户进入房间配桌信息
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iLastPlayerIndex;//近期同桌玩家索引
	int iLatestPlayUser[10];//近期玩家，最多保留10个
}RDNCGetUserRoomResDef;

typedef struct RDNCSetLatestUser//REDID_NCENTER_SET_LATEST_USER_MSG 新配桌流程中配桌成功后记录同桌玩家
{
	MsgHeadDef msgHeadInfo;
	int iJudgeGameID;
	int iLatestIndex[10];
	int iUserID[10];
}RDNCSetLatestUserDef;

typedef struct GamePlatformStatInfo
{
	int iActiveCnt;//活跃人数
	int iNewAddCnt;//新增人数
	int iNewAddContinue2;//新增次留人数
	int iNewAddContinue3;//新增3留人数
	int iNewAddContinue7;//新增7留人数
	int iNewAddContinue15;//新增15留人数
	int iNewAddContinue30;//新增30留人数
	int iAllGameCnt;//总局数
}GamePlatformStatInfo;

typedef struct GameRoomStatInfo
{
	//游戏通用统计
	int iBrokenCnt;	//破产次数
	int iKickCnt;	//低于最低限制被踢出次数
	int iReliefCnt;	//救济次数
	long long iReliefNum;	//救济总额
	int iVacCnt;	//充值次数
	long long iVacNum;	//充值总额
	int iWinCnt;	//玩家总赢局数
	long long llAllWin;		//玩家总赢分
	long long llAllLose;	//玩家总输分
	int iLoseCnt;			//玩家总输局数
	int iPingCnt;			//玩家平局局数
	long long iTableMoney;//理论总桌费
	int iAIPlayCnt;         //与ai同桌次数

	//陪打ai
	int iAIPlayWin;					//AI总赢次数
	int iAIPlayLose;				//AI总输次数
	long long llAIPlayWinMoney;		//AI总赢分	
	long long llAIPlayLoseMoney;	//AI总输分
	int iAIPlayPing;				//AI平次数

	//控制ai
	int iAIControlWinCnt;		//控制ai赢次数
	int iAIControlLoseCnt;		//控制ai输次数
	long long llAIControlWin;	//控制ai赢分	
	long long llAIControlLose;	//控制ai输分

	long long llExtraSendMoney;	//额外发放游戏币	
	long long llSendMoney;      //总发放游戏币
	int iSendDiamond;			//总发放钻石数
	int iUseDiamond;			//总消耗钻石数
	long long llUseMoney;       //总消耗游戏币
	long long llExtraUseMoney;  //额外总消耗游戏币
	int iSendIntegral;			//发放奖券
	int iUseIntegral;			//消耗奖券
}GameRoomStatInfoDef;

typedef struct RdGameBaseStatInfo// REDIS_GAME_BASE_STAT_INFO_MSG 游戏公用基础统计信息
{
	MsgHeadDef msgHeadInfo;
	int iJudgeGameID;
	int iTmFlag;
	int iNewResultWin[10];//新增用户游戏赢玩家数量，依次是第1-9局，10局以上
	int iNewResultLose[10];//新增用户游戏输玩家数量，依次是第1-9局，10局以上
	GamePlatformStatInfo stPlatStatInfo[3];	//2个平台对应统计信息，0Android，1ios，微信小游戏
	GameRoomStatInfoDef stRoomStat[5];	//4,5,6,7,8这五种房间对应的详细信息
}RdGameBaseStatInfoDef;

typedef struct NCAssignStaInfo //配桌统计信息
{
	int iAbnormalSitReq;	//异常发送入座请求次数
	int iAbnormalSitUser;	//异常发送入座请求玩家数
	int iSendReq2Num;		//发送2次请求入座的玩家数
	int iSendReq3Num;		//发送3次请求入座的玩家数
	int iSendReqMoreNum;	//发送4次及以上入座的玩家数
	int iNeedChangeNum;		//需要换服次数
	int iSucTableOkNum;		//成功开桌次数
	int iStartGameOk[5];	//5种房间开局次数
	int iRoomWaitCnt[5];	//5种房间等待配桌人数
	int iHighUserCnt;		//等待配桌或已配桌高几率人数
	int iMidUserCnt;		//等待配桌或已配桌中几率人数
	int iLowUserCnt;		//等待配桌或已配桌低几率人数
	int iRightSecTableCnt;	//同几率开桌次数
	int iRightRoomTableCnt;	//同房间开桌次数
	int iUserRateCnt[11];	//用户首次进入时落在各档几率的人数
	int iTmFlag;
}NCAssignStaInfoDef;

typedef struct RDNCRecordStatInfo	//REDIS_RECORD_NCENTER_STAT_INFO  0x2E //记录新中心服务器统计信息
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	NCAssignStaInfoDef stAssignStaInfo;
}RDNCRecordStatInfoDef;

typedef struct RDNCAssignTmStat//REDIS_ASSIGN_NCENTER_STAT_INFO
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	int iSubNum;    //后接iSubNum个数据块
	//int iExtraType;
	//int iAllCnt;      //记录次数
	//int iAverageTm;   //上次统计到这次之间iAllCnt平均时长
}RDNCAssignTmStatDef;


typedef struct RdGameCheckRoomNo //RD_GAME_CHECK_ROOMNO_MSG 检测房号是否可用
{
	MsgHeadDef msgHeadInfo;
	int iRoomUserID;//房主
	int iRoomNum;
	int iServerID;
	int iGameID;
}RdGameCheckRoomNoDef;

typedef struct RdGameCheckRoomNoRes //RD_GAME_CHECK_ROOMNO_MSG 检测房号是否可用
{
	MsgHeadDef msgHeadInfo;
	int iRoomUserID;//房主
	int iRoomNum;
	int iRt;//0房号已失效，1房号正常可用
}RdGameCheckRoomNoResDef;

typedef struct RdGameUpdateRoomNumStatus //RD_GAME_UPDATE_ROOMNO_STATUS 更新房号最后状态
{
	MsgHeadDef msgHeadInfo;
	int iRoomNum;
	int iIfInUse;//是否在使用（=1开局/结算表示在使用，=0房主离开表示该房号失效）
	int iGameID;
}RdGameUpdateRoomNumStatusDef;


typedef struct RdUpdateUserAchieveInfoRes   //RD_UPDATE_USER_ACHIEVE_INFO_MSG
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iTaskType;
	int iCompleteNum;
	int iAchievePoint;
}RdUpdateUserAchieveInfoResDef;

typedef struct RdGetRankConfInfoReq	//RD_GET_RANK_CONF_INFO_MSG
{
	MsgHeadDef msgHeadInfo;
	int iRankId;	//排行id 1世界排行榜
}RdGetRankConfInfoReqDef;

typedef struct RdGetRankConfInfoRes	//RD_GET_RANK_CONF_INFO_MSG
{
	MsgHeadDef msgHeadInfo;
	int iRankId;
	int iBeginTm;
	int iEndTm;
	int iJifenLen;		//积分配置长度
	int iExtraLen;		//额外配置长度
	//szJifenConf[]		//积分配置内容
	//char szBuffExtra[];	//额外配置内容
}RdGetRankConfInfoResDef;

typedef struct RdGetUserRankInfoReq	//RD_GET_USER_RANK_INFO_MSG
{
	MsgHeadDef msgHeadInfo;
	int iUserId;
	int iRankId;	//排行id
}RdGetUserRankInfoReqDef;

typedef struct RdGetUserRankInfoRes	//RD_GET_USER_RANK_INFO_MSG
{
	MsgHeadDef msgHeadInfo;
	int iUserId;
	int iRankId;	//排行id
	int iJifen;		//当前期排行积分 客户端需要显示
	int iExtraLen;	//额外内容长度 动态获取
	//szExtra[];
}RdGetUserRankInfoResDef;

typedef struct RdUpdateUserRankInfoReq	//RD_UPDATE_USER_RANK_INFO_MSG
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	char szNickName[60];
	int iHeadImage;
	int iHeadFrameId;
	int iRankID;	//排行id
	int iAddJifen;	//增加的积分 增量式
	int iExtraLen;	//预留额外信息
	//char szExtra[];
}RdUpdateUserRankInfoReqDef;

typedef struct RdUpdateUserRankInfoRes	//RD_UPDATE_USER_RANK_INFO_MSG	跨期数了需要重置积分 才会有回应
{
	MsgHeadDef msgHeadInfo;
	int iUserId;
	int iRankId;	//排行id
	int iJifen;		//当前期排行积分
}RdUpdateUserRankInfoResDef;

typedef struct RdGetVirtualAIInfoReq //RD_GET_VIRTUAL_AI_INFO_MSG 通过redis获取虚拟ai的信息请求
{
	MsgHeadDef msgHeadInfo;
	int iVirtualID;
	int iVTableID;
	int iGameID;
	int iServerType;
	int iServerID;		//当前消息需要发往哪个游戏服
	int iNeedNum;
}RdGetVirtualAIInfoReqDef;

typedef struct RdCtrlAIStatusMsg  //RD_USE_CTRL_AI_MSG
{
	MsgHeadDef msgHeadInfo;
	int iVirtualID;
	int iGameID;
}RdCtrlAIStatusMsgDef;

typedef struct RdGetVirtualAIRtMsg  //RD_GET_VIRTUAL_AI_RES_MSG
{
	MsgHeadDef msgHeadInfo;
	int iVTableID;//将请求的转发回来即可
	int iServerID;		//当前消息需要发往哪个游戏服
	int iRetNum;    //返回ai信息的数量

	//每个游戏自己需要的数据结构 iRetNum个
	//int game%d_iVirtuanlID;
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
	//game%d_strBuffer0   //最大90字节
	//int istrBuffer2Len;
	//game%d_strBuffer1   //最大90字节
}RdGetVirtualAIRtMsgDef;

typedef struct RdGetVirtualAIInfoRes //RD_GET_VIRTUAL_AI_INFO_MSG 通过redis获取虚拟ai的信息回应
{
	MsgHeadDef msgHeadInfo;
	int iVirtualID;
	int iVTableID;//将请求的转发回来即可
	int iPlayCnt;//总玩局数
	int iWinCnt;//总赢局数
	int iDurakCnt;//durak局数
	int iAchieveLevel;//成就等级
	int iHeadFrame;//头像框
	int iServerID;		//当前消息需要发往哪个游戏服
	int iExp;

	//每个游戏自己需要的数据结构
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
	//game%d_strBuffer0  //最大90字节
	//int istrBuffer2Len;
	//game%d_strBuffer1  //最大90字节
}RdGetVirtualAIInfoResDef;



typedef struct RdSetVirtualAIRtMsg //RD_SET_VIRTUAL_AI_RT_MSG 结算时对虚拟AI的总局数等计费
{
	MsgHeadDef msgHeadInfo;
	int iVirtualID;
	int iAddPlayCnt;//增加总玩局数
	int iAddWinCnt;//增加总赢局数
	int iAddDurakCnt;//增加durak局数

	//每个游戏自己需要的数据结构
	int iGameID;
	int iExp;
	int iBufferA0;
	int iBufferA1;
	int iBufferA2;
	int iBufferA3;
	int iBufferA4;
	int iBufferB0;
	int iBufferB1;
	int iBufferB2;
	int iBufferB3;
	int iBufferB4;
	int isBuffer1Len;
	int isBuffer2Len;
	//game%d_strBuffer0  //最大90字节
	//game%d_strBuffer1  //最大90字节
}RdSetVirtualAIRtMsgDef;

typedef struct RdGetIntegralTaskMsg   //RD_INTEGRAL_TASK_HIS_RES
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	int iUserID;
	int iRecentNum;
	//后接iRecentNum个任务id
}RdGetIntegralTaskMsgDef;


typedef struct RdUserDayInfoReqMsg  //RD_USER_DAY_INFO_MSG   玩家当日数据刷新
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	int iUserID;
	int iAddTaskIntegral;  //增量
	int iAddGameCnt;
}RdUserDayInfoMsgReqDef;

typedef struct RdUserDayInfoResMsg  //RD_USER_DAY_INFO_MSG  玩家当日数据回应
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iDayTaskIntegral;  //当日任务获取奖券数量
}RdUserDayInfoResMsgDef;

typedef struct TogetherUser
{
	int iUserID;
	char szNickName[64];
	int	iExp;
	int	iHeadImg;//头像
	int iHeadFrameId;        //头像框id
}TogetherUserDef;

typedef struct RdGameTogetherUserReq  //RD_GAME_TOGETHER_USER_MSG 0x31F	//玩家同桌玩家信息
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iBasePoint;
	int iGameID;
	int iExtraFlag;  //预留
	int iUserNum;
	//后接iUserNum个玩家信息TogetherUserDef
}RdGameTogetherUserReqDef;

typedef struct RdGameRoomTaskInfoReq  //RD_GAME_ROOM_TASK_INFO_MSG  0x300        //获取房间任务信息
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGameID;
	int iRoomType;
	int iReqType;    //0表示获取 1表示更新（请求更新不作回应）
}RdGetRoomTaskInfoReqDef;

typedef struct RdGameRoomTaskInfoRes
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGameID;
	int iRoomType;          //0表示没有房间任务，玩家在游戏服中可能会更换房间类型，所以这里需要返回一次
	int iCompNum;           //完成数量
	int iNeedNum;           //需要数量
	int iAwardID;
	int iAwardNum;
}RdGameRoomTaskInfoResDef;

typedef struct GameGetIntegralTaskInfoReq		//RD_GET_INTEGRAL_TASK_CONF 获取奖券任务信息
{
	MsgHeadDef msgHeadInfo;
	int iGameID;
	int iReqFlag;    //第1位获取表概率配置 第2位获取任务池配置
}GameGetIntegralTaskInfoReqDef;

typedef struct GameGetIntegralTaskInfoRes
{
	MsgHeadDef msgHeadInfo;
	int iIfLastMsg;		//是否最后一次补发消息 消息可能过长 分多次补发 标记最后一次补发
	int iTableSize;     //表概率配置数量
	//int iTableGrade;    //表等级
	//int iMinIntegral;   //最低历史奖券
	//int iMaxIntegral;   //最高历史奖券
	//int iWeight1;       //奖励权重1
	//int iWeight2;       //奖励权重2
	//int iWeight3;       //奖励权重3
	//int iWeight4;       //奖励权重4
	//int iWeight5;       //奖励权重5
	//int iWeight6;       //奖励权重6

	//int iTaskSize;         //任务配置数量
	//int iTaskID;			 //任务编号
	//int iTaskType;		 //任务类型
	//int iTaskNameCHLen;     //任务名称（中文）长度
	//char szTaskNameCH[];    //任务名称（中文）
	//int iTaskNameRULen;     //任务名称（俄文）长度
	//char szTaskNameRU[];    //任务名称（俄文）
	//int iPlayType;          //玩法屏蔽 第1位屏蔽不可转移玩法
	//int iTriggerWeight1;    //表1触发权重
	//int iTriggerWeight2;    //表2触发权重
	//int iTriggerWeight3;    //表3触发权重
	//int iTriggerWeight4;    //表4触发权重
	//int iTriggerWeight5;    //表5触发权重
	//int iTriggerWeight6;    //表6触发权重
	//int iAwardType;        //奖励类型
	//int iAwardNum1;        //表1奖励数量
	//int iAwardNum2;        //表2奖励数量
	//int iAwardNum3;        //表3奖励数量
	//int iAwardNum4;        //表4奖励数量
	//int iAwardNum5;        //表5奖励数量
	//int iAwardNum6;        //表6奖励数量
}GameGetIntegralTaskInfoResDef;

typedef struct GameGetMulGrowupEventReq		//GAME_GET_MUL_GROWUP_EVENT_MSG  查询用户多条成长事件信息请求
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGrowEventNum;	//要查询的成长事件数量，后面跟着iGrowEventNum个iGrowEventId
	//int iGrowEventId;	//查询的成长事件id
}GameGetMulGrowupEventReqDef;

typedef struct GameGetMulGrowupEventRes		//GAME_GET_MUL_GROWUP_EVENT_MSG  查询用户多条成长事件信息回应
{
	MsgHeadDef msgHeadInfo;
	int iUserID;
	int iGrowEventNum;		//后面跟着iGrowEventNum个成长事件信息
	//int iGrowEventId;
	//int iGrowNumInfo;
	//int iGrowStrInfoLen;	//字符串内容长度
	//char szGrowStrInfo[];
}GameGetMulGrowupEventResDef;

#endif