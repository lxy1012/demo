#ifndef __GAME_CONF_H__
#define __GAME_CONF_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iterator>
#include "conf.h"
#include "log.h"
#include "msg.h"

using namespace std;

#define MAX_BET_LEVEL  28  //押注分档最大数
class  GameConf
{
public:
	static GameConf* GetInstance();
	static int StringToIntAarry(char* szStrAarry, vector<int>& vcAarry,const char* szToken = ","); //将格式为1,2,3,19这样的字符串格式成整数数组
	static int StringToIntAarry(char* szStrAarry, int *iAarry,int iAarryLen,const char* szToken = ","); //将格式为1,2,3,19这样的字符串格式成整数数组
	static int StringToCharAarry(char* szStrAarry, char *cAarry,int iAarryLen,const char* szToken = ","); //将格式为1,2,3,19这样的字符串格式成char数组
	static int CheckRateFatalIndex(const vector<int>& vcRates); //在数组中找到命中的概率 命中单个元素的概率为 vcRates[i]/vcRates元素累积值
	static int CheckRateFatalIndex(int iAllRates[],int iNum);

	void ReadGameConf(char* szFileName); 	//读游戏配置
public:
	//-----游戏配置-----------
	static time_t  ChangeStringToData(char * szStr,string cflag = "-");

	//配牌测试用
	vector<int> m_vcPeiPai;		//配牌用的
	//是否开启配牌
	int m_iIfPeiPai;
	char m_strBullInfo[256];
	int m_iChangeCard;
	int m_iChangeOptimize;
	int m_iZhuangPos;
private:
	GameConf();
	~GameConf();
	
	static GameConf* g_GameConf;
};



#endif//__GAME_CONF_H__
