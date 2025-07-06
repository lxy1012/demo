#pragma once
#ifndef _HZXL_AI_CONTROL_H
#define _HZXL_AI_CONTROL_H

#include   <map> 
#include "log.h"
#include "msg.h"
#include "mjg_tableitem.h"
#include "mjg_playernode.h"
#include "gamefactorycomm.h"

using namespace std;
#define KEY_HZXL_CONTROL_GAME_NUM	"HZXL_CONTROL_GAME_NUM"
#define KEY_HZXL_CONTROL_GAME_RATE	"HZXL_CONTROL_GAME_RATE"
enum PlayerType
{
	NORMAL_PLAYER = 0,	//普通玩家
	MIAN_AI_PLAYER,		//主AI-1
	ASSIST_AI_PLAYER,	//主AI-2
	DEPUTY_AI_PLAYER,    //副AI
	ROBOT_AI_PLAYER,	// 最后一个AI
};

enum AIPlayerDelayMsgType
{
	AI_DELAY_TYPE_CHANGE_CARD = 1,
	AI_DELAY_TYPE_CONFIRM_QUE,
	AI_DELAY_TYPE_SEND_CARD,
	AI_DELAY_TYPE_SPECIAL_CARD,
};
enum GearNum
{
	first_level=0,
	second_level,
	third_level,
	fourth_level,
	fifth_level,
	level_max
};
enum control_rate
{
	game_rate_low = 0,		//游戏几率下界
	game_rate_up,			//游戏几率上界
	room_type,				//房间类型下界
	amount_low,				//净分下界
	amount_up,				//净分上界
	control_rate,			//控制概率
	control_max
};

// AI玩家延时标记定义
const int AI_DELAY_LEAST_TIME = 3;		// 延时时间下限
const int AI_CHANGE_CARD_TIME = 3;		//换牌时间上限
const int AI_CONFIRM_QUE_TIME = 3;		//定缺时间上限
const int AI_SEND_CARD_TIME = 9;		// 出牌时间上限
const int AI_SPECIAL_CARD_TIME = 5;		// 操作时间上限

class HZXLMJ_AIControl :GameLogic
{
//AI相关
public:
	HZXLMJ_AIControl();
	~HZXLMJ_AIControl();

	void ReadRoomConf();																												//获取AI相关配置
	void ReadParamValueConf();																											//获取params表数据

	void GetAIVirtualNickName(MJGTableItemDef *tableItem, int iAIExtra, char* szNickName, int iLen);			//初始化AI玩家名字
	void GetAIVirtualArea(MJGTableItemDef *tableItem, char* szAreaNAme);								//初始化AI玩家的地址
	long long	 GetAIVirtualMoney(int iRoomID);														//初始化AI玩家金币
	int  GetAIVirtualLevel();																					//初始化AI玩家等级
	

	void CreateAIChangeCardReq(MJGTableItemDef *tableItem, MJGPlayerNodeDef *pPlayers);													//模拟客户端，生成AI的换牌请求
	void CreateAIConfirmQueReq(MJGTableItemDef* tableItem, MJGPlayerNodeDef *pPlayers);													//模拟客户端，生成AI的定缺请求
	void CreateAISendCardReq(MJGTableItemDef *tableItem, MJGPlayerNodeDef *pPlayers);													//模拟客户端，生成AI的出牌请求
	void CreateAISpcialCardReq(MJGTableItemDef *tableItem, MJGPlayerNodeDef *pPlayers);													//模拟客户端，生成AI的特殊操作请求（碰杠胡）

	void CreateChangeCardAIMsg(MJGTableItemDef *tableItem, MJGPlayerNodeDef *pPlayers, ChangeCardsReqDef &msgChangeCards);				//创建AI换牌消息
	void CreateConfirmQueAIMsg(MJGTableItemDef* tableItem, MJGPlayerNodeDef *pPlayers, ConfirmQueReqDef &msgConfirmQue);				//创建AI定缺消息
	void CreateSendCardAIMsg(MJGTableItemDef *tableItem, MJGPlayerNodeDef *pPlayers, SendCardsReqDef msgSendCards);						//创建AI出牌消息
	void CreateSpecialCardAIMsg(MJGTableItemDef *tableItem, MJGPlayerNodeDef *pPlayers, SpecialCardsReqDef msgSpecialCard);				//创建AI特殊操作消息（碰杠胡）
	//以下四个handle函数都移植到主逻辑
	//void HandleChangeCardsReqsAI(MJGPlayerNodeDef *pPlayers, void *pMsgData);															//服务端处理客户端AI玩家的换牌请求	
	//void HandleConfirmQueReqAI(MJGPlayerNodeDef *pPlayers, void *pMsgData);																//服务端处理客户端AI玩家的定缺消息
	//void HandleSendCardsReqAI(MJGPlayerNodeDef *pPlayers, void *pMsgData);																//服务端处理客户端AI玩家请求出牌消息
	//void HandleSpecialCardsReqAI(MJGPlayerNodeDef *pPlayers, void *pMsgData);															//服务端处理客户端AI玩家特殊操作消息（碰杠胡）
	
	int  GetCtlCardSize() { return m_vecCTLCards.size(); };
	
	map<MJGPlayerNodeDef*, vector<AIDelayCardMsgDef> > m_mapAIOperationDelayMsg;		// AI延时操作消息

	int m_iIfOpenAI;							// 是否开启玩家AI
	int m_iIfNormalCanMatchNormal;				// 普通玩家是否可以匹配普通玩家
	int m_iNormalWaitAiTimeHigh;				// 普通玩家不可以匹配普通玩家，等待AI时间上限
	int m_iNormalWaitAiTimeLow;					// 普通玩家不可以匹配普通玩家，等待AI时间下限
	int m_iDoubtfulWaitAiTimeHigh;				// 可疑玩家不可以匹配普通玩家，等待AI时间上限
	int m_iDoubtfulWaitAiTimeLow;				// 可疑玩家不可以匹配普通玩家，等待AI时间下限

//控制相关																														
public:
	void SetPlayerIfControl(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers, HZXLControlCards &CtlCard);								//设置与AI对局时玩家相关配置
	bool JudgePlayerBeforeSit(MJGPlayerNodeDef *nodePlayers);																					//判断当前玩家是否需要匹配AI

	void SetControlCard(vector<HZXLControlCards>vecCTLCards);																						//从radius中取出牌库做校验并存放
	bool AnalysisControlCard(char cMainCards[], char cNeedCards[],char cAIChi, char cAIPeng ,char cAIGang);										//解析并校验牌库中的牌

	void CTLSetControlCard(MJGTableItemDef *tableItem,int iCTLCardsNumber);																//控制流程下的配牌控制
	void CTLExchangeCardAI(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers,ChangeCardsReqDef &msgChangeCardReq);						//控制流程下的AI的换牌控制
	void CTLConfirmQue(MJGTableItemDef* tableItem, MJGPlayerNodeDef *nodePlayers, ConfirmQueReqDef &msgConfirmQueReq);							//控制流程下的AI的定缺控制

	void CTLGetCardControl(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers);																//控制流程下的摸牌控制
	char CTLFindOptimalSendCard(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers,char &cIfTing);											//控制流程下的AI出牌控制
	void CTLJudgeHandleSpecialCards(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers, int &iSpecialFlag, char &cFirstCard, char cCards[]);//控制流程下的AI特殊操作控制(碰杠胡)

	bool CTLJudgeIfAIPlayerSuitPeng(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers);														//控制流程下AI的碰牌控制
	bool CTLJudgeIfAIPlayerSuitGang(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers,char &cCards);										//控制流程下AI的杠牌控制
	bool CTLJudgeIfAIPlayerSuitHu(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers);														//控制流程下AI的胡牌控制
	
private:
//AI相关
	bool JudegIfSinglePartCardlegal(vector<int>vecSinglePart);							//判断非将牌区间的单个区间组牌是否合法，是否能构成合理的顺/刻子
	void AutoBuQiHandCard(MJGTableItemDef *tableItem ,int iCurHandCardsNum,char cAllCard[],vector<char>&vecBuQiCards);	//补齐主AI-1和AI-2的手牌
	void AutoBuQiAssistAICard(int iAssistAIQue,char cAllCard[], vector<char>vecTmpAssistAIHandCards, vector<char>&vecBuQICard);								//补齐主AI-2的手牌
	void CreateRandSetCard(int iSetType,int iAssistAIQue,char cAllCard[], vector<char> &vecTmpCard);														//为主AI-2随机生成成组手牌
	bool CheckIfRandSetCardValue(char cAllCard[], vector<char> &vecTmpAssistCard);																			//检查AI-2随机生成的成组手牌是否可用
	
	int  CalWallCardNum(MJGTableItemDef &tableItem, char cCard);																							//计算牌墙中某张牌的剩余张数
	bool CheckRestAIHandCard(MJGTableItemDef *tableItem, char cCard,int &iCurIndex);																		//校验剩余AI手牌中是否存在主AI-1的可胡牌
	char MakeCardNumToChar(int iCardNum);																													//从牌值到char值转化
	
	// 获取所有AI玩家名字信息
	vector<char*> m_szFirstNameText;
	vector<char*> m_szLastNameText;
	vector<char *> m_szAreaText;
	void ReadAINickNameInfoConf();
	void ReadAIAreaInfoConf();
//控制相关
	void CalMainAIHuCard(MJGPlayerNodeDef *nodePlayers,vector<char>&vecHuCard);																		//控制流程下，需要自行计算出主AI-1的可胡牌
	bool JudgeIsMainAINeedCard(MJGTableItemDef *tableItem, char cCard);																		        //控制流程下判断当前牌是否是主AI-1的缺牌
	
	
	vector<char> CreateShunZiForDeputyAI(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers, char cCard ,vector<char>vecCards);				//控制流程下为副AI生成顺子
	char FindOptimalSingleCard(MJGPlayerNodeDef *nodePlayers);																						//针对副AI使用的最优出牌选择
	void CLTSendSpecialServerToPlayer(MJGTableItemDef *tableItem);																					//当AI出牌后，判断给普通玩家发送特殊操作消息，AI玩家走判断是否执行特殊操作
	//bool CTLJudgeIfAIPlayerSuitPeng(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers);														//控制流程下AI的碰牌控制
	//bool CTLJudgeIfAIPlayerSuitGang(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers);														//控制流程下AI的杠牌控制
	//bool CTLJudgeIfAIPlayerSuitHu(MJGTableItemDef *tableItem, MJGPlayerNodeDef *nodePlayers);														//控制流程下AI的胡牌控制

	bool CTLJudgeIsPureKeZi(MJGTableItemDef *tableItem,MJGPlayerNodeDef *nodePlayers, int iKeZiSection);											//控制流程下判断主AI-1当前主牌区间是否是纯刻子区间
	bool m_bIfOpen;																																	//AI控制是否开启
	int m_iCTLGameNum;																																//局数控制
	int m_iCTLGameRate[5][6];																														//多档控制对应概率
	vector <HZXLControlCards>m_vecCTLCards;
//辅助函数，以指定字符分割长字符串
	std::vector<std::string> Split(std::string str, std::string pattern);

};
#endif // !_HZXL_AI_CONTROL_H
