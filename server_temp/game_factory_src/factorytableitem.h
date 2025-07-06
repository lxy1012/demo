#ifndef __FACTORY_TABLE_ITEM__
#define __FACTORY_TABLE_ITEM__

#include <time.h>
#include "factoryplayernode.h"
#include "factorymessage.h"
//class FactoryPlayerNodeDef

class FactoryTableItemDef
{
public:
	FactoryTableItemDef()
	{	
	}
	virtual ~FactoryTableItemDef()
	{
		
	}
	
	virtual FactoryTableItemDef* operator->() 
	{
		return this;
	}
	
	FactoryPlayerNodeDef   *pFactoryPlayers[10];         //本桌的玩家：指向玩家链表的节点,对应位置为0，1，2,3
	char						cPlayerNum;						//本桌玩家当前个数
	bool						bIfReady[10];
	int						iReadyNum;
	int                     iSeatNum;    //玩法最大座位数
	int                     iRealNum;    //开局时实际人数（最小玩法人数到最大人数之间）
	
	unsigned char		cReplayData[1024];//游戏重放数据
	int							iDataLength;//当前重放数据的长度
	char						cReplayGameNum[20];//游戏编号
	char                    cPlayerName[256];  //桌上玩家名字，游戏日志用
	time_t 					iBeginGameTime;//每局游戏开始的时间
	bool 					bIfDealOK[10];	   //发牌同步标记

	int iTableID;
	int iFirstDisPlayer;//第一个掉线玩家的位置
	int iIfFriendTable;//好友桌（对应好友房id）=0正常桌

	time_t			tmLastRecUserMsg;//上次收到桌上玩家信息的时候（超过5分钟，桌上玩家都没有任何动静，该桌有异常了，所有玩家解散）
	int   iNextVTableID;//本桌若未能正常继续下一局，剩余玩家可用桌号
};

#endif //__FACTORY_TABLE_ITEM__
