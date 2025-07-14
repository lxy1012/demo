/***
	红中麻将逻辑处理函数
***/
#ifndef __HZXLMJ_GAMELOGIC_H__
#define __HZXLMJ_GAMELOGIC_H__  

//系统头文件
#include <stdio.h>
#include <string>
#include <vector>
#include "gamefactorycomm.h"
#include "msgqueue.h"
#include "msg.h"
#include "hashtable.h"
#include "emutex.h"
#include "factorymessage.h"
#include "mjg_playernode.h"
#include "mjg_tableitem.h"
#include <map>
#include "mjg_define.h"
#include "hzxlmj_GameAlgorithm.h"

using namespace std;

enum RechargeProcess
{
	ASK_RECHARGE = 1,		// 玩家请求充值
	RECHARGE_SUCCESS,		// 玩家充值成功
	RECHARGE_FAIL,			// 玩家充值失败
	ABANDON_RECHARGE,		// 玩家在弹框后直接选择放弃充值
	BEGIN_RECHARGE,			// 广播玩家开始充值流程
	RECHARGE_OVERTIME,		// 玩家充值超时
};

enum RechargeState
{
	NO_RECHARGE = 0,		// 没有充值操作
	WAIT_RECHARE,			// 玩家处于等待充值状态
	END_RECHARGE,			// 玩家完成充值或者充值超时
};

enum RechargeType
{
	RECHARGE_FIRST_MONEY = 1,	// 福券充值
	RECHARGE_CASH,				// 现金充值
};

//特殊玩家标签，如果是该类玩家匹配AI
const int AI_MARK_BRUSH_PLAYER = 9;		// 刷子玩家

const int KICK_MAX_CNT = 1000;			// 单个服务器允许最多同时的在线人数

class HZXLMJ_AIControl;

class HZXLGameLogic :public GameLogic
{

public:
	HZXLGameLogic();
	~HZXLGameLogic();

	static HZXLGameLogic* shareHZXLGameLogic();

	virtual void AllUsersSendNoitce(void *pDate, int iType);//向游戏中所有用户发送公告通知
	
	
	// 玩家掉线的处理
	virtual void HandlePlayStateDisconnect(int iStatusCode, int iUserID, void *pMsgData);

	virtual void JudgeAgainLoginTime(int iTime);
	
	//可以直接复制
	void FreshRoomOnline();

	//取得具体桌信息指针.可以直接复制
	virtual FactoryTableItemDef* GetTableItem(int iRoomID, int iTableIndex);
	
	//移除玩家节点,直接复制
	virtual void RemoveTablePlayer(FactoryTableItemDef *pTableItem, int iTableNumExtra);
	
	//取得自由节点.游戏直接复制
	virtual void* GetFreeNode();
	
	//释放节点,游戏直接复制
	virtual void ReleaseNode(void* pNode);
	
	//处理客户端发来的消息
	virtual void HandleOtherGameNetMessage(int iMsgType, int iStatusCode, int iUserID, void *pMsgData);
	
	//清空桌信息		自动出牌
	virtual void AutoSendCards(int iUserID);
	
	// 24小时定时器
	virtual void DoOneDay();
	
	//部分游戏要重新获取一些相关用户信息
	virtual void CallBackRDUserInfo(FactoryPlayerNodeDef *nodePlayers);
	
	// 断线重连函数
	virtual void HandleAgainLoginReq(int iUserID, int iSocketIndex);

	virtual bool CheckIfPlayWithControlAI(FactoryPlayerNodeDef* nodePlayers);

	// 所有玩家已经准备好，开始游戏
	virtual void CallBackReadyReq(void *pPlayerNode, void *pTableItem, int iRoomID);
	
	virtual void ResetTableState(void *pTableItem);

	virtual bool JudgeMidwayLeave(int iUserID, void *pMsgData);

	virtual void DoOneHour();			// 一个小时的定时器

	virtual void CallBackFiveMinutes();		// 5分钟的定时器

	//判断玩家长时间没有操作剔除
	virtual void JudgePlayerKickOutTime(int iTime);
	virtual void HandleNormalEscapeReq(FactoryPlayerNodeDef *nodePlayers, void *pMsgData);
	virtual void CallbackHandleRedisServerMsg(char* pMsgData, int iMsgLen); //redis消息回调

	virtual void CallBackGameRefreshUserInfo(FactoryPlayerNodeDef *nodePlayers, int iErrorType);		// iType为0表示成功 非0值表示错误类型

	virtual void CallBackGetRoomInfo();//底层回调，在这里获取红中血流控制牌库以及控制相关的玩家的个人信息

private:

	void ResetTableInfo();
	
	void CallBackGameStart(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem);
	
	void CreateCard(MJGTableItemDef *tableItem);
	
	void SendCardToPlayer(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, bool bCP = false, bool bFirst = false);//
		
	int JudgeSpecialPeng(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem);
	
	int JudgeSpecialGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, int &iGameNum, char iGangCard[], char iGangType[]);//,bool bGuaDaFeng = false
	
	int JudgeSpecialTing(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, ResultTypeDef resultType, vector<char> vcCards, map<char, vector<char> > &mpTingCard, map<char, map<char, int> > &mpTingFan);

	int JudgeSpecialHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char *cFanResult, char cQiangGangCard = 0);
	
	void GameEndHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, bool bLiuJu, int iSpecialFlag);
	
	void CallBackChangeCard(MJGTableItemDef *tableItem);
	//处理玩家出牌
	void HandleSendCard(int iUserID, void *pMsgData);
	//处理玩家定缺
	void HandleConfirmQueReq(int iUserID, void *pMsgData);

	void SendSpecialServerToPlayer(MJGTableItemDef *tableItem);
		
	//处理玩家 吃碰杠听胡
	void HandleSpecialCard(int iUserID, void *pMsgData);
	
	void HandlePeng(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, void * pMsgData);
	
	void HandleGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, void * pMsgData);
	
	void HandleQi(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, void * pMsgData);

	void HandleHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, void * pMsgData);

	void HandleChangeCardReq(int iUserID, void *pMsgData);
	void HandleRechargeReqMsg(int iUserID, void *pMsgData);			// 玩家游戏内破产请求充值
	void HandleRechargeResultMsg(int iUserID, void *pMsgData);		// 玩家游戏内破产充值结果
	void HandleRechargeRecordMsg(int iUserID, void *pMsgData);		// 玩家游戏内破产充值参数统计
	void SendRechargeRecordToRedis(int iRecordId, int iRecordNum);	// 发送游戏内破产充值统计数据到redis

	void SendExtraAgainLoginToClient(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef* pTableItem);							// 发送断线重连中的吃碰杠信息到客户端

	virtual void CallBackDisconnect(FactoryPlayerNodeDef *nodePlayers);//部分类游戏在离开游戏前需要用到清除玩家节点的信息

	virtual void CallBackSitSuccedDownRes(FactoryTableItemDef *pTableItem, FactoryPlayerNodeDef *nodePlayers);//玩家坐下后回应每个游戏需要的信息

	virtual void CallbackHandleRadiusServerMsg(char* pMsgData, int iMsgLen);

	MJGPlayerNodeDef *m_pNodeFree;                          //空闲节点

	MJGPlayerNodeDef *m_pNodePlayer;                        //使用中的节点

	MJGTableItemDef m_tbItem[MAX_ROOM_NUM][MAX_TABLE_NUM];  //桌

	hzxlmj_GameAlgorithm *m_pGameAlgorithm;

private:
	void SelectChangeCards(MJGPlayerNodeDef *nodePlayers, char *cResultChangeCard);
	int CalculateTotalMulti(MJGPlayerNodeDef *nodePlayers);								// 计算胡的总倍数
	int CalculateMaxFanType(MJGPlayerNodeDef *nodePlayers);								// 计算胡牌玩家胡的最大番型
	int GetNormalPlayerNum(MJGTableItemDef *pTableItem);									// 计算本桌中未破产玩家数量
	int CalCulateQueType(MJGPlayerNodeDef *nodePlayers);
	bool TransResultScore(int iSeat, int viResultScore[], int viTransScore[]);			// 将玩家的结算输赢分转换到自己的视角下
	void StatisticAllHuFan(MJGTableItemDef *tableItem,MJGPlayerNodeDef *nodePlayers);								// 统计所有玩家的胡牌番数情况
	void SendRoomInfoToClient(MJGPlayerNodeDef *nodePlayers, char cIfLoginAgain);		// 发送房间信息到客户端
	void SendSpecialInfoToClient(MJGTableItemDef *tableItem, int viSpecialSeatFlag[4]);								// 某个玩家有操作的时候，将消息告知其他玩家，用于倒计时重置与显示


	// 函数拆分
	void HandlePengGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, SpecialCardsReqDef *pReq);		// 处理碰杠
	void HandleAnGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, SpecialCardsReqDef *pReq);			// 处理暗杠
	void HandleMingGang(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, SpecialCardsReqDef *pReq);		// 处理明杠

	void HandleZiMoHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char cRealPoChan[4]);			// 处理自摸胡牌
	void CalculateHuTingInfo(MJGTableItemDef *tableItem, char cTingHuCard[4][28], int iTingHuFan[4][28], char cNeedUpdateTing[4]);		// 玩家胡牌时，如果没有听牌信息，计算胡牌玩家的听牌信息
	void HandleZiMoHuLiuShui(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, SpecialHuNoticeDef pMsgHu, int iPos);

	void HandleDianPaoHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char cRealPoChan[4]);
	void HandleHJZYHu(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, long long viHJZYScore[4], int &iHJZYFan);					// 处理点炮胡中的呼叫转移

	void OperateAfterHuState(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem);							// 玩家胡牌后的处理
	void HandleGameEndHuLiuJu(MJGTableItemDef *tableItem, GameResultServerDef &pMsgResult, char *cHuaZhu, LiuJuSpecialInfoDef &liujuSpecialInfoDef);			// 游戏结束，处理流局相关
																													
	bool HandleLiuJuHuaZhu(MJGTableItemDef *tableItem, char *cHuaZhu, const int iHuaZhuNum, char vcHuaZhuPoChan[4][4], long long viHuaZhuMoney[4][4][4], char vcHuaZhuFan[4][4][4]);	// 处理流局中的花猪计算
	bool HandleLiuJuChaDaJiao(MJGTableItemDef *tableItem, char *cHuaZhu, const int iHuaZhuNum, char vcChaDaJiaoPoChan[4][4], long long viChaDaJiaoMoney[4][4][4], int viChaDaJiaoFan[4][4][4]);
public:
	// 混服
	virtual int GetSimulateMoney(int iRoomIndex, FactoryPlayerNodeDef *otherNodePlayers); // 获取某个玩家在指定房间显示的钱
	//AI相关
	virtual void OneSecTime();//1秒的定时器
	void OnTimeAIPlayerMsg();																											// 每秒定时检查ai玩家的消息
	void OnTimeMatchAiPlayer();																											//每秒定时检查匹配AI玩家

	virtual bool JudgeIsPlayWithAI(FactoryPlayerNodeDef *nodePlayers);																	//重写底层，判断当前玩家是否与AI玩家同桌
	void InitERMJAIInfo(MJGTableItemDef *pTableItem, const int iPlayerSeat);															//初始化AI玩家相关信息
	
	void FindEmptyTableWithAI(FactoryPlayerNodeDef* nodePlayers);																		//为需要匹配AI的玩家匹配AI
	void ReleaseAIPlayerNode(MJGTableItemDef *tableItem, MJGPlayerNodeDef *pPlayers);													// 删除AI玩家信息

	void HandleChangeCardsReqsAI(MJGPlayerNodeDef *pPlayers, void *pMsgData);															//服务端处理客户端AI玩家的换牌请求	
	void HandleConfirmQueReqAI(MJGPlayerNodeDef *pPlayers, void *pMsgData);																//服务端处理客户端AI玩家的定缺消息
	void HandleSendCardsReqAI(MJGPlayerNodeDef *pPlayers, void *pMsgData);																//服务端处理客户端AI玩家请求出牌消息
	void HandleSpecialCardsReqAI(MJGPlayerNodeDef *pPlayers, void *pMsgData);															//服务端处理客户端AI玩家特殊操作消息（碰杠胡）

	HZXLMJ_AIControl *m_pHzxlAICTL;														//红中血流麻将AI相关
public:
	//从Params表获取数据
	//Params表
	map<string, string> m_mapParams;
	void SendRadiusGetParamsValueReq();
	void HandleBKGetParamsValueRes(void* msgData);
	string GetBKParamsValueByKey(string key);
	void CallBackGameNetDelay(int iMsgType, int iStatusCode, int iUserID, void *pMsgData, int iDelayUs);
	//radius相关
	void GetControlCardFromRadius();                                            //请求获取牌库
	void HandleGetControlCardMsg(void *pMsgData);                               //获取牌库回应处理

private:
//	MJGPlayerNodeDef *m_pTmpNodeFree;                          //空闲的临时节点
//	MJGPlayerNodeDef *m_pTmpNodePlayer;                        //使用中的临时节点
//
	void* GetTmpFreeNode();										//获取空闲的临时节点
	void ReleaseTmpNode(void* pNode);							//释放使用中的临时节点
	vector<MJGPlayerNodeDef*> m_vTmpPlayerNode;				// 破产等使用的临时节点

// 自动发送表情相关
private:
	void AutoSendMagicExpress(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char cTargetExtra);
	void SendMagicExpress(MJGTableItemDef *tableItem, int iUserID, int iToUserID, int iPropId);
	void OnTimeCheckOverTime();		// 每秒定时检查是否有玩家破产支付超时
	void ExchargeEnd(MJGPlayerNodeDef *nodePlayers, MJGTableItemDef *tableItem, char cRechargeFlag, bool bOverTime = false);
	bool m_bOpenRecharge;			//是否开启破产充值功能
};



#endif    //__HZXLMJ_GAMELOGIC_H__













