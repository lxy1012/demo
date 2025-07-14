// RandKiss.cpp: implementation of the CRandKiss class.
//
//////////////////////////////////////////////////////////////////////

#include "RandKiss.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRandKiss CRandKiss::g_comRandKiss;
CRandKiss::CRandKiss()
{
	x=123456789;
	y=362436;
	z=521288629;
	c=7654321;
}

CRandKiss::~CRandKiss()
{

}

unsigned long CRandKiss::RandKiss()
{
	/*_int64 t;
	_int64	a=698769069;

  //方法1：线性同余
  x=69069*x+12345;

  //方法2：移位异或
  y^=(y<<13); y^=(y>>17); y^=(y<<5);

  //方法3：带进位的乘法
  t=a*z+c; c=(t>>32);
  return x+y+(z=t);*/
  unsigned long long  t;
	unsigned long long 	a=698769069;

  //方法1：线性同余
  x=69069*x+12345;

  //方法2：移位异或
  y^=(y<<13); y^=(y>>17); y^=(y<<5);

  //方法3：带进位的乘法
  t=a*z+c; c=(t>>32);
  return x+y+(z=t);

}

//在数组中找到命中的概率 命中单个元素的概率为 vcRates[i]/vcRates元素累积值
int CRandKiss::CheckRateFatalIndex(const vector<int>& vcRates)
{
	int iRateAll = 0;
	for (int i = 0;i < vcRates.size();++i)
	{
		iRateAll += vcRates[i];
	}
	if (iRateAll <= 0)
	{
		return 0;
	}
	int iRatePoint = RandKiss()%iRateAll;
	int iRateCelBegin = 0;
	int iRateCelEnd = 0;
	for (int i = 0;i < vcRates.size();++i)
	{
		iRateCelEnd = iRateCelBegin + vcRates[i];
		if (iRatePoint >= iRateCelBegin && iRatePoint < iRateCelEnd)
		{
			return i;
		}
		iRateCelBegin = iRateCelEnd;
	}
	return 0;
}

int CRandKiss::CheckRateFatalIndex(int iAllRates[],int iNum)
{
	vector<int> vcRates;
	for (int i = 0;i < iNum;++i)
	{
		vcRates.push_back(iAllRates[i]);
	}
	return CheckRateFatalIndex(vcRates);
}