#ifndef __GAME_EVENT_H__
#define __GAME_EVENT_H__

#include <vector>
#include <map>
#include "gamelogic.h"

using namespace std;

enum EVENT_1
{
	GAME_CNT = 8,
	SEND_MONEY = 10,
	RECYCLE_MONEY = 13,
	SEND_DIAMOND = 15,
	RECYCLE_DIAMOND = 18,
	SEND_INTEGRAL = 37,
	RECYCLE_INTEGRAL = 38,

	//20221229 add by cent
	FRIEND_ROOM_CNT = 21,
	WITH_FRIEND_CNT = 41,
	NEW_PLAYER_DAY_CNT = 42,   //今日注册玩家游戏总局数
};

class GameLogic;
class GameEvent
{
public:
	GameEvent();
	void ClearEventMap();
	~GameEvent();
	
	void AddComEventData(int iEventID, int iSubID, long long iAddNum = 1);
	void SendComStatRedis(GameLogic * pLogic);

	//道具数据统计 iPropID>0玩家消耗使用 iPropID<0系统发放
	void AddUesPropData(int iRoomType, int iPropID, int iAddNum = 1);
private:
	std::map<int, map<int, long long> > m_mapGameEvent;

	//道具消耗报表统计
	map<int, int> m_mapUsePropInfo[4]; //5678四个房间里游戏使用道具信息
};

#endif