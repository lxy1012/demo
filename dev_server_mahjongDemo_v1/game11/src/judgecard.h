#ifndef _JUDGE_CARD_H_
#define _JUDGE_CARD_H_
#include "mjg_define.h"

//#define SHI_BA_LUO_HAN			1	// 128番，十八罗汉 由4个杠和一对将组成的胡牌
//#define QING_YAO_JIU			2	// 88番，清幺九 全由序数牌1和9组成的胡牌，不计碰碰和、同刻、无字  ,混幺九 *
//#define LIAN_QI_DUI				3	// 88番，连七对 由一种花色序数牌组成序数相连的7个对子的和牌。不计清一色、不求人、单钓
//#define YI_SE_SHUANG_LONG_HUI   4	// 64番，一色双龙会 一种花色的两个老少副，5为将牌。不计平胡、七对、清一色、老少副、一般高、喜相逢、缺一门、无字
//#define TIAN_HU					5	// 48番，天胡,庄家在发完牌就胡牌。如果庄家在发完牌后有暗杠，那么不算天胡。 不计其他番型
//#define JIU_LIAN_BAO_DENG		6	// 48番，九莲宝灯 由一种花色序数牌子按1112345678999组成的特定牌型，见同花色任何1张序数牌即成和牌。不计清一色，幺九刻，不求人 *
//#define LV_YI_SE				7	// 32番，绿一色 由23468条及发字中的任何牌组成的顺子、刻子（杠）、将牌的胡牌 *
//#define QUAN_DAI_YAO			8	// 32番，全带幺：玩家手牌中，全部是用1的连牌或者9的连牌组成的牌
//#define QUAN_DAI_WU				9	// 32番，全带五 牌型中所有的顺子，刻子，将  都含有点数5
//#define DI_HU					10	// 24番，地胡，非庄家在发完手牌后就听牌，第一圈摸牌后就自摸胡牌，闲家不能  碰，杠（包括暗杠），玩家放炮不算，可以点过，等自摸。（不计其他牌型，地胡不会计算加倍）
//#define QUAN_DA					11  // 24番，全大，仅由序数牌789组成的胡牌
//#define QUAN_ZHONG				12  // 24番，全中，仅由序数牌456组成的胡牌
//#define QUAN_XIAO				13  // 24番，全小，仅由序数牌123组成的胡牌
//#define SHOU_ZHONG_BAO_YI		14  // 24番，守中抱一：牌型中只剩1张红中单吊红中胡牌 *
//#define SI_AN_KE				15  // 16番，四暗刻，牌型中有4个暗杠/暗刻
//#define SI_JIE_GAO				16  // 16番，四节高：胡牌时，有4个同种花色且点数连续的刻子/杠
//#define LONG_QI_DUI				17	// 16番，龙七对：玩家手牌为暗七对牌型，没有碰过或者杠过，并且有四张牌是一样的，叫龙七对。（不计七对、1根）
//#define	SHI_ER_JIN_CHAI			18	// 12番，胡牌中有3个杠
//#define JIN_GOU_DIAO			19  // 8番，金钩钓 所有其余牌均已碰/杠，只留有一张手牌的大对子单钓。（不计对对胡）
//#define DA_YU_WU				20	// 8番，大于五：由序数牌6-9的顺子、刻子（杠）、将牌组成的胡牌
//#define XIAO_YU_WU				21	// 8番，小于五：由序数牌1-4的顺子、刻子（杠）、将牌组成的胡牌
//#define SAN_JIE_GAO				22  // 8番，三节高：胡牌时，有3个同种花色且点数连续的刻子/杠
//#define QUAN_SHUANG_KE			23	// 8番，全双刻：牌型中只有刻/杠和将，且刻子点数都是偶数
//#define BAI_WAN_DAN				24	// 8番，百万石：牌型中万字牌的点数≥100
//#define QI_DUI					25	// 7番，七对：玩家的手牌全部是两张一对的，没有碰过和杠过
//#define QING_YI_SE				26	// 6番，清一色：玩家胡牌的手牌全部都是一门花色
//#define QING_LONG				27	// 6番，清龙：胡牌时，有一种花1-9相连接的序数牌
//#define SAN_AN_KE				28	// 6番，三暗刻：牌型中有3个暗杠/暗刻
//#define GANG_SHANG_KAI_HUA		29	// 4番，杠上开花：杠后自模胡牌（杠了之后补牌而胡）
//#define TUI_BU_DAO				30	// 4番，推不倒：仅由1234589筒和245689条组成的胡牌
//#define BU_QIU_REN				31	// 4番，不求人：胡牌时，没有碰牌（包括明杠），自摸胡牌牌
//#define SU_HU					32	// 2番，素胡：胡牌时，没有红中癞子参与的牌型
//#define PENG_PENG_HU			33	// 2番，碰碰胡：由4种刻牌/杠外加1对牌组成的胡牌
//#define BIAN_ZHANG				34	// 2番，边张：12胡3,89胡7的牌型
//#define KAN_ZHANG				35	// 2番，坎张：胡牌时，知乎顺子中间的那张牌
//#define DANDIAO					36	// 2番，单钓：胡牌时，是属于钓将胡牌
//#define SHUANG_TONG_KE			37	// 2番，双同刻：牌型中包含2个点数相同的刻子
//#define SHUANG_AN_KE			38	// 2番，双暗刻：牌型中有2个暗杠/暗刻
//#define LAO_SHAO_PEI			39	// 2番，老少配：牌型中包含同花色的123+789这两组顺子
//#define DUAN_YAO_JIU			40	// 2番，断幺九：不包含任何的1、9的牌
//#define PING_HU					41	// 1番，平胡：普通的四种 刻牌或顺子，外加一对 将牌  组成的胡牌
//#define  MJ_FAN_TYPE_CNT		42	// 所有番型数量

#define MJ_ONLY_GEN_FAN		199		// 麻将仅胡根番
#define RECURSION_MAX_CNT 10000		// 某一副牌型中，允许递归调用的最大次数

// 红中血流麻将番型定义
enum HZMJFanXing
{
	SHI_BA_LUO_HAN = 1,					// 128番，十八罗汉 由4个杠和一对将组成的胡牌（不计金钩钓、单钓、十二金钗）
	QING_YAO_JIU,						// 88番，清幺九 全由序数牌1和9组成的胡牌，不计碰碰和、同刻
	LIAN_QI_DUI,						// 88番，连七对 由同一种花色序数牌组成序数相连的7个对子的和牌。不计清一色、不求人、单钓、门清、七对
	YI_SE_SHUANG_LONG_HUI,				// 64番，一色双龙会 一种花色的两个老少副，5为将牌。不计平胡、七对、清一色、老少配
	MJ_TIAN_HU,							// 48番，天胡,庄家在发完牌就胡牌。庄家不可以 碰，杠（包括暗杠）。（不计其他牌型，天胡不会计算加倍）
	JIU_LIAN_BAO_DENG,					// 48番，九莲宝灯 由一种花色序数牌子按1112345678999组成的特定牌型，见同花色任何1张序数牌即成和牌。不计清一色、幺九刻、不求人、门清、老少配
	LV_YI_SE,							// 32番，绿一色 由23468条中的任何牌组成的顺子、刻子（杠）、将牌的胡牌
	QUAN_DAI_YAO,						// 32番，全带幺：玩家手牌中，全部是用1的连牌或者9的连牌组成的牌
	QUAN_DAI_WU,						// 32番，全带五 牌型中所有的顺子，刻子，将  都含有点数5
	MJ_DI_HU,							// 24番，地胡，非庄家在发完手牌后就听牌，第一圈摸牌后就自摸胡牌，闲家不能  碰，杠（包括暗杠），玩家放炮不算，可以点过，等自摸。（不计其他牌型，地胡不会计算加倍）
	QUAN_DA,							// 24番，全大，仅由序数牌789组成的胡牌，不计大于五
	QUAN_ZHONG,							// 24番，全中，仅由序数牌456组成的胡牌
	QUAN_XIAO,							// 24番，全小，仅由序数牌123组成的胡牌，不计小于五
	SHOU_ZHONG_BAO_YI,					// 24番，守中抱一：牌型中只剩1张红中单吊红中胡牌
	SI_AN_KE,							// 16番，四暗刻，牌型中有4个暗杠/暗刻，不计门清、碰碰胡、三暗刻、双暗刻、不求人、单钓
	SI_JIE_GAO,							// 16番，四节高：胡牌时，有4个同种花色且点数连续的刻子/杠(暗/明)，不计三节高、碰碰胡	222333444555xx
	LONG_QI_DUI,						// 16番，龙七对：玩家手牌为暗七对牌型，没有碰过或者杠过，并且有四张牌是一样的，叫龙七对。（不计七对、1根）
	SHI_ER_JIN_CHAI,					// 12番，十二金钗: 胡牌中有3个杠，不计金钩钓、单钓
	MJ_JIN_GOU_DIAO,					// 8番，金钩钓 所有其余牌均已碰/杠，只留有一张手牌的大对子单钓。（不计碰碰胡，单钓）
	DA_YU_WU,							// 8番，大于五：由序数牌6-9的顺子、刻子（杠）、将牌组成的胡牌
	XIAO_YU_WU,							// 8番，小于五：由序数牌1-4的顺子、刻子（杠）、将牌组成的胡牌
	SAN_JIE_GAO,						// 8番，三节高：胡牌时，有3个同种花色且点数连续的刻子/杠, 不算三暗刻
	QUAN_SHUANG_KE,						// 8番，全双刻：牌型中只有刻/杠和将，且刻子点数都是偶数，不计碰碰胡
	BAI_WAN_DAN,						// 8番，百万石：牌型中万字牌的点数≥100，必须为刻子+顺子+对子组成，七对等特殊牌型不算
	QI_DUI,								// 7番，七对：玩家的手牌全部是两张一对的，没有碰过和杠过, 不计不求人、单钓、门清
	MJ_QING_YI_SE,						// 6番，清一色：玩家胡牌的手牌全部都是一门花色
	QING_LONG,							// 6番，清龙：胡牌时，有一种花1-9相连接的序数牌, 不计老少配
	SAN_AN_KE,							// 6番，三暗刻：牌型中有3个暗杠/暗刻, 不计双暗刻
	TUI_BU_DAO,							// 4番，推不倒：仅由1234589筒和245689条组成的胡牌
	BU_QIU_REN,							// 4番，不求人：胡牌时，没有碰牌（包括明杠），自摸胡牌, 不计自摸、门前清
	SU_HU,								// 2番，素胡：胡牌时，没有红中癞子参与的牌型
	MJ_PENG_PENG_HU,					// 2番，碰碰胡：由4种刻牌/杠外加1对牌组成的胡牌
	BIAN_ZHANG,							// 2番，边张：12胡3,89胡7的牌型
	KAN_ZHANG,							// 2番，坎张：胡牌时，只胡顺子中间的那张牌
	DAN_DIAO_JIANG,						// 2番，单钓将：胡牌时，是属于钓将胡牌
	SHUANG_TONG_KE,						// 2番，双同刻：牌型中包含2个点数相同的刻子
	SHUANG_AN_KE,						// 2番，双暗刻：牌型中有2个暗杠/暗刻
	LAO_SHAO_PEI,						// 2番，老少配：牌型中包含同花色的123+789这两组顺子
	DUAN_YAO_JIU,						// 2番，断幺九：不包含任何的1、9的牌

	ZI_MO,								// 自摸， 2番
	MEN_QING,							// 2番，门清：没有碰，补杠，明杠的胡牌，胡他人打出的牌。（暗杠算门清）
	MJ_GEN,								// 2番，根：胡牌玩家所有牌中有4张一样的牌即算1根
	MJ_GANG_SHANG_PAO,					// 2番，杠上炮：杠牌后，打出的牌造成其他玩家点炮胡牌
	MJ_QIANG_GANG_HU,					// 2番，抢杠胡：玩家补杠操作时，其他玩家刚好胡这张牌，此时为抢杠胡，且本次补杠无效
	MJ_GANG_SHANG_KAI_HUA,				// 2番，杠上开花：杠后自摸胡牌（杠了之后补牌而胡）
	HAI_DI_LAO_YUE,						// 2番，海底捞月：摸到最后1张牌胡牌
	MIAO_SHOU_HUI_CHUN,					// 2番，妙手回春：打出牌堆中最后一张牌点炮

	MJ_PING_HU,							// 1番，平胡：普通的四种 刻牌或顺子，外加一对 将牌  组成的胡牌
	MJ_FAN_TYPE_CNT,					// 红中血流麻将所有番型数量
};

class JudgeResult
{
public:
	JudgeResult(void) {};
	~JudgeResult(void) {};

	static int MakeCharToCardNum(char cCard);
	static int JudgeHZXLMJHu(int iAllCard[5][10], ResultTypeDef &resultType, int iQueType, char *cFanResult);
	static int JudgeSZBYHu(int iAllCard[5][10], ResultTypeDef &resultType, int iQueType, char *cFanResult);		// 判断守中抱一胡牌
	
private:
	static bool JudgeNormalHu(int allPai[3][10], ResultTypeDef &resultType);
	static bool Analyze(int aKindPai[], bool ziPai, int iType, ResultTypeDef &resultType);
	static bool Analyze2(int aKindPai[], bool ziPai, int iType, ResultTypeDef & resultType);

	static bool JudgeJiang(int iLastCard, int allPai[3][10], ResultTypeDef &resultType);

// 算番相关的函数
private:
	static int JudgeFan(ResultTypeDef resultType, char *cResult);
	static int JudgeMJFan(int iAllCards[3][10], ResultTypeDef resultType, char *cResult);
	static void JudgeQiDuiFan(int allPai[3][10], ResultTypeDef &resultType, char *cResult/*, int iHZCardNum = 0*/);
	static int JudgeSpecialFan(ResultTypeDef resultType, char *cResult);		// 一些特殊的番型计算
	static void JudgeFanLevelOne(ResultTypeDef resultType, char *cResult);		// 128,88,64,48,32番
	static void JudgeFanLevelTwo(ResultTypeDef resultType, char *cResult);		// 24,16番
	static void JudgeFanLevelThree(ResultTypeDef resultType, char *cResult);	// 12,8番
	static void JudgeFanLevelFour(ResultTypeDef resultType, char *cResult);		// 7，6，4番
	static void JudgeFanLevelFive(ResultTypeDef resultType, char *cResult);		// 2番
	static void JudgeGenFan(ResultTypeDef resultType, char *cResult);		// 根

	static void JudgeSpecialGenFan(int iAllCards[5][10], ResultTypeDef &resultType, char *cFanResult);		// 根

	static int CalculateTotalFan(char *cResult);			// 统计所有的番数
	static int CalculateTotalCardCnt(int iAllCard[3][10], int iQueType);		// 计算手牌的张数

	static int m_iRecursionCnt;			// 统计胡牌函数递归调用的次数

// 判断胡牌相关的函数
private:
	static int JudgeNormalHongZhongHu(int iAllCard[3][10], ResultTypeDef &resultType, int iHZCardNum, int iQueType, char *cFanResult);		// 带红中的正常格式胡牌(3N+2的格式胡牌)
	static int JudgeNormalHZhuErgodic(int iAllCard[3][10], ResultTypeDef &resultType, int iHZCardNum, int iQueType, char *cFanResult);	// 遍历的方式判断带红中的胡牌
	static int JudgeSpecialHu(int iAllCard[3][10], ResultTypeDef &resultType, int iHZCardNum, int iQueType, char *cFanResult);			// 非正常格式的胡牌(不符合3N+2的胡牌格式)
	static int JudgeHZCardHu(int iTempCard[3][10], ResultTypeDef TempResultType, int iHZCardNum, const int i, const int j, char *cFanResult);			// 判断带红中的胡牌
	static int JudgeNormalHuWithoutJiang(int allPai[3][10], ResultTypeDef &resultType, int iValue, int iType, char *cFanResult);

	static bool AnalyzeHongZhongCard(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int index, vector<int>& viTransHZCards);		// 先刻后顺
	static bool AnalyzeHongZhongCard2(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int iIndex, vector<int>& viTransHZCards);		// 先顺后刻
	static bool AnalyzeHongZhongCardKe(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int index, vector<int>& viTransHZCards);
	static bool AnalyzeHongZhongCardShun(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int index, vector<int>& viTransHZCards);	// 两张红中，选择较大的顺子
	static bool AnalyzeHongZhongCardShun2(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int iIndex, vector<int>& viTransHZCards);	// 两张红中，选择较小的顺子
};



#endif //_JUDGE_CARD_H_



