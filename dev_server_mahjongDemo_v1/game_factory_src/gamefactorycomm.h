#ifndef __GAMEFACTORY_COMM_H__
#define __GAMEFACTORY_COMM_H__

#include "gamelogic.h"
#include "factorymessage.h"
#include "factoryplayernode.h"
#include "factorytableitem.h"


////1福券，2金币，3钻石 4奖券
enum class MonetaryType {
	FIRST_MONEY  = 1,
	Money = 2,
	Diamond = 3,
	Integral = 4
};

//游戏内通用日志类型，尽量和redis内统计编号保持一致
enum class ComGameLogType
{
	GAME_USE_PROP = 100,/*游戏内使用道具*/
	GAME_ADD_PROP = 110,/*游戏发放道具*/
	GAME_PROP_BUY_DIAMOND = 306,/*游戏内购买道具回收钻石*/
	GAME_LOGIN_GET_CHARGE = 500,/*游戏内登录补充到账*/
	GAME_MID_CHARGE = 501,/*游戏内充值到账*/
	GAME_NORMAL_ACCOUNT = 201,/*游戏内正常游戏结束计费*/
	GAME_SPECIAL_ACCOUNT = 204,/*游戏内特殊计费*/
	GAME_TASK_SEND_INTEGRAL = 404,
};
#endif //__GAMEFACTORY_COMM_H__