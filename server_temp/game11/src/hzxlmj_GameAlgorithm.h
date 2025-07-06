/****
intrudction: 红中血流麻将算法实现类
****/

#ifndef _HZXLMJ_GAME_ALGORITHM_H_
#define _HZXLMJ_GAME_ALGORITHM_H_

class hzxlmj_GameAlgorithm
{
public:
	hzxlmj_GameAlgorithm();
	~hzxlmj_GameAlgorithm();

public:
	void SortTotalResultByScore(int viTotalScore[], int viAwardNum[], int iResultSeatRank[], int iResultAwardNum[]);			// 总分排序，获得玩家排名					

private:
	void BubbleSort(int arr[], int len, int iIndex[]);									// 冒泡排序
};

#endif	// _HZXLMJ_GAME_ALGORITHM_H_