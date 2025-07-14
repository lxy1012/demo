#ifndef __MSG_H__
#define __MSG_H__

#include "factorymessage.h"
#include "judgecard.h"

#define PLAYER_NUM		4			//游戏人数

//服务器请求客户端
#define MJG_SPECIAL_CARD_SERVER		0x11				//服务器 请求 吃碰杠 
#define	MJG_MO_CARD_SERVER			0x12				//服务器摸牌请求
#define HEBMJ_DEAL_CARD_SERVER		0x13				//发送发牌消息

#define HEBMJ_AGAIN_LOGIN_SERVER	0x15				//断线重入消息

#define XLMJ_CHANGE_CARD_SERVER     0x16				//发送换牌消息
#define XLMJ_CONFIRM_QUE_SERVER     0x19                //发送定缺消息

#define HZXLMJ_MAGIC_EXPRESS_MSG	0x1A				// 达成指定条件自动发送表情消息
#define HZXLMJ_RECHARGE_RECORD_MSG	0x1B				// 充值统计消息

#define XLMJ_AGAIN_LOGIN_EXTRA		0x20				// 断线重连吃碰杠相关的信息
#define XLMJ_AGAIN_LOGIN_TING		0x21				// 断线重连 
#define XLMJ_AGAIN_LOGIN_LIUSHUI	0x22				// 断线重连流水相关的信息
#define XLMJ_AGAIN_LOGIN_HU_INFO	0X1C				// 断线重连胡牌相关的信息

#define HZXLMJ_ROOM_INFO_MSG		0x23				// 房间信息
#define HZXLMJ_SPECIAL_INFO_MSG		0x24				// 其他玩家特殊操作消息
#define HZXLMJ_GAME_INFO_MSG        0x25				// 服务器相关信息

#define  HZXLMJ_UPDATE_PLAYER_INFO 0x30					// 玩家游戏内破产后充值，刷新玩家信息

#define GET_HZXL_CONTROL_CARD_MSG   0x2a				//获取红中血流控制牌库

//客户端请求服务器
#define MJG_SEND_CARD_REQ			0x42				//客户端请求服务器出牌
#define MJG_SPECIAL_CARD_REQ		0x43				//客户端吃碰杠听牌请求
#define XLMJ_CONFIRM_QUE_REQ        0x44                //客户端定缺请求
#define XLMJ_CHANGE_CARD_REQ        0x45
//#define XLMJ_SEND_AUTO_HU_REQ		0x46				//客户端请求自动胡牌
#define HZXLMJ_RECHARGE_REQ			0x46				// 玩家游戏内破产请求充值
#define HZXLMJ_RECHARGE_RESULT_MSG	0x47				// 玩家充值结束，广播给玩家

//服务器回应客户端
#define MJG_SPECIAL_CARD_RES		0x32				//客户端吃碰杠听牌回应
#define MJG_SEND_CARD_RES			0x31				//服务器发送出牌结果
#define CONFIRM_QUE_NOTICE_MSG        0x33                //回应客户端定缺
#define CHANGE_CARD_NOTICE_MSG        0x34                //回应客户端换牌
#define CHANGE_CARDS_NOTICE_MSG   0x35                
#define SURE_CHANGE_CARD_NOTICE_MSG  0x36
#define MJG_PLAYER_POCHAN_MSG		0x38				// 玩家破产消息
#define HZXLMJ_GAME_RESULT_SERVER	0x39				// 游戏结束消息


typedef struct RoomInfoServer		//HZXLMJ_ROOM_INFO_MSG 房间信息
{
	MsgHeadDef  msgHeadInfo;
	int iEnterRoomType;					// 房间类型
	long long iBaseScore;						// 底分        
	long long iTableMoney;					// 桌费
	char cIfLoginAgain;					// 是否是断线重连发的消息
}RoomInfoServerDef;

typedef struct GameInfoServer		//HZXLMJ_GAME_INFO_MSG 服务器信息
{
	MsgHeadDef  msgHeadInfo;
	int iEnterRoomType;					// 房间类型
	int iFengDingFan;				// 封顶番
	long long iMaxPoint;	//结算封顶值
	int viEmpty[8];						// 备用字段
}GameInfoServerDef;

typedef struct DealCardsServer		//DEAL_CARDS_SERVER_MSG “发牌“
{
	MsgHeadDef  msgHeadInfo;
	char		cCards[13];					//只发属于玩家自己的牌
	char		cPoint[2];					//两个骰子的点数        
	char		cZhuangPos;					//庄家位置
	long long		viPlayerMoney[4];			// 四个玩家当前携带的金币数量
}DealCardsServerDef;

typedef struct ConfirmQueServer
{
	MsgHeadDef  msgHeadInfo;
	char cTableNumExtra;
	int iQueType;							// 推荐的定缺类型
}ConfirmQueServerDef;

typedef struct ChangeCardServer
{
	MsgHeadDef msgHeadInfo;
	char cTableNumExtra;
	char cChangeCard[3];
	int cCannotChoose[2];
	char cZhuangPos;
	char cZhuangMoCard;
}ChangeCardServerDef;

typedef struct SendCardsServer		//SEND_CARDS_SERVER_MSG  ”出牌“请求
{
	MsgHeadDef  msgHeadInfo;
	char		cTableNumExtra;
	char		cCard;					//同时把发的牌给他,如果是Special之后的请求就不给牌了,
	char		cFirst;
	int			iSpecialFlag;					//所有吃碰听杠都放在一个标记里面  0位吃 1碰 2-3 杠 4听 5吃听 6碰听 
	int			iTingNum;
	char		iTingSend[14];			//可以听哪几张牌
	char		iTingHuCard[14][28];	//打掉牌后，可以胡的牌
	int			iTingHuFan[14][28];		//打掉牌后，可以胡的牌的番数
	int			iGangNum;
	int			iHuMulti;			// 胡牌倍数
	char		iGangCard[14];		//杠的话，明杠当然只有一种，但是暗杠和碰杠最多也可能有3种
	char		cGangType[14];		//对应杠的那张牌，手里有几张，当然明杠为4，暗杠为12，碰杠为8
	char        cIfHu;			//判断玩家摸牌之后 是否是胡牌
	bool        bIsLastFourCard;
}SendCardsServerDef;

typedef struct SendCardsReq    //   玩家出牌请求
{
	MsgHeadDef      msgHeadInfo;
	char			cCard;		   //出的牌,为0表示胡牌
	char			cIfTing;		//是否听牌
}SendCardsReqDef;

typedef struct ConfirmQueReq //CONFIRM_QUE_REQ_MSG
{
	MsgHeadDef msgHeadInfo;
	int iQueType;				//定缺的一门	
	char cTableNumExtra;
}ConfirmQueReqDef;

typedef struct ChangeCardsReq	//CHANGE_CARDS_REQ_MSG 0x45 玩家换牌请求	
{
	MsgHeadDef msgHeadInfo;
	char cCards[3];				//请求换牌的3张牌
}ChangeCardsReqDef;

typedef struct SureChangeCardNotice	//SURE_CHANGE_CARD_NOTICE_MSG	0x38 玩家换牌请求得到确认的信息通知
{
	MsgHeadDef msgHeadInfo;
	int  iTableNumExtra;	//请求换牌玩家的位置
	char cSourceCards[3];	//客户端发来的要换的牌
}SureChangeCardNoticeDef;

typedef struct SendCardsNotice		//SEND_CARDS_NOTICE_MSG 玩家的“出牌”通知
{
	MsgHeadDef   msgHeadInfo;
	char			cTableNumExtra;
	char			cCard;
	char			cIfTing;
}SendCardsNoticeDef;

typedef struct ConfirmQueNotice //CONFIRM_QUE_NOTICE_MSG
{
	MsgHeadDef msgHeadInfo;
	int iQueType;
	int iCurCompleteQueCnt;			// 当前完成定缺的玩家人数
	char cTableNumExtra;
}ConfirmQueNoticeDef;


//注意换牌通知必需通知客户端所有玩家  即使是没有换牌的也要通知一次
typedef struct ChangeCardsNotice 	//CHANGE_CARDS_NOTICE_MSG 0x35 玩家换牌通知
{
	MsgHeadDef msgHeadInfo;
	char cTableNumExtra;
	char cSourceCards[3];	//客户端发来的要换的牌
	char cCards[3];			//已经换好的牌
	int iChangeType;        //0顺时针  1逆时针  2对家
}ChangeCardsNoticeDef;


typedef struct SpecialCardsServer	//
{
	MsgHeadDef   msgHeadInfo;
	int			iSpecialFlag;
	char		cTableNumExtra;
	char        cChiTingCard[6];		//把玩家吃听的牌发给玩家
	char        cChiCard[6];		//把玩家吃的牌发给玩家
	char		cHuCard;
	char		cWillHuNum;
	int			iHuType;
	int			iHuFlag;
	int			iHuMulti;			// 胡牌倍数
	char		cTagExtra;
}SpecialCardsServerDef;


typedef struct SpecialCardsReq	//
{
	MsgHeadDef   msgHeadInfo;
	int			iSpecialFlag;
	char		cFirstCard;
	char		cCards[4];
}SpecialCardsReqDef;

typedef struct SendAutoHuReq
{
	MsgHeadDef   msgHeadInfo;
	bool		 bAutoHu;		//玩家是否自动胡牌
}SendAutoHuReqDef;

typedef struct MidWayLeaveNotice  //MID_WAY_LEAVE_NOTICE_MSG	0x34 玩家游戏中途离开的通知
{
	MsgHeadDef msgHeadInfo;
	int iTableNumExtra; //中途离开玩家的得分
}MidWayLeaveNoticeDef;


typedef struct PoChanCardNotice
{
	char         cHandCards[4][14];	//把玩家手牌发送给客户端
	MJGCPGInfoDef cpgInfo[PLAYER_NUM];//吃碰杠的牌
}PoChanCardNoticeDef;

typedef struct PoChanCardNoticeMsg
{
	MsgHeadDef   msgHeadInfo;
	int			 iTableNumExtra;	// 破产玩家座位号
	char         cHandCards[4][14];	// 把玩家手牌发送给客户端
	MJGCPGInfoDef cpgInfo[PLAYER_NUM];//吃碰杠的牌
}PoChanCardNoticeMsgDef;

typedef struct SpecialHuNotice
{
	int			 iZhuangPos;
	long long			 iMoneyResult[PLAYER_NUM];			// 玩家本次输赢分
	long long			 iMoney[PLAYER_NUM];				// 玩家当前分
	long long			 viCurWinMoney[PLAYER_NUM];			// 玩家当前赢分
	char		 cHuNumExtra[PLAYER_NUM];			// 胡牌者的位置，自摸胡牌者位置
	int			 iSpecialFlag[PLAYER_NUM];			// 赢家番型
	int			 iHuType[PLAYER_NUM];
	int			 iHuFlag[PLAYER_NUM];		// 胡牌的牌型
	char		 cIfTing[PLAYER_NUM];		// 是否听牌
	long long          iTableMoney;				// 桌费
	long long			 iHuJiaoZhuanYi;			// 呼叫转移的底分
	int			 ihjzyFan;					// 呼叫转移的倍数
	char		 cFengDing[PLAYER_NUM];		// 是否封顶
	char		 cPoChan[PLAYER_NUM];		// 是否破产
	int			 iHuFan[PLAYER_NUM];
	int			 iMaxFanType[PLAYER_NUM];	// 胡牌玩家胡的最大番型
	int			 iFirstHu;
	int			 iTingHuFan[28];
	char		 vcNeedUpdateTingInfo[PLAYER_NUM];		// 胡牌时，胡牌玩家是否需要更新听牌信息
	char		 cTingHuCard[28];
	char		 cLastWinCard;							// 最后胡牌的牌
	char		cEmpty[3];								// 字节对齐 第一位是当前桌上是否有AI的字段
	char		 cHuFanResult[PLAYER_NUM][MJ_FAN_TYPE_CNT];			// 胡的番数
}SpecialHuNoticeDef;


typedef struct SpecialCardsNotice	//SPECIAL_CARDS_NOTICE_MSG	玩家“吃杠碰胡”通知
{
	MsgHeadDef	 msgHeadInfo;
	long long          iGangFen[4];			//杠产生的积分
	int          iHuFlag;							//胡牌的类型
	int			 iHuType;				//胡牌的类型
	int			 iHuPos[4];
	int			 iWillHuNum;				//胡牌人数
	int			 iCurWinMoney[4];
	char		 cTableNumExtra;
	char		 iSpecialFlag;	  //操作类型	//2  碰  8补杠
	char		 cTagNumExtra;	  //点炮者，自摸为-1，暗杠为-1
	char		 cQiangGangHu;						//是否是抢杠胡
	char		 cFourEnd;
	char		 cHaveHuNotice;
	char		 cCards[3];
	char         cLiuJuHu;			// 是否是流局胡
	char		 cEmpty[2];			// 用于字节对齐
}SpecialCardsNoticeDef;



typedef struct GameResultServer		//HEBMJ_GAME_RESULT_SERVER 当局结果
{
	MsgHeadDef   msgHeadInfo;
	char		 cEnd;
	char		 cLiuJu;
	char         cHandCards[4][14];	//把玩家手牌发送给客户端
	long long			 iMoneyResult[PLAYER_NUM];		//玩家输赢分
	long long			 iMoney[PLAYER_NUM];		//玩家当前分
	char		 cHuNumExtra[PLAYER_NUM];	      //胡牌者的位置，自摸胡牌者位置
	char		 cIfTing[PLAYER_NUM];	//是否听牌(2022.0415，改为用来标识玩家是否破产充值过)
	char		 cHuCount[PLAYER_NUM];	// 玩家总计胡牌张数
	int			 iZhuangPos;			//庄家位置
	long long          iTableMoney;			//桌费
	int			 iMaxFan;
	int		     iHuNum;				// 本局胡牌人数
	char		 cHuOrder[PLAYER_NUM];
	char		 cPoChan[PLAYER_NUM];	//是否破产
	char		 cTuiShui[PLAYER_NUM][PLAYER_NUM];	//char 记录的个数(几组杠牌)
	char		 cHuaZhu[PLAYER_NUM][PLAYER_NUM];	//下面显示的番数
	int			 iDaJiaoFan[PLAYER_NUM][PLAYER_NUM];
	long long			 iTuiShui[PLAYER_NUM][PLAYER_NUM];			// 玩家要退的税
	long long			 iHuaZhu[PLAYER_NUM][PLAYER_NUM];
	long long			 iDaJiao[PLAYER_NUM][PLAYER_NUM];
	long long iMoneyHuZongFen[PLAYER_NUM];
	long long iWinMoney[PLAYER_NUM];
	MJGCPGInfoDef cpgInfo[PLAYER_NUM];//吃碰杠的牌
}GameResultServerDef;

typedef struct GameAgainLoginRes		//GAME_AGAIN_LOGIN_MSG	发送断线重入响应
{
	AgainLoginResBaseDef stAgainLoginBase;

	int iZhuangPos; //庄家位置
	int iCurMoPlayer;	//当前摸牌的玩家
	int iCurSendPlayer;	//当前出牌的玩家
	int iTingPlyerPos[PLAYER_NUM];	//听牌玩家的位置
	int iHandCardNum[PLAYER_NUM];	//手中牌数量
	int iLeftCardNum;				//剩余牌的数量
	char cHandCards[14];//手中牌
	char cSendCards[PLAYER_NUM][34];//已经出的牌
	char cTingCard[28];	//玩家听的牌
	int iTingFan[28];
	char cTableCard;//当前所出的牌,桌上的牌

	char cCurMoCard;//摸牌
	int  iSpecialFlag;	//当前玩家的特殊操作

	int			iGangNum;
	char		iGangCard[14];		//杠的话，明杠当然只有一种，但是暗杠和碰杠最多也可能有3种
	char		cGangType[14];		//对应杠的那张牌，手里有几张，当然明杠为4，暗杠为12，碰杠为8
	bool		bIsChange;
	int			iChangeExtra[4];
	char		cChangeCards[3];
	char		cIsQue;
	int			iQueType[4];
	long long			iCurMoney[4];			// 四个玩家当前剩余金币
	int			iNormalPlayerNum;	// 当前游戏中未破产玩家数量
	char        vcPlayerState[4];		// 当前四个玩家的状态

	char		cAutoHu;
	char		cHuExtra[4];
	HuCardInfo	cHuCardInfo[56];
	char		cPoChan[4];
}GameAgainLoginResDef;

// 断线重连额外的信息
typedef struct ExtraLoginAgainServer
{
	MsgHeadDef msgHeadInfo;
	int iTableNumExtra;					// 位置
	MJGCPGInfoDef cpgInfo[PLAYER_NUM];	// 吃碰杠的牌
}ExtraLoginAgainServerDef;

typedef struct GameAgainLoginTingRes
{
	MsgHeadDef      msgHeadInfo;
	char						cTableNumExtra;		//位置
	int iCurMoPlayer;	//当前摸牌的玩家

	int  iSpecialFlag;	//当前玩家的特殊操作
	int			iTingNum;
	char        iTingHuNum[14];
	char		iTingSend[14];			//可以听哪几张牌,当然最多14张打掉都可以听
	char		iTingHuCard[14][28];	//打掉牌后，可以胡的牌,最多胡10张
}GameAginLoginTingResDef;

typedef struct GameAgainLoginLiuShuiRes
{
	MsgHeadDef  msgHeadInfo;
	int		iLiuShui;				// 流水记录条数
}GameAgainLoginLiuShuiResDef;

//断线重连胡牌相关的信息  XLMJ_AGAIN_LOGIN_HU_INFO			0X1C				
typedef struct GameAgainLoginHuInfoRes
{
	MsgHeadDef  msgHeadInfo;
	char		vcCurHuCount[PLAYER_NUM];			// 玩家当前总计胡牌张数
	char		vcEmpty[4];							// 备用字段
	int			viEmpty[4];							// 备用字段
}GameAgainLoginHuInfoResDef;


// 统计麻将玩家的胡牌牌型及对应的番数信息
typedef struct MJHuFanStatistics
{
	MsgHeadDef msgHeadInfo;
	int iGameID;								// 游戏gameId，不能为空
	int	iRoomType;							// 房间类型：5，6，7，8分别对应初级房，中级房，高级房，VIP房
	char cFanResult[MJ_FAN_TYPE_CNT];			// 玩家胡牌番数
}MJHuFanStatisticsDef;

typedef struct SpecialInfoServer	// HZXLMJ_SPECIAL_INFO_MSG
{
	MsgHeadDef   msgHeadInfo;
	int iSpecialFlag;
	char		cTableNumExtra;
}SpecialInfoServerDef;

typedef struct HZXLControlCards //红中血流控制牌库回应
{
	char cMainCard[64];	//主要牌
	char cNeedCard[32];	//补缺牌
	char cAIChi;		//AI能否吃
	char cAIPeng;		//AI能否碰
	char cAIGang;		//AI能否杠
	char cEmpty;		//补齐
}HZXLControlCards;

typedef struct GetHZXLControlCardReq //红中血流控制牌库请求
{
	MsgHeadDef   msgHeadInfo;
}GetHZXLControlCardReqDef;

typedef struct GetHZXLControlCardRes //红中血流控制牌库回应
{
	MsgHeadDef   msgHeadInfo;
	int iConfNum;	//牌库配置数量
					//ControlCardsDef szControlCards[iConfNUm]; //后面跟iConfNum个ControlCardsDef
}GetHZXLControlCardResDef;

typedef struct AIPlayerRoomInfo {
	time_t tPlayerSitTime;			// 正常玩家坐下的时间
	int iTableNum;					// 桌号
	char cRoomNum;					// 房间号
}AIPlayerRoomInfoDef;

//延时出牌消息
typedef struct AIDelayCardMsg
{
	int  iDelayMilliSeconds; //延时多少毫秒
	int  iMsgLen;			 //消息长度
	int  iMsgType;           //消息类型
	char szBuffer[1024];     //消息内容
}AIDelayCardMsgDef;

typedef struct Cards
{
	char cCard;
	int iRestNumInWall;
	int iRestNumInDeputyAI;
}CardsDef;


typedef struct UpdatePlayerInfo	// HZXLMJ_UPDATE_PLAYER_INFO
{
	MsgHeadDef   msgHeadInfo;
	long long iMoney;					// 要更新信息的玩家要显示的金币
	char cUpdateTableNumExtra;	// 要更新信息的玩家座位号
}UpdatePlayerInfoDef;

// 玩家游戏内破产请求充值
typedef struct RechargeReqMsg		// HZXLMJ_RECHARGE_REQ		0x46	
{
	MsgHeadDef msgHeadInfo;
	char cTableNumExtra;		// 破产充值玩家的座位号
	char cRechargeType;			// 玩家充值类型 1表示福券充值；2表示现金充值
	char cRechargeFlag;			// 破产充值玩家的标志	1 表示玩家请求充值；2表示玩家充值成功；3表示玩家充值失败
}RechargeReqMsgDef;

// 玩家游戏内破产充值结束
typedef struct RechargeResultMsg		// HZXLMJ_RECHARGE_RESULT_MSG		0x47	
{
	MsgHeadDef msgHeadInfo;
	long long iTableMoney;			// 桌费
	char cTableNumExtra;		// 破产充值玩家的座位号
	char cRechargeFlag;			// 充值结果标识：2表示玩家充值成功；3表示玩家充值失败
}RechargeResultMsgDef;

typedef struct	SpecialMagicExpressMsg	// #define HZXLMJ_MAGIC_EXPRESS_MSG	0x1A				// 达成指定条件自动发送表情消息
{
	MsgHeadDef msgHeadInfo;
	int iPropId;				// 发送的魔法表情ID
	int iUserID;				// 发送的玩家座位号
	int iToUserID;				// 被发送的玩家座位号
}SpecialMagicExpressMsgDef;

// 充值统计消息
typedef struct	RechargeRecordMsg	// #define HZXLMJ_RECHARGE_RECORD_MSG	0x1B
{
	MsgHeadDef msgHeadInfo;
	int iRecordId;				// 统计事件的ID
	int iRecordNum;			// 统计事件的数量
}RechargeRecordMsgDef;
#endif //__MSG_H__
