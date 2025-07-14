#ifndef _MJG_TABLE_ITEM_H_


#define _MJG_TABLE_ITEM_H_


#include "factorytableitem.h"
#include "mjg_playernode.h"


#define MJG_MAX_CARD_NUM	144//108		//按照麻将牌 万 饼 条 字 箭 花  总数144张  每个游戏再单独设置当前游戏用几张牌   



class MJGTableItemDef : public FactoryTableItemDef


{


public:


	MJGTableItemDef()
	{


	}


	~MJGTableItemDef()
	{


	}
	virtual MJGTableItemDef* operator->()
	{
		return this;
	}


	void Reset()
	{
		for (int i = 0; i < 10; i++)
		{
			pPlayers[i] = NULL;
		}
		for (int i = 0; i < 4; i++)
		{
			bSeziOK[i] = false;
			iYiPaoDuoXiangPlayer[i] = 0;
		}
		for (int i = 0; i < PLAYER_NUM; i++)
		{
			iTingPlyerPos[i] = -1;
		}
		memset(cAllCards, 0, sizeof(cAllCards));

		iAllCardNum = 0;
		iDongPos = -1;
		iZhuangPos = -1;
		cTableCard = 0;			//当前桌上的牌
		cCurMoCard = 0;         //当前摸的牌
		iCardIndex = 0;
		iCurMoPlayer = -1;
		iCurSendPlayer = -1;
		iMaxMoCardNum = 0;
		iLastSendPlayer = -1;
		iHasHuNum = 0;
		iCanHuPlayerCount = 0;
		cGameEnd = 0;
		iWillHuNum = 0;
		iAllHuNum = 0;
		bYiPao = false;
		iHuTime = 0;
		iPoChanNum = 0;
		vector<char>().swap(vcOrderHu);
		vector<GameLiuShuiInfoDef>().swap(vLiuShui);
		vector<HuCardInfoDef>().swap(vcAllHuCard);

		bIfExistAIPlayer = false;

		for (int i = 0; i < 4; i++)
		{
			vector<GameLiuShuiInfoDef>().swap(vLiuShuiList[i]);
			bAutoHu[i] = false;
		}

		cLeastPengExtra = -1;
		bExistWaitRecharge = false;
		iNextWaitSendExtra = -1;
		iWaitRechargeExtra = 0;
	}

	MJGPlayerNodeDef   *pPlayers[10];        //在CallBackReady里赋值,和FACTORY里的pFactoryPlayers指针值一样.不然改指针强制转换太烦了
	char 			cGameEnd;							//本桌牌局是否结束  1牌局结束了
	bool iHuTime;//第几次胡
	int             iYiPaoDuoXiangPlayer[4];
	int             iWillHuNum;
	int				iAllHuNum;				// 同时胡牌人数(一炮多响)
	bool bYiPao;//判断这次胡牌是不是一炮多响
	bool			bSeziOK[4];				//骰子
		
	char			cAllCards[MJG_MAX_CARD_NUM];				//

	int             iAllCardNum;			//当前麻将总共用上几张牌

	int             iDongPos;				//东风家位置

	int             iZhuangPos;				//庄家位置
	
	char            cTableCard;           //当前所出的牌,桌上的牌
	
	char			cCurMoCard;			  //最后一个摸牌的玩家摸的牌
	
	int				iCardIndex;			 //出牌的下标

	int             iCurMoPlayer;		//当前摸牌的玩家
	
	int             iCurSendPlayer;		//当前出牌的玩家

	int				iMaxMoCardNum;		//最大摸牌数量

	//char            cBaoCard;			//宝牌

	int				iTingPlyerPos[PLAYER_NUM];	//听牌玩家的位置	//哈尔滨麻将经典玩法里面的对宝要按照听牌的先后顺序来判定

	int             iLastSendPlayer;		//上一次打牌的玩家

	int             iHasHuNum;//已经胡牌的玩家数量  到3 gameend
	int             iCanHuPlayerCount;//玩家出牌一炮多响是有几个玩家可以胡牌
	int				iPoChanNum;
	vector <char>	vcOrderHu;
	vector <HuCardInfoDef>	vcAllHuCard;

	vector <GameLiuShuiInfoDef> vLiuShui;

	vector<GameLiuShuiInfoDef> vLiuShuiList[4];		// 四个玩家的流水不一样，分开保存

	bool bIfExistAIPlayer;							//当前桌上是否有AI玩家
	bool bAutoHu[4];				//是否自动胡

	char cLeastPengExtra;							// 标记最近一次放给上家碰牌的玩家
	bool bExistWaitRecharge;						// 桌子上是否存在游戏内破产等待充值的玩家
	int iWaitRechargeExtra;							// 桌子上等待充值玩家的座位号
	int iNextWaitSendExtra;							// 桌子上等待充值结束下一个要出牌的玩家座位号
};

#endif //_TABLE_ITEM_H_


