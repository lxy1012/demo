// RandKiss.h: interface for the CRandKiss class.

//

//////////////////////////////////////////////////////////////////////

#ifndef __RAND_KISS__

#define __RAND_KISS__

#include <vector>

using std::vector;



class CRandKiss  

{

public:

	CRandKiss();

	virtual ~CRandKiss();



	unsigned long x;

	unsigned long y;

	unsigned long z;

	unsigned long c;

	unsigned long RandKiss();

	void SrandKiss(unsigned int iSeed)

	{

		c = iSeed & 0x1FFFFFFF;

	}

	int  CheckRateFatalIndex(const vector<int>& vcRates); //在数组中找到命中的概率 命中单个元素的概率为 vcRates[i]/vcRates元素累积值

	int  CheckRateFatalIndex(int iAllRates[],int iNum);

public:

	static CRandKiss g_comRandKiss; //共用的RandKiss
};


#endif // __RAND_KISS__

