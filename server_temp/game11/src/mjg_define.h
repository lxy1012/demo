#ifndef _MJG_DEFINE_H_
#define _MJG_DEFINE_H_

#include <vector>
using namespace std;

//牌型0-5,0为万，1为条，2为饼 3为字 4为箭 5为花

//牌型0-2  1-9 分别为 1-9万饼条
//牌型 3 1-4为 东西南北
//牌型 4 1-3为 中发白
//牌型 5 1-8 为 春夏秋冬 梅兰竹菊

//胡牌番
#define PING_HU         			0  	//平胡 		 1番 
#define PENG_PENG_HU 				1   //碰碰胡   	 2番
#define QING_YI_SE 					2   //清一色     3番
#define DAI_YAO_JIU 				3 	//带幺久	 3番
#define QI_DUI_HU					4	//七对		 3番
#define JIN_GOU_DIAO				5	//金钩钓     3番
#define QING_PENG_HU                6   //清碰       4番
#define JIANG_DUI_HU				7	//将对胡     4番
#define LONG_QI_DUI_HU				8	//龙七对	 4番
#define QING_QI_DUI_HU              9   //清七对     5番
#define QING_YAO_JIU_HU             10  //清幺久     5番
#define QING_JIN_GOU_DIAO			11	//清金钩钓   5番
#define QING_LONG_QI_DUI_HU			12	//清龙七对   6番
#define SHIBA_LUOHAN                13  //十八罗汉   7番
#define QING_SHIBA_LUOHAN           14  //清十八罗汉 9番
#define	TIAN_HU						15	//天胡		 6番 不计其他任何番数
#define	DI_HU						16	//地胡		 6番 不计其他任何番数


//另加番
#define	GANG_SHANG_KAI_HUA  		17	//杠上花	1番
#define	QIANG_GANG_HU				18	//抢杠胡	1番
#define GANG_SHANG_PAO				19	//杠上炮 	1番
#define SAO_DI_HU           		20  //扫地胡    1番
#define JIN_GOU_HU           		21  //金钩胡    1番

#define	SI_GENG						22	//四根		1番
#define ZIMO                        23  //自摸      1番

#define ZHI_GANG					24	//直杠		1番
#define MIAN_XIA_GANG               25  //面下杠    1番
#define AN_GANG						26	//暗杠    	1番
#define CARDS_TYPE_NUM              27  //番型数组大小

#define LIMITE_MAX_FAN_NUM		500		//限定最大番数
#define HUAZHU_LOSE_MIN_FAN_NUM 4 //定义花猪的最低输赢番数 含了平胡一番


#define HZXLMJ_GAME_ID 11 			// GameID

#define MAX_WIN_FAN 999999999		// 最大番数，超过这个值默认为这个值

enum RecordEventID
{
	// 福券兑换统计
	RECHARGE_BKB_POP_CNT = 1,			// 弹出福券兑换次数
	RECHARGE_BKB_CLICK_CLOSE_CNT,		// 点击关闭弹窗次数
	RECHARGE_BKB_COUNT_DOWN_CLOSE_CNT,	// 倒计时结束关闭次数
	RECHARGE_BKB_CLICK_RECHARGE_CNT,	// 点击兑换福券次数
	RECHARGE_BKB_SUCCESS_CNT,			// 成功兑换福券次数

	// 现金充值统计
	RECHARGE_BKD_POP_CNT = 6,			// 弹出现金付费弹窗次数
	RECHARGE_BKD_CLICK_CLOSE_CNT,		// 点击关闭现金充值弹窗次数
	RECHARGE_BKD_COUNT_DOWN_CLOSE_CNT,	// 现金充值弹窗倒计时结束关闭次数
	RECHARGE_BKD_CLICK_RECHARGE_CNT,	// 点击现金充值购买次数
	RECHARGE_BKD_SUCCESS_CNT,			// 现金充值成功购买次数
};

typedef struct ChiType
{
	bool bOther;     //靠别人吃来的为TRUE
	int iType;       //类型
	int iFirstValue; //顺子的第一个值
	int iChiValue;   //吃来的那张牌
}ChiTypeDef;       //顺子信息

typedef struct PengType
{
	bool bOther;     //靠别人碰来的为TRUE
	int iType;
	int iValue;
	int iGangType;   //0为碰牌，1为明杠牌，2为暗杠牌     resultType.pengType[i].iGangType == 0peng 1ming gang 2an gang

}PengTypeDef;      //刻子信息

typedef struct JiangType
{
	int iType;
	int iValue;
}JiangTypeDef;    //将牌信息

typedef struct LastCard
{
	bool bOther;    //是别人的牌，true其实就是别人放炮拉,false为自摸
	bool bGang;     //是杠来的牌
	bool bQiangGang;//是抢杠来的牌
	int iType;
	int iValue;
	//int iLastTableCardIndex;
	// int iGangShangPao;	//是杠牌后打的那张牌 1表示杠上炮
	// int iTianHu;				//胡牌是否是天胡 1天胡
	// int iDiHu;					//胡牌是否是地胡 1地胡
}LastCardDef;     //最后张到手牌信息

typedef struct WinPlayerInfo
{
	int     iHuFlag;  
	int     iHuFan; 
	int     iGangFan;   
	int     iLoseFen;   
	int     iGengFan; 
}WinPlayerInfoDef;

typedef struct JudgeHuInfo
{
	int  	iHuOrder;					//玩家点击胡牌的顺序 只能是1 2 3
	int 	iHuType;					//胡牌类型 1炮胡  2自摸 -1初值
	int		iHuCardIndex;				//胡牌时摸牌摸到哪一张了 一炮多响时有多余一个玩家的此值相等
	int 	iLosePlayer[3];				//胡牌输家 -1初值 保留输钱玩家的绝对位置
	int     iAllFanNum;					//胡牌的总番数  胡牌时记录  免得结算的时候再计算了 这个使用前需要限定番数
	int		iRealAllFanNum;				//玩家胡牌的真实的总番数
	char    cFanResult[CARDS_TYPE_NUM]; //保留番数结果
	//int     iWinPlayerSpecial;
}JudgeHuInfoDef;

typedef struct ResultType
{
	int			iChiNum;
	int			iPengNum;
	ChiTypeDef chiType[4];//对于16张麻将吃的牌最多可能5组
	PengTypeDef pengType[4];//对于16张麻将碰的牌最多可能5组
	JiangTypeDef jiangType;
	LastCardDef	lastCard;

	JudgeHuInfoDef	judgeHuInfo;	//胡牌信息
}ResultTypeDef;



typedef struct MJGCPG
{
	int iCPGType;		// 0表示吃    1表示碰  2表示杠
	char cCard[4];	        //4个牌的牌值
	char cTagNum;		//被吃碰的人位置  暗杠自摸就为-1
	char cGangType;		//0 明杠 1补杠 2暗杠
	char cChiValue;		//吃的那张牌
	char cGangTag;		//放杠玩家 以位数记录
}MJGCPGDef;

typedef struct MJGCPGInfo
{
	int iAllNum;
	MJGCPGDef Info[4];
}MJGCPGInfoDef;


typedef struct MJGTempCPGReq		//保存临时的吃碰杠请求
{
	int         iSpecialFlag;
	char		cFirstCard;
	char		cCards[4];
	char		cBeQiangGangHu;		// 是否是抢杠胡之后的杠牌
}MJGTempCPGReqDef;



typedef struct HuCardInfo
{
	char cHuCard;
	char cHuExtra;
}HuCardInfoDef;


typedef struct GameLiuShuiInfo
{
	char			cWinner;		//赢钱方
	char			cLoser;			//输钱方
	char			cFengDing;
	char			cPoChan;
	int				iSpecialFlag;	//操作flag
	int				iFan;			//番数 明杠 2
	long long				iWinMoney;		// 本次结算的赢分(断线重连时用于客户端显示)
	long long				iMoney[4];		// 本次结算后每个玩家的金币
	char			cFanResult[52];	// 玩家胡牌番型
}GameLiuShuiInfoDef;

typedef struct GameEndPlayerInfo
{
	char			cHuCard;
	char			cHandCards[14];			//玩家胡牌或破产后的手牌
	char			cSendCards[34];
	char			cPaoExtra;
	char			cTing;
	char			cPoChan;
	char			cFengDing;
	char			cDuoXiang;
	int				iHandCardsNum;
	MJGCPGInfoDef	cpgInfo;				//玩家胡牌或破产后记录他的碰杠信息
	int				iGangFen;				//玩家胡牌或破产后记录他的杠分
	int				iGangFan;
	int				iGenFan;
	int				iSpecialFlag;		//玩家胡牌或破产之后的特殊标示
	int				iHuType;
	int				iHuFlag;
	int				iHuFan;
	int				iMoneyResult;

}GameEndPlayerInfoDef;

// 特殊的输赢类型
enum WinSpecialType 
{
		WIN_GUA_FENG = 1,					// 刮风
		WIN_XIA_YU = 2,						// 下雨
		WIN_HU_JIAO_ZHUAN_YI = 3,			// 呼叫转移
		WIN_TUI_SHUI = 4,					// 退税
		WIN_HUA_ZHU = 5,					// 花猪
		WIN_CHA_DA_JIAO = 6,				// 查大叫
};


enum PLAYER_STATE
{
	PLAYER_STATE_NORMAL = 1,		// 正常游戏玩家
	PLAYER_STATE_BANKRUPT = 2,		// 玩家破产
	PLAYER_STATE_LEAVE = 3,			// 玩家离桌
};

// 流局信息
typedef struct LiuJuSpecialInfo
{
	char vcPoChan[4][4];					// 破产
	char cTuiShui;
	long long viTuiShuiMoney[4][4][4];		// 退税分
	char vcTuiShuiFan[4][4][4];			// 退税倍数
	char cHuaZhu;
	long long viHuaZhuMoney[4][4][4];			// 花猪分数
	char vcHuaZhuFan[4][4][4];			// 花猪倍数
	char cChaDaJiao;
	long long viChaDaJiaoMoney[4][4][4];		// 查大叫分数
	int viChaDaJiaoFan[4][4][4];		// 查大叫倍数
	long long viTotalScore[4][4];				// 三种特殊胡牌的总分
}LiuJuSpecialInfoDef;

#endif