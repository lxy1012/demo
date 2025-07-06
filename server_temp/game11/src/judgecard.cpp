#include "judgecard.h"


#include "string.h"


#include "stdio.h"


#include <math.h>

#include "log.h"

int JudgeResult::m_iRecursionCnt = 0;

int JudgeResult::MakeCharToCardNum(char cCard)
{
	int iValueTemp;                 //1-9
	int	iTypeTemp;                  //0-4
	iValueTemp = cCard & 0xf;        //取低4位的牌值R
	iTypeTemp = cCard >> 4;          //取高4位的牌型
	int rt = iTypeTemp * 9 + iValueTemp - 1;//因为从1开始算所以-1
	return rt;
}

int JudgeResult::CalculateTotalFan(char *cResult)
{
	int iTotalFan = 0;
	bool bCanHu = false;
	for (int i = 0; i < MJ_FAN_TYPE_CNT; i++)
	{
		if (cResult[i] > 0)
		{
			bCanHu = true;
			break;
		}
	}

	if (bCanHu)
	{
		iTotalFan = 1;
		for (int i = 0; i < MJ_FAN_TYPE_CNT; i++)
		{
			if (cResult[i] > 0)
			{
				//_log(_DEBUG, "JudgeResult", "CalculateTotalFan_Log() cResult[%d] = %d", i, cResult[i]);
				if (i == SHI_BA_LUO_HAN && cResult[i] == 127)
					iTotalFan *= 128;
				else
					iTotalFan *= cResult[i];

			}
		}
	}

	if (bCanHu && (iTotalFan > MAX_WIN_FAN || iTotalFan < 0))
		iTotalFan = MAX_WIN_FAN;

	//if(iTotalFan != 0)
	//	_log(_DEBUG, "JudgeResult", "CalculateTotalFan_Log() iTotalFan = %d", iTotalFan);
	return iTotalFan;
}

// 计算手牌的张数
int JudgeResult::CalculateTotalCardCnt(int iAllCard[3][10], int iQueType)
{
	int iTotalCardCnt = 0;
	for (int i = 0; i < 3; i++)
	{
		if (i == iQueType)
			continue;

		iTotalCardCnt += iAllCard[i][0];
	}

	return iTotalCardCnt;
}

//分解成“刻”“顺”组合    
/***
@param aKindPai[]: 手牌中某一花色(万条饼)的所有牌；
@param ziPai：是否是字牌，type == 4，即中发白，红中麻将里只有红中。这里指是否是红中
@param iType：牌的花色，0-5，标识万条饼东南西北等；
@return resultType：返回解析的结果
***/
bool JudgeResult::Analyze(int aKindPai[], bool ziPai, int iType, ResultTypeDef &resultType)//手上的牌   红中(血战没必要)  将的类型  结果结构体
{
	m_iRecursionCnt = m_iRecursionCnt + 1;
	if (m_iRecursionCnt > RECURSION_MAX_CNT)
	{
		_log(_ERROR, "JudgeResult", "Analyze_ m_iReCnt[%d] ziPai[%d]iType[%d]", m_iRecursionCnt, ziPai, iType);
		return false;
	}
	int j;
	if (aKindPai[0] == 0)
	{
		return true;
	}
	//寻找第一张牌
	for (j = 1; j<10; j++)
	{
		if (aKindPai[j] != 0)//aKindPai[j]哪张牌的数量
		{
			break;
		}
	}
	bool result;

	if (aKindPai[j] >= 3)//作为刻牌
	{
		//除去这3张刻牌
		aKindPai[j] -= 3;
		aKindPai[0] -= 3;

		resultType.pengType[resultType.iPengNum].iType = iType;      //把刻牌作为碰放入结果结构体
		resultType.pengType[resultType.iPengNum].iValue = j;
		resultType.iPengNum++;


		result = Analyze(aKindPai, ziPai, iType, resultType);
		//还原这3张刻牌
		aKindPai[j] += 3;
		aKindPai[0] += 3;
		return result;
	}
	//作为顺牌
	if ((!ziPai) && (j<8)
		&& (aKindPai[j + 1]>0)
		&& (aKindPai[j + 2]>0))
	{
		//除去这3张顺牌
		aKindPai[j]--;
		aKindPai[j + 1]--;
		aKindPai[j + 2]--;
		aKindPai[0] -= 3;

		resultType.chiType[resultType.iChiNum].iType = iType;
		resultType.chiType[resultType.iChiNum].iFirstValue = j;
		resultType.iChiNum++;

		result = Analyze(aKindPai, ziPai, iType, resultType);
		//还原这3张顺牌
		aKindPai[j]++;
		aKindPai[j + 1]++;
		aKindPai[j + 2]++;
		aKindPai[0] += 3;
		return result;
	}
	return false;
}

// 分解成顺刻组合
bool JudgeResult::Analyze2(int aKindPai[], bool ziPai, int iType, ResultTypeDef &resultType)//手上的牌   红中(血战没必要)  将的类型  结果结构体
{
	m_iRecursionCnt = m_iRecursionCnt + 1;
	if (m_iRecursionCnt > RECURSION_MAX_CNT)
	{
		_log(_ERROR, "JudgeResult", "Analyze2_ m_iReCnt[%d] ziPai[%d]iType[%d]", m_iRecursionCnt, ziPai, iType);
		return false;
	}

	int j;
	if (aKindPai[0] == 0)
	{
		return true;
	}
	//寻找第一张牌
	for (j = 1; j<10; j++)
	{
		if (aKindPai[j] != 0)//aKindPai[j]哪张牌的数量
		{
			break;
		}
	}
	bool result;

	//作为顺牌
	if ((!ziPai) && (j<8)
		&& (aKindPai[j + 1]>0)
		&& (aKindPai[j + 2]>0))
	{
		//除去这3张顺牌
		aKindPai[j]--;
		aKindPai[j + 1]--;
		aKindPai[j + 2]--;
		aKindPai[0] -= 3;

		resultType.chiType[resultType.iChiNum].iType = iType;
		resultType.chiType[resultType.iChiNum].iFirstValue = j;
		resultType.iChiNum++;

		result = Analyze2(aKindPai, ziPai, iType, resultType);
		//还原这3张顺牌
		aKindPai[j]++;
		aKindPai[j + 1]++;
		aKindPai[j + 2]++;
		aKindPai[0] += 3;
		return result;
	}
	if (aKindPai[j] >= 3)//作为刻牌
	{
		//除去这3张刻牌
		aKindPai[j] -= 3;
		aKindPai[0] -= 3;

		resultType.pengType[resultType.iPengNum].iType = iType;      //把刻牌作为碰放入结果结构体
		resultType.pengType[resultType.iPengNum].iValue = j;
		resultType.iPengNum++;


		result = Analyze2(aKindPai, ziPai, iType, resultType);
		//还原这3张刻牌
		aKindPai[j] += 3;
		aKindPai[0] += 3;
		return result;
	}

	return false;
}

bool JudgeResult::JudgeJiang(int iLastCard, int allPai[3][10], ResultTypeDef &resultType)
{
	int i, j, m;
	int jiangPos;//“将”的位置
	int yuShu;//余数
	bool jiangExisted = false;
	//是否满足3，3，3，3，2模型
	for (i = 0; i<3; i++)
	{
		yuShu = (int)fmod(allPai[i][0], 3);
		if (yuShu == 1)
		{
			return false;
		}
		if (yuShu == 2)
		{
			if (jiangExisted)
			{
				return false;
			}
			jiangPos = i;
			jiangExisted = true;
		}
	}

	if (jiangExisted == false)
		return false;

	//将手中牌去掉当前牌看是不是可以胡
	if (allPai[iLastCard >> 4][iLastCard & 0x0f] < 2)
	{
		return false;
	}
	allPai[iLastCard >> 4][0] -= 2;
	allPai[iLastCard >> 4][iLastCard & 0x0f] -= 2;


	for (i = 0; i<3; i++)
	{
		if (!Analyze(allPai[i], i == 4, i, resultType))
		{
			return false;
		}

	}
	return true;

}

//  一对将牌  33332，判断要胡的手牌是否符合基本胡牌牌型
bool JudgeResult::JudgeNormalHu(int allPai[3][10], ResultTypeDef &resultType)//手上的牌   结果结构体 两张或三张相同的牌的值  左移右移确定类型和值
{
	int i, j, m;
	int jiangPos;//“将”的位置
	int yuShu;//余数
	bool jiangExisted = false;
	//是否满足3，3，3，3，2模型
	for (i = 0; i < 3; i++)
	{
		yuShu = (int)fmod(allPai[i][0], 3);
		if (yuShu == 1)
		{
			return false;
		}
		if (yuShu == 2)
		{
			if (jiangExisted)
			{
				return false;
			}
			jiangPos = i;
			jiangExisted = true;
		}
	}

	if (jiangExisted == false)
		return false;

	for (i = 0; i < 3; i++)
	{
		if (i != jiangPos)
		{
			if (!Analyze(allPai[i], i == 3, i, resultType))
			{
				return false;
			}
		}
	}

	//该类牌中要包含将，因为要对将进行轮询，效率较低，放在最后
	bool success = false;//指示除掉“将”后能否通过
	for (j = 1; j < 10; j++)//对列进行操作，用j表示
	{
		if (allPai[jiangPos][j] >= 2)
		{
			//临时定义变量保存，如果判断成功则累积进resultType,不然无视
			ResultTypeDef tempResultType;
			memset(&tempResultType, 0, sizeof(ResultTypeDef));
			//除去这2张将牌
			allPai[jiangPos][j] -= 2;
			allPai[jiangPos][0] -= 2;
			if (Analyze(allPai[jiangPos], jiangPos == 3, jiangPos, tempResultType))
			{
				success = true;

				for (m = 0; m<tempResultType.iChiNum; m++)
				{
					memcpy(&(resultType.chiType[resultType.iChiNum]), &(tempResultType.chiType[m]), sizeof(ChiTypeDef));
					resultType.iChiNum++;
				}
				for (m = 0; m<tempResultType.iPengNum; m++)
				{
					memcpy(&(resultType.pengType[resultType.iPengNum]), &(tempResultType.pengType[m]), sizeof(PengTypeDef));
					resultType.iPengNum++;
				}
			}
			//还原这2张将牌
			allPai[jiangPos][j] += 2;
			allPai[jiangPos][0] += 2;
			if (success)
			{
				resultType.jiangType.iType = jiangPos;
				resultType.jiangType.iValue = j;
				break;
			}
		}
	}
	return success;
}

// 判断胡牌的番数
int JudgeResult::JudgeFan(ResultTypeDef resultType, char *cResult)
{
	if (resultType.iChiNum + resultType.iPengNum != 4 || resultType.jiangType.iValue == 0 || resultType.lastCard.iValue == 0)    //|| resultType.iQuanFengValue == 0 
	{
		//_log(_ERROR, "JudgeResult", "JudgeFan_Log() resultType.iChiNum[%d], resultType.iPengNum = [%d]", resultType.iChiNum , resultType.iPengNum);
		//_log(_ERROR, "JudgeResult", "JudgeFan_Log() resultType.jiangType.iValue[%d], resultType.lastCard.iValue = [%d]", resultType.jiangType.iValue, resultType.lastCard.iValue);

		return -1;
	}

	//_log(_DEBUG, "JudgeResult", "JudgeFan_Log() resultType.iChiNum[%d], resultType.iPengNum = [%d]", resultType.iChiNum, resultType.iPengNum);

	//for (int i = 0; i < (resultType.iChiNum); i++)
	//{
	//	_log(_DEBUG, "JudgeResult", "JudgeFan_Log() resultType.chiType[%d].iType = %d, resultType.chiType.iFirstValue = %d", i, resultType.chiType[i].iType, resultType.chiType[i].iFirstValue);
	//}

	//for (int i = 0; i < (resultType.iPengNum); i++)
	//{
	//	_log(_DEBUG, "JudgeResult", "JudgeFan_Log() resultType.pengType[%d].iType = %d, resultType.pengType.iValue = %d", i, resultType.pengType[i].iType, resultType.pengType[i].iValue);
	//}

	//首先对resultType里的chiType和pengType根据iValue进行冒泡排序,从小到大
	ChiTypeDef tempChiType;
	for (int i = 0; i < (resultType.iChiNum - 1); i++)
	{
		for (int j = i + 1; j<resultType.iChiNum; j++)
		{
			if (resultType.chiType[i].iFirstValue > resultType.chiType[j].iFirstValue)//前数牌值大于后数牌值
			{
				memcpy(&tempChiType, &(resultType.chiType[i]), sizeof(ChiTypeDef));
				memcpy(&(resultType.chiType[i]), &(resultType.chiType[j]), sizeof(ChiTypeDef));
				memcpy(&(resultType.chiType[j]), &tempChiType, sizeof(ChiTypeDef));
			}
		}
	}

	PengTypeDef tempPengType;
	for (int i = 0; i < (resultType.iPengNum - 1); i++)
	{
		for (int j = i + 1; j<resultType.iPengNum; j++)
		{
			if (resultType.pengType[i].iValue > resultType.pengType[j].iValue)//前数牌值大于后数牌值
			{
				memcpy(&tempPengType, &(resultType.pengType[i]), sizeof(PengTypeDef));
				memcpy(&(resultType.pengType[i]), &(resultType.pengType[j]), sizeof(PengTypeDef));
				memcpy(&(resultType.pengType[j]), &tempPengType, sizeof(PengTypeDef));
			}
		}
	}

	JudgeFanLevelOne(resultType, cResult);			// 128,88,64,48,32番
	JudgeFanLevelTwo(resultType, cResult);			// 24,16番
	JudgeFanLevelThree(resultType, cResult);		// 12,8番
	JudgeFanLevelFour(resultType, cResult);			// 7，6，4番
	JudgeFanLevelFive(resultType, cResult);			// 2番
													//JudgeGenFan(resultType, cResult);				// 根

	int iTotalFan = CalculateTotalFan(cResult);

	return iTotalFan;
}

// 七对牌的番型单独计算
/*
@param allPai:玩家手牌
@param resultType:吃碰杠信息，用于判断自摸
@iHZCardNum:凑成七对中，使用红中的张数
@return cResult:番型记录
*/
void JudgeResult::JudgeQiDuiFan(int allPai[3][10], ResultTypeDef &resultType, char *cResult/*, int iHZCardNum = 0*/)
{
	//int i, j, m;
	int iTotalFan = 0;
	// 88番，连七对 由同一种花色序数牌组成序数相连的7个对子的和牌。不计清一色、不求人、单钓、门清、缺一门
	int iTotalNum = 0;
	for (int i = 0; i < 3; i++)
	{
		iTotalNum += allPai[i][0];
	}
	if (iTotalNum != 14)			// 胡七对不能有吃碰杠
	{
		return;
	}

	int iTypeTwoNum[3];				// 统计各花色对子数
	int iTypeFourNum[3];			// 统计各花色四张相同的牌的数量
	int iCardNum[3];				// 如果存在4张一样的牌，保存牌的ID
	memset(iTypeTwoNum, 0, sizeof(iTypeTwoNum));
	memset(iTypeFourNum, 0, sizeof(iTypeFourNum));
	memset(iCardNum, 0, sizeof(iCardNum));

	bool b19 = false;
	bool b1234 = false;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 1; j < 10; j++)
		{
			if (allPai[i][j] == 2)
			{
				iTypeTwoNum[i]++;
			}
			else if (allPai[i][j] == 4)
			{
				iTypeTwoNum[i]++;
				iTypeTwoNum[i]++;
				iCardNum[iTypeFourNum[i]] = i * 9 + j;			// 四张一样的牌
				iTypeFourNum[i]++;
			}
			else if (allPai[i][j] == 6)
			{
				iTypeTwoNum[i] += 3;
				iCardNum[iTypeFourNum[i]] = i * 9 + j;			// 6张一样的牌
				iTypeFourNum[i]++;
			}
			else if (allPai[i][j] == 8)
			{
				iTypeTwoNum[i] += 4;
				iCardNum[iTypeFourNum[i]] = i * 9 + j;			// 8张一样的牌
				iTypeFourNum[i] += 2;
			}
			if (allPai[i][j] > 0)
			{
				if (j == 1 || j == 9)
				{
					b19 = true;
				}
			}
			if (j > 4 && allPai[i][j] > 0)   // 不满足小于5的番型
			{
				b1234 = true;
			}
		}
	}

	//七对
	if (iTypeTwoNum[0] + iTypeTwoNum[1] + iTypeTwoNum[2] == 7)
	{
		// 4番，推不倒：仅由1234589筒和245689条组成的胡牌
		if (cResult[TUI_BU_DAO] != -1)
		{
			bool  bTypeOk = true;
			bool  bValueOk = true;

			if (allPai[0][0] > 0)   //先判断牌型，存在万，不符合推不倒
				bTypeOk = false;

			if (bTypeOk)
			{
				for (int i = 1; i < 3; i++)
				{
					if (i == 1)
					{
						if (allPai[i][1] > 0 || allPai[i][3] > 0 || allPai[i][7] > 0)		// 存在1，3，7条
						{
							bValueOk = false;
							break;
						}
					}
					else if (i == 2)
					{
						if (allPai[i][6] > 0 || allPai[i][7] > 0)			// 存在6，7筒
						{
							bValueOk = false;
							break;
						}
					}
				}
			}

			if (bTypeOk && bValueOk)
			{
				cResult[TUI_BU_DAO] = 4;
			}
		}

		// 判断龙七对
		if (iTypeFourNum[0] + iTypeFourNum[1] + iTypeFourNum[2] > 0)
		{
			if (cResult[LONG_QI_DUI] != -1)
			{
				cResult[LONG_QI_DUI] = 16;
				cResult[QI_DUI] = -1;
			}
		}

		if (iTypeFourNum[0] + iTypeFourNum[1] + iTypeFourNum[2] == 3)// 三个杠牌
		{
			if (!b1234)
			{
				if (cResult[XIAO_YU_WU] != -1)
					cResult[XIAO_YU_WU] = 8;
			}

			bool b12 = false;
			bool b13 = false;
			if (iCardNum[0] + 1 == iCardNum[1])
			{
				b12 = true;
			}

			if (iCardNum[0] + 2 == iCardNum[2])
			{
				b13 = true;
			}
			if (b12 && b13)			// 一色三节高
			{
				if (cResult[SI_JIE_GAO] != -1)
				{
					cResult[SI_JIE_GAO] = 16;
					cResult[SAN_AN_KE] = -1;
				}
			}
		}

		for (int i = 0; i < 3; i++)
		{
			// 32番，绿一色 由23468条中的任何牌组成的顺子、刻子（杠）、将牌的胡牌
			if (i == 1)
			{
				if (iTypeTwoNum[i] == 7)
				{
					if (allPai[i][1] == 0 && allPai[i][5] == 0 && allPai[i][7] == 0 && allPai[i][9] == 0)
					{
						cResult[LV_YI_SE] = 32;
					}
				}
			}

			if (iTypeTwoNum[i] == 7)	//单色七对,判断连七对，并且跳过 一色双龙会
			{
				for (int j = 1; j < 10; j++)
				{
					if (allPai[i][j] > 0)
					{
						int iAddNum = 0;
						int m = 0;
						for (m = j + 1; m < 10; m++)
						{
							if (allPai[i][m] != 2)
							{
								break;
							}
						}
						if (m - j == 7)//连七对
						{
							cResult[LIAN_QI_DUI] = 88;

							cResult[MJ_QING_YI_SE] = -1;
							cResult[BU_QIU_REN] = -1;
							cResult[DAN_DIAO_JIANG] = -1;
							cResult[MEN_QING] = -1;
							cResult[QI_DUI] = -1;
							if (j == 2)//2开始的连七对肯定是断幺九
							{
								cResult[DUAN_YAO_JIU] = 2;
							}
							if (resultType.lastCard.bOther == false)//自摸
							{
								cResult[ZI_MO] = 2;
							}
							return;
						}
					}
				}
				//上面的连七对没返回继续判断 一色双龙会(一种花色的两个老少副，5为将牌)
				if (allPai[i][1] == 2 && allPai[i][2] == 2 && allPai[i][3] == 2 && allPai[i][5] == 2
					&& allPai[i][7] == 2 && allPai[i][8] == 2 && allPai[i][9] == 2)
				{
					return;	//直接跳过交给JudageNormalHu处理
				}
				else//七对
				{
					//23468条绿一色
					if (i == 1)
					{
						if (allPai[i][1] == 0 && allPai[i][5] == 0 && allPai[i][7] == 0 && allPai[i][9] == 0)
						{
							cResult[LV_YI_SE] = 32;
						}
					}

					if (cResult[QI_DUI] != -1)
						cResult[QI_DUI] = 7;
					cResult[BU_QIU_REN] = -1;
					cResult[DAN_DIAO_JIANG] = -1;
					cResult[MEN_QING] = -1;

					cResult[MJ_QING_YI_SE] = 6;

					if (resultType.lastCard.bOther == false)//自摸
					{
						cResult[ZI_MO] = 2;
					}
					return;
				}
			}
		}

		//前面单色七对都没返回,继续判断混七对或者其他
		int iTotalType = 0;
		int iJudgeType = -1;
		for (int i = 0; i < 3; i++)
		{
			if (iTypeTwoNum[i] > 0)
			{
				iTotalType++;
				iJudgeType = i;
			}
		}
		if (iTotalType == 1)
		{
			bool bLYS = true;
			if (iJudgeType == 1)//只有条，判断下绿一色吧,23468条绿一色
			{

				for (int j = 0; j < 9; j++)
				{
					if (allPai[3][j] > 0 && j != 6)
					{
						bLYS = false;
						break;
					}
					if (allPai[2][j] > 0 && j != 2 && j != 3 && j != 4 && j != 6 && j != 8)
					{
						bLYS = false;
						break;
					}
				}

			}
			else
			{
				bLYS = false;
			}

			if (bLYS)
			{
				cResult[LV_YI_SE] = 32;
			}

			if (cResult[QI_DUI] != -1)
				cResult[QI_DUI] = 7;
			cResult[BU_QIU_REN] = -1;
			cResult[DAN_DIAO_JIANG] = -1;
			cResult[MEN_QING] = -1;

			if (resultType.lastCard.bOther == false)//自摸
			{
				cResult[ZI_MO] = 2;
			}
			if (b19 == false)
			{
				cResult[DUAN_YAO_JIU] = 2;
			}

			return;
		}
		else
		{
			if (cResult[QI_DUI] != -1)
				cResult[QI_DUI] = 7;
			cResult[BU_QIU_REN] = -1;
			cResult[DAN_DIAO_JIANG] = -1;
			cResult[MEN_QING] = -1;

			if (resultType.lastCard.bOther == false)//自摸
			{
				cResult[ZI_MO] = 2;
			}
			if (b19 == false)
			{
				cResult[DUAN_YAO_JIU] = 2;
			}

			return;
		}
	}
}

// 一些特殊的番型放在这里判断，需要在普通番型之前判断
int JudgeResult::JudgeSpecialFan(ResultTypeDef resultType, char *cResult)
{
	// 88番，连七对 由同一种花色序数牌组成序数相连的7个对子的和牌。不计清一色、不求人、单钓、门清、缺一门

	// 48番，天胡,庄家在发完牌就胡牌。庄家不可以 碰，杠（包括暗杠）。（不计其他牌型，天胡不会计算加倍）

	// 24番，地胡，非庄家在发完手牌后就听牌，第一圈摸牌后就自摸胡牌，闲家不能  碰，杠（包括暗杠），玩家放炮不算，可以点过，等自摸。（不计其他牌型，地胡不会计算加倍）

	// 16番，龙七对：玩家手牌为暗七对牌型，没有碰过或者杠过，并且有四张牌是一样的，叫龙七对。（不计七对、1根）

	// 7番，七对：玩家的手牌全部是两张一对的，没有碰过和杠过, 不计不求人、单钓、门清

	// 2番，素胡：胡牌时，没有红中癞子参与的牌型

	// 2番，根：胡牌玩家所有牌中有4张一样的牌即算1根

	// 2番，杠上炮：杠牌后，打出的牌造成其他玩家点炮胡牌

	// 2番，抢杠胡：玩家补杠操作时，其他玩家刚好胡这张牌，此时为抢杠胡，且本次补杠无效

	// 2番，海底捞月：摸到最后1张牌胡牌

	// 2番，妙手回春：打出牌堆中最后一张牌点炮
}

// 红中麻将番型计算：128,88,64,48,32番
void JudgeResult::JudgeFanLevelOne(ResultTypeDef resultType, char *cResult)
{
	//int i, j, m;

	// 128番，十八罗汉 由4个杠和一对将组成的胡牌（不计金钩钓、单钓、十二金钗）
	if (cResult[SHI_BA_LUO_HAN] != -1)
	{
		//_log(_DEBUG, "JudgeResult", "JudgeFanLevelOne_Log() resultType.iPengNum = %d", resultType.iPengNum);
		if (resultType.iPengNum == 4)
		{
			int i = 0;
			for (i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iGangType == 0)		// 有碰牌，则不能算
				{
					break;
				}
			}
			if (i > 0 && i == resultType.iPengNum)//是4个杠
			{
				cResult[SHI_BA_LUO_HAN] = 127;		// 超过127数组越界，特殊处理

				cResult[MJ_JIN_GOU_DIAO] = -1;
				cResult[DAN_DIAO_JIANG] = -1;
				cResult[SHI_ER_JIN_CHAI] = -1;
			}
		}
	}

	// 88番，清幺九 全由序数牌1和9组成的胡牌，不计碰碰和、同刻
	if (cResult[QING_YAO_JIU] != -1)
	{
		if (resultType.jiangType.iType != 4)							// 将牌不是红中对
		{
			int i = 0;
			if (resultType.iPengNum == 4)								// 不能出现顺子
			{
				for (i = 0; i<resultType.iPengNum; i++)
				{
					if (resultType.pengType[i].iType > 2)				// 非万条筒
					{
						break;
					}
					else if (resultType.pengType[i].iValue != 1 && resultType.pengType[i].iValue != 9)		// 非幺九刻
					{
						break;
					}
				}
			}

			if (i > 0 && i == resultType.iPengNum)	// 全是1,9碰
			{
				//_log(_DEBUG, "JudgeResult", "JudgeFanLevelOne_Log() resultType.iPengNum = %d, jiangType.iValue= %d", resultType.iPengNum, resultType.jiangType.iValue);
				if (resultType.jiangType.iValue == 1 || resultType.jiangType.iValue == 9)
				{
					cResult[QING_YAO_JIU] = 88;
					cResult[MJ_PENG_PENG_HU] = -1;
					cResult[SHUANG_TONG_KE] = -1;
				}
			}
		}
	}

	// 88番，连七对 由同一种花色序数牌组成序数相连的7个对子的和牌。不计清一色、不求人、单钓、门清、缺一门

	// 64番，一色双龙会 一种花色的两个老少副，5为将牌。不计平胡、七对、清一色、老少配
	if (cResult[YI_SE_SHUANG_LONG_HUI] != -1)
	{
		if (resultType.iChiNum == 4)
		{
			if (resultType.jiangType.iValue == 5 && resultType.jiangType.iType < 3)//将牌必须为5
			{
				int i123Num = 0;
				int i789Num = 0;
				for (int i = 0; i<resultType.iChiNum; i++)
				{
					if (resultType.chiType[i].iFirstValue == 1 && resultType.chiType[i].iType == resultType.jiangType.iType)		// 两个123顺
					{
						i123Num++;
					}
					else if (resultType.chiType[i].iFirstValue == 7 && resultType.chiType[i].iType == resultType.jiangType.iType)	// 两个789顺
					{
						i789Num++;
					}
				}
				if (i123Num == 2 && i789Num == 2)
				{
					cResult[YI_SE_SHUANG_LONG_HUI] = 64;
					cResult[MJ_PING_HU] = -1;
					cResult[QI_DUI] = -1;
					cResult[MJ_QING_YI_SE] = -1;
					cResult[LAO_SHAO_PEI] = -1;
				}
			}
		}
	}

	// 48番，天胡,庄家在发完牌就胡牌。庄家不可以 碰，杠（包括暗杠）。（不计其他牌型，天胡不会计算加倍）

	// 48番，九莲宝灯 由一种花色序数牌子按1112345678999组成的特定牌型，见同花色任何1张序数牌即成和牌。不计清一色、幺九刻、不求人、门清、老少配
	if (cResult[JIU_LIAN_BAO_DENG] != -1)
	{
		char tempCardNum[10];
		memset(tempCardNum, 0, sizeof(tempCardNum));
		bool bJLBDOK = true;
		for (int i = 0; i<resultType.iChiNum; i++)
		{
			if (resultType.chiType[i].bOther)			// 不能有吃牌
			{
				bJLBDOK = false;
				break;
			}
			if (resultType.chiType[i].iType != resultType.jiangType.iType)
			{
				bJLBDOK = false;
				break;
			}
			tempCardNum[resultType.chiType[i].iFirstValue]++;
			tempCardNum[resultType.chiType[i].iFirstValue + 1]++;
			tempCardNum[resultType.chiType[i].iFirstValue + 2]++;

		}
		if (bJLBDOK == true)
		{
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].bOther || resultType.pengType[i].iGangType == 2)				// 不能有碰杠牌
				{
					bJLBDOK = false;
					break;
				}
				if (resultType.pengType[i].iType != resultType.jiangType.iType)
				{
					bJLBDOK = false;
					break;
				}
				tempCardNum[resultType.pengType[i].iValue] += 3;

			}
		}
		if (bJLBDOK == true)
		{
			tempCardNum[resultType.jiangType.iValue] += 2;
			if (tempCardNum[1] >= 3 && tempCardNum[2] >= 1 && tempCardNum[3] >= 1 && tempCardNum[4] >= 1
				&& tempCardNum[5] >= 1 && tempCardNum[6] >= 1 && tempCardNum[7] >= 1 && tempCardNum[8] >= 1
				&& tempCardNum[9] >= 3)
			{
				cResult[JIU_LIAN_BAO_DENG] = 48;
				cResult[MJ_QING_YI_SE] = -1;
				cResult[BU_QIU_REN] = -1;
				cResult[MEN_QING] = -1;
				cResult[LAO_SHAO_PEI] = -1;
			}
		}
	}

	// 48番，天胡,庄家在发完牌就胡牌。庄家不可以 碰，杠（包括暗杠）。（不计其他牌型，天胡不会计算加倍）
	//if (cResult[MJ_TIAN_HU] != -1)
	//{
	//	if (resultType.lastCard.bTianHu == true)
	//	{
	//		cResult[TIAN_HU] = 48;
	//	}
	//}

	// 32番，绿一色 由23468条中的任何牌组成的顺子、刻子（杠）、将牌的胡牌
	if (cResult[LV_YI_SE] != -1)
	{
		bool bChiOK = true;
		for (int i = 0; i<resultType.iChiNum; i++)
		{
			if (resultType.chiType[i].iType != 1)//必须为条
			{
				bChiOK = false;
				break;
			}
			else
			{
				if (resultType.chiType[i].iFirstValue != 2)//只能为234条了
				{
					bChiOK = false;
					break;
				}
			}
		}
		bool bPengOK = true;
		if (bChiOK == true)
		{
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iType != 1)//必须为条
				{
					bPengOK = false;
					break;
				}
				else
				{
					if (resultType.pengType[i].iValue != 2 && resultType.pengType[i].iValue != 3
						&& resultType.pengType[i].iValue != 4 && resultType.pengType[i].iValue != 6
						&& resultType.pengType[i].iValue != 8)//只能为23468条
					{
						bPengOK = false;
						break;
					}
				}
			}
		}
		bool bJiangOK = true;
		if (bChiOK == true && bPengOK == true)
		{
			if (resultType.jiangType.iType != 1)//必须为条
			{
				bJiangOK = false;
			}
			else
			{
				if (resultType.jiangType.iValue != 2 && resultType.jiangType.iValue != 3
					&& resultType.jiangType.iValue != 4 && resultType.jiangType.iValue != 6
					&& resultType.jiangType.iValue != 8)//只能为23468条及发字
				{
					bJiangOK = false;
				}
			}
		}
		if (bChiOK == true && bPengOK == true && bJiangOK == true)
		{
			cResult[LV_YI_SE] = 32;
		}
	}

	// 32番，全带幺：玩家手牌中，全部是用1的连牌或者9的连牌组成的牌
	if (cResult[QUAN_DAI_YAO] != -1)
	{
		if (resultType.jiangType.iType < 3)		// 将牌不能为红中
		{
			if (resultType.jiangType.iValue == 1 || resultType.jiangType.iValue == 9)		// 将牌必须为幺九
			{
				int i = 0;
				for (i = 0; i<resultType.iChiNum; i++)
				{
					if (resultType.chiType[i].iFirstValue != 1 && resultType.chiType[i].iFirstValue != 7)
					{
						break;
					}
				}

				if (i == resultType.iChiNum)
				{
					int j = 0;
					for (j = 0; j<resultType.iPengNum; j++)
					{
						if (resultType.pengType[j].iValue != 1 && resultType.pengType[j].iValue != 9)
						{
							break;
						}
					}
					if (j == resultType.iPengNum)
					{
						cResult[QUAN_DAI_YAO] = 32;
					}
				}
			}
		}
	}

	// 32番，全带五 牌型中所有的顺子，刻子，将  都含有点数5
	if (cResult[QUAN_DAI_WU] != -1)
	{
		if (resultType.jiangType.iType < 3 && resultType.jiangType.iValue == 5)
		{
			int i = 0;
			for (i = 0; i<resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iFirstValue < 3 || resultType.chiType[i].iFirstValue > 5)
				{
					break;
				}
			}
			if (i == resultType.iChiNum)
			{
				int j = 0;
				for (j = 0; j<resultType.iPengNum; j++)
				{
					if (resultType.pengType[j].iValue != 5)
					{
						break;
					}
				}
				if (j == resultType.iPengNum)
				{
					cResult[QUAN_DAI_WU] = 32;
				}

			}
		}
	}
}

// 红中麻将番型计算：24,16番
void JudgeResult::JudgeFanLevelTwo(ResultTypeDef resultType, char *cResult)
{
	//int i, j, m;
	// 24番，地胡，非庄家在发完手牌后就听牌，第一圈摸牌后就自摸胡牌，闲家不能  碰，杠（包括暗杠），玩家放炮不算，可以点过，等自摸。（不计其他牌型，地胡不会计算加倍）
	//if (cResult[MJ_DI_HU] != -1)
	//{
	//	if (resultType.lastCard.bDiHu == true)
	//	{
	//		cResult[MJ_DI_HU] = 32;
	//	}
	//}

	// 24番，全大，仅由序数牌789组成的胡牌，不计大于五
	if (cResult[QUAN_DA] != -1)
	{
		bool bJiangOK = false;
		if (resultType.jiangType.iType < 3)
		{
			if (resultType.jiangType.iValue > 6)
			{
				bJiangOK = true;
			}
		}
		bool bChiOK = true;
		if (bJiangOK)
		{
			for (int i = 0; i < resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iFirstValue != 7)
				{
					bChiOK = false;
					break;
				}
			}
		}
		bool bPengOK = true;
		if (bJiangOK && bChiOK)
		{
			for (int i = 0; i < resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iValue < 7)
				{
					bPengOK = false;
					break;
				}
			}
		}
		if (bJiangOK && bChiOK && bPengOK)
		{
			cResult[QUAN_DA] = 24;
			cResult[DA_YU_WU] = -1;
		}
	}

	// 24番，全中，仅由序数牌456组成的胡牌
	if (cResult[QUAN_ZHONG] != -1)
	{
		bool bJiangOK = false;
		if (resultType.jiangType.iType < 3)
		{
			if (resultType.jiangType.iValue < 7 && resultType.jiangType.iValue > 3)
			{
				bJiangOK = true;
			}
		}
		bool bChiOK = true;
		if (bJiangOK)
		{
			for (int i = 0; i < resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iFirstValue != 4)
				{
					bChiOK = false;
					break;
				}
			}
		}
		bool bPengOK = true;
		if (bJiangOK && bChiOK)
		{
			for (int i = 0; i < resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iValue > 6 || resultType.pengType[i].iValue < 4)
				{
					bPengOK = false;
					break;
				}
			}
		}
		if (bJiangOK && bChiOK && bPengOK)
		{
			cResult[QUAN_ZHONG] = 24;
		}
	}

	// 24番，全小，仅由序数牌123组成的胡牌，不计小于五
	if (cResult[QUAN_XIAO] != -1)
	{
		bool bJiangOK = false;
		if (resultType.jiangType.iType < 3)
		{
			if (resultType.jiangType.iValue < 4)
			{
				bJiangOK = true;
			}
		}
		bool bChiOK = true;
		if (bJiangOK)
		{
			for (int i = 0; i < resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iFirstValue != 1)
				{
					bChiOK = false;
					break;
				}
			}
		}
		bool bPengOK = true;
		if (bJiangOK && bChiOK)
		{
			for (int i = 0; i < resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iValue > 3)
				{
					bPengOK = false;
					break;
				}
			}
		}
		if (bJiangOK && bChiOK && bPengOK)
		{
			cResult[QUAN_XIAO] = 24;
			cResult[XIAO_YU_WU] = -1;
		}
	}

	// 24番，守中抱一：牌型中只剩1张红中单吊红中胡牌
	if (cResult[SHOU_ZHONG_BAO_YI] != -1)
	{
	}

	// 16番，四暗刻，牌型中有4个暗杠/暗刻，不计门清、碰碰胡、三暗刻、双暗刻、不求人、单钓
	if (cResult[SI_AN_KE] != -1)
	{
		if (resultType.iPengNum == 4)	//有4副刻子
		{
			int iAnNum = 0;
			int iLastValue = resultType.lastCard.iType * 9 + resultType.lastCard.iValue - 1;
			int iPengValue = 0;
			for (int i = 0; i < resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].bOther == false)//是求人的,不对
				{
					if (resultType.lastCard.bOther)			// 点炮胡
					{
						iPengValue = resultType.pengType[i].iType * 9 + resultType.pengType[i].iValue - 1;
						int iGangType = resultType.pengType[i].iGangType;
						if (iGangType == 2 || iGangType == 0 && iLastValue != iPengValue)		// 非双碰胡
						{
							iAnNum++;
						}
					}
					else
					{
						iAnNum++;
					}
				}
			}

			if (iAnNum == 4)
			{
				cResult[SI_AN_KE] = 16;
				cResult[SI_JIE_GAO] = -1;
				cResult[SAN_JIE_GAO] = -1;
				cResult[MEN_QING] = -1;
				cResult[MJ_PENG_PENG_HU] = -1;
				cResult[SAN_AN_KE] = -1;
				cResult[SHUANG_AN_KE] = -1;
				cResult[BU_QIU_REN] = -1;
				cResult[DAN_DIAO_JIANG] = -1;
			}
		}
	}

	// 16番，四节高：胡牌时，有4个同种花色且点数连续的刻子/杠，不计三节高、碰碰胡
	if (cResult[SI_JIE_GAO] != -1)
	{
		if (resultType.iPengNum == 4)
		{
			int iBaseType = resultType.pengType[0].iType;
			int iBaseValue = resultType.pengType[0].iValue;
			if (iBaseType < 3)
			{
				if (iBaseType == resultType.pengType[1].iType && iBaseValue == resultType.pengType[1].iValue - 1
					&& iBaseType == resultType.pengType[2].iType && iBaseValue == resultType.pengType[2].iValue - 2
					&& iBaseType == resultType.pengType[3].iType && iBaseValue == resultType.pengType[3].iValue - 3)
				{
					cResult[SI_JIE_GAO] = 16;
					cResult[SAN_JIE_GAO] = -1;
					cResult[MJ_PENG_PENG_HU] = -1;
				}
			}
		}
	}

	// 16番，龙七对：玩家手牌为暗七对牌型，没有碰过或者杠过，并且有四张牌是一样的，叫龙七对。（不计七对、1根）
	if (cResult[LONG_QI_DUI] != -1)
	{
	}
}

// 红中麻将番型计算：12，8番
void JudgeResult::JudgeFanLevelThree(ResultTypeDef resultType, char *cResult)
{
	//_log(_DEBUG, "JudgeResult", "JudgeFanLevelThree_log() 0000  iPengNum[%d]", resultType.iPengNum);
	//int i, j, m;
	// 12番，十二金钗: 胡牌中有3个杠，不计金钩钓、单钓
	if (cResult[SHI_ER_JIN_CHAI] != -1)
	{
		if (resultType.iPengNum >= 3)
		{
			int iGangNum = 0;
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iGangType > 0)
				{
					iGangNum++;
				}
			}
			if (iGangNum >= 3)	//是3个杠
			{
				cResult[SHI_ER_JIN_CHAI] = 12;
				cResult[MJ_JIN_GOU_DIAO] = -1;
				cResult[DAN_DIAO_JIANG] = -1;
			}
		}
	}

	// 8番，金钩钓 所有其余牌均已碰/杠，只留有一张手牌的大对子单钓。（不计碰碰胡， 单钓）
	if (cResult[MJ_JIN_GOU_DIAO] != -1)
	{
		if (resultType.lastCard.iType == resultType.jiangType.iType && resultType.lastCard.iValue == resultType.jiangType.iValue)		// 单钓胡牌
		{
			if (resultType.iPengNum == 4)
			{
				bool bAllOther = true;
				for (int i = 0; i < resultType.iPengNum; i++)
				{
					if (resultType.pengType[i].iGangType == 0 && resultType.pengType[i].bOther == false)
					{
						bAllOther = false;
						break;
					}
				}

				if (bAllOther)
				{
					cResult[MJ_JIN_GOU_DIAO] = 8;
					cResult[MJ_PENG_PENG_HU] = -1;
					cResult[DAN_DIAO_JIANG] = -1;
				}
			}
		}
	}

	// 8番，大于五：由序数牌6-9的顺子、刻子（杠）、将牌组成的胡牌
	if (cResult[DA_YU_WU] != -1)
	{
		bool bJiangOK = false;
		if (resultType.jiangType.iType < 3)			// 红中不能做将牌
		{
			if (resultType.jiangType.iValue > 5)
			{
				bJiangOK = true;
			}
		}
		bool bChiOK = true;
		if (bJiangOK)
		{
			for (int i = 0; i<resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iFirstValue < 6)
				{
					bChiOK = false;
					break;
				}
			}
		}
		bool bPengOK = true;
		if (bJiangOK && bChiOK)
		{
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iValue < 6)
				{
					bPengOK = false;
					break;
				}
			}
		}
		if (bJiangOK && bChiOK && bPengOK)
		{
			cResult[DA_YU_WU] = 8;
		}
	}

	// 8番，小于五：由序数牌1-4的顺子、刻子（杠）、将牌组成的胡牌
	if (cResult[XIAO_YU_WU] != -1)
	{
		bool bJiangOK = false;
		if (resultType.jiangType.iType < 3)
		{
			if (resultType.jiangType.iValue < 5)
			{
				bJiangOK = true;
			}
		}
		bool bChiOK = true;
		if (bJiangOK)
		{
			for (int i = 0; i<resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iFirstValue > 2)
				{
					bChiOK = false;
					break;
				}
			}
		}

		bool bPengOK = true;   // 必须考虑一色四同顺子
		if (resultType.iChiNum < 4)
		{
			if (bJiangOK && bChiOK)
			{
				for (int i = 0; i<resultType.iPengNum; i++)
				{
					if (resultType.pengType[i].iValue > 4)
					{
						bPengOK = false;
						break;
					}
				}
			}
		}

		if (bJiangOK && bChiOK && bPengOK)
		{
			cResult[XIAO_YU_WU] = 8;
		}
	}

	// 8番，三节高：胡牌时，有3个同种花色且点数连续的刻子/杠
	if (cResult[SAN_JIE_GAO] != -1)
	{
		if (resultType.iPengNum >= 3)
		{
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iType < 3)
				{
					bool bAdd1 = false;
					bool bAdd2 = false;
					for (int j = i + 1; j<resultType.iPengNum; j++)
					{
						if (resultType.pengType[i].iValue == resultType.pengType[j].iValue - 1
							&& resultType.pengType[i].iType == resultType.pengType[j].iType)
						{
							bAdd1 = true;
						}
						else if (resultType.pengType[i].iValue == resultType.pengType[j].iValue - 2
							&& resultType.pengType[i].iType == resultType.pengType[j].iType)
						{
							bAdd2 = true;
						}
					}
					if (bAdd1 && bAdd2)
					{
						cResult[SAN_JIE_GAO] = 8;
						cResult[SAN_AN_KE] = -1;
						cResult[SHUANG_AN_KE] = -1;
						break;
					}
				}
			}
		}
		else
		{
			int iType = resultType.chiType[0].iType;
			int iValue = resultType.chiType[0].iFirstValue;

			int iNext0 = 1;
			int iNext1 = 1;
			int iNext2 = 1;

			if (resultType.chiType[0].bOther == false && resultType.iChiNum >= 3)
			{
				for (int i = 1; i<resultType.iChiNum; i++)
				{
					if (resultType.chiType[i].iType == iType && resultType.chiType[i].bOther == false)
					{
						if (resultType.chiType[i].iFirstValue == iValue + 1)
						{
							iNext1++;
							iNext2++;
						}

						if (resultType.chiType[i].iFirstValue == iValue + 2)
						{
							iNext2++;
						}
					}
				}

				if (resultType.jiangType.iType == iType)
				{
					if (resultType.jiangType.iValue == iValue)
					{
						iNext0++;
						iNext0++;
					}
					else if (resultType.jiangType.iValue == iValue + 1)
					{
						iNext1++;
						iNext1++;
					}
					else if (resultType.jiangType.iValue == iValue + 2)
					{
						iNext2++;
						iNext2++;
					}
				}

				if (iNext0 == 3 && iNext1 == 3 && iNext2 == 3)
				{
					cResult[SAN_JIE_GAO] = 8;
					cResult[SAN_AN_KE] = -1;
					cResult[SHUANG_AN_KE] = -1;
				}
			}
		}
	}

	// 8番，全双刻：牌型中只有刻/杠和将，且刻子点数都是偶数，不计碰碰胡
	if (cResult[QUAN_SHUANG_KE] != -1)
	{
		if (resultType.iPengNum == 4)
		{
			int i = 0;
			for (i = 0; i<4; i++)
			{
				if (resultType.pengType[i].iType >= 3)//不能有字
				{
					break;
				}
				else
				{
					if (resultType.pengType[i].iValue % 2 != 0)//不为2的倍数
					{
						break;
					}
				}
			}
			if (i == 4)
			{
				if (resultType.jiangType.iType < 3)
				{
					if (resultType.jiangType.iValue % 2 == 0)
					{
						cResult[QUAN_SHUANG_KE] = 8;
						cResult[MJ_PENG_PENG_HU] = -1;
						//cResult[DUAN_YAO] = -1;
					}
				}
			}
		}
	}

	// 8番，百万石：牌型中万字牌的点数≥100
	if (cResult[BAI_WAN_DAN] != -1)
	{
		int iAllWanNum = 0;
		if (resultType.jiangType.iType == 0)
		{
			iAllWanNum += resultType.jiangType.iValue * 2;
		}

		for (int i = 0; i < resultType.iChiNum; i++)
		{
			if (resultType.chiType[i].iType == 0)
			{
				iAllWanNum += resultType.chiType[i].iFirstValue * 3;
				iAllWanNum += 1;
				iAllWanNum += 2;
			}
		}

		for (int i = 0; i < resultType.iPengNum; i++)
		{
			if (resultType.pengType[i].iType == 0)
			{
				if (resultType.pengType[i].iGangType == 0)
					iAllWanNum += resultType.pengType[i].iValue * 3;
				else
				{
					if (resultType.pengType[i].iGangType > 0)
						iAllWanNum += resultType.pengType[i].iValue * 4;
				}
			}
		}

		if (iAllWanNum >= 100)
			cResult[BAI_WAN_DAN] = 8;
	}
}

// 红中麻将番型计算：7,6,4番
void JudgeResult::JudgeFanLevelFour(ResultTypeDef resultType, char *cResult)
{
	//int i, j, m;
	// 7番，七对：玩家的手牌全部是两张一对的，没有碰过和杠过, 不计不求人、单钓、门清

	//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFour_Log() cResult[MJ_QING_YI_SE] = %d", cResult[MJ_QING_YI_SE]);
	//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFour_Log() jiangType[%d]", resultType.jiangType.iType);
	//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFour_Log() iChiNum[%d], iPengNum[%d]", resultType.iChiNum, resultType.iPengNum);
	// 6番，清一色：玩家胡牌的手牌全部都是一门花色
	if (cResult[MJ_QING_YI_SE] != -1)
	{
		if (resultType.jiangType.iType < 3)
		{
			int i = 0;
			for (i = 0; i<resultType.iChiNum; i++)
			{
				//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFour_Log()  iType = %d, iFirstValue = %d", resultType.chiType[i].iType, resultType.chiType[i].iFirstValue);
				if (resultType.chiType[i].iType != resultType.jiangType.iType)
				{
					break;
				}
			}
			if (i == resultType.iChiNum)
			{
				int j = 0;
				for (j = 0; j<resultType.iPengNum; j++)
				{
					//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFour_Log() resultType.pengType.iType = %d, resultType.pengType.iValue = %d", resultType.pengType[j].iType, resultType.pengType[j].iValue);
					if (resultType.pengType[j].iType != resultType.jiangType.iType)
					{
						break;
					}
				}
				//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFour_Log() j = %d, iPengNum = %d", j, resultType.iPengNum);
				if (j == resultType.iPengNum)
				{
					cResult[MJ_QING_YI_SE] = 6;
				}
			}
		}
	}

	// 6番，清龙：胡牌时，有一种花1-9相连接的序数牌, 不计老少配
	if (cResult[QING_LONG] != -1)
	{
		//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFour_Log() resultType.iChiNum = %d", resultType.iChiNum);
		if (resultType.iChiNum >= 3)
		{
			int iShunValue[4] = { 0 };
			int iShunType[4] = { 0 };
			for (int i = 0; i < resultType.iChiNum; i++)
			{
				iShunValue[i] = resultType.chiType[i].iFirstValue;
				iShunType[i] = resultType.chiType[i].iType;
				//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFour_Log() iShunValue[%d] = %d, iShunType[%d]= %d", i, iShunValue[i], i, iShunType[i]);
			}

			for (int i = 0; i < resultType.iChiNum; i++)
			{
				bool bOne = false;
				bool bTwo = false;
				for (int j = 0; j < resultType.iChiNum; j++)
				{
					if (j != i)
					{
						if (iShunValue[i] % 10 + 3 == iShunValue[j] % 10 && iShunType[j] == iShunType[i])
						{
							bOne = true;
						}
						if (iShunValue[i] % 10 + 6 == iShunValue[j] % 10 && iShunType[j] == iShunType[i])
						{
							bTwo = true;
						}
					}
				}
				if (bOne && bTwo)
				{
					cResult[QING_LONG] = 6;
					cResult[LAO_SHAO_PEI] = -1;
					break;
				}
			}
		}
	}

	// 6番，三暗刻：牌型中有3个暗杠/暗刻, 不计双暗刻
	if (cResult[SAN_AN_KE] != -1)
	{
		if (resultType.iPengNum >= 3)	// 有3副刻子
		{
			int iAnNum = 0;
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].bOther == false)//是求人的,不对
				{
					if (resultType.pengType[i].iType == resultType.lastCard.iType && resultType.pengType[i].iValue == resultType.lastCard.iValue)
					{
						if (resultType.lastCard.bOther == false)		// 自摸胡
						{
							iAnNum++;
						}
						else //别人放炮的情况下 如果最后一张牌和玩家吃牌数组相同 说明玩家手中已经有了暗刻
						{
							for (int j = 0; j < resultType.iChiNum; j++)
							{
								if (!resultType.chiType[j].bOther &&
									resultType.chiType[j].iType == resultType.lastCard.iType
									&& (resultType.chiType[j].iFirstValue == resultType.lastCard.iValue || resultType.chiType[j].iFirstValue + 1 == resultType.lastCard.iValue || resultType.chiType[j].iFirstValue + 2 == resultType.lastCard.iValue))
								{
									iAnNum++;
								}
							}
						}
					}
					else //一定暗刻
					{
						iAnNum++;
					}
				}
			}
			if (iAnNum >= 3)
			{
				cResult[SAN_AN_KE] = 6;
				cResult[SHUANG_AN_KE] = -1;
			}
		}
	}

	// 4番，杠上开花：杠后自模胡牌（杠了之后补牌而胡）
	if (cResult[MJ_GANG_SHANG_KAI_HUA] != -1)
	{
		if (resultType.lastCard.bGang == true)
		{
			cResult[MJ_GANG_SHANG_KAI_HUA] = 4;
		}
	}

	// 4番，推不倒：仅由1234589筒和245689条组成的胡牌
	if (cResult[TUI_BU_DAO] != -1)
	{
		bool bJiangOK = false;
		if (resultType.jiangType.iType < 3)
		{
			if (resultType.jiangType.iType == 2)//饼
			{
				if (resultType.jiangType.iValue < 6 || resultType.jiangType.iValue == 8 || resultType.jiangType.iValue == 9)
				{
					bJiangOK = true;
				}
			}
			else if (resultType.jiangType.iType == 1)//条
			{
				if (resultType.jiangType.iValue == 2 || resultType.jiangType.iValue == 4
					|| resultType.jiangType.iValue == 5 || resultType.jiangType.iValue == 6
					|| resultType.jiangType.iValue == 8 || resultType.jiangType.iValue == 9)
				{
					bJiangOK = true;
				}
			}
		}

		bool bChiOK = true;
		if (bJiangOK)
		{
			for (int i = 0; i<resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iType == 0)
				{
					bChiOK = false;
					break;
				}
				else if (resultType.chiType[i].iType == 2)
				{
					if (resultType.chiType[i].iFirstValue > 3)
					{
						bChiOK = false;
						break;
					}
				}
				else if (resultType.chiType[i].iType == 1)
				{
					if (resultType.chiType[i].iFirstValue != 4)
					{
						bChiOK = false;
						break;
					}
				}
			}

		}
		bool bPengOK = true;
		if (bJiangOK && bChiOK)
		{
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iType == 0)
				{
					bPengOK = false;
					break;
				}
				else if (resultType.pengType[i].iType == 2)
				{
					if (resultType.pengType[i].iValue == 6 || resultType.pengType[i].iValue == 7)
					{
						bPengOK = false;
						break;
					}
				}
				else if (resultType.pengType[i].iType == 1)
				{
					if (resultType.pengType[i].iValue == 1 || resultType.pengType[i].iValue == 3 || resultType.pengType[i].iValue == 7)
					{
						bPengOK = false;
						break;
					}
				}
			}
		}
		if (bJiangOK && bChiOK && bPengOK)
		{
			cResult[TUI_BU_DAO] = 4;
		}
	}

	// 4番，不求人：胡牌时，没有碰牌（包括明杠），自摸胡牌牌, 不计自摸、门前清
	if (cResult[BU_QIU_REN] != -1)
	{
		if (resultType.lastCard.bOther == false)			// 自摸胡
		{
			int i = 0;
			for (i = 0; i<resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].bOther == true)
				{
					break;
				}
			}
			if (i == resultType.iChiNum)
			{
				int j = 0;
				for (j = 0; j<resultType.iPengNum; j++)
				{
					if (resultType.pengType[j].bOther == true)
					{
						break;
					}
				}
				if (j == resultType.iPengNum)
				{
					cResult[BU_QIU_REN] = 4;
					cResult[ZI_MO] = -1;
				}
			}
		}
	}
}

// 红中麻将番型计算：2番
void JudgeResult::JudgeFanLevelFive(ResultTypeDef resultType, char *cResult)
{
	//int i, j, m;
	// 2番，素胡：胡牌时，没有红中癞子参与的牌型，素胡放在前面判断

	// 2番，碰碰胡：由4种刻牌/杠外加1对牌组成的胡牌
	if (cResult[MJ_PENG_PENG_HU] != -1)
	{
		if (resultType.iPengNum == 4)
		{
			cResult[MJ_PENG_PENG_HU] = 2;
		}
	}

	// 2番，单钓：胡牌时，是属于钓将胡牌
	if (cResult[DAN_DIAO_JIANG] != -1)
	{
		//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFive_Log() lastCard.iType[%d], jiangType.iType[%d]", resultType.lastCard.iType, resultType.jiangType.iType);
		//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFive_Log() lastCard.iValue[%d], jiangType.iValue[%d]", resultType.lastCard.iValue, resultType.jiangType.iValue);
		if (resultType.lastCard.iType == resultType.jiangType.iType && resultType.lastCard.iValue == resultType.jiangType.iValue)
		{
			//还需要考虑该单张只能是唯一的胡牌.考虑假设将牌是4，那么没有123和567的顺子应该就可以了(1234，可以单钓1或者4，这种不算单钓将)
			int i = 0;
			for (i = 0; i<resultType.iChiNum; i++)
			{
				//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFive_Log() i[%d], bOther[%d], iType[%d]", i, resultType.chiType[i].bOther, resultType.chiType[i].iType);
				if (resultType.chiType[i].bOther == false && resultType.chiType[i].iType == resultType.jiangType.iType)
				{
					if (resultType.chiType[i].iFirstValue == resultType.jiangType.iValue - 3
						|| resultType.chiType[i].iFirstValue == resultType.jiangType.iValue + 1)
					{
						//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFive_Log() i[%d], iFirstValue[%d]", i, resultType.chiType[i].iFirstValue);
						break;
					}
				}
			}
			//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFive_Log() i[%d], iChiNum[%d]", i, resultType.iChiNum);
			if (i == resultType.iChiNum)
			{
				cResult[DAN_DIAO_JIANG] = 2;
				//_log(_DEBUG, "JudgeResult", "JudgeFanLevelFive_Log() DAN_DIAO_JIANG");
			}
		}
	}

	// 2番，边张：12胡3,89胡7的牌型, 单和123的3及789的7或1233和3、77879和7都为张。手中有12345和3，56789和7不算边张 
	if (cResult[BIAN_ZHANG] != -1)
	{
		if (resultType.lastCard.iType < 3)
		{
			if (resultType.lastCard.iValue == 3)
			{
				//有123，没有345
				bool b1 = false;
				bool b3 = false;
				bool bNoYao = false;
				for (int i = 0; i<resultType.iChiNum; i++)
				{
					if (resultType.chiType[i].iType == resultType.lastCard.iType && resultType.chiType[i].bOther == false)
					{
						if (resultType.chiType[i].iFirstValue == 1)
						{
							b1 = true;
							if (resultType.chiType[i].iType == resultType.jiangType.iType && resultType.jiangType.iValue == 1)  //防止11123333  
								b1 = false;
						}
						else if (resultType.chiType[i].iFirstValue == 3)
						{
							b3 = true;
						}
					}

					if (resultType.chiType[i].iType == resultType.jiangType.iType
						&&resultType.chiType[i].iFirstValue == 1 && resultType.chiType[i].bOther == false)
					{
						bNoYao = true;
					}

				}
				if (b1 == true && b3 == false)
				{
					cResult[BIAN_ZHANG] = 2;
					//cResult[KAN_ZHANG] = -1;
					//cResult[DAN_DIAO_JIANG] = -1;
				}
			}
			else if (resultType.lastCard.iValue == 7)
			{
				//有789，没有567
				bool b5 = false;
				bool b7 = false;
				for (int i = 0; i<resultType.iChiNum; i++)
				{
					if (resultType.chiType[i].iType == resultType.lastCard.iType && resultType.chiType[i].bOther == false)
					{
						if (resultType.chiType[i].iFirstValue == 5)
						{
							b5 = true;
						}
						else if (resultType.chiType[i].iFirstValue == 7)
						{
							b7 = true;
							if (resultType.chiType[i].iType == resultType.jiangType.iType && resultType.jiangType.iValue == 9)  //防止77778999 
								b7 = false;

							if (cResult[BIAN_ZHANG] != -1 && resultType.chiType[i].iType == resultType.jiangType.iType && resultType.jiangType.iValue == 7)//防止77789
								b7 = false;
						}
					}
				}
				if (b5 == false && b7 == true)
				{
					cResult[BIAN_ZHANG] = 2;
					//cResult[KAN_ZHANG] = -1;
					//cResult[DAN_DIAO_JIANG] = -1;
				}
			}
		}
	}

	// 2番，坎张：胡牌时，只胡顺子中间的那张牌
	if (cResult[KAN_ZHANG] != -1)
	{
		if (resultType.lastCard.iType < 3)
		{
			bool bLError = false;
			bool bMRight = false;
			bool bRError = false;
			for (int i = 0; i<resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iType == resultType.lastCard.iType  && resultType.chiType[i].bOther == false)
				{
					if (resultType.chiType[i].iFirstValue == resultType.lastCard.iValue - 2)
					{
						bLError = true;
					}
					else if (resultType.chiType[i].iFirstValue == resultType.lastCard.iValue - 1)
					{
						bMRight = true;
					}
					else if (resultType.chiType[i].iFirstValue == resultType.lastCard.iValue)
					{
						bRError = true;
					}
				}
			}
			if (bMRight == true && bLError == false && bRError == false)
			{
				cResult[KAN_ZHANG] = 2;
				//cResult[BIAN_ZHANG] = -1;
				//cResult[DAN_DIAO_JIANG] = -1;
			}
		}
	}

	// 2番，双同刻：牌型中包含2个点数相同的刻子
	if (cResult[SHUANG_TONG_KE] != -1)
	{
		if (resultType.iPengNum >= 2)
		{
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].iType < 3)
				{
					for (int j = i + 1; j<resultType.iPengNum; j++)
					{
						if (resultType.pengType[j].iType < 3 && resultType.pengType[j].iValue == resultType.pengType[i].iValue)
						{
							cResult[SHUANG_TONG_KE] = 2;
							break;
						}
					}
				}
			}
		}
	}

	// 2番，双暗刻：牌型中有2个暗杠/暗刻
	if (cResult[SHUANG_AN_KE] != -1)
	{
		if (resultType.iPengNum >= 2)//有2副刻子
		{
			int iAnNum = 0;
			for (int i = 0; i<resultType.iPengNum; i++)
			{
				if (resultType.pengType[i].bOther == false)//是求人的,不对
				{
					if (resultType.pengType[i].iType == resultType.lastCard.iType && resultType.pengType[i].iValue == resultType.lastCard.iValue)
					{
						if (resultType.lastCard.bOther == false)
						{
							iAnNum++;
						}
						else //别人放炮的情况下 如果最后一张牌和玩家吃牌数组相同 说明玩家手中已经有了暗刻
						{
							for (int j = 0; j < resultType.iChiNum; j++)
							{
								if (resultType.chiType[j].bOther == false && resultType.chiType[j].iType == resultType.lastCard.iType
									&& (resultType.chiType[j].iFirstValue == resultType.lastCard.iValue || resultType.chiType[j].iFirstValue + 1 == resultType.lastCard.iValue || resultType.chiType[j].iFirstValue + 2 == resultType.lastCard.iValue))
								{
									iAnNum++;
								}
							}
						}
					}
					else //一定暗刻
					{
						iAnNum++;
					}
				}
			}
			if (iAnNum >= 2)
			{
				cResult[SHUANG_AN_KE] = 2;
			}
		}
	}

	// 2番，老少配：牌型中包含同花色的123+789这两组顺子
	if (cResult[LAO_SHAO_PEI] != -1)
	{
		for (int i = 0; i<resultType.iChiNum; i++)
		{
			if (resultType.chiType[i].iFirstValue == 1)
			{
				for (int j = i + 1; j<resultType.iChiNum; j++)
				{
					if (resultType.chiType[j].iType == resultType.chiType[i].iType && resultType.chiType[j].iFirstValue == 7)
					{
						cResult[LAO_SHAO_PEI] = 2;
						break;
					}
				}
			}
		}
	}

	// 2番，断幺九：不包含任何的1、9的牌
	if (cResult[DUAN_YAO_JIU] != -1)
	{
		if (resultType.jiangType.iValue != 1 && resultType.jiangType.iValue != 9)
		{
			int i = 0;
			for (i = 0; i<resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].iFirstValue == 1 || resultType.chiType[i].iFirstValue == 7)
				{
					break;
				}
			}
			if (i == resultType.iChiNum)
			{
				int j = 0;
				for (j = 0; j<resultType.iPengNum; j++)
				{
					if (resultType.pengType[j].iValue == 1 || resultType.pengType[j].iValue == 9)		// resultType.pengType[j].iType == 3 || 
					{
						break;
					}
				}
				if (j == resultType.iPengNum)
				{
					cResult[DUAN_YAO_JIU] = 2;
				}
			}
		}
	}

	// 2番，门清：没有碰，补杠，明杠的胡牌， 胡他人打出的牌。（暗杠算门清）
	if (cResult[MEN_QING] != -1)
	{
		if (resultType.lastCard.bOther == true)			// 点炮胡
		{
			int i = 0;
			for (i = 0; i<resultType.iChiNum; i++)
			{
				if (resultType.chiType[i].bOther == true)
				{
					break;
				}
			}
			if (i == resultType.iChiNum)
			{
				int j = 0;
				for (j = 0; j<resultType.iPengNum; j++)
				{
					if (resultType.pengType[j].bOther == true)
					{
						break;
					}
				}
				if (j == resultType.iPengNum)
				{
					cResult[MEN_QING] = 2;
				}
			}
		}
	}

	// 2番，自摸
	if (cResult[ZI_MO] != -1)
	{
		if (resultType.lastCard.bOther == false)			// 自摸胡
		{
			cResult[ZI_MO] = 2;
		}
	}
}

// 根最后算，七对相关的根放在七对算番的函数中处理
void JudgeResult::JudgeGenFan(ResultTypeDef resultType, char *cResult)		// 根
{
	int iValueCnt[27];
	memset(iValueCnt, 0, sizeof(iValueCnt));
	int iTotalGenCnt = 0;			// 根的数目

									//_log(_DEBUG, "JudgeResult", "JudgeGenFan_Log() cResult[MJ_GEN][%d]", cResult[MJ_GEN]);
	if (cResult[MJ_GEN] != -1)
	{
		for (int i = 0; i < resultType.iChiNum; i++)
		{
			int iFirstValue = resultType.chiType[i].iFirstValue;
			int iType = resultType.chiType[i].iType;
			int iFirst = iType * 9 + iFirstValue - 1;
			iValueCnt[iFirst]++;
			iValueCnt[iFirst + 1]++;
			iValueCnt[iFirst + 2]++;

			//_log(_DEBUG, "JudgeResult", "JudgeGenFan_Log() i[%d], iFirstValue[%d], iType[%d], iFirst[%d]", i, iFirstValue, iType, iFirst);
		}

		for (int i = 0; i < resultType.iPengNum; i++)
		{
			int iPengValue = resultType.pengType[i].iValue;
			int iType = resultType.pengType[i].iType;
			int iPeng = iType * 9 + iPengValue - 1;
			iValueCnt[iPeng] += 3;
			if (resultType.pengType[i].iGangType > 0)		// 明杠或者暗杠
			{
				iValueCnt[iPeng]++;
			}
			//_log(_DEBUG, "JudgeResult", "JudgeGenFan_Log() i[%d], iPengValue[%d], iType[%d], iPeng[%d], iGangType[%d]", i, iPengValue, iType, iPeng, resultType.pengType[i].iGangType);
		}

		// 将牌
		int iJiangCardId = resultType.jiangType.iType * 9 + resultType.jiangType.iValue - 1;
		if (iJiangCardId >= 0 && iJiangCardId < 27)
			iValueCnt[iJiangCardId] += 2;

		// 删除最后一张牌
		int iLastCardId = resultType.lastCard.iType * 9 + resultType.lastCard.iValue - 1;
		if (iLastCardId >= 0 && iLastCardId < 27)
			iValueCnt[iLastCardId] = 0;

		for (int i = 0; i < 27; i++)
		{
			if (iValueCnt[i] == 4)
				iTotalGenCnt++;
		}

		if (iTotalGenCnt > 0)
		{
			if (cResult[LONG_QI_DUI] > 0)
				iTotalGenCnt--;
		}

		int iTotalGenFan = 0;
		if (iTotalGenCnt > 0)
		{
			iTotalGenFan = 1;
			for (int i = 0; i < iTotalGenCnt; i++)
			{
				iTotalGenFan *= 2;
			}
		}

		//if(iTotalGenCnt > 0)
		//	_log(_DEBUG, "JudgeResult", "JudgeGenFan_Log() iTotalGenCnt[%d], iTotalGenFan[%d]", iTotalGenCnt, iTotalGenFan);
		cResult[MJ_GEN] = iTotalGenFan;
	}
}

// 红中血流麻将胡牌判断
/*
@param iAllCard:胡牌玩家的手牌，包括红中
@param resultType:玩家吃碰杠信息
@param iQueType: 玩家定缺的类型
@return cFanResult:胡牌番型
@return iTotalFan:胡牌总番数
*/
int JudgeResult::JudgeHZXLMJHu(int iAllCard[5][10], ResultTypeDef &resultType, int iQueType, char *cFanResult)
{
	if (iAllCard[iQueType][0] > 0)
		return 0;

	int iTotalFan = 0;						// 胡牌的总番数
	int iTotalCardNum = 0;					// 手牌总张数
	int iHZCardNum = iAllCard[4][1];		// 红中牌张数
	int iAllNormalCard[3][10] = { 0 };
	int iTotalNormalNum = 0;

	for (int i = 0; i < 3; i++)
	{
		iAllNormalCard[i][0] = iAllCard[i][0];
		int iTypeCardNum = 0;
		for (int j = 1; j < 10; j++)
		{
			iTypeCardNum += iAllCard[i][j];         //万条饼类型的牌各有多少张
			iTotalCardNum += iAllCard[i][j];        //牌共有多少张
			iAllNormalCard[i][j] = iAllCard[i][j];
		}
		if (iTypeCardNum != iAllCard[i][0])
		{
			_log(_ERROR, "JudgeResult", "JudgeHZXLMJHu_Log() 花色牌数量不对 iTypeCardNum = [%d]   iAllCard[%d][0]= [%d] ", iTypeCardNum, i, iAllCard[i][0]);
			return 0;
		}
	}
	iTotalNormalNum = iTotalCardNum;
	iTotalCardNum += iHZCardNum;

	m_iRecursionCnt = 0;

	//_log(_DEBUG, "JudgeResult", "JudgeHZXLMJHu_Log() iQueType[%d], iHZCardNum[%d], iTotalCardNum[%d]", iQueType, iHZCardNum, iTotalCardNum);

	//只有万、筒、条，共108张，红中另外处理。
	int iKindOfCard = 0;
	for (int i = 0; i < 3; i++)
	{
		if (iAllCard[i][0] > 0) //有某个类型的牌的数量大于0  
		{
			iKindOfCard++;
		}
	}
	if (iKindOfCard == 3)  //有3种花色不能胡
	{
		_log(_ERROR, "[JudgeResult]", "JudgeHZXLMJHu_Log() iKindOfCard == 3 有3种花色不能胡");
		return 0;
	}

	if (iTotalCardNum != 2 && iTotalCardNum != 5 && iTotalCardNum != 8 && iTotalCardNum != 11 && iTotalCardNum != 14)
	{
		_log(_ERROR, "JudgeResult", "JudgeHZXLMJHu_Log() Total Card Num:  iTotalCardNum = %d, iHZCardNum[%d]", iTotalCardNum, iHZCardNum);
		return 0;
	}

	// 手牌ID
	int iTempAllCard[3][10] = { 0 };
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			iTempAllCard[i][j] = iAllNormalCard[i][j];
		}
	}

	// 遍历所有可能做将牌的牌
	ResultTypeDef originalResultType;
	memset(&originalResultType, 0, sizeof(ResultTypeDef));
	memcpy(&originalResultType, &resultType, sizeof(ResultTypeDef));
	// 1. 判定没有红中牌的情况下是否可以胡牌(不包括胡七对)
	if (iHZCardNum == 0)
	{
		ResultTypeDef TempResultType1;
		memset(&TempResultType1, 0, sizeof(ResultTypeDef));
		memcpy(&TempResultType1, &originalResultType, sizeof(ResultTypeDef));

		int iTempAllCard1[3][10] = { 0 };
		for (int m = 0; m < 3; m++)
		{
			for (int n = 0; n < 10; n++)
			{
				iTempAllCard1[m][n] = iTempAllCard[m][n];
			}
		}

		if (JudgeNormalHu(iTempAllCard1, TempResultType1))
		{
			// 计算番型
			if (cFanResult[SU_HU] != -1)
				cFanResult[SU_HU] = 2;
			if (cFanResult[MJ_PING_HU] != -1)
				cFanResult[MJ_PING_HU] = 1;

			int iFan = JudgeMJFan(iTempAllCard1, TempResultType1, cFanResult);
			if (iFan > iTotalFan)
				iTotalFan = iFan;

			//_log(_DEBUG, "JudgeResult", "JudgeHZXLMJHu_Log() Success iQueType[%d], iFan[%d],iTotalFan[%d]", iQueType, iFan, iTotalFan);
		}
	}

	// 2. 考虑带红中的胡牌
	if (iHZCardNum > 0)
	{
		ResultTypeDef TempResultType2;
		memset(&TempResultType2, 0, sizeof(ResultTypeDef));
		memcpy(&TempResultType2, &originalResultType, sizeof(ResultTypeDef));

		char cNormalHZFanResult[MJ_FAN_TYPE_CNT] = { 0 };
		int iNormalFan = JudgeNormalHongZhongHu(iTempAllCard, TempResultType2, iHZCardNum, iQueType, cNormalHZFanResult);
		if (iNormalFan > iTotalFan)
		{
			iTotalFan = iNormalFan;
			memcpy(cFanResult, cNormalHZFanResult, sizeof(cNormalHZFanResult));
			if (cFanResult[MJ_PING_HU] != -1)
				cFanResult[MJ_PING_HU] = 1;
		}
		cFanResult[SU_HU] = -1;
	}

	iTotalFan = CalculateTotalFan(cFanResult);

	// 3.考虑特殊胡牌，这里只需要考虑与胡七对相关的番型
	char cQiDuiFanResult[MJ_FAN_TYPE_CNT] = { 0 };
	memset(cQiDuiFanResult, 0, sizeof(cQiDuiFanResult));
	int iAallQiDuiCard[3][10] = { 0 };
	for (int m = 0; m < 3; m++)
	{
		for (int n = 0; n < 10; n++)
		{
			iAallQiDuiCard[m][n] = iAllNormalCard[m][n];
		}
	}
	ResultTypeDef qiDuiResultType;
	memset(&qiDuiResultType, 0, sizeof(ResultTypeDef));
	memcpy(&qiDuiResultType, &originalResultType, sizeof(ResultTypeDef));
	int iSpecialFan = JudgeSpecialHu(iAallQiDuiCard, qiDuiResultType, iHZCardNum, iQueType, cQiDuiFanResult);
	if (iSpecialFan > 0)
	{
		cQiDuiFanResult[MJ_PING_HU] = -1;
		if (iHZCardNum > 0)
			cQiDuiFanResult[SU_HU] = -1;
		else
			cQiDuiFanResult[SU_HU] = 2;

		cQiDuiFanResult[BAI_WAN_DAN] = -1;
	}

	int iTotalQiDuiFan = CalculateTotalFan(cQiDuiFanResult);
	if (iTotalQiDuiFan > iTotalFan)
	{
		memset(cFanResult, 0, sizeof(cFanResult));
		memcpy(cFanResult, cQiDuiFanResult, sizeof(cQiDuiFanResult));
	}

	// 最后计算胡的番数
	iTotalFan = CalculateTotalFan(cFanResult);
	if (iTotalFan > 0)
	{
		// 最后再算根番
		JudgeSpecialGenFan(iAllCard, originalResultType, cFanResult);
		iTotalFan = CalculateTotalFan(cFanResult);
	}

	if(m_iRecursionCnt > RECURSION_MAX_CNT)
		_log(_ERROR, "JudgeResult", "JudgeHZXLMJHu_Log()  End m_iRecursionCnt[%d], iTotalFan[%d]", m_iRecursionCnt, iTotalFan);

	m_iRecursionCnt = 0;

	return iTotalFan;
}

// 判断带红中的胡牌
/*
@param iAllCard:玩家手牌，不带红中
@param resultType:玩家吃碰杠信息
@param iHZCardNum:玩家手牌中红中牌的张数
@param iQueType:玩家定缺的花色
@return cFanResult:最大番数对应的番型
@return iMaxFan:最大番数
*/
int JudgeResult::JudgeNormalHongZhongHu(int iAllCard[3][10], ResultTypeDef &resultType, int iHZCardNum, int iQueType, char *cFanResult)
{
	char cFinalFanResult[MJ_FAN_TYPE_CNT] = { 0 };		// 最终的到的番数最大对应的番型
	char cTmpFanResult[MJ_FAN_TYPE_CNT] = { 0 };				// 计算过程中使用的临时番型
	int iMaxFan = 0;
	ResultTypeDef TempResultType = resultType;
	ResultTypeDef finalResultType;
	memset(&finalResultType, 0, sizeof(finalResultType));

	int iLastCard = resultType.lastCard.iType * 16 + resultType.lastCard.iValue;

	// 1. 使用手牌做将牌，分为使用一对牌，或者使用一张单牌+红中牌
	vector<int> vcJiangCard;
	for (int i = 0; i < 3; i++)
	{
		if (iQueType == i)
			continue;
		for (int j = 1; j < 10; j++)
		{
			// 1. 对于普通牌中每个可以做将牌的对子，判断胡牌
			if (iAllCard[i][j] >= 2)                       // 相同的牌，可以考虑做将牌
			{
				int iTempCard[3][10] = { 0 };
				for (int m = 0; m < 3; m++)
				{
					for (int n = 0; n < 10; n++)
					{
						iTempCard[m][n] = iAllCard[m][n];
					}
				}
				iTempCard[i][j] -= 2;		// 删除手牌中的将牌
				iTempCard[i][0] -= 2;

				ResultTypeDef TempColorResultType;
				memset(&TempColorResultType, 0, sizeof(ResultTypeDef));
				TempColorResultType = TempResultType;
				memset(cTmpFanResult, 0, sizeof(cTmpFanResult));

				int iColorFan = JudgeHZCardHu(iTempCard, TempColorResultType, iHZCardNum, i, j, cTmpFanResult);
				if (iColorFan > iMaxFan)
				{
					iMaxFan = iColorFan;
					memcpy(cFinalFanResult, cTmpFanResult, sizeof(cTmpFanResult));
					finalResultType = TempColorResultType;
				}
			}

			// 2. 使用一张红中搭配一张单牌做将牌的情况
			int iNowCard = i * 16 + j;
			bool bHongZhongJiang = false;
			if (iNowCard == iLastCard)	// 除红中牌外，最后一张牌不可以和红中牌构成将牌	 && resultType.lastCard.bOther == true
				bHongZhongJiang = true;
			if (iAllCard[i][j] >= 1 && iHZCardNum >= 1 && bHongZhongJiang == false)
			{
				int iTempCard[3][10] = { 0 };
				for (int m = 0; m < 3; m++)
				{
					for (int n = 0; n < 10; n++)
					{
						iTempCard[m][n] = iAllCard[m][n];
					}
				}

				iTempCard[i][j] -= 1;
				iTempCard[i][0] -= 1;

				//判断每个花色是否可以组成顺子 刻子
				int iTempHZCardNum = iHZCardNum - 1;

				ResultTypeDef TempColorResultType;
				memset(&TempColorResultType, 0, sizeof(ResultTypeDef));
				TempColorResultType = TempResultType;
				memset(cTmpFanResult, 0, sizeof(cTmpFanResult));

				int iColorFan = 0;
				if (iTempHZCardNum > 0)
					iColorFan = JudgeHZCardHu(iTempCard, TempColorResultType, iTempHZCardNum, i, j, cTmpFanResult);
				else				//红中牌在将牌中用完了
				{
					iColorFan = JudgeNormalHuWithoutJiang(iTempCard, TempColorResultType, j, i, cTmpFanResult);
				}

				if (iColorFan > iMaxFan)
				{
					iMaxFan = iColorFan;
					memcpy(cFinalFanResult, cTmpFanResult, sizeof(cTmpFanResult));
					finalResultType = TempColorResultType;
				}
			}
		}
	}

	// 2. 如果红中牌张数 >= 2, 需要考虑红中牌做将牌的情况,如果还有红中剩余，不需要再考虑红中(两张红中不能凑成刻子或者顺子)
	if (iHZCardNum >= 2)
	{
		//判断每个花色是否可以组成顺子 刻子
		int iTempHZCardNum = iHZCardNum - 2;
		bool bAnalyzeSuccess = false;

		ResultTypeDef TempResultType1;
		memset(&TempResultType1, 0, sizeof(ResultTypeDef));
		TempResultType1 = TempResultType;

		int iTempCard1[3][10] = { 0 };
		for (int m = 0; m < 3; m++)
		{
			for (int n = 0; n < 10; n++)
			{
				iTempCard1[m][n] = iAllCard[m][n];
			}
		}

		for (int m = 0; m < 3; m++)
		{
			if (iQueType == m)
				continue;

			for (int n = 1; n < 10; n++)
			{
				int iColorFan = 0;

				//_log(_DEBUG, "JudgeResult", "JudgeNormalHongZhongHu() 22 i[%d]j[%d] iChiNum[%d], iPengNum[%d]", m, n, TempResultType1.iChiNum, TempResultType1.iPengNum);
				memset(&TempResultType1, 0, sizeof(ResultTypeDef));
				TempResultType1 = TempResultType;
				if (iTempHZCardNum > 0)
					iColorFan = JudgeHZCardHu(iTempCard1, TempResultType1, iTempHZCardNum, m, n, cTmpFanResult);
				else				//红中牌在将牌中用完了
				{
					iColorFan = JudgeNormalHuWithoutJiang(iTempCard1, TempResultType1, n, m, cTmpFanResult);
				}

				if (iColorFan > iMaxFan)
				{
					iMaxFan = iColorFan;
					memcpy(cFinalFanResult, cTmpFanResult, sizeof(cTmpFanResult));
					finalResultType = TempResultType1;
				}
			}
		}
	}

	if (iMaxFan > 0)
	{
		memcpy(cFanResult, cFinalFanResult, sizeof(cFinalFanResult));
		memcpy(&resultType, &finalResultType, sizeof(finalResultType));
	}

	return iMaxFan;
}

// 分解带红中的牌
int JudgeResult::JudgeHZCardHu(int iTempCard[3][10], ResultTypeDef TempResultType, int iHZCardNum, const int i, const int j, char *cFanResult)
{
	char cTmpFanResult[MJ_FAN_TYPE_CNT];				// 计算过程中使用的临时番型
	memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
	int iTempHZCardNum = iHZCardNum;
	bool bAnalyzeSuccess = false;
	int iTotalFan = 0;
	int iColorFan = 0;

	char cFinalFan[MJ_FAN_TYPE_CNT];
	memset(cFinalFan, 0, sizeof(cFinalFan));
	ResultTypeDef resultType1;
	memset(&resultType1, 0, sizeof(ResultTypeDef));
	resultType1 = TempResultType;

	vector<int> viHZCardIndex;
	if (iTempHZCardNum > 0)
	{
		viHZCardIndex.push_back(1);			// 红中做首部
		viHZCardIndex.push_back(3);			// 红中做尾部
		if (iTempHZCardNum > 1)
		{
			viHZCardIndex.push_back(2);		// 红中做顺子中间
		}
	}

	bool bLastHZCard = false;		// 最后 一张牌是否是红中牌
	int iLastCard = resultType1.lastCard.iType * 16 + resultType1.lastCard.iValue;
	if (iLastCard == 65)
		bLastHZCard = true;

	// 红中凑成的顺子首尾都要计算一次
	for (int t = 0; t < viHZCardIndex.size(); t++)
	{
		ResultTypeDef TempColorResultType;
		memset(&TempColorResultType, 0, sizeof(ResultTypeDef));
		TempColorResultType = TempResultType;
		int iIndex = viHZCardIndex[t];
		bool bCanHu = true;
		iTempHZCardNum = iHZCardNum;

		vector<int> viHZCardTransID;			// 红中牌替代的牌

		//_log(_DEBUG, "JudgeResult", "JudgeHZCardHu, 1 iTempHZCardNum[%d], t[%d]", iTempHZCardNum, t);
		// 先刻后顺 判断每个花色是否可以组成顺子，刻子  
		for (int k = 0; k < 3; k++)
		{
			int iTempColorCard[3][10] = { 0 };
			for (int m = 0; m < 3; m++)
			{
				for (int n = 0; n < 10; n++)
				{
					iTempColorCard[m][n] = iTempCard[m][n];
				}
			}

			bool bResult = AnalyzeHongZhongCard(iTempColorCard[k], k, TempColorResultType, iTempHZCardNum, iIndex, viHZCardTransID);
			//_log(_DEBUG, "JudgeResult", "JudgeHZCardHu, 1 bResult[%d]k[%d]iIndex[%d]", bResult, k, iIndex);
			if (!bResult)
			{
				bCanHu = false;
				break;
			}
		}

		// 先刻后顺能胡牌
		if (bCanHu && iTempHZCardNum == 0)
		{
			TempColorResultType.jiangType.iType = i;
			TempColorResultType.jiangType.iValue = j;
			// 算番
			memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
			cTmpFanResult[MJ_PING_HU] = 1;
			iColorFan = JudgeMJFan(iTempCard, TempColorResultType, cTmpFanResult);				// 算番，这里不需要计算七对相关的番型
			if (iColorFan > iTotalFan)
			{
				iTotalFan = iColorFan;
				memcpy(&resultType1, &TempColorResultType, sizeof(TempColorResultType));
				memcpy(cFinalFan, cTmpFanResult, sizeof(cTmpFanResult));
			}

			if (bLastHZCard && viHZCardTransID.size() > 0)
			{
				for (int k = 0; k < viHZCardTransID.size(); k++)
				{
					TempColorResultType.lastCard.iType = viHZCardTransID[k] >> 4;
					TempColorResultType.lastCard.iValue = viHZCardTransID[k] & 0xf;

					memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
					cTmpFanResult[MJ_PING_HU] = 1;
					iColorFan = JudgeMJFan(iTempCard, TempColorResultType, cTmpFanResult);				// 算番，这里不需要计算七对相关的番型
					if (iColorFan > iTotalFan)
					{
						iTotalFan = iColorFan;
						memcpy(&resultType1, &TempColorResultType, sizeof(TempColorResultType));
						memcpy(cFinalFan, cTmpFanResult, sizeof(cTmpFanResult));
					}
				}
			}
		}
		vector<int>().swap(viHZCardTransID);

		// 先顺后刻
		bCanHu = true;
		iTempHZCardNum = iHZCardNum;
		memset(&TempColorResultType, 0, sizeof(ResultTypeDef));
		TempColorResultType = TempResultType;

		//判断每个花色是否可以组成顺子 刻子  先顺后刻
		for (int k = 0; k < 3; k++)
		{
			int iTempColorCard[3][10] = { 0 };
			for (int m = 0; m < 3; m++)
			{
				for (int n = 0; n < 10; n++)
				{
					iTempColorCard[m][n] = iTempCard[m][n];
				}
			}

			bool bResult = AnalyzeHongZhongCard2(iTempColorCard[k], k, TempColorResultType, iTempHZCardNum, iIndex, viHZCardTransID);
			//_log(_DEBUG, "JudgeResult", "JudgeHZCardHu, 2 bResult[%d]k[%d]iIndex[%d]", bResult, k, iIndex);
			if (!bResult)
			{
				bCanHu = false;
				break;
			}
		}

		// 先顺后刻能胡牌
		if (bCanHu && iTempHZCardNum == 0)
		{
			TempColorResultType.jiangType.iType = i;
			TempColorResultType.jiangType.iValue = j;
			// 算番
			memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
			cTmpFanResult[MJ_PING_HU] = 1;
			iColorFan = JudgeMJFan(iTempCard, TempColorResultType, cTmpFanResult);				// 算番，这里不需要计算七对相关的番型
			if (iColorFan > iTotalFan)
			{
				iTotalFan = iColorFan;
				memcpy(&resultType1, &TempColorResultType, sizeof(TempColorResultType));
				memcpy(cFinalFan, cTmpFanResult, sizeof(cTmpFanResult));
			}

			if (bLastHZCard && viHZCardTransID.size() > 0)
			{
				for (int k = 0; k < viHZCardTransID.size(); k++)
				{
					TempColorResultType.lastCard.iType = viHZCardTransID[k] >> 4;
					TempColorResultType.lastCard.iValue = viHZCardTransID[k] & 0xf;

					memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
					cTmpFanResult[MJ_PING_HU] = 1;
					iColorFan = JudgeMJFan(iTempCard, TempColorResultType, cTmpFanResult);				// 算番，这里不需要计算七对相关的番型
					if (iColorFan > iTotalFan)
					{
						iTotalFan = iColorFan;
						memcpy(&resultType1, &TempColorResultType, sizeof(TempColorResultType));
						memcpy(cFinalFan, cTmpFanResult, sizeof(cTmpFanResult));
					}
				}
			}
		}
		vector<int>().swap(viHZCardTransID);
	}

	if (iTotalFan > 0)	// 说明手牌可以凑成3n+2的格式
	{
		memcpy(&TempResultType, &resultType1, sizeof(resultType1));
		memcpy(cFanResult, cFinalFan, sizeof(cFinalFan));
		//else if (iTempHZCardNum == 3)
		//{

		//}
	}

	return iTotalFan;
}

// 分解带红中的牌为刻子和顺子：先刻后顺
/*
index:当前红中牌凑顺子时红中牌的序号
*/
bool JudgeResult::AnalyzeHongZhongCard(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int iIndex, vector<int>& viTransHZCards)
{
	m_iRecursionCnt = m_iRecursionCnt + 1;
	if (m_iRecursionCnt > RECURSION_MAX_CNT)
	{
		_log(_ERROR, "JudgeResult", "AnalyzeHongZhongCard_ m_iReCnt[%d] cType[%d]iHZCNum[%d]iIndex[%d]", m_iRecursionCnt, colorType, iHZCardNum, iIndex);
		return false;
	}
	bool result = false;
	result = AnalyzeHongZhongCardKe(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);
	if (result == false)
	{
		result = AnalyzeHongZhongCardShun(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);
	}

	return result;
}

// 先顺后刻
bool JudgeResult::AnalyzeHongZhongCard2(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int iIndex, vector<int>& viTransHZCards)
{
	m_iRecursionCnt = m_iRecursionCnt + 1;
	if (m_iRecursionCnt > RECURSION_MAX_CNT)
	{
		_log(_ERROR, "JudgeResult", "AnalyzeHongZhongCard2_ m_iReCnt[%d] cType[%d]iHZCNum[%d]iIndex[%d]", m_iRecursionCnt, colorType, iHZCardNum, iIndex);
		return false;
	}

	bool result = false;
	result = AnalyzeHongZhongCardShun2(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);
	if (result == false)
	{
		result = AnalyzeHongZhongCardKe(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);
	}

	return result;
}

// 分解手牌中的刻子
/*
@param iColorCard:某一花色的手牌ID
@param colorType:要分解的花色
@param resultType: 吃碰杠牌
@param iHZCardNum:红中牌的张数
*/
bool JudgeResult::AnalyzeHongZhongCardKe(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int index, vector<int>& viTransHZCards)
{
	int j = 0;
	if (iColorCard[0] == 0)
	{
		return true;
	}
	//寻找第一张牌
	for (j = 1; j < 10; j++)
	{
		if (iColorCard[j] != 0)
		{
			break;
		}
	}
	bool result = false;

	// 正常的刻子
	if (iColorCard[j] >= 3)
	{
		//除去这3张刻牌
		iColorCard[j] -= 3;
		iColorCard[0] -= 3;

		result = AnalyzeHongZhongCard(iColorCard, colorType, resultType, iHZCardNum, index, viTransHZCards);

		//还原这3张刻牌
		iColorCard[j] += 3;
		iColorCard[0] += 3;

		if (resultType.iPengNum > 3)
			return false;

		if (result)
		{
			resultType.pengType[resultType.iPengNum].iType = colorType;
			resultType.pengType[resultType.iPengNum].iValue = j;
			resultType.iPengNum++;
		}

		return result;
	}

	// 对子，红中补一张构成刻子
	if (iColorCard[j] == 2 && iHZCardNum >= 1)
	{
		//除去这2张刻牌
		iColorCard[j] -= 2;
		iColorCard[0] -= 2;
		iHZCardNum -= 1;
		result = AnalyzeHongZhongCard(iColorCard, colorType, resultType, iHZCardNum, index, viTransHZCards);

		//还原这2张刻牌
		iColorCard[j] += 2;
		iColorCard[0] += 2;

		if (resultType.iPengNum > 3)
			return false;

		if (result)		// 还原使用的红中牌
		{
			resultType.pengType[resultType.iPengNum].iType = colorType;
			resultType.pengType[resultType.iPengNum].iValue = j;
			resultType.iPengNum++;
		}
		else
		{
			iHZCardNum += 1;
		}

		return result;
	}

	// 单牌，红中补2张构成刻子
	if (iColorCard[j] == 1 && iHZCardNum >= 2)
	{
		//除去这张刻牌
		iColorCard[j] -= 1;
		iColorCard[0] -= 1;

		iHZCardNum -= 2;
		result = AnalyzeHongZhongCard(iColorCard, colorType, resultType, iHZCardNum, index, viTransHZCards);

		//还原这张刻牌
		iColorCard[j] += 1;
		iColorCard[0] += 1;

		if (resultType.iPengNum > 3)
			return false;

		if (result)
		{
			resultType.pengType[resultType.iPengNum].iType = colorType;
			resultType.pengType[resultType.iPengNum].iValue = j;
			resultType.iPengNum++;
		}
		else
		{
			iHZCardNum += 2;
		}

		return result;
	}

	return result;
}

// 分解手牌中的顺子
bool JudgeResult::AnalyzeHongZhongCardShun(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int iIndex, vector<int>& viTransHZCards)
{
	int j = 0;
	if (iColorCard[0] == 0)
	{
		return true;
	}

	//寻找第一张牌
	for (j = 1; j < 10; j++)
	{
		if (iColorCard[j] != 0)
		{
			break;
		}
	}
	bool result = false;

	//正常的顺子
	if ((j < 8) && (iColorCard[j + 1] > 0) && (iColorCard[j + 2] > 0))
	{
		//除去这3张顺牌
		iColorCard[j]--;
		iColorCard[j + 1]--;
		iColorCard[j + 2]--;
		iColorCard[0] -= 3;

		result = AnalyzeHongZhongCard(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);

		//还原这3张顺牌
		iColorCard[j]++;
		iColorCard[j + 1]++;
		iColorCard[j + 2]++;
		iColorCard[0] += 3;

		if (resultType.iChiNum > 3)
			return false;

		if (result)
		{
			resultType.chiType[resultType.iChiNum].iType = colorType;
			resultType.chiType[resultType.iChiNum].iFirstValue = j;
			resultType.iChiNum++;
		}

		return result;
	}

	// 连对，红中补一张构成顺子
	if ((j <= 8) && iColorCard[j + 1] > 0 && iColorCard[j + 2] == 0 && iHZCardNum >= 1)
	{
		//除去这3张顺牌
		iColorCard[j]--;
		iColorCard[j + 1]--;
		iColorCard[0] -= 2;
		iHZCardNum -= 1;

		result = AnalyzeHongZhongCard(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);

		//还原这3张顺牌
		iColorCard[j]++;
		iColorCard[j + 1]++;
		iColorCard[0] += 2;

		if (resultType.iChiNum > 3)
			return false;

		if (result)
		{
			if (iIndex == 1 && j > 2)
			{
				resultType.chiType[resultType.iChiNum].iType = colorType;
				resultType.chiType[resultType.iChiNum].iFirstValue = j - 1;
				int iTransCardId = colorType << 4 | j - 1;
				viTransHZCards.push_back(iTransCardId);
				resultType.iChiNum++;
			}
			else if (j <= 7)
			{
				resultType.chiType[resultType.iChiNum].iType = colorType;
				resultType.chiType[resultType.iChiNum].iFirstValue = j;
				int iTransCardId = colorType << 4 | j + 2;
				viTransHZCards.push_back(iTransCardId);
				resultType.iChiNum++;
			}
			else
			{
				iHZCardNum += 1;
			}
		}
		else
		{
			iHZCardNum += 1;
		}

		return result;
	}

	// 坎对，红中补一张构成顺子
	if ((j < 8) && iColorCard[j + 1] == 0 && iColorCard[j + 2] > 0 && iHZCardNum >= 1)
	{
		//除去这3张顺牌
		iColorCard[j]--;
		iColorCard[j + 2]--;
		iColorCard[0] -= 2;

		iHZCardNum -= 1;

		result = AnalyzeHongZhongCard(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);

		//还原这3张顺牌
		iColorCard[j]++;
		iColorCard[j + 2]++;
		iColorCard[0] += 2;

		if (resultType.iChiNum > 3)
			return false;

		if (result)
		{
			resultType.chiType[resultType.iChiNum].iType = colorType;
			resultType.chiType[resultType.iChiNum].iFirstValue = j;
			resultType.iChiNum++;

			int iTransCardId = colorType << 4 | j + 1;
			viTransHZCards.push_back(iTransCardId);
		}
		else
		{
			iHZCardNum += 1;
		}

		return result;
	}

	return false;
}

// 分解手牌中的顺子
bool JudgeResult::AnalyzeHongZhongCardShun2(int iColorCard[], int colorType, ResultTypeDef &resultType, int &iHZCardNum, int iIndex, vector<int>& viTransHZCards)
{
	int j = 0;
	if (iColorCard[0] == 0)
	{
		return true;
	}

	//寻找第一张牌
	for (j = 1; j < 10; j++)
	{
		if (iColorCard[j] != 0)
		{
			break;
		}
	}
	bool result = false;

	//正常的顺子
	if ((j < 8) && (iColorCard[j + 1] > 0) && (iColorCard[j + 2] > 0))
	{
		//除去这3张顺牌
		iColorCard[j]--;
		iColorCard[j + 1]--;
		iColorCard[j + 2]--;
		iColorCard[0] -= 3;

		result = AnalyzeHongZhongCard2(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);

		//还原这3张顺牌
		iColorCard[j]++;
		iColorCard[j + 1]++;
		iColorCard[j + 2]++;
		iColorCard[0] += 3;

		if (resultType.iChiNum > 3)
			return false;

		if (result)
		{
			resultType.chiType[resultType.iChiNum].iType = colorType;
			resultType.chiType[resultType.iChiNum].iFirstValue = j;
			resultType.iChiNum++;
		}

		return result;
	}

	// 连对，红中补一张构成顺子
	if ((j <= 8) && iColorCard[j + 1] > 0 && (j < 8 && iColorCard[j + 2] == 0 || j == 8) && iHZCardNum >= 1)
	{
		//除去这3张顺牌
		iColorCard[j]--;
		iColorCard[j + 1]--;
		iColorCard[0] -= 2;
		iHZCardNum -= 1;

		result = AnalyzeHongZhongCard2(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);

		//还原这2张顺牌
		iColorCard[j]++;
		iColorCard[j + 1]++;
		iColorCard[0] += 2;

		if (resultType.iChiNum > 3)
			return false;

		if (result)
		{
			if (iIndex == 1 && j > 2)
			{
				resultType.chiType[resultType.iChiNum].iType = colorType;
				resultType.chiType[resultType.iChiNum].iFirstValue = j - 1;
				int iTransCardId = colorType << 4 | j - 1;
				viTransHZCards.push_back(iTransCardId);
				resultType.iChiNum++;
			}
			else if (j <= 7)
			{
				resultType.chiType[resultType.iChiNum].iType = colorType;
				resultType.chiType[resultType.iChiNum].iFirstValue = j;
				int iTransCardId = colorType << 4 | j + 2;
				viTransHZCards.push_back(iTransCardId);
				resultType.iChiNum++;
			}
			else
			{
				iHZCardNum += 1;
			}
		}
		else
		{
			iHZCardNum += 1;
		}

		return result;
	}

	// 坎对，红中补一张构成顺子
	if ((j < 8) && iColorCard[j + 1] == 0 && iColorCard[j + 2] > 0 && iHZCardNum >= 1)
	{
		//除去这3张顺牌
		iColorCard[j]--;
		iColorCard[j + 2]--;
		iColorCard[0] -= 2;

		iHZCardNum -= 1;

		result = AnalyzeHongZhongCard2(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);

		//还原这3张顺牌
		iColorCard[j]++;
		iColorCard[j + 2]++;
		iColorCard[0] += 2;

		if (resultType.iChiNum > 3)
			return false;

		if (result)
		{
			resultType.chiType[resultType.iChiNum].iType = colorType;
			resultType.chiType[resultType.iChiNum].iFirstValue = j;
			resultType.iChiNum++;

			int iTransCardId = colorType << 4 | j + 1;
			viTransHZCards.push_back(iTransCardId);
		}
		else
		{
			iHZCardNum += 1;
		}

		return result;
	}

	// 单张顺牌，可能凑成三个顺子
	if (iColorCard[j] == 1 && (j > 1 && iColorCard[j - 1] == 0) && (j > 2 && iColorCard[j - 2] == 0) && (j < 9 && iColorCard[j + 1] == 0)
		&& (j < 8 && iColorCard[j + 2] == 0) && iHZCardNum >= 2)
	{
		//除去这张顺牌
		iColorCard[j]--;
		iColorCard[0] -= 1;
		iHZCardNum -= 2;

		result = AnalyzeHongZhongCard2(iColorCard, colorType, resultType, iHZCardNum, iIndex, viTransHZCards);

		//还原这张顺牌
		iColorCard[j]++;
		iColorCard[0] += 1;

		if (resultType.iChiNum > 3)
			return false;

		if (result)
		{
			if (iIndex == 2 && j > 1)
			{
				resultType.chiType[resultType.iChiNum].iType = colorType;
				resultType.chiType[resultType.iChiNum].iFirstValue = j - 1;

				int iTransCardId1 = colorType << 4 | j - 1;
				int iTransCardId2 = colorType << 4 | j + 1;
				viTransHZCards.push_back(iTransCardId1);
				viTransHZCards.push_back(iTransCardId2);
				resultType.iChiNum++;
			}
			else if (iIndex == 3 && j > 2)
			{
				resultType.chiType[resultType.iChiNum].iType = colorType;
				resultType.chiType[resultType.iChiNum].iFirstValue = j - 2;

				int iTransCardId1 = colorType << 4 | j - 2;
				int iTransCardId2 = colorType << 4 | j - 1;
				viTransHZCards.push_back(iTransCardId1);
				viTransHZCards.push_back(iTransCardId2);
				resultType.iChiNum++;
			}
			else if (j <= 7)
			{
				resultType.chiType[resultType.iChiNum].iType = colorType;
				resultType.chiType[resultType.iChiNum].iFirstValue = j;

				int iTransCardId1 = colorType << 4 | j + 1;
				int iTransCardId2 = colorType << 4 | j + 2;
				viTransHZCards.push_back(iTransCardId1);
				viTransHZCards.push_back(iTransCardId2);
				resultType.iChiNum++;
			}
			else
			{
				iHZCardNum += 2;
			}
		}
		else
		{
			iHZCardNum += 2;
		}

		return result;
	}

	return false;
}

// 非正常格式的胡牌(不符合3N+2的胡牌格式)，这里只需要考虑七对相关的番型，根的番型另外单独计算
/*
@param iAllNormalCard:玩家手牌，不包括红中
*/
int JudgeResult::JudgeSpecialHu(int iAllNormalCard[3][10], ResultTypeDef &resultType, int iHZCardNum, int iQueType, char *cFanResult)
{
	//判定7对
	int iTotalFan = 0;
	int iAllCount = 0;
	for (int i = 0; i < 3; i++)
		iAllCount += iAllNormalCard[i][0];

	if (iAllCount + iHZCardNum != 14)		// 不允许有吃碰杠
		return iTotalFan;

	char cTmpFanResult[MJ_FAN_TYPE_CNT] = { 0 };

	if (iHZCardNum == 0)
	{
		bool bOddCnt = false;
		bool bHasLong = false;
		for (int i = 0; i < 3; i++)
		{
			for (int j = 1; j < 10; j++)
			{
				if (iAllNormalCard[i][j] % 2 != 0)
				{
					bOddCnt = true;
					break;
				}
			}
			if (bOddCnt)
				break;
		}

		// 7番，七对：玩家的手牌全部是两张一对的，没有碰过和杠过, 不计不求人、单钓、门清
		if (bOddCnt == false)
		{
			JudgeQiDuiFan(iAllNormalCard, resultType, cTmpFanResult);
		}
	}
	// 存在红中牌，需要凑成七对
	else
	{
		int iTempAllPai[3][10] = { 0 };
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				iTempAllPai[i][j] = iAllNormalCard[i][j];
			}
		}
		int iLeftHZCardNum = iHZCardNum;

		//把所有不成对子的牌 全部补成对子,如果最后一张牌是别人打出来的 并且总数为单数 那么不可以胡牌
		bool bLast = false;		// 红中牌是否用来替代最后一张牌
		for (int i = 0; i < 3; i++)
		{
			for (int j = 1; j < 10; j++)
			{
				if (iTempAllPai[i][j] % 2 != 0)		// 存在单数张牌，考虑用红中补成对子
				{
					if (iLeftHZCardNum > 0)
					{
						if (i == resultType.lastCard.iType && j == resultType.lastCard.iValue)
						{
							if (iTempAllPai[i][j] == 1)			// 红中牌做胡的那张牌时，如果原来手牌中只有一张牌（自摸），或者没有这张牌（点炮），不能单调
								continue;
						}

						iTempAllPai[i][j]++;
						iTempAllPai[i][0]++;
						iLeftHZCardNum--;

						if (i == resultType.lastCard.iType && j == resultType.lastCard.iValue)		// 自摸最后一张牌，并且用红中凑成了对子，算单调将
						{
							bLast = true;
						}
					}
					else
						return iTotalFan;
				}
			}
		}

		// 七对的情况，如果没有剩余红中，说明红中都补给单牌凑成了对子；如果剩余红中，则必然是剩余2张或者4张红中
		if (iLeftHZCardNum > 0 && iLeftHZCardNum % 2 != 0)
			return iTotalFan;

		// 没有红中剩余，直接计算番数
		if (iLeftHZCardNum == 0)
		{
			JudgeQiDuiFan(iTempAllPai, resultType, cTmpFanResult);
		}
		// 还有红中剩余
		else if (iLeftHZCardNum > 0)
		{
			int iMaxFan = 0;
			char cMaxFanResult[MJ_FAN_TYPE_CNT] = { 0 };
			int iTotalCardCnt = CalculateTotalCardCnt(iTempAllPai, iQueType);
			if (iLeftHZCardNum == 2 && iTotalCardCnt == 12)
			{
				for (int i = 0; i < 3; i++)
				{
					if (iQueType == i)
						continue;
					for (int j = 1; j < 9; j++)
					{
						int iQiDuiAllCard[3][10] = { 0 };
						memcpy(iQiDuiAllCard, iTempAllPai, sizeof(iTempAllPai));
						char cQiDuiFanResult[MJ_FAN_TYPE_CNT] = { 0 };

						iQiDuiAllCard[i][j] += 2;
						iQiDuiAllCard[i][0] += 2;

						JudgeQiDuiFan(iQiDuiAllCard, resultType, cQiDuiFanResult);
						int iFan = CalculateTotalFan(cQiDuiFanResult);
						if (iFan > iMaxFan)
						{
							iMaxFan = iFan;
							memcpy(cMaxFanResult, cQiDuiFanResult, sizeof(cQiDuiFanResult));
						}

						iQiDuiAllCard[i][j] -= 2;		// 还原
						iQiDuiAllCard[i][0] -= 2;
					}
				}
			}
			else if (iLeftHZCardNum == 4 && iTotalCardCnt == 10)
			{
				for (int i = 0; i < 3; i++)
				{
					if (iQueType == i)
						continue;
					for (int j = 1; j < 9; j++)
					{
						int iQiDuiAllCard[3][10] = { 0 };
						memcpy(iQiDuiAllCard, iTempAllPai, sizeof(iTempAllPai));
						char cQiDuiFanResult[MJ_FAN_TYPE_CNT] = { 0 };

						iQiDuiAllCard[i][j] += 2;
						iQiDuiAllCard[i][0] += 2;

						for (int k = 0; k < 3; k++)
						{
							for (int m = 1; m < 10; m++)
							{
								iQiDuiAllCard[k][m] += 2;
								iQiDuiAllCard[k][0] += 2;
								JudgeQiDuiFan(iQiDuiAllCard, resultType, cQiDuiFanResult);
								int iFan = CalculateTotalFan(cQiDuiFanResult);
								if (iFan > iMaxFan)
								{
									iMaxFan = iFan;
									memcpy(cMaxFanResult, cQiDuiFanResult, sizeof(cQiDuiFanResult));
								}
								iQiDuiAllCard[k][m] -= 2;
								iQiDuiAllCard[k][0] -= 2;
							}
						}

						iQiDuiAllCard[i][j] -= 2;		// 还原
						iQiDuiAllCard[i][0] -= 2;
					}
				}
			}

			if (iMaxFan > 0)
				memcpy(cTmpFanResult, cMaxFanResult, sizeof(cMaxFanResult));
		}
	}

	iTotalFan = CalculateTotalFan(cTmpFanResult);
	if (iTotalFan > 0)
		memcpy(cFanResult, cTmpFanResult, sizeof(cTmpFanResult));

	return iTotalFan;
}


// 判断守中抱一胡牌
int JudgeResult::JudgeSZBYHu(int iAllCard[5][10], ResultTypeDef &resultType, int iQueType, char *cFanResult)
{
	if (resultType.iChiNum + resultType.iPengNum != 4)
		return 0;

	int iTotalFan = 0;
	int iAllNormalCard[3][10] = { 0 };
	for (int i = 0; i < 3; i++)
	{
		iAllNormalCard[i][0] = iAllCard[i][0];
		for (int j = 1; j < 10; j++)
		{
			iAllNormalCard[i][j] = iAllCard[i][j];
			if (iAllNormalCard[i][j] != 0)
				return 0;
		}
	}

	int iOriginalType = resultType.lastCard.iType;
	int iOrginalValue = resultType.lastCard.iValue;

	// 守中抱一时，红中将牌可以当做任何的将牌
	char cTmpFanResult[MJ_FAN_TYPE_CNT] = { 0 };
	int iTmpFan = 0;
	for (int i = 0; i < 3; i++)
	{
		if (i == iQueType)
			continue;
		for (int j = 1; j < 10; j++)
		{
			memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
			cTmpFanResult[SHOU_ZHONG_BAO_YI] = 24;
			resultType.lastCard.iType = i;
			resultType.lastCard.iValue = j;
			resultType.jiangType.iType = i;
			resultType.jiangType.iValue = j;
			JudgeMJFan(iAllNormalCard, resultType, cTmpFanResult);
			JudgeSpecialGenFan(iAllCard, resultType, cTmpFanResult);
			iTmpFan = CalculateTotalFan(cTmpFanResult);
			if (iTmpFan > iTotalFan)
			{
				iTotalFan = iTmpFan;
				memcpy(cFanResult, cTmpFanResult, sizeof(cTmpFanResult));
				iOriginalType = i;
				iOrginalValue = j;
			}
		}
	}

	resultType.lastCard.iType = iOriginalType;
	resultType.lastCard.iValue = iOrginalValue;
	resultType.jiangType.iType = iOriginalType;
	resultType.jiangType.iValue = iOrginalValue;

	//if (iTotalFan > 0)
	//	_log(_DEBUG, "JudgeResult", "JudgeSZBYHu_Log() iTotalFan[%d], iOriginalType[%d], iOrginalValue[%d]", iTotalFan, iOriginalType, iOrginalValue);

	return iTotalFan;
}

/*
使用手牌判断根番
@param iAllCards:手牌ID
@param resultType:吃碰杠信息
@return cFanResult:胡牌番型
*/
void JudgeResult::JudgeSpecialGenFan(int iAllCards[5][10], ResultTypeDef &resultType, char *cFanResult)
{
	if (cFanResult[MJ_GEN] == -1)
		return;

	int iHZCardNum = iAllCards[4][1];
	int iTotalGenCnt = 0;				// 根的个数
	if (iHZCardNum == 4 && resultType.lastCard.iType < 4)				// 4张红中算根番
	{
		iTotalGenCnt++;
	}

	int iValueCnt[27];
	memset(iValueCnt, 0, sizeof(iValueCnt));

	// 吃牌
	for (int i = 0; i < resultType.iChiNum; i++)
	{
		int iFirstValue = resultType.chiType[i].iFirstValue;
		int iType = resultType.chiType[i].iType;
		int iFirst = iType * 9 + iFirstValue - 1;
		iValueCnt[iFirst]++;
		iValueCnt[iFirst + 1]++;
		iValueCnt[iFirst + 2]++;
	}

	// 碰杠牌
	for (int i = 0; i < resultType.iPengNum; i++)
	{
		int iPengValue = resultType.pengType[i].iValue;
		int iType = resultType.pengType[i].iType;
		int iPeng = iType * 9 + iPengValue - 1;
		iValueCnt[iPeng] += 3;
		if (resultType.pengType[i].iGangType > 0)		// 明杠或者暗杠
		{
			iValueCnt[iPeng]++;
		}
	}

	// 将牌
	int iJiangValue = resultType.jiangType.iValue;
	int iJiangType = resultType.jiangType.iType;
	int iJiangId = iJiangType * 9 + iJiangValue - 1;
	if (iJiangId >= 0 && iJiangId < 27)
		iValueCnt[iJiangId] += 2;
	//else
	//	_log(_DEBUG, "JudgeResult", "JudgeSpecialGenFan_Log() iJiangId[%d]", iJiangId);

	// 手牌
	for (int i = 0; i < 3; i++)
	{
		for (int j = 1; j < 10; j++)
		{
			if (iAllCards[i][j] > 0)
			{
				int iCardId = i * 9 + j - 1;
				if (iCardId >= 0 && iCardId < 27)
					iValueCnt[iCardId]++;
			}
		}
	}

	// 胡的那张牌不能是根(这个限制取消了)
	//int iLastCardId = resultType.lastCard.iType * 9 + resultType.lastCard.iValue - 1;
	//if (iLastCardId >= 0 && iLastCardId < 27)
	//	if(iValueCnt[iLastCardId] < 6)				// 红中将牌加暗杠排除
	//		iValueCnt[iLastCardId] = 0;

	for (int i = 0; i < 27; i++)
	{
		if (iValueCnt[i] == 4 || iValueCnt[i] == 6 || iValueCnt[i] == 8)
			iTotalGenCnt++;
	}

	if (iTotalGenCnt > 0)
	{
		if (cFanResult[LONG_QI_DUI] > 0)
			iTotalGenCnt--;
	}

	int iTotalGenFan = 0;
	if (iTotalGenCnt > 0)
	{
		iTotalGenFan = 1;
		for (int i = 0; i < iTotalGenCnt; i++)
		{
			iTotalGenFan *= 2;
		}
	}

	//if(iTotalGenCnt > 0)
	//	_log(_DEBUG, "JudgeResult", "JudgeSpecialGenFan_Log() iTotalGenCnt[%d], iTotalGenFan[%d]", iTotalGenCnt, iTotalGenFan);
	cFanResult[MJ_GEN] = iTotalGenFan;
}

// 遍历的方式判断带红中的胡牌(平胡，不包括七对)
/*
@param iAllCard:玩家手牌，不带红中
@param resultType:玩家吃碰杠信息
@param iHZCardNum:玩家手牌中红中牌的张数
@param iQueType:玩家定缺的花色
@return cFanResult:最大番数对应的番型
@return iMaxFan:最大番数
*/
int JudgeResult::JudgeNormalHZhuErgodic(int iAllCard[3][10], ResultTypeDef &resultType, int iHZCardNum, int iQueType, char *cFanResult)
{
	char cFinalFanResult[MJ_FAN_TYPE_CNT] = { 0 };				// 最终的到的番数最大对应的番型
	char cTmpFanResult[MJ_FAN_TYPE_CNT] = { 0 };				// 计算过程中使用的临时番型
	int iMaxFan = 0;											// 最终计算得到的最大番数
	ResultTypeDef TempResultType = resultType;
	ResultTypeDef finalResultType;
	memset(&finalResultType, 0, sizeof(ResultTypeDef));

	vector<char > vcAllBaseCards;          //麻将中所有的牌1-9万条饼，排除定缺的花色
	for (int i = 0; i < 3; i++)
	{
		if (i == iQueType)
			continue;
		for (int j = 1; j < 10; j++)
		{
			vcAllBaseCards.push_back((i << 4) | j);
		}
	}

	if (iHZCardNum == 1)
	{
		for (int i = 0; i < vcAllBaseCards.size(); i++)
		{
			int iFirstType = vcAllBaseCards[i] >> 4;
			int iFirstValue = vcAllBaseCards[i] & 0x000f;

			iAllCard[iFirstType][0]++;						// 单类牌的数量
			iAllCard[iFirstType][iFirstValue]++;			// 单张牌的数量

			ResultTypeDef firstResultType = TempResultType;
			bool bNormalHu = JudgeNormalHu(iAllCard, firstResultType);
			if (bNormalHu)
			{
				memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
				cTmpFanResult[MJ_PING_HU] = 1;
				//int iFirstFan = JudgeFan(firstResultType, cTmpFanResult);
				int iFirstFan = JudgeMJFan(iAllCard, firstResultType, cTmpFanResult);
				if (iFirstFan > iMaxFan)
				{
					iMaxFan = iFirstFan;
					finalResultType = firstResultType;
					memcpy(cFinalFanResult, cTmpFanResult, sizeof(cTmpFanResult));
				}
			}

			iAllCard[iFirstType][0]--;
			iAllCard[iFirstType][iFirstValue]--;
		}
	}
	else if (iHZCardNum == 2)
	{
		for (int i = 0; i < vcAllBaseCards.size(); i++)
		{
			int iFirstType = vcAllBaseCards[i] >> 4;
			int iFirstValue = vcAllBaseCards[i] & 0x000f;

			iAllCard[iFirstType][0]++;						// 单类牌的数量
			iAllCard[iFirstType][iFirstValue]++;			// 单张牌的数量

			for (int j = 0; j < vcAllBaseCards.size(); j++)
			{
				int iSecondType = vcAllBaseCards[j] >> 4;
				int iSecondValue = vcAllBaseCards[j] & 0x000f;

				iAllCard[iSecondType][0]++;						// 单类牌的数量
				iAllCard[iSecondType][iSecondValue]++;          // 单张牌的数量

				ResultTypeDef secondResultType = resultType;
				bool bNormalHu = JudgeNormalHu(iAllCard, secondResultType);
				if (bNormalHu)
				{
					memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
					cTmpFanResult[MJ_PING_HU] = 1;
					//int iSecondFan = JudgeFan(secondResultType, cTmpFanResult);
					int iSecondFan = JudgeMJFan(iAllCard, secondResultType, cTmpFanResult);
					if (iSecondFan > iMaxFan)
					{
						iMaxFan = iSecondFan;
						finalResultType = secondResultType;
						memcpy(cFinalFanResult, cTmpFanResult, sizeof(cTmpFanResult));
					}
				}

				iAllCard[iSecondType][0]--;
				iAllCard[iSecondType][iSecondValue]--;
			}

			iAllCard[iFirstType][0]--;
			iAllCard[iFirstType][iFirstValue]--;
		}
	}
	else if (iHZCardNum == 3)
	{
		for (int i = 0; i < vcAllBaseCards.size(); i++)
		{
			int iFirstType = vcAllBaseCards[i] >> 4;
			int iFirstValue = vcAllBaseCards[i] & 0x000f;

			iAllCard[iFirstType][0]++;						// 单类牌的数量
			iAllCard[iFirstType][iFirstValue]++;			// 单张牌的数量

			for (int j = 0; j < vcAllBaseCards.size(); j++)
			{
				int iSecondType = vcAllBaseCards[j] >> 4;
				int iSecondValue = vcAllBaseCards[j] & 0x000f;

				iAllCard[iSecondType][0]++;						// 单类牌的数量
				iAllCard[iSecondType][iSecondValue]++;          // 单张牌的数量

				for (int k = 0; k < vcAllBaseCards.size(); k++)
				{
					int iThridType = vcAllBaseCards[k] >> 4;
					int iThridValue = vcAllBaseCards[k] & 0x000f;

					iAllCard[iThridType][0]++;						// 单类牌的数量
					iAllCard[iThridType][iThridValue]++;			// 单张牌的数量

					ResultTypeDef thridResultType = resultType;
					bool bNormalHu = JudgeNormalHu(iAllCard, thridResultType);
					if (bNormalHu)
					{
						memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
						cTmpFanResult[MJ_PING_HU] = 1;
						//int iThridFan = JudgeFan(thridResultType, cTmpFanResult);
						int iThridFan = JudgeMJFan(iAllCard, thridResultType, cTmpFanResult);
						if (iThridFan > iMaxFan)
						{
							iMaxFan = iThridFan;
							finalResultType = thridResultType;
							memcpy(cFinalFanResult, cTmpFanResult, sizeof(cTmpFanResult));
						}
					}

					iAllCard[iThridType][0]--;
					iAllCard[iThridType][iThridValue]--;
				}

				iAllCard[iSecondType][0]--;
				iAllCard[iSecondType][iSecondValue]--;
			}

			iAllCard[iFirstType][0]--;
			iAllCard[iFirstType][iFirstValue]--;
		}
	}
	else if (iHZCardNum == 4)
	{
		for (int i = 0; i < vcAllBaseCards.size(); i++)
		{
			int iFirstType = vcAllBaseCards[i] >> 4;
			int iFirstValue = vcAllBaseCards[i] & 0x000f;

			iAllCard[iFirstType][0]++;						// 单类牌的数量
			iAllCard[iFirstType][iFirstValue]++;			// 单张牌的数量

			for (int j = 0; j < vcAllBaseCards.size(); j++)
			{
				int iSecondType = vcAllBaseCards[j] >> 4;
				int iSecondValue = vcAllBaseCards[j] & 0x000f;

				iAllCard[iSecondType][0]++;						// 单类牌的数量
				iAllCard[iSecondType][iSecondValue]++;          // 单张牌的数量

				for (int k = 0; k < vcAllBaseCards.size(); k++)
				{
					int iThridType = vcAllBaseCards[k] >> 4;
					int iThridValue = vcAllBaseCards[k] & 0x000f;

					iAllCard[iThridType][0]++;						// 单类牌的数量
					iAllCard[iThridType][iThridValue]++;			// 单张牌的数量

					for (int t = 0; t < vcAllBaseCards.size(); t++)
					{
						int iForthType = vcAllBaseCards[t] >> 4;
						int iForthValue = vcAllBaseCards[t] & 0x000f;

						iAllCard[iForthType][0]++;						// 单类牌的数量
						iAllCard[iForthType][iForthValue]++;			// 单张牌的数量

						ResultTypeDef forthResultType = resultType;
						bool bNormalHu = JudgeNormalHu(iAllCard, forthResultType);
						if (bNormalHu)
						{
							memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
							cTmpFanResult[MJ_PING_HU] = 1;
							//int iForthFan = JudgeFan(forthResultType, cTmpFanResult);
							int iForthFan = JudgeMJFan(iAllCard, forthResultType, cTmpFanResult);
							if (iForthFan > iMaxFan)
							{
								iMaxFan = iForthFan;
								finalResultType = forthResultType;
								memcpy(cFinalFanResult, cTmpFanResult, sizeof(cTmpFanResult));
							}
						}

						iAllCard[iForthType][0]--;
						iAllCard[iForthType][iForthValue]--;
					}

					iAllCard[iThridType][0]--;
					iAllCard[iThridType][iThridValue]--;
				}

				iAllCard[iSecondType][0]--;
				iAllCard[iSecondType][iSecondValue]--;
			}

			iAllCard[iFirstType][0]--;
			iAllCard[iFirstType][iFirstValue]--;
		}
	}

	if (iMaxFan > 0)
	{
		resultType = finalResultType;
		memcpy(cFanResult, cFinalFanResult, sizeof(cFinalFanResult));

		//_log(_DEBUG, "JudgeResult", "JudgeNormalHZhuErgodic_log() iHZCardNum[%d], iMaxFan[%d]", iHZCardNum, iMaxFan);
	}

	return iMaxFan;
}

// 带入手牌，解决单调将判断错误问题
int JudgeResult::JudgeMJFan(int iAllCards[3][10], ResultTypeDef resultType, char *cResult)
{
	int iFan = JudgeFan(resultType, cResult);
	if (cResult[DAN_DIAO_JIANG] == -1)
		return iFan;

	//_log(_DEBUG, "JudgeResult", "JudgeNormalHZhuErgodic_log() cResult[DAN_DIAO_JIANG] = [%d]", cResult[DAN_DIAO_JIANG]);

	int iLastType = resultType.lastCard.iType;
	int iValue = resultType.lastCard.iValue;

	//_log(_DEBUG, "JudgeResult", "JudgeMJFan_log() iLastType[%d], iValue[%d]", iLastType, iValue);
	if (iAllCards[iLastType][iValue] > 2)
		cResult[DAN_DIAO_JIANG] = -1;

	int iTotalFan = CalculateTotalFan(cResult);

	return iTotalFan;
}

// 判断红中做将牌后，红中张数为0的情况下，能否胡牌
int JudgeResult::JudgeNormalHuWithoutJiang(int allPai[3][10], ResultTypeDef &resultType, int iValue, int iType, char *cFanResult)
{
	ResultTypeDef TempColorResultType;
	memset(&TempColorResultType, 0, sizeof(ResultTypeDef));
	int iTotalFan = 0;

	char cFinalFan[MJ_FAN_TYPE_CNT];
	memset(cFinalFan, 0, sizeof(cFinalFan));

	// 先刻后顺
	ResultTypeDef TempColorResultType1;
	memcpy(&TempColorResultType1, &resultType, sizeof(ResultTypeDef));
	int iTempAllPai[3][10] = { 0 };
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			iTempAllPai[i][j] = allPai[i][j];
		}
	}

	for (int i = 0; i < 3; i++)
	{
		if (!Analyze(iTempAllPai[i], false, i, TempColorResultType1))
		{
			return 0;
		}
	}

	TempColorResultType1.jiangType.iType = iType;
	TempColorResultType1.jiangType.iValue = iValue;

	char cTmpFanResult[MJ_FAN_TYPE_CNT] = { 0 };				// 计算过程中使用的临时番型
	memset(cTmpFanResult, 0, sizeof(cTmpFanResult));
	cTmpFanResult[MJ_PING_HU] = 1;
	cTmpFanResult[SU_HU] = -1;
	//_log(_DEBUG, "JudgeResult", "JudgeNormalHuWithoutJiang() 11 iChiNum[%d], iPengNum[%d]", TempColorResultType1.iChiNum, TempColorResultType1.iPengNum);
	iTotalFan = JudgeMJFan(iTempAllPai, TempColorResultType1, cTmpFanResult);				// 算番，这里不需要计算七对相关的番型

	if (iTotalFan > 0)
	{
		memcpy(cFinalFan, cTmpFanResult, sizeof(cTmpFanResult));
		memcpy(&TempColorResultType, &TempColorResultType1, sizeof(ResultTypeDef));

		ResultTypeDef TempColorResultType2;
		memcpy(&TempColorResultType2, &resultType, sizeof(ResultTypeDef));

		int iTempAllPai2[3][10] = { 0 };
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				iTempAllPai2[i][j] = allPai[i][j];
			}
		}

		bool bCanHu = true;
		for (int i = 0; i < 3; i++)
		{
			if (!Analyze2(iTempAllPai2[i], false, i, TempColorResultType2))
			{
				bCanHu = false;
				break;
			}
		}

		if (bCanHu)
		{
			TempColorResultType2.jiangType.iType = iType;
			TempColorResultType2.jiangType.iValue = iValue;

			char cTmpFanResult2[MJ_FAN_TYPE_CNT] = { 0 };				// 计算过程中使用的临时番型
			memset(cTmpFanResult2, 0, sizeof(cTmpFanResult2));
			cTmpFanResult2[MJ_PING_HU] = 1;
			cTmpFanResult[SU_HU] = -1;
			//_log(_DEBUG, "JudgeResult", "JudgeNormalHuWithoutJiang()22 iChiNum[%d], iPengNum[%d]", TempColorResultType2.iChiNum, TempColorResultType2.iPengNum);
			int iCurFan = JudgeMJFan(iTempAllPai2, TempColorResultType2, cTmpFanResult2);				// 算番，这里不需要计算七对相关的番型
			if (iCurFan > iTotalFan)
			{
				iTotalFan = iCurFan;
				memcpy(cFinalFan, cTmpFanResult2, sizeof(cTmpFanResult2));
				memcpy(&TempColorResultType, &TempColorResultType2, sizeof(ResultTypeDef));
			}
		}
	}

	if (iTotalFan > 0)
	{
		memcpy(cFanResult, cFinalFan, sizeof(cFinalFan));
		memcpy(&resultType, &TempColorResultType, sizeof(ResultTypeDef));
	}

	return iTotalFan;
}
