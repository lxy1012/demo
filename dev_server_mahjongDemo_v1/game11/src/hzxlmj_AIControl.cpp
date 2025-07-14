#include "hzxlmj_AIControl.h"
#include "hzxlmj_gamelogic.h"
#include "conf.h"
#include <algorithm>
extern ServerBaseInfoDef stSysConfBaseInfo;
HZXLMJ_AIControl::HZXLMJ_AIControl()
{
	ReadAINickNameInfoConf();
	ReadAIAreaInfoConf();
	m_bIfOpen = false;
	m_iCTLGameNum = 0;
	memset(m_iCTLGameRate, 0, sizeof(m_iCTLGameRate));
	m_vecCTLCards.clear();
	m_iMaxPlayerNum = 4;
	srand(time(NULL));
}

HZXLMJ_AIControl::~HZXLMJ_AIControl()
{
	m_bIfOpen = false;
	m_iCTLGameNum = 0;
	memset(m_iCTLGameRate, 0, sizeof(m_iCTLGameRate));
	m_vecCTLCards.clear();
}
/***
从congfig文件中获取AI相关配置如：是否开启AI、匹配AI时间等
***/
void HZXLMJ_AIControl::ReadRoomConf()
{
	//获取配置——是否开启AI，1开启，0关闭
	GetValueInt(&m_iIfOpenAI, "hzxlmj_if_open_ai", "hzxl_room.conf", "GENERAL", "0");
	_log(_DEBUG, "ReadRoomConf", "m_iIfOpenAi[%d]", m_iIfOpenAI);
	//获取配置——是否可用匹配普通玩家，1可以，0不可以
	GetValueInt(&m_iIfNormalCanMatchNormal, "hzxlmj_if_normal_can_match_normal", "hzxl_room.conf", "GENERAL", "1");
	_log(_DEBUG, "ReadRoomConf", "m_iIfNormalCanMatchNormal[%d]", m_iIfNormalCanMatchNormal);
	//获取配置——当前玩家不可以匹配普通玩家时需要等待AI的时间
	GetValueInt(&m_iNormalWaitAiTimeHigh, "hzxlmj_normal_wait_ai_time_high", "hzxl_room.conf", "GENERAL", "6");							// 等待AI时间上限
	_log(_DEBUG, "ReadRoomTypeConf", "m_iNormalWaitAiTimeHigh[%d]", m_iNormalWaitAiTimeHigh);
	GetValueInt(&m_iNormalWaitAiTimeLow, "hzxlmj_normal_wait_ai_time_low", "hzxl_room.conf", "GENERAL", "1");							// 等待AI时间下限
	_log(_DEBUG, "ReadRoomTypeConf", "m_iNormalWaitAiTimeLow[%d]", m_iNormalWaitAiTimeLow);
	//获取配置——可疑玩家匹配AI玩家等待时间
	GetValueInt(&m_iDoubtfulWaitAiTimeHigh, "hzxlmj_doubtful_wait_ai_time_high", "hzxl_room.conf", "GENERAL", "6");						// 等待AI时间上限
	_log(_DEBUG, "ReadRoomTypeConf", "m_iDoubtfulWaitAiTimeHigh[%d]", m_iDoubtfulWaitAiTimeHigh);
	GetValueInt(&m_iDoubtfulWaitAiTimeLow, "hzxlmj_doubtful_wait_ai_time_low", "hzxl_room.conf", "GENERAL", "1");						// 等待AI时间下限
	_log(_DEBUG, "ReadRoomTypeConf", "m_iDoubtfulWaitAiTimeLow[%d]", m_iDoubtfulWaitAiTimeLow);
}

void HZXLMJ_AIControl::ReadParamValueConf()
{
	string value = "";
	value = HZXLGameLogic::shareHZXLGameLogic()->GetBKParamsValueByKey(KEY_HZXL_CONTROL_GAME_NUM);
	if (value.size() > 0)
	{
		m_iCTLGameNum = atoi(value.c_str());
	}
	_log(_DEBUG, "HZXLMJ_AIControl", "ReadParamValueConf m_iCTLGameNum [%d]", m_iCTLGameNum);
	value = HZXLGameLogic::shareHZXLGameLogic()->GetBKParamsValueByKey(KEY_HZXL_CONTROL_GAME_RATE);
	if (value.size() > 0)
	{
		//先以;为分割符分割出单档控制的string
		std::vector<std::string> vecResult = Split(value, ";");
		int iValue[level_max][control_max];
		_log(_DEBUG, "HZXLMJ_AIControl", "ReadParamValueConf vecResult [%d]", vecResult.size());
		if (vecResult.size() == level_max)
		{
			for (int i = 0; i < vecResult.size(); i++)
			{
				int iItemNum = sscanf(vecResult[i].c_str(), "%d_%d_%d_%d_%d_%d", &iValue[i][game_rate_low], &iValue[i][game_rate_up], &iValue[i][room_type], &iValue[i][amount_low], &iValue[i][amount_up], &iValue[i][control_rate]);
				if (iItemNum == control_max)
				{
					for (int j = 0; j < control_max; j++)
					{
						m_iCTLGameRate[i][j] = iValue[i][j];
					}
				}
				_log(_DEBUG, "HZXLMJ_AIControl", "ReadParamValueConf 第[%d]档控制 [%d][%d][%d][%d][%d][%d]", i, m_iCTLGameRate[i][game_rate_low], m_iCTLGameRate[i][game_rate_up], m_iCTLGameRate[i][room_type], m_iCTLGameRate[i][amount_low], m_iCTLGameRate[i][amount_up], m_iCTLGameRate[i][control_rate]);
			}
		}
	}
	//字段校验
	if (m_iCTLGameNum <= 0 || m_iIfOpenAI == 0)
	{
		m_bIfOpen = false;
		return;
	}
	//多档控制校验
	bool bConfError = false;
	for (int i = 0; i < level_max; i++)
	{
		if (m_iCTLGameRate[i][game_rate_low] < 0 || m_iCTLGameRate[i][game_rate_low] > 200 || m_iCTLGameRate[i][game_rate_low] >= m_iCTLGameRate[i][game_rate_up])
		{
			_log(_ERROR, "ErmjAIControl", "ReadParamValueConf CTLGameRateLow Error [%d]GameRateLo=[%d]", i,m_iCTLGameRate[i][game_rate_low]);
			bConfError = true;
		}
		if (m_iCTLGameRate[i][game_rate_up] < 0 || m_iCTLGameRate[i][game_rate_up] > 200)
		{
			_log(_ERROR, "ErmjAIControl", "ReadParamValueConf CTLGameRateUp Error GameRateUp=[%d]", m_iCTLGameRate[i][game_rate_up]);
			bConfError = true;
		}
		if (m_iCTLGameRate[i][room_type] < 5 || m_iCTLGameRate[i][room_type] > 8)
		{
			_log(_ERROR, "ErmjAIControl", "ReadParamValueConf CTLRoomType Error RoomType=[%d]", m_iCTLGameRate[i][room_type]);
			bConfError = true;
		}
		if (m_iCTLGameRate[i][amount_low] < 0 || m_iCTLGameRate[i][amount_low] >= m_iCTLGameRate[i][amount_up])
		{
			_log(_ERROR, "ErmjAIControl", "ReadParamValueConf CTLJingFenLow Error JingFenLow=[%d]", m_iCTLGameRate[i][amount_low]);
			bConfError = true;
		}
		if (m_iCTLGameRate[i][amount_up] <= 0)
		{
			_log(_ERROR, "ErmjAIControl", "ReadParamValueConf CTLJingFenUp Error JingFenUp=[%d]", m_iCTLGameRate[i][amount_up]);
			bConfError = true;
		}
		if (m_iCTLGameRate[i][control_rate] < 0 || m_iCTLGameRate[i][control_rate] > 100)
		{
			_log(_ERROR, "ErmjAIControl", "ReadParamValueConf CTLRate Error CTLRate=[%d]", m_iCTLGameRate[i][control_rate]);
			bConfError = true;
		}
	}
	if (!bConfError)
	{
		_log(_DEBUG, "ErmjAIControl", "ReadParamValueConf DataRight");
		m_bIfOpen = true;
	}
}

/***
服务器模拟客户端，生成AI的换牌请求
***/
void HZXLMJ_AIControl::CreateAIChangeCardReq(MJGTableItemDef *tableItem, MJGPlayerNodeDef * pPlayers)
{
	//AI节点校验
	if (!pPlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CreateAIChangeCardReq_Log(): nodePlayers is NULL ●●●●");
		return;
	}
	if (pPlayers->iPlayerType <= NORMAL_PLAYER)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CreateAIChangeCardReq_Log(): nodePlayers Is Not AI-Node ●●●●");
		return;
	}
	//AI节点状态校验
	if (pPlayers->iStatus != PS_WAIT_CHANGE_CARD)
	{
		_log(_ERROR, "HZXLMJ_AIControl", " CreateAIChangeCardReq_Log(): Status Error=[%d]", pPlayers->iStatus);
		return;
	}
	if (pPlayers->bChangeCardReq)
	{
		_log(_ERROR, "HZXLMJ_AIControl", " CreateAIChangeCardReq_Log():  nodePlayer Already Changed Cards");
		return;
	}
	//AI不做换牌处理
	ChangeCardsReqDef msgChangeCardsReq;
	memset(&msgChangeCardsReq, 0, sizeof(ChangeCardsReqDef));
	CTLExchangeCardAI(tableItem, pPlayers, msgChangeCardsReq);
	CreateChangeCardAIMsg(tableItem, pPlayers, msgChangeCardsReq);
}
/***
服务器模拟客户端，生成AI定缺请求
***/
void HZXLMJ_AIControl::CreateAIConfirmQueReq(MJGTableItemDef* tableItem, MJGPlayerNodeDef * pPlayers)
{
	//安全校验
	if (!pPlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CreateAIConfirmQueReq_Log(): nodePlayers is NULL ●●●●");
		return;
	}
	if (pPlayers->iPlayerType <= NORMAL_PLAYER)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CreateAIConfirmQueReq_Log(): nodePlayers Is Not AI-Node ●●●●");
		return;
	}
	//AI节点状态校验
	if (pPlayers->iStatus != PS_WAIT_CONFIRM_QUE)
	{
		_log(_ERROR, "HZXLMJ_AIControl", " CreateAIConfirmQueReq_Log(): Status Error=[%d]", pPlayers->iStatus);
		return;
	}
	if (pPlayers->iQueType > 0)
	{
		_log(_ERROR, "HZXLMJ_AIControl", " CreateAIConfirmQueReq_Log(): nodePlayer Already ConfirmQue");
		return;
	}
	//AI的定缺
	ConfirmQueReqDef msgConfirmQueReq;
	memset(&msgConfirmQueReq, 0, sizeof(ConfirmQueReqDef));
	CTLConfirmQue(tableItem, pPlayers, msgConfirmQueReq);
	CreateConfirmQueAIMsg(tableItem, pPlayers, msgConfirmQueReq);
}
/***
服务器模拟客户端，生成AI的出牌请求
***/
void HZXLMJ_AIControl::CreateAISendCardReq(MJGTableItemDef * tableItem, MJGPlayerNodeDef * pPlayers)
{
	if (!pPlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CreateAISendCardReq_Log(): nodePlayers is NULL ●●●●");
		return;
	}
	if (pPlayers->iPlayerType <= NORMAL_PLAYER)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CreateAISendCardReq_Log(): nodePlayers Is Not AINodepPlayer ●●●●");
		return;
	}
	if (pPlayers->iStatus != PS_WAIT_SEND)
	{
		_log(_ERROR, "HZXLMJ_AIControl", " CreateAISendCardReq_Log(): Status Error=[%d]", pPlayers->iStatus);
		return;
	}
	/*if (pPlayers->cTableNumExtra != tableItem->iCurSendPlayer)
	{
		_log(_ERROR, "HZXLMJ_AIControl", " CreateAISendCardReq_Log(): nodePlayers Is Not CurSendPlayer nodePlayers[%d];iCurSendPlayer[%d]", pPlayers->cTableNumExtra, tableItem->iCurSendPlayer);
		return;
	}*/
	SendCardsReqDef msgSendCardReq;
	memset(&msgSendCardReq, 0, sizeof(SendCardsReqDef));
	char cIfTing;
	char cSendCard = CTLFindOptimalSendCard(tableItem, pPlayers, cIfTing);
	msgSendCardReq.cCard = cSendCard;
	msgSendCardReq.cIfTing = cIfTing;
	CreateSendCardAIMsg(tableItem, pPlayers, msgSendCardReq);
}
/***
服务器模拟客户端，生成AI的特殊操作请求(两种：自摸胡；碰杠+放炮)
***/
void HZXLMJ_AIControl::CreateAISpcialCardReq(MJGTableItemDef *tableItem, MJGPlayerNodeDef * pPlayers)
{
	if (!pPlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CreateAISpcialCardReq_Log(): nodePlayers is NULL ●●●●");
		return;
	}
	if (pPlayers->iPlayerType <= NORMAL_PLAYER)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CreateAISpcialCardReq_Log(): nodePlayers Is Not AINodepPlayer ●●●●");
		return;
	}
	pPlayers->iStatus = PS_WAIT_SPECIAL;

	SpecialCardsReqDef msgSpecialCardsReq;
	memset(&msgSpecialCardsReq, 0, sizeof(SpecialCardsReqDef));
	int iSpecialFlag = -1;
	char cFirstCard = -1;
	char cCards[4] = { 0 };

	CTLJudgeHandleSpecialCards(tableItem, pPlayers, iSpecialFlag, cFirstCard, cCards);
	//记录当前AI操作
	if (iSpecialFlag == -1)
	{
		//弃操作
		iSpecialFlag = 0;
	}
	msgSpecialCardsReq.cFirstCard = cFirstCard;
	msgSpecialCardsReq.iSpecialFlag = iSpecialFlag;
	memcpy(msgSpecialCardsReq.cCards, cCards, sizeof(cCards));
	_log(_DEBUG, "HZXLMJ_AIControl", " CreateAISpcialCardReq_Log(): iSpecialFlag [%d] ,cFirstCard[%d] cCards[0]", iSpecialFlag, cFirstCard, cCards[0]);
	CreateSpecialCardAIMsg(tableItem, pPlayers, msgSpecialCardsReq);
}



void HZXLMJ_AIControl::CreateChangeCardAIMsg(MJGTableItemDef *tableItem, MJGPlayerNodeDef *pPlayers, ChangeCardsReqDef &msgChangeCards)
{
	//随机换牌时间
	int iDelayTime = AI_DELAY_LEAST_TIME + rand() % AI_CHANGE_CARD_TIME;
	_log(_DEBUG, "HZXLMJ_AIControl", "CreateChangeCardAIMsg_Log() ChangeCards[%d][%d][%d],iDelayTime = %d", msgChangeCards.cCards[0], msgChangeCards.cCards[1], msgChangeCards.cCards[2], iDelayTime);
	//容错校验
	if (iDelayTime < 1 || iDelayTime > AI_CHANGE_CARD_TIME + 1)
	{
		iDelayTime = AI_CHANGE_CARD_TIME - 1;
	}
	//生成延迟出牌消息
	AIDelayCardMsgDef AIChangeCardMsg;
	memset(&AIChangeCardMsg, 0, sizeof(AIDelayCardMsgDef));
	AIChangeCardMsg.iDelayMilliSeconds = iDelayTime;
	AIChangeCardMsg.iMsgLen = sizeof(ChangeCardsReqDef);
	AIChangeCardMsg.iMsgType = AI_DELAY_TYPE_CHANGE_CARD;
	memcpy(AIChangeCardMsg.szBuffer, &msgChangeCards, sizeof(ChangeCardsReqDef));
	m_mapAIOperationDelayMsg[pPlayers].push_back(AIChangeCardMsg);
}

void HZXLMJ_AIControl::CreateConfirmQueAIMsg(MJGTableItemDef* tableItem, MJGPlayerNodeDef *pPlayers, ConfirmQueReqDef &msgConfirmQue)
{
	int iDelayTime = AI_DELAY_LEAST_TIME + rand() % AI_CONFIRM_QUE_TIME;
	_log(_DEBUG, "HZXLMJ_AIControl", "Create ConfirmQueAIMsg_Log() iQueType[%d],iDelayTime = %d", msgConfirmQue.iQueType, iDelayTime);

	if (iDelayTime<1 || iDelayTime>AI_CONFIRM_QUE_TIME)
	{
		iDelayTime = AI_CONFIRM_QUE_TIME - 1;
	}

	AIDelayCardMsgDef AIConfirmQueMsg;
	memset(&AIConfirmQueMsg, 0, sizeof(AIDelayCardMsgDef));
	AIConfirmQueMsg.iDelayMilliSeconds = iDelayTime;
	AIConfirmQueMsg.iMsgLen = sizeof(ConfirmQueReqDef);
	AIConfirmQueMsg.iMsgType = AI_DELAY_TYPE_CONFIRM_QUE;
	memcpy(AIConfirmQueMsg.szBuffer, &msgConfirmQue, sizeof(ConfirmQueReqDef));
	m_mapAIOperationDelayMsg[pPlayers].push_back(AIConfirmQueMsg);
}

void HZXLMJ_AIControl::CreateSendCardAIMsg(MJGTableItemDef * tableItem, MJGPlayerNodeDef * pPlayers, SendCardsReqDef msgSendCards)
{
	int iDelayTime = AI_DELAY_LEAST_TIME + rand() % AI_SEND_CARD_TIME;
	_log(_DEBUG, "HZXLMJ_AIControl", "CreateSendCardAIMsg_Log() cCard=[%d],cIfTing=[%d],iDelayTime = %d", msgSendCards.cCard, msgSendCards.cIfTing, iDelayTime);

	if (iDelayTime<1 || iDelayTime>AI_SEND_CARD_TIME)
	{
		iDelayTime = AI_SEND_CARD_TIME - 1;
	}
	//判断当前AI是否已经胡牌，如果已经胡牌延迟为1s;判断当前出牌是否是定缺牌，是的话出牌时间在3-6s
	if (pPlayers->bIsHu)
	{
		iDelayTime = 1;
		_log(_DEBUG, "HZXLMJ_AIControl", "CreateSendCardAIMsg_Log() pPlayers->bIsHu[%d] ,iDelayTime = %d", pPlayers->bIsHu,iDelayTime);
	}
	else
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "CreateSendCardAIMsg_Log() pPlayers->bIsHu[%d] ,cCardType= %d", pPlayers->bIsHu, msgSendCards.cCard >> 4);
		if (msgSendCards.cCard >> 4 == pPlayers->iQueType)
		{
			iDelayTime = 3 + rand() % 3;
			_log(_DEBUG, "HZXLMJ_AIControl", "CreateSendCardAIMsg_Log() pPlayers->bIsHu[%d] ,cCardType= %d ,iDelayTime ", pPlayers->bIsHu, msgSendCards.cCard >> 4,iDelayTime);
		}
	}
	AIDelayCardMsgDef AISendCardMsg;
	memset(&AISendCardMsg, 0, sizeof(AIDelayCardMsgDef));
	AISendCardMsg.iDelayMilliSeconds = iDelayTime;
	AISendCardMsg.iMsgLen = sizeof(SendCardsReqDef);
	AISendCardMsg.iMsgType = AI_DELAY_TYPE_SEND_CARD;
	memcpy(AISendCardMsg.szBuffer, &msgSendCards, sizeof(SendCardsReqDef));
	m_mapAIOperationDelayMsg[pPlayers].push_back(AISendCardMsg);
}

void HZXLMJ_AIControl::CreateSpecialCardAIMsg(MJGTableItemDef * tableItem, MJGPlayerNodeDef * pPlayers, SpecialCardsReqDef msgSpecialCard)
{
	int iDelayTime = AI_DELAY_LEAST_TIME + rand() % AI_SPECIAL_CARD_TIME;
	_log(_DEBUG, "HZXLMJ_AIControl", "CreateSpecialCardAIMsg_Log() iDelayTime = %d", iDelayTime);

	if (iDelayTime<1 || iDelayTime>AI_SPECIAL_CARD_TIME)
	{
		iDelayTime = AI_SPECIAL_CARD_TIME - 1;
	}
	//如果是已经胡牌后的继续胡牌，胡牌时间定死为2秒
	if (pPlayers->bIsHu)
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "CreateSpecialCardAIMsg_Log() pPlayers->bIsHu[%d]", pPlayers->bIsHu);
		if ((msgSpecialCard.iSpecialFlag >> 5) > 0)
		{
			iDelayTime = 3;
			_log(_DEBUG, "HZXLMJ_AIControl", "CreateSpecialCardAIMsg_Log() pPlayers->bIsHu[%d] iDelayTime[%d]", pPlayers->bIsHu,iDelayTime);
		}
	}

	AIDelayCardMsgDef AISpecialCardMsg;
	memset(&AISpecialCardMsg, 0, sizeof(AIDelayCardMsgDef));
	AISpecialCardMsg.iDelayMilliSeconds = iDelayTime;
	AISpecialCardMsg.iMsgLen = sizeof(SpecialCardsReqDef);
	AISpecialCardMsg.iMsgType = AI_DELAY_TYPE_SPECIAL_CARD;
	memcpy(AISpecialCardMsg.szBuffer, &msgSpecialCard, sizeof(SpecialCardsReqDef));
	m_mapAIOperationDelayMsg[pPlayers].push_back(AISpecialCardMsg);
}

/***
判断当前AI是否碰牌
***/
bool HZXLMJ_AIControl::CTLJudgeIfAIPlayerSuitPeng(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers)
{
	if (nodePlayers->iPlayerType == MIAN_AI_PLAYER)
	{
		//首先判断当前配置牌型是否可以碰
		if (nodePlayers->bIfAICanPeng && !nodePlayers->bIsHu)
		{
			//主AI-1的碰牌规则:只有主牌+缺牌能构成纯刻子才执行碰牌操作（纯刻子，是指不含红中的刻子）
			bool bIfPeng = false;
			int iKeZiSection = -1;
			for (int i = 0; i < 10; i++)
			{
				for (int j = 0; j < 10; j++)
				{
					if (nodePlayers->cMainCards[i][j] == tableItem->cTableCard)
					{
						//当前区间存在碰牌
						iKeZiSection = i;
						break;
					}
				}
				if (iKeZiSection != -1)
				{
					bIfPeng = CTLJudgeIsPureKeZi(tableItem, nodePlayers, iKeZiSection);
					if (bIfPeng)
					{
						//说明当前是可碰牌，需要将这张可碰牌从主AI-1的needcard中移除
						for (int i = 0; i < 10; i++)
						{
							for (int j = 0; j < 10; j++)
							{
								if (nodePlayers->cNeedCards[i][j] == tableItem->cTableCard)
								{
									nodePlayers->cNeedCards[i][j] == -1;
								}
							}
						}
						return true;
					}
					else
					{
						iKeZiSection = -1;
					}
				}
			}
			if (iKeZiSection == -1)
			{
				//此时没有找到可碰的对应区间
				return false;
			}
		}
	}
	else if (nodePlayers->iPlayerType == ASSIST_AI_PLAYER)
	{
		//处理主AI-2的碰牌逻辑，主AI-2的碰牌需要发生在cMainCards的缺牌刻子组合中
		return true;
	}
	else if (nodePlayers->iPlayerType == DEPUTY_AI_PLAYER)
	{
		//副AI主要负责做牌墙，咱不考虑其AI陪打功能
		return false;
	}
	return false;
}
bool HZXLMJ_AIControl::CTLJudgeIfAIPlayerSuitGang(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers, char &cCards)
{
	//明杠 杠的是的别人的出牌即ctableCard 补杠 补得是自己的摸牌 暗杠不同 暗杠需要单独判断返回暗杠的牌值
	int iType = nodePlayers->iSpecialFlag & 0x000C;
	if (nodePlayers->iPlayerType == MIAN_AI_PLAYER)
	{
		//判断当前配置牌型能否杠牌
		if (nodePlayers->bIfAICanGang)
		{
			if (iType == 0x0004)
			{
				//判断杠牌是否出现在单个区间
				vector<char>vecSectionNum;
				for (int i = 0; i < 10; i++)
				{
					for (int j = 0; j < 10; j++)
					{
						if (nodePlayers->cMainCards[i][j] == tableItem->cTableCard)
						{
							vecSectionNum.push_back(i);
							break;
						}
					}
				}
				if (vecSectionNum.size() == 1)
				{
					cCards = tableItem->cTableCard;
					return true;
				}
				else
				{
					return false;
				}
			}
			else if (iType == 0x0008)
			{
				//补杠判断,判断当前needcards中是否有这张牌
				vector<char>vecNeedCards;
				for (int i = 0; i < 10; i++)
				{
					for (int j = 0; j < 10; j++)
					{
						if (nodePlayers->cNeedCards[i][j] != -1)
						{
							vecNeedCards.push_back(nodePlayers->cNeedCards[i][j]);
						}
					}
				}
				if (find(vecNeedCards.begin(), vecNeedCards.end(), tableItem->cCurMoCard) == vecNeedCards.end())
				{
					//可以杠，并将该摸牌移除多余牌
					for (int i = 0; i < nodePlayers->vecRestCards.size(); i++)
					{
						if (nodePlayers->vecRestCards[i] == tableItem->cCurMoCard)
						{
							nodePlayers->vecRestCards.erase(nodePlayers->vecRestCards.begin() + i);
							break;
						}
					}
					cCards = tableItem->cCurMoCard;
					return true;
				}
				else
				{
					return false;
				}
			}
			else if (iType == 0x000C)
			{
				//暗杠，判断当前手里的四张暗杠牌是否只现在单个主区间
				char cAllCards[5][10];
				memset(cAllCards, 0, sizeof(cAllCards));
				for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
				{
					int iCardType = nodePlayers->cHandCards[i] >> 4;
					int iCardValue = nodePlayers->cHandCards[i] & 0xf;

					cAllCards[iCardType][0]++;
					cAllCards[iCardType][iCardValue]++;
				}
				//只判断条万筒的杠
				vector<char>vecAnGangCard;
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 10; j++)
					{
						if (cAllCards[i][j] == 4)
						{
							vecAnGangCard.push_back(j | i << 4);
						}
					}
				}
				for (int i = 0; i < vecAnGangCard.size(); i++)
				{
					vector<char>vecSectionNum;
					for (int k = 0; k < 10; k++)
					{
						for (int j = 0; j < 10; j++)
						{
							if (nodePlayers->cMainCards[k][j] == vecAnGangCard[i])
							{
								vecSectionNum.push_back(k);
								break;
							}
						}
					}
					if (vecSectionNum.size() == 1)
					{
						//如果杠的牌是摸到的牌，移除牌
						if (tableItem->cCurMoCard == vecAnGangCard[i])
						{
							for (int k = 0; k < 10; k++)
							{
								for (int j = 0; j < 10; j++)
								{
									if (nodePlayers->cNeedCards[k][j] == vecAnGangCard[i])
									{
										nodePlayers->cNeedCards[k][j] = -1;
									}
								}
							}
						}
						cCards = vecAnGangCard[i];
						return true;
					}
				}
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else if (nodePlayers->iPlayerType == ASSIST_AI_PLAYER)
	{
		if (iType == 0x0004)
		{
			//明杠，说明手上已有刻子，判断该刻子是否是主牌的刻子
			cCards = tableItem->cTableCard;
			return true;
		}
		else if (iType == 0x0008)
		{
			//补杠，说明是碰牌后的补杠，能碰则一定是主牌中的刻子，可以杠
			cCards = tableItem->cCurMoCard;
			return true;
		}
		else if (iType == 0x000C)
		{
			//暗杠，判断暗杠的牌是不是成牌中非听牌区间的刻子牌
			char cAllCards[5][10];
			memset(cAllCards, 0, sizeof(cAllCards));
			for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
			{
				int iCardType = nodePlayers->cHandCards[i] >> 4;
				int iCardValue = nodePlayers->cHandCards[i] & 0xf;

				cAllCards[iCardType][0]++;
				cAllCards[iCardType][iCardValue]++;
			}
			vector<char>vecAnGangCard;
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 10; j++)
				{
					if (cAllCards[i][j] == 4)
					{
						vecAnGangCard.push_back(j | i << 4);
					}
				}
			}
			//如果有多个暗杠每次只处理第一个暗杠
			cCards = vecAnGangCard[0];
			return true;
		}
		return false;
	}
	else if (nodePlayers->iPlayerType == DEPUTY_AI_PLAYER)
	{
		//暂不考虑陪打
		return false;
	}
	return false;
}
/***
判断当前AI是否胡牌
***/
bool HZXLMJ_AIControl::CTLJudgeIfAIPlayerSuitHu(MJGTableItemDef *tableItem, MJGPlayerNodeDef * nodePlayers)
{
	if (nodePlayers->iPlayerType == MIAN_AI_PLAYER)
	{
		//主AI-1的的胡牌控制，只有当缺牌全部补齐才能开始胡牌
		//获取当前主AI-1的缺牌
		vector<char>vecTmpNeedCard;
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				if (nodePlayers->cNeedCards[i][j] != -1)
				{
					vecTmpNeedCard.push_back(nodePlayers->cNeedCards[i][j]);
				}
			}
		}
		if (vecTmpNeedCard.size() != 0)
		{
			//当前主AI-1的缺牌还没有补完，不可以胡牌
			return false;
		}
		else
		{
			//nodePlayers->bIsHu = true;
			return true;
		}
	}
	else if (nodePlayers->iPlayerType == ASSIST_AI_PLAYER)
	{
		//AI-2 能胡就胡
		//判断当前胡牌是否是初次胡牌
		if (!nodePlayers->bIsHu)
		{
			//初次胡牌不胡红中，选择重新出牌
			if (tableItem->cCurMoCard == 65)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		return true;
	}
	else if (nodePlayers->iPlayerType == DEPUTY_AI_PLAYER)
	{
		return false;
	}
}
/***
主AI-1的纯刻子判断规则：
1.判断当前cMainCard区间是否已经构成三张成牌刻子，该情况下应该考虑杠而不是刻
2.判断当前CMainCard区间和对应的缺牌区间是否构成纯刻子，如果不是纯刻子不考虑碰牌
***/
bool HZXLMJ_AIControl::CTLJudgeIsPureKeZi(MJGTableItemDef *tableItem, MJGPlayerNodeDef * nodePlayers, int iKeZiSection)
{
	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CTLJudgeIsPureKeZi_Log(): nodePlayers is NULL ●●●●");
		return false;
	}
	if (nodePlayers->iPlayerType != MIAN_AI_PLAYER)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CTLJudgeIsPureKeZi_Log(): nodePlayers is not Main AI  ●●●●");
		return false;
	}
	//查找对应的缺牌区间
	int iMatchNeedCardSection = -1;
	for (int i = 0; i <= iKeZiSection; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (nodePlayers->cMainCards[i][j] == 99)
			{
				iMatchNeedCardSection++;
			}
		}
	}
	vector<char>vecTmpCards;
	int iCardCount = 0;
	for (int i = 0; i < 10; i++)
	{
		if (nodePlayers->cMainCards[iKeZiSection][i] != -1)
		{
			vecTmpCards.push_back(nodePlayers->cMainCards[iKeZiSection][i]);
		}
		if (nodePlayers->cMainCards[iKeZiSection][i] == tableItem->cTableCard)
		{
			iCardCount++;
		}
	}
	if (iCardCount == 3)
	{
		// 当前主牌区间已经是纯刻子，不考虑碰牌，考虑杠牌
		return false;
	}
	else
	{
		//对应区间cTableCard张数
		int iNeedCardCount = 0;
		for (int i = 0; i < 10; i++)
		{
			if (nodePlayers->cNeedCardsBackUp[iMatchNeedCardSection][i] == tableItem->cTableCard)
			{
				iNeedCardCount++;
			}
		}
		if (iCardCount == 1)
		{
			//对应的缺牌区间就应该有两张cTableCard
			if (iNeedCardCount != 2)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		else if (iCardCount == 2)
		{
			//对应的缺牌区间就应该只有一张cTableCard
			if (iNeedCardCount != 1)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
	}
}

std::vector<std::string> HZXLMJ_AIControl::Split(std::string strSource, std::string strPattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	strSource += strPattern;//扩展字符串以方便操作
	int iSize = strSource.size();

	for (int i = 0; i < iSize; i++)
	{
		pos = strSource.find(strPattern, i);
		if (pos < iSize)
		{
			std::string s = strSource.substr(i, pos - i);
			result.push_back(s);
			i = pos + strPattern.size() - 1;
		}
	}
	return result;
}

void HZXLMJ_AIControl::GetAIVirtualNickName(MJGTableItemDef *pTableItem, int iAIExtra, char * szNickName, int iStrLen)
{
	//当前桌上存在三个AI，需要注意名字不能重复
	for (int i = 0; i < 1; i++)
	{
		memset(szNickName, 0, iStrLen);

		int iNameType = rand() % 100;
		if (iNameType < 10)
		{
			strcpy(szNickName, "N");
			return;
		}
		else if (iNameType < 20 && m_szFirstNameText.size() > 0 && m_szLastNameText.size() > 0)
		{
			//纯汉字
			int iRand = rand() % m_szFirstNameText.size();
			memcpy(szNickName, m_szFirstNameText[iRand], strlen(m_szFirstNameText[iRand]));

			int iRandL = rand() % m_szLastNameText.size();
			memcpy(szNickName + strlen(m_szFirstNameText[iRand]), m_szLastNameText[iRandL], strlen(m_szLastNameText[iRandL]));
		}
		else if (iNameType < 35 && m_szLastNameText.size() > 0)
		{
			//汉字 + 英文
			int iRandL = rand() % m_szLastNameText.size();
			int iLen = strlen(m_szLastNameText[iRandL]);
			memcpy(szNickName, m_szLastNameText[iRandL], iLen);

			int iNameLen = 1 + rand() % 3;
			for (int m = 0; m < iNameLen; m++)
			{
				szNickName[iLen + m] = 'a' + rand() % 26;
			}
		}
		else if (iNameType < 60 && m_szLastNameText.size() > 0)
		{
			//汉字 + 数字
			int iRandL = rand() % m_szLastNameText.size();
			int iLen = strlen(m_szLastNameText[iRandL]);
			memcpy(szNickName, m_szLastNameText[iRandL], iLen);

			int iNameLen = 1 + rand() % 3;
			for (int m = 0; m < iNameLen; m++)
			{
				szNickName[iLen + m] = '0' + rand() % 10;
			}
		}
		else if (iNameType < 85)//英文 + 数字
		{
			//首位是英文(大写)
			szNickName[0] = 'A' + rand() % 26;
			//随机后面的英文和数字
			int iNameLen = 4 + rand() % 7;
			int iEngLen = 1 + rand() % iNameLen;
			int iNumLen = iNameLen - iEngLen;
			for (int m = 1; m < iEngLen; m++)
			{
				szNickName[m] = 'a' + rand() % 26;
			}
			for (int m = 0; m < iNumLen; m++)
			{
				szNickName[m + iEngLen] = '0' + rand() % 10;
			}
		}
		else //纯英文
		{

			int iNameLen = 4 + rand() % 8;
			for (int m = 0; m < iNameLen; m++)
			{
				if (m == 0)
					szNickName[m] = 'A' + rand() % 26;
				else
					szNickName[m] = 'a' + rand() % 26;
			}
		}
	}
	_log(_DEBUG, "HZXLMJ_AIControl", "GetAIVirtulNickName B [%s][%d][%d]", szNickName, strlen(szNickName), szNickName[strlen(szNickName)]);
}

void HZXLMJ_AIControl::GetAIVirtualArea(MJGTableItemDef * tableItem, char * szAreaNAme)
{
	int iRandArea = rand() % m_szAreaText.size();
	memcpy(szAreaNAme, m_szAreaText[iRandArea], strlen(m_szAreaText[iRandArea]));
}

long long HZXLMJ_AIControl::GetAIVirtualMoney(int iRoomID)
{
	long long iResultMoney = 0;
	long long iBaseScore = stSysConfBaseInfo.stRoom[iRoomID].iBasePoint;
	int iFengDingFan = stSysConfBaseInfo.stRoom[iRoomID].iMinPoint;
	_log(_DEBUG, "HZXLMJ_AIControl", "GetAIVirtualMoney_log(): iBaseScore[%d] iFengDingFan[%d]", iBaseScore, iFengDingFan);

	int iRandFan = 180 + rand() % 320;
	iResultMoney = iBaseScore*iRandFan*iFengDingFan;
	_log(_DEBUG, "HZXLMJ_AIControl", "GetAIVirtualMoney_log(): iRoomID[%d] iRandFan[%d] iResultMoney[%lld]", iRoomID, iRandFan, iResultMoney);
	return iResultMoney;
}

int HZXLMJ_AIControl::GetAIVirtualLevel()
{
	float fExp = 0;
	int iRand = rand() % 100;
	if (iRand < 95)
	{
		fExp = 15 + rand() % 9;
	}
	else
	{
		fExp = 23 + rand() % 9;
	}
	return fExp;
}

void HZXLMJ_AIControl::ReadAINickNameInfoConf()
{
	m_szFirstNameText.clear();
	int iLen = 0;
	char buf[1024] = { 0 };
	FILE* fp = fopen("ai_first_name.conf", "rb");
	if (fp)
	{
		while (fgets(buf, 1024, fp))
		{
			iLen = strlen(buf);
			buf[iLen - 1] = 0;					//去掉末尾的换行符

			char* pBuff = new char[iLen];
			memset(pBuff, 0, iLen);
			memcpy(pBuff, buf, iLen - 2);

			m_szFirstNameText.push_back(pBuff);
		}
		fclose(fp);
	}

	m_szLastNameText.clear();
	fp = fopen("ai_last_name.conf", "rb");
	if (fp)
	{
		while (fgets(buf, 1024, fp))
		{
			iLen = strlen(buf);
			buf[iLen - 1] = 0;					//去掉末尾的换行符

			char* pBuff = new char[iLen];
			memset(pBuff, 0, iLen);
			memcpy(pBuff, buf, iLen - 2);
			
			m_szLastNameText.push_back(pBuff);
		}
		fclose(fp);
	}
	
}

void HZXLMJ_AIControl::ReadAIAreaInfoConf()
{
	m_szAreaText.clear();
	int iLen = 0;
	char buf[1024] = { 0 };
	FILE* fp = fopen("ai_area_name.conf", "rb");
	if (fp)
	{
		while (fgets(buf, 1024, fp))
		{
			iLen = strlen(buf);
			buf[iLen - 1] = 0;					//去掉末尾的换行符

			char* pBuff = new char[iLen];
			memset(pBuff, 0, iLen);
			memcpy(pBuff, buf, iLen - 2);

			m_szAreaText.push_back(pBuff);
		}
		fclose(fp);
	}
	
	for (int i = 0; i < m_szAreaText.size(); i++)
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "ReadAIAreaInfoConf_log():[%s]", m_szAreaText[i]);
	}
}


void HZXLMJ_AIControl::SetPlayerIfControl(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers, HZXLControlCards & CtlCard)
{
	nodePlayers->bIfControl = true;
}

bool HZXLMJ_AIControl::JudgePlayerBeforeSit(MJGPlayerNodeDef * nodePlayers)
{
	//暂时先只匹配AI
	//return true;
	if (!m_bIfOpen)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "JudgePlayerBeforeSit AI IfOpen:[%d]", m_bIfOpen);
		return false;
	}
	else
	{
		if (m_vecCTLCards.size() == 0)
		{
			_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit m_vecCTLCards.size:[%d]", m_vecCTLCards.size());
			return false;
		}
		_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit AI IfOpen:[%d]", m_bIfOpen);
	}
	//玩家当日局数校验
	if (nodePlayers->iCurDayPlayNum < m_iCTLGameNum)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "JudgePlayerBeforeSit GameNum Error UserID[%d]NowDayNum=[%d],CTLNum=[%d]", nodePlayers->iUserID, nodePlayers->iCurDayPlayNum, m_iCTLGameNum);
		return false;
	}
	else
	{
		_log(_ERROR, "HZXLMJ_AIControl", "JudgePlayerBeforeSit GameNum UserID[%d]NowDayNum=[%d],CTLNum=[%d]", nodePlayers->iUserID, nodePlayers->iCurDayPlayNum, m_iCTLGameNum);
	}
	//玩家游戏几率,净分校验
	int iCurGameRate = 2 * nodePlayers->iCurDayWinMoney * 100 / (nodePlayers->iCurDayWinMoney - nodePlayers->iCurDayLoseMoney);
	_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit UserID[%d],iCurGameRate:[%d] iCurDayWinMoney[%d]iCurDayLoseMoney[%d] ", nodePlayers->iUserID, iCurGameRate, nodePlayers->iCurDayWinMoney, nodePlayers->iCurDayLoseMoney);
	int iCurJingFen = (nodePlayers->iCurDayWinMoney + nodePlayers->iCurDayLoseMoney) / 10000;
	_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit UserID[%d],iCurJingFen:[%d]", nodePlayers->iUserID, iCurJingFen);
	//判断当前的游戏几率是否需要进入控制
	int iGameCTLRate = 0;
	int iJingFenCTLRate = 0;

	//判断玩家当前游戏几率所在区间
	for (int i = 0; i < level_max; i++)
	{
		if (iCurGameRate >= m_iCTLGameRate[i][game_rate_low] && iCurGameRate <= m_iCTLGameRate[i][game_rate_up])
		{
			iGameCTLRate = m_iCTLGameRate[i][control_rate];
			_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit UserID[%d], Current GameRate Interval:[%d][%d]", nodePlayers->iUserID, m_iCTLGameRate[i][game_rate_low], m_iCTLGameRate[i][game_rate_up]);
			break;
		}
	}
	if (iGameCTLRate == 0)
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit UserID[%d],iCurGameRate Not Qualified :[%d]", nodePlayers->iUserID, iCurGameRate);
	}
	//判断玩家当前的净分所在区间，注意要考虑到房间类型
	for (int i = 0; i < level_max; i++)
	{
		if (nodePlayers->iEnterRoomType >= m_iCTLGameRate[i][room_type])
		{
			if (iCurJingFen >= m_iCTLGameRate[i][amount_low] && iCurJingFen < m_iCTLGameRate[i][amount_up])
			{
				iJingFenCTLRate = m_iCTLGameRate[i][control_rate];
				_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit UserID[%d], Current Jingfen Interval:[%d][%d]", nodePlayers->iUserID, m_iCTLGameRate[i][game_rate_low], m_iCTLGameRate[i][game_rate_up]);
				break;
			}
		}
	}
	if (iJingFenCTLRate == 0)
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit UserID[%d],iJingFenCTLRate Not Qualified :[%d]", nodePlayers->iUserID, iJingFenCTLRate);
	}
	//判断当前几率、净分是否使得玩家进入控制
	int iRandGameRate = rand() % 100;
	int iRandJingFenRate = rand() % 100;
	if (iRandGameRate <= iGameCTLRate || iRandJingFenRate <= iJingFenCTLRate)
	{
		//游戏几率、净分满足一个就进入控制
		_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit UserID[%d],Accecc AI CTL iCurGameRate[%d] iCurJingFen[%d] iRandGameRate[%d] iRandJingFenRate[%d]", nodePlayers->iUserID, iCurGameRate, iCurJingFen, iRandGameRate, iRandJingFenRate);
		return true;
	}
	else
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "JudgePlayerBeforeSit UserID[%d] Accecc AI Failed  iCurGameRate[%d] iCurJingFen[%d] iRandGameRate[%d] iRandJingFenRate[%d]", nodePlayers->iUserID, iCurGameRate, iCurJingFen, iRandGameRate, iRandJingFenRate);
		return false;
	}
}

void HZXLMJ_AIControl::SetControlCard(vector<HZXLControlCards>vecCTLCards)
{
	//清除当前牌库缓存
	m_vecCTLCards.clear();
	//处理vector数据，判断每副牌是否是合格手牌
	bool bIfQualified = true;
	int iCurNum = 0;
	std::vector<HZXLControlCards>::iterator it;
	for (it = vecCTLCards.begin(); it != vecCTLCards.end();)
	{
		bIfQualified = AnalysisControlCard(it->cMainCard, it->cNeedCard, it->cAIChi, it->cAIPeng, it->cAIGang);
		if (!bIfQualified)
		{
			iCurNum++;
			_log(_ERROR, "SetControlCards", "SetControlCards iCurNum[%d]", iCurNum);
			it = vecCTLCards.erase(it);
		}
		else
		{
			iCurNum++;
			//获取数据打印
			/*_log(_ERROR, "SetControlCards", "SetControlCards cAIChi[%d]cAIPeng[%d]cAIGang[%d]",it->cAIChi, it->cAIPeng, it->cAIGang);
			for (int i = 0; i < strlen(it->cMainCard); i++)
			{
				_log(_ERROR, "SetControlCards", "SetControlCards cMainCard[%c]", it->cMainCard[i]);
			}
			for (int i = 0; i < strlen(it->cNeedCard); i++)
			{
				_log(_ERROR, "SetControlCards", "SetControlCards cNeedCard[%c]", it->cNeedCard[i]);
			}*/
			it++;
		}
	}
	//校验是否剩余可用手牌组
	if (vecCTLCards.size() == 0)
	{
		_log(_ERROR, "SetControlCards", "SetControlCards error:no Qualified ControlCards ");
		return;
	}
	for (int i = 0; i < vecCTLCards.size(); i++)
	{
		m_vecCTLCards.push_back(vecCTLCards[i]);
	}
	_log(_DEBUG, "SetControlCards", "SetControlCards Qualified ControlCards Num=[%d] ", m_vecCTLCards.size());
}

bool HZXLMJ_AIControl::AnalysisControlCard(char cMainCards[], char cNeedCards[], char cAIChi, char cAIPeng, char cAIGang)
{
	//安全校验各个字段是否合格
	//主牌校验
	int iCurSection = 0;
	int iCurCard = 0;
	char iSinglePart[10][10];
	memset(&iSinglePart, 0, sizeof(iSinglePart));
	for (int i = 0; i < strlen(cMainCards); i++)
	{
		//_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard : cMainCards[%c]", cMainCards[i]);
		if (cMainCards[i] == '_')
		{
			//以下划线区分各个区间
			iCurSection++;
			iCurCard = 0;
		}
		else
		{
			iSinglePart[iCurSection][iCurCard] = cMainCards[i];
			iCurCard++;
		}
	}
	//校验iCurSection值
	if (iCurSection != 4)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard iCurSection Error: iCurSection=[%d]", iCurSection);
		return false;
	}
	//校验单组
	int iNeedCardNum = 0;					//统计主牌中的缺牌张数
	for (int i = 0; i <= iCurSection; i++)
	{
		int iSpacerNum = 0;
		for (int j = 0; j < 10; j++)
		{
			//_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard iCurSection Error: [%c]", iSinglePart[i][j]);
			if (iSinglePart[i][j] == '-')
			{
				iSpacerNum++;
			}
			if (iSinglePart[i][j] == '/')
			{
				break;
			}
		}
		int iAnalysisNum[10] = { 0 };
		if (iSpacerNum == 1)
		{
			//将牌组校验
			int iItemNum = sscanf(iSinglePart[i], "%d-%d", &iAnalysisNum[0], &iAnalysisNum[1]);
			if (iItemNum != 2)
			{
				_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard JiangCardNum Error: JiangCardNum=[%d]", iItemNum);
				return false;
			}
			else
			{
				for (int j = 0; j < iItemNum; j++)
				{
					if (iAnalysisNum[j] == 99)
					{
						iNeedCardNum++;
					}
				}
				//校验将牌是否是成对,如果将牌组两张牌都不是缺牌
				if (iAnalysisNum[0] != iAnalysisNum[1])
				{
					//如果将牌组中不存在缺牌或红中
					if (iAnalysisNum[0] != 99 && iAnalysisNum[1] != 99 && iAnalysisNum[0] != 31 && iAnalysisNum[1] != 31)
					{
						_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard JiangCard Error: JiangCard=[%d][%d]", iAnalysisNum[0], iAnalysisNum[1]);
						return false;
					}
				}
			}
		}
		else if (iSpacerNum == 2)
		{
			//刻子or顺子校验
			int iItemNum = sscanf(iSinglePart[i], "%d-%d-%d", &iAnalysisNum[0], &iAnalysisNum[1], &iAnalysisNum[2]);
			if (iItemNum != 3)
			{
				_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard ZuCardNum Error: ZuCardNum=[%d]", iItemNum);
				return false;
			}
			else
			{
				for (int j = 0; j < iItemNum; j++)
				{
					if (iAnalysisNum[j] == 99)
					{
						iNeedCardNum++;
					}
				}

				bool bIfQualified = true;
				vector<int>vecTmpSinglePartCard;
				//校验是否成刻子or顺子
				for (int j = 0; j < iItemNum; j++)
				{
					if (iAnalysisNum[j] != 31 && iAnalysisNum[j] != 99)
					{
						//取出非空牌、非红中牌
						vecTmpSinglePartCard.push_back(iAnalysisNum[j]);
					}
				}
				bIfQualified = JudegIfSinglePartCardlegal(vecTmpSinglePartCard);
				if (!bIfQualified)
				{
					_log(_ERROR, "HZXLMJ_AIControl", "JudegIfSinglePartCardlegal: Retrun false");
					return false;
				}
			}
		}
		else if (iSpacerNum == 3)
		{
			//杠牌校验
			int iItemNum = sscanf(iSinglePart[i], "%d-%d-%d-%d", &iAnalysisNum[0], &iAnalysisNum[1], &iAnalysisNum[2], &iAnalysisNum[3]);
			if (iItemNum != 4)
			{
				_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard ZuCardNum Error: ZuCardNum=[%d]", iItemNum);
				return false;
			}

			for (int j = 0; j < iItemNum; j++)
			{
				if (iAnalysisNum[j] == 99)
				{
					iNeedCardNum++;
				}
			}

			vector<int>vecTmpSinglePartCard;
			//校验是否成杠牌
			for (int j = 0; j < iItemNum; j++)
			{
				if (iAnalysisNum[j] != '31' && iAnalysisNum[j] != '99')
				{
					//取出非空牌、非红中牌
					vecTmpSinglePartCard.push_back(iAnalysisNum[j]);
				}
			}
			sort(vecTmpSinglePartCard.begin(), vecTmpSinglePartCard.end());
			if (vecTmpSinglePartCard.size() != 0)
			{
				for (int j = 0; j < vecTmpSinglePartCard.size(); j++)
				{
					if (vecTmpSinglePartCard[j] != vecTmpSinglePartCard[0])
					{
						//杠牌中不能存在不同牌值的牌
						_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard SinglePart Gang-Check Error", iItemNum);
						return false;
					}
				}
			}
		}
		else
		{
			//组牌错误
			_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard SinglePartCard Error iSpacerNum[%d]iCurSection[%d] [%d][%d][%d]", iSpacerNum, iCurSection, iSinglePart[iCurSection][0], iSinglePart[iCurSection][1], iSinglePart[iCurSection][2]);
			return false;
		}
	}

	//缺牌校验
	int iNeedCurSection = 0;
	int iNeedCurCard = 0;
	char iNeedCard[10][10];
	memset(&iNeedCard, 0, sizeof(iNeedCard));
	int iNeedCardCount = 0;
	for (int i = 0; i < strlen(cNeedCards); i++)
	{
		if (cNeedCards[i] == '_')
		{
			iNeedCurSection++;
			iNeedCurCard = 0;
		}
		else
		{
			iNeedCard[iNeedCurSection][iNeedCurCard] = cNeedCards[i];
			iNeedCurCard++;
		}
	}
	for (int i = 0; i <= iNeedCurSection; i++)
	{
		int iSpacerNum = 0;
		for (int j = 0; j < 10; j++)
		{
			if (iNeedCard[i][j] == '-')
			{
				iSpacerNum++;
			}
			if (iNeedCard[i][j] == '/')
			{
				break;
			}
		}
		if (iSpacerNum == 0)
		{
			//当前缺牌区间只有一张缺牌
			iNeedCardCount++;
		}
		else if (iSpacerNum == 1)
		{
			iNeedCardCount += 2;
		}
		else if (iSpacerNum == 2)
		{
			iNeedCardCount += 3;
		}
		else if (iSpacerNum == 3)
		{
			iNeedCardCount += 4;
		}
	}
	if (iNeedCardCount != iNeedCardNum - 1)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard NeedCards Number Does Not Match  InMain=[%d] ,InNeed=[%d]", iNeedCardNum, iNeedCardCount);
		return false;
	}
	//能否吃碰杠校验，血流红中不能吃
	if (cAIChi == 1)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "AnalysisControlCard cAIChi Error=[%d]", cAIChi);
		return false;
	}
	return true;
}

/***
判断非将牌区间的组牌区间的顺子/刻子的合法性
***/
bool HZXLMJ_AIControl::JudegIfSinglePartCardlegal(vector<int> vecSinglePart)
{
	if (vecSinglePart.size() == 0 || vecSinglePart.size() == 1)
	{
		//当前区间的非空牌和非红中牌只有 1or0 张 当前区间必定是合法的
		_log(_DEBUG, "HZXLMJ_AIControl", "JudegIfSinglePartCardlegal vecSinglePart.size=[%d]", vecSinglePart.size());
		return true;
	}
	else if (vecSinglePart.size() == 2)
	{
		//当前区间的非空牌和非红中牌有两张，判断是否为连张、卡当or同张(值相同)
		if ((vecSinglePart[0] / 9) != (vecSinglePart[1] / 9))
		{
			//如果两张牌不是同一花色
			_log(_ERROR, "HZXLMJ_AIControl", "JudegIfSinglePartCardlegal Two Cards HuaSe is Different Card1=[%d],Card2=[%d]", vecSinglePart[0], vecSinglePart[1]);
			return false;
		}
		else
		{
			if (vecSinglePart[0] == vecSinglePart[1])
			{
				//同张则一定可以构成刻子
				_log(_DEBUG, "HZXLMJ_AIControl", "JudegIfSinglePartCardlegal Two Cards is Same", vecSinglePart.size());
				return true;
			}
			else
			{
				//升序排序
				sort(vecSinglePart.begin(), vecSinglePart.end());
				if (vecSinglePart[0] == vecSinglePart[1] - 1 || vecSinglePart[0] == vecSinglePart[1] - 2)
				{
					return true;
				}
				else
				{
					_log(_ERROR, "HZXLMJ_AIControl", "JudegIfSinglePartCardlegal Two Cards Is Not Shun or Ke Card1=[%d],Card2=[%d]", vecSinglePart[0], vecSinglePart[1]);
					return false;
				}
			}
		}
	}
	else if (vecSinglePart.size() == 3)
	{
		//三张牌齐全的情况下判断是否成顺/刻
		//先判断是否在同一区间
		if ((vecSinglePart[0] / 9) != (vecSinglePart[1] / 9) || (vecSinglePart[0] / 9) != (vecSinglePart[2] / 9) || (vecSinglePart[1] / 9) != (vecSinglePart[2] / 9))
		{
			_log(_ERROR, "HZXLMJ_AIControl", "JudegIfSinglePartCardlegal Three Cards HuaSe is Different Card1=[%d],Card2=[%d],Card3=[%d]", vecSinglePart[0], vecSinglePart[1], vecSinglePart[3]);
			return false;
		}
		else
		{
			//先判断是否成刻子
			if (vecSinglePart[0] == vecSinglePart[1] && vecSinglePart[0] == vecSinglePart[2])
			{
				return true;
			}
			else
			{
				//判断是否成顺子,升序排序
				sort(vecSinglePart.begin(), vecSinglePart.end());
				if (vecSinglePart[0] == vecSinglePart[1] - 1 && vecSinglePart[2] == vecSinglePart[1] + 1)
				{
					return true;
				}
				else
				{
					_log(_ERROR, "HZXLMJ_AIControl", "JudegIfSinglePartCardlegal Three Cards Is Not Shun or Ke Card1=[%d],Card2=[%d],Card3=[%d]", vecSinglePart[0], vecSinglePart[1], vecSinglePart[2]);
					return false;
				}
			}
		}
	}
	else
	{
		//2个破折号 - 最多传入三张牌
		_log(_ERROR, "HZXLMJ_AIControl", "JudegIfSinglePartCardlegal vecSinglePart.Size Error=[%d]", vecSinglePart.size());
		return false;
	}
}
/***
配牌原则：只配置AI-1的牌，控制玩家不会拿到红中，AI-2、AI-3不再做控制
***/
void HZXLMJ_AIControl::CTLSetControlCard(MJGTableItemDef * tableItem, int iCTLCardsNumber)
{
	//处理玩家节点
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		tableItem->pPlayers[i]->bIfControl = true;
		memset(&tableItem->pPlayers[i]->cMainCards, -1, sizeof(tableItem->pPlayers[i]->cMainCards));
		memset(&tableItem->pPlayers[i]->cNeedCards, -1, sizeof(tableItem->pPlayers[i]->cNeedCards));
		tableItem->pPlayers[i]->vecRestCards.clear();
	}
	//找到当前桌的主AI-1节点，其iPlayerType=1
	int iMainAIPos = -1;
	int iAssistAIPos = -1;
	int iDeputyAIPos = -1;
	int iNormalPlayerPos = -1;
	
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (tableItem->pPlayers[i]->iPlayerType == MIAN_AI_PLAYER)
		{
			iMainAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		else if (tableItem->pPlayers[i]->iPlayerType == ASSIST_AI_PLAYER)
		{
			iAssistAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		else if (tableItem->pPlayers[i]->iPlayerType == DEPUTY_AI_PLAYER)
		{
			iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		else if (tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
		{
			iNormalPlayerPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		
	}
	//获取对应牌库
	HZXLControlCards CTLControlCard;
	memcpy(&CTLControlCard, &m_vecCTLCards[iCTLCardsNumber], sizeof(HZXLControlCards));
	//配置能否吃碰杠
	if (CTLControlCard.cAIChi == 48) 
	{
		tableItem->pPlayers[iMainAIPos]->bIfAICanChi = true;
	}
	else if (CTLControlCard.cAIChi == 49)
	{
		tableItem->pPlayers[iMainAIPos]->bIfAICanChi = false;
	}
	if (CTLControlCard.cAIPeng == 48)
	{
		tableItem->pPlayers[iMainAIPos]->bIfAICanPeng = true;
	}
	else if (CTLControlCard.cAIPeng == 49)
	{
		tableItem->pPlayers[iMainAIPos]->bIfAICanPeng = false;
	}
	if (CTLControlCard.cAIGang == 48)
	{
		tableItem->pPlayers[iMainAIPos]->bIfAICanGang = true;
	}
	else if (CTLControlCard.cAIGang == 49)
	{
		tableItem->pPlayers[iMainAIPos]->bIfAICanGang = false;
	}

	//解析CTLControlcard
	int iMainCurSection = 0;											//主牌区间数
	int iMainCurCard = 0;												//主牌在当前区间的位置
	char iMainCardSinglePart[10][10];									//按区间拆分各组牌
	memset(&iMainCardSinglePart, -1, sizeof(iMainCardSinglePart));
	for (int i = 0; i < strlen(CTLControlCard.cMainCard); i++)
	{
		if (CTLControlCard.cMainCard[i] == '_')
		{
			//以下划线区分各个区间
			iMainCurSection++;
			iMainCurCard = 0;
		}
		else if (CTLControlCard.cMainCard[i] == '/')
		{
			break;
		}
		else
		{
			iMainCardSinglePart[iMainCurSection][iMainCurCard] = CTLControlCard.cMainCard[i];
			iMainCurCard++;
		}
	}
	//处理并存放单个区间数据, 主牌
	for (int i = 0; i <= iMainCurSection; i++)
	{
		int iSpacerNum = 0;
		int iEndPos = 0;
		for (int j = 0; j < 10; j++)
		{
			if (iMainCardSinglePart[i][j] == '-')
			{
				iSpacerNum++;
			}
			if (iMainCardSinglePart[i][j] == -1)
			{
				iEndPos = j;
				break;
			}
		}
		int iAnalysisNum[10] = { 0 };
		//将牌组数据处理
		if (iSpacerNum == 1)
		{
			int iItemNum = sscanf(iMainCardSinglePart[i], "%d-%d", &iAnalysisNum[0], &iAnalysisNum[1]);
			for (int j = 0; j < 2; j++)
			{
				tableItem->pPlayers[iMainAIPos]->cMainCards[i][j] = MakeCardNumToChar(iAnalysisNum[j]);
			}
		}
		//顺/刻组数据处理
		else if (iSpacerNum == 2)
		{
			int iItemNum = sscanf(iMainCardSinglePart[i], "%d-%d-%d", &iAnalysisNum[0], &iAnalysisNum[1], &iAnalysisNum[2]);
			for (int j = 0; j < 3; j++)
			{
				tableItem->pPlayers[iMainAIPos]->cMainCards[i][j] = MakeCardNumToChar(iAnalysisNum[j]);
			}
		}
		//杠牌组数据处理
		else
		{
			int iItemNum = sscanf(iMainCardSinglePart[i], "%d-%d-%d-%d", &iAnalysisNum[0], &iAnalysisNum[1], &iAnalysisNum[2], &iAnalysisNum[3]);
			for (int j = 0; j < 4; j++)
			{
				tableItem->pPlayers[iMainAIPos]->cMainCards[i][j] = MakeCardNumToChar(iAnalysisNum[j]);
			}
		}
	}
	//生成所有牌;
	char cAllCard[66];
	memset(&cAllCard, 0, sizeof(cAllCard));
	//存入四张万筒条
	for (int j = 1; j <= 9; j++)
	{
		for (int k = 0; k <= 2; k++)
		{
			int iCardType = k;
			int iCardValue = j;
			char cTmpCard = iCardValue | (iCardType << 4);
			cAllCard[cTmpCard] = 4;
		}
	}
	//存入四张红中
	char cTmpHZCard = 1 | (4 << 4);
	cAllCard[cTmpHZCard] = 4;
	//将AI-1的主要牌从牌池中移除一张
	for (int i = 0; i <= iMainCurSection; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (tableItem->pPlayers[iMainAIPos]->cMainCards[i][j] != 99 && tableItem->pPlayers[iMainAIPos]->cMainCards[i][j] != -1)
			{
				char cCard = tableItem->pPlayers[iMainAIPos]->cMainCards[i][j];
				if (cCard != 0)
				{
					cAllCard[cCard]--;
					_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard  Final AI-1 MainCard=[%d]", tableItem->pPlayers[iMainAIPos]->cMainCards[i][j]);
				}
			}
		}
	}
	// 处理AI-1的缺牌部分
	int iNeedCurSection = 0;
	int iNeedCurCard = 0;
	char iNeedSingleCard[10][10];
	memset(&iNeedSingleCard, -1, sizeof(iNeedSingleCard));
	for (int i = 0; i < strlen(CTLControlCard.cNeedCard); i++)
	{
		if (CTLControlCard.cNeedCard[i] == '_')
		{
			//以下划线区分各个区间
			iNeedCurSection++;
			iNeedCurCard = 0;
		}
		else if (CTLControlCard.cNeedCard[i] == '/')
		{
			break;
		}
		else
		{
			iNeedSingleCard[iNeedCurSection][iNeedCurCard] = CTLControlCard.cNeedCard[i];
			iNeedCurCard++;
		}
	}
	//将牌库中的缺牌存入主AI-1节点
	for (int i = 0; i <=iNeedCurSection; i++)
	{
		int iSpacerNum = 0;
		for (int j = 0; j < 10; j++)
		{
			if (iNeedSingleCard[i][j] == '-')
			{
				iSpacerNum++;
			}
			if (iNeedSingleCard[i][j] == -1)
			{
				break;
			}
		}
		int iAnalysisNum[10] = { 0 };
		//单张缺牌处理
		if (iSpacerNum == 0)
		{
			//不存在分割符 - 则当前组别为单张缺牌
			int iItemNum = sscanf(iNeedSingleCard[i], "%d", &iAnalysisNum[0]);
			tableItem->pPlayers[iMainAIPos]->cNeedCards[i][0] = MakeCardNumToChar(iAnalysisNum[0]);
		}
		else if (iSpacerNum == 1)
		{
			//当前组别为缺牌两张
			int iItemNum = sscanf(iNeedSingleCard[i], "%d-%d", &iAnalysisNum[0], &iAnalysisNum[1]);
			for (int j = 0; j < 2; j++)
			{
				tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j] = MakeCardNumToChar(iAnalysisNum[j]);
			}
		}
		else if (iSpacerNum == 2)
		{
			int iItemNum = sscanf(iNeedSingleCard[i], "%d-%d-%d", &iAnalysisNum[0], &iAnalysisNum[1], &iAnalysisNum[2]);
			for (int j = 0; j < 3; j++)
			{
				tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j] = MakeCardNumToChar(iAnalysisNum[j]);
			}
		}
		else if (iSpacerNum == 3)
		{
			int iItemNum = sscanf(iNeedSingleCard[i], "%d-%d-%d-%d", &iAnalysisNum[0], &iAnalysisNum[1], &iAnalysisNum[2], &iAnalysisNum[3]);
			for (int j = 0; j < 4; j++)
			{
				tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j] = MakeCardNumToChar(iAnalysisNum[j]);
			}
		}
	}
	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard_Log() Middle");
	//缺牌备份
	memcpy(&tableItem->pPlayers[iMainAIPos]->cNeedCardsBackUp, tableItem->pPlayers[iMainAIPos]->cNeedCards, sizeof(tableItem->pPlayers[iMainAIPos]->cNeedCards));
	//将对应缺牌从牌池中移除一份
	for (int i = 0; i <= iNeedCurSection; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j] != -1)
			{
				char cCard = tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j];
				if (cCard != 0)
				{
					cAllCard[cCard]--;
					//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard Final AI-1 NeedCard=[%d]", tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j]);
				}
			}
		}
	}
	//判断主AI-1的定缺花色
	int iTmpHuaseArr[3];
	memset(&iTmpHuaseArr, 0, sizeof(iTmpHuaseArr));
	int iMainAIQue = -1;
	//获取当前AI-1的主牌
	vector<char>vecTmpMainAIMainCard;
	for (int i = 0; i <=iMainCurSection; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (tableItem->pPlayers[iMainAIPos]->cMainCards[i][j] != -1 && tableItem->pPlayers[iMainAIPos]->cMainCards[i][j] != 99)
			{
				vecTmpMainAIMainCard.push_back(tableItem->pPlayers[iMainAIPos]->cMainCards[i][j]);
			}
		}
	}
	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard Final AI-1 MainCards.size=[%d]", vecTmpMainAIMainCard.size());
	vector<char>vecTmpMainAINeedCard;
	for (int i = 0; i <=iNeedCurSection; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j] != -1)
			{
				vecTmpMainAINeedCard.push_back(tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j]);
			}
		}
	}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard Final AI-1 NeedCards.size=[%d]", vecTmpMainAINeedCard.size());
	//获取AI-1的定缺花色
	for (int i = 0; i < vecTmpMainAIMainCard.size(); i++)
	{
		if (vecTmpMainAIMainCard[i] != 0)
		{
			iTmpHuaseArr[(vecTmpMainAIMainCard[i] >> 4)]++;
		}
	}
	for (int i = 0; i < vecTmpMainAINeedCard.size(); i++)
	{
		if (vecTmpMainAINeedCard[i] != 0)
		{
			iTmpHuaseArr[(vecTmpMainAINeedCard[i] >> 4)]++;
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (iTmpHuaseArr[i] == 0)
		{
			iMainAIQue = i;
			_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard MainAI-Que=[%d]", iMainAIQue);
			break;
		}
	}
	tableItem->pPlayers[iMainAIPos]->iAIConfirmQueType = iMainAIQue;
	//得到主AI-1的所有可胡牌
	vector<char>vecMainAIHuCard;
	CalMainAIHuCard(tableItem->pPlayers[iMainAIPos], vecMainAIHuCard);
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard vecMainAIHuCard.szie()=[%d]", vecMainAIHuCard.size());
	//将这部分牌从牌池中移除一张
	for (int i = 0; i < vecMainAIHuCard.size(); i++)
	{
		char cCard = vecMainAIHuCard[i];
		cAllCard[cCard]--;
		//将之存入主AI-1节点
		tableItem->pPlayers[iMainAIPos]->vecAIHuCard.push_back(vecMainAIHuCard[i]);
	}

	////配置AI-2+副AI手牌
	//int iAssistAIPos = -1;
	//int iDeputyAIPos = -1;
	//for (int i = 0; i < m_iMaxPlayerNum; i++)
	//{
	//	if (tableItem->pPlayers[i]->iPlayerType == ASSIST_AI_PLAYER)
	//	{
	//		iAssistAIPos = tableItem->pPlayers[i]->cTableNumExtra;
	//	}
	//	else if (tableItem->pPlayers[i]->iPlayerType == DEPUTY_AI_PLAYER)
	//	{
	//		iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
	//	}
	//}
	////主AI-2成牌中不能包含的花色,且主AI-2的缺不可以和主AI-1一致
	//int iAssistAIQue = -1;
	//for (int i = 0; i < 1; i++)
	//{
	//	iAssistAIQue = rand() % 3;
	//	if (iAssistAIQue == iMainAIQue)
	//	{
	//		i--;
	//	}
	//}
	//tableItem->pPlayers[iAssistAIPos]->iAIConfirmQueType = iAssistAIQue;
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 Que=[%d]", iAssistAIQue);
	////确定主AI-2二次配牌时的清一色花色为AI-1的定缺花色
	//int iQingYiSeType = tableItem->pPlayers[iMainAIPos]->iAIConfirmQueType;
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 QingYiSeType=[%d]", iQingYiSeType);
	//vector<char>vecAssistAITingCardsSection;
	//vector<char>vecAssistAIHuCard;
	////单独创建有关听牌区间的主牌 刻子加临张(4445,听346,注意此时的刻子应该是2-7的刻子 1/9刻子不能做到听三张)
	//for (int i = 0; i < 1; i++)
	//{
	//	int iType = iQingYiSeType;
	//	int iValue = rand() % 7 + 2;
	//	int iCard = (iType << 4) | iValue;
	//	//判断刻子余量
	//	if (cAllCard[iCard] >= 3)
	//	{
	//		int iNearByCards = 0;
	//		if (iValue == 2)
	//		{
	//			//此时的临张只能是3
	//			iNearByCards = (iType << 4) | (iValue + 1);
	//		}
	//		else if (iValue == 8)
	//		{
	//			//此时的临张只能是7
	//			iNearByCards = (iType << 4) | (iValue - 1);
	//		}
	//		else
	//		{
	//			//此时的领张可以为上下领张
	//			int iNearByCardsUp = iNearByCards = (iType << 4) | (iValue + 1);
	//			int iNearByCardsDown = iNearByCards = (iType << 4) | (iValue - 1);
	//			if (cAllCard[iNearByCardsUp] > 0 && cAllCard[iNearByCardsDown] > 0)
	//			{
	//				//此时上下领张都有余量
	//				int iRand = rand() % 2;
	//				if (iRand == 0)
	//				{
	//					iNearByCards = iNearByCardsUp;
	//				}
	//				else
	//				{
	//					iNearByCards = iNearByCardsDown;
	//				}
	//			}
	//			else
	//			{
	//				if (cAllCard[iNearByCardsUp] > 0)
	//				{
	//					iNearByCards = iNearByCardsUp;
	//				}
	//				else if (cAllCard[iNearByCardsDown] > 0)
	//				{
	//					iNearByCards = iNearByCardsDown;
	//				}
	//				else
	//				{
	//					//上下领张都没有余量
	//					iNearByCards = iNearByCardsDown;
	//				}
	//			}
	//		}
	//		//判断领张余量
	//		if (cAllCard[iNearByCards] > 0)
	//		{
	//			//听牌区间存入
	//			for (int j = 0; j < 3; j++)
	//			{
	//				vecAssistAITingCardsSection.push_back(iCard);
	//				cAllCard[iCard]--;
	//				_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 TingCardSection =[%d]", iCard);
	//			}
	//			vecAssistAITingCardsSection.push_back(iNearByCards);
	//			cAllCard[iNearByCards]--;
	//			_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 TingCardSection NearByCard=[%d]", iNearByCards);
	//			//胡牌存入
	//			if (iValue == 2)
	//			{
	//				vecAssistAIHuCard.push_back((iType << 4) | (1));
	//				vecAssistAIHuCard.push_back((iType << 4) | (3));
	//				vecAssistAIHuCard.push_back((iType << 4) | (4));
	//			}
	//			else if (iValue == 8)
	//			{
	//				vecAssistAIHuCard.push_back((iType << 4) | (6));
	//				vecAssistAIHuCard.push_back((iType << 4) | (7));
	//				vecAssistAIHuCard.push_back((iType << 4) | (9));
	//			}
	//			else
	//			{
	//				if (iNearByCards == iCard + 1)
	//				{
	//					vecAssistAIHuCard.push_back((iType << 4) | (iCard - 1));
	//					vecAssistAIHuCard.push_back((iType << 4) | (iCard + 1));
	//					vecAssistAIHuCard.push_back((iType << 4) | (iCard + 2));
	//				}
	//				else if (iNearByCards == iCard - 1)
	//				{
	//					vecAssistAIHuCard.push_back((iType << 4) | (iCard - 1));
	//					vecAssistAIHuCard.push_back((iType << 4) | (iCard + 1));
	//					vecAssistAIHuCard.push_back((iType << 4) | (iCard - 2));
	//				}
	//			}
	//		}
	//		else
	//		{
	//			i--;
	//		}
	//	}
	//	else
	//	{
	//		//当前刻子组合不可用
	//		i--;
	//	}
	//}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 ResetCard-HuCard NUM=[%d]", vecAssistAIHuCard.size());
	//for (int i = 0; i < vecAssistAIHuCard.size(); i++)
	//{
	//	//将胡牌部分移除一张
	//	cAllCard[vecAssistAIHuCard[i]]--;
	//	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 ResetCard-HuCard=[%d]", vecAssistAIHuCard[i]);
	//}
	////创建主AI-2的主牌部分
	//vector<char>vecAssistAIMainCard;
	//for (int i = 0; i < 3; i++)
	//{
	//	int iRandType = rand() % 2;
	//	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 CreateRandSetCard TYPE=[%d]", iRandType);
	//	CreateRandSetCard(iRandType, iQingYiSeType, cAllCard, vecAssistAIMainCard);
	//}
	////分割主牌，将一组主牌存放在副AI手中,作为副AI的主牌
	//int iDeputyAISectionNum = 0;
	//int iDeputyAISectionCount = 0;
	//for (int i = 0; i < 3; i++)
	//{
	//	tableItem->pPlayers[iDeputyAIPos]->cMainCards[iDeputyAISectionNum][iDeputyAISectionCount] = vecAssistAIMainCard[i];
	//	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 TO AI-3 MaincCards[%d]", vecAssistAIMainCard[i]);
	//	iDeputyAISectionCount++;
	//}
	////将剩余主牌（两组）存放在主AI-2的cMainCards中
	//int iAssistAISectionNum = 0;
	//int iAssistAISectionCount = 0;
	//for (int i = 3; i < vecAssistAIMainCard.size(); i++)
	//{
	//	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 Remain  MaincCards[%d]", vecAssistAIMainCard[i]);
	//	if (iAssistAISectionCount < 3)
	//	{
	//		tableItem->pPlayers[iAssistAIPos]->cMainCards[iAssistAISectionNum][iAssistAISectionCount] = vecAssistAIMainCard[i];
	//		iAssistAISectionCount++;
	//	}
	//	else
	//	{
	//		iAssistAISectionNum++;
	//		iAssistAISectionCount = 0;
	//		
	//		tableItem->pPlayers[iAssistAIPos]->cMainCards[iAssistAISectionNum][iAssistAISectionCount] = vecAssistAIMainCard[i];
	//		iAssistAISectionCount++;
	//	}
	//}
	////处理主AI-2的缺牌,如果保留在主AI-2主牌中的vecAssistAIMainCard是一副刻子，则拿出一张作为缺牌
	//int iCurNeedSection = 0;
	//vector<char>vecAssistAINeedCard;
	//for (int i = 0; i < 10; i++)
	//{
	//	if (tableItem->pPlayers[iAssistAIPos]->cMainCards[i][0] != -1)
	//	{
	//		//判断当前区间是否是刻子区间
	//		if (tableItem->pPlayers[iAssistAIPos]->cMainCards[i][0] == tableItem->pPlayers[iAssistAIPos]->cMainCards[i][1])
	//		{
	//			_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 NeedCards[%d]", tableItem->pPlayers[iAssistAIPos]->cMainCards[i][0]);
	//			char cCard = tableItem->pPlayers[iAssistAIPos]->cMainCards[i][0];
	//			tableItem->pPlayers[iAssistAIPos]->cMainCards[i][0] = 99;
	//			tableItem->pPlayers[iAssistAIPos]->cNeedCards[iCurNeedSection][0] = tableItem->pPlayers[iAssistAIPos]->cMainCards[i][1];
	//			tableItem->pPlayers[iAssistAIPos]->cNeedCardsBackUp[iCurNeedSection][0] = tableItem->pPlayers[iAssistAIPos]->cMainCards[i][1];
	//			vecAssistAINeedCard.push_back(cCard);
	//			iCurNeedSection++;
	//		}
	//	}
	//	else
	//	{
	//		break;
	//	}
	//}
	////听牌区间
	//iAssistAISectionCount = 0;
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 iAssistAISectionNum[%d]", iAssistAISectionNum);
	//for (int i = 0; i < vecAssistAITingCardsSection.size(); i++)
	//{
	//	tableItem->pPlayers[iAssistAIPos]->cMainCards[iAssistAISectionNum+1][iAssistAISectionCount] = vecAssistAITingCardsSection[i];
	//	iAssistAISectionCount++;
	//}
	////处理得到当前主AI-2的所有主牌
	//vecAssistAIMainCard.clear();
	//for (int i = 0; i < 10; i++)
	//{
	//	for (int j = 0; j < 10; j++)
	//	{
	//		if (tableItem->pPlayers[iAssistAIPos]->cMainCards[i][j] != -1 && tableItem->pPlayers[iAssistAIPos]->cMainCards[i][j] != 99)
	//		{
	//			_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 vecAssistAIMainCard[%d]", tableItem->pPlayers[iAssistAIPos]->cMainCards[i][j]);
	//			vecAssistAIMainCard.push_back(tableItem->pPlayers[iAssistAIPos]->cMainCards[i][j]);
	//		}
	//	}
	//}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard vecAssistAIMainCard.szie()=[%d]", vecAssistAIMainCard.size());
	////将主AI-2的胡牌保存一份到副AI手牌中,作为主牌
	//for (int i = 0; i < vecAssistAIHuCard.size(); i++)
	//{
	//	tableItem->pPlayers[iDeputyAIPos]->cMainCards[1][i] = vecAssistAIHuCard[i];
	//}
	//补齐AI-1玩家手牌 
	//补齐主AI-1手牌，补齐的这部分牌同样需要存入AI-1的多余牌数组中
	vector<char> vecTmpMainAIBuQi;//主AI-1的补齐牌吃
	AutoBuQiHandCard(tableItem, vecTmpMainAIMainCard.size(), cAllCard, vecTmpMainAIBuQi);
	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-1 vecTmpMainAIBuQi.szie()=[%d]", vecTmpMainAIBuQi.size());
	for (int i = 0; i < vecTmpMainAIBuQi.size(); i++)
	{
		tableItem->pPlayers[iMainAIPos]->vecRestCards.push_back(vecTmpMainAIBuQi[i]);
		_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-1 BUQICard=[%d]", vecTmpMainAIBuQi[i]);
	}
	vector<char>vecMainAIHandCard;
	for (int i = 0; i < vecTmpMainAIMainCard.size(); i++)
	{
		vecMainAIHandCard.push_back(vecTmpMainAIMainCard[i]);
	}
	for (int i = 0; i < vecTmpMainAIBuQi.size(); i++)
	{
		vecMainAIHandCard.push_back(vecTmpMainAIBuQi[i]);
	}

	////补齐AI-2玩家的手牌
	//vector<char>vecTmpAssistAIBuQI;
	//AutoBuQiHandCard(tableItem, vecAssistAIMainCard.size(), cAllCard, vecAssistAIHuCard, vecTmpAssistAIBuQI);
	//for (int i = 0; i < vecTmpAssistAIBuQI.size(); i++)
	//{
	//	tableItem->pPlayers[iAssistAIPos]->vecRestCards.push_back(vecTmpAssistAIBuQI[i]);
	//}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-2 vecTmpAssistAIBuQI.szie()=[%d]", vecTmpAssistAIBuQI.size());
	//vector<char>vecAssistAIHandCard;
	//for (int i = 0; i < vecAssistAIMainCard.size(); i++)
	//{
	//	vecAssistAIHandCard.push_back(vecAssistAIMainCard[i]);
	//}
	//for (int i = 0; i < vecTmpAssistAIBuQI.size(); i++)
	//{
	//	vecAssistAIHandCard.push_back(vecTmpAssistAIBuQI[i]);
	//}
	////补齐副AI玩家的手牌
	//vector<char>vecTmpDeputyMainCard;
	//for (int i = 0; i < 10; i++)
	//{
	//	for (int j = 0; j < 10; j++)
	//	{
	//		if (tableItem->pPlayers[iDeputyAIPos]->cMainCards[i][j] != -1)
	//		{
	//			vecTmpDeputyMainCard.push_back(tableItem->pPlayers[iDeputyAIPos]->cMainCards[i][j]);
	//		}
	//	}
	//}
	//vector<char>vecTmpDeputyAIBuQI;
	//AutoBuQiHandCard(tableItem, vecTmpDeputyMainCard.size(), cAllCard, vecAssistAIHuCard, vecTmpDeputyAIBuQI);
	//for (int i = 0; i < vecTmpDeputyAIBuQI.size(); i++)
	//{
	//	tableItem->pPlayers[iDeputyAIPos]->vecRestCards.push_back(vecTmpDeputyAIBuQI[i]);
	//}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AI-3 vecTmpDeputyAIBuQI.szie()=[%d]", vecTmpDeputyAIBuQI.size());
	//vector<char>vecDeputyAIHandCard;
	//for (int i = 0; i < vecTmpDeputyMainCard.size(); i++)
	//{
	//	vecDeputyAIHandCard.push_back(vecTmpDeputyMainCard[i]);
	//}
	//for (int i = 0; i < vecTmpDeputyAIBuQI.size(); i++)
	//{
	//	vecDeputyAIHandCard.push_back(vecTmpDeputyAIBuQI[i]);
	//}

	//随机生成主AI-1的上张次序
	int iGetCardCount = 0;
	GetValueInt(&iGetCardCount, "hzxlmj_AI_getcard_count ", "hzxl_room.conf", "GENERAL", "6");
	//安全保护,当设定上张张数过少时，重置为6张
	if (vecTmpMainAINeedCard.size() >= iGetCardCount)
	{
		iGetCardCount = 6;
	}
	//安全保护当张数过多时（>剩余手牌的1/4时），重置为6
	if (iGetCardCount > 14)
	{
		iGetCardCount = 6;
	}
	//生成顺序
	int iOrderGetCard[13];
	memset(iOrderGetCard, 0, sizeof(iOrderGetCard));
	for (int i = 0; i < vecTmpMainAINeedCard.size(); i++)
	{
		//保证摸牌顺序的最大值是不大于iGetCardCount
		int iRand = rand() % iGetCardCount;
		if (iOrderGetCard[iRand] == 1)
		{
			i--;
		}
		else
		{
			iOrderGetCard[iRand] = 1;
		}
	}
	//将生成顺序存入主AI节点
	for (int i = 0; i < iGetCardCount; i++)
	{
		tableItem->pPlayers[iMainAIPos]->vecAIGetCardOrder.push_back(iOrderGetCard[i]);
		_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard MainAI-GetCardOrder=[%d]", iOrderGetCard[i]);
	}
	//处理剩余所有牌
	//处理所有卡牌 得到的卡牌是所有牌-主AI-1手牌-主AI-1的可胡牌（各一张）-主AI-1缺牌
	vector<char> vecAllCardRest;
	for (int i = 0; i < 66; i++)
	{
		if (cAllCard[i] != 0)
		{
			for (int j = 0; j < cAllCard[i]; j++)
			{
				vecAllCardRest.push_back(i);
			}
		}
	}
	//洗牌
	random_shuffle(vecAllCardRest.begin(), vecAllCardRest.end());
	memset(tableItem->cAllCards, 0, sizeof(tableItem->cAllCards));
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard LeftCard Size=[%d]", vecAllCardRest.size());
	//主AI-1的手牌存入
	for (int i = 0; i < vecMainAIHandCard.size(); i++)
	{
		tableItem->cAllCards[iMainAIPos * 13 + i] = vecMainAIHandCard[i];
	}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard MainAI HandCard Size=[%d]", vecMainAIHandCard.size());
	////主AI-2的成组手牌存入
	//for (int i = 0; i < vecAssistAIHandCard.size(); i++)
	//{
	//	tableItem->cAllCards[iAssistAIPos * 13 + i] = vecAssistAIHandCard[i];
	//}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard AssistAI HandCard Size=[%d]", vecAssistAIHandCard.size());
	////副AI的手牌存入
	//for (int i = 0; i < vecDeputyAIHandCard.size(); i++)
	//{
	//	tableItem->cAllCards[iDeputyAIPos * 13 + i] = vecDeputyAIHandCard[i];
	//}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard DeputyAI HandCard Size=[%d]", vecDeputyAIHandCard.size());
	//普通玩家手牌存入
	for (int i = 0; i < 13; i++)
	{
		tableItem->cAllCards[iAssistAIPos * 13 + i] = vecAllCardRest[0];
		vecAllCardRest.erase(vecAllCardRest.begin());
	}
	for (int i = 0; i < 13; i++)
	{
		tableItem->cAllCards[iDeputyAIPos * 13 + i] = vecAllCardRest[0];
		vecAllCardRest.erase(vecAllCardRest.begin());
	}
	for (int i = 0; i < 13; i++)
	{
		for (int j = 0; j < vecAllCardRest.size(); j++)
		{
			if (vecAllCardRest[j] != 65)
			{
				tableItem->cAllCards[iNormalPlayerPos * 13 + i] = vecAllCardRest[j];
				vecAllCardRest.erase(vecAllCardRest.begin() + j);
				break;
			}
			else
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard NORMAL_PLAYER GET HZ");
			}
		}
	}

	//将主AI-1的可胡牌（各一张）；主AI-1+主AI-2的缺牌存入剩余牌尾部
	vector<char>vecWallCardsTail;
	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard MainAI HuCard Size=[%d]", vecMainAIHuCard.size());
	for (int i = 0; i < vecMainAIHuCard.size(); i++)
	{
		vecWallCardsTail.push_back(vecMainAIHuCard[i]);
		_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard MainAI HuCard=[%d]", vecMainAIHuCard[i]);
	}

	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard MainAI NeedCard Size=[%d]", vecTmpMainAINeedCard.size());
	for (int i = 0; i < vecTmpMainAINeedCard.size(); i++)
	{
		vecWallCardsTail.push_back(vecTmpMainAINeedCard[i]);
		_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard MainAI NeedCard=[%d]", vecTmpMainAINeedCard[i]);
	}
	random_shuffle(vecWallCardsTail.begin(), vecWallCardsTail.end());
	for (int i = 0; i < vecWallCardsTail.size(); i++)
	{
		vecAllCardRest.push_back(vecWallCardsTail[i]);
	}
	//_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard LeftCard Size=[%d]", vecAllCardRest.size());
	//剩余手牌存入
	for (int i = 0; i < vecAllCardRest.size(); i++)
	{
		tableItem->cAllCards[4 * 13 + i] = vecAllCardRest[i];
	}
	
	//普通玩家手牌红中数量校验,vecHZPos存放红中在普通玩家的手牌中的位置
	vector<int>vecHZPos;
	for (int i = 0; i < 13; i++)
	{
		if (tableItem->cAllCards[iNormalPlayerPos * 13 + i] == 65)
		{
			vecHZPos.push_back(iNormalPlayerPos * 13 + i);
		}
	}
	//当普通玩家手中红中数量>时，将从第二张红中开始的所有红中替换成非主AI-1的缺牌
	if (vecHZPos.size()!=0)
	{
		for (int i = 0; i < vecHZPos.size(); i++)
		{
			int iRandType = rand() % 3;
			int iRandValue = rand() % 9 + 1;
			int iRandCard = iRandValue | (iRandType << 4);
			if (std::find(vecTmpMainAINeedCard.begin(), vecTmpMainAINeedCard.end(), iRandCard) == vecTmpMainAINeedCard.end())
			{
				for (int j = 13 * 4; j < 112; j++)
				{
					if (tableItem->cAllCards[j] == iRandCard)
					{
						char cTmpCard = tableItem->cAllCards[vecHZPos[i]];
						tableItem->cAllCards[vecHZPos[i]] = tableItem->cAllCards[j];
						tableItem->cAllCards[j] = cTmpCard;
					}
				}
			}
			else
			{
				i--;
			}
		}

	}
	//校验是否所有牌存入
	int iCount = 0;
	for (int i = 0; i < 112; i++)
	{
		if (tableItem->cAllCards[i] != 0)
		{
			iCount++;
		}
	}
	if (iCount != 112)
	{
		_log(_ERROR, "HZXLMJ_AIControl", " CTLSetControlCard UserID =[%d],Final CardCount Error =[%d]", tableItem->pPlayers[0]->iUserID, iCount);
	}
	//校验每张牌是否都是4张
	char cCheckAllCard[5][10];
	memset(&cCheckAllCard, 0, sizeof(cCheckAllCard));
	_log(_DEBUG, "HZXLMJ_AIControl", " CTLSetControlCard iMaxMoCardNum =[%d]", 112);
	for (int i = 0; i < 112; i++)
	{
		_log(_DEBUG, "HZXLMJ_AIControl", " CTLSetControlCard Final tablecard =[%d]", tableItem->cAllCards[i]);
		if (tableItem->cAllCards[i] != 0)
		{
			int iCardType = tableItem->cAllCards[i] >> 4;
			int iCardValue = tableItem->cAllCards[i] & 0xf;

			cCheckAllCard[iCardType][0]++;
			cCheckAllCard[iCardType][iCardValue]++;
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (cCheckAllCard[i][0] != 36)
		{
			for (int j = 1; j < 10; j++)
			{
				if (cCheckAllCard[i][j] != 4)
				{
					char cCard = (i << 4) | j;
					_log(_ERROR, "AnalysisControlCard", "UserID =[%d],Card[%d] ErrorNum=[%d]", tableItem->pPlayers[0]->iUserID, cCard, cCheckAllCard[i][j]);
				}
			}
		}
	}
	//校验红中
	if (cCheckAllCard[4][0] != 4)
	{
		char cCard = (4 << 4) | 1;
		_log(_ERROR, "AnalysisControlCard", "UserID =[%d],Card[%d] ErrorNum=[%d]", tableItem->pPlayers[0]->iUserID, 65, cCheckAllCard[4][1]);
	}
	_log(_DEBUG, "HZXLMJ_AIControl", "CTLSetControlCard_Log() End");
}

void HZXLMJ_AIControl::CTLExchangeCardAI(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers, ChangeCardsReqDef &msgChangeCardReq)
{
	//AI的换牌,不换
}

void HZXLMJ_AIControl::CTLConfirmQue(MJGTableItemDef* tableItem, MJGPlayerNodeDef * nodePlayers, ConfirmQueReqDef & msgConfirmQueReq)
{
	//AI的定缺
	if (nodePlayers->iPlayerType == MIAN_AI_PLAYER )
	{
		//主AI-1和主AI-2的定缺，都是在配牌的时候配好的
		msgConfirmQueReq.iQueType = nodePlayers->iAIConfirmQueType;
		msgConfirmQueReq.cTableNumExtra = nodePlayers->cTableNumExtra;
	}
	else if (nodePlayers->iPlayerType == DEPUTY_AI_PLAYER || nodePlayers->iPlayerType == ASSIST_AI_PLAYER)
	{
		////副AI定缺和普通玩家定缺一致
		//int iNormalPlayerPos = -1;
		//for (int i = 0; i < m_iMaxPlayerNum; i++)
		//{
		//	if (tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
		//	{
		//		iNormalPlayerPos = i;
		//	}
		//}
		//if (tableItem->pPlayers[iNormalPlayerPos]->iQueType >= 0)
		//{
		//	msgConfirmQueReq.iQueType = tableItem->pPlayers[iNormalPlayerPos]->iQueType;
		//}
		int iTmpArr[3] = { 0 };
		int iMinHuaSe = 0;
		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
		{
			int iCardType = nodePlayers->cHandCards[i] >> 4;
			if (iCardType < 3)
			{
				iTmpArr[iCardType]++;
			}
		}
		//找到花色最少的牌
		for (int i = 0; i < 3; i++)
		{
			if (iTmpArr[i] <= iTmpArr[iMinHuaSe])
			{
				iMinHuaSe = i;
			}
		}
		msgConfirmQueReq.iQueType = iMinHuaSe;
	}

	_log(_DEBUG, "HZXLMJ_AIControl", " CreateAI ConfirmQueReq_Log(): AI[%d]ConfirmQue=[%d]", nodePlayers->iPlayerType, msgConfirmQueReq.iQueType);
	msgConfirmQueReq.cTableNumExtra = nodePlayers->cTableNumExtra;
}

void HZXLMJ_AIControl::CTLGetCardControl(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers)
{

	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CTLGetCardControlReq_Log(): nodePlayers is NULL ●●●●");
		return;
	}
	int iCurIndex = -1;

	//普通玩家的摸牌控制
	if (nodePlayers->iPlayerType == NORMAL_PLAYER)
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log(): ACCESS NORMAL_PLAYER GETCARD CONTROL");
		int iMainAIPos = -1;
		int iDeputyAIPos = -1;
		for (int i = 0; i < m_iMaxPlayerNum; i++)
		{
			if (tableItem->pPlayers[i]->iPlayerType == MIAN_AI_PLAYER)
			{
				iMainAIPos = tableItem->pPlayers[i]->cTableNumExtra;
			}
			else if (tableItem->pPlayers[i]->iPlayerType == DEPUTY_AI_PLAYER)
			{
				iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
			}
		}
		vector<char> vecMainAINeedCard;
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				if (tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j] > 0 )
				{
					vecMainAINeedCard.push_back(tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j]);
				}
				else
				{
					break;
				}
			}
		}
		//如果主AI-1没有听牌，即缺牌没有全部补入
		if (vecMainAINeedCard.size() != 0)
		{
			_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log(): NORMALPLAYER CTL ----MainAI Not TING");
			//判断当前普通玩家是否胡牌
			if (nodePlayers->bIsHu)
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log():NORMALPLAYER CTL ----NormalPalyer Already Hu");
				for (int i = tableItem->iCardIndex; i < 112; i++)
				{
					bool bIsHuCard = (find(nodePlayers->vcTingCard.begin(), nodePlayers->vcTingCard.end(), tableItem->cAllCards[i]) == nodePlayers->vcTingCard.end());
					bool bIsMianAIQue = (find(vecMainAINeedCard.begin(), vecMainAINeedCard.end(), tableItem->cAllCards[i]) == vecMainAINeedCard.end());

					if (bIsHuCard && bIsMianAIQue && tableItem->cAllCards[i] != 65)
					{
						//当前牌不是主AI-1缺牌，也不是普通玩家可胡牌
						iCurIndex = i;
						break;
					}
				}
			}
			else
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log(): NORMALPLAYER CTL ----NormalPalyer Not Hu");
				//只控制不拿到红中+主AI-1的缺牌
				for (int i = tableItem->iCardIndex; i < 112; i++)
				{
					bool bIsMianAIQue = (find(vecMainAINeedCard.begin(), vecMainAINeedCard.end(), tableItem->cAllCards[i]) == vecMainAINeedCard.end());
					if (bIsMianAIQue)
					{
						//控制玩家拿不到红中的数量
						if (tableItem->cAllCards[i] != 65)
						{
							iCurIndex = i;
							break;
						}
					}
				}
			}
		}
		else
		{
			_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log(): NORMALPLAYER CTL---MainAI  AlreadyTING");
			//当前主AI-1已经听牌，不做控制，控制不摸到红中即可
			if (nodePlayers->bIsHu)
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log(): NORMALPLAYER CTL---Already HU");
				if (tableItem->cAllCards[tableItem->iCardIndex] == 65)
				{
					_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log(): NORMALPLAYER CTL---HzCount CTL");
					for (int i = tableItem->iCardIndex; i < 112; i++)
					{
						if (tableItem->cAllCards[i] != 65)
						{
							iCurIndex = i;
							break;
						}
					}
				}
			}
			else
			{
				//当前普通玩家未胡牌，仅控制红中数量
				_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log(): NORMALPLAYER CTL---Not HU");
				if (tableItem->cAllCards[tableItem->iCardIndex] == 65)
				{
					_log(_DEBUG, "HZXLMJ_AIControl", "CTLGetCardControlReq_Log(): NORMALPLAYER CTL---HzCount CTL");
					for (int i = tableItem->iCardIndex; i < 112; i++)
					{
						if (tableItem->cAllCards[i] != 65)
						{
							iCurIndex = i;
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "ACCESS AI_PLAYER GETCARD CONTROL");
		//AI的摸牌逻辑
		int iNormalPlayerPos = -1;
		for (int i = 0; i < m_iMaxPlayerNum; i++)
		{
			if (tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
			{
				iNormalPlayerPos = tableItem->pPlayers[i]->cTableNumExtra;
			}
		}
		if (nodePlayers->iPlayerType == MIAN_AI_PLAYER)
		{
			_log(_DEBUG, "HZXLMJ_AIControl", "ACCESS MIAN_AI_PLAYER GETCARD CONTROL");
			//获取当前主AI-1的缺牌
			vector<char>vecMainAINeedCard;
			for (int i = 0; i < 10; i++)
			{
				for (int j = 0; j < 10; j++)
				{
					if (nodePlayers->cNeedCards[i][j] != -1)
					{
						_log(_DEBUG, "HZXLMJ_AIControl", "MIAN_AI_PLAYER Cur NeedCards[%d]", nodePlayers->cNeedCards[i][j]);
						vecMainAINeedCard.push_back(nodePlayers->cNeedCards[i][j]);
					}
				}
			}
			//获取当前剩余次序
			//_log(_DEBUG, "HZXLMJ_AIControl", "MIAN_AI_PLAYER Cur vecAIGetCardOrder.Size[%d]", nodePlayers->vecAIGetCardOrder.size());
			for (int i = 0; i < nodePlayers->vecAIGetCardOrder.size(); i++)
			{
				//_log(_DEBUG, "HZXLMJ_AIControl", "MIAN_AI_PLAYER Cur vecAIGetCardOrder[%d][%d]",i, nodePlayers->vecAIGetCardOrder[i]);
			}
			char cCard;
			if (vecMainAINeedCard.size() != 0)
			{
				//_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL---Not TING vecMainAINeedCard.size=[%d]   vecAIGetCardOrder.size()=[%d]  vecAIGetCardOrder[%d]", vecMainAINeedCard.size(),  nodePlayers->vecAIGetCardOrder.size(),nodePlayers->vecAIGetCardOrder[0]);
				//当前缺牌还没有摸完，根据上张顺序摸牌,如果补入缺牌，则需要移除对应缺牌
				if (nodePlayers->vecAIGetCardOrder[0] == 1)
				{
					_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL---Get NeedCard");
					bool bIfFind = false;
					for (int i = 0; i < 10; i++)
					{
						for (int j = 0; j < 10; j++)
						{
							if (nodePlayers->cNeedCards[i][j] != -1)
							{
								_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL---Get NeedCard[%d]", nodePlayers->cNeedCards[i][j]);
								cCard = nodePlayers->cNeedCards[i][j];
								nodePlayers->cNeedCards[i][j] = -1;
								bIfFind = true;
								break;
							}
						}
						if (bIfFind)
						{
							break;
						}
					}
					//在剩余牌墙中找到对应牌
					for (int i = tableItem->iCardIndex; i < 112; i++)
					{
						if (tableItem->cAllCards[i] == cCard)
						{
							iCurIndex = i;
							nodePlayers->vecAIGetCardOrder.erase(nodePlayers->vecAIGetCardOrder.begin());
							break;
						}
					}
				}
				else
				{
					_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL---Not Get NeedCard");
					//当前主AI-1不需要上张
					nodePlayers->vecAIGetCardOrder.erase(nodePlayers->vecAIGetCardOrder.begin());
					//判断当前摸牌是否是缺牌
					bool bIsNeedCard = JudgeIsMainAINeedCard(tableItem, tableItem->cAllCards[tableItem->iCardIndex]);
					if (bIsNeedCard)
					{
						_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- Get NeedCard Naturally ");
						//当前摸牌是缺牌，清除该缺牌
						for (int j = 0; j < 10; j++)
						{
							bool bIfFind = false;
							for (int k = 0; k < 10; k++)
							{
								if (nodePlayers->cNeedCards[j][k] == tableItem->cAllCards[tableItem->iCardIndex])
								{
									nodePlayers->cNeedCards[j][k] = -1;
									bIfFind = true;
									break;
								}
							}
							if (bIfFind)
							{
								break;
							}
						}
					}
					else
					{
						_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- CurCard Not Need Card ");
						//判断当前摸牌是不是主AI的可胡牌
						bool bIsHuCard = (find(nodePlayers->vecAIHuCard.begin(), nodePlayers->vecAIHuCard.end(), tableItem->cAllCards[tableItem->iCardIndex]) == nodePlayers->vecAIHuCard.end());
						if (!bIsHuCard)
						{
							_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- CurCard is Hu Card ");
							//若当前摸到的是胡牌，替换
							for (int i = tableItem->iCardIndex; i < 112; i++)
							{
								bool bIsHuCardAgagin = (find(nodePlayers->vecAIHuCard.begin(), nodePlayers->vecAIHuCard.end(), tableItem->cAllCards[i]) != nodePlayers->vecAIHuCard.end());
								bool bIsNeedCardAgain = JudgeIsMainAINeedCard(tableItem, tableItem->cAllCards[i]);
								if (!bIsHuCardAgagin && !bIsNeedCardAgain)
								{
									//将牌存入多余牌组
									_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- 存入多余牌 pos[%d]Card[%d]",i, tableItem->cAllCards[i]);
									nodePlayers->vecRestCards.push_back(tableItem->cAllCards[i]);
									iCurIndex = i;
									break;
								}

							}
						}
						else
						{
							nodePlayers->vecRestCards.push_back(tableItem->cAllCards[tableItem->iCardIndex]);
						}
					}
				}
			}
			else
			{
				_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- Already TING");
				//打印所有可胡牌
				for (int i = 0; i < nodePlayers->vecAIHuCard.size(); i++)
				{
					//_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI TING CARD[%d]", nodePlayers->vecAIHuCard[i]);
				}
				//当前主AI-1的手牌已经进入听牌状态，判断普通玩家当前输赢番
				//_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- Normal Player iCurWinMoney=[%d]", tableItem->pPlayers[iNormalPlayerPos]->iCurWinMoney);
				if (tableItem->pPlayers[iNormalPlayerPos]->iCurWinMoney > 0)
				{
					//_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- Need Hu");
					//当前普通玩家总赢分为正，控制胡牌
					bool bIfExistInWall = false;
					if (nodePlayers->vecAIHuCard.size() != 0)
					{
						_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- Find In Wall ");
						//在牌墙中找主AI-1的可胡牌
						for (int i = 0; i < nodePlayers->vecAIHuCard.size(); i++)
						{
							for (int j = tableItem->iCardIndex; j < 112; j++)
							{
								if (tableItem->cAllCards[j] == nodePlayers->vecAIHuCard[i])
								{
									iCurIndex = j;
									bIfExistInWall = true;
									break;
								}
							}
							if (bIfExistInWall)
							{
								break;
							}
						}
						//在主AI-2的非主牌和副AI手牌中找
						if (!bIfExistInWall)
						{
							_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- Find In Rest AI ");
							bool bIfExistInAIHand;
							for (int i = 0; i < nodePlayers->vecAIHuCard.size(); i++)
							{
								bIfExistInAIHand = CheckRestAIHandCard(tableItem, nodePlayers->vecAIHuCard[i], iCurIndex);
								if (bIfExistInAIHand)
								{
									break;
								}
							}
							if (!bIfExistInAIHand)
							{
								_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- Main AI CanNot Hu ");
								//此时主AI-1已经不可胡牌，需要主AI-2和副AI胡牌
								nodePlayers->vecAIHuCard.clear();
							}
						}
					}
					else
					{
						_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- Main AI CanNot Hu ");
						//当前主AI-1无牌可胡，将当前摸牌存放多余牌中（胡牌后的出牌不收控制）
						//nodePlayers->vecRestCards.push_back(tableItem->cAllCards[tableItem->iCardIndex]);
					}
				}
				else if (tableItem->pPlayers[iNormalPlayerPos]->iCurWinMoney <= 0)
				{
					_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL--- NormalPlayer iCurWinMoney<=0 [%d]", tableItem->pPlayers[iNormalPlayerPos]->iWinMoney);
					//正常摸牌，判断当前摸牌是否是可胡牌
					_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL 当前摸牌[%d]", tableItem->cAllCards[tableItem->iCardIndex]);
					bool bIsHuCard = (find(nodePlayers->vecAIHuCard.begin(), nodePlayers->vecAIHuCard.end(), tableItem->cAllCards[tableItem->iCardIndex]) != nodePlayers->vecAIHuCard.end());
					if (bIsHuCard)
					{
						//是可胡牌，不做控制，可以胡牌
						_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL GET HU CARD");
					}
					else
					{
						_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL GET NOT HU CARD");
						//_log(_DEBUG, "HZXLMJ_AIControl", " MIAN-AI GETCARD CTL 当前摸牌[%d]存入多余牌组");
						nodePlayers->vecRestCards.push_back(tableItem->cAllCards[tableItem->iCardIndex]);
					}
				}
			}
		}
		else if (nodePlayers->iPlayerType == ASSIST_AI_PLAYER || nodePlayers->iPlayerType == DEPUTY_AI_PLAYER)
		{
			_log(_DEBUG, "HZXLMJ_AIControl", "ACCESS ASSIST_AI_PLAYER GETCARD CONTROL");
			int iMainAIPos = -1;
			int iDeputyAIPos = -1;
			for (int i = 0; i < m_iMaxPlayerNum; i++)
			{
				if (tableItem->pPlayers[i]->iPlayerType == MIAN_AI_PLAYER)
				{
					iMainAIPos = tableItem->pPlayers[i]->cTableNumExtra;
				}
			}
			vector<char>vecMainAINeedCard;
			for (int i = 0; i < 10; i++)
			{
				for (int j = 0; j < 10; j++)
				{
					if (tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j] == -1)
					{
						break;
					}
					else
					{
						vecMainAINeedCard.push_back(tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j]);
					}
				}
			}
			//主AI-2的摸牌控制
			//当前主AI-1没有听牌
			if (vecMainAINeedCard.size() != 0)
			{
				_log(_DEBUG, "HZXLMJ_AIControl", " ASSIST_AI GETCARD CTL---Main AI Not TING");
				for (int i = tableItem->iCardIndex; i < 112; i++)
				{
					bool bIsMianAIQue = (find(vecMainAINeedCard.begin(), vecMainAINeedCard.end(), tableItem->cAllCards[i]) == vecMainAINeedCard.end());
					if (bIsMianAIQue)
					{
						iCurIndex = i;
						break;
					}
				}
			}
			else
			{
				_log(_DEBUG, "HZXLMJ_AIControl", " ASSIST_AI GETCARD CTL---Main AI Already TING");
				//此时主AI-1已经完成听牌,如果普通玩家还在赢分，控制AI-2不摸到AI-1的胡牌
				if (tableItem->pPlayers[iNormalPlayerPos]->iCurWinMoney > 0)
				{
					for (int i = tableItem->iCardIndex; i < 112; i++)
					{
						bool bIsMianAIHu = (find(tableItem->pPlayers[iMainAIPos]->vecAIHuCard.begin(), tableItem->pPlayers[iMainAIPos]->vecAIHuCard.end(), tableItem->cAllCards[i]) == tableItem->pPlayers[iMainAIPos]->vecAIHuCard.end());
						if (bIsMianAIHu)
						{
							iCurIndex = i;
							break;
						}
					}
				}
				//if (tableItem->pPlayers[iNormalPlayerPos]->iCurWinMoney > 0 && tableItem->pPlayers[iMainAIPos]->vecAIHuCard.size() != 0)
				//{
				//	_log(_DEBUG, "HZXLMJ_AIControl", " ASSIST_AI GETCARD CTL---Main AI Already TING");
				//	for (int i = tableItem->iCardIndex; i < 112; i++)
				//	{
				//		bool bIsMianAIHu = (find(tableItem->pPlayers[iMainAIPos]->vecAIHuCard.begin(), tableItem->pPlayers[iMainAIPos]->vecAIHuCard.end(), tableItem->cAllCards[i]) == vecMainAINeedCard.end());
				//		if (bIsMianAIHu)
				//		{
				//			iCurIndex = i;
				//			//判断当前牌是否是主AI-2的缺牌,如果摸到替換
				//			for (int i = 0; i < 10; i++)
				//			{
				//				bool bIfExist = false;
				//				for (int j = 0; j < 10; j++)
				//				{
				//					if (nodePlayers->cNeedCards[i][j] == tableItem->cAllCards[i])
				//					{
				//						bIfExist = true;
				//						nodePlayers->cNeedCards[i][j] = -1;
				//					}
				//				}
				//				if (bIfExist)
				//				{
				//					if (nodePlayers->cMainCards[i][1] == tableItem->cAllCards[i])
				//					{
				//						nodePlayers->cMainCards[i][0] = tableItem->cAllCards[i];
				//					}
				//				}
				//			}
				//			break;
				//		}
				//	}
				//}
				//if (tableItem->pPlayers[iNormalPlayerPos]->iCurWinMoney > 0 && nodePlayers->iIfResetHandCard == 1)
				//{
				//	_log(_DEBUG, "HZXLMJ_AIControl", " ASSIST_AI GETCARD CTL---Main AI CanNot CTL");
				//	//当前主AI-1已经无法完全控制普通玩家，需要控制其摸到自摸牌
				//	//先在牌墙中找
				//	_log(_DEBUG, "HZXLMJ_AIControl", " ASSIST_AI GETCARD CTL---Find Hu Card In Wall");
				//	for (int i = tableItem->iCardIndex; i < 112; i++)
				//	{
				//		if (find(nodePlayers->vecResetHuCard.begin(), nodePlayers->vecResetHuCard.end(), tableItem->cAllCards[i]) != nodePlayers->vecResetHuCard.end())
				//		{
				//			iCurIndex = i;
				//			break;
				//		}
				//	}
				//	//在副AI手牌中找
				//	if (iCurIndex == -1)
				//	{
				//		bool bIfExist = false;
				//		_log(_DEBUG, "HZXLMJ_AIControl", " ASSIST_AI GETCARD CTL---Find Hu Card In Deputy AI");
				//		for (int i = 0; i < tableItem->pPlayers[iDeputyAIPos]->iHandCardsNum; i++)
				//		{
				//			if (find(nodePlayers->vecResetHuCard.begin(), nodePlayers->vecResetHuCard.end(), tableItem->pPlayers[iDeputyAIPos]->cHandCards[i]) != nodePlayers->vecResetHuCard.end())
				//			{
				//				bIfExist = true;
				//				char cTmpCard = tableItem->cAllCards[tableItem->iCardIndex];
				//				tableItem->cAllCards[tableItem->iCardIndex] = tableItem->pPlayers[iDeputyAIPos]->cHandCards[i];
				//				tableItem->pPlayers[iDeputyAIPos]->cHandCards[i] = cTmpCard;
				//				break;
				//			}
				//		}
				//		if (!bIfExist)
				//		{
				//			_log(_DEBUG, "HZXLMJ_AIControl", " ASSIST_AI GETCARD CTL---Find Hu Card In Deputy AI fail");
				//		}
				//	}
				//}
				int iCTLLeftCardsNum = 92;	//当剩余张数小于20张时，要将多余的红中放到AI手中
				
				if (nodePlayers->bIfCTLGetHZ == false && tableItem->iCardIndex >= iCTLLeftCardsNum)
				{
					_log(_DEBUG, "HZXLMJ_AIControl", " AI-Player[%d]Access HZ CTL ",nodePlayers->iPlayerType);
					for (int i = tableItem->iCardIndex; i < 112; i++)
					{
						if (tableItem->cAllCards[i] == 65)
						{
							_log(_DEBUG, "HZXLMJ_AIControl", " AI-Player[%d] Find HZ", nodePlayers->iPlayerType);
							iCurIndex = i;
							nodePlayers->bIfCTLGetHZ = true;
							break;
						}
					}
				}
				else if (nodePlayers->bIfCTLGetHZ == true && tableItem->iCardIndex >= iCTLLeftCardsNum )
				{
					//判断剩余AI是否已经控制摸到红中
					int iRestAI = 0;
					for (int i = 0; i < 4; i++)
					{
						if (tableItem->pPlayers[i]->iPlayerType >= ASSIST_AI_PLAYER && tableItem->pPlayers[i]->iPlayerType != nodePlayers->iPlayerType)
						{
							iRestAI = tableItem->pPlayers[i]->cTableNumExtra;
						}
					}
					if (tableItem->pPlayers[iRestAI]->bIfCTLGetHZ)
					{
						_log(_DEBUG, "HZXLMJ_AIControl", "TWO AI ALready Get HZ", nodePlayers->iPlayerType);
						//此时两个AI都摸到过红中了，如果牌墙中仍有红中，控制AI摸到
						for (int i = tableItem->iCardIndex; i < 112; i++)
						{
							if (tableItem->cAllCards[i] == 65)
							{
								_log(_DEBUG, "HZXLMJ_AIControl", "Get HZ", nodePlayers->iPlayerType);
								iCurIndex = i;
								nodePlayers->bIfCTLGetHZ = true;
								break;
							}
						}
					}
				}
			}
		}
	}
	if (iCurIndex != -1)
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "Change Card Successful: Original[%d] TO Current[%d]", tableItem->cAllCards[tableItem->iCardIndex], tableItem->cAllCards[iCurIndex]);
		//交换手牌 
		char cTmpCard = tableItem->cAllCards[tableItem->iCardIndex];
		tableItem->cAllCards[tableItem->iCardIndex] = tableItem->cAllCards[iCurIndex];
		tableItem->cAllCards[iCurIndex] = cTmpCard;
	}
}

char HZXLMJ_AIControl::CTLFindOptimalSendCard(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers, char & cIfTing)
{
	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CTLFindOptimalSendCard_Log(): nodePlayers is NULL ●●●●");
		return 0;
	}
	if (nodePlayers->iPlayerType <= 0)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CTLFindOptimalSendCard_Log(): nodePlayers Is Not AI  ●●●●");
		return 0;
	}
	_log(_ERROR, "HZXLMJ_AIControl", "CTLFindOptimalSendCard_Log(): AI[%d] FindCard ", nodePlayers->iPlayerType);
	char cCard = 0;
	//主AI-1的出牌控制
	if (nodePlayers->iPlayerType == MIAN_AI_PLAYER)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "CTLFindOptimalSendCard_Log(): Main AI vecRestCards.size[%d] ",nodePlayers->vecRestCards.size());
		//只出多余牌，多余牌中必须先出定缺花色牌
		if (nodePlayers->vecRestCards.size() != 0)
		{
			//_log(_ERROR, "HZXLMJ_AIControl", "CTLFindOptimalSendCard_Log(): Main AI In vecRestCards");
			bool bIfExistQue = false;
			for (int i = 0; i < nodePlayers->vecRestCards.size(); i++)
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "CTLFindOptimalSendCard_Log(): Main AI vecRestCard[%d] ", nodePlayers->vecRestCards[i]);
				int iCardType = nodePlayers->vecRestCards[i] >> 4;
				if (iCardType == nodePlayers->iQueType)
				{
					_log(_DEBUG, "HZXLMJ_AIControl", "CTLFindOptimalSendCard_Log(): Main AI vecRestCard[%d] is QueType ", nodePlayers->vecRestCards[i]);
					bIfExistQue = true;
					cCard = nodePlayers->vecRestCards[i];
					nodePlayers->vecRestCards.erase(nodePlayers->vecRestCards.begin() + i);
					break;
				}
			}
			if (!bIfExistQue)
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "CTLFindOptimalSendCard_Log(): Main AI 出第一张缺牌[%d] ", nodePlayers->vecRestCards[0]);
				cCard = nodePlayers->vecRestCards[0];
				nodePlayers->vecRestCards.erase(nodePlayers->vecRestCards.begin());
			}
		}
	}
	//主AI-2、副AI的出牌控制
	else if (nodePlayers->iPlayerType == ASSIST_AI_PLAYER || nodePlayers->iPlayerType == DEPUTY_AI_PLAYER)
	{
		//这里需要加入判断AI是否已经胡过牌了
		if (nodePlayers->bIsHu)
		{
			//AI 已经胡牌了，此时出牌为摸到的牌
			cCard = tableItem->cCurMoCard;
		}
		else
		{
			cCard = FindOptimalSingleCard(nodePlayers);
		}
	}

	// 做个容错处理，如果上面没找到要出的牌，再随机找
	if (cCard <= 0)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "CTLFindOptimalSendCard_Log():  SendCards UID[%d], cCard[%d]", nodePlayers->iUserID, cCard);
		cCard = FindOptimalSingleCard(nodePlayers);
	}

	_log(_DEBUG, "HZXLMJ_AIControl", "CTLFindOptimalSendCard_Log():  SendCards [%d]", cCard);
	return cCard;
}
/***
控制流程下，AI玩家的碰杠胡操作选择
***/
void HZXLMJ_AIControl::CTLJudgeHandleSpecialCards(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers, int & iSpecialFlag, char & cFirstCard, char cCards[])
{
	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CTLJudgeHandleSpecialCards_Log(): nodePlayers is NULL ●●●●");
		return;
	}
	if (nodePlayers->iPlayerType <= 0)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CTLJudgeHandleSpecialCards_Log(): nodePlayers Is Not AI  ●●●●");
		return;
	}
	_log(_ERROR, "HZXLMJ_AIControl", "CTLJudgeHandleSpecialCards_Log():当前判断AI[%d]-flag",nodePlayers->iPlayerType, nodePlayers->iSpecialFlag);
	//处理特殊操作优先级 自摸>放炮胡>碰、杠
	int iFlag = -1;
	//自摸胡
	iFlag = nodePlayers->iSpecialFlag >> 6;
	if (iFlag > 0)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "CTLJudgeHandleSpecialCards_Log() 自摸 iFlag[%d]", iFlag);
		//当前AI可以胡牌，判断是否胡牌
		bool bIsHu = CTLJudgeIfAIPlayerSuitHu(tableItem, nodePlayers);
		if (bIsHu)
		{
			//当前AI可以胡牌，判断是自摸胡牌还是放炮胡
			if (nodePlayers->cTableNumExtra == tableItem->iCurMoPlayer)
			{
				//当前胡牌玩家是摸排玩家则是自摸胡牌
				iSpecialFlag = 1 << 6;
			}
		}
		return;
	}
	//放炮胡
	iFlag = nodePlayers->iSpecialFlag >> 5;
	if (iFlag > 0)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "CTLJudgeHandleSpecialCards_Log()  放炮 iFlag[%d]", iFlag);
		bool bIsHu = CTLJudgeIfAIPlayerSuitHu(tableItem, nodePlayers);
		if (bIsHu)
		{
			//当前AI可以胡牌
			iSpecialFlag = 1 << 5;
		}
		return;
	}
	iFlag = nodePlayers->iSpecialFlag >> 2;
	if (iFlag > 0)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "CTLJudgeHandleSpecialCards_Log() 杠 iFlag[%d]", iFlag);
		//处理玩家的杠
		bool bIfGang = CTLJudgeIfAIPlayerSuitGang(tableItem, nodePlayers , cFirstCard);
		if (bIfGang)
		{
			iSpecialFlag = iFlag << 2;
			
			for (int i = 0; i < 4; i++)
			{
				cCards[i] = cFirstCard;
			}
			_log(_ERROR, "HZXLMJ_AIControl", "CTLJudgeHandleSpecialCards_Log() 杠type[%d] cFirstCard[%d] cCards[%d]", iFlag, cFirstCard, cCards[0]);
		}
		return;
	}
	iFlag = nodePlayers->iSpecialFlag >> 1;
	if (iFlag > 0)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "CTLJudgeHandleSpecialCards_Log()  碰 iFlag[%d]", iFlag);
		//处理玩家的碰
		bool bIfPEng = CTLJudgeIfAIPlayerSuitPeng(tableItem, nodePlayers);
		if (bIfPEng)
		{
			iSpecialFlag = 1 << 1;
			cFirstCard = tableItem->cTableCard;
			for (int i = 0; i < 3; i++)
			{
				cCards[i] = cFirstCard;
			}
			_log(_ERROR, "HZXLMJ_AIControl", "CTLJudgeHandleSpecialCards_Log() 碰 cFirstCard[%d] cCards[%d]", cFirstCard, cCards[0]);
		}
		return;
	}
}

/***
char型 从1开始 int型从0开始
***/
char HZXLMJ_AIControl::MakeCardNumToChar(int iCardNum)
{
	//当前传入的牌值是万条饼
	//_log(_DEBUG, "HZXLMJ_AIControl", "MakeCardNumToChar(): cCardNum [%d]", iCardNum);
	if ((iCardNum / 9) < 3)
	{
		int iCardType = iCardNum / 9;
		int iCardValue = iCardNum % 9 + 1;
		char cCard = iCardValue | (iCardType << 4);
		//_log(_DEBUG, "HZXLMJ_AIControl", "MakeCardNumToChar(): iCardType[%d],iCardValue[%d],cCard [%d]", iCardType, iCardValue, cCard);
		return cCard;
	}
	else if (iCardNum == 31)
	{
		char cCard = 1 | (4 << 4);
		//_log(_DEBUG, "HZXLMJ_AIControl", "MakeCardNumToChar(): cCard [%d]", cCard);
		return cCard;
	}
	else if (iCardNum == 99)
	{
		char cCard = 99;
		return cCard;
	}
}
/***
为主AI-1和主AI-2的手牌补齐：
补齐原则：不补入主AI-1的缺牌、胡牌，主AI-2的缺牌、胡牌
***/
void HZXLMJ_AIControl::AutoBuQiHandCard(MJGTableItemDef *tableItem, int iCurHandCardsNum, char cAllCard[], vector<char> &vecBuQiCards)
{
	_log(_DEBUG, "HZXLMJ_AIControl", "AutoBuQiHandCard iCurHandCardsNum=[%d]", iCurHandCardsNum);
	//处理得到主AI-1、主AI-2的缺牌以及胡牌
	int iMainAIPos = -1;
	int iAssistAIPos = -1;
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (tableItem->pPlayers[i]->iPlayerType == MIAN_AI_PLAYER)
		{
			iMainAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		else if (tableItem->pPlayers[i]->iPlayerType == ASSIST_AI_PLAYER)
		{
			iAssistAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
	}
	vector<char>vecMainAINeedCards;
	vector<char>vecMainAIHuCards;
	for (int i = 0; i < tableItem->pPlayers[iMainAIPos]->vecAIHuCard.size(); i++)
	{
		vecMainAIHuCards.push_back(tableItem->pPlayers[iMainAIPos]->vecAIHuCard[i]);
	}
	//生成补齐手牌
	for (int i = 0; i < 13 - iCurHandCardsNum; i++)
	{
		int iRandType = rand() % 3;
		int iRandValue = rand() % 9 + 1;
		char cRandCard = iRandValue | (iRandType << 4);
		//校验是否重复
		bool bIsMainAICards;
		if (find(vecMainAINeedCards.begin(), vecMainAINeedCards.end(), cRandCard) == vecMainAINeedCards.end() && find(vecMainAIHuCards.begin(), vecMainAIHuCards.end(), cRandCard) == vecMainAIHuCards.end())
		{
			bIsMainAICards = false;
		}
		else
		{
			bIsMainAICards = true;
		}
		if (!bIsMainAICards)
		{
			if (cAllCard[cRandCard] > 0)
			{
				cAllCard[cRandCard]--;
				vecBuQiCards.push_back(cRandCard);
				_log(_DEBUG, "HZXLMJ_AIControl", "AutoBuQiHandCard BuQICard=[%d]", cRandCard);
			}
			else
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "AutoBuQiHandCard BuQICard 重复", cRandCard);
				i--;
			}
		}
		else
		{
			i--;
		}
	}
}
/***
为主AI-2补齐手牌,配置原则：
不再配置已有的成牌、缺牌花色不宜过多(配置三张缺牌花色)
***/
void HZXLMJ_AIControl::AutoBuQiAssistAICard(int iAssistAIQue, char cAllCard[], vector<char> vecTmpAssistAIHandCards, vector<char>& vecBuQICard)
{
	//随机生成三张定缺牌型的牌
	for (int i = 0; i < 3; i++)
	{
		int iCardType = iAssistAIQue;
		int iRandValue = rand() % 9 + 1;
		int iCard = (iCardType << 4) | iRandValue;
		if (cAllCard[iCard] > 0)
		{
			cAllCard[iCard]--;
			vecBuQICard.push_back(iCard);
		}
		else
		{
			i--;
		}
	}
	//补齐剩余牌
	for (int i = 0; i < 13 - vecTmpAssistAIHandCards.size() - 3; i++)
	{
		int iRandType = rand() % 3;
		int iRandValue = rand() % 9 + 1;
		int iCard = (iRandType << 4) | iRandValue;
		bool bIfAvailable = false;
		if (iRandType != iAssistAIQue || cAllCard[iCard] > 0)
		{
			//不和已有的成组手牌重复
			if (std::find(vecTmpAssistAIHandCards.begin(), vecTmpAssistAIHandCards.end(), iCard) == vecTmpAssistAIHandCards.end())
			{
				bIfAvailable = true;
			}
		}
		if (bIfAvailable)
		{
			cAllCard[iCard]--;
			vecBuQICard.push_back(iCard);
			_log(_DEBUG, "HZXLMJ_AIControl", "AutoBuQiAssistAICard BuQICard=[%d]", iCard);
		}
		else
		{
			i--;
		}
	}
}
/***
为主AI-2创建成组的手牌(刻子/顺子):
iSetType:成组手牌类型，0=刻子，1=顺子
iQingYiSeType:主AI-2的清一色花色
cAllCard:当前剩余所有手牌的数量
vecTmpCard:将生成的手牌存入该vector数组
***/
void HZXLMJ_AIControl::CreateRandSetCard(int iSetType, int iQingYiSeType, char cAllCard[], vector<char> &vecTmpCard)
{
	if (iSetType == 0)
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 生成刻");
		//生产一组可用刻子
		for (int i = 0; i < 1; i++)
		{
			_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 开始生成刻子");
			int iType = iQingYiSeType;
			int iValue = rand() % 9 + 1;
			char cCard = iValue | (iType << 4);
			_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 随机张=[%d]",cCard);
			if (cAllCard[cCard] < 3)
			{
				//_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 随机张余量=[%d]", cAllCard[cCard]);
				i = -1;
				//_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 生成刻子失败 重来 当前 i值[%d]",i);
			}
			else
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 生成刻子成功");
				for (int j = 0; j < 3; j++)
				{
					//对应数组做移除和添加
					cAllCard[cCard]--;
					vecTmpCard.push_back(cCard);
					//_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard AssistAIMainCard=[%d]", cCard);
				}
				break;
			}
		}
	}
	else
	{
		_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 生成顺");
		//生成可用顺子
		vector<char> vecTmpShun;
		for (int i = 0; i < 1; i++)
		{
			_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 开始生成顺子");
			int iType = iQingYiSeType;
			int iValue = rand() % 9 + 1;
			if (iValue == 1)
			{
				vecTmpShun.push_back(iValue | (iType << 4));
				vecTmpShun.push_back(iValue + 1 | (iType << 4));
				vecTmpShun.push_back(iValue + 2 | (iType << 4));
			}
			else if (iValue == 9)
			{
				vecTmpShun.push_back(iValue | (iType << 4));
				vecTmpShun.push_back(iValue - 1 | (iType << 4));
				vecTmpShun.push_back(iValue - 2 | (iType << 4));
			}
			else
			{
				vecTmpShun.push_back(iValue | (iType << 4));
				vecTmpShun.push_back(iValue - 1 | (iType << 4));
				vecTmpShun.push_back(iValue + 1 | (iType << 4));
			}

			bool bIfAvailable = CheckIfRandSetCardValue(cAllCard, vecTmpShun);
			if (!bIfAvailable)
			{
				//_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 生成顺子失败 重来 当前 i值[%d]", i);
				i = -1;
				vecTmpShun.clear();
			}
			else
			{
				_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard 生成顺子成功");
				for (int j = 0; j < vecTmpShun.size(); j++)
				{
					vecTmpCard.push_back(vecTmpShun[j]);
					_log(_DEBUG, "HZXLMJ_AIControl", "CreateRandSetCard AssistAIMainCard=[%d]", vecTmpShun[j]);
				}
				break;
			}
		}
	}
}

bool HZXLMJ_AIControl::CheckIfRandSetCardValue(char cAllCard[], vector<char>& vecTmpAssistCard)
{
	char cTmpAllCard[66];
	memset(cTmpAllCard, 0, sizeof(cTmpAllCard));

	memcpy(cTmpAllCard, cAllCard, sizeof(cTmpAllCard));

	for (int i = 0; i < vecTmpAssistCard.size(); i++)
	{
		char cCard = vecTmpAssistCard[i];
		if (cAllCard[cCard] > 0)
		{
			cAllCard[cCard]--;
		}
		else
		{
			memcpy(cAllCard, cTmpAllCard, sizeof(cTmpAllCard));//还原
			return false;
		}
	}
	return true;
}
/***
判断剩余牌墙中某张牌的剩余数量
***/
int HZXLMJ_AIControl::CalWallCardNum(MJGTableItemDef & tableItem, char cCard)
{
	// 先获取牌墙剩余牌
	vector <char> cWallCard;
	for (int i = tableItem.iCardIndex; i <= tableItem.iMaxMoCardNum; i++)
	{
		cWallCard.push_back(tableItem.cAllCards[i]);
	}
	int iCardNum = 0;
	if (cCard >= 0 && cCard < 66)
	{
		for (int i = 0; i < cWallCard.size(); i++)
		{
			if (cWallCard[i] == cCard)
				iCardNum++;
		}
	}
	return iCardNum;
}
/***
判断主AI-2的非主牌和副AI的所有手牌中是否存在某张牌
***/
bool HZXLMJ_AIControl::CheckRestAIHandCard(MJGTableItemDef * tableItem, char cCard, int &iCurIndex)
{
	int iDeputyAIPos = -1;
	int iAssistAIPos = -1;
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (tableItem->pPlayers[i]->iPlayerType == ASSIST_AI_PLAYER)
		{
			iAssistAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
		else if (tableItem->pPlayers[i]->iPlayerType == DEPUTY_AI_PLAYER)
		{
			iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
		}
	}
	/*优先检查副AI手牌,且保证副AI没胡过  */
	//副AI手牌中cCard的位置
	vector<int>vecDeputyCardPos;
	if (!tableItem->pPlayers[iDeputyAIPos]->bIsHu)
	{
		for (int i = 0; i < tableItem->pPlayers[iDeputyAIPos]->iHandCardsNum; i++)
		{
			if (tableItem->pPlayers[iDeputyAIPos]->cHandCards[i] == cCard)
			{
				vecDeputyCardPos.push_back(i);
				break;
			}
		}
	}
	//在副AI的手牌中找到了对应牌
	if (vecDeputyCardPos.size() != 0)
	{
		//与当前摸牌替换
		//iCurIndex = vecDeputyCardPos[0];
		//直接处理交换
		_log(_DEBUG, "HZXLMJ_AIControl", "CheckRestAIHandCard Find MainAI Hucard=[%d] In DeputyAI HandCard[%d]", tableItem->pPlayers[iDeputyAIPos]->cHandCards[vecDeputyCardPos[0]], vecDeputyCardPos[0]);
		char cTmpCard = tableItem->cAllCards[tableItem->iCardIndex];
		tableItem->cAllCards[tableItem->iCardIndex] = tableItem->pPlayers[iDeputyAIPos]->cHandCards[vecDeputyCardPos[0]];
		tableItem->pPlayers[iDeputyAIPos]->cHandCards[vecDeputyCardPos[0]] = cTmpCard;
		return true;
	}
	else
	{
		vector<char> vecAssistAIMainCard;
		vector<int>  vecAssistCardPos;
		//在副AI手牌中没有找到对应牌,在主AI-2的非主要牌中查找对应牌
		if (!tableItem->pPlayers[iAssistAIPos]->bIsHu)
		{
			for (int i = 0; i < 10; i++)
			{
				for (int j = 0; j < 10; j++)
				{
					if (tableItem->pPlayers[iAssistAIPos]->cMainCards[i][j] == 0)
					{
						break;
					}
					else
					{
						vecAssistAIMainCard.push_back(tableItem->pPlayers[iAssistAIPos]->cMainCards[i][j]);
					}
				}
			}

			for (int i = vecAssistAIMainCard.size(); i < tableItem->pPlayers[iAssistAIPos]->iHandCardsNum; i++)
			{
				if (tableItem->pPlayers[iAssistAIPos]->cHandCards[i] == cCard)
				{
					vecAssistCardPos.push_back(i);
					break;
				}
			}
		}

		if (vecAssistCardPos.size() == 0)
		{
			return false;
		}
		else
		{
			//与当前摸牌替换
			//iCurIndex = vecAssistCardPos[0]; 
			_log(_DEBUG, "HZXLMJ_AIControl", "CheckRestAIHandCard Find MainAI Hucard=[%d] In Assist HandCard[%d]", tableItem->pPlayers[iAssistAIPos]->cHandCards[vecAssistCardPos[0]], vecAssistCardPos[0]);
			char cTmpCard = tableItem->cAllCards[tableItem->iCardIndex];
			tableItem->cAllCards[tableItem->iCardIndex] = tableItem->pPlayers[iAssistAIPos]->cHandCards[vecAssistCardPos[0]];
			tableItem->pPlayers[iAssistAIPos]->cHandCards[vecAssistCardPos[0]] = cTmpCard;
			return true;
		}
	}
}
/***
计算使用当前的配牌，主AI-1的可胡牌（非红中）有哪些
***/
void HZXLMJ_AIControl::CalMainAIHuCard(MJGPlayerNodeDef *nodePlayers, vector<char>& vecHuCard)
{
	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CalMainAIHuCard_Log(): nodePlayers is NULL ●●●●");
		return;
	}
	if (nodePlayers->iPlayerType != MIAN_AI_PLAYER)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● CalMainAIHuCard_Log(): nodePlayers Is Not Main AI ●●●●");
		return;
	}
	int iAllCard[5][10];
	memset(&iAllCard, 0, sizeof(iAllCard));
	int iCout = 0;
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			//将主牌中的非缺牌、缺牌中的所有牌存入
			if (nodePlayers->cMainCards[i][j] != -1 && nodePlayers->cMainCards[i][j] != 99)
			{
				int iCardType = nodePlayers->cMainCards[i][j] >> 4;
				int iCardValuie = nodePlayers->cMainCards[i][j] & 0xf;
				iAllCard[iCardType][0]++;
				iAllCard[iCardType][iCardValuie]++;
				iCout++;
			}
			if (nodePlayers->cNeedCards[i][j] != -1)
			{
				int iCardType = nodePlayers->cNeedCards[i][j] >> 4;
				int iCardValuie = nodePlayers->cNeedCards[i][j] & 0xf;
				iAllCard[iCardType][0]++;
				iAllCard[iCardType][iCardValuie]++;
				iCout++;
			}
		}
	}
	vector<char>vecAllCard;
	for (int j = 1; j < 10; j++)
	{
		for (int k = 0; k < 3; k++)
		{
			if (k == nodePlayers->iAIConfirmQueType)
			{
				continue;
			}
			int iCardType = k;
			int iCardValue = j;
			char cTmpCard = iCardValue | (iCardType << 4);
			vecAllCard.push_back(cTmpCard);
		}
	}
	char cTmpHZCard = 1 | (4 << 4);
	vecAllCard.push_back(cTmpHZCard);
	
	for (int i = 0; i < vecAllCard.size(); i++)
	{
		int iTmpAllCard[5][10];
		memcpy(iTmpAllCard, iAllCard, sizeof(iTmpAllCard));

		int iTmpCardType = vecAllCard[i] >> 4;
		int iTmpCardValue = vecAllCard[i] & 0xf;

		iTmpAllCard[iTmpCardType][0]++;
		iTmpAllCard[iTmpCardType][iTmpCardValue]++;

		ResultTypeDef tempResult;
		tempResult.lastCard.iType = iTmpCardType;
		tempResult.lastCard.iValue = iTmpCardValue;
		tempResult.lastCard.bOther = true;
		char cFanResult[MJ_FAN_TYPE_CNT];
		memset(cFanResult, 0, sizeof(cFanResult));

		int iTotalFan = JudgeResult::JudgeHZXLMJHu(iTmpAllCard, tempResult, nodePlayers->iAIConfirmQueType, cFanResult);
		if (iTotalFan > 0)
		{
			//当前牌为可胡牌，存入数组
			vecHuCard.push_back(vecAllCard[i]);
			_log(_ERROR, "HZXLMJ_AIControl", "CalMainAIHuCard_Log(): vecAllCard[%d],iTotalFan[%d]", vecAllCard[i], iTotalFan);
			//mpTingFan[vecAllCard[i]] = iTotalFan;
			nodePlayers->mpTingFan.insert(make_pair(vecAllCard[i], iTotalFan));
		}
	}
}

/***
判断当前摸牌是否是主AI的缺牌
***/
bool HZXLMJ_AIControl::JudgeIsMainAINeedCard(MJGTableItemDef * tableItem, char cCard)
{
	int iMainAIPos;
	for (int i = 0; i < m_iMaxPlayerNum; i++)
	{
		if (tableItem->pPlayers[i]->iPlayerType == MIAN_AI_PLAYER)
		{
			iMainAIPos = tableItem->pPlayers[i]->cTableNumExtra;
			break;
		}
	}

	vector<char> vecTmpMainAINeedCard;
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j] == -1)
			{
				break;
			}
			else
			{
				vecTmpMainAINeedCard.push_back(tableItem->pPlayers[iMainAIPos]->cNeedCards[i][j]);
			}
		}
	}
	if (find(vecTmpMainAINeedCard.begin(), vecTmpMainAINeedCard.end(), cCard) == vecTmpMainAINeedCard.end())
	{
		return false;
	}
	else
	{
		return true;
	}
}


//void HZXLMJ_AIControl::ReSetAssistAIHandCard(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers)
//{
//	_log(_DEBUG, "HZXLMJ_AIControl", "●●●● ReSetAssistAIHandCard_Log()●●●●");
//	nodePlayers->iIfResetHandCard = 1;
//	int iMainAIPos = -1;
//	int iDeputyAIPos = -1;
//	int iNormalPlayerPos = -1;
//	for (int i = 0; i < m_iMaxPlayerNum; i++)
//	{
//		if (tableItem->pPlayers[i]->iPlayerType == DEPUTY_AI_PLAYER)
//		{
//			iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
//		}
//		else if (tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
//		{
//			iNormalPlayerPos = tableItem->pPlayers[i]->cTableNumExtra;
//		}
//		else if (tableItem->pPlayers[i]->iPlayerType == MIAN_AI_PLAYER)
//		{
//			iMainAIPos = tableItem->pPlayers[i]->cTableNumExtra;
//		}
//	}
//	//将副AI-cMainCards中第0区间的主牌取出，第0区间存放的是一组连张，第1区间存放的是主AI-2的三张可胡牌
//	int iAvailableSection = 0;//主AI-2的可用起始区间
//	for (int i = 0; i < 10; i++)
//	{
//		if (nodePlayers->cMainCards[i][0] == -1)
//		{
//			//从当前区间开始是可用区间
//			iAvailableSection = i;
//		}
//	}
//	//存放副AI第0区间主牌
//	vector<char>vecDeputyMainCards;
//	for (int i = 0; i < 10; i++)
//	{
//		if (tableItem->pPlayers[iDeputyAIPos]->cMainCards[0][i] != -1)
//		{
//			_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Deputy-AI cMainCards[%d]", tableItem->pPlayers[iDeputyAIPos]->cMainCards[0][i]);
//			nodePlayers->cMainCards[iAvailableSection][i] == tableItem->pPlayers[iDeputyAIPos]->cMainCards[0][i];
//			vecDeputyMainCards.push_back(tableItem->pPlayers[iDeputyAIPos]->cMainCards[0][i]);
//			//删除对应主牌
//			tableItem->pPlayers[iDeputyAIPos]->cMainCards[0][i] = -1;
//		}
//	}
//	vector<char>vecAssistAINewMainCards;
//	vector<char>vecAssistAIQueCards;
//	vector<char>vecAssistAIRestCards;
//	for (int i = 0; i < 10; i++)
//	{
//		for (int j = 0; j < 10; j++)
//		{
//			if (nodePlayers->cMainCards[i][j] != -1)
//			{
//				if (nodePlayers->cMainCards[i][j] != 99)
//				{
//					_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Assist-AI NEW  cMainCards[%d]", tableItem->pPlayers[iDeputyAIPos]->cMainCards[0][i]);
//					vecAssistAINewMainCards.push_back(nodePlayers->cMainCards[i][j]);
//				}
//				else
//				{
//					//当前区间为刻子区间，且有一张牌为缺牌没有存入，这张牌在牌尾
//					vecAssistAIQueCards.push_back(nodePlayers->cMainCards[i][j + 1]);
//				}
//			}
//		}
//	}
//	//替换AI-2手牌,将副AI第0区间的牌和AI-2手中的非主要牌替换
//	vector<char>vecDeputyNewCards;
//	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
//	{
//		if (vecDeputyMainCards.size() != 0)
//		{
//			_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Assist-AI:Change Cards Finish");
//			if (find(vecAssistAINewMainCards.begin(), vecAssistAINewMainCards.end(), nodePlayers->cHandCards[i]) == vecAssistAINewMainCards.end())
//			{
//				//当前牌需要被替换
//				_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Assist-AI:Cur Change Cards[%d],Change To[%d]", nodePlayers->cHandCards[i], vecDeputyMainCards[0]);
//				char cTmpCard = nodePlayers->cHandCards[i];
//				nodePlayers->cHandCards[i] = vecDeputyMainCards[0];
//				for (int j = 0; j < tableItem->pPlayers[iDeputyAIPos]->iHandCardsNum; j++)
//				{
//					if (tableItem->pPlayers[iDeputyAIPos]->cHandCards[j] == vecDeputyMainCards[0])
//					{
//						tableItem->pPlayers[iDeputyAIPos]->cHandCards[j] = cTmpCard;
//						break;
//					}
//				}
//				vecDeputyNewCards.push_back(cTmpCard);
//				vecDeputyMainCards.erase(vecDeputyMainCards.begin());
//			}
//		}
//		else
//		{
//			//三张牌已经替换完成
//			_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Assist-AI:Change Cards Finish");
//			//判断此时手牌中是否仍旧存在多余牌，即有缺牌未补入的情况
//			if (vecAssistAIQueCards.size() != 0)
//			{
//				//判断当前牌是不是需要的牌
//				if (find(vecAssistAINewMainCards.begin(), vecAssistAINewMainCards.end(), nodePlayers->cHandCards[i]) == vecAssistAINewMainCards.end())
//				{
//					//该牌需要被替换成缺牌
//					_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Assist-AI RestCards[%d]", nodePlayers->cHandCards[i]);
//					vecAssistAIRestCards.push_back(nodePlayers->cHandCards[i]);
//				}
//			}
//		}
//	}
//	if (vecAssistAIRestCards.size() != 0)
//	{
//		//说明此时仍有缺牌没有补入，仍需要替换手牌
//		_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Assist-AI: Que CardsNum=[%d]", vecAssistAIQueCards.size());
//		for (int i = 0; i < vecAssistAIRestCards.size(); i++)
//		{
//			bool bIfNeedChange = false;
//			int iChangePos = -1;
//			for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
//			{
//				if (nodePlayers->cHandCards[j] == vecAssistAIRestCards[i])
//				{
//					//当前牌需要被替换为缺牌
//					bIfNeedChange = true;
//					iChangePos = j;
//					_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Assist-AI RestCards=[%d]", nodePlayers->cHandCards[j]);
//					break;
//				}
//			}
//			if (bIfNeedChange)
//			{
//				for (int j = tableItem->iCardIndex; j < 112; j++)
//				{
//					if (tableItem->cAllCards[j] == vecAssistAIQueCards[i]);
//					{
//						_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Assist-AI RestCards=[%d] to QueCards[%d]", nodePlayers->cHandCards[iChangePos], vecAssistAIQueCards[i]);
//						char cCard = tableItem->cAllCards[j];
//						tableItem->cAllCards[j] = nodePlayers->cHandCards[iChangePos];
//						nodePlayers->cHandCards[iChangePos] = cCard;
//						break;
//					}
//				}
//				
//			}
//		}
//	}
//	//将主AI-2的三张胡牌和牌墙中的非AI-1胡牌交换
//	for (int i = 0; i < nodePlayers->vecResetHuCard.size(); i++)
//	{
//		for (int j = 0; j < tableItem->pPlayers[iDeputyAIPos]->iHandCardsNum; j++)
//		{
//			if (tableItem->pPlayers[iDeputyAIPos]->cHandCards[j] == nodePlayers->vecResetHuCard[i])
//			{
//				for (int k = tableItem->iCardIndex; k < 112; k++)
//				{
//					if (find(tableItem->pPlayers[iMainAIPos]->vecAIHuCard.begin(), tableItem->pPlayers[iMainAIPos]->vecAIHuCard.end(), tableItem->cAllCards[k]) == tableItem->pPlayers[iMainAIPos]->vecAIHuCard.end())
//					{
//						if (find(nodePlayers->vecResetHuCard.begin(), nodePlayers->vecResetHuCard.end(), tableItem->cAllCards[k]) == nodePlayers->vecResetHuCard.end())
//						{
//							//交换手牌
//							_log(_DEBUG, "HZXLMJ_AIControl", "ReSetAssistAIHandCard_Log() Deputy-AI [%d] to [%d]", tableItem->pPlayers[iDeputyAIPos]->cHandCards[j], tableItem->cAllCards[k]);
//							char cCard = tableItem->cAllCards[k];
//							tableItem->cAllCards[k] = tableItem->pPlayers[iDeputyAIPos]->cHandCards[j];
//							tableItem->pPlayers[iDeputyAIPos]->cHandCards[j] = cCard;
//						}
//					}
//				}
//			}
//		}
//	}
//	//此时主AI-2的手牌已经调整完成，需要将副AI手牌重新调整
//	int iMaxFan = 0;
//	vector<char>vecTmpCard;
//	for (int i = 0; i < tableItem->pPlayers[iNormalPlayerPos]->vcTingCard.size(); i++)
//	{
//		map<char, int>::iterator it = tableItem->pPlayers[iNormalPlayerPos]->mpTingFan.find(tableItem->pPlayers[iNormalPlayerPos]->vcTingCard[i]);
//		if (it != tableItem->pPlayers[iNormalPlayerPos]->mpTingFan.end())
//		{
//			if (it->second >= 32)
//			{
//				vecTmpCard.push_back(it->first);
//			}
//		}
//	}
//	vector<Cards>vecRestCard;
//	//计算这些超过封顶番的牌在牌墙中、副AI手牌的余量
//	for (int i = 0; i < vecTmpCard.size(); i++)
//	{
//		int iRestNumInWall = 0;
//		int iRestNumInDeputyAI = 0;
//		for (int j = tableItem->iCardIndex; j < 112; j++)
//		{
//			if (tableItem->cAllCards[j] == vecTmpCard[i])
//			{
//				iRestNumInWall++;
//			}
//		}
//		for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
//		{
//			if (nodePlayers->cHandCards[j] == vecTmpCard[i])
//			{
//				iRestNumInDeputyAI++;
//			}
//		}
//		if (iRestNumInWall + iRestNumInDeputyAI != 0)
//		{
//			CardsDef CurCard;
//			CurCard.cCard = vecTmpCard[i];
//			CurCard.iRestNumInWall = iRestNumInWall;
//			CurCard.iRestNumInDeputyAI = iRestNumInDeputyAI;
//			vecRestCard.push_back(CurCard);
//		}
//	}
//	//开始替换，替换时优先将定缺牌替换掉
//	vector<char>vecTmpHandCard;
//	char cNewMainCard[10][10];
//	memset(&cNewMainCard, -1, sizeof(cNewMainCard));
//	int iCurZuCard = 0;
//	for (int i = 0; i < vecRestCard.size(); i++)
//	{
//		//先判断当前牌是不是副AI的定缺花色
//		if (vecRestCard[i].cCard >> 4 != nodePlayers->iQueType)
//		{
//			if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall >= 3)
//			{
//				//生成刻子
//				for (int j = 0; j < 3; j++)
//				{
//					vecTmpHandCard.push_back(vecRestCard[i].cCard);
//					cNewMainCard[iCurZuCard][j] = vecRestCard[i].cCard;
//					iCurZuCard++;
//				}
//			}
//			else if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall == 2)
//			{
//				//生成对子
//				for (int j = 0; j < 2; j++)
//				{
//					vecTmpHandCard.push_back(vecRestCard[i].cCard);
//					cNewMainCard[iCurZuCard][j] = vecRestCard[i].cCard;
//					iCurZuCard++;
//				}
//			}
//			else if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall == 1)
//			{
//				//生成顺子
//				vector<char>vecShunZi = CreateShunZiForDeputyAI(tableItem, nodePlayers, vecRestCard[i].cCard, vecTmpHandCard);
//				if (vecShunZi.size() != 0)
//				{
//					for (int j = 0; j < vecShunZi.size(); j++)
//					{
//						vecTmpHandCard.push_back(vecShunZi[i]);
//						cNewMainCard[iCurZuCard][j] = vecRestCard[i].cCard;
//					}
//					iCurZuCard++;
//				}
//			}
//			if (iCurZuCard == 3)
//			{
//				break;
//			}
//		}
//	}
//	//处理副AI的maincards
//	memcpy(&tableItem->pPlayers[iDeputyAIPos]->cMainCards, cNewMainCard, sizeof(cNewMainCard));
//	//处理实际手牌,应该优先处理手牌中的定缺花色，使牌型看起来较为合理
//	for (int i = 0; i < tableItem->pPlayers[iDeputyAIPos]->iHandCardsNum; i++)
//	{
//		if (tableItem->pPlayers[iDeputyAIPos]->cHandCards[i] >> 4 == tableItem->pPlayers[iDeputyAIPos]->iQueType)
//		{
//			if (vecTmpHandCard.size() != 0)
//			{
//				if (find(vecTmpHandCard.begin(), vecTmpHandCard.end(), tableItem->pPlayers[iDeputyAIPos]->cHandCards[i]) == vecTmpHandCard.end())
//				{
//					//需要替换,说明需要从牌墙中找一张vecTmpHandCard的牌替换
//					for (int j = tableItem->iCardIndex; j < 112; j++)
//					{
//						if (tableItem->cAllCards[j] == vecTmpHandCard[0])
//						{
//							//替换手牌
//							char cTmpCard = tableItem->cAllCards[j];
//							tableItem->cAllCards[j] = nodePlayers->cHandCards[i];
//							tableItem->pPlayers[iDeputyAIPos]->cHandCards[i] = cTmpCard;
//							//移除第一张
//							vecTmpHandCard.erase(vecTmpHandCard.begin());
//						}
//					}
//				}
//				else
//				{
//					//不需要替换，则将vecTmpHandCard对应牌移除
//					for (int j = 0; j < vecTmpHandCard.size(); j++)
//					{
//						if (vecTmpHandCard[j] == tableItem->pPlayers[iDeputyAIPos]->cHandCards[i])
//						{
//							vecTmpHandCard.erase(vecTmpHandCard.begin() + j);
//						}
//					}
//				}
//			}
//		}
//	}
//	//在处理一次
//	for (int i = 0; i < tableItem->pPlayers[iDeputyAIPos]->iHandCardsNum; i++)
//	{
//		if (vecTmpHandCard.size() != 0)
//		{
//			if (find(vecTmpHandCard.begin(), vecTmpHandCard.end(), tableItem->pPlayers[iDeputyAIPos]->cHandCards[i]) == vecTmpHandCard.end())
//			{
//				//需要替换,说明需要从牌墙中找一张vecTmpHandCard的牌替换
//				for (int j = tableItem->iCardIndex; j < 112; j++)
//				{
//					if (tableItem->cAllCards[j] == vecTmpHandCard[0])
//					{
//						//替换手牌
//						char cTmpCard = tableItem->cAllCards[j];
//						tableItem->cAllCards[j] = nodePlayers->cHandCards[i];
//						tableItem->pPlayers[iDeputyAIPos]->cHandCards[i] = cTmpCard;
//						//移除第一张
//						vecTmpHandCard.erase(vecTmpHandCard.begin());
//					}
//				}
//			}
//			else
//			{
//				//不需要替换，则将vecTmpHandCard对应牌移除
//				for (int j = 0; j < vecTmpHandCard.size(); j++)
//				{
//					if (vecTmpHandCard[j] == tableItem->pPlayers[iDeputyAIPos]->cHandCards[i])
//					{
//						vecTmpHandCard.erase(vecTmpHandCard.begin() + j);
//					}
//				}
//			}
//		}
//	}
//}
/***
在主AI-2未重置手牌的情况下，主AI-2胡牌了，那现在将普通玩家部分可胡牌成组存入副AI手牌中，并作为主牌
***/
//void HZXLMJ_AIControl::ResetDeputyAIHandCard(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers)
//{
//	_log(_DEBUG, "HZXLMJ_AIControl", "●●●● ResetDeputyAIHandCard_Log()●●●●");
//
//	int iAssisAIPos = -1;
//	int iNormalPlayerPos = -1;
//	for (int i = 0; i < m_iMaxPlayerNum; i++)
//	{
//		if (tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
//		{
//			iNormalPlayerPos = tableItem->pPlayers[i]->cTableNumExtra;
//		}
//		else if (tableItem->pPlayers[i]->iPlayerType == ASSIST_AI_PLAYER)
//		{
//			iAssisAIPos = tableItem->pPlayers[i]->cTableNumExtra;
//		}
//	}
//	if (tableItem->pPlayers[iAssisAIPos]->bIsHu && tableItem->pPlayers[iAssisAIPos]->iIfResetHandCard == -1)
//	{
//		_log(_DEBUG, "HZXLMJ_AIControl", "ResetDeputyAIHandCard_Log() [%d]", tableItem->pPlayers[iAssisAIPos]->iIfResetHandCard);
//		return;
//	}
//	nodePlayers->iIfResetHandCard = 1;
//	//获取当前普通玩家胡牌的较大番型牌,暂时只计算超过封顶番的
//	int iMaxFan = 0;
//	vector<char>vecTmpCard;
//	for (int i = 0; i < tableItem->pPlayers[iNormalPlayerPos]->vcTingCard.size(); i++)
//	{
//		map<char, int>::iterator it = tableItem->pPlayers[iNormalPlayerPos]->mpTingFan.find(tableItem->pPlayers[iNormalPlayerPos]->vcTingCard[i]);
//		if (it != tableItem->pPlayers[iNormalPlayerPos]->mpTingFan.end())
//		{
//			if (it->second >= 32)
//			{
//				vecTmpCard.push_back(it->first);
//			}
//		}
//	}
//
//	vector<Cards>vecRestCard;
//	//计算这些超过封顶番的牌在牌墙中、副AI手牌的余量
//	for (int i = 0; i < vecTmpCard.size(); i++)
//	{
//		int iRestNumInWall = 0;
//		int iRestNumInDeputyAI = 0;
//		for (int j = tableItem->iCardIndex; j < 112; j++)
//		{
//			if (tableItem->cAllCards[j] == vecTmpCard[i])
//			{
//				iRestNumInWall++;
//			}
//		}
//		for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
//		{
//			if (nodePlayers->cHandCards[j] == vecTmpCard[i])
//			{
//				iRestNumInDeputyAI++;
//			}
//		}
//		if (iRestNumInWall + iRestNumInDeputyAI != 0)
//		{
//			CardsDef CurCard;
//			CurCard.cCard = vecTmpCard[i];
//			CurCard.iRestNumInWall = iRestNumInWall;
//			CurCard.iRestNumInDeputyAI = iRestNumInDeputyAI;
//			vecRestCard.push_back(CurCard);
//		}
//	}
//	//处理这些剩余牌，生成成组卡牌，成组数量限制（最多生成三组太多了会过假）
//	vector<char>vecTmpHandCard;
//	char cNewMainCard[10][10];
//	memset(&cNewMainCard, 0, sizeof(cNewMainCard));
//	int iCurZuCard = 0;
//	for (int i = 0; i < vecRestCard.size(); i++)
//	{
//		//先判断当前牌是不是副AI的定缺花色
//		if (vecRestCard[i].cCard >> 4 != nodePlayers->iQueType)
//		{
//			if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall >= 3)
//			{
//				//生成刻子
//				for (int j = 0; j < 3; j++)
//				{
//					vecTmpHandCard.push_back(vecRestCard[i].cCard);
//					cNewMainCard[iCurZuCard][j] = vecRestCard[i].cCard;
//					iCurZuCard++;
//				}
//			}
//			else if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall == 2)
//			{
//				//生成对子
//				for (int j = 0; j < 2; j++)
//				{
//					vecTmpHandCard.push_back(vecRestCard[i].cCard);
//					cNewMainCard[iCurZuCard][j] = vecRestCard[i].cCard;
//					iCurZuCard++;
//				}
//			}
//			else if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall == 1)
//			{
//				//生成顺子
//				vector<char>vecShunZi = CreateShunZiForDeputyAI(tableItem, nodePlayers, vecRestCard[i].cCard, vecTmpHandCard);
//				if (vecShunZi.size() != 0)
//				{
//					for (int j = 0; j < vecShunZi.size(); j++)
//					{
//						vecTmpHandCard.push_back(vecShunZi[i]);
//						cNewMainCard[iCurZuCard][j] = vecRestCard[i].cCard;
//					}
//					iCurZuCard++;
//				}
//			}
//			if (iCurZuCard == 3)
//			{
//				break;
//			}
//		}
//	}
//
//	//先重新设定副AI的CMainCards
//	memcpy(&nodePlayers->cMainCards, cNewMainCard, sizeof(cNewMainCard));
//	//替换实际手牌
//	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
//	{
//		if (nodePlayers->cHandCards[i] >> 4 == nodePlayers->iQueType)
//		{
//			if (vecTmpHandCard.size() != 0)
//			{
//				if (find(vecTmpHandCard.begin(), vecTmpHandCard.end(), nodePlayers->cHandCards[i]) == vecTmpHandCard.end())
//				{
//					//需要替换,说明需要从牌墙中找一张vecTmpHandCard的牌替换
//					for (int j = tableItem->iCardIndex; j < 112; j++)
//					{
//						if (tableItem->cAllCards[j] == vecTmpHandCard[0])
//						{
//							//替换手牌
//							char cTmpCard = tableItem->cAllCards[j];
//							tableItem->cAllCards[j] = nodePlayers->cHandCards[i];
//							nodePlayers->cHandCards[i] = cTmpCard;
//							//移除第一张
//							vecTmpHandCard.erase(vecTmpHandCard.begin());
//						}
//					}
//				}
//				else
//				{
//					//不需要替换，则将vecTmpHandCard对应牌移除
//					for (int j = 0; j < vecTmpHandCard.size(); j++)
//					{
//						if (vecTmpHandCard[j] == nodePlayers->cHandCards[i])
//						{
//							vecTmpHandCard.erase(vecTmpHandCard.begin() + j);
//						}
//					}
//				}
//			}
//		}
//	}
//	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
//	{
//		if (vecTmpHandCard.size() != 0)
//		{
//			if (find(vecTmpHandCard.begin(), vecTmpHandCard.end(), nodePlayers->cHandCards[i]) == vecTmpHandCard.end())
//			{
//				//需要替换,说明需要从牌墙中找一张vecTmpHandCard的牌替换
//				for (int j = tableItem->iCardIndex; j < 112; j++)
//				{
//					if (tableItem->cAllCards[j] == vecTmpHandCard[0])
//					{
//						//替换手牌
//						char cTmpCard = tableItem->cAllCards[j];
//						tableItem->cAllCards[j] = nodePlayers->cHandCards[i];
//						nodePlayers->cHandCards[i] = cTmpCard;
//						//移除第一张
//						vecTmpHandCard.erase(vecTmpHandCard.begin());
//					}
//				}
//			}
//			else
//			{
//				//不需要替换，则将vecTmpHandCard对应牌移除
//				for (int j = 0; j < vecTmpHandCard.size(); j++)
//				{
//					if (vecTmpHandCard[j] == nodePlayers->cHandCards[i])
//					{
//						vecTmpHandCard.erase(vecTmpHandCard.begin() + j);
//					}
//				}
//			}
//		}
//	}
//}
/***
剩余张数<=20 此时如果主AI-2没有胡牌、没有二次配牌，将六张AI-2的 需要的牌存入牌墙
***/
//void HZXLMJ_AIControl::ResetDeputyAIHandCardToWall(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers)
//{
//	int iNormalPlayerPos = -1;
//	for (int i = 0; i < 4; i++)
//	{
//		if (tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
//		{
//			iNormalPlayerPos = tableItem->pPlayers[i]->cTableNumExtra;
//		}
//	}
//	//将得到的六张牌做替换，替换原则，成组替换
//	for (int i = 0; i < 10; i++)
//	{
//		for (int j = 0; j < 10; j++)
//		{
//			if (nodePlayers->cMainCards[i][j] != -1)
//			{
//				nodePlayers->vecAssistAICards.push_back(nodePlayers->cMainCards[i][j]);
//				nodePlayers->cMainCards[i][j] = -1;
//			}
//		}
//	}
//	
//	if (tableItem->pPlayers[iNormalPlayerPos]->bIsHu)
//	{
//		vector<char>vecTmpCard;
//		for (int i = 0; i < tableItem->pPlayers[iNormalPlayerPos]->vcTingCard.size(); i++)
//		{
//			map<char, int>::iterator it = tableItem->pPlayers[iNormalPlayerPos]->mpTingFan.find(tableItem->pPlayers[iNormalPlayerPos]->vcTingCard[i]);
//			if (it != tableItem->pPlayers[iNormalPlayerPos]->mpTingFan.end())
//			{
//				if (it->second >= 32)
//				{
//					vecTmpCard.push_back(it->first);
//				}
//			}
//		}
//		vector<Cards>vecRestCard;
//
//		//计算这些超过封顶番的牌在牌墙中、副AI手牌的余量
//		for (int i = 0; i < vecTmpCard.size(); i++)
//		{
//			int iRestNumInWall = 0;
//			int iRestNumInDeputyAI = 0;
//			for (int j = tableItem->iCardIndex; j < 112; j++)
//			{
//				if (tableItem->cAllCards[j] == vecTmpCard[i])
//				{
//					iRestNumInWall++;
//				}
//			}
//			for (int j = 0; j < nodePlayers->iHandCardsNum; j++)
//			{
//				if (nodePlayers->cHandCards[j] == vecTmpCard[i])
//				{
//					iRestNumInDeputyAI++;
//				}
//			}
//			if (iRestNumInWall + iRestNumInDeputyAI != 0)
//			{
//				CardsDef CurCard;
//				CurCard.cCard = vecTmpCard[i];
//				CurCard.iRestNumInWall = iRestNumInWall;
//				CurCard.iRestNumInDeputyAI = iRestNumInDeputyAI;
//				vecRestCard.push_back(CurCard);
//			}
//		}
//		//处理这些剩余牌，生成2组卡牌
//		vector<char>vecTmpHandCard;
//		char cNewMainCard[10][10];
//		memset(&cNewMainCard, 0, sizeof(cNewMainCard));
//		int iCurZuCard = 0;
//		for (int i = 0; i < vecRestCard.size(); i++)
//		{
//			//先判断当前牌是不是副AI的定缺花色
//			if (vecRestCard[i].cCard >> 4 != nodePlayers->iQueType)
//			{
//				if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall >= 3)
//				{
//					//生成刻子
//					for (int j = 0; j < 3; j++)
//					{
//						vecTmpHandCard.push_back(vecRestCard[i].cCard);
//						cNewMainCard[iCurZuCard][j] = vecRestCard[i].cCard;
//						iCurZuCard++;
//					}
//				}
//				else if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall == 2)
//				{
//					//生成对子
//					for (int j = 0; j < 2; j++)
//					{
//						vecTmpHandCard.push_back(vecRestCard[i].cCard);
//						cNewMainCard[iCurZuCard][j] = vecRestCard[i].cCard;
//						iCurZuCard++;
//					}
//				}
//				else if (vecRestCard[i].iRestNumInDeputyAI + vecRestCard[i].iRestNumInWall == 1)
//				{
//					//生成顺子
//					vector<char>vecShunZi = CreateShunZiForDeputyAI(tableItem, nodePlayers, vecRestCard[i].cCard, vecTmpHandCard);
//					if (vecShunZi.size() != 0)
//					{
//						for (int j = 0; j < vecShunZi.size(); j++)
//						{
//							vecTmpHandCard.push_back(vecShunZi[i]);
//							cNewMainCard[iCurZuCard][j] = vecShunZi[i];
//						}
//						iCurZuCard++;
//					}
//				}
//				if (iCurZuCard == 2)
//				{
//					break;
//				}
//			}
//		}
//
//		//替换
//		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
//		{
//			if (vecTmpHandCard.size() != 0)
//			{
//				if (find(nodePlayers->vecAssistAICards.begin(), nodePlayers->vecAssistAICards.end(), nodePlayers->cHandCards[i]) != nodePlayers->vecAssistAICards.end())
//				{
//					for (int j = tableItem->iCardIndex; j < 112; j++)
//					{
//						if (tableItem->cAllCards[j] == vecTmpHandCard[0])
//						{
//							char cCard = vecTmpHandCard[0];
//							tableItem->cAllCards[j] = nodePlayers->cHandCards[i];
//							nodePlayers->cHandCards[i] = cCard;
//							break;
//						}
//					}
//					vecTmpHandCard.erase(vecTmpHandCard.begin());
//				}
//			}
//		}
//
//		memcpy(&nodePlayers->cMainCards, cNewMainCard, sizeof(cNewMainCard));
//	}
//	else
//	{
//		//随机替换
//		for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
//		{
//			if (find(nodePlayers->vecAssistAICards.begin(), nodePlayers->vecAssistAICards.end(), nodePlayers->cHandCards[i]) != nodePlayers->vecAssistAICards.end())
//			{
//				for (int j = tableItem->iCardIndex; j < 112; j++)
//				{
//					if (tableItem->cAllCards[j] >> 4 != nodePlayers->iQueType)
//					{
//						char cCard = tableItem->cAllCards[j];
//						tableItem->cAllCards[j] = nodePlayers->cHandCards[i];
//						nodePlayers->cHandCards[i] = cCard;
//						break;
//					}
//				}
//			}
//		}
//		memset(&nodePlayers->cMainCards, -1, sizeof(nodePlayers->cMainCards));
//	}
//}
//
//void HZXLMJ_AIControl::ResetAssistAIHandCardFromWall(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers)
//{
//	int iDeputyAIPos = -1;
//	for (int i = 0; i < 4; i++)
//	{
//		if (tableItem->pPlayers[i]->iPlayerType == NORMAL_PLAYER)
//		{
//			iDeputyAIPos = tableItem->pPlayers[i]->cTableNumExtra;
//		}
//	}
//	vector<char>vecAssistAIMainCards;
//	for (int i = 0; i < 10; i++)
//	{
//		for (int j = 0; j < 10; j++)
//		{
//			if (nodePlayers->cMainCards[i][j] != -1 && nodePlayers->cMainCards[i][j] != 99)
//			{
//				vecAssistAIMainCards.push_back(nodePlayers->cMainCards[i][j]);
//			}
//		}
//	}
//	vector<char>vecTmpCards;
//	for (int i = 0; i < 3; i++)
//	{
//		vecTmpCards.push_back(tableItem->pPlayers[iDeputyAIPos]->vecAssistAICards[i]);
//	}
//	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
//	{
//		if (vecAssistAIMainCards.size() != 0)
//		{
//			if (find(vecAssistAIMainCards.begin(), vecAssistAIMainCards.end(), nodePlayers->cHandCards[i]) == vecAssistAIMainCards.end())
//			{
//				for (int j = tableItem->iCardIndex; j < 112; j++)
//				{
//					if (tableItem->cAllCards[j] == vecAssistAIMainCards[0])
//					{
//						tableItem->cAllCards[j] = nodePlayers->cHandCards[i];
//						nodePlayers->cHandCards[i] = vecAssistAIMainCards[0];
//						break;
//					}
//				}
//				vecAssistAIMainCards.erase(vecAssistAIMainCards.begin());
//			}
//		}
//	}
//	//校验此时是否仍有不是主牌的牌，如果有意味着缺牌没有补齐
//	vector<char>vecNeedCards;
//	for (int i = 0; i < 10; i++)
//	{
//		for (int j = 0; j < 10; j++)
//		{
//			if (nodePlayers->cNeedCards[i][j] != -1)
//			{
//				vecNeedCards.push_back(nodePlayers->cNeedCards[i][j]);
//			}
//		}
//	}
//	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
//	{
//		if (vecNeedCards.size() != 0)
//		{
//			if (find(vecAssistAIMainCards.begin(), vecAssistAIMainCards.end(), nodePlayers->cHandCards[i]) == vecNeedCards.end())
//			{
//				for (int j = tableItem->iCardIndex; j < 112; j++)
//				{
//					if (tableItem->cAllCards[j] == vecNeedCards[0])
//					{
//						tableItem->cAllCards[j] = nodePlayers->cHandCards[i];
//						nodePlayers->cHandCards[i] = vecNeedCards[0];
//						break;
//					}
//				}
//				vecNeedCards.erase(vecNeedCards.begin());
//			}
//		}
//	}
//}

/***
cCard:当前需要生成顺子的牌
vecShun:重置副AI手牌时已经使用过的牌
***/
vector<char> HZXLMJ_AIControl::CreateShunZiForDeputyAI(MJGTableItemDef * tableItem, MJGPlayerNodeDef * nodePlayers, char cCard, vector<char>vecCards)
{
	//处理剩余牌墙+副AI剩余牌
	int iAllCard[5][10];
	memset(&iAllCard, 0, sizeof(iAllCard));
	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		int iCardType = nodePlayers->cHandCards[i] >> 4;
		int iCardValue = nodePlayers->cHandCards[i] % 0xf;

		iAllCard[iCardType][0]++;
		iAllCard[iCardType][iCardValue]++;
	}
	for (int i = tableItem->iCardIndex; i < 112; i++)
	{
		int iCardType = tableItem->cAllCards[i] >> 4;
		int iCardValue = tableItem->cAllCards[i] % 0xf;

		iAllCard[iCardType][0]++;
		iAllCard[iCardType][iCardValue]++;
	}
	//处理已经使用的牌
	for (int i = 0; i < vecCards.size(); i++)
	{
		int iCardType = vecCards[i] >> 4;
		int iCardValue = vecCards[i] % 0xf;

		iAllCard[iCardType][0]--;
		iAllCard[iCardType][iCardValue]--;
	}

	vector<char>vecShunZi;
	int iCardType = cCard >> 4;
	int iCardValue = cCard & 0xf;

	char cShunZiFirst = 0;
	char cShunZiSecond = 0;

	if (iCardValue == 1)
	{
		cShunZiFirst = (iCardType << 4) | (iCardValue + 1);
		cShunZiSecond = (iCardType << 4) | (iCardValue + 2);
		//校验余量
		if (iAllCard[iCardType][iCardValue + 1] > 0 && iAllCard[iCardType][iCardValue + 2] > 0)
		{
			vecShunZi.push_back(cShunZiFirst);
			vecShunZi.push_back(cShunZiSecond);
		}
	}
	else if (iCardValue == 2)
	{
		//此时有两种配顺子方法以2位卡当/以2为第一张
		int iRandType = rand() % 2;
		int iFindTimes = 0;
		for (int i = 0; i < 1; i++)
		{
			if (iRandType == 0)
			{
				//以2为卡当寻找顺子组合
				cShunZiFirst = (iCardType << 4) | (iCardValue + 1);
				cShunZiSecond = (iCardType << 4) | (iCardValue - 1);

				//余量校验
				if (iAllCard[iCardType][iCardValue + 1] > 0 && iAllCard[iCardType][iCardValue - 1] > 0)
				{
					vecShunZi.push_back(cShunZiFirst);
					vecShunZi.push_back(cShunZiSecond);
					break;
				}
				else
				{
					//当前组合的顺子余量不足
					i--;
					iFindTimes++;
					iRandType = 1 - iRandType;
				}
			}
			else
			{
				//以2为第一张寻找顺子组合
				cShunZiFirst = (iCardType << 4) | (iCardValue + 1);
				cShunZiSecond = (iCardType << 4) | (iCardValue + 2);
				//余量校验
				if (iAllCard[iCardType][iCardValue + 1] > 0 && iAllCard[iCardType][iCardValue + 2] > 0)
				{
					vecShunZi.push_back(cShunZiFirst);
					vecShunZi.push_back(cShunZiSecond);
					break;
				}
				else
				{
					//当前组合的顺子余量不足
					i--;
					iFindTimes++;
					iRandType = 1 - iRandType;
				}
			}
			if (iFindTimes >= 2)
			{
				//找了两次都没找到合适的顺子组合，说明找不到了
				break;
			}
		}
	}
	else if (iCardValue >= 3 && iCardValue <= 7)
	{
		//此时有三种配顺子方法以iCardValue为卡当/以iCardValue为第一张/以iCardValue为最后一张
		int iRandType = rand() % 3;
		int iFindTimes = 0;
		for (int i = 0; i < 1; i++)
		{
			if (iRandType == 0)
			{
				//iCardValue为卡当查找
				cShunZiFirst = (iCardType << 4) | (iCardValue + 1);
				cShunZiSecond = (iCardType << 4) | (iCardValue - 1);
				//余量校验
				if (iAllCard[iCardType][iCardValue + 1] > 0 && iAllCard[iCardType][iCardValue - 1] > 0)
				{
					vecShunZi.push_back(cShunZiFirst);
					vecShunZi.push_back(cShunZiSecond);
					break;
				}
				else
				{
					//当前组合的顺子余量不足
					i--;
					iFindTimes++;
					iRandType = (iRandType + 1) % 3;
				}
			}
			else if (iRandType == 1)
			{
				//iCardValue为第一张查找
				cShunZiFirst = (iCardType << 4) | (iCardValue + 1);
				cShunZiSecond = (iCardType << 4) | (iCardValue + 2);
				//余量校验
				if (iAllCard[iCardType][iCardValue + 1] > 0 && iAllCard[iCardType][iCardValue + 2] > 0)
				{
					vecShunZi.push_back(cShunZiFirst);
					vecShunZi.push_back(cShunZiSecond);
					break;
				}
				else
				{
					//当前组合的顺子余量不足
					i--;
					iFindTimes++;
					iRandType = (iRandType + 1) % 3;
				}
			}
			else if (iRandType == 2)
			{
				//iCardValue为最后一张查找
				cShunZiFirst = (iCardType << 4) | (iCardValue - 1);
				cShunZiSecond = (iCardType << 4) | (iCardValue - 2);
				//余量校验
				if (iAllCard[iCardType][iCardValue - 1] > 0 && iAllCard[iCardType][iCardValue - 2] > 0)
				{
					vecShunZi.push_back(cShunZiFirst);
					vecShunZi.push_back(cShunZiSecond);
					break;
				}
				else
				{
					//当前组合的顺子余量不足
					i--;
					iFindTimes++;
					iRandType = (iRandType + 1) % 3;
				}
			}
			if (iFindTimes >= 3)
			{
				//已经查找三次，什么没有可用的顺子组合
				break;
			}
		}
	}
	else if (iCardValue == 8)
	{
		//此时有两种配顺子方法以8位卡当/8为最后一张
		int iRandType = rand() % 2;
		int iFindTimes = 0;
		for (int i = 0; i < 1; i++)
		{
			if (iRandType == 0)
			{
				//以8为卡当寻找顺子组合
				cShunZiFirst = (iCardType << 4) | (iCardValue + 1);
				cShunZiSecond = (iCardType << 4) | (iCardValue - 1);
				//余量校验
				if (iAllCard[iCardType][iCardValue + 1] > 0 && iAllCard[iCardType][iCardValue - 1] > 0)
				{
					vecShunZi.push_back(cShunZiFirst);
					vecShunZi.push_back(cShunZiSecond);
					break;
				}
				else
				{
					//当前组合的顺子余量不足
					i--;
					iFindTimes++;
					iRandType = 1 - iRandType;
				}
			}
			else
			{
				//以8为最后一张寻找顺子组合
				cShunZiFirst = (iCardType << 4) | (iCardValue - 1);
				cShunZiSecond = (iCardType << 4) | (iCardValue - 2);
				//余量校验
				if (iAllCard[iCardType][iCardValue - 1] > 0 && iAllCard[iCardType][iCardValue - 2] > 0)
				{
					vecShunZi.push_back(cShunZiFirst);
					vecShunZi.push_back(cShunZiSecond);
					break;
				}
				else
				{
					//当前组合的顺子余量不足
					i--;
					iFindTimes++;
					iRandType = 1 - iRandType;
				}
			}
			if (iFindTimes >= 2)
			{
				//找了两次都没找到合适的顺子组合，说明找不到了
				break;
			}
		}
	}
	else if (iCardValue == 9)
	{
		cShunZiFirst = (iCardType << 4) | (iCardValue - 1);
		cShunZiSecond = (iCardType << 4) | (iCardValue - 2);
		//校验余量
		if (iAllCard[iCardType][iCardValue + 1] > 0 && iAllCard[iCardType][iCardValue + 2] > 0)
		{
			vecShunZi.push_back(cShunZiFirst);
			vecShunZi.push_back(cShunZiSecond);
		}
	}

	return vecShunZi;
}
/***
针对主AI-2、副AI的寻找最优出牌
***/
char HZXLMJ_AIControl::FindOptimalSingleCard(MJGPlayerNodeDef * nodePlayers)
{

	if (!nodePlayers)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● FindOptimalSingleCard_Log(): nodePlayers is NULL ●●●●");
		return 0;
	}
	if (nodePlayers->iPlayerType != ASSIST_AI_PLAYER && nodePlayers->iPlayerType != DEPUTY_AI_PLAYER)
	{
		_log(_ERROR, "HZXLMJ_AIControl", "●●●● FindOptimalSingleCard_Log(): nodePlayers Is Not Assist AI OR Deputy AI  ●●●●");
		return 0;
	}
	_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): AI[%d] FindCard ", nodePlayers->iPlayerType);
	//处理手牌
	int iHandCard[5][10];
	memset(iHandCard, 0, sizeof(iHandCard));
	for (int i = 0; i < nodePlayers->iHandCardsNum; i++)
	{
		int iCardType = nodePlayers->cHandCards[i] >> 4;
		int iCardValue = nodePlayers->cHandCards[i] & 0xf;
		//_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): AI HandCard[%d] ", nodePlayers->cHandCards[i]);
	
		iHandCard[iCardType][0]++;
		iHandCard[iCardType][iCardValue]++;
	}
	
	//获取剩余可出牌
	vector<char>vecRestCards;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 1; j < 10; j++)
		{
			if (iHandCard[i][j] > 0)
			{
				char cCard = i << 4 | j;
				vecRestCards.push_back(cCard);
				//_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): AI 可出[%d] 张数[%d] ", cCard, iHandCard[i][j]);
			}
		}
	}
	//先出定缺牌
	if (iHandCard[nodePlayers->iQueType][0] != 0)
	{
		for (int j = 1; j < 10; j++)
		{
			if (iHandCard[nodePlayers->iQueType][j] != 0)
			{
				char cCard = (nodePlayers->iQueType << 4) | j;
				_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): AI 出定缺拍[%d] ", cCard);
				return cCard;
			}
		}
	}

	//判断当前是否有单张
	_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 出单张");
	vector<char>vecSingleCard;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 1; j < 10; j++)
		{
			if (iHandCard[i][j] == 1)
			{
				if (j == 1 && iHandCard[i][j + 1] == 0 && iHandCard[i][j + 2] == 0)
				{
					char cCard = (i << 4) | j;
					vecSingleCard.push_back(cCard);
				}
				else if (j == 9 && iHandCard[i][j - 1] == 0 && iHandCard[i][j - 2] == 0)
				{
					char cCard = (i << 4) | j;
					vecSingleCard.push_back(cCard);
				}
				else if (iHandCard[i][j - 1] == 0 && iHandCard[i][j + 1] == 0)
				{
					char cCard = (i << 4) | j;
					vecSingleCard.push_back(cCard);
				}
			}
		}
	}
	if (vecSingleCard.size() != 0)
	{
		//优先出边张
		char  cCard = 0;
		int iMinDifference = 10;
		for (int i = 0; i < vecSingleCard.size(); i++)
		{
			int iCardValue = vecSingleCard[i] & 0xf;
			if (iCardValue - 1 < iMinDifference)
			{
				iMinDifference = iCardValue - 1;
				cCard = vecSingleCard[i];
			}
			if (9 - iCardValue < iMinDifference)
			{
				iMinDifference = 9 - iCardValue;
				cCard = vecSingleCard[i];
			}
		}
		if (cCard != 0)
		{
			_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 出单张 [%d]", cCard);
			return cCard;
		}
	}
	//判断对子数量
	_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 出单张失败");
	int iDuiZiCount = 0;
	vector<char>vecDuiZi;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 1; j < 10; j++)
		{
			if (iHandCard[i][j] == 2)
			{
				iDuiZiCount++;
				char cCard = (i << 4) | j;
				_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 对子[%d]", cCard);
				vecDuiZi.push_back(cCard);

			}
		}
	}
	if (iDuiZiCount > 1)
	{
		// 多对将牌
		char  cCard = 0;
		int iMinDifference = 10;
		for (int i = 0; i < vecDuiZi.size(); i++)
		{
			int iCardValue = vecDuiZi[i] & 0xf;
			if (iCardValue - 1 < iMinDifference)
			{
				iMinDifference = iCardValue - 1;
				cCard = vecDuiZi[i];
			}
			if (9 - iCardValue < iMinDifference)
			{
				iMinDifference = 9 - iCardValue;
				cCard = vecDuiZi[i];
			}
		}
		if (cCard != 0)
		{
			return cCard;
		}
	}
	//对子数量不足或过多，则需要拆无效卡当
	_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 拆对子失败");
	for (int i = 0; i < 3; i++)
	{
		for (int j = 1; j < 10; j++)
		{
			if (j < 4)
			{
				if (iHandCard[i][j] == 1 && iHandCard[i][j + 2] > 1 && iHandCard[i][j + 3] > 1)
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
			else if (j > 6)
			{
				if (iHandCard[i][j] == 1 && iHandCard[i][j - 2] > 1 && iHandCard[i][j - 3] > 1)
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
			else
			{
				if (iHandCard[i][j] == 1 && iHandCard[i][j - 2] > 1 && iHandCard[i][j - 3] > 1)
				{
					char cCard = j | (i << 4);
					return cCard;
				}
				if (iHandCard[i][j] == 1 && iHandCard[i][j + 2] > 1 && iHandCard[i][j + 3] > 1)
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
		}
	}
	//拆连张
	_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 拆无效卡当失败");
	//拆卡当
	for (int i = 0; i < 3; i++)
	{
		for (int j = 1; j < 10; j++)
		{
			if (j < 3)
			{
				if (iHandCard[i][j] > 0 && iHandCard[i][j + 2] > 0)
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
			else if (j > 7)
			{
				if (iHandCard[i][j] > 0 && iHandCard[i][j - 2] > 0)
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
			else
			{
				if ((iHandCard[i][j] > 0 && iHandCard[i][j - 2] > 0) ||(iHandCard[i][j] > 0 && iHandCard[i][j + 2] > 0))
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
		}
	}
	_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 拆卡当失败");
	for (int i = 0; i < 3; i++)
	{
		for (int j = 1; j < 10; j++)
		{
			if (j == 1)
			{
				if (iHandCard[i][j] > 0 && iHandCard[i][j + 1] > 0)
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
			else if (j == 9)
			{
				if (iHandCard[i][j] > 0 && iHandCard[i][j - 1] > 0)
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
			else
			{
				if ((iHandCard[i][j] > 0 && iHandCard[i][j - 1] > 0) || (iHandCard[i][j] > 0 && iHandCard[i][j + 1] > 0))
				{
					char cCard = j | (i << 4);
					return cCard;
				}
			}
		}
	}
	_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 拆连张失败");
	int iRandCard = rand() % vecRestCards.size();
	_log(_ERROR, "HZXLMJ_AIControl", "FindOptimalSingleCard_Log(): 随机出牌[%d]", vecRestCards[iRandCard]);
	return vecRestCards[iRandCard];
}
