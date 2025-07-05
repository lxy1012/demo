#ifndef __MJG_PLAYER_NODE__
#define __MJG_PLAYER_NODE__
#include "factoryplayernode.h"
#include "mjg_define.h"
#include "string.h"
#include <map>
#include "msg.h"

#define PS_WAIT_SPECIAL 10    //等待吃碰杠胡弃操作
#define PS_WAIT_CHANGE_CARD    11		// 选牌
#define PS_WAIT_CONFIRM_QUE    12		// 定缺

class MJGPlayerNodeDef : public FactoryPlayerNodeDef
{
public:
	MJGPlayerNodeDef()
	{
		Reset();
		//本地数据缓存，游戏结束时不能清零
		iCurDayPlayNum = 0;
		iCurDayWinMoney = 0;
		iCurDayLoseMoney = 0;

		iNowDayWinNum = 0;
		iNowDayLoseNum = 0;
		iNowDayWinMoney = 0;
		iNowDayLoseMoney = 0;
		iNowWeekWinNum = 0;
		iNowWeekLoseNum = 0;
		iNowWeekWinMoney = 0;
		iNowWeekLoseMoney = 0;
		iDayWinLoseMoney = 0;
		cIfReviewVersion = 0;
	}
	~MJGPlayerNodeDef()
	{
	}
	virtual MJGPlayerNodeDef* operator->()
	{
		return this;
	}
	MJGPlayerNodeDef      *pNext;           //指向前一节点
	MJGPlayerNodeDef      *pPrev;           //指向后一节点
	time_t				  tmSitDownTime;	//玩家坐下时间
	char		cHandCards[14];			    //手中实际牌,因为不这样的话判断吃碰杠的时候会把已经吃碰杠过了的牌也算进去
	int			iHandCardsNum;			    //手中实际牌的张数
	char		cSendCards[34];			    //已经出的牌
	char        cMoCardsNum;				//当前摸到的牌	
	ResultTypeDef  resultType;		        //用来判断胡牌的
	MJGCPGInfoDef  CPGInfo;					//记录玩家吃碰杠信息的 
	bool        bTing;						//是否听牌
	int			iSpecialFlag;				//玩家吃碰听杠胡的标记
	int         iHuFlag;					//= iHuType 在消息里	//记录玩家胡什么牌（如碰碰胡）
	char		cFanResult[MJ_FAN_TYPE_CNT];	// 玩家胡牌番数
	bool		bBankruptRecharge;				// 玩家是否破产充值过
	int iKickOutTime;
	bool bPlayerKick;


	int			iAllSendCardsNum;			//玩家出的牌的总数
	vector<char> vcSendTingCard;			//听牌出的牌
	vector<char> vcTingCard;				//听的牌
	map<char, int> mpTingFan;
	int         iWinFlag;			//= iHuFlag 在消息里		//赢牌的标记  1对宝 2刮大风 3抢杠胡

	int iTingHuType;
	int iTingHuFlag;
	int iTingHuFan;

	MJGTempCPGReqDef pTempCPG;				//临时保存的吃碰杠信息
	int         iGangFen;	   			//杠分
	
	bool bAutoHu;
	char cQiangGangCard;
    int iQueType;						// 玩家定缺的那一门
	bool bIsHu;							// 玩家是否胡牌了
	bool		bGuoHu;                         //能不能胡(过胡不胡的判定条件)
	bool		bCanQiangGangHu;		// 是否过抢杠胡(抢杠胡点了弃)
	bool bChangeCardReq;				//是否发来了 换牌请求
	bool bConfirmQueReq;
	char cChangeReqCard[3];		//玩家请求换的牌
	char cChangeIndex[3];		// 玩家要换的牌在手牌中的序号
	bool bQiangGangHu;
	int iQiangGangPos;

	int iHuFan;						//玩家胡 番数
	bool bLastGang;
	bool bHuaZhu;
	bool bDaJiao;
	int iMoneyHuZongFen;
	char cFengDing;
	char cPoChan;
	char cHuCard;					// 玩家当前胡的牌
	int  cTempTableStatus;

	int iBeiQiangGangPos;
	long long  iCurWinMoney;				// 玩家当局输赢金币
	long long iAllUserCurWinMoney[10];		// 当前玩家看到的其他玩家赢的钱

	vector<char > vcHuCard;

	GameUserAccountReqDef msgRDTemp;

	//int iPlayerShowMoney[10];			// 每个玩家看到的桌上其他玩家的金币(混合配桌房间专用，看到的其他用户的钱是模拟的),自家的金币是真实的

	//AI相关
	int  iPlayerType;					//0是正常玩家，1是主AI-1，2是主AI-2，3是副AI
	bool bNeedMatchAI;					//当前玩家是否需要匹配AI
	bool bOnlyMatchAI;					//当前玩家是否值匹配AI
	bool bIfControl;					//该玩家是否处于控制流程下
	int  iWaitAiTime;					// 真实玩家等待AI玩家的时间

	int iCurDayPlayNum;					//当日玩家游戏局数
	int iCurDayWinMoney;				//当日玩家赢分
	int iCurDayLoseMoney;				//当日玩家输分

	char cMainCards[10][10];			//主AI-1的主牌 、主AI-2的主牌
	char cNeedCards[10][10];			//主AI-1的缺牌
	char cNeedCardsBackUp[10][10];		//主AI-1的缺牌备份
	vector<char>vecRestCards;			//主AI-1的多余牌
	vector<char>vecAIGetCardOrder;		//主AI-1的上张顺序

	bool bIfAICanChi;					//主AI-1能否吃
	bool bIfAICanPeng;					//主AI-1能否碰
	bool bIfAICanGang;					//主AI-1能否杠

	int iIfResetHandCard;				//主AI-2+副AI是否重置手牌 -1 不需要重置 0未重置 1已经重置

	bool bIfCTLGetHZ;					//主AI-2和副AI是否进入控制获取一张红中

	vector<char>vecAIHuCard;			//主AI-1的可胡牌
	vector<char>vecResetHuCard;			//重置手牌后主AI-2的可胡牌
	vector<char>vecAssistAICards;		//AI-2未胡牌、副AI将六张牌存到牌墙里

	int iAIConfirmQueType;				//主AI-1 && 主AI-2的定缺类型
	int iAIHuCount;						//AI胡牌次数

	//记录玩家extragameinfo
	int iNowDayWinNum;
	int iNowDayLoseNum;
	int iNowDayWinMoney;
	int iNowDayLoseMoney;
	int iNowWeekWinNum;
	int iNowWeekLoseNum;
	int iNowWeekWinMoney;
	int iNowWeekLoseMoney;

	int iDayWinLoseMoney;
	char cIfReviewVersion;

	int iContinousZiMoCnt;				// 玩家连续自摸的次数
	int viContinousHuPosCnt[4];			// 玩家出一张牌，被其他玩家连续胡牌的次数
	int iZiMoHuHongZhongCnt;			// 统计本局自摸胡红中的次数
	bool bWaitRecharge;					// 玩家游戏内破产等待充值
	time_t tmBeginCharge;				// 玩家破产后开始充值时间
	int iAllowChargeTime;				// 玩家允许游戏内破产的充值时间
	char cRechargeType;					// 玩家充值类型：1福券；2现金

	// 重置玩家信息，游戏开始和游戏结束时调用
	void Reset()
	{		
		FactoryPlayerNodeDef::Reset();
		iKickOutTime = 0;
		bPlayerKick = false;
		memset(cHandCards,0,sizeof(cHandCards));
		memset(cChangeReqCard,0,sizeof(cChangeReqCard));
		memset(cChangeIndex, 0, sizeof(cChangeIndex));
		bChangeCardReq = false;
		bConfirmQueReq = false;
		bQiangGangHu = false;
		bHuaZhu = false;
		bDaJiao = false;
		iCurWinMoney = 0;
		iQiangGangPos = 0;
		iBeiQiangGangPos = -1;
		iMoneyHuZongFen = 0;
		bAutoHu = false;
		bLastGang = false;
		iHuFan=0;
		iHandCardsNum = 0;
		memset(cSendCards, 0, sizeof(cSendCards));
		cMoCardsNum = 0;
		cQiangGangCard = 0;
		memset(&resultType, 0, sizeof(ResultTypeDef));
		memset(&CPGInfo, 0, sizeof(MJGCPGInfoDef));
		bTing = false;
		bIsHu = false;
		bBankruptRecharge = false;
		iSpecialFlag = 0;
		iHuFlag = 0;
		iAllSendCardsNum = 0;

		vector<char>().swap(vcTingCard);
		iTingHuType = 0;
		iTingHuFlag = 0;
		iTingHuFan = 0;
		memset(&msgRDTemp, 0, sizeof(GameUserAccountReqDef));
		//vcChiCard.clear();
		bGuoHu = false;
		bCanQiangGangHu = false;
		bLastGang = false;
		//vcChiTingCard.clear();
		iWinFlag = 0;
		memset(&pTempCPG, 0, sizeof(MJGTempCPGReqDef));
		vector<char>().swap(vcSendTingCard);
		map<char, int>().swap(mpTingFan);

		vector<char>().swap(vcHuCard);

		iGangFen = 0;
		iQueType = -1;
		cPoChan = 0;
		cFengDing = 0;
		cHuCard = 0;
		cTempTableStatus = 0;

		//iHuTotalFan = 0;
		memset(cFanResult, 0, sizeof(cFanResult));
		memset(iAllUserCurWinMoney, 0, sizeof(iAllUserCurWinMoney));

		iPlayerType = -1;
		iWaitAiTime = -1;
		iAIConfirmQueType = -1;
		bIfControl = false;
		bNeedMatchAI = false;
		bOnlyMatchAI = false;
		iAIHuCount = 0;
		

		bIfAICanChi = false;
		bIfAICanPeng = false;
		bIfAICanGang = false;

		iIfResetHandCard = 0;

		bIfCTLGetHZ = false;

		memset(&cMainCards, -1, sizeof(cMainCards));
		memset(&cNeedCards, -1, sizeof(cNeedCards));
		memset(&cNeedCardsBackUp, -1, sizeof(cNeedCardsBackUp));
		vector<char>().swap(vecRestCards);
		vector<char>().swap(vecResetHuCard);
		vector<char>().swap(vecAIGetCardOrder);
		vector<char>().swap(vecAIHuCard);
		vector<char>().swap(vecAssistAICards);
		
		iContinousZiMoCnt = 0;
		memset(viContinousHuPosCnt, 0, sizeof(viContinousHuPosCnt));
		iZiMoHuHongZhongCnt = 0;
		bWaitRecharge = false;
		tmBeginCharge = 0;
		iAllowChargeTime = 0;
	}
};





#endif //__PLAYER_NODE__


