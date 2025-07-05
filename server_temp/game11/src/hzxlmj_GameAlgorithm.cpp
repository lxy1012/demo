
#include "hzxlmj_GameAlgorithm.h"

hzxlmj_GameAlgorithm::hzxlmj_GameAlgorithm()
{
}

hzxlmj_GameAlgorithm::~hzxlmj_GameAlgorithm()
{
}

// 冒泡排序
void hzxlmj_GameAlgorithm::BubbleSort(int arr[], int len, int iIndex[])
{
	int temp;
	int iTmpIndex;
	int i, j;
	for (i = 0; i<len; i++)				/* 外循环为排序趟数，len个数进行len-1趟 */
		for (j = 0; j<len - 1 - i; j++)		/* 内循环为每趟比较的次数，第i趟比较len-i次 */
		{
			if (arr[j] < arr[j + 1])		/* 相邻元素比较，若逆序则交换（升序为左大于右，降序反之） */
			{
				temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;

				iTmpIndex = iIndex[j];
				iIndex[j] = iIndex[j + 1];
				iIndex[j + 1] = iTmpIndex;
			}
		}
}

// 总分排序，获得玩家排名及奖励的元宝数量
// iResultSeatRank:0-3四个玩家最终排名:第0名~第3名
// iResultAwardNum:0-3玩家最终奖励的元宝数量
void hzxlmj_GameAlgorithm::SortTotalResultByScore(int viTotalScore[], int viAwardNum[], int iResultSeatRank[], int iResultAwardNum[])
{
	int iIndex[4] = { 0,1,2,3 };
	BubbleSort(viTotalScore, 4, iIndex);

	for (int i = 0; i < 4; i++)
		iResultSeatRank[iIndex[i]] = i;

	for (int i = 0; i < 4; i++)
		iResultAwardNum[iIndex[i]] = viAwardNum[i];

	int iPos = 1;
	while (iPos <= 3)
	{
		if (viTotalScore[iPos] == viTotalScore[iPos - 1])
		{
			if (iResultAwardNum[iIndex[iPos]] < iResultAwardNum[iIndex[iPos - 1]])
			{
				iResultAwardNum[iIndex[iPos]] = iResultAwardNum[iIndex[iPos - 1]];
				iResultSeatRank[iIndex[iPos]] = iResultSeatRank[iIndex[iPos - 1]];
			}
		}
		iPos++;
	}
}

