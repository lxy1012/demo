#include "GameConf.h"
#include "RandKiss.h"

GameConf* GameConf::g_GameConf = NULL;

GameConf* GameConf::GetInstance()
{
	if (g_GameConf == NULL)
	{
		g_GameConf = new GameConf();
	}
	return g_GameConf;
}

GameConf::GameConf()
{
	
}

GameConf::~ GameConf()
{

}

int GameConf::StringToIntAarry( char* szStrAarry, vector<int>& vcAarry,const char* szToken /*= ","*/)
{
	char szBuffer[256] = {0};
	strcpy(szBuffer,szStrAarry);

	if (szBuffer == NULL)
	{
		return 0;
	}
	vcAarry.clear();
	char *token = strtok(szBuffer, szToken);
	int iData = 0;
	while (token != NULL)
	{
		iData = atoi(token);
		vcAarry.push_back(iData);
		token = strtok(NULL, szToken);
	}
	return vcAarry.size();
}

int GameConf::StringToIntAarry(char* szStrAarry, int *iAarry,int iAarryLen,const char* szToken /*= ","*/) //将格式为1,2,3,19这样的字符串格式成整数数组
{
	char szBuffer[256] = {0};
	strcpy(szBuffer,szStrAarry);

	if (szBuffer == NULL)
	{
		return 0;
	}
	vector<int> vcAarry;
	vcAarry.clear();
	char *token = strtok(szBuffer, szToken);
	int iData = 0;
	while (token != NULL)
	{
		iData = atoi(token);
		vcAarry.push_back(iData);
		token = strtok(NULL, szToken);
	}
	if(vcAarry.size() > 0)
	{
		for(int i = 0; i < iAarryLen && i < vcAarry.size();i++)
		{
			iAarry[i] = vcAarry[i];
		}
	}
	else
	{
		return 0;
	}
	return (iAarryLen > vcAarry.size())? vcAarry.size():iAarryLen;
}

int GameConf::StringToCharAarry(char* szStrAarry, char *cAarry,int iAarryLen,const char* szToken /*= ","*/) //将格式为1,2,3,19这样的字符串格式成整数数组
{
	char szBuffer[256] = {0};
	strcpy(szBuffer,szStrAarry);

	if (szBuffer == NULL)
	{
		return 0;
	}
	vector<char> vcAarry;
	vcAarry.clear();
	char *token = strtok(szBuffer, szToken);
	int iData = 0;
	while (token != NULL)
	{
		iData = atoi(token);
		vcAarry.push_back(iData);
		token = strtok(NULL, szToken);
	}
	if(vcAarry.size() > 0)
	{
		for(int i = 0; i < iAarryLen && i < vcAarry.size();i++)
		{
			cAarry[i] = vcAarry[i];
		}
	}
	else
	{
		return 0;
	}
	return (iAarryLen > vcAarry.size())? vcAarry.size():iAarryLen;
}

//在数组中找到命中的概率 命中单个元素的概率为 vcRates[i]/vcRates元素累积值
int GameConf::CheckRateFatalIndex(const vector<int>& vcRates)
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
	int iRatePoint = CRandKiss::g_comRandKiss.RandKiss()%iRateAll;
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

int GameConf::CheckRateFatalIndex(int iAllRates[],int iNum)
{
	vector<int> vcRates;
	for (int i = 0;i < iNum;++i)
	{
		vcRates.push_back(iAllRates[i]);
	}
	return CheckRateFatalIndex(vcRates);
}


void GameConf::ReadGameConf( char* szFileName )
{
	_log(_NOTE,"GameConf","ReadDDZConf-----start");

	char szBuff[256] = {0};
	char szData[48] = {0};
	
	
	m_vcPeiPai.clear();

	for(int i = 0 ; i < 18 ; i++)
	{
		char szPath[32] = {0};
		sprintf(szPath,"iPeiPai_%d",i);
		vector<int> vcTemp;
		GetValueStr(szBuff,szPath,"peipai.conf","PeiPai","0");
		StringToIntAarry(szBuff,vcTemp);
		m_vcPeiPai.insert(m_vcPeiPai.end(),vcTemp.begin(),vcTemp.end());
	}
		
	GetValueInt(&m_iIfPeiPai,"if_open_peipai",szFileName,"if_peipai","0");
	memset(m_strBullInfo, 0, sizeof(m_strBullInfo));
	GetValueStr(m_strBullInfo, "g_strBullFormat", szFileName, "game_info", "0");
	_log(_NOTE, "HZXL", "GameConf::m_strBullInfo = %s", m_strBullInfo);
	GetValueInt(&m_iChangeCard, "if_change_card", szFileName, "change_card", 0);
	_log(_NOTE, "HZXL", "GameConf::if_change_card = %d", m_iChangeCard);
	GetValueInt(&m_iChangeOptimize, "if_change_optimize", szFileName, "change_optimize", 0);
	_log(_NOTE, "HZXL", "GameConf::if_change_optimize = %d", m_iChangeOptimize);
	GetValueInt(&m_iZhuangPos, "if_set_zhuangpos", szFileName, "set_zhuangpos", 0);
	_log(_NOTE, "HZXL", "GameConf::if_set_zhuangpos = %d", m_iZhuangPos);
	
}


time_t  GameConf::ChangeStringToData(char * szStr,string cflag /*= ""*/)
{
	if(strlen(szStr) == 0)
		return 0;
	struct tm tm1;  
    time_t time1;  
   
	char szFormot[128] = {0};

	sprintf(szFormot,"%%4d%s%%2d%s%%2d %%2d:%%2d:%%2d",cflag.c_str(),cflag.c_str(),cflag.c_str());
	_log(_DEBUG,"GameConf","ChangeStringToData  szFormot = %s",szFormot);

    int a = sscanf(szStr, szFormot,      
          &tm1.tm_year,   
          &tm1.tm_mon,   
          &tm1.tm_mday,   
          &tm1.tm_hour,   
          &tm1.tm_min,  
          &tm1.tm_sec);  
	_log(_DEBUG,"GameConf","ChangeStringToData  szStr = %s a = %d",szStr,a);
    if(a != 6)
		return 0;
    tm1.tm_year -= 1900;  
    tm1.tm_mon --;  
  
  
    tm1.tm_isdst=-1;  
    time1 = mktime(&tm1);  
	_log(_DEBUG,"GameConf","ChangeStringToData  time1 = %d",time1);
	return time1;
}